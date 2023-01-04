// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: 2021 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "video_core/host1x/syncpoint_manager.h"

namespace Service::Nvidia::NvCore {
class Container;
class SyncpointManager;
} // namespace Service::Nvidia::NvCore

namespace Service::Nvidia::Devices {

class nvhost_ctrl final : public nvdevice {
public:
    explicit nvhost_ctrl(Core::System& system_, EventInterface& events_interface_,
                         NvCore::Container& core);
    ~nvhost_ctrl() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                    std::span<const u8> inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input, std::vector<u8>& output,
                    std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

    Kernel::KEvent* QueryEvent(u32 event_id) override;

    union SyncpointEventValue {
        u32 raw;

        union {
            BitField<0, 4, u32> partial_slot;
            BitField<4, 28, u32> syncpoint_id;
        };

        struct {
            u16 slot;
            union {
                BitField<0, 12, u16> syncpoint_id_for_allocation;
                BitField<12, 1, u16> event_allocated;
            };
        };
    };
    static_assert(sizeof(SyncpointEventValue) == sizeof(u32));

private:
    struct InternalEvent {
        // Mask representing registered events

        // Each kernel event associated to an NV event
        Kernel::KEvent* kevent{};
        // The status of the current NVEvent
        std::atomic<EventState> status{};

        // Tells the NVEvent that it has failed.
        u32 fails{};
        // When an NVEvent is waiting on GPU interrupt, this is the sync_point
        // associated with it.
        u32 assigned_syncpt{};
        // This is the value of the GPU interrupt for which the NVEvent is waiting
        // for.
        u32 assigned_value{};

        // Tells if an NVEvent is registered or not
        bool registered{};

        // Used for waiting on a syncpoint & canceling it.
        Tegra::Host1x::SyncpointManager::ActionHandle wait_handle{};

        bool IsBeingUsed() const {
            const auto current_status = status.load(std::memory_order_acquire);
            return current_status == EventState::Waiting ||
                   current_status == EventState::Cancelling ||
                   current_status == EventState::Signalling;
        }
    };

    std::unique_lock<std::mutex> NvEventsLock();

    void CreateNvEvent(u32 event_id);

    void FreeNvEvent(u32 event_id);

    u32 FindFreeNvEvent(u32 syncpoint_id);

    std::array<InternalEvent, MaxNvEvents> events{};
    std::mutex events_mutex;
    u64 events_mask{};

    struct IocSyncptReadParams {
        u32_le id{};
        u32_le value{};
    };
    static_assert(sizeof(IocSyncptReadParams) == 8, "IocSyncptReadParams is incorrect size");

    struct IocSyncptIncrParams {
        u32_le id{};
    };
    static_assert(sizeof(IocSyncptIncrParams) == 4, "IocSyncptIncrParams is incorrect size");

    struct IocSyncptWaitParams {
        u32_le id{};
        u32_le thresh{};
        s32_le timeout{};
    };
    static_assert(sizeof(IocSyncptWaitParams) == 12, "IocSyncptWaitParams is incorrect size");

    struct IocModuleMutexParams {
        u32_le id{};
        u32_le lock{}; // (0 = unlock and 1 = lock)
    };
    static_assert(sizeof(IocModuleMutexParams) == 8, "IocModuleMutexParams is incorrect size");

    struct IocModuleRegRDWRParams {
        u32_le id{};
        u32_le num_offsets{};
        u32_le block_size{};
        u32_le offsets{};
        u32_le values{};
        u32_le write{};
    };
    static_assert(sizeof(IocModuleRegRDWRParams) == 24, "IocModuleRegRDWRParams is incorrect size");

    struct IocSyncptWaitexParams {
        u32_le id{};
        u32_le thresh{};
        s32_le timeout{};
        u32_le value{};
    };
    static_assert(sizeof(IocSyncptWaitexParams) == 16, "IocSyncptWaitexParams is incorrect size");

    struct IocSyncptReadMaxParams {
        u32_le id{};
        u32_le value{};
    };
    static_assert(sizeof(IocSyncptReadMaxParams) == 8, "IocSyncptReadMaxParams is incorrect size");

    struct IocGetConfigParams {
        std::array<char, 0x41> domain_str{};
        std::array<char, 0x41> param_str{};
        std::array<char, 0x101> config_str{};
    };
    static_assert(sizeof(IocGetConfigParams) == 387, "IocGetConfigParams is incorrect size");

    struct IocCtrlEventClearParams {
        SyncpointEventValue event_id{};
    };
    static_assert(sizeof(IocCtrlEventClearParams) == 4,
                  "IocCtrlEventClearParams is incorrect size");

    struct IocCtrlEventWaitParams {
        NvFence fence{};
        u32_le timeout{};
        SyncpointEventValue value{};
    };
    static_assert(sizeof(IocCtrlEventWaitParams) == 16,
                  "IocCtrlEventWaitAsyncParams is incorrect size");

    struct IocCtrlEventRegisterParams {
        u32_le user_event_id{};
    };
    static_assert(sizeof(IocCtrlEventRegisterParams) == 4,
                  "IocCtrlEventRegisterParams is incorrect size");

    struct IocCtrlEventUnregisterParams {
        u32_le user_event_id{};
    };
    static_assert(sizeof(IocCtrlEventUnregisterParams) == 4,
                  "IocCtrlEventUnregisterParams is incorrect size");

    struct IocCtrlEventUnregisterBatchParams {
        u64_le user_events{};
    };
    static_assert(sizeof(IocCtrlEventUnregisterBatchParams) == 8,
                  "IocCtrlEventKill is incorrect size");

    NvResult NvOsGetConfigU32(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocCtrlEventWait(std::span<const u8> input, std::vector<u8>& output,
                              bool is_allocation);
    NvResult IocCtrlEventRegister(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocCtrlEventUnregister(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocCtrlEventUnregisterBatch(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocCtrlClearEventWait(std::span<const u8> input, std::vector<u8>& output);

    NvResult FreeEvent(u32 slot);

    EventInterface& events_interface;
    NvCore::Container& core;
    NvCore::SyncpointManager& syncpoint_manager;
};

} // namespace Service::Nvidia::Devices
