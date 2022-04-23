// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>
#include "common/common_types.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvdrv/nvdrv.h"

namespace Service::Nvidia::Devices {

class nvhost_ctrl final : public nvdevice {
public:
    explicit nvhost_ctrl(Core::System& system_, EventInterface& events_interface_,
                         SyncpointManager& syncpoint_manager_);
    ~nvhost_ctrl() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    const std::vector<u8>& inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output, std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

private:
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

    struct IocCtrlEventSignalParams {
        u32_le event_id{};
    };
    static_assert(sizeof(IocCtrlEventSignalParams) == 4,
                  "IocCtrlEventSignalParams is incorrect size");

    struct IocCtrlEventWaitParams {
        u32_le syncpt_id{};
        u32_le threshold{};
        s32_le timeout{};
        u32_le value{};
    };
    static_assert(sizeof(IocCtrlEventWaitParams) == 16, "IocCtrlEventWaitParams is incorrect size");

    struct IocCtrlEventWaitAsyncParams {
        u32_le syncpt_id{};
        u32_le threshold{};
        u32_le timeout{};
        u32_le value{};
    };
    static_assert(sizeof(IocCtrlEventWaitAsyncParams) == 16,
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

    struct IocCtrlEventKill {
        u64_le user_events{};
    };
    static_assert(sizeof(IocCtrlEventKill) == 8, "IocCtrlEventKill is incorrect size");

    NvResult NvOsGetConfigU32(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult IocCtrlEventWait(const std::vector<u8>& input, std::vector<u8>& output, bool is_async);
    NvResult IocCtrlEventRegister(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult IocCtrlEventUnregister(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult IocCtrlClearEventWait(const std::vector<u8>& input, std::vector<u8>& output);

    EventInterface& events_interface;
    SyncpointManager& syncpoint_manager;
};

} // namespace Service::Nvidia::Devices
