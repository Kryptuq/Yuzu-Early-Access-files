// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <optional>

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Service::NFC {
class NfcDevice;

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
    void GetState(HLERequestContext& ctx);
    void IsNfcEnabled(HLERequestContext& ctx);
    void ListDevices(HLERequestContext& ctx);
    void GetDeviceState(HLERequestContext& ctx);
    void GetNpadId(HLERequestContext& ctx);
    void AttachAvailabilityChangeEvent(HLERequestContext& ctx);
    void StartDetection(HLERequestContext& ctx);
    void StopDetection(HLERequestContext& ctx);
    void GetTagInfo(HLERequestContext& ctx);
    void AttachActivateEvent(HLERequestContext& ctx);
    void AttachDeactivateEvent(HLERequestContext& ctx);
    void SendCommandByPassThrough(HLERequestContext& ctx);

    std::optional<std::shared_ptr<NfcDevice>> GetNfcDevice(u64 handle);

    KernelHelpers::ServiceContext service_context;

    std::array<std::shared_ptr<NfcDevice>, 10> devices{};

    State state{State::NonInitialized};
    Kernel::KEvent* availability_change_event;
};

} // namespace Service::NFC
