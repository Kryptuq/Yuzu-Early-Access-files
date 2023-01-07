// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: 2021 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit>
#include <cstdlib>
#include <cstring>

#include <fmt/format.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvdrv/core/syncpoint_manager.h"
#include "core/hle/service/nvdrv/devices/nvhost_ctrl.h"
#include "video_core/gpu.h"
#include "video_core/host1x/host1x.h"

namespace Service::Nvidia::Devices {

nvhost_ctrl::nvhost_ctrl(Core::System& system_, EventInterface& events_interface_,
                         NvCore::Container& core_)
    : nvdevice{system_}, events_interface{events_interface_}, core{core_},
      syncpoint_manager{core_.GetSyncpointManager()} {}

nvhost_ctrl::~nvhost_ctrl() {
    for (auto& event : events) {
        if (!event.registered) {
            continue;
        }
        events_interface.FreeEvent(event.kevent);
    }
}

NvResult nvhost_ctrl::Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                             std::vector<u8>& output) {
    switch (command.group) {
    case 0x0:
        switch (command.cmd) {
        case 0x1b:
            return NvOsGetConfigU32(input, output);
        case 0x1c:
            return IocCtrlClearEventWait(input, output);
        case 0x1d:
            return IocCtrlEventWait(input, output, true);
        case 0x1e:
            return IocCtrlEventWait(input, output, false);
        case 0x1f:
            return IocCtrlEventRegister(input, output);
        case 0x20:
            return IocCtrlEventUnregister(input, output);
        case 0x21:
            return IocCtrlEventUnregisterBatch(input, output);
        }
        break;
    default:
        break;
    }

    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_ctrl::Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                             std::span<const u8> inline_input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

NvResult nvhost_ctrl::Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input,
                             std::vector<u8>& output, std::vector<u8>& inline_outpu) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl={:08X}", command.raw);
    return NvResult::NotImplemented;
}

void nvhost_ctrl::OnOpen(DeviceFD fd) {}

void nvhost_ctrl::OnClose(DeviceFD fd) {}

NvResult nvhost_ctrl::NvOsGetConfigU32(std::span<const u8> input, std::vector<u8>& output) {
    IocGetConfigParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_TRACE(Service_NVDRV, "called, setting={}!{}", params.domain_str.data(),
              params.param_str.data());
    return NvResult::ConfigVarNotFound; // Returns error on production mode
}

NvResult nvhost_ctrl::IocCtrlEventWait(std::span<const u8> input, std::vector<u8>& output,
                                       bool is_allocation) {
    IocCtrlEventWaitParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_DEBUG(Service_NVDRV, "syncpt_id={}, threshold={}, timeout={}, is_allocation={}",
              params.fence.id, params.fence.value, params.timeout, is_allocation);

    bool must_unmark_fail = !is_allocation;
    const u32 event_id = params.value.raw;
    SCOPE_EXIT({
        std::memcpy(output.data(), &params, sizeof(params));
        if (must_unmark_fail) {
            events[event_id].fails = 0;
        }
    });

    const u32 fence_id = static_cast<u32>(params.fence.id);

    if (fence_id >= MaxSyncPoints) {
        return NvResult::BadParameter;
    }

    if (params.fence.value == 0) {
        if (!syncpoint_manager.IsSyncpointAllocated(params.fence.id)) {
            LOG_WARNING(Service_NVDRV,
                        "Unallocated syncpt_id={}, threshold={}, timeout={}, is_allocation={}",
                        params.fence.id, params.fence.value, params.timeout, is_allocation);
        } else {
            params.value.raw = syncpoint_manager.ReadSyncpointMinValue(fence_id);
        }
        return NvResult::Success;
    }

    if (syncpoint_manager.IsFenceSignalled(params.fence)) {
        params.value.raw = syncpoint_manager.ReadSyncpointMinValue(fence_id);
        return NvResult::Success;
    }

    if (const auto new_value = syncpoint_manager.UpdateMin(fence_id);
        syncpoint_manager.IsFenceSignalled(params.fence)) {
        params.value.raw = new_value;
        return NvResult::Success;
    }

    auto& host1x_syncpoint_manager = system.Host1x().GetSyncpointManager();
    const u32 target_value = params.fence.value;

    auto lock = NvEventsLock();

    u32 slot = [&]() {
        if (is_allocation) {
            params.value.raw = 0;
            return FindFreeNvEvent(fence_id);
        } else {
            return params.value.raw;
        }
    }();

    must_unmark_fail = false;

    const auto check_failing = [&]() {
        if (events[slot].fails > 2) {
            {
                auto lk = system.StallProcesses();
                host1x_syncpoint_manager.WaitHost(fence_id, target_value);
                system.UnstallProcesses();
            }
            params.value.raw = target_value;
            return true;
        }
        return false;
    };

    if (slot >= MaxNvEvents) {
        return NvResult::BadParameter;
    }

    if (params.timeout == 0) {
        if (check_failing()) {
            events[slot].fails = 0;
            return NvResult::Success;
        }
        return NvResult::Timeout;
    }

    auto& event = events[slot];

    if (!event.registered) {
        return NvResult::BadParameter;
    }

    if (event.IsBeingUsed()) {
        return NvResult::BadParameter;
    }

    if (check_failing()) {
        event.fails = 0;
        return NvResult::Success;
    }

    params.value.raw = 0;

    event.status.store(EventState::Waiting, std::memory_order_release);
    event.assigned_syncpt = fence_id;
    event.assigned_value = target_value;
    if (is_allocation) {
        params.value.syncpoint_id_for_allocation.Assign(static_cast<u16>(fence_id));
        params.value.event_allocated.Assign(1);
    } else {
        params.value.syncpoint_id.Assign(fence_id);
    }
    params.value.raw |= slot;

    event.wait_handle =
        host1x_syncpoint_manager.RegisterHostAction(fence_id, target_value, [this, slot]() {
            auto& event_ = events[slot];
            if (event_.status.exchange(EventState::Signalling, std::memory_order_acq_rel) ==
                EventState::Waiting) {
                event_.kevent->Signal();
            }
            event_.status.store(EventState::Signalled, std::memory_order_release);
        });
    return NvResult::Timeout;
}

NvResult nvhost_ctrl::FreeEvent(u32 slot) {
    if (slot >= MaxNvEvents) {
        return NvResult::BadParameter;
    }

    auto& event = events[slot];

    if (!event.registered) {
        return NvResult::Success;
    }

    if (event.IsBeingUsed()) {
        return NvResult::Busy;
    }

    FreeNvEvent(slot);
    return NvResult::Success;
}

NvResult nvhost_ctrl::IocCtrlEventRegister(std::span<const u8> input, std::vector<u8>& output) {
    IocCtrlEventRegisterParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    const u32 event_id = params.user_event_id;
    LOG_DEBUG(Service_NVDRV, " called, user_event_id: {:X}", event_id);
    if (event_id >= MaxNvEvents) {
        return NvResult::BadParameter;
    }

    auto lock = NvEventsLock();

    if (events[event_id].registered) {
        const auto result = FreeEvent(event_id);
        if (result != NvResult::Success) {
            return result;
        }
    }
    CreateNvEvent(event_id);
    return NvResult::Success;
}

NvResult nvhost_ctrl::IocCtrlEventUnregister(std::span<const u8> input, std::vector<u8>& output) {
    IocCtrlEventUnregisterParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    const u32 event_id = params.user_event_id & 0x00FF;
    LOG_DEBUG(Service_NVDRV, " called, user_event_id: {:X}", event_id);

    auto lock = NvEventsLock();
    return FreeEvent(event_id);
}

NvResult nvhost_ctrl::IocCtrlEventUnregisterBatch(std::span<const u8> input,
                                                  std::vector<u8>& output) {
    IocCtrlEventUnregisterBatchParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    u64 event_mask = params.user_events;
    LOG_DEBUG(Service_NVDRV, " called, event_mask: {:X}", event_mask);

    auto lock = NvEventsLock();
    while (event_mask != 0) {
        const u64 event_id = std::countr_zero(event_mask);
        event_mask &= ~(1ULL << event_id);
        const auto result = FreeEvent(static_cast<u32>(event_id));
        if (result != NvResult::Success) {
            return result;
        }
    }
    return NvResult::Success;
}

NvResult nvhost_ctrl::IocCtrlClearEventWait(std::span<const u8> input, std::vector<u8>& output) {
    IocCtrlEventClearParams params{};
    std::memcpy(&params, input.data(), sizeof(params));

    u32 event_id = params.event_id.slot;
    LOG_DEBUG(Service_NVDRV, "called, event_id: {:X}", event_id);

    if (event_id >= MaxNvEvents) {
        return NvResult::BadParameter;
    }

    auto lock = NvEventsLock();

    auto& event = events[event_id];
    if (event.status.exchange(EventState::Cancelling, std::memory_order_acq_rel) ==
        EventState::Waiting) {
        auto& host1x_syncpoint_manager = system.Host1x().GetSyncpointManager();
        host1x_syncpoint_manager.DeregisterHostAction(event.assigned_syncpt, event.wait_handle);
        syncpoint_manager.UpdateMin(event.assigned_syncpt);
        event.wait_handle = {};
    }
    event.fails++;
    event.status.store(EventState::Cancelled, std::memory_order_release);
    event.kevent->Clear();

    return NvResult::Success;
}

Kernel::KEvent* nvhost_ctrl::QueryEvent(u32 event_id) {
    const auto desired_event = SyncpointEventValue{.raw = event_id};

    const bool allocated = desired_event.event_allocated.Value() != 0;
    const u32 slot{allocated ? desired_event.partial_slot.Value()
                             : static_cast<u32>(desired_event.slot)};
    if (slot >= MaxNvEvents) {
        ASSERT(false);
        return nullptr;
    }

    const u32 syncpoint_id{allocated ? desired_event.syncpoint_id_for_allocation.Value()
                                     : desired_event.syncpoint_id.Value()};

    auto lock = NvEventsLock();

    auto& event = events[slot];
    if (event.registered && event.assigned_syncpt == syncpoint_id) {
        ASSERT(event.kevent);
        return event.kevent;
    }
    // Is this possible in hardware?
    ASSERT_MSG(false, "Slot:{}, SyncpointID:{}, requested", slot, syncpoint_id);
    return nullptr;
}

std::unique_lock<std::mutex> nvhost_ctrl::NvEventsLock() {
    return std::unique_lock<std::mutex>(events_mutex);
}

void nvhost_ctrl::CreateNvEvent(u32 event_id) {
    auto& event = events[event_id];
    ASSERT(!event.kevent);
    ASSERT(!event.registered);
    ASSERT(!event.IsBeingUsed());
    event.kevent = events_interface.CreateEvent(fmt::format("NVCTRL::NvEvent_{}", event_id));
    event.status = EventState::Available;
    event.registered = true;
    const u64 mask = 1ULL << event_id;
    event.fails = 0;
    events_mask |= mask;
    event.assigned_syncpt = 0;
}

void nvhost_ctrl::FreeNvEvent(u32 event_id) {
    auto& event = events[event_id];
    ASSERT(event.kevent);
    ASSERT(event.registered);
    ASSERT(!event.IsBeingUsed());
    events_interface.FreeEvent(event.kevent);
    event.kevent = nullptr;
    event.status = EventState::Available;
    event.registered = false;
    const u64 mask = ~(1ULL << event_id);
    events_mask &= mask;
}

u32 nvhost_ctrl::FindFreeNvEvent(u32 syncpoint_id) {
    u32 slot{MaxNvEvents};
    u32 free_slot{MaxNvEvents};
    for (u32 i = 0; i < MaxNvEvents; i++) {
        auto& event = events[i];
        if (event.registered) {
            if (!event.IsBeingUsed()) {
                slot = i;
                if (event.assigned_syncpt == syncpoint_id) {
                    return slot;
                }
            }
        } else if (free_slot == MaxNvEvents) {
            free_slot = i;
        }
    }
    if (free_slot < MaxNvEvents) {
        CreateNvEvent(free_slot);
        return free_slot;
    }

    if (slot < MaxNvEvents) {
        return slot;
    }

    LOG_CRITICAL(Service_NVDRV, "Failed to allocate an event");
    return 0;
}

} // namespace Service::Nvidia::Devices
