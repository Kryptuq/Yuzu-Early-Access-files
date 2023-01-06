// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/core/syncpoint_manager.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/memory.h"
#include "video_core/control/channel_state.h"
#include "video_core/engines/puller.h"
#include "video_core/gpu.h"
#include "video_core/host1x/host1x.h"

namespace Service::Nvidia::Devices {
namespace {
Tegra::CommandHeader BuildFenceAction(Tegra::Engines::Puller::FenceOperation op, u32 syncpoint_id) {
    Tegra::Engines::Puller::FenceAction result{};
    result.op.Assign(op);
    result.syncpoint_id.Assign(syncpoint_id);
    return {result.raw};
}
} // namespace

nvhost_gpu::nvhost_gpu(Core::System& system_, EventInterface& events_interface_,
                       NvCore::Container& core_)
    : nvdevice{system_}, events_interface{events_interface_}, core{core_},
      syncpoint_manager{core_.GetSyncpointManager()}, nvmap{core.GetNvMapFile()},
      channel_state{system.GPU().AllocateChannel()} {
    channel_syncpoint = syncpoint_manager.AllocateSyncpoint(false);
    sm_exception_breakpoint_int_report_event =
        events_interface.CreateEvent("GpuChannelSMExceptionBreakpointInt");
    sm_exception_breakpoint_pause_report_event =
        events_interface.CreateEvent("GpuChannelSMExceptionBreakpointPause");
    error_notifier_event = events_interface.CreateEvent("GpuChannelErrorNotifier");
}

nvhost_gpu::~nvhost_gpu() {
    events_interface.FreeEvent(sm_exception_breakpoint_int_report_event);
    events_interface.FreeEvent(sm_exception_breakpoint_pause_report_event);
    events_interface.FreeEvent(error_notifier_event);
    syncpoint_manager.FreeSyncpoint(channel_syncpoint);
}

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

    if (channel_state->initialized) {
        LOG_CRITICAL(Service_NVDRV, "Already allocated!");
        return NvResult::AlreadyAllocated;
    }

    system.GPU().InitChannel(*channel_state);

    params.fence_out = syncpoint_manager.GetSyncpointFence(channel_syncpoint);

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

static std::vector<Tegra::CommandHeader> BuildWaitCommandList(NvFence fence) {
    return {
        Tegra::BuildCommandHeader(Tegra::BufferMethods::SyncpointPayload, 1,
                                  Tegra::SubmissionMode::Increasing),
        {fence.value},
        Tegra::BuildCommandHeader(Tegra::BufferMethods::SyncpointOperation, 1,
                                  Tegra::SubmissionMode::Increasing),
        BuildFenceAction(Tegra::Engines::Puller::FenceOperation::Acquire, fence.id),
    };
}

static std::vector<Tegra::CommandHeader> BuildIncrementCommandList(NvFence fence) {
    std::vector<Tegra::CommandHeader> result{
        Tegra::BuildCommandHeader(Tegra::BufferMethods::SyncpointPayload, 1,
                                  Tegra::SubmissionMode::Increasing),
        {}};

    for (u32 count = 0; count < 2; ++count) {
        result.emplace_back(Tegra::BuildCommandHeader(Tegra::BufferMethods::SyncpointOperation, 1,
                                                      Tegra::SubmissionMode::Increasing));
        result.emplace_back(
            BuildFenceAction(Tegra::Engines::Puller::FenceOperation::Increment, fence.id));
    }

    return result;
}

static std::vector<Tegra::CommandHeader> BuildIncrementWithWfiCommandList(NvFence fence) {
    std::vector<Tegra::CommandHeader> result{
        Tegra::BuildCommandHeader(Tegra::BufferMethods::WaitForIdle, 1,
                                  Tegra::SubmissionMode::Increasing),
        {}};
    const std::vector<Tegra::CommandHeader> increment{BuildIncrementCommandList(fence)};

    result.insert(result.end(), increment.begin(), increment.end());

    return result;
}

NvResult nvhost_gpu::SubmitGPFIFOImpl(IoctlSubmitGpfifo& params, std::vector<u8>& output,
                                      Tegra::CommandList&& entries) {
    LOG_TRACE(Service_NVDRV, "called, gpfifo={:X}, num_entries={:X}, flags={:X}", params.address,
              params.num_entries, params.flags.raw);

    auto& gpu = system.GPU();

    std::scoped_lock lock(channel_mutex);

    const auto bind_id = channel_state->bind_id;

    auto& flags = params.flags;

    if (flags.fence_wait.Value()) {
        if (flags.increment_value.Value()) {
            return NvResult::BadParameter;
        }

        if (!syncpoint_manager.IsFenceSignalled(params.fence)) {
            gpu.PushGPUEntries(bind_id, Tegra::CommandList{BuildWaitCommandList(params.fence)});
        }
    }

    params.fence.id = channel_syncpoint;

    u32 increment{(flags.fence_increment.Value() != 0 ? 2 : 0) +
                  (flags.increment_value.Value() != 0 ? params.fence.value : 0)};
    params.fence.value = syncpoint_manager.IncrementSyncpointMaxExt(channel_syncpoint, increment);
    gpu.PushGPUEntries(bind_id, std::move(entries));

    if (flags.fence_increment.Value()) {
        if (flags.suppress_wfi.Value()) {
            gpu.PushGPUEntries(bind_id,
                               Tegra::CommandList{BuildIncrementCommandList(params.fence)});
        } else {
            gpu.PushGPUEntries(bind_id,
                               Tegra::CommandList{BuildIncrementWithWfiCommandList(params.fence)});
        }
    }

    flags.raw = 0;

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

Kernel::KEvent* nvhost_gpu::QueryEvent(u32 event_id) {
    switch (event_id) {
    case 1:
        return sm_exception_breakpoint_int_report_event;
    case 2:
        return sm_exception_breakpoint_pause_report_event;
    case 3:
        return error_notifier_event;
    default:
        LOG_CRITICAL(Service_NVDRV, "Unknown Ctrl GPU Event {}", event_id);
        return nullptr;
    }
}

} // namespace Service::Nvidia::Devices
