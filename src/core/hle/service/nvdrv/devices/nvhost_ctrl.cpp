// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <cstring>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/service/nvdrv/devices/nvhost_ctrl.h"
#include "video_core/gpu.h"

namespace Service::Nvidia::Devices {

nvhost_ctrl::nvhost_ctrl(Core::System& system_, EventInterface& events_interface_,
                         SyncpointManager& syncpoint_manager_)
    : nvdevice{system_}, events_interface{events_interface_}, syncpoint_manager{
                                                                  syncpoint_manager_} {}
nvhost_ctrl::~nvhost_ctrl() = default;

NvResult nvhost_ctrl::Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                             std::vector<u8>& output) {
    switch (command.group) {
    case 0x0:
        switch (command.cmd) {
        case 0x1b:
            return NvOsGetConfigU32(input, output);
        case 0x1c:
            return IocCtrlClearEventWait(input, output);
        case 0x1d:
            return IocCtrlEventWait(input, output, false);
        case 0x1e:
            return IocCtrlEventWait(input, output, true);
        case 0x1f:
            return IocCtrlEventRegister(input, output);
        case 0x20:
            return IocCtrlEventUnregister(input, output);
        }
        break;
    default:
        break;
    }

    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_ctrl::Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                             const std::vector<u8>& inline_input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_ctrl::Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                             std::vector<u8>& output, std::vector<u8>& inline_outpu) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_ctrl::OnOpen(DeviceFD fd) {}
void nvhost_ctrl::OnClose(DeviceFD fd) {}

NvResult nvhost_ctrl::NvOsGetConfigU32(const std::vector<u8>& input, std::vector<u8>& output) {
    IocGetConfigParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_TRACE(Service_NVDRV, "called, setting={}!{}", params.domain_str.data(),
              params.param_str.data());
    return NvResult::ConfigVarNotFound; // Returns error on production mode
}

NvResult nvhost_ctrl::IocCtrlEventWait(const std::vector<u8>& input, std::vector<u8>& output,
                                       bool is_async) {
    IocCtrlEventWaitParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_DEBUG(Service_NVDRV, "syncpt_id={}, threshold={}, timeout={}, is_async={}",
              params.syncpt_id, params.threshold, params.timeout, is_async);

    if (params.syncpt_id >= MaxSyncPoints) {
        return NvResult::BadParameter;
    }

    u32 event_id = params.value & 0x00FF;

    if (event_id >= MaxNvEvents) {
        std::memcpy(output.data(), &params, sizeof(params));
        return NvResult::BadParameter;
    }

    if (syncpoint_manager.IsSyncpointExpired(params.syncpt_id, params.threshold)) {
        params.value = syncpoint_manager.GetSyncpointMin(params.syncpt_id);
        std::memcpy(output.data(), &params, sizeof(params));
        events_interface.failed[event_id] = false;
        return NvResult::Success;
    }

    if (const auto new_value = syncpoint_manager.RefreshSyncpoint(params.syncpt_id);
        syncpoint_manager.IsSyncpointExpired(params.syncpt_id, params.threshold)) {
        params.value = new_value;
        std::memcpy(output.data(), &params, sizeof(params));
        events_interface.failed[event_id] = false;
        return NvResult::Success;
    }

    auto& event = events_interface.events[event_id];
    auto& gpu = system.GPU();

    // This is mostly to take into account unimplemented features. As synced
    // gpu is always synced.
    if (!gpu.IsAsync()) {
        event.event->GetWritableEvent().Signal();
        return NvResult::Success;
    }
    const u32 current_syncpoint_value = event.fence.value;
    const s32 diff = current_syncpoint_value - params.threshold;
    if (diff >= 0) {
        event.event->GetWritableEvent().Signal();
        params.value = current_syncpoint_value;
        std::memcpy(output.data(), &params, sizeof(params));
        events_interface.failed[event_id] = false;
        return NvResult::Success;
    }
    const u32 target_value = current_syncpoint_value - diff;

    if (!is_async) {
        params.value = 0;
    }

    if (params.timeout == 0) {
        std::memcpy(output.data(), &params, sizeof(params));
        return NvResult::Timeout;
    }

    EventState status = events_interface.status[event_id];
    const bool bad_parameter = status == EventState::Busy;
    if (bad_parameter) {
        std::memcpy(output.data(), &params, sizeof(params));
        return NvResult::BadParameter;
    }
    events_interface.SetEventStatus(event_id, EventState::Waiting);
    events_interface.assigned_syncpt[event_id] = params.syncpt_id;
    events_interface.assigned_value[event_id] = target_value;
    if (is_async) {
        params.value = params.syncpt_id << 4;
    } else {
        params.value = ((params.syncpt_id & 0xfff) << 16) | 0x10000000;
    }
    params.value |= event_id;
    event.event->GetWritableEvent().Clear();
    if (events_interface.failed[event_id]) {
        {
            auto lk = system.StallCPU();
            gpu.WaitFence(params.syncpt_id, target_value);
            system.UnstallCPU();
        }
        std::memcpy(output.data(), &params, sizeof(params));
        events_interface.failed[event_id] = false;
        return NvResult::Success;
    }
    gpu.RegisterSyncptInterrupt(params.syncpt_id, target_value);
    std::memcpy(output.data(), &params, sizeof(params));
    return NvResult::Timeout;
}

NvResult nvhost_ctrl::IocCtrlEventRegister(const std::vector<u8>& input, std::vector<u8>& output) {
    IocCtrlEventRegisterParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    const u32 event_id = params.user_event_id & 0x00FF;
    LOG_DEBUG(Service_NVDRV, " called, user_event_id: {:X}", event_id);
    if (event_id >= MaxNvEvents) {
        return NvResult::BadParameter;
    }
    if (events_interface.registered[event_id]) {
        const auto event_state = events_interface.status[event_id];
        if (event_state != EventState::Free) {
            LOG_WARNING(Service_NVDRV, "Event already registered! Unregistering previous event");
            events_interface.UnregisterEvent(event_id);
        } else {
            return NvResult::BadParameter;
        }
    }
    events_interface.RegisterEvent(event_id);
    return NvResult::Success;
}

NvResult nvhost_ctrl::IocCtrlEventUnregister(const std::vector<u8>& input,
                                             std::vector<u8>& output) {
    IocCtrlEventUnregisterParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    const u32 event_id = params.user_event_id & 0x00FF;
    LOG_DEBUG(Service_NVDRV, " called, user_event_id: {:X}", event_id);
    if (event_id >= MaxNvEvents) {
        return NvResult::BadParameter;
    }
    if (!events_interface.registered[event_id]) {
        return NvResult::BadParameter;
    }
    events_interface.UnregisterEvent(event_id);
    return NvResult::Success;
}

NvResult nvhost_ctrl::IocCtrlClearEventWait(const std::vector<u8>& input, std::vector<u8>& output) {
    IocCtrlEventSignalParams params{};
    std::memcpy(&params, input.data(), sizeof(params));

    u32 event_id = params.event_id & 0x00FF;
    LOG_WARNING(Service_NVDRV, "cleared event wait on, event_id: {:X}", event_id);

    if (event_id >= MaxNvEvents) {
        return NvResult::BadParameter;
    }
    if (events_interface.status[event_id] == EventState::Waiting) {
        events_interface.LiberateEvent(event_id);
    }
    events_interface.failed[event_id] = true;

    syncpoint_manager.RefreshSyncpoint(events_interface.events[event_id].fence.id);

    return NvResult::Success;
}

} // namespace Service::Nvidia::Devices
