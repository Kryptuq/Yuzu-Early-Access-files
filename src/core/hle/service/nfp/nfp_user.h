// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <optional>

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Service::NFP {
class NfpDevice;

class IUser final : public ServiceFramework<IUser> {
public:
    explicit IUser(Core::System& system_);
    ~IUser();

private:
    enum class State : u32 {
        NonInitialized,
        Initialized,
    };

    void Initialize(HLERequestContext& ctx);
    void Finalize(HLERequestContext& ctx);
    void ListDevices(HLERequestContext& ctx);
    void StartDetection(HLERequestContext& ctx);
    void StopDetection(HLERequestContext& ctx);
    void Mount(HLERequestContext& ctx);
    void Unmount(HLERequestContext& ctx);
    void OpenApplicationArea(HLERequestContext& ctx);
    void GetApplicationArea(HLERequestContext& ctx);
    void SetApplicationArea(HLERequestContext& ctx);
    void Flush(HLERequestContext& ctx);
    void Restore(HLERequestContext& ctx);
    void CreateApplicationArea(HLERequestContext& ctx);
    void GetTagInfo(HLERequestContext& ctx);
    void GetRegisterInfo(HLERequestContext& ctx);
    void GetCommonInfo(HLERequestContext& ctx);
    void GetModelInfo(HLERequestContext& ctx);
    void AttachActivateEvent(HLERequestContext& ctx);
    void AttachDeactivateEvent(HLERequestContext& ctx);
    void GetState(HLERequestContext& ctx);
    void GetDeviceState(HLERequestContext& ctx);
    void GetNpadId(HLERequestContext& ctx);
    void GetApplicationAreaSize(HLERequestContext& ctx);
    void AttachAvailabilityChangeEvent(HLERequestContext& ctx);
    void RecreateApplicationArea(HLERequestContext& ctx);

    std::optional<std::shared_ptr<NfpDevice>> GetNfpDevice(u64 handle);

    KernelHelpers::ServiceContext service_context;

    std::array<std::shared_ptr<NfpDevice>, 10> devices{};

    State state{State::NonInitialized};
    Kernel::KEvent* availability_change_event;
};

} // namespace Service::NFP
