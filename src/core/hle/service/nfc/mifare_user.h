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

class MFIUser final : public ServiceFramework<MFIUser> {
public:
    explicit MFIUser(Core::System& system_);
    ~MFIUser();

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
    void Read(HLERequestContext& ctx);
    void Write(HLERequestContext& ctx);
    void GetTagInfo(HLERequestContext& ctx);
    void GetActivateEventHandle(HLERequestContext& ctx);
    void GetDeactivateEventHandle(HLERequestContext& ctx);
    void GetState(HLERequestContext& ctx);
    void GetDeviceState(HLERequestContext& ctx);
    void GetNpadId(HLERequestContext& ctx);
    void GetAvailabilityChangeEventHandle(HLERequestContext& ctx);

    std::optional<std::shared_ptr<NfcDevice>> GetNfcDevice(u64 handle);

    KernelHelpers::ServiceContext service_context;

    std::array<std::shared_ptr<NfcDevice>, 10> devices{};

    State state{State::NonInitialized};
    Kernel::KEvent* availability_change_event;
};

} // namespace Service::NFC
