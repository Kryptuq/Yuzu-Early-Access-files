// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"
#include "core/hle/service/nvdrv/syncpoint_manager.h"
#include "core/memory.h"
#include "video_core/gpu.h"

namespace Service::Nvidia::Devices {
namespace {
Tegra::CommandHeader BuildFenceAction(Tegra::GPU::FenceOperation op, u32 syncpoint_id) {
    Tegra::GPU::FenceAction result{};
    result.op.Assign(op);
    result.syncpoint_id.Assign(syncpoint_id);
    return {result.raw};
}
} // namespace

nvhost_gpu::nvhost_gpu(Core::System& system_, std::shared_ptr<nvmap> nvmap_dev_,
                       SyncpointManager& syncpoint_manager_)
    : nvdevice{system_}, nvmap_dev{std::move(nvmap_dev_)}, syncpoint_manager{syncpoint_manager_} {
    channel_fence.id = syncpoint_manager_.AllocateSyncpoint();
    channel_fence.value = system_.GPU().GetSyncpointValue(channel_fence.id);
}

nvhost_gpu::~nvhost_gpu() = default;

NvResult nvhost_gpu::Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                            std::vector<u8>& output) {
    switch (command.group) {
    case 0x0:
        switch (command.cmd) {
        case 0x3:
            return GetWaitbase(input, output);
        default:
            break;
        }
        break;
    case 'H':
        switch (command.cmd) {
        case 0x1:
            return SetNVMAPfd(input, output);
        case 0x3:
            return ChannelSetTimeout(input, output);
        case 0x8:
            return SubmitGPFIFOBase(input, output, false);
        case 0x9:
            return AllocateObjectContext(input, output);
        case 0xb:
            return ZCullBind(input, output);
        case 0xc:
            return SetErrorNotifier(input, output);
        case 0xd:
            return SetChannelPriority(input, output);
        case 0x1a:
            return AllocGPFIFOEx2(input, output);
        case 0x1b:
            return SubmitGPFIFOBase(input, output, true);
        case 0x1d:
            return ChannelSetTimeslice(input, output);
        default:
            break;
        }
        break;
    case 'G':
        switch (command.cmd) {
        case 0x14:
            return SetClientData(input, output);
        case 0x15:
            return GetClientData(input, output);
        default:
            break;
        }
        break;
    }
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
};

NvResult nvhost_gpu::Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                            const std::vector<u8>& inline_input, std::vector<u8>& output) {
    switch (command.group) {
    case 'H':
        switch (command.cmd) {
        case 0x1b:
            return SubmitGPFIFOBase(input, inline_input, output);
        }
        break;
    }
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_gpu::Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                            std::vector<u8>& output, std::vector<u8>& inline_output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_gpu::OnOpen(DeviceFD fd) {}
void nvhost_gpu::OnClose(DeviceFD fd) {}

NvResult nvhost_gpu::SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd={}", params.nvmap_fd);

    nvmap_fd = params.nvmap_fd;
    return NvResult::Success;
}

NvResult nvhost_gpu::SetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");

    IoctlClientData params{};
    std::memcpy(&params, input.data(), input.size());
    user_data = params.data;
    return NvResult::Success;
}

NvResult nvhost_gpu::GetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");

    IoctlClientData params{};
    std::memcpy(&params, input.data(), input.size());
    params.data = user_data;
    std::memcpy(output.data(), &params, output.size());
    return NvResult::Success;
}

NvResult nvhost_gpu::ZCullBind(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&zcull_params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, gpu_va={:X}, mode={:X}", zcull_params.gpu_va,
              zcull_params.mode);

    std::memcpy(output.data(), &zcull_params, output.size());
    return NvResult::Success;
}

NvResult nvhost_gpu::SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetErrorNotifier params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, offset={:X}, size={:X}, mem={:X}", params.offset,
                params.size, params.mem);

    std::memcpy(output.data(), &params, output.size());
    return NvResult::Success;
}

NvResult nvhost_gpu::SetChannelPriority(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&channel_priority, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "(STUBBED) called, priority={:X}", channel_priority);

    return NvResult::Success;
}

NvResult nvhost_gpu::AllocGPFIFOEx2(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocGpfifoEx2 params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV,
                "(STUBBED) called, num_entries={:X}, flags={:X}, unk0={:X}, "
                "unk1={:X}, unk2={:X}, unk3={:X}",
                params.num_entries, params.flags, params.unk0, params.unk1, params.unk2,
                params.unk3);

    channel_fence.value = system.GPU().GetSyncpointValue(channel_fence.id);

    params.fence_out = channel_fence;

    std::memcpy(output.data(), &params, output.size());
    return NvResult::Success;
}

NvResult nvhost_gpu::AllocateObjectContext(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocObjCtx params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, class_num={:X}, flags={:X}", params.class_num,
                params.flags);

    params.obj_id = 0x0;
    std::memcpy(output.data(), &params, output.size());
    return NvResult::Success;
}

static std::vector<Tegra::CommandHeader> BuildWaitCommandList(Fence fence) {
    return {
        Tegra::BuildCommandHeader(Tegra::BufferMethods::FenceValue, 1,
                                  Tegra::SubmissionMode::Increasing),
        {fence.value},
        Tegra::BuildCommandHeader(Tegra::BufferMethods::FenceAction, 1,
                                  Tegra::SubmissionMode::Increasing),
        BuildFenceAction(Tegra::GPU::FenceOperation::Acquire, fence.id),
    };
}

static std::vector<Tegra::CommandHeader> BuildIncrementCommandList(Fence fence, u32 add_increment) {
    std::vector<Tegra::CommandHeader> result{
        Tegra::BuildCommandHeader(Tegra::BufferMethods::FenceValue, 1,
                                  Tegra::SubmissionMode::Increasing),
        {}};

    for (u32 count = 0; count < add_increment; ++count) {
        result.emplace_back(Tegra::BuildCommandHeader(Tegra::BufferMethods::FenceAction, 1,
                                                      Tegra::SubmissionMode::Increasing));
        result.emplace_back(BuildFenceAction(Tegra::GPU::FenceOperation::Increment, fence.id));
    }

    return result;
}

static std::vector<Tegra::CommandHeader> BuildIncrementWithWfiCommandList(Fence fence,
                                                                          u32 add_increment) {
    std::vector<Tegra::CommandHeader> result{
        Tegra::BuildCommandHeader(Tegra::BufferMethods::WaitForInterrupt, 1,
                                  Tegra::SubmissionMode::Increasing),
        {}};
    const std::vector<Tegra::CommandHeader> increment{
        BuildIncrementCommandList(fence, add_increment)};

    result.insert(result.end(), increment.begin(), increment.end());

    return result;
}

NvResult nvhost_gpu::SubmitGPFIFOImpl(IoctlSubmitGpfifo& params, std::vector<u8>& output,
                                      Tegra::CommandList&& entries) {
    LOG_TRACE(Service_NVDRV, "called, gpfifo={:X}, num_entries={:X}, flags={:X}", params.address,
              params.num_entries, params.flags.raw);

    auto& gpu = system.GPU();

    params.fence_out.id = channel_fence.id;

    if (params.flags.add_wait.Value() &&
        !syncpoint_manager.IsSyncpointExpired(params.fence_out.id, params.fence_out.value)) {
        gpu.PushGPUEntries(Tegra::CommandList{BuildWaitCommandList(params.fence_out)});
    }

    if (params.flags.add_increment.Value() || params.flags.increment.Value()) {
        const u32 increment_value = params.flags.increment.Value() ? params.fence_out.value : 0;
        params.fence_out.value = syncpoint_manager.IncreaseSyncpoint(
            params.fence_out.id, params.AddIncrementValue() + increment_value);
    } else {
        params.fence_out.value = syncpoint_manager.GetSyncpointMax(params.fence_out.id);
    }

    gpu.PushGPUEntries(std::move(entries));

    if (params.flags.add_increment.Value()) {
        if (params.flags.suppress_wfi) {
            gpu.PushGPUEntries(Tegra::CommandList{
                BuildIncrementCommandList(params.fence_out, params.AddIncrementValue())});
        } else {
            gpu.PushGPUEntries(Tegra::CommandList{
                BuildIncrementWithWfiCommandList(params.fence_out, params.AddIncrementValue())});
        }
    }

    std::memcpy(output.data(), &params, sizeof(IoctlSubmitGpfifo));
    return NvResult::Success;
}

NvResult nvhost_gpu::SubmitGPFIFOBase(const std::vector<u8>& input, std::vector<u8>& output,
                                      bool kickoff) {
    if (input.size() < sizeof(IoctlSubmitGpfifo)) {
        UNIMPLEMENTED();
        return NvResult::InvalidSize;
    }
    IoctlSubmitGpfifo params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSubmitGpfifo));
    Tegra::CommandList entries(params.num_entries);

    if (kickoff) {
        system.Memory().ReadBlock(params.address, entries.command_lists.data(),
                                  params.num_entries * sizeof(Tegra::CommandListHeader));
    } else {
        std::memcpy(entries.command_lists.data(), &input[sizeof(IoctlSubmitGpfifo)],
                    params.num_entries * sizeof(Tegra::CommandListHeader));
    }

    return SubmitGPFIFOImpl(params, output, std::move(entries));
}

NvResult nvhost_gpu::SubmitGPFIFOBase(const std::vector<u8>& input,
                                      const std::vector<u8>& input_inline,
                                      std::vector<u8>& output) {
    if (input.size() < sizeof(IoctlSubmitGpfifo)) {
        UNIMPLEMENTED();
        return NvResult::InvalidSize;
    }
    IoctlSubmitGpfifo params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSubmitGpfifo));
    Tegra::CommandList entries(params.num_entries);
    std::memcpy(entries.command_lists.data(), input_inline.data(), input_inline.size());
    return SubmitGPFIFOImpl(params, output, std::move(entries));
}

NvResult nvhost_gpu::GetWaitbase(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetWaitbase params{};
    std::memcpy(&params, input.data(), sizeof(IoctlGetWaitbase));
    LOG_INFO(Service_NVDRV, "called, unknown=0x{:X}", params.unknown);

    params.value = 0; // Seems to be hard coded at 0
    std::memcpy(output.data(), &params, output.size());
    return NvResult::Success;
}

NvResult nvhost_gpu::ChannelSetTimeout(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlChannelSetTimeout params{};
    std::memcpy(&params, input.data(), sizeof(IoctlChannelSetTimeout));
    LOG_INFO(Service_NVDRV, "called, timeout=0x{:X}", params.timeout);

    return NvResult::Success;
}

NvResult nvhost_gpu::ChannelSetTimeslice(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetTimeslice params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSetTimeslice));
    LOG_INFO(Service_NVDRV, "called, timeslice=0x{:X}", params.timeslice);

    channel_timeslice = params.timeslice;

    return NvResult::Success;
}

} // namespace Service::Nvidia::Devices
