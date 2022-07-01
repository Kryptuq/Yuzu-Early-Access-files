// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/core/syncpoint_manager.h"
#include "core/hle/service/nvdrv/devices/nvhost_nvdec_common.h"
#include "core/memory.h"
#include "video_core/host1x/host1x.h"
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

std::unordered_map<DeviceFD, u32> nvhost_nvdec_common::fd_to_id{};
std::deque<u32> nvhost_nvdec_common::syncpts_accumulated{};

nvhost_nvdec_common::nvhost_nvdec_common(Core::System& system_, NvCore::Container& core_,
                                         NvCore::ChannelType channel_type_)
    : nvdevice{system_}, core{core_}, syncpoint_manager{core.GetSyncpointManager()},
      nvmap{core.GetNvMapFile()}, channel_type{channel_type_} {
    if (syncpts_accumulated.empty()) {
        channel_syncpoint = syncpoint_manager.AllocateSyncpoint(false);
    } else {
        channel_syncpoint = syncpts_accumulated.front();
        syncpts_accumulated.pop_front();
    }
}
nvhost_nvdec_common::~nvhost_nvdec_common() {
    syncpts_accumulated.push_back(channel_syncpoint);
}

NvResult nvhost_nvdec_common::SetNVMAPfd(const std::vector<u8>& input) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSetNvmapFD));
    LOG_DEBUG(Service_NVDRV, "called, fd={}", params.nvmap_fd);

    nvmap_fd = params.nvmap_fd;
    return NvResult::Success;
}

NvResult nvhost_nvdec_common::Submit(DeviceFD fd, const std::vector<u8>& input,
                                     std::vector<u8>& output) {
    IoctlSubmit params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSubmit));
    LOG_DEBUG(Service_NVDRV, "called NVDEC Submit, cmd_buffer_count={}", params.cmd_buffer_count);

    // Instantiate param buffers
    std::vector<CommandBuffer> command_buffers(params.cmd_buffer_count);
    std::vector<Reloc> relocs(params.relocation_count);
    std::vector<u32> reloc_shifts(params.relocation_count);
    std::vector<SyncptIncr> syncpt_increments(params.syncpoint_count);
    std::vector<u32> fence_thresholds(params.fence_count);

    // Slice input into their respective buffers
    std::size_t offset = sizeof(IoctlSubmit);
    offset += SliceVectors(input, command_buffers, params.cmd_buffer_count, offset);
    offset += SliceVectors(input, relocs, params.relocation_count, offset);
    offset += SliceVectors(input, reloc_shifts, params.relocation_count, offset);
    offset += SliceVectors(input, syncpt_increments, params.syncpoint_count, offset);
    offset += SliceVectors(input, fence_thresholds, params.fence_count, offset);

    auto& gpu = system.GPU();
    if (gpu.UseNvdec()) {
        for (std::size_t i = 0; i < syncpt_increments.size(); i++) {
            const SyncptIncr& syncpt_incr = syncpt_increments[i];
            fence_thresholds[i] =
                syncpoint_manager.IncrementSyncpointMaxExt(syncpt_incr.id, syncpt_incr.increments);
        }
    }
    for (const auto& cmd_buffer : command_buffers) {
        const auto object = nvmap.GetHandle(cmd_buffer.memory_id);
        ASSERT_OR_EXECUTE(object, return NvResult::InvalidState;);
        Tegra::ChCommandHeaderList cmdlist(cmd_buffer.word_count);
        system.Memory().ReadBlock(object->address + cmd_buffer.offset, cmdlist.data(),
                                  cmdlist.size() * sizeof(u32));
        gpu.PushCommandBuffer(fd_to_id[fd], cmdlist);
    }
    std::memcpy(output.data(), &params, sizeof(IoctlSubmit));
    // Some games expect command_buffers to be written back
    offset = sizeof(IoctlSubmit);
    offset += WriteVectors(output, command_buffers, offset);
    offset += WriteVectors(output, relocs, offset);
    offset += WriteVectors(output, reloc_shifts, offset);
    offset += WriteVectors(output, syncpt_increments, offset);
    offset += WriteVectors(output, fence_thresholds, offset);

    return NvResult::Success;
}

NvResult nvhost_nvdec_common::GetSyncpoint(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetSyncpoint params{};
    std::memcpy(&params, input.data(), sizeof(IoctlGetSyncpoint));
    LOG_DEBUG(Service_NVDRV, "called GetSyncpoint, id={}", params.param);

    // const u32 id{NvCore::SyncpointManager::channel_syncpoints[static_cast<u32>(channel_type)]};
    params.value = channel_syncpoint;
    std::memcpy(output.data(), &params, sizeof(IoctlGetSyncpoint));

    return NvResult::Success;
}

NvResult nvhost_nvdec_common::GetWaitbase(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetWaitbase params{};
    LOG_CRITICAL(Service_NVDRV, "called WAITBASE");
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

    for (auto& cmd_buffer : cmd_buffer_handles) {
        cmd_buffer.map_address = nvmap.PinHandle(cmd_buffer.map_handle);
    }
    std::memcpy(output.data(), &params, sizeof(IoctlMapBuffer));
    std::memcpy(output.data() + sizeof(IoctlMapBuffer), cmd_buffer_handles.data(),
                cmd_buffer_handles.size() * sizeof(MapBufferEntry));

    return NvResult::Success;
}

NvResult nvhost_nvdec_common::UnmapBuffer(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlMapBuffer params{};
    std::memcpy(&params, input.data(), sizeof(IoctlMapBuffer));
    std::vector<MapBufferEntry> cmd_buffer_handles(params.num_entries);

    SliceVectors(input, cmd_buffer_handles, params.num_entries, sizeof(IoctlMapBuffer));
    for (auto& cmd_buffer : cmd_buffer_handles) {
        nvmap.UnpinHandle(cmd_buffer.map_handle);
    }

    std::memset(output.data(), 0, output.size());
    return NvResult::Success;
}

NvResult nvhost_nvdec_common::SetSubmitTimeout(const std::vector<u8>& input,
                                               std::vector<u8>& output) {
    std::memcpy(&submit_timeout, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called");
    return NvResult::Success;
}

Kernel::KEvent* nvhost_nvdec_common::QueryEvent(u32 event_id) {
    LOG_CRITICAL(Service_NVDRV, "Unknown HOSTX1 Event {}", event_id);
    return nullptr;
}

void nvhost_nvdec_common::Reset() {
    fd_to_id.clear();
}

} // namespace Service::Nvidia::Devices
