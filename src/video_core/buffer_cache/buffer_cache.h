// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <numeric>
#include <span>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/icl/interval_set.hpp>

#include "common/common_types.h"
#include "common/div_ceil.h"
#include "common/literals.h"
#include "common/lru_cache.h"
#include "common/microprofile.h"
#include "common/polyfill_ranges.h"
#include "common/scratch_buffer.h"
#include "common/settings.h"
#include "core/memory.h"
#include "video_core/buffer_cache/buffer_base.h"
#include "video_core/control/channel_state_cache.h"
#include "video_core/delayed_destruction_ring.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/draw_manager.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/slot_vector.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon {

MICROPROFILE_DECLARE(GPU_PrepareBuffers);
MICROPROFILE_DECLARE(GPU_BindUploadBuffers);
MICROPROFILE_DECLARE(GPU_DownloadMemory);

using BufferId = SlotId;

using VideoCore::Surface::PixelFormat;
using namespace Common::Literals;

constexpr u32 NUM_VERTEX_BUFFERS = 32;
constexpr u32 NUM_TRANSFORM_FEEDBACK_BUFFERS = 4;
constexpr u32 NUM_GRAPHICS_UNIFORM_BUFFERS = 18;
constexpr u32 NUM_COMPUTE_UNIFORM_BUFFERS = 8;
constexpr u32 NUM_STORAGE_BUFFERS = 16;
constexpr u32 NUM_TEXTURE_BUFFERS = 16;
constexpr u32 NUM_STAGES = 5;

enum class ObtainBufferSynchronize : u32 {
    NoSynchronize = 0,
    FullSynchronize = 1,
    SynchronizeNoDirty = 2,
};

enum class ObtainBufferOperation : u32 {
    DoNothing = 0,
    MarkAsWritten = 1,
    DiscardWrite = 2,
    MarkQuery = 3,
};

using UniformBufferSizes = std::array<std::array<u32, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES>;
using ComputeUniformBufferSizes = std::array<u32, NUM_COMPUTE_UNIFORM_BUFFERS>;

template <typename P>
class BufferCache : public VideoCommon::ChannelSetupCaches<VideoCommon::ChannelInfo> {

    // Page size for caching purposes.
    // This is unrelated to the CPU page size and it can be changed as it seems optimal.
    static constexpr u32 YUZU_PAGEBITS = 16;
    static constexpr u64 YUZU_PAGESIZE = u64{1} << YUZU_PAGEBITS;

    static constexpr bool IS_OPENGL = P::IS_OPENGL;
    static constexpr bool HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS =
        P::HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS;
    static constexpr bool HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT =
        P::HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT;
    static constexpr bool NEEDS_BIND_UNIFORM_INDEX = P::NEEDS_BIND_UNIFORM_INDEX;
    static constexpr bool NEEDS_BIND_STORAGE_INDEX = P::NEEDS_BIND_STORAGE_INDEX;
    static constexpr bool USE_MEMORY_MAPS = P::USE_MEMORY_MAPS;
    static constexpr bool SEPARATE_IMAGE_BUFFERS_BINDINGS = P::SEPARATE_IMAGE_BUFFER_BINDINGS;

    static constexpr BufferId NULL_BUFFER_ID{0};

    static constexpr s64 DEFAULT_EXPECTED_MEMORY = 512_MiB;
    static constexpr s64 DEFAULT_CRITICAL_MEMORY = 1_GiB;
    static constexpr s64 TARGET_THRESHOLD = 4_GiB;

    using Maxwell = Tegra::Engines::Maxwell3D::Regs;

    using Runtime = typename P::Runtime;
    using Buffer = typename P::Buffer;

    using IntervalSet = boost::icl::interval_set<VAddr>;
    using IntervalType = typename IntervalSet::interval_type;

    struct Empty {};

    struct OverlapResult {
        std::vector<BufferId> ids;
        VAddr begin;
        VAddr end;
        bool has_stream_leap = false;
    };

    struct Binding {
        VAddr cpu_addr{};
        u32 size{};
        BufferId buffer_id;
    };

    struct TextureBufferBinding : Binding {
        PixelFormat format;
    };

    static constexpr Binding NULL_BINDING{
        .cpu_addr = 0,
        .size = 0,
        .buffer_id = NULL_BUFFER_ID,
    };

public:
    static constexpr u32 DEFAULT_SKIP_CACHE_SIZE = static_cast<u32>(4_KiB);

    explicit BufferCache(VideoCore::RasterizerInterface& rasterizer_,
                         Core::Memory::Memory& cpu_memory_, Runtime& runtime_);

    void TickFrame();

    void WriteMemory(VAddr cpu_addr, u64 size);

    void CachedWriteMemory(VAddr cpu_addr, u64 size);

    void DownloadMemory(VAddr cpu_addr, u64 size);

    bool InlineMemory(VAddr dest_address, size_t copy_size, std::span<const u8> inlined_buffer);

    void BindGraphicsUniformBuffer(size_t stage, u32 index, GPUVAddr gpu_addr, u32 size);

    void DisableGraphicsUniformBuffer(size_t stage, u32 index);

    void UpdateGraphicsBuffers(bool is_indexed);

    void UpdateComputeBuffers();

    void BindHostGeometryBuffers(bool is_indexed);

    void BindHostStageBuffers(size_t stage);

    void BindHostComputeBuffers();

    void SetUniformBuffersState(const std::array<u32, NUM_STAGES>& mask,
                                const UniformBufferSizes* sizes);

    void SetComputeUniformBufferState(u32 mask, const ComputeUniformBufferSizes* sizes);

    void UnbindGraphicsStorageBuffers(size_t stage);

    void BindGraphicsStorageBuffer(size_t stage, size_t ssbo_index, u32 cbuf_index, u32 cbuf_offset,
                                   bool is_written);

    void UnbindGraphicsTextureBuffers(size_t stage);

    void BindGraphicsTextureBuffer(size_t stage, size_t tbo_index, GPUVAddr gpu_addr, u32 size,
                                   PixelFormat format, bool is_written, bool is_image);

    void UnbindComputeStorageBuffers();

    void BindComputeStorageBuffer(size_t ssbo_index, u32 cbuf_index, u32 cbuf_offset,
                                  bool is_written);

    void UnbindComputeTextureBuffers();

    void BindComputeTextureBuffer(size_t tbo_index, GPUVAddr gpu_addr, u32 size, PixelFormat format,
                                  bool is_written, bool is_image);

    void FlushCachedWrites();

    /// Return true when there are uncommitted buffers to be downloaded
    [[nodiscard]] bool HasUncommittedFlushes() const noexcept;

    void AccumulateFlushes();

    /// Return true when the caller should wait for async downloads
    [[nodiscard]] bool ShouldWaitAsyncFlushes() const noexcept;

    /// Commit asynchronous downloads
    void CommitAsyncFlushes();
    void CommitAsyncFlushesHigh();

    /// Pop asynchronous downloads
    void PopAsyncFlushes();

    bool DMACopy(GPUVAddr src_address, GPUVAddr dest_address, u64 amount);

    bool DMAClear(GPUVAddr src_address, u64 amount, u32 value);

    [[nodiscard]] std::pair<Buffer*, u32> ObtainBuffer(GPUVAddr gpu_addr, u32 size,
                                                       ObtainBufferSynchronize sync_info,
                                                       ObtainBufferOperation post_op);

    /// Return true when a CPU region is modified from the GPU
    [[nodiscard]] bool IsRegionGpuModified(VAddr addr, size_t size);

    /// Return true when a region is registered on the cache
    [[nodiscard]] bool IsRegionRegistered(VAddr addr, size_t size);

    /// Return true when a CPU region is modified from the CPU
    [[nodiscard]] bool IsRegionCpuModified(VAddr addr, size_t size);

    void SetDrawIndirect(
        const Tegra::Engines::DrawManager::IndirectParams* current_draw_indirect_) {
        current_draw_indirect = current_draw_indirect_;
    }

    [[nodiscard]] std::pair<Buffer*, u32> GetDrawIndirectCount();

    [[nodiscard]] std::pair<Buffer*, u32> GetDrawIndirectBuffer();

    std::recursive_mutex mutex;
    Runtime& runtime;

private:
    template <typename Func>
    static void ForEachEnabledBit(u32 enabled_mask, Func&& func) {
        for (u32 index = 0; enabled_mask != 0; ++index, enabled_mask >>= 1) {
            const int disabled_bits = std::countr_zero(enabled_mask);
            index += disabled_bits;
            enabled_mask >>= disabled_bits;
            func(index);
        }
    }

    template <typename Func>
    void ForEachBufferInRange(VAddr cpu_addr, u64 size, Func&& func) {
        const u64 page_end = Common::DivCeil(cpu_addr + size, YUZU_PAGESIZE);
        for (u64 page = cpu_addr >> YUZU_PAGEBITS; page < page_end;) {
            const BufferId buffer_id = page_table[page];
            if (!buffer_id) {
                ++page;
                continue;
            }
            Buffer& buffer = slot_buffers[buffer_id];
            func(buffer_id, buffer);

            const VAddr end_addr = buffer.CpuAddr() + buffer.SizeBytes();
            page = Common::DivCeil(end_addr, YUZU_PAGESIZE);
        }
    }

    template <typename Func>
    void ForEachWrittenRange(VAddr cpu_addr, u64 size, Func&& func) {
        const VAddr start_address = cpu_addr;
        const VAddr end_address = start_address + size;
        const VAddr search_base =
            static_cast<VAddr>(std::min<s64>(0LL, static_cast<s64>(start_address - size)));
        const IntervalType search_interval{search_base, search_base + 1};
        auto it = common_ranges.lower_bound(search_interval);
        if (it == common_ranges.end()) {
            it = common_ranges.begin();
        }
        for (; it != common_ranges.end(); it++) {
            VAddr inter_addr_end = it->upper();
            VAddr inter_addr = it->lower();
            if (inter_addr >= end_address) {
                break;
            }
            if (inter_addr_end <= start_address) {
                continue;
            }
            if (inter_addr_end > end_address) {
                inter_addr_end = end_address;
            }
            if (inter_addr < start_address) {
                inter_addr = start_address;
            }
            func(inter_addr, inter_addr_end);
        }
    }

    static bool IsRangeGranular(VAddr cpu_addr, size_t size) {
        return (cpu_addr & ~Core::Memory::YUZU_PAGEMASK) ==
               ((cpu_addr + size) & ~Core::Memory::YUZU_PAGEMASK);
    }

    void RunGarbageCollector();

    void BindHostIndexBuffer();

    void BindHostVertexBuffers();

    void BindHostDrawIndirectBuffers();

    void BindHostGraphicsUniformBuffers(size_t stage);

    void BindHostGraphicsUniformBuffer(size_t stage, u32 index, u32 binding_index, bool needs_bind);

    void BindHostGraphicsStorageBuffers(size_t stage);

    void BindHostGraphicsTextureBuffers(size_t stage);

    void BindHostTransformFeedbackBuffers();

    void BindHostComputeUniformBuffers();

    void BindHostComputeStorageBuffers();

    void BindHostComputeTextureBuffers();

    void DoUpdateGraphicsBuffers(bool is_indexed);

    void DoUpdateComputeBuffers();

    void UpdateIndexBuffer();

    void UpdateVertexBuffers();

    void UpdateVertexBuffer(u32 index);

    void UpdateDrawIndirect();

    void UpdateUniformBuffers(size_t stage);

    void UpdateStorageBuffers(size_t stage);

    void UpdateTextureBuffers(size_t stage);

    void UpdateTransformFeedbackBuffers();

    void UpdateTransformFeedbackBuffer(u32 index);

    void UpdateComputeUniformBuffers();

    void UpdateComputeStorageBuffers();

    void UpdateComputeTextureBuffers();

    void MarkWrittenBuffer(BufferId buffer_id, VAddr cpu_addr, u32 size);

    [[nodiscard]] BufferId FindBuffer(VAddr cpu_addr, u32 size);

    [[nodiscard]] OverlapResult ResolveOverlaps(VAddr cpu_addr, u32 wanted_size);

    void JoinOverlap(BufferId new_buffer_id, BufferId overlap_id, bool accumulate_stream_score);

    [[nodiscard]] BufferId CreateBuffer(VAddr cpu_addr, u32 wanted_size);

    void Register(BufferId buffer_id);

    void Unregister(BufferId buffer_id);

    template <bool insert>
    void ChangeRegister(BufferId buffer_id);

    void TouchBuffer(Buffer& buffer, BufferId buffer_id) noexcept;

    bool SynchronizeBuffer(Buffer& buffer, VAddr cpu_addr, u32 size);

    bool SynchronizeBufferImpl(Buffer& buffer, VAddr cpu_addr, u32 size);

    void UploadMemory(Buffer& buffer, u64 total_size_bytes, u64 largest_copy,
                      std::span<BufferCopy> copies);

    void ImmediateUploadMemory(Buffer& buffer, u64 largest_copy,
                               std::span<const BufferCopy> copies);

    void MappedUploadMemory(Buffer& buffer, u64 total_size_bytes, std::span<BufferCopy> copies);

    void DownloadBufferMemory(Buffer& buffer_id);

    void DownloadBufferMemory(Buffer& buffer_id, VAddr cpu_addr, u64 size);

    void DeleteBuffer(BufferId buffer_id);

    void NotifyBufferDeletion();

    [[nodiscard]] Binding StorageBufferBinding(GPUVAddr ssbo_addr, u32 cbuf_index,
                                               bool is_written = false) const;

    [[nodiscard]] TextureBufferBinding GetTextureBufferBinding(GPUVAddr gpu_addr, u32 size,
                                                               PixelFormat format);

    [[nodiscard]] std::span<const u8> ImmediateBufferWithData(VAddr cpu_addr, size_t size);

    [[nodiscard]] std::span<u8> ImmediateBuffer(size_t wanted_capacity);

    [[nodiscard]] bool HasFastUniformBufferBound(size_t stage, u32 binding_index) const noexcept;

    void ClearDownload(IntervalType subtract_interval);

    VideoCore::RasterizerInterface& rasterizer;
    Core::Memory::Memory& cpu_memory;

    SlotVector<Buffer> slot_buffers;
    DelayedDestructionRing<Buffer, 8> delayed_destruction_ring;

    const Tegra::Engines::DrawManager::IndirectParams* current_draw_indirect{};

    u32 last_index_count = 0;

    Binding index_buffer;
    std::array<Binding, NUM_VERTEX_BUFFERS> vertex_buffers;
    std::array<std::array<Binding, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES> uniform_buffers;
    std::array<std::array<Binding, NUM_STORAGE_BUFFERS>, NUM_STAGES> storage_buffers;
    std::array<std::array<TextureBufferBinding, NUM_TEXTURE_BUFFERS>, NUM_STAGES> texture_buffers;
    std::array<Binding, NUM_TRANSFORM_FEEDBACK_BUFFERS> transform_feedback_buffers;
    Binding count_buffer_binding;
    Binding indirect_buffer_binding;

    std::array<Binding, NUM_COMPUTE_UNIFORM_BUFFERS> compute_uniform_buffers;
    std::array<Binding, NUM_STORAGE_BUFFERS> compute_storage_buffers;
    std::array<TextureBufferBinding, NUM_TEXTURE_BUFFERS> compute_texture_buffers;

    std::array<u32, NUM_STAGES> enabled_uniform_buffer_masks{};
    u32 enabled_compute_uniform_buffer_mask = 0;

    const UniformBufferSizes* uniform_buffer_sizes{};
    const ComputeUniformBufferSizes* compute_uniform_buffer_sizes{};

    std::array<u32, NUM_STAGES> enabled_storage_buffers{};
    std::array<u32, NUM_STAGES> written_storage_buffers{};
    u32 enabled_compute_storage_buffers = 0;
    u32 written_compute_storage_buffers = 0;

    std::array<u32, NUM_STAGES> enabled_texture_buffers{};
    std::array<u32, NUM_STAGES> written_texture_buffers{};
    std::array<u32, NUM_STAGES> image_texture_buffers{};
    u32 enabled_compute_texture_buffers = 0;
    u32 written_compute_texture_buffers = 0;
    u32 image_compute_texture_buffers = 0;

    std::array<u32, 16> uniform_cache_hits{};
    std::array<u32, 16> uniform_cache_shots{};

    u32 uniform_buffer_skip_cache_size = DEFAULT_SKIP_CACHE_SIZE;

    bool has_deleted_buffers = false;

    std::conditional_t<HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS, std::array<u32, NUM_STAGES>, Empty>
        dirty_uniform_buffers{};
    std::conditional_t<IS_OPENGL, std::array<u32, NUM_STAGES>, Empty> fast_bound_uniform_buffers{};
    std::conditional_t<HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS,
                       std::array<std::array<u32, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES>, Empty>
        uniform_buffer_binding_sizes{};

    std::vector<BufferId> cached_write_buffer_ids;

    IntervalSet uncommitted_ranges;
    IntervalSet common_ranges;
    std::deque<IntervalSet> committed_ranges;

    Common::ScratchBuffer<u8> immediate_buffer_alloc;

    struct LRUItemParams {
        using ObjectType = BufferId;
        using TickType = u64;
    };
    Common::LeastRecentlyUsedCache<LRUItemParams> lru_cache;
    u64 frame_tick = 0;
    u64 total_used_memory = 0;
    u64 minimum_memory = 0;
    u64 critical_memory = 0;

    std::array<BufferId, ((1ULL << 39) >> YUZU_PAGEBITS)> page_table;
};

template <class P>
BufferCache<P>::BufferCache(VideoCore::RasterizerInterface& rasterizer_,
                            Core::Memory::Memory& cpu_memory_, Runtime& runtime_)
    : runtime{runtime_}, rasterizer{rasterizer_}, cpu_memory{cpu_memory_} {
    // Ensure the first slot is used for the null buffer
    void(slot_buffers.insert(runtime, NullBufferParams{}));
    common_ranges.clear();

    if (!runtime.CanReportMemoryUsage()) {
        minimum_memory = DEFAULT_EXPECTED_MEMORY;
        critical_memory = DEFAULT_CRITICAL_MEMORY;
        return;
    }

    const s64 device_memory = static_cast<s64>(runtime.GetDeviceLocalMemory());
    const s64 min_spacing_expected = device_memory - 1_GiB - 512_MiB;
    const s64 min_spacing_critical = device_memory - 1_GiB;
    const s64 mem_threshold = std::min(device_memory, TARGET_THRESHOLD);
    const s64 min_vacancy_expected = (6 * mem_threshold) / 10;
    const s64 min_vacancy_critical = (3 * mem_threshold) / 10;
    minimum_memory = static_cast<u64>(
        std::max(std::min(device_memory - min_vacancy_expected, min_spacing_expected),
                 DEFAULT_EXPECTED_MEMORY));
    critical_memory = static_cast<u64>(
        std::max(std::min(device_memory - min_vacancy_critical, min_spacing_critical),
                 DEFAULT_CRITICAL_MEMORY));
}

template <class P>
void BufferCache<P>::RunGarbageCollector() {
    const bool aggressive_gc = total_used_memory >= critical_memory;
    const u64 ticks_to_destroy = aggressive_gc ? 60 : 120;
    int num_iterations = aggressive_gc ? 64 : 32;
    const auto clean_up = [this, &num_iterations](BufferId buffer_id) {
        if (num_iterations == 0) {
            return true;
        }
        --num_iterations;
        auto& buffer = slot_buffers[buffer_id];
        DownloadBufferMemory(buffer);
        DeleteBuffer(buffer_id);
        return false;
    };
    lru_cache.ForEachItemBelow(frame_tick - ticks_to_destroy, clean_up);
}

template <class P>
void BufferCache<P>::TickFrame() {
    // Calculate hits and shots and move hit bits to the right
    const u32 hits = std::reduce(uniform_cache_hits.begin(), uniform_cache_hits.end());
    const u32 shots = std::reduce(uniform_cache_shots.begin(), uniform_cache_shots.end());
    std::copy_n(uniform_cache_hits.begin(), uniform_cache_hits.size() - 1,
                uniform_cache_hits.begin() + 1);
    std::copy_n(uniform_cache_shots.begin(), uniform_cache_shots.size() - 1,
                uniform_cache_shots.begin() + 1);
    uniform_cache_hits[0] = 0;
    uniform_cache_shots[0] = 0;

    const bool skip_preferred = hits * 256 < shots * 251;
    uniform_buffer_skip_cache_size = skip_preferred ? DEFAULT_SKIP_CACHE_SIZE : 0;

    // If we can obtain the memory info, use it instead of the estimate.
    if (runtime.CanReportMemoryUsage()) {
        total_used_memory = runtime.GetDeviceMemoryUsage();
    }
    if (total_used_memory >= minimum_memory) {
        RunGarbageCollector();
    }
    ++frame_tick;
    delayed_destruction_ring.Tick();
}

template <class P>
void BufferCache<P>::WriteMemory(VAddr cpu_addr, u64 size) {
    ForEachBufferInRange(cpu_addr, size, [&](BufferId, Buffer& buffer) {
        buffer.MarkRegionAsCpuModified(cpu_addr, size);
    });
}

template <class P>
void BufferCache<P>::CachedWriteMemory(VAddr cpu_addr, u64 size) {
    ForEachBufferInRange(cpu_addr, size, [&](BufferId buffer_id, Buffer& buffer) {
        if (!buffer.HasCachedWrites()) {
            cached_write_buffer_ids.push_back(buffer_id);
        }
        buffer.CachedCpuWrite(cpu_addr, size);
    });
}

template <class P>
void BufferCache<P>::DownloadMemory(VAddr cpu_addr, u64 size) {
    ForEachBufferInRange(cpu_addr, size, [&](BufferId, Buffer& buffer) {
        DownloadBufferMemory(buffer, cpu_addr, size);
    });
}

template <class P>
void BufferCache<P>::ClearDownload(IntervalType subtract_interval) {
    uncommitted_ranges.subtract(subtract_interval);
    for (auto& interval_set : committed_ranges) {
        interval_set.subtract(subtract_interval);
    }
}

template <class P>
bool BufferCache<P>::DMACopy(GPUVAddr src_address, GPUVAddr dest_address, u64 amount) {
    const std::optional<VAddr> cpu_src_address = gpu_memory->GpuToCpuAddress(src_address);
    const std::optional<VAddr> cpu_dest_address = gpu_memory->GpuToCpuAddress(dest_address);
    if (!cpu_src_address || !cpu_dest_address) {
        return false;
    }
    const bool source_dirty = IsRegionRegistered(*cpu_src_address, amount);
    const bool dest_dirty = IsRegionRegistered(*cpu_dest_address, amount);
    if (!source_dirty && !dest_dirty) {
        return false;
    }

    const IntervalType subtract_interval{*cpu_dest_address, *cpu_dest_address + amount};
    ClearDownload(subtract_interval);

    BufferId buffer_a;
    BufferId buffer_b;
    do {
        has_deleted_buffers = false;
        buffer_a = FindBuffer(*cpu_src_address, static_cast<u32>(amount));
        buffer_b = FindBuffer(*cpu_dest_address, static_cast<u32>(amount));
    } while (has_deleted_buffers);
    auto& src_buffer = slot_buffers[buffer_a];
    auto& dest_buffer = slot_buffers[buffer_b];
    SynchronizeBuffer(src_buffer, *cpu_src_address, static_cast<u32>(amount));
    SynchronizeBuffer(dest_buffer, *cpu_dest_address, static_cast<u32>(amount));
    std::array copies{BufferCopy{
        .src_offset = src_buffer.Offset(*cpu_src_address),
        .dst_offset = dest_buffer.Offset(*cpu_dest_address),
        .size = amount,
    }};

    boost::container::small_vector<IntervalType, 4> tmp_intervals;
    auto mirror = [&](VAddr base_address, VAddr base_address_end) {
        const u64 size = base_address_end - base_address;
        const VAddr diff = base_address - *cpu_src_address;
        const VAddr new_base_address = *cpu_dest_address + diff;
        const IntervalType add_interval{new_base_address, new_base_address + size};
        uncommitted_ranges.add(add_interval);
        tmp_intervals.push_back(add_interval);
    };
    ForEachWrittenRange(*cpu_src_address, amount, mirror);
    // This subtraction in this order is important for overlapping copies.
    common_ranges.subtract(subtract_interval);
    const bool has_new_downloads = tmp_intervals.size() != 0;
    for (const IntervalType& add_interval : tmp_intervals) {
        common_ranges.add(add_interval);
    }
    runtime.CopyBuffer(dest_buffer, src_buffer, copies);
    if (has_new_downloads) {
        dest_buffer.MarkRegionAsGpuModified(*cpu_dest_address, amount);
    }
    std::vector<u8> tmp_buffer(amount);
    cpu_memory.ReadBlockUnsafe(*cpu_src_address, tmp_buffer.data(), amount);
    cpu_memory.WriteBlockUnsafe(*cpu_dest_address, tmp_buffer.data(), amount);
    return true;
}

template <class P>
bool BufferCache<P>::DMAClear(GPUVAddr dst_address, u64 amount, u32 value) {
    const std::optional<VAddr> cpu_dst_address = gpu_memory->GpuToCpuAddress(dst_address);
    if (!cpu_dst_address) {
        return false;
    }
    const bool dest_dirty = IsRegionRegistered(*cpu_dst_address, amount);
    if (!dest_dirty) {
        return false;
    }

    const size_t size = amount * sizeof(u32);
    const IntervalType subtract_interval{*cpu_dst_address, *cpu_dst_address + size};
    ClearDownload(subtract_interval);
    common_ranges.subtract(subtract_interval);

    const BufferId buffer = FindBuffer(*cpu_dst_address, static_cast<u32>(size));
    auto& dest_buffer = slot_buffers[buffer];
    const u32 offset = dest_buffer.Offset(*cpu_dst_address);
    runtime.ClearBuffer(dest_buffer, offset, size, value);
    return true;
}

template <class P>
std::pair<typename P::Buffer*, u32> BufferCache<P>::ObtainBuffer(GPUVAddr gpu_addr, u32 size,
                                                                 ObtainBufferSynchronize sync_info,
                                                                 ObtainBufferOperation post_op) {
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
    if (!cpu_addr) {
        return {&slot_buffers[NULL_BUFFER_ID], 0};
    }
    const BufferId buffer_id = FindBuffer(*cpu_addr, size);
    Buffer& buffer = slot_buffers[buffer_id];

    // synchronize op
    switch (sync_info) {
    case ObtainBufferSynchronize::FullSynchronize:
        SynchronizeBuffer(buffer, *cpu_addr, size);
        break;
    default:
        break;
    }

    switch (post_op) {
    case ObtainBufferOperation::MarkAsWritten:
        MarkWrittenBuffer(buffer_id, *cpu_addr, size);
        break;
    case ObtainBufferOperation::DiscardWrite: {
        IntervalType interval{*cpu_addr, size};
        ClearDownload(interval);
        break;
    }
    default:
        break;
    }

    return {&buffer, buffer.Offset(*cpu_addr)};
}

template <class P>
void BufferCache<P>::BindGraphicsUniformBuffer(size_t stage, u32 index, GPUVAddr gpu_addr,
                                               u32 size) {
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
    const Binding binding{
        .cpu_addr = *cpu_addr,
        .size = size,
        .buffer_id = BufferId{},
    };
    uniform_buffers[stage][index] = binding;
}

template <class P>
void BufferCache<P>::DisableGraphicsUniformBuffer(size_t stage, u32 index) {
    uniform_buffers[stage][index] = NULL_BINDING;
}

template <class P>
void BufferCache<P>::UpdateGraphicsBuffers(bool is_indexed) {
    MICROPROFILE_SCOPE(GPU_PrepareBuffers);
    do {
        has_deleted_buffers = false;
        DoUpdateGraphicsBuffers(is_indexed);
    } while (has_deleted_buffers);
}

template <class P>
void BufferCache<P>::UpdateComputeBuffers() {
    MICROPROFILE_SCOPE(GPU_PrepareBuffers);
    do {
        has_deleted_buffers = false;
        DoUpdateComputeBuffers();
    } while (has_deleted_buffers);
}

template <class P>
void BufferCache<P>::BindHostGeometryBuffers(bool is_indexed) {
    MICROPROFILE_SCOPE(GPU_BindUploadBuffers);
    if (is_indexed) {
        BindHostIndexBuffer();
    } else if constexpr (!HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT) {
        const auto& draw_state = maxwell3d->draw_manager->GetDrawState();
        if (draw_state.topology == Maxwell::PrimitiveTopology::Quads ||
            draw_state.topology == Maxwell::PrimitiveTopology::QuadStrip) {
            runtime.BindQuadIndexBuffer(draw_state.topology, draw_state.vertex_buffer.first,
                                        draw_state.vertex_buffer.count);
        }
    }
    BindHostVertexBuffers();
    BindHostTransformFeedbackBuffers();
    if (current_draw_indirect) {
        BindHostDrawIndirectBuffers();
    }
}

template <class P>
void BufferCache<P>::BindHostStageBuffers(size_t stage) {
    MICROPROFILE_SCOPE(GPU_BindUploadBuffers);
    BindHostGraphicsUniformBuffers(stage);
    BindHostGraphicsStorageBuffers(stage);
    BindHostGraphicsTextureBuffers(stage);
}

template <class P>
void BufferCache<P>::BindHostComputeBuffers() {
    MICROPROFILE_SCOPE(GPU_BindUploadBuffers);
    BindHostComputeUniformBuffers();
    BindHostComputeStorageBuffers();
    BindHostComputeTextureBuffers();
}

template <class P>
void BufferCache<P>::SetUniformBuffersState(const std::array<u32, NUM_STAGES>& mask,
                                            const UniformBufferSizes* sizes) {
    if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
        if (enabled_uniform_buffer_masks != mask) {
            if constexpr (IS_OPENGL) {
                fast_bound_uniform_buffers.fill(0);
            }
            dirty_uniform_buffers.fill(~u32{0});
            uniform_buffer_binding_sizes.fill({});
        }
    }
    enabled_uniform_buffer_masks = mask;
    uniform_buffer_sizes = sizes;
}

template <class P>
void BufferCache<P>::SetComputeUniformBufferState(u32 mask,
                                                  const ComputeUniformBufferSizes* sizes) {
    enabled_compute_uniform_buffer_mask = mask;
    compute_uniform_buffer_sizes = sizes;
}

template <class P>
void BufferCache<P>::UnbindGraphicsStorageBuffers(size_t stage) {
    enabled_storage_buffers[stage] = 0;
    written_storage_buffers[stage] = 0;
}

template <class P>
void BufferCache<P>::BindGraphicsStorageBuffer(size_t stage, size_t ssbo_index, u32 cbuf_index,
                                               u32 cbuf_offset, bool is_written) {
    enabled_storage_buffers[stage] |= 1U << ssbo_index;
    written_storage_buffers[stage] |= (is_written ? 1U : 0U) << ssbo_index;

    const auto& cbufs = maxwell3d->state.shader_stages[stage];
    const GPUVAddr ssbo_addr = cbufs.const_buffers[cbuf_index].address + cbuf_offset;
    storage_buffers[stage][ssbo_index] = StorageBufferBinding(ssbo_addr, cbuf_index, is_written);
}

template <class P>
void BufferCache<P>::UnbindGraphicsTextureBuffers(size_t stage) {
    enabled_texture_buffers[stage] = 0;
    written_texture_buffers[stage] = 0;
    image_texture_buffers[stage] = 0;
}

template <class P>
void BufferCache<P>::BindGraphicsTextureBuffer(size_t stage, size_t tbo_index, GPUVAddr gpu_addr,
                                               u32 size, PixelFormat format, bool is_written,
                                               bool is_image) {
    enabled_texture_buffers[stage] |= 1U << tbo_index;
    written_texture_buffers[stage] |= (is_written ? 1U : 0U) << tbo_index;
    if constexpr (SEPARATE_IMAGE_BUFFERS_BINDINGS) {
        image_texture_buffers[stage] |= (is_image ? 1U : 0U) << tbo_index;
    }
    texture_buffers[stage][tbo_index] = GetTextureBufferBinding(gpu_addr, size, format);
}

template <class P>
void BufferCache<P>::UnbindComputeStorageBuffers() {
    enabled_compute_storage_buffers = 0;
    written_compute_storage_buffers = 0;
    image_compute_texture_buffers = 0;
}

template <class P>
void BufferCache<P>::BindComputeStorageBuffer(size_t ssbo_index, u32 cbuf_index, u32 cbuf_offset,
                                              bool is_written) {
    enabled_compute_storage_buffers |= 1U << ssbo_index;
    written_compute_storage_buffers |= (is_written ? 1U : 0U) << ssbo_index;

    const auto& launch_desc = kepler_compute->launch_description;
    ASSERT(((launch_desc.const_buffer_enable_mask >> cbuf_index) & 1) != 0);

    const auto& cbufs = launch_desc.const_buffer_config;
    const GPUVAddr ssbo_addr = cbufs[cbuf_index].Address() + cbuf_offset;
    compute_storage_buffers[ssbo_index] = StorageBufferBinding(ssbo_addr, cbuf_index, is_written);
}

template <class P>
void BufferCache<P>::UnbindComputeTextureBuffers() {
    enabled_compute_texture_buffers = 0;
    written_compute_texture_buffers = 0;
    image_compute_texture_buffers = 0;
}

template <class P>
void BufferCache<P>::BindComputeTextureBuffer(size_t tbo_index, GPUVAddr gpu_addr, u32 size,
                                              PixelFormat format, bool is_written, bool is_image) {
    enabled_compute_texture_buffers |= 1U << tbo_index;
    written_compute_texture_buffers |= (is_written ? 1U : 0U) << tbo_index;
    if constexpr (SEPARATE_IMAGE_BUFFERS_BINDINGS) {
        image_compute_texture_buffers |= (is_image ? 1U : 0U) << tbo_index;
    }
    compute_texture_buffers[tbo_index] = GetTextureBufferBinding(gpu_addr, size, format);
}

template <class P>
void BufferCache<P>::FlushCachedWrites() {
    for (const BufferId buffer_id : cached_write_buffer_ids) {
        slot_buffers[buffer_id].FlushCachedWrites();
    }
    cached_write_buffer_ids.clear();
}

template <class P>
bool BufferCache<P>::HasUncommittedFlushes() const noexcept {
    return !uncommitted_ranges.empty() || !committed_ranges.empty();
}

template <class P>
void BufferCache<P>::AccumulateFlushes() {
    if (Settings::values.gpu_accuracy.GetValue() != Settings::GPUAccuracy::High) {
        uncommitted_ranges.clear();
        return;
    }
    if (uncommitted_ranges.empty()) {
        return;
    }
    committed_ranges.emplace_back(std::move(uncommitted_ranges));
}

template <class P>
bool BufferCache<P>::ShouldWaitAsyncFlushes() const noexcept {
    return false;
}

template <class P>
void BufferCache<P>::CommitAsyncFlushesHigh() {
    AccumulateFlushes();

    if (committed_ranges.empty()) {
        return;
    }
    MICROPROFILE_SCOPE(GPU_DownloadMemory);
    const bool is_accuracy_normal =
        Settings::values.gpu_accuracy.GetValue() == Settings::GPUAccuracy::Normal;

    auto it = committed_ranges.begin();
    while (it != committed_ranges.end()) {
        auto& current_intervals = *it;
        auto next_it = std::next(it);
        while (next_it != committed_ranges.end()) {
            for (auto& interval : *next_it) {
                current_intervals.subtract(interval);
            }
            next_it++;
        }
        it++;
    }

    boost::container::small_vector<std::pair<BufferCopy, BufferId>, 1> downloads;
    u64 total_size_bytes = 0;
    u64 largest_copy = 0;
    for (const IntervalSet& intervals : committed_ranges) {
        for (auto& interval : intervals) {
            const std::size_t size = interval.upper() - interval.lower();
            const VAddr cpu_addr = interval.lower();
            ForEachBufferInRange(cpu_addr, size, [&](BufferId buffer_id, Buffer& buffer) {
                buffer.ForEachDownloadRangeAndClear(
                    cpu_addr, size, [&](u64 range_offset, u64 range_size) {
                        if (is_accuracy_normal) {
                            return;
                        }
                        const VAddr buffer_addr = buffer.CpuAddr();
                        const auto add_download = [&](VAddr start, VAddr end) {
                            const u64 new_offset = start - buffer_addr;
                            const u64 new_size = end - start;
                            downloads.push_back({
                                BufferCopy{
                                    .src_offset = new_offset,
                                    .dst_offset = total_size_bytes,
                                    .size = new_size,
                                },
                                buffer_id,
                            });
                            // Align up to avoid cache conflicts
                            constexpr u64 align = 8ULL;
                            constexpr u64 mask = ~(align - 1ULL);
                            total_size_bytes += (new_size + align - 1) & mask;
                            largest_copy = std::max(largest_copy, new_size);
                        };

                        const VAddr start_address = buffer_addr + range_offset;
                        const VAddr end_address = start_address + range_size;
                        ForEachWrittenRange(start_address, range_size, add_download);
                        const IntervalType subtract_interval{start_address, end_address};
                        common_ranges.subtract(subtract_interval);
                    });
            });
        }
    }
    committed_ranges.clear();
    if (downloads.empty()) {
        return;
    }
    if constexpr (USE_MEMORY_MAPS) {
        auto download_staging = runtime.DownloadStagingBuffer(total_size_bytes);
        runtime.PreCopyBarrier();
        for (auto& [copy, buffer_id] : downloads) {
            // Have in mind the staging buffer offset for the copy
            copy.dst_offset += download_staging.offset;
            const std::array copies{copy};
            runtime.CopyBuffer(download_staging.buffer, slot_buffers[buffer_id], copies, false);
        }
        runtime.PostCopyBarrier();
        runtime.Finish();
        for (const auto& [copy, buffer_id] : downloads) {
            const Buffer& buffer = slot_buffers[buffer_id];
            const VAddr cpu_addr = buffer.CpuAddr() + copy.src_offset;
            // Undo the modified offset
            const u64 dst_offset = copy.dst_offset - download_staging.offset;
            const u8* read_mapped_memory = download_staging.mapped_span.data() + dst_offset;
            cpu_memory.WriteBlockUnsafe(cpu_addr, read_mapped_memory, copy.size);
        }
    } else {
        const std::span<u8> immediate_buffer = ImmediateBuffer(largest_copy);
        for (const auto& [copy, buffer_id] : downloads) {
            Buffer& buffer = slot_buffers[buffer_id];
            buffer.ImmediateDownload(copy.src_offset, immediate_buffer.subspan(0, copy.size));
            const VAddr cpu_addr = buffer.CpuAddr() + copy.src_offset;
            cpu_memory.WriteBlockUnsafe(cpu_addr, immediate_buffer.data(), copy.size);
        }
    }
}

template <class P>
void BufferCache<P>::CommitAsyncFlushes() {
    if (Settings::values.gpu_accuracy.GetValue() == Settings::GPUAccuracy::High) {
        CommitAsyncFlushesHigh();
    } else {
        uncommitted_ranges.clear();
        committed_ranges.clear();
    }
}

template <class P>
void BufferCache<P>::PopAsyncFlushes() {}

template <class P>
bool BufferCache<P>::IsRegionGpuModified(VAddr addr, size_t size) {
    const u64 page_end = Common::DivCeil(addr + size, YUZU_PAGESIZE);
    for (u64 page = addr >> YUZU_PAGEBITS; page < page_end;) {
        const BufferId image_id = page_table[page];
        if (!image_id) {
            ++page;
            continue;
        }
        Buffer& buffer = slot_buffers[image_id];
        if (buffer.IsRegionGpuModified(addr, size)) {
            return true;
        }
        const VAddr end_addr = buffer.CpuAddr() + buffer.SizeBytes();
        page = Common::DivCeil(end_addr, YUZU_PAGESIZE);
    }
    return false;
}

template <class P>
bool BufferCache<P>::IsRegionRegistered(VAddr addr, size_t size) {
    const VAddr end_addr = addr + size;
    const u64 page_end = Common::DivCeil(end_addr, YUZU_PAGESIZE);
    for (u64 page = addr >> YUZU_PAGEBITS; page < page_end;) {
        const BufferId buffer_id = page_table[page];
        if (!buffer_id) {
            ++page;
            continue;
        }
        Buffer& buffer = slot_buffers[buffer_id];
        const VAddr buf_start_addr = buffer.CpuAddr();
        const VAddr buf_end_addr = buf_start_addr + buffer.SizeBytes();
        if (buf_start_addr < end_addr && addr < buf_end_addr) {
            return true;
        }
        page = Common::DivCeil(end_addr, YUZU_PAGESIZE);
    }
    return false;
}

template <class P>
bool BufferCache<P>::IsRegionCpuModified(VAddr addr, size_t size) {
    const u64 page_end = Common::DivCeil(addr + size, YUZU_PAGESIZE);
    for (u64 page = addr >> YUZU_PAGEBITS; page < page_end;) {
        const BufferId image_id = page_table[page];
        if (!image_id) {
            ++page;
            continue;
        }
        Buffer& buffer = slot_buffers[image_id];
        if (buffer.IsRegionCpuModified(addr, size)) {
            return true;
        }
        const VAddr end_addr = buffer.CpuAddr() + buffer.SizeBytes();
        page = Common::DivCeil(end_addr, YUZU_PAGESIZE);
    }
    return false;
}

template <class P>
void BufferCache<P>::BindHostIndexBuffer() {
    Buffer& buffer = slot_buffers[index_buffer.buffer_id];
    TouchBuffer(buffer, index_buffer.buffer_id);
    const u32 offset = buffer.Offset(index_buffer.cpu_addr);
    const u32 size = index_buffer.size;
    const auto& draw_state = maxwell3d->draw_manager->GetDrawState();
    if (!draw_state.inline_index_draw_indexes.empty()) {
        if constexpr (USE_MEMORY_MAPS) {
            auto upload_staging = runtime.UploadStagingBuffer(size);
            std::array<BufferCopy, 1> copies{
                {BufferCopy{.src_offset = upload_staging.offset, .dst_offset = 0, .size = size}}};
            std::memcpy(upload_staging.mapped_span.data(),
                        draw_state.inline_index_draw_indexes.data(), size);
            runtime.CopyBuffer(buffer, upload_staging.buffer, copies);
        } else {
            buffer.ImmediateUpload(0, draw_state.inline_index_draw_indexes);
        }
    } else {
        SynchronizeBuffer(buffer, index_buffer.cpu_addr, size);
    }
    if constexpr (HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT) {
        const u32 new_offset =
            offset + draw_state.index_buffer.first * draw_state.index_buffer.FormatSizeInBytes();
        runtime.BindIndexBuffer(buffer, new_offset, size);
    } else {
        runtime.BindIndexBuffer(draw_state.topology, draw_state.index_buffer.format,
                                draw_state.index_buffer.first, draw_state.index_buffer.count,
                                buffer, offset, size);
    }
}

template <class P>
void BufferCache<P>::BindHostVertexBuffers() {
    auto& flags = maxwell3d->dirty.flags;
    for (u32 index = 0; index < NUM_VERTEX_BUFFERS; ++index) {
        const Binding& binding = vertex_buffers[index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        TouchBuffer(buffer, binding.buffer_id);
        SynchronizeBuffer(buffer, binding.cpu_addr, binding.size);
        if (!flags[Dirty::VertexBuffer0 + index]) {
            continue;
        }
        flags[Dirty::VertexBuffer0 + index] = false;

        const u32 stride = maxwell3d->regs.vertex_streams[index].stride;
        const u32 offset = buffer.Offset(binding.cpu_addr);
        runtime.BindVertexBuffer(index, buffer, offset, binding.size, stride);
    }
}

template <class P>
void BufferCache<P>::BindHostDrawIndirectBuffers() {
    const auto bind_buffer = [this](const Binding& binding) {
        Buffer& buffer = slot_buffers[binding.buffer_id];
        TouchBuffer(buffer, binding.buffer_id);
        SynchronizeBuffer(buffer, binding.cpu_addr, binding.size);
    };
    if (current_draw_indirect->include_count) {
        bind_buffer(count_buffer_binding);
    }
    bind_buffer(indirect_buffer_binding);
}

template <class P>
void BufferCache<P>::BindHostGraphicsUniformBuffers(size_t stage) {
    u32 dirty = ~0U;
    if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
        dirty = std::exchange(dirty_uniform_buffers[stage], 0);
    }
    u32 binding_index = 0;
    ForEachEnabledBit(enabled_uniform_buffer_masks[stage], [&](u32 index) {
        const bool needs_bind = ((dirty >> index) & 1) != 0;
        BindHostGraphicsUniformBuffer(stage, index, binding_index, needs_bind);
        if constexpr (NEEDS_BIND_UNIFORM_INDEX) {
            ++binding_index;
        }
    });
}

template <class P>
void BufferCache<P>::BindHostGraphicsUniformBuffer(size_t stage, u32 index, u32 binding_index,
                                                   bool needs_bind) {
    const Binding& binding = uniform_buffers[stage][index];
    const VAddr cpu_addr = binding.cpu_addr;
    const u32 size = std::min(binding.size, (*uniform_buffer_sizes)[stage][index]);
    Buffer& buffer = slot_buffers[binding.buffer_id];
    TouchBuffer(buffer, binding.buffer_id);
    const bool use_fast_buffer = binding.buffer_id != NULL_BUFFER_ID &&
                                 size <= uniform_buffer_skip_cache_size &&
                                 !buffer.IsRegionGpuModified(cpu_addr, size);
    if (use_fast_buffer) {
        if constexpr (IS_OPENGL) {
            if (runtime.HasFastBufferSubData()) {
                // Fast path for Nvidia
                const bool should_fast_bind =
                    !HasFastUniformBufferBound(stage, binding_index) ||
                    uniform_buffer_binding_sizes[stage][binding_index] != size;
                if (should_fast_bind) {
                    // We only have to bind when the currently bound buffer is not the fast version
                    fast_bound_uniform_buffers[stage] |= 1U << binding_index;
                    uniform_buffer_binding_sizes[stage][binding_index] = size;
                    runtime.BindFastUniformBuffer(stage, binding_index, size);
                }
                const auto span = ImmediateBufferWithData(cpu_addr, size);
                runtime.PushFastUniformBuffer(stage, binding_index, span);
                return;
            }
        }
        if constexpr (IS_OPENGL) {
            fast_bound_uniform_buffers[stage] |= 1U << binding_index;
            uniform_buffer_binding_sizes[stage][binding_index] = size;
        }
        // Stream buffer path to avoid stalling on non-Nvidia drivers or Vulkan
        const std::span<u8> span = runtime.BindMappedUniformBuffer(stage, binding_index, size);
        cpu_memory.ReadBlockUnsafe(cpu_addr, span.data(), size);
        return;
    }
    // Classic cached path
    const bool sync_cached = SynchronizeBuffer(buffer, cpu_addr, size);
    if (sync_cached) {
        ++uniform_cache_hits[0];
    }
    ++uniform_cache_shots[0];

    // Skip binding if it's not needed and if the bound buffer is not the fast version
    // This exists to avoid instances where the fast buffer is bound and a GPU write happens
    needs_bind |= HasFastUniformBufferBound(stage, binding_index);
    if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
        needs_bind |= uniform_buffer_binding_sizes[stage][binding_index] != size;
    }
    if (!needs_bind) {
        return;
    }
    const u32 offset = buffer.Offset(cpu_addr);
    if constexpr (IS_OPENGL) {
        // Fast buffer will be unbound
        fast_bound_uniform_buffers[stage] &= ~(1U << binding_index);

        // Mark the index as dirty if offset doesn't match
        const bool is_copy_bind = offset != 0 && !runtime.SupportsNonZeroUniformOffset();
        dirty_uniform_buffers[stage] |= (is_copy_bind ? 1U : 0U) << index;
    }
    if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
        uniform_buffer_binding_sizes[stage][binding_index] = size;
    }
    if constexpr (NEEDS_BIND_UNIFORM_INDEX) {
        runtime.BindUniformBuffer(stage, binding_index, buffer, offset, size);
    } else {
        runtime.BindUniformBuffer(buffer, offset, size);
    }
}

template <class P>
void BufferCache<P>::BindHostGraphicsStorageBuffers(size_t stage) {
    u32 binding_index = 0;
    ForEachEnabledBit(enabled_storage_buffers[stage], [&](u32 index) {
        const Binding& binding = storage_buffers[stage][index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        TouchBuffer(buffer, binding.buffer_id);
        const u32 size = binding.size;
        SynchronizeBuffer(buffer, binding.cpu_addr, size);

        const u32 offset = buffer.Offset(binding.cpu_addr);
        const bool is_written = ((written_storage_buffers[stage] >> index) & 1) != 0;
        if constexpr (NEEDS_BIND_STORAGE_INDEX) {
            runtime.BindStorageBuffer(stage, binding_index, buffer, offset, size, is_written);
            ++binding_index;
        } else {
            runtime.BindStorageBuffer(buffer, offset, size, is_written);
        }
    });
}

template <class P>
void BufferCache<P>::BindHostGraphicsTextureBuffers(size_t stage) {
    ForEachEnabledBit(enabled_texture_buffers[stage], [&](u32 index) {
        const TextureBufferBinding& binding = texture_buffers[stage][index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        const u32 size = binding.size;
        SynchronizeBuffer(buffer, binding.cpu_addr, size);

        const u32 offset = buffer.Offset(binding.cpu_addr);
        const PixelFormat format = binding.format;
        if constexpr (SEPARATE_IMAGE_BUFFERS_BINDINGS) {
            if (((image_texture_buffers[stage] >> index) & 1) != 0) {
                runtime.BindImageBuffer(buffer, offset, size, format);
            } else {
                runtime.BindTextureBuffer(buffer, offset, size, format);
            }
        } else {
            runtime.BindTextureBuffer(buffer, offset, size, format);
        }
    });
}

template <class P>
void BufferCache<P>::BindHostTransformFeedbackBuffers() {
    if (maxwell3d->regs.transform_feedback_enabled == 0) {
        return;
    }
    for (u32 index = 0; index < NUM_TRANSFORM_FEEDBACK_BUFFERS; ++index) {
        const Binding& binding = transform_feedback_buffers[index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        TouchBuffer(buffer, binding.buffer_id);
        const u32 size = binding.size;
        SynchronizeBuffer(buffer, binding.cpu_addr, size);

        const u32 offset = buffer.Offset(binding.cpu_addr);
        runtime.BindTransformFeedbackBuffer(index, buffer, offset, size);
    }
}

template <class P>
void BufferCache<P>::BindHostComputeUniformBuffers() {
    if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
        // Mark all uniform buffers as dirty
        dirty_uniform_buffers.fill(~u32{0});
        fast_bound_uniform_buffers.fill(0);
    }
    u32 binding_index = 0;
    ForEachEnabledBit(enabled_compute_uniform_buffer_mask, [&](u32 index) {
        const Binding& binding = compute_uniform_buffers[index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        TouchBuffer(buffer, binding.buffer_id);
        const u32 size = std::min(binding.size, (*compute_uniform_buffer_sizes)[index]);
        SynchronizeBuffer(buffer, binding.cpu_addr, size);

        const u32 offset = buffer.Offset(binding.cpu_addr);
        if constexpr (NEEDS_BIND_UNIFORM_INDEX) {
            runtime.BindComputeUniformBuffer(binding_index, buffer, offset, size);
            ++binding_index;
        } else {
            runtime.BindUniformBuffer(buffer, offset, size);
        }
    });
}

template <class P>
void BufferCache<P>::BindHostComputeStorageBuffers() {
    u32 binding_index = 0;
    ForEachEnabledBit(enabled_compute_storage_buffers, [&](u32 index) {
        const Binding& binding = compute_storage_buffers[index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        TouchBuffer(buffer, binding.buffer_id);
        const u32 size = binding.size;
        SynchronizeBuffer(buffer, binding.cpu_addr, size);

        const u32 offset = buffer.Offset(binding.cpu_addr);
        const bool is_written = ((written_compute_storage_buffers >> index) & 1) != 0;
        if constexpr (NEEDS_BIND_STORAGE_INDEX) {
            runtime.BindComputeStorageBuffer(binding_index, buffer, offset, size, is_written);
            ++binding_index;
        } else {
            runtime.BindStorageBuffer(buffer, offset, size, is_written);
        }
    });
}

template <class P>
void BufferCache<P>::BindHostComputeTextureBuffers() {
    ForEachEnabledBit(enabled_compute_texture_buffers, [&](u32 index) {
        const TextureBufferBinding& binding = compute_texture_buffers[index];
        Buffer& buffer = slot_buffers[binding.buffer_id];
        const u32 size = binding.size;
        SynchronizeBuffer(buffer, binding.cpu_addr, size);

        const u32 offset = buffer.Offset(binding.cpu_addr);
        const PixelFormat format = binding.format;
        if constexpr (SEPARATE_IMAGE_BUFFERS_BINDINGS) {
            if (((image_compute_texture_buffers >> index) & 1) != 0) {
                runtime.BindImageBuffer(buffer, offset, size, format);
            } else {
                runtime.BindTextureBuffer(buffer, offset, size, format);
            }
        } else {
            runtime.BindTextureBuffer(buffer, offset, size, format);
        }
    });
}

template <class P>
void BufferCache<P>::DoUpdateGraphicsBuffers(bool is_indexed) {
    do {
        has_deleted_buffers = false;
        if (is_indexed) {
            UpdateIndexBuffer();
        }
        UpdateVertexBuffers();
        UpdateTransformFeedbackBuffers();
        for (size_t stage = 0; stage < NUM_STAGES; ++stage) {
            UpdateUniformBuffers(stage);
            UpdateStorageBuffers(stage);
            UpdateTextureBuffers(stage);
        }
        if (current_draw_indirect) {
            UpdateDrawIndirect();
        }
    } while (has_deleted_buffers);
}

template <class P>
void BufferCache<P>::DoUpdateComputeBuffers() {
    UpdateComputeUniformBuffers();
    UpdateComputeStorageBuffers();
    UpdateComputeTextureBuffers();
}

template <class P>
void BufferCache<P>::UpdateIndexBuffer() {
    // We have to check for the dirty flags and index count
    // The index count is currently changed without updating the dirty flags
    const auto& draw_state = maxwell3d->draw_manager->GetDrawState();
    const auto& index_array = draw_state.index_buffer;
    auto& flags = maxwell3d->dirty.flags;
    if (!flags[Dirty::IndexBuffer]) {
        return;
    }
    flags[Dirty::IndexBuffer] = false;
    last_index_count = index_array.count;
    if (!draw_state.inline_index_draw_indexes.empty()) {
        auto inline_index_size = static_cast<u32>(draw_state.inline_index_draw_indexes.size());
        index_buffer = Binding{
            .cpu_addr = 0,
            .size = inline_index_size,
            .buffer_id = CreateBuffer(0, inline_index_size),
        };
        return;
    }
    const GPUVAddr gpu_addr_begin = index_array.StartAddress();
    const GPUVAddr gpu_addr_end = index_array.EndAddress();
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr_begin);
    const u32 address_size = static_cast<u32>(gpu_addr_end - gpu_addr_begin);
    const u32 draw_size = (index_array.count + index_array.first) * index_array.FormatSizeInBytes();
    const u32 size = std::min(address_size, draw_size);
    if (size == 0 || !cpu_addr) {
        index_buffer = NULL_BINDING;
        return;
    }
    index_buffer = Binding{
        .cpu_addr = *cpu_addr,
        .size = size,
        .buffer_id = FindBuffer(*cpu_addr, size),
    };
}

template <class P>
void BufferCache<P>::UpdateVertexBuffers() {
    auto& flags = maxwell3d->dirty.flags;
    if (!maxwell3d->dirty.flags[Dirty::VertexBuffers]) {
        return;
    }
    flags[Dirty::VertexBuffers] = false;

    for (u32 index = 0; index < NUM_VERTEX_BUFFERS; ++index) {
        UpdateVertexBuffer(index);
    }
}

template <class P>
void BufferCache<P>::UpdateVertexBuffer(u32 index) {
    if (!maxwell3d->dirty.flags[Dirty::VertexBuffer0 + index]) {
        return;
    }
    const auto& array = maxwell3d->regs.vertex_streams[index];
    const auto& limit = maxwell3d->regs.vertex_stream_limits[index];
    const GPUVAddr gpu_addr_begin = array.Address();
    const GPUVAddr gpu_addr_end = limit.Address() + 1;
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr_begin);
    u32 address_size = static_cast<u32>(
        std::min(gpu_addr_end - gpu_addr_begin, static_cast<u64>(std::numeric_limits<u32>::max())));
    if (array.enable == 0 || address_size == 0 || !cpu_addr) {
        vertex_buffers[index] = NULL_BINDING;
        return;
    }
    if (!gpu_memory->IsWithinGPUAddressRange(gpu_addr_end)) {
        address_size =
            static_cast<u32>(gpu_memory->MaxContinuousRange(gpu_addr_begin, address_size));
    }
    const u32 size = address_size; // TODO: Analyze stride and number of vertices
    vertex_buffers[index] = Binding{
        .cpu_addr = *cpu_addr,
        .size = size,
        .buffer_id = FindBuffer(*cpu_addr, size),
    };
}

template <class P>
void BufferCache<P>::UpdateDrawIndirect() {
    const auto update = [this](GPUVAddr gpu_addr, size_t size, Binding& binding) {
        const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
        if (!cpu_addr) {
            binding = NULL_BINDING;
            return;
        }
        binding = Binding{
            .cpu_addr = *cpu_addr,
            .size = static_cast<u32>(size),
            .buffer_id = FindBuffer(*cpu_addr, static_cast<u32>(size)),
        };
    };
    if (current_draw_indirect->include_count) {
        update(current_draw_indirect->count_start_address, sizeof(u32), count_buffer_binding);
    }
    update(current_draw_indirect->indirect_start_address, current_draw_indirect->buffer_size,
           indirect_buffer_binding);
}

template <class P>
void BufferCache<P>::UpdateUniformBuffers(size_t stage) {
    ForEachEnabledBit(enabled_uniform_buffer_masks[stage], [&](u32 index) {
        Binding& binding = uniform_buffers[stage][index];
        if (binding.buffer_id) {
            // Already updated
            return;
        }
        // Mark as dirty
        if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
            dirty_uniform_buffers[stage] |= 1U << index;
        }
        // Resolve buffer
        binding.buffer_id = FindBuffer(binding.cpu_addr, binding.size);
    });
}

template <class P>
void BufferCache<P>::UpdateStorageBuffers(size_t stage) {
    const u32 written_mask = written_storage_buffers[stage];
    ForEachEnabledBit(enabled_storage_buffers[stage], [&](u32 index) {
        // Resolve buffer
        Binding& binding = storage_buffers[stage][index];
        const BufferId buffer_id = FindBuffer(binding.cpu_addr, binding.size);
        binding.buffer_id = buffer_id;
        // Mark buffer as written if needed
        if (((written_mask >> index) & 1) != 0) {
            MarkWrittenBuffer(buffer_id, binding.cpu_addr, binding.size);
        }
    });
}

template <class P>
void BufferCache<P>::UpdateTextureBuffers(size_t stage) {
    ForEachEnabledBit(enabled_texture_buffers[stage], [&](u32 index) {
        Binding& binding = texture_buffers[stage][index];
        binding.buffer_id = FindBuffer(binding.cpu_addr, binding.size);
        // Mark buffer as written if needed
        if (((written_texture_buffers[stage] >> index) & 1) != 0) {
            MarkWrittenBuffer(binding.buffer_id, binding.cpu_addr, binding.size);
        }
    });
}

template <class P>
void BufferCache<P>::UpdateTransformFeedbackBuffers() {
    if (maxwell3d->regs.transform_feedback_enabled == 0) {
        return;
    }
    for (u32 index = 0; index < NUM_TRANSFORM_FEEDBACK_BUFFERS; ++index) {
        UpdateTransformFeedbackBuffer(index);
    }
}

template <class P>
void BufferCache<P>::UpdateTransformFeedbackBuffer(u32 index) {
    const auto& binding = maxwell3d->regs.transform_feedback.buffers[index];
    const GPUVAddr gpu_addr = binding.Address() + binding.start_offset;
    const u32 size = binding.size;
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
    if (binding.enable == 0 || size == 0 || !cpu_addr) {
        transform_feedback_buffers[index] = NULL_BINDING;
        return;
    }
    const BufferId buffer_id = FindBuffer(*cpu_addr, size);
    transform_feedback_buffers[index] = Binding{
        .cpu_addr = *cpu_addr,
        .size = size,
        .buffer_id = buffer_id,
    };
    MarkWrittenBuffer(buffer_id, *cpu_addr, size);
}

template <class P>
void BufferCache<P>::UpdateComputeUniformBuffers() {
    ForEachEnabledBit(enabled_compute_uniform_buffer_mask, [&](u32 index) {
        Binding& binding = compute_uniform_buffers[index];
        binding = NULL_BINDING;
        const auto& launch_desc = kepler_compute->launch_description;
        if (((launch_desc.const_buffer_enable_mask >> index) & 1) != 0) {
            const auto& cbuf = launch_desc.const_buffer_config[index];
            const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(cbuf.Address());
            if (cpu_addr) {
                binding.cpu_addr = *cpu_addr;
                binding.size = cbuf.size;
            }
        }
        binding.buffer_id = FindBuffer(binding.cpu_addr, binding.size);
    });
}

template <class P>
void BufferCache<P>::UpdateComputeStorageBuffers() {
    ForEachEnabledBit(enabled_compute_storage_buffers, [&](u32 index) {
        // Resolve buffer
        Binding& binding = compute_storage_buffers[index];
        binding.buffer_id = FindBuffer(binding.cpu_addr, binding.size);
        // Mark as written if needed
        if (((written_compute_storage_buffers >> index) & 1) != 0) {
            MarkWrittenBuffer(binding.buffer_id, binding.cpu_addr, binding.size);
        }
    });
}

template <class P>
void BufferCache<P>::UpdateComputeTextureBuffers() {
    ForEachEnabledBit(enabled_compute_texture_buffers, [&](u32 index) {
        Binding& binding = compute_texture_buffers[index];
        binding.buffer_id = FindBuffer(binding.cpu_addr, binding.size);
        // Mark as written if needed
        if (((written_compute_texture_buffers >> index) & 1) != 0) {
            MarkWrittenBuffer(binding.buffer_id, binding.cpu_addr, binding.size);
        }
    });
}

template <class P>
void BufferCache<P>::MarkWrittenBuffer(BufferId buffer_id, VAddr cpu_addr, u32 size) {
    Buffer& buffer = slot_buffers[buffer_id];
    buffer.MarkRegionAsGpuModified(cpu_addr, size);

    const IntervalType base_interval{cpu_addr, cpu_addr + size};
    common_ranges.add(base_interval);

    const bool is_async = Settings::values.use_asynchronous_gpu_emulation.GetValue();
    if (!is_async) {
        return;
    }
    uncommitted_ranges.add(base_interval);
}

template <class P>
BufferId BufferCache<P>::FindBuffer(VAddr cpu_addr, u32 size) {
    if (cpu_addr == 0) {
        return NULL_BUFFER_ID;
    }
    const u64 page = cpu_addr >> YUZU_PAGEBITS;
    const BufferId buffer_id = page_table[page];
    if (!buffer_id) {
        return CreateBuffer(cpu_addr, size);
    }
    const Buffer& buffer = slot_buffers[buffer_id];
    if (buffer.IsInBounds(cpu_addr, size)) {
        return buffer_id;
    }
    return CreateBuffer(cpu_addr, size);
}

template <class P>
typename BufferCache<P>::OverlapResult BufferCache<P>::ResolveOverlaps(VAddr cpu_addr,
                                                                       u32 wanted_size) {
    static constexpr int STREAM_LEAP_THRESHOLD = 16;
    std::vector<BufferId> overlap_ids;
    VAddr begin = cpu_addr;
    VAddr end = cpu_addr + wanted_size;
    int stream_score = 0;
    bool has_stream_leap = false;
    if (begin == 0) {
        return OverlapResult{
            .ids = std::move(overlap_ids),
            .begin = begin,
            .end = end,
            .has_stream_leap = has_stream_leap,
        };
    }
    for (; cpu_addr >> YUZU_PAGEBITS < Common::DivCeil(end, YUZU_PAGESIZE);
         cpu_addr += YUZU_PAGESIZE) {
        const BufferId overlap_id = page_table[cpu_addr >> YUZU_PAGEBITS];
        if (!overlap_id) {
            continue;
        }
        Buffer& overlap = slot_buffers[overlap_id];
        if (overlap.IsPicked()) {
            continue;
        }
        overlap_ids.push_back(overlap_id);
        overlap.Pick();
        const VAddr overlap_cpu_addr = overlap.CpuAddr();
        const bool expands_left = overlap_cpu_addr < begin;
        if (expands_left) {
            cpu_addr = begin = overlap_cpu_addr;
        }
        const VAddr overlap_end = overlap_cpu_addr + overlap.SizeBytes();
        const bool expands_right = overlap_end > end;
        if (overlap_end > end) {
            end = overlap_end;
        }
        stream_score += overlap.StreamScore();
        if (stream_score > STREAM_LEAP_THRESHOLD && !has_stream_leap) {
            // When this memory region has been joined a bunch of times, we assume it's being used
            // as a stream buffer. Increase the size to skip constantly recreating buffers.
            has_stream_leap = true;
            if (expands_right) {
                begin -= YUZU_PAGESIZE * 256;
                cpu_addr = begin;
            }
            if (expands_left) {
                end += YUZU_PAGESIZE * 256;
            }
        }
    }
    return OverlapResult{
        .ids = std::move(overlap_ids),
        .begin = begin,
        .end = end,
        .has_stream_leap = has_stream_leap,
    };
}

template <class P>
void BufferCache<P>::JoinOverlap(BufferId new_buffer_id, BufferId overlap_id,
                                 bool accumulate_stream_score) {
    Buffer& new_buffer = slot_buffers[new_buffer_id];
    Buffer& overlap = slot_buffers[overlap_id];
    if (accumulate_stream_score) {
        new_buffer.IncreaseStreamScore(overlap.StreamScore() + 1);
    }
    std::vector<BufferCopy> copies;
    const size_t dst_base_offset = overlap.CpuAddr() - new_buffer.CpuAddr();
    overlap.ForEachDownloadRange([&](u64 begin, u64 range_size) {
        copies.push_back(BufferCopy{
            .src_offset = begin,
            .dst_offset = dst_base_offset + begin,
            .size = range_size,
        });
        new_buffer.UnmarkRegionAsCpuModified(begin, range_size);
        new_buffer.MarkRegionAsGpuModified(begin, range_size);
    });
    if (!copies.empty()) {
        runtime.CopyBuffer(slot_buffers[new_buffer_id], overlap, copies);
    }
    DeleteBuffer(overlap_id);
}

template <class P>
BufferId BufferCache<P>::CreateBuffer(VAddr cpu_addr, u32 wanted_size) {
    const OverlapResult overlap = ResolveOverlaps(cpu_addr, wanted_size);
    const u32 size = static_cast<u32>(overlap.end - overlap.begin);
    const BufferId new_buffer_id = slot_buffers.insert(runtime, rasterizer, overlap.begin, size);
    auto& new_buffer = slot_buffers[new_buffer_id];
    runtime.ClearBuffer(new_buffer, 0, new_buffer.SizeBytes(), 0);
    for (const BufferId overlap_id : overlap.ids) {
        JoinOverlap(new_buffer_id, overlap_id, !overlap.has_stream_leap);
    }
    Register(new_buffer_id);
    TouchBuffer(slot_buffers[new_buffer_id], new_buffer_id);
    return new_buffer_id;
}

template <class P>
void BufferCache<P>::Register(BufferId buffer_id) {
    ChangeRegister<true>(buffer_id);
}

template <class P>
void BufferCache<P>::Unregister(BufferId buffer_id) {
    ChangeRegister<false>(buffer_id);
}

template <class P>
template <bool insert>
void BufferCache<P>::ChangeRegister(BufferId buffer_id) {
    Buffer& buffer = slot_buffers[buffer_id];
    const auto size = buffer.SizeBytes();
    if (insert) {
        total_used_memory += Common::AlignUp(size, 1024);
        buffer.setLRUID(lru_cache.Insert(buffer_id, frame_tick));
    } else {
        total_used_memory -= Common::AlignUp(size, 1024);
        lru_cache.Free(buffer.getLRUID());
    }
    const VAddr cpu_addr_begin = buffer.CpuAddr();
    const VAddr cpu_addr_end = cpu_addr_begin + size;
    const u64 page_begin = cpu_addr_begin / YUZU_PAGESIZE;
    const u64 page_end = Common::DivCeil(cpu_addr_end, YUZU_PAGESIZE);
    for (u64 page = page_begin; page != page_end; ++page) {
        if constexpr (insert) {
            page_table[page] = buffer_id;
        } else {
            page_table[page] = BufferId{};
        }
    }
}

template <class P>
void BufferCache<P>::TouchBuffer(Buffer& buffer, BufferId buffer_id) noexcept {
    if (buffer_id != NULL_BUFFER_ID) {
        lru_cache.Touch(buffer.getLRUID(), frame_tick);
    }
}

template <class P>
bool BufferCache<P>::SynchronizeBuffer(Buffer& buffer, VAddr cpu_addr, u32 size) {
    if (buffer.CpuAddr() == 0) {
        return true;
    }
    return SynchronizeBufferImpl(buffer, cpu_addr, size);
}

template <class P>
bool BufferCache<P>::SynchronizeBufferImpl(Buffer& buffer, VAddr cpu_addr, u32 size) {
    boost::container::small_vector<BufferCopy, 4> copies;
    u64 total_size_bytes = 0;
    u64 largest_copy = 0;
    buffer.ForEachUploadRange(cpu_addr, size, [&](u64 range_offset, u64 range_size) {
        copies.push_back(BufferCopy{
            .src_offset = total_size_bytes,
            .dst_offset = range_offset,
            .size = range_size,
        });
        total_size_bytes += range_size;
        largest_copy = std::max(largest_copy, range_size);
    });
    if (total_size_bytes == 0) {
        return true;
    }
    const std::span<BufferCopy> copies_span(copies.data(), copies.size());
    UploadMemory(buffer, total_size_bytes, largest_copy, copies_span);
    return false;
}

template <class P>
void BufferCache<P>::UploadMemory(Buffer& buffer, u64 total_size_bytes, u64 largest_copy,
                                  std::span<BufferCopy> copies) {
    if constexpr (USE_MEMORY_MAPS) {
        MappedUploadMemory(buffer, total_size_bytes, copies);
    } else {
        ImmediateUploadMemory(buffer, largest_copy, copies);
    }
}

template <class P>
void BufferCache<P>::ImmediateUploadMemory(Buffer& buffer, u64 largest_copy,
                                           std::span<const BufferCopy> copies) {
    std::span<u8> immediate_buffer;
    for (const BufferCopy& copy : copies) {
        std::span<const u8> upload_span;
        const VAddr cpu_addr = buffer.CpuAddr() + copy.dst_offset;
        if (IsRangeGranular(cpu_addr, copy.size)) {
            upload_span = std::span(cpu_memory.GetPointer(cpu_addr), copy.size);
        } else {
            if (immediate_buffer.empty()) {
                immediate_buffer = ImmediateBuffer(largest_copy);
            }
            cpu_memory.ReadBlockUnsafe(cpu_addr, immediate_buffer.data(), copy.size);
            upload_span = immediate_buffer.subspan(0, copy.size);
        }
        buffer.ImmediateUpload(copy.dst_offset, upload_span);
    }
}

template <class P>
void BufferCache<P>::MappedUploadMemory(Buffer& buffer, u64 total_size_bytes,
                                        std::span<BufferCopy> copies) {
    auto upload_staging = runtime.UploadStagingBuffer(total_size_bytes);
    const std::span<u8> staging_pointer = upload_staging.mapped_span;
    for (BufferCopy& copy : copies) {
        u8* const src_pointer = staging_pointer.data() + copy.src_offset;
        const VAddr cpu_addr = buffer.CpuAddr() + copy.dst_offset;
        cpu_memory.ReadBlockUnsafe(cpu_addr, src_pointer, copy.size);

        // Apply the staging offset
        copy.src_offset += upload_staging.offset;
    }
    runtime.CopyBuffer(buffer, upload_staging.buffer, copies);
}

template <class P>
bool BufferCache<P>::InlineMemory(VAddr dest_address, size_t copy_size,
                                  std::span<const u8> inlined_buffer) {
    const bool is_dirty = IsRegionRegistered(dest_address, copy_size);
    if (!is_dirty) {
        return false;
    }
    if (!IsRegionGpuModified(dest_address, copy_size)) {
        return false;
    }

    const IntervalType subtract_interval{dest_address, dest_address + copy_size};
    ClearDownload(subtract_interval);
    common_ranges.subtract(subtract_interval);

    BufferId buffer_id = FindBuffer(dest_address, static_cast<u32>(copy_size));
    auto& buffer = slot_buffers[buffer_id];
    SynchronizeBuffer(buffer, dest_address, static_cast<u32>(copy_size));

    if constexpr (USE_MEMORY_MAPS) {
        auto upload_staging = runtime.UploadStagingBuffer(copy_size);
        std::array copies{BufferCopy{
            .src_offset = upload_staging.offset,
            .dst_offset = buffer.Offset(dest_address),
            .size = copy_size,
        }};
        u8* const src_pointer = upload_staging.mapped_span.data();
        std::memcpy(src_pointer, inlined_buffer.data(), copy_size);
        runtime.CopyBuffer(buffer, upload_staging.buffer, copies);
    } else {
        buffer.ImmediateUpload(buffer.Offset(dest_address), inlined_buffer.first(copy_size));
    }

    return true;
}

template <class P>
void BufferCache<P>::DownloadBufferMemory(Buffer& buffer) {
    DownloadBufferMemory(buffer, buffer.CpuAddr(), buffer.SizeBytes());
}

template <class P>
void BufferCache<P>::DownloadBufferMemory(Buffer& buffer, VAddr cpu_addr, u64 size) {
    boost::container::small_vector<BufferCopy, 1> copies;
    u64 total_size_bytes = 0;
    u64 largest_copy = 0;
    buffer.ForEachDownloadRangeAndClear(cpu_addr, size, [&](u64 range_offset, u64 range_size) {
        const VAddr buffer_addr = buffer.CpuAddr();
        const auto add_download = [&](VAddr start, VAddr end) {
            const u64 new_offset = start - buffer_addr;
            const u64 new_size = end - start;
            copies.push_back(BufferCopy{
                .src_offset = new_offset,
                .dst_offset = total_size_bytes,
                .size = new_size,
            });
            // Align up to avoid cache conflicts
            constexpr u64 align = 256ULL;
            constexpr u64 mask = ~(align - 1ULL);
            total_size_bytes += (new_size + align - 1) & mask;
            largest_copy = std::max(largest_copy, new_size);
        };

        const VAddr start_address = buffer_addr + range_offset;
        const VAddr end_address = start_address + range_size;
        ForEachWrittenRange(start_address, range_size, add_download);
        const IntervalType subtract_interval{start_address, end_address};
        ClearDownload(subtract_interval);
        common_ranges.subtract(subtract_interval);
    });
    if (total_size_bytes == 0) {
        return;
    }
    MICROPROFILE_SCOPE(GPU_DownloadMemory);

    if constexpr (USE_MEMORY_MAPS) {
        auto download_staging = runtime.DownloadStagingBuffer(total_size_bytes);
        const u8* const mapped_memory = download_staging.mapped_span.data();
        const std::span<BufferCopy> copies_span(copies.data(), copies.data() + copies.size());
        for (BufferCopy& copy : copies) {
            // Modify copies to have the staging offset in mind
            copy.dst_offset += download_staging.offset;
        }
        runtime.CopyBuffer(download_staging.buffer, buffer, copies_span);
        runtime.Finish();
        for (const BufferCopy& copy : copies) {
            const VAddr copy_cpu_addr = buffer.CpuAddr() + copy.src_offset;
            // Undo the modified offset
            const u64 dst_offset = copy.dst_offset - download_staging.offset;
            const u8* copy_mapped_memory = mapped_memory + dst_offset;
            cpu_memory.WriteBlockUnsafe(copy_cpu_addr, copy_mapped_memory, copy.size);
        }
    } else {
        const std::span<u8> immediate_buffer = ImmediateBuffer(largest_copy);
        for (const BufferCopy& copy : copies) {
            buffer.ImmediateDownload(copy.src_offset, immediate_buffer.subspan(0, copy.size));
            const VAddr copy_cpu_addr = buffer.CpuAddr() + copy.src_offset;
            cpu_memory.WriteBlockUnsafe(copy_cpu_addr, immediate_buffer.data(), copy.size);
        }
    }
}

template <class P>
void BufferCache<P>::DeleteBuffer(BufferId buffer_id) {
    const auto scalar_replace = [buffer_id](Binding& binding) {
        if (binding.buffer_id == buffer_id) {
            binding.buffer_id = BufferId{};
        }
    };
    const auto replace = [scalar_replace](std::span<Binding> bindings) {
        std::ranges::for_each(bindings, scalar_replace);
    };
    scalar_replace(index_buffer);
    replace(vertex_buffers);
    std::ranges::for_each(uniform_buffers, replace);
    std::ranges::for_each(storage_buffers, replace);
    replace(transform_feedback_buffers);
    replace(compute_uniform_buffers);
    replace(compute_storage_buffers);
    std::erase(cached_write_buffer_ids, buffer_id);

    // Mark the whole buffer as CPU written to stop tracking CPU writes
    Buffer& buffer = slot_buffers[buffer_id];
    buffer.MarkRegionAsCpuModified(buffer.CpuAddr(), buffer.SizeBytes());

    Unregister(buffer_id);
    delayed_destruction_ring.Push(std::move(slot_buffers[buffer_id]));
    slot_buffers.erase(buffer_id);

    NotifyBufferDeletion();
}

template <class P>
void BufferCache<P>::NotifyBufferDeletion() {
    if constexpr (HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS) {
        dirty_uniform_buffers.fill(~u32{0});
        uniform_buffer_binding_sizes.fill({});
    }
    auto& flags = maxwell3d->dirty.flags;
    flags[Dirty::IndexBuffer] = true;
    flags[Dirty::VertexBuffers] = true;
    for (u32 index = 0; index < NUM_VERTEX_BUFFERS; ++index) {
        flags[Dirty::VertexBuffer0 + index] = true;
    }
    has_deleted_buffers = true;
}

template <class P>
typename BufferCache<P>::Binding BufferCache<P>::StorageBufferBinding(GPUVAddr ssbo_addr,
                                                                      u32 cbuf_index,
                                                                      bool is_written) const {
    const GPUVAddr gpu_addr = gpu_memory->Read<u64>(ssbo_addr);
    const auto size = [&]() {
        const bool is_nvn_cbuf = cbuf_index == 0;
        // The NVN driver buffer (index 0) is known to pack the SSBO address followed by its size.
        if (is_nvn_cbuf) {
            return gpu_memory->Read<u32>(ssbo_addr + 8);
        }
        // Other titles (notably Doom Eternal) may use STG/LDG on buffer addresses in custom defined
        // cbufs, which do not store the sizes adjacent to the addresses, so use the fully
        // mapped buffer size for now.
        const u32 memory_layout_size = static_cast<u32>(gpu_memory->GetMemoryLayoutSize(gpu_addr));
        LOG_INFO(HW_GPU, "Binding storage buffer for cbuf index {}, MemoryLayoutSize 0x{:X}",
                 cbuf_index, memory_layout_size);
        return memory_layout_size;
    }();
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
    if (!cpu_addr || size == 0) {
        LOG_WARNING(HW_GPU, "Failed to find storage buffer for cbuf index {}", cbuf_index);
        return NULL_BINDING;
    }
    const VAddr cpu_end = Common::AlignUp(*cpu_addr + size, Core::Memory::YUZU_PAGESIZE);
    const Binding binding{
        .cpu_addr = *cpu_addr,
        .size = is_written ? size : static_cast<u32>(cpu_end - *cpu_addr),
        .buffer_id = BufferId{},
    };
    return binding;
}

template <class P>
typename BufferCache<P>::TextureBufferBinding BufferCache<P>::GetTextureBufferBinding(
    GPUVAddr gpu_addr, u32 size, PixelFormat format) {
    const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
    TextureBufferBinding binding;
    if (!cpu_addr || size == 0) {
        binding.cpu_addr = 0;
        binding.size = 0;
        binding.buffer_id = NULL_BUFFER_ID;
        binding.format = PixelFormat::Invalid;
    } else {
        binding.cpu_addr = *cpu_addr;
        binding.size = size;
        binding.buffer_id = BufferId{};
        binding.format = format;
    }
    return binding;
}

template <class P>
std::span<const u8> BufferCache<P>::ImmediateBufferWithData(VAddr cpu_addr, size_t size) {
    u8* const base_pointer = cpu_memory.GetPointer(cpu_addr);
    if (IsRangeGranular(cpu_addr, size) ||
        base_pointer + size == cpu_memory.GetPointer(cpu_addr + size)) {
        return std::span(base_pointer, size);
    } else {
        const std::span<u8> span = ImmediateBuffer(size);
        cpu_memory.ReadBlockUnsafe(cpu_addr, span.data(), size);
        return span;
    }
}

template <class P>
std::span<u8> BufferCache<P>::ImmediateBuffer(size_t wanted_capacity) {
    immediate_buffer_alloc.resize_destructive(wanted_capacity);
    return std::span<u8>(immediate_buffer_alloc.data(), wanted_capacity);
}

template <class P>
bool BufferCache<P>::HasFastUniformBufferBound(size_t stage, u32 binding_index) const noexcept {
    if constexpr (IS_OPENGL) {
        return ((fast_bound_uniform_buffers[stage] >> binding_index) & 1) != 0;
    } else {
        // Only OpenGL has fast uniform buffers
        return false;
    }
}

template <class P>
std::pair<typename BufferCache<P>::Buffer*, u32> BufferCache<P>::GetDrawIndirectCount() {
    auto& buffer = slot_buffers[count_buffer_binding.buffer_id];
    return std::make_pair(&buffer, buffer.Offset(count_buffer_binding.cpu_addr));
}

template <class P>
std::pair<typename BufferCache<P>::Buffer*, u32> BufferCache<P>::GetDrawIndirectBuffer() {
    auto& buffer = slot_buffers[indirect_buffer_binding.buffer_id];
    return std::make_pair(&buffer, buffer.Offset(indirect_buffer_binding.cpu_addr));
}

} // namespace VideoCommon
