// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <bitset>
#include <ctime>
#include <memory>
#include <random>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/program_metadata.h"
#include "core/hle/kernel/code_set.h"
#include "core/hle/kernel/k_memory_block_manager.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_shared_memory_info.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {
namespace {
/**
 * Sets up the primary application thread
 *
 * @param system The system instance to create the main thread under.
 * @param owner_process The parent process for the main thread
 * @param priority The priority to give the main thread
 */
void SetupMainThread(Core::System& system, KProcess& owner_process, u32 priority, VAddr stack_top) {
    const VAddr entry_point = owner_process.PageTable().GetCodeRegionStart();
    ASSERT(owner_process.GetResourceLimit()->Reserve(LimitableResource::Threads, 1));

    KThread* thread = KThread::Create(system.Kernel());
    SCOPE_EXIT({ thread->Close(); });

    ASSERT(KThread::InitializeUserThread(system, thread, entry_point, 0, stack_top, priority,
                                         owner_process.GetIdealCoreId(), &owner_process)
               .IsSuccess());

    // Register 1 must be a handle to the main thread
    Handle thread_handle{};
    owner_process.GetHandleTable().Add(&thread_handle, thread);

    thread->SetName("main");
    thread->GetContext32().cpu_registers[0] = 0;
    thread->GetContext64().cpu_registers[0] = 0;
    thread->GetContext32().cpu_registers[1] = thread_handle;
    thread->GetContext64().cpu_registers[1] = thread_handle;

    if (system.DebuggerEnabled()) {
        thread->RequestSuspend(SuspendType::Debug);
    }

    // Run our thread.
    void(thread->Run());
}
} // Anonymous namespace

Result KProcess::Initialize(KProcess* process, Core::System& system, std::string process_name,
                            ProcessType type, KResourceLimit* res_limit) {
    auto& kernel = system.Kernel();

    process->name = std::move(process_name);
    process->resource_limit = res_limit;
    process->status = ProcessStatus::Created;
    process->program_id = 0;
    process->process_id = type == ProcessType::KernelInternal ? kernel.CreateNewKernelProcessID()
                                                              : kernel.CreateNewUserProcessID();
    process->capabilities.InitializeForMetadatalessProcess();
    process->is_initialized = true;

    std::mt19937 rng(Settings::values.rng_seed.GetValue().value_or(std::time(nullptr)));
    std::uniform_int_distribution<u64> distribution;
    std::generate(process->random_entropy.begin(), process->random_entropy.end(),
                  [&] { return distribution(rng); });

    kernel.AppendNewProcess(process);

    // Clear remaining fields.
    process->num_running_threads = 0;
    process->is_signaled = false;
    process->exception_thread = nullptr;
    process->is_suspended = false;
    process->schedule_count = 0;

    // Open a reference to the resource limit.
    process->resource_limit->Open();

    return ResultSuccess;
}

void KProcess::DoWorkerTaskImpl() {
    UNIMPLEMENTED();
}

KResourceLimit* KProcess::GetResourceLimit() const {
    return resource_limit;
}

void KProcess::IncrementRunningThreadCount() {
    ASSERT(num_running_threads.load() >= 0);
    ++num_running_threads;
}

void KProcess::DecrementRunningThreadCount() {
    ASSERT(num_running_threads.load() > 0);

    if (const auto prev = num_running_threads--; prev == 1) {
        // TODO(bunnei): Process termination to be implemented when multiprocess is supported.
        UNIMPLEMENTED_MSG("KProcess termination is not implemennted!");
    }
}

u64 KProcess::GetTotalPhysicalMemoryAvailable() const {
    const u64 capacity{resource_limit->GetFreeValue(LimitableResource::PhysicalMemory) +
                       page_table->GetNormalMemorySize() + GetSystemResourceSize() + image_size +
                       main_thread_stack_size};
    if (const auto pool_size = kernel.MemoryManager().GetSize(KMemoryManager::Pool::Application);
        capacity != pool_size) {
        LOG_WARNING(Kernel, "capacity {} != application pool size {}", capacity, pool_size);
    }
    if (capacity < memory_usage_capacity) {
        return capacity;
    }
    return memory_usage_capacity;
}

u64 KProcess::GetTotalPhysicalMemoryAvailableWithoutSystemResource() const {
    return GetTotalPhysicalMemoryAvailable() - GetSystemResourceSize();
}

u64 KProcess::GetTotalPhysicalMemoryUsed() const {
    return image_size + main_thread_stack_size + page_table->GetNormalMemorySize() +
           GetSystemResourceSize();
}

u64 KProcess::GetTotalPhysicalMemoryUsedWithoutSystemResource() const {
    return GetTotalPhysicalMemoryUsed() - GetSystemResourceUsage();
}

bool KProcess::ReleaseUserException(KThread* thread) {
    KScopedSchedulerLock sl{kernel};

    if (exception_thread == thread) {
        exception_thread = nullptr;

        // Remove waiter thread.
        s32 num_waiters{};
        if (KThread* next = thread->RemoveWaiterByKey(
                std::addressof(num_waiters),
                reinterpret_cast<uintptr_t>(std::addressof(exception_thread)));
            next != nullptr) {
            next->EndWait(ResultSuccess);
        }

        KScheduler::SetSchedulerUpdateNeeded(kernel);

        return true;
    } else {
        return false;
    }
}

void KProcess::PinCurrentThread(s32 core_id) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Get the current thread.
    KThread* cur_thread =
        kernel.Scheduler(static_cast<std::size_t>(core_id)).GetSchedulerCurrentThread();

    // If the thread isn't terminated, pin it.
    if (!cur_thread->IsTerminationRequested()) {
        // Pin it.
        PinThread(core_id, cur_thread);
        cur_thread->Pin(core_id);

        // An update is needed.
        KScheduler::SetSchedulerUpdateNeeded(kernel);
    }
}

void KProcess::UnpinCurrentThread(s32 core_id) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Get the current thread.
    KThread* cur_thread =
        kernel.Scheduler(static_cast<std::size_t>(core_id)).GetSchedulerCurrentThread();

    // Unpin it.
    cur_thread->Unpin();
    UnpinThread(core_id, cur_thread);

    // An update is needed.
    KScheduler::SetSchedulerUpdateNeeded(kernel);
}

void KProcess::UnpinThread(KThread* thread) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    // Get the thread's core id.
    const auto core_id = thread->GetActiveCore();

    // Unpin it.
    UnpinThread(core_id, thread);
    thread->Unpin();

    // An update is needed.
    KScheduler::SetSchedulerUpdateNeeded(kernel);
}

Result KProcess::AddSharedMemory(KSharedMemory* shmem, [[maybe_unused]] VAddr address,
                                 [[maybe_unused]] size_t size) {
    // Lock ourselves, to prevent concurrent access.
    KScopedLightLock lk(state_lock);

    // Try to find an existing info for the memory.
    KSharedMemoryInfo* shemen_info = nullptr;
    const auto iter = std::find_if(
        shared_memory_list.begin(), shared_memory_list.end(),
        [shmem](const KSharedMemoryInfo* info) { return info->GetSharedMemory() == shmem; });
    if (iter != shared_memory_list.end()) {
        shemen_info = *iter;
    }

    if (shemen_info == nullptr) {
        shemen_info = KSharedMemoryInfo::Allocate(kernel);
        R_UNLESS(shemen_info != nullptr, ResultOutOfMemory);

        shemen_info->Initialize(shmem);
        shared_memory_list.push_back(shemen_info);
    }

    // Open a reference to the shared memory and its info.
    shmem->Open();
    shemen_info->Open();

    return ResultSuccess;
}

void KProcess::RemoveSharedMemory(KSharedMemory* shmem, [[maybe_unused]] VAddr address,
                                  [[maybe_unused]] size_t size) {
    // Lock ourselves, to prevent concurrent access.
    KScopedLightLock lk(state_lock);

    KSharedMemoryInfo* shemen_info = nullptr;
    const auto iter = std::find_if(
        shared_memory_list.begin(), shared_memory_list.end(),
        [shmem](const KSharedMemoryInfo* info) { return info->GetSharedMemory() == shmem; });
    if (iter != shared_memory_list.end()) {
        shemen_info = *iter;
    }

    ASSERT(shemen_info != nullptr);

    if (shemen_info->Close()) {
        shared_memory_list.erase(iter);
        KSharedMemoryInfo::Free(kernel, shemen_info);
    }

    // Close a reference to the shared memory.
    shmem->Close();
}

void KProcess::RegisterThread(KThread* thread) {
    KScopedLightLock lk{list_lock};

    thread_list.push_back(thread);
}

void KProcess::UnregisterThread(KThread* thread) {
    KScopedLightLock lk{list_lock};

    thread_list.remove(thread);
}

Result KProcess::Reset() {
    // Lock the process and the scheduler.
    KScopedLightLock lk(state_lock);
    KScopedSchedulerLock sl{kernel};

    // Validate that we're in a state that we can reset.
    R_UNLESS(status != ProcessStatus::Exited, ResultInvalidState);
    R_UNLESS(is_signaled, ResultInvalidState);

    // Clear signaled.
    is_signaled = false;
    return ResultSuccess;
}

Result KProcess::SetActivity(ProcessActivity activity) {
    // Lock ourselves and the scheduler.
    KScopedLightLock lk{state_lock};
    KScopedLightLock list_lk{list_lock};
    KScopedSchedulerLock sl{kernel};

    // Validate our state.
    R_UNLESS(status != ProcessStatus::Exiting, ResultInvalidState);
    R_UNLESS(status != ProcessStatus::Exited, ResultInvalidState);

    // Either pause or resume.
    if (activity == ProcessActivity::Paused) {
        // Verify that we're not suspended.
        if (is_suspended) {
            return ResultInvalidState;
        }

        // Suspend all threads.
        for (auto* thread : GetThreadList()) {
            thread->RequestSuspend(SuspendType::Process);
        }

        // Set ourselves as suspended.
        SetSuspended(true);
    } else {
        ASSERT(activity == ProcessActivity::Runnable);

        // Verify that we're suspended.
        if (!is_suspended) {
            return ResultInvalidState;
        }

        // Resume all threads.
        for (auto* thread : GetThreadList()) {
            thread->Resume(SuspendType::Process);
        }

        // Set ourselves as resumed.
        SetSuspended(false);
    }

    return ResultSuccess;
}

Result KProcess::LoadFromMetadata(const FileSys::ProgramMetadata& metadata, std::size_t code_size) {
    program_id = metadata.GetTitleID();
    ideal_core = metadata.GetMainThreadCore();
    is_64bit_process = metadata.Is64BitProgram();
    system_resource_size = metadata.GetSystemResourceSize();
    image_size = code_size;

    KScopedResourceReservation memory_reservation(resource_limit, LimitableResource::PhysicalMemory,
                                                  code_size + system_resource_size);
    if (!memory_reservation.Succeeded()) {
        LOG_ERROR(Kernel, "Could not reserve process memory requirements of size {:X} bytes",
                  code_size + system_resource_size);
        return ResultLimitReached;
    }
    // Initialize proces address space
    if (const Result result{page_table->InitializeForProcess(metadata.GetAddressSpaceType(), false,
                                                             0x8000000, code_size,
                                                             KMemoryManager::Pool::Application)};
        result.IsError()) {
        return result;
    }

    // Map process code region
    if (const Result result{page_table->MapProcessCode(page_table->GetCodeRegionStart(),
                                                       code_size / PageSize, KMemoryState::Code,
                                                       KMemoryPermission::None)};
        result.IsError()) {
        return result;
    }

    // Initialize process capabilities
    const auto& caps{metadata.GetKernelCapabilities()};
    if (const Result result{
            capabilities.InitializeForUserProcess(caps.data(), caps.size(), *page_table)};
        result.IsError()) {
        return result;
    }

    // Set memory usage capacity
    switch (metadata.GetAddressSpaceType()) {
    case FileSys::ProgramAddressSpaceType::Is32Bit:
    case FileSys::ProgramAddressSpaceType::Is36Bit:
    case FileSys::ProgramAddressSpaceType::Is39Bit:
        memory_usage_capacity = page_table->GetHeapRegionEnd() - page_table->GetHeapRegionStart();
        break;

    case FileSys::ProgramAddressSpaceType::Is32BitNoMap:
        memory_usage_capacity = page_table->GetHeapRegionEnd() - page_table->GetHeapRegionStart() +
                                page_table->GetAliasRegionEnd() - page_table->GetAliasRegionStart();
        break;

    default:
        ASSERT(false);
    }

    // Create TLS region
    R_TRY(this->CreateThreadLocalRegion(std::addressof(tls_region_address)));
    memory_reservation.Commit();

    return handle_table.Initialize(capabilities.GetHandleTableSize());
}

void KProcess::Run(s32 main_thread_priority, u64 stack_size) {
    AllocateMainThreadStack(stack_size);
    resource_limit->Reserve(LimitableResource::Threads, 1);
    resource_limit->Reserve(LimitableResource::PhysicalMemory, main_thread_stack_size);

    const std::size_t heap_capacity{memory_usage_capacity - (main_thread_stack_size + image_size)};
    ASSERT(!page_table->SetMaxHeapSize(heap_capacity).IsError());

    ChangeStatus(ProcessStatus::Running);

    SetupMainThread(kernel.System(), *this, main_thread_priority, main_thread_stack_top);
}

void KProcess::PrepareForTermination() {
    ChangeStatus(ProcessStatus::Exiting);

    const auto stop_threads = [this](const std::vector<KThread*>& in_thread_list) {
        for (auto* thread : in_thread_list) {
            if (thread->GetOwnerProcess() != this)
                continue;

            if (thread == GetCurrentThreadPointer(kernel))
                continue;

            // TODO(Subv): When are the other running/ready threads terminated?
            ASSERT_MSG(thread->GetState() == ThreadState::Waiting,
                       "Exiting processes with non-waiting threads is currently unimplemented");

            thread->Exit();
        }
    };

    stop_threads(kernel.System().GlobalSchedulerContext().GetThreadList());

    this->DeleteThreadLocalRegion(tls_region_address);
    tls_region_address = 0;

    if (resource_limit) {
        resource_limit->Release(LimitableResource::PhysicalMemory,
                                main_thread_stack_size + image_size);
    }

    ChangeStatus(ProcessStatus::Exited);
}

void KProcess::Finalize() {
    // Free all shared memory infos.
    {
        auto it = shared_memory_list.begin();
        while (it != shared_memory_list.end()) {
            KSharedMemoryInfo* info = *it;
            KSharedMemory* shmem = info->GetSharedMemory();

            while (!info->Close()) {
                shmem->Close();
            }

            shmem->Close();

            it = shared_memory_list.erase(it);
            KSharedMemoryInfo::Free(kernel, info);
        }
    }

    // Release memory to the resource limit.
    if (resource_limit != nullptr) {
        resource_limit->Close();
        resource_limit = nullptr;
    }

    // Finalize the page table.
    page_table.reset();

    // Perform inherited finalization.
    KAutoObjectWithSlabHeapAndContainer<KProcess, KWorkerTask>::Finalize();
}

Result KProcess::CreateThreadLocalRegion(VAddr* out) {
    KThreadLocalPage* tlp = nullptr;
    VAddr tlr = 0;

    // See if we can get a region from a partially used TLP.
    {
        KScopedSchedulerLock sl{kernel};

        if (auto it = partially_used_tlp_tree.begin(); it != partially_used_tlp_tree.end()) {
            tlr = it->Reserve();
            ASSERT(tlr != 0);

            if (it->IsAllUsed()) {
                tlp = std::addressof(*it);
                partially_used_tlp_tree.erase(it);
                fully_used_tlp_tree.insert(*tlp);
            }

            *out = tlr;
            return ResultSuccess;
        }
    }

    // Allocate a new page.
    tlp = KThreadLocalPage::Allocate(kernel);
    R_UNLESS(tlp != nullptr, ResultOutOfMemory);
    auto tlp_guard = SCOPE_GUARD({ KThreadLocalPage::Free(kernel, tlp); });

    // Initialize the new page.
    R_TRY(tlp->Initialize(kernel, this));

    // Reserve a TLR.
    tlr = tlp->Reserve();
    ASSERT(tlr != 0);

    // Insert into our tree.
    {
        KScopedSchedulerLock sl{kernel};
        if (tlp->IsAllUsed()) {
            fully_used_tlp_tree.insert(*tlp);
        } else {
            partially_used_tlp_tree.insert(*tlp);
        }
    }

    // We succeeded!
    tlp_guard.Cancel();
    *out = tlr;
    return ResultSuccess;
}

Result KProcess::DeleteThreadLocalRegion(VAddr addr) {
    KThreadLocalPage* page_to_free = nullptr;

    // Release the region.
    {
        KScopedSchedulerLock sl{kernel};

        // Try to find the page in the partially used list.
        auto it = partially_used_tlp_tree.find_key(Common::AlignDown(addr, PageSize));
        if (it == partially_used_tlp_tree.end()) {
            // If we don't find it, it has to be in the fully used list.
            it = fully_used_tlp_tree.find_key(Common::AlignDown(addr, PageSize));
            R_UNLESS(it != fully_used_tlp_tree.end(), ResultInvalidAddress);

            // Release the region.
            it->Release(addr);

            // Move the page out of the fully used list.
            KThreadLocalPage* tlp = std::addressof(*it);
            fully_used_tlp_tree.erase(it);
            if (tlp->IsAllFree()) {
                page_to_free = tlp;
            } else {
                partially_used_tlp_tree.insert(*tlp);
            }
        } else {
            // Release the region.
            it->Release(addr);

            // Handle the all-free case.
            KThreadLocalPage* tlp = std::addressof(*it);
            if (tlp->IsAllFree()) {
                partially_used_tlp_tree.erase(it);
                page_to_free = tlp;
            }
        }
    }

    // If we should free the page it was in, do so.
    if (page_to_free != nullptr) {
        page_to_free->Finalize();

        KThreadLocalPage::Free(kernel, page_to_free);
    }

    return ResultSuccess;
}

bool KProcess::InsertWatchpoint(Core::System& system, VAddr addr, u64 size,
                                DebugWatchpointType type) {
    const auto watch{std::find_if(watchpoints.begin(), watchpoints.end(), [&](const auto& wp) {
        return wp.type == DebugWatchpointType::None;
    })};

    if (watch == watchpoints.end()) {
        return false;
    }

    watch->start_address = addr;
    watch->end_address = addr + size;
    watch->type = type;

    for (VAddr page = Common::AlignDown(addr, PageSize); page < addr + size; page += PageSize) {
        debug_page_refcounts[page]++;
        system.Memory().MarkRegionDebug(page, PageSize, true);
    }

    return true;
}

bool KProcess::RemoveWatchpoint(Core::System& system, VAddr addr, u64 size,
                                DebugWatchpointType type) {
    const auto watch{std::find_if(watchpoints.begin(), watchpoints.end(), [&](const auto& wp) {
        return wp.start_address == addr && wp.end_address == addr + size && wp.type == type;
    })};

    if (watch == watchpoints.end()) {
        return false;
    }

    watch->start_address = 0;
    watch->end_address = 0;
    watch->type = DebugWatchpointType::None;

    for (VAddr page = Common::AlignDown(addr, PageSize); page < addr + size; page += PageSize) {
        debug_page_refcounts[page]--;
        if (!debug_page_refcounts[page]) {
            system.Memory().MarkRegionDebug(page, PageSize, false);
        }
    }

    return true;
}

void KProcess::LoadModule(CodeSet code_set, VAddr base_addr) {
    const auto ReprotectSegment = [&](const CodeSet::Segment& segment,
                                      Svc::MemoryPermission permission) {
        page_table->SetProcessMemoryPermission(segment.addr + base_addr, segment.size, permission);
    };

    kernel.System().Memory().WriteBlock(*this, base_addr, code_set.memory.data(),
                                        code_set.memory.size());

    ReprotectSegment(code_set.CodeSegment(), Svc::MemoryPermission::ReadExecute);
    ReprotectSegment(code_set.RODataSegment(), Svc::MemoryPermission::Read);
    ReprotectSegment(code_set.DataSegment(), Svc::MemoryPermission::ReadWrite);
}

bool KProcess::IsSignaled() const {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());
    return is_signaled;
}

KProcess::KProcess(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, page_table{std::make_unique<KPageTable>(
                                                        kernel_.System())},
      handle_table{kernel_}, address_arbiter{kernel_.System()}, condition_var{kernel_.System()},
      state_lock{kernel_}, list_lock{kernel_} {}

KProcess::~KProcess() = default;

void KProcess::ChangeStatus(ProcessStatus new_status) {
    if (status == new_status) {
        return;
    }

    status = new_status;
    is_signaled = true;
    NotifyAvailable();
}

Result KProcess::AllocateMainThreadStack(std::size_t stack_size) {
    ASSERT(stack_size);

    // The kernel always ensures that the given stack size is page aligned.
    main_thread_stack_size = Common::AlignUp(stack_size, PageSize);

    const VAddr start{page_table->GetStackRegionStart()};
    const std::size_t size{page_table->GetStackRegionEnd() - start};

    CASCADE_RESULT(main_thread_stack_top,
                   page_table->AllocateAndMapMemory(
                       main_thread_stack_size / PageSize, PageSize, false, start, size / PageSize,
                       KMemoryState::Stack, KMemoryPermission::UserReadWrite));

    main_thread_stack_top += main_thread_stack_size;

    return ResultSuccess;
}

} // namespace Kernel
