// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstring>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/devices/nvhost_nvdec_common.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"
#include "core/hle/service/nvdrv/syncpoint_manager.h"
#include "core/memory.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_base.h"

namespace Service::Nvidia::Devices {

namespace {
// Copies count amount of type T from the input vector into the dst vector.
// Returns the number of bytes written into dst.
template <typename T>
std::size_t SliceVectors(const std::vector<u8>& input, std::vector<T>& dst, std::size_t count,
                         std::size_t offset) {
    if (dst.empty()) {
        return 0;
    }
    const size_t bytes_copied = count * sizeof(T);
    std::memcpy(dst.data(), input.data() + offset, bytes_copied);
    return bytes_copied;
}

// Writes the data in src to an offset into the dst vector. The offset is specified in bytes
// Returns the number of bytes written into dst.
template <typename T>
std::size_t WriteVectors(std::vector<u8>& dst, const std::vector<T>& src, std::size_t offset) {
    if (src.empty()) {
        return 0;
    }
    const size_t bytes_copied = src.size() * sizeof(T);
    std::memcpy(dst.data() + offset, src.data(), bytes_copied);
    return bytes_copied;
}
} // Anonymous namespace

nvhost_nvdec_common::nvhost_nvdec_common(Core::System& system_, std::shared_ptr<nvmap> nvmap_dev_,
                                         SyncpointManager& syncpoint_manager_)
    : nvdevice{system_}, nvmap_dev{std::move(nvmap_dev_)}, syncpoint_manager{syncpoint_manager_} {}
nvhost_nvdec_common::~nvhost_nvdec_common() = default;

NvResult nvhost_nvdec_common::SetNVMAPfd(const std::vector<u8>& input) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSetNvmapFD));
    LOG_DEBUG(Service_NVDRV, "called, fd={}", params.nvmap_fd);

    nvmap_fd = params.nvmap_fd;
    return NvResult::Success;
}

NvResult nvhost_nvdec_common::Submit(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSubmit params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSubmit));
    LOG_DEBUG(Service_NVDRV, "called NVDEC Submit, cmd_buffer_count={}", params.cmd_buffer_count);

    // Instantiate param buffers
    std::vector<CommandBuffer> command_buffers(params.cmd_buffer_count);
    std::vector<Reloc> relocs(params.relocation_count);
    std::vector<u32> reloc_shifts(params.relocation_count);
    std::vector<SyncptIncr> syncpt_increments(params.syncpoint_count);
    std::vector<SyncptIncr> wait_checks(params.syncpoint_count);
    std::vector<Fence> fences(params.fence_count);

    // Slice input into their respective buffers
    std::size_t offset = sizeof(IoctlSubmit);
    offset += SliceVectors(input, command_buffers, params.cmd_buffer_count, offset);
    offset += SliceVectors(input, relocs, params.relocation_count, offset);
    offset += SliceVectors(input, reloc_shifts, params.relocation_count, offset);
    offset += SliceVectors(input, syncpt_increments, params.syncpoint_count, offset);
    offset += SliceVectors(input, wait_checks, params.syncpoint_count, offset);
    offset += SliceVectors(input, fences, params.fence_count, offset);

    auto& gpu = system.GPU();
    if (gpu.UseNvdec()) {
        for (std::size_t i = 0; i < syncpt_increments.size(); i++) {
            const SyncptIncr& syncpt_incr = syncpt_increments[i];
            fences[i].id = syncpt_incr.id;
            fences[i].value =
                syncpoint_manager.IncreaseSyncpoint(syncpt_incr.id, syncpt_incr.increments);
        }
    }
    for (const auto& cmd_buffer : command_buffers) {
        const auto object = nvmap_dev->GetObject(cmd_buffer.memory_id);
        ASSERT_OR_EXECUTE(object, return NvResult::InvalidState;);
        Tegra::ChCommandHeaderList cmdlist(cmd_buffer.word_count);
        system.Memory().ReadBlock(object->addr + cmd_buffer.offset, cmdlist.data(),
                                  cmdlist.size() * sizeof(u32));
        gpu.PushCommandBuffer(cmdlist);
    }
    if (gpu.UseNvdec()) {
        fences[0].value = syncpoint_manager.IncreaseSyncpoint(fences[0].id, 1);
        Tegra::ChCommandHeaderList cmdlist{{(4 << 28) | fences[0].id}};
        gpu.PushCommandBuffer(cmdlist);
    }
    std::memcpy(output.data(), &params, sizeof(IoctlSubmit));
    // Some games expect command_buffers to be written back
    offset = sizeof(IoctlSubmit);
    offset += WriteVectors(output, command_buffers, offset);
    offset += WriteVectors(output, relocs, offset);
    offset += WriteVectors(output, reloc_shifts, offset);
    offset += WriteVectors(output, syncpt_increments, offset);
    offset += WriteVectors(output, wait_checks, offset);
    offset += WriteVectors(output, fences, offset);

    return NvResult::Success;
}

NvResult nvhost_nvdec_common::GetSyncpoint(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetSyncpoint params{};
    std::memcpy(&params, input.data(), sizeof(IoctlGetSyncpoint));
    LOG_DEBUG(Service_NVDRV, "called GetSyncpoint, id={}", params.param);

    if (device_syncpoints[params.param] == 0 && system.GPU().UseNvdec()) {
        device_syncpoints[params.param] = syncpoint_manager.AllocateSyncpoint();
    }
    params.value = device_syncpoints[params.param];
    std::memcpy(output.data(), &params, sizeof(IoctlGetSyncpoint));

    return NvResult::Success;
}

NvResult nvhost_nvdec_common::GetWaitbase(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetWaitbase params{};
    std::memcpy(&params, input.data(), sizeof(IoctlGetWaitbase));
    params.value = 0; // Seems to be hard coded at 0
    std::memcpy(output.data(), &params, sizeof(IoctlGetWaitbase));
    return NvResult::Success;
}

NvResult nvhost_nvdec_common::MapBuffer(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlMapBuffer params{};
    std::memcpy(&params, input.data(), sizeof(IoctlMapBuffer));
    std::vector<MapBufferEntry> cmd_buffer_handles(params.num_entries);

    SliceVectors(input, cmd_buffer_handles, params.num_entries, sizeof(IoctlMapBuffer));

    auto& gpu = system.GPU();

    for (auto& cmd_buffer : cmd_buffer_handles) {
        auto object{nvmap_dev->GetObject(cmd_buffer.map_handle)};
        if (!object) {
            LOG_ERROR(Service_NVDRV, "invalid cmd_buffer nvmap_handle={:X}", cmd_buffer.map_handle);
            std::memcpy(output.data(), &params, output.size());
            return NvResult::InvalidState;
        }
        if (object->dma_map_addr == 0) {
            // NVDEC and VIC memory is in the 32-bit address space
            // MapAllocate32 will attempt to map a lower 32-bit value in the shared gpu memory space
            const GPUVAddr low_addr = gpu.MemoryManager().MapAllocate32(object->addr, object->size);
            object->dma_map_addr = static_cast<u32>(low_addr);
            // Ensure that the dma_map_addr is indeed in the lower 32-bit address space.
            ASSERT(object->dma_map_addr == low_addr);
        }
        if (!object->dma_map_addr) {
            LOG_ERROR(Service_NVDRV, "failed to map size={}", object->size);
        } else {
            cmd_buffer.map_address = object->dma_map_addr;
        }
    }
    std::memcpy(output.data(), &params, sizeof(IoctlMapBuffer));
    std::memcpy(output.data() + sizeof(IoctlMapBuffer), cmd_buffer_handles.data(),
                cmd_buffer_handles.size() * sizeof(MapBufferEntry));

    return NvResult::Success;
}

NvResult nvhost_nvdec_common::UnmapBuffer(const std::vector<u8>& input, std::vector<u8>& output) {
    // This is intntionally stubbed.
    // Skip unmapping buffers here, as to not break the continuity of the VP9 reference frame
    // addresses, and risk invalidating data before the async GPU thread is done with it
    std::memset(output.data(), 0, output.size());
    LOG_DEBUG(Service_NVDRV, "(STUBBED) called");
    return NvResult::Success;
}

NvResult nvhost_nvdec_common::SetSubmitTimeout(const std::vector<u8>& input,
                                               std::vector<u8>& output) {
    std::memcpy(&submit_timeout, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called");
    return NvResult::Success;
}

} // namespace Service::Nvidia::Devices
