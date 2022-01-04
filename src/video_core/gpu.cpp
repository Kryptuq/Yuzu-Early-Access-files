// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>

#include "common/assert.h"
#include "common/microprofile.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hardware_interrupt_manager.h"
#include "core/hle/service/nvdrv/nvdata.h"
#include "core/hle/service/nvflinger/buffer_queue.h"
#include "core/perf_stats.h"
#include "video_core/cdma_pusher.h"
#include "video_core/dma_pusher.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/gpu.h"
#include "video_core/gpu_thread.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_base.h"
#include "video_core/shader_notify.h"

namespace Tegra {

MICROPROFILE_DEFINE(GPU_wait, "GPU", "Wait for the GPU", MP_RGB(128, 128, 192));

struct GPU::Impl {
    explicit Impl(GPU& gpu_, Core::System& system_, bool is_async_, bool use_nvdec_)
        : gpu{gpu_}, system{system_}, memory_manager{std::make_unique<Tegra::MemoryManager>(
                                          system)},
          dma_pusher{std::make_unique<Tegra::DmaPusher>(system, gpu)}, use_nvdec{use_nvdec_},
          maxwell_3d{std::make_unique<Engines::Maxwell3D>(system, *memory_manager)},
          fermi_2d{std::make_unique<Engines::Fermi2D>()},
          kepler_compute{std::make_unique<Engines::KeplerCompute>(system, *memory_manager)},
          maxwell_dma{std::make_unique<Engines::MaxwellDMA>(system, *memory_manager)},
          kepler_memory{std::make_unique<Engines::KeplerMemory>(system, *memory_manager)},
          shader_notify{std::make_unique<VideoCore::ShaderNotify>()}, is_async{is_async_},
          gpu_thread{system_, is_async_} {}

    ~Impl() = default;

    /// Binds a renderer to the GPU.
    void BindRenderer(std::unique_ptr<VideoCore::RendererBase> renderer_) {
        renderer = std::move(renderer_);
        rasterizer = renderer->ReadRasterizer();

        memory_manager->BindRasterizer(rasterizer);
        maxwell_3d->BindRasterizer(rasterizer);
        fermi_2d->BindRasterizer(rasterizer);
        kepler_compute->BindRasterizer(rasterizer);
        maxwell_dma->BindRasterizer(rasterizer);
    }

    /// Calls a GPU method.
    void CallMethod(const GPU::MethodCall& method_call) {
        LOG_TRACE(HW_GPU, "Processing method {:08X} on subchannel {}", method_call.method,
                  method_call.subchannel);

        ASSERT(method_call.subchannel < bound_engines.size());

        if (ExecuteMethodOnEngine(method_call.method)) {
            CallEngineMethod(method_call);
        } else {
            CallPullerMethod(method_call);
        }
    }

    /// Calls a GPU multivalue method.
    void CallMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                         u32 methods_pending) {
        LOG_TRACE(HW_GPU, "Processing method {:08X} on subchannel {}", method, subchannel);

        ASSERT(subchannel < bound_engines.size());

        if (ExecuteMethodOnEngine(method)) {
            CallEngineMultiMethod(method, subchannel, base_start, amount, methods_pending);
        } else {
            for (std::size_t i = 0; i < amount; i++) {
                CallPullerMethod(GPU::MethodCall{
                    method,
                    base_start[i],
                    subchannel,
                    methods_pending - static_cast<u32>(i),
                });
            }
        }
    }

    /// Flush all current written commands into the host GPU for execution.
    void FlushCommands() {
        rasterizer->FlushCommands();
    }

    /// Synchronizes CPU writes with Host GPU memory.
    void SyncGuestHost() {
        rasterizer->SyncGuestHost();
    }

    /// Signal the ending of command list.
    void OnCommandListEnd() {
        if (is_async) {
            // This command only applies to asynchronous GPU mode
            gpu_thread.OnCommandListEnd();
        }
    }

    /// Request a host GPU memory flush from the CPU.
    [[nodiscard]] u64 RequestFlush(VAddr addr, std::size_t size) {
        std::unique_lock lck{flush_request_mutex};
        const u64 fence = ++last_flush_fence;
        flush_requests.emplace_back(fence, addr, size);
        return fence;
    }

    /// Obtains current flush request fence id.
    [[nodiscard]] u64 CurrentFlushRequestFence() const {
        return current_flush_fence.load(std::memory_order_relaxed);
    }

    /// Tick pending requests within the GPU.
    void TickWork() {
        std::unique_lock lck{flush_request_mutex};
        while (!flush_requests.empty()) {
            auto& request = flush_requests.front();
            const u64 fence = request.fence;
            const VAddr addr = request.addr;
            const std::size_t size = request.size;
            flush_requests.pop_front();
            flush_request_mutex.unlock();
            rasterizer->FlushRegion(addr, size);
            current_flush_fence.store(fence);
            flush_request_mutex.lock();
        }
    }

    /// Returns a reference to the Maxwell3D GPU engine.
    [[nodiscard]] Engines::Maxwell3D& Maxwell3D() {
        return *maxwell_3d;
    }

    /// Returns a const reference to the Maxwell3D GPU engine.
    [[nodiscard]] const Engines::Maxwell3D& Maxwell3D() const {
        return *maxwell_3d;
    }

    /// Returns a reference to the KeplerCompute GPU engine.
    [[nodiscard]] Engines::KeplerCompute& KeplerCompute() {
        return *kepler_compute;
    }

    /// Returns a reference to the KeplerCompute GPU engine.
    [[nodiscard]] const Engines::KeplerCompute& KeplerCompute() const {
        return *kepler_compute;
    }

    /// Returns a reference to the GPU memory manager.
    [[nodiscard]] Tegra::MemoryManager& MemoryManager() {
        return *memory_manager;
    }

    /// Returns a const reference to the GPU memory manager.
    [[nodiscard]] const Tegra::MemoryManager& MemoryManager() const {
        return *memory_manager;
    }

    /// Returns a reference to the GPU DMA pusher.
    [[nodiscard]] Tegra::DmaPusher& DmaPusher() {
        return *dma_pusher;
    }

    /// Returns a const reference to the GPU DMA pusher.
    [[nodiscard]] const Tegra::DmaPusher& DmaPusher() const {
        return *dma_pusher;
    }

    /// Returns a reference to the underlying renderer.
    [[nodiscard]] VideoCore::RendererBase& Renderer() {
        return *renderer;
    }

    /// Returns a const reference to the underlying renderer.
    [[nodiscard]] const VideoCore::RendererBase& Renderer() const {
        return *renderer;
    }

    /// Returns a reference to the shader notifier.
    [[nodiscard]] VideoCore::ShaderNotify& ShaderNotify() {
        return *shader_notify;
    }

    /// Returns a const reference to the shader notifier.
    [[nodiscard]] const VideoCore::ShaderNotify& ShaderNotify() const {
        return *shader_notify;
    }

    /// Allows the CPU/NvFlinger to wait on the GPU before presenting a frame.
    void WaitFence(u32 syncpoint_id, u32 value, std::stop_token stop_token = {}) {
        // Synced GPU, is always in sync
        if (!is_async) {
            return;
        }
        if (syncpoint_id == UINT32_MAX) {
            // TODO: Research what this does.
            LOG_ERROR(HW_GPU, "Waiting for syncpoint -1 not implemented");
            return;
        }
        MICROPROFILE_SCOPE(GPU_wait);
        std::unique_lock lock{sync_mutex};
        sync_cv.wait(lock, stop_token,
                     [=, this] { return syncpoints.at(syncpoint_id).load() >= value; });
    }

    void IncrementSyncPoint(u32 syncpoint_id) {
        auto& syncpoint = syncpoints.at(syncpoint_id);
        syncpoint++;
        std::lock_guard lock{sync_mutex};
        sync_cv.notify_all();
        auto& interrupt = syncpt_interrupts.at(syncpoint_id);
        if (!interrupt.empty()) {
            u32 value = syncpoint.load();
            auto it = interrupt.begin();
            while (it != interrupt.end()) {
                if (value >= *it) {
                    TriggerCpuInterrupt(syncpoint_id, *it);
                    it = interrupt.erase(it);
                    continue;
                }
                it++;
            }
        }
    }

    [[nodiscard]] u32 GetSyncpointValue(u32 syncpoint_id) const {
        return syncpoints.at(syncpoint_id).load();
    }

    void RegisterSyncptInterrupt(u32 syncpoint_id, u32 value) {
        std::lock_guard lock{sync_mutex};
        auto& interrupt = syncpt_interrupts.at(syncpoint_id);
        bool contains = std::any_of(interrupt.begin(), interrupt.end(),
                                    [value](u32 in_value) { return in_value == value; });
        if (contains) {
            return;
        }
        interrupt.emplace_back(value);
    }

    [[nodiscard]] bool CancelSyncptInterrupt(u32 syncpoint_id, u32 value) {
        std::lock_guard lock{sync_mutex};
        auto& interrupt = syncpt_interrupts.at(syncpoint_id);
        const auto iter =
            std::find_if(interrupt.begin(), interrupt.end(),
                         [value](u32 interrupt_value) { return value == interrupt_value; });

        if (iter == interrupt.end()) {
            return false;
        }
        interrupt.erase(iter);
        return true;
    }

    [[nodiscard]] u64 GetTicks() const {
        // This values were reversed engineered by fincs from NVN
        // The gpu clock is reported in units of 385/625 nanoseconds
        constexpr u64 gpu_ticks_num = 384;
        constexpr u64 gpu_ticks_den = 625;

        u64 nanoseconds = system.CoreTiming().GetGlobalTimeNs().count();
        if (Settings::values.use_fast_gpu_time.GetValue()) {
            nanoseconds /= 256;
        }
        const u64 nanoseconds_num = nanoseconds / gpu_ticks_den;
        const u64 nanoseconds_rem = nanoseconds % gpu_ticks_den;
        return nanoseconds_num * gpu_ticks_num + (nanoseconds_rem * gpu_ticks_num) / gpu_ticks_den;
    }

    [[nodiscard]] bool IsAsync() const {
        return is_async;
    }

    [[nodiscard]] bool UseNvdec() const {
        return use_nvdec;
    }

    void RendererFrameEndNotify() {
        system.GetPerfStats().EndGameFrame();
    }

    /// Performs any additional setup necessary in order to begin GPU emulation.
    /// This can be used to launch any necessary threads and register any necessary
    /// core timing events.
    void Start() {
        gpu_thread.StartThread(*renderer, renderer->Context(), *dma_pusher);
        cpu_context = renderer->GetRenderWindow().CreateSharedContext();
        cpu_context->MakeCurrent();
    }

    /// Obtain the CPU Context
    void ObtainContext() {
        cpu_context->MakeCurrent();
    }

    /// Release the CPU Context
    void ReleaseContext() {
        cpu_context->DoneCurrent();
    }

    /// Push GPU command entries to be processed
    void PushGPUEntries(Tegra::CommandList&& entries) {
        gpu_thread.SubmitList(std::move(entries));
    }

    /// Push GPU command buffer entries to be processed
    void PushCommandBuffer(u32 id, Tegra::ChCommandHeaderList& entries) {
        if (!use_nvdec) {
            return;
        }

        if (!cdma_pushers.contains(id)) {
            cdma_pushers.insert_or_assign(id, std::make_unique<Tegra::CDmaPusher>(gpu));
        }

        // SubmitCommandBuffer would make the nvdec operations async, this is not currently working
        // TODO(ameerj): RE proper async nvdec operation
        // gpu_thread.SubmitCommandBuffer(std::move(entries));
        cdma_pushers[id]->ProcessEntries(std::move(entries));
    }

    /// Frees the CDMAPusher instance to free up resources
    void ClearCdmaInstance(u32 id) {
        const auto iter = cdma_pushers.find(id);
        if (iter != cdma_pushers.end()) {
            cdma_pushers.erase(iter);
        }
    }

    /// Swap buffers (render frame)
    void SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
        gpu_thread.SwapBuffers(framebuffer);
    }

    /// Notify rasterizer that any caches of the specified region should be flushed to Switch memory
    void FlushRegion(VAddr addr, u64 size) {
        gpu_thread.FlushRegion(addr, size);
    }

    /// Notify rasterizer that any caches of the specified region should be invalidated
    void InvalidateRegion(VAddr addr, u64 size) {
        gpu_thread.InvalidateRegion(addr, size);
    }

    /// Notify rasterizer that any caches of the specified region should be flushed and invalidated
    void FlushAndInvalidateRegion(VAddr addr, u64 size) {
        gpu_thread.FlushAndInvalidateRegion(addr, size);
    }

    void TriggerCpuInterrupt(u32 syncpoint_id, u32 value) const {
        auto& interrupt_manager = system.InterruptManager();
        interrupt_manager.GPUInterruptSyncpt(syncpoint_id, value);
    }

    void ProcessBindMethod(const GPU::MethodCall& method_call) {
        // Bind the current subchannel to the desired engine id.
        LOG_DEBUG(HW_GPU, "Binding subchannel {} to engine {}", method_call.subchannel,
                  method_call.argument);
        const auto engine_id = static_cast<EngineID>(method_call.argument);
        bound_engines[method_call.subchannel] = static_cast<EngineID>(engine_id);
        switch (engine_id) {
        case EngineID::FERMI_TWOD_A:
            dma_pusher->BindSubchannel(fermi_2d.get(), method_call.subchannel);
            break;
        case EngineID::MAXWELL_B:
            dma_pusher->BindSubchannel(maxwell_3d.get(), method_call.subchannel);
            break;
        case EngineID::KEPLER_COMPUTE_B:
            dma_pusher->BindSubchannel(kepler_compute.get(), method_call.subchannel);
            break;
        case EngineID::MAXWELL_DMA_COPY_A:
            dma_pusher->BindSubchannel(maxwell_dma.get(), method_call.subchannel);
            break;
        case EngineID::KEPLER_INLINE_TO_MEMORY_B:
            dma_pusher->BindSubchannel(kepler_memory.get(), method_call.subchannel);
            break;
        default:
            UNIMPLEMENTED_MSG("Unimplemented engine {:04X}", engine_id);
        }
    }

    void ProcessFenceActionMethod() {
        switch (regs.fence_action.op) {
        case GPU::FenceOperation::Acquire:
            WaitFence(regs.fence_action.syncpoint_id, regs.fence_value);
            break;
        case GPU::FenceOperation::Increment:
            IncrementSyncPoint(regs.fence_action.syncpoint_id);
            break;
        default:
            UNIMPLEMENTED_MSG("Unimplemented operation {}", regs.fence_action.op.Value());
        }
    }

    void ProcessWaitForInterruptMethod() {
        // TODO(bunnei) ImplementMe
        LOG_WARNING(HW_GPU, "(STUBBED) called");
    }

    void ProcessSemaphoreTriggerMethod() {
        const auto semaphoreOperationMask = 0xF;
        const auto op =
            static_cast<GpuSemaphoreOperation>(regs.semaphore_trigger & semaphoreOperationMask);
        if (op == GpuSemaphoreOperation::WriteLong) {
            struct Block {
                u32 sequence;
                u32 zeros = 0;
                u64 timestamp;
            };

            Block block{};
            block.sequence = regs.semaphore_sequence;
            // TODO(Kmather73): Generate a real GPU timestamp and write it here instead of
            // CoreTiming
            block.timestamp = GetTicks();
            memory_manager->WriteBlock(regs.semaphore_address.SemaphoreAddress(), &block,
                                       sizeof(block));
        } else {
            const u32 word{memory_manager->Read<u32>(regs.semaphore_address.SemaphoreAddress())};
            if ((op == GpuSemaphoreOperation::AcquireEqual && word == regs.semaphore_sequence) ||
                (op == GpuSemaphoreOperation::AcquireGequal &&
                 static_cast<s32>(word - regs.semaphore_sequence) > 0) ||
                (op == GpuSemaphoreOperation::AcquireMask && (word & regs.semaphore_sequence))) {
                // Nothing to do in this case
            } else {
                regs.acquire_source = true;
                regs.acquire_value = regs.semaphore_sequence;
                if (op == GpuSemaphoreOperation::AcquireEqual) {
                    regs.acquire_active = true;
                    regs.acquire_mode = false;
                } else if (op == GpuSemaphoreOperation::AcquireGequal) {
                    regs.acquire_active = true;
                    regs.acquire_mode = true;
                } else if (op == GpuSemaphoreOperation::AcquireMask) {
                    // TODO(kemathe) The acquire mask operation waits for a value that, ANDed with
                    // semaphore_sequence, gives a non-0 result
                    LOG_ERROR(HW_GPU, "Invalid semaphore operation AcquireMask not implemented");
                } else {
                    LOG_ERROR(HW_GPU, "Invalid semaphore operation");
                }
            }
        }
    }

    void ProcessSemaphoreRelease() {
        memory_manager->Write<u32>(regs.semaphore_address.SemaphoreAddress(),
                                   regs.semaphore_release);
    }

    void ProcessSemaphoreAcquire() {
        const u32 word = memory_manager->Read<u32>(regs.semaphore_address.SemaphoreAddress());
        const auto value = regs.semaphore_acquire;
        if (word != value) {
            regs.acquire_active = true;
            regs.acquire_value = value;
            // TODO(kemathe73) figure out how to do the acquire_timeout
            regs.acquire_mode = false;
            regs.acquire_source = false;
        }
    }

    /// Calls a GPU puller method.
    void CallPullerMethod(const GPU::MethodCall& method_call) {
        regs.reg_array[method_call.method] = method_call.argument;
        const auto method = static_cast<BufferMethods>(method_call.method);

        switch (method) {
        case BufferMethods::BindObject: {
            ProcessBindMethod(method_call);
            break;
        }
        case BufferMethods::Nop:
        case BufferMethods::SemaphoreAddressHigh:
        case BufferMethods::SemaphoreAddressLow:
        case BufferMethods::SemaphoreSequence:
        case BufferMethods::UnkCacheFlush:
        case BufferMethods::WrcacheFlush:
        case BufferMethods::FenceValue:
            break;
        case BufferMethods::RefCnt:
            rasterizer->SignalReference();
            break;
        case BufferMethods::FenceAction:
            ProcessFenceActionMethod();
            break;
        case BufferMethods::WaitForInterrupt:
            ProcessWaitForInterruptMethod();
            break;
        case BufferMethods::SemaphoreTrigger: {
            ProcessSemaphoreTriggerMethod();
            break;
        }
        case BufferMethods::NotifyIntr: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method NotifyIntr not implemented");
            break;
        }
        case BufferMethods::Unk28: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method Unk28 not implemented");
            break;
        }
        case BufferMethods::SemaphoreAcquire: {
            ProcessSemaphoreAcquire();
            break;
        }
        case BufferMethods::SemaphoreRelease: {
            ProcessSemaphoreRelease();
            break;
        }
        case BufferMethods::Yield: {
            // TODO(Kmather73): Research and implement this method.
            LOG_ERROR(HW_GPU, "Special puller engine method Yield not implemented");
            break;
        }
        default:
            LOG_ERROR(HW_GPU, "Special puller engine method {:X} not implemented", method);
            break;
        }
    }

    /// Calls a GPU engine method.
    void CallEngineMethod(const GPU::MethodCall& method_call) {
        const EngineID engine = bound_engines[method_call.subchannel];

        switch (engine) {
        case EngineID::FERMI_TWOD_A:
            fermi_2d->CallMethod(method_call.method, method_call.argument,
                                 method_call.IsLastCall());
            break;
        case EngineID::MAXWELL_B:
            maxwell_3d->CallMethod(method_call.method, method_call.argument,
                                   method_call.IsLastCall());
            break;
        case EngineID::KEPLER_COMPUTE_B:
            kepler_compute->CallMethod(method_call.method, method_call.argument,
                                       method_call.IsLastCall());
            break;
        case EngineID::MAXWELL_DMA_COPY_A:
            maxwell_dma->CallMethod(method_call.method, method_call.argument,
                                    method_call.IsLastCall());
            break;
        case EngineID::KEPLER_INLINE_TO_MEMORY_B:
            kepler_memory->CallMethod(method_call.method, method_call.argument,
                                      method_call.IsLastCall());
            break;
        default:
            UNIMPLEMENTED_MSG("Unimplemented engine");
        }
    }

    /// Calls a GPU engine multivalue method.
    void CallEngineMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                               u32 methods_pending) {
        const EngineID engine = bound_engines[subchannel];

        switch (engine) {
        case EngineID::FERMI_TWOD_A:
            fermi_2d->CallMultiMethod(method, base_start, amount, methods_pending);
            break;
        case EngineID::MAXWELL_B:
            maxwell_3d->CallMultiMethod(method, base_start, amount, methods_pending);
            break;
        case EngineID::KEPLER_COMPUTE_B:
            kepler_compute->CallMultiMethod(method, base_start, amount, methods_pending);
            break;
        case EngineID::MAXWELL_DMA_COPY_A:
            maxwell_dma->CallMultiMethod(method, base_start, amount, methods_pending);
            break;
        case EngineID::KEPLER_INLINE_TO_MEMORY_B:
            kepler_memory->CallMultiMethod(method, base_start, amount, methods_pending);
            break;
        default:
            UNIMPLEMENTED_MSG("Unimplemented engine");
        }
    }

    /// Determines where the method should be executed.
    [[nodiscard]] bool ExecuteMethodOnEngine(u32 method) {
        const auto buffer_method = static_cast<BufferMethods>(method);
        return buffer_method >= BufferMethods::NonPullerMethods;
    }

    struct Regs {
        static constexpr size_t NUM_REGS = 0x40;

        union {
            struct {
                INSERT_PADDING_WORDS_NOINIT(0x4);
                struct {
                    u32 address_high;
                    u32 address_low;

                    [[nodiscard]] GPUVAddr SemaphoreAddress() const {
                        return static_cast<GPUVAddr>((static_cast<GPUVAddr>(address_high) << 32) |
                                                     address_low);
                    }
                } semaphore_address;

                u32 semaphore_sequence;
                u32 semaphore_trigger;
                INSERT_PADDING_WORDS_NOINIT(0xC);

                // The pusher and the puller share the reference counter, the pusher only has read
                // access
                u32 reference_count;
                INSERT_PADDING_WORDS_NOINIT(0x5);

                u32 semaphore_acquire;
                u32 semaphore_release;
                u32 fence_value;
                GPU::FenceAction fence_action;
                INSERT_PADDING_WORDS_NOINIT(0xE2);

                // Puller state
                u32 acquire_mode;
                u32 acquire_source;
                u32 acquire_active;
                u32 acquire_timeout;
                u32 acquire_value;
            };
            std::array<u32, NUM_REGS> reg_array;
        };
    } regs{};

    GPU& gpu;
    Core::System& system;
    std::unique_ptr<Tegra::MemoryManager> memory_manager;
    std::unique_ptr<Tegra::DmaPusher> dma_pusher;
    std::map<u32, std::unique_ptr<Tegra::CDmaPusher>> cdma_pushers;
    std::unique_ptr<VideoCore::RendererBase> renderer;
    VideoCore::RasterizerInterface* rasterizer = nullptr;
    const bool use_nvdec;

    /// Mapping of command subchannels to their bound engine ids
    std::array<EngineID, 8> bound_engines{};
    /// 3D engine
    std::unique_ptr<Engines::Maxwell3D> maxwell_3d;
    /// 2D engine
    std::unique_ptr<Engines::Fermi2D> fermi_2d;
    /// Compute engine
    std::unique_ptr<Engines::KeplerCompute> kepler_compute;
    /// DMA engine
    std::unique_ptr<Engines::MaxwellDMA> maxwell_dma;
    /// Inline memory engine
    std::unique_ptr<Engines::KeplerMemory> kepler_memory;
    /// Shader build notifier
    std::unique_ptr<VideoCore::ShaderNotify> shader_notify;

    std::array<std::atomic<u32>, Service::Nvidia::MaxSyncPoints> syncpoints{};

    std::array<std::list<u32>, Service::Nvidia::MaxSyncPoints> syncpt_interrupts;

    std::mutex sync_mutex;
    std::mutex device_mutex;

    std::condition_variable_any sync_cv;

    struct FlushRequest {
        explicit FlushRequest(u64 fence_, VAddr addr_, std::size_t size_)
            : fence{fence_}, addr{addr_}, size{size_} {}
        u64 fence;
        VAddr addr;
        std::size_t size;
    };

    std::list<FlushRequest> flush_requests;
    std::atomic<u64> current_flush_fence{};
    u64 last_flush_fence{};
    std::mutex flush_request_mutex;

    const bool is_async;

    VideoCommon::GPUThread::ThreadManager gpu_thread;
    std::unique_ptr<Core::Frontend::GraphicsContext> cpu_context;

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Regs, field_name) == position * 4,                                      \
                  "Field " #field_name " has invalid position")

    ASSERT_REG_POSITION(semaphore_address, 0x4);
    ASSERT_REG_POSITION(semaphore_sequence, 0x6);
    ASSERT_REG_POSITION(semaphore_trigger, 0x7);
    ASSERT_REG_POSITION(reference_count, 0x14);
    ASSERT_REG_POSITION(semaphore_acquire, 0x1A);
    ASSERT_REG_POSITION(semaphore_release, 0x1B);
    ASSERT_REG_POSITION(fence_value, 0x1C);
    ASSERT_REG_POSITION(fence_action, 0x1D);

    ASSERT_REG_POSITION(acquire_mode, 0x100);
    ASSERT_REG_POSITION(acquire_source, 0x101);
    ASSERT_REG_POSITION(acquire_active, 0x102);
    ASSERT_REG_POSITION(acquire_timeout, 0x103);
    ASSERT_REG_POSITION(acquire_value, 0x104);

#undef ASSERT_REG_POSITION

    enum class GpuSemaphoreOperation {
        AcquireEqual = 0x1,
        WriteLong = 0x2,
        AcquireGequal = 0x4,
        AcquireMask = 0x8,
    };
};

GPU::GPU(Core::System& system, bool is_async, bool use_nvdec)
    : impl{std::make_unique<Impl>(*this, system, is_async, use_nvdec)} {}

GPU::~GPU() = default;

void GPU::BindRenderer(std::unique_ptr<VideoCore::RendererBase> renderer) {
    impl->BindRenderer(std::move(renderer));
}

void GPU::CallMethod(const MethodCall& method_call) {
    impl->CallMethod(method_call);
}

void GPU::CallMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                          u32 methods_pending) {
    impl->CallMultiMethod(method, subchannel, base_start, amount, methods_pending);
}

void GPU::FlushCommands() {
    impl->FlushCommands();
}

void GPU::SyncGuestHost() {
    impl->SyncGuestHost();
}

void GPU::OnCommandListEnd() {
    impl->OnCommandListEnd();
}

u64 GPU::RequestFlush(VAddr addr, std::size_t size) {
    return impl->RequestFlush(addr, size);
}

u64 GPU::CurrentFlushRequestFence() const {
    return impl->CurrentFlushRequestFence();
}

void GPU::TickWork() {
    impl->TickWork();
}

Engines::Maxwell3D& GPU::Maxwell3D() {
    return impl->Maxwell3D();
}

const Engines::Maxwell3D& GPU::Maxwell3D() const {
    return impl->Maxwell3D();
}

Engines::KeplerCompute& GPU::KeplerCompute() {
    return impl->KeplerCompute();
}

const Engines::KeplerCompute& GPU::KeplerCompute() const {
    return impl->KeplerCompute();
}

Tegra::MemoryManager& GPU::MemoryManager() {
    return impl->MemoryManager();
}

const Tegra::MemoryManager& GPU::MemoryManager() const {
    return impl->MemoryManager();
}

Tegra::DmaPusher& GPU::DmaPusher() {
    return impl->DmaPusher();
}

const Tegra::DmaPusher& GPU::DmaPusher() const {
    return impl->DmaPusher();
}

VideoCore::RendererBase& GPU::Renderer() {
    return impl->Renderer();
}

const VideoCore::RendererBase& GPU::Renderer() const {
    return impl->Renderer();
}

VideoCore::ShaderNotify& GPU::ShaderNotify() {
    return impl->ShaderNotify();
}

const VideoCore::ShaderNotify& GPU::ShaderNotify() const {
    return impl->ShaderNotify();
}

void GPU::WaitFence(u32 syncpoint_id, u32 value, std::stop_token stop_token) {
    impl->WaitFence(syncpoint_id, value, stop_token);
}

void GPU::IncrementSyncPoint(u32 syncpoint_id) {
    impl->IncrementSyncPoint(syncpoint_id);
}

u32 GPU::GetSyncpointValue(u32 syncpoint_id) const {
    return impl->GetSyncpointValue(syncpoint_id);
}

void GPU::RegisterSyncptInterrupt(u32 syncpoint_id, u32 value) {
    impl->RegisterSyncptInterrupt(syncpoint_id, value);
}

bool GPU::CancelSyncptInterrupt(u32 syncpoint_id, u32 value) {
    return impl->CancelSyncptInterrupt(syncpoint_id, value);
}

u64 GPU::GetTicks() const {
    return impl->GetTicks();
}

bool GPU::IsAsync() const {
    return impl->IsAsync();
}

bool GPU::UseNvdec() const {
    return impl->UseNvdec();
}

void GPU::RendererFrameEndNotify() {
    impl->RendererFrameEndNotify();
}

void GPU::Start() {
    impl->Start();
}

void GPU::ObtainContext() {
    impl->ObtainContext();
}

void GPU::ReleaseContext() {
    impl->ReleaseContext();
}

void GPU::PushGPUEntries(Tegra::CommandList&& entries) {
    impl->PushGPUEntries(std::move(entries));
}

void GPU::PushCommandBuffer(u32 id, Tegra::ChCommandHeaderList& entries) {
    impl->PushCommandBuffer(id, entries);
}

void GPU::ClearCdmaInstance(u32 id) {
    impl->ClearCdmaInstance(id);
}

void GPU::SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
    impl->SwapBuffers(framebuffer);
}

void GPU::FlushRegion(VAddr addr, u64 size) {
    impl->FlushRegion(addr, size);
}

void GPU::InvalidateRegion(VAddr addr, u64 size) {
    impl->InvalidateRegion(addr, size);
}

void GPU::FlushAndInvalidateRegion(VAddr addr, u64 size) {
    impl->FlushAndInvalidateRegion(addr, size);
}

} // namespace Tegra
