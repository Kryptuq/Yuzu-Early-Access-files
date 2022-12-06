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

    void Initialize(Kernel::HLERequestContext& ctx);
    void Finalize(Kernel::HLERequestContext& ctx);
    void ListDevices(Kernel::HLERequestContext& ctx);
    void StartDetection(Kernel::HLERequestContext& ctx);
    void StopDetection(Kernel::HLERequestContext& ctx);
    void Read(Kernel::HLERequestContext& ctx);
    void Write(Kernel::HLERequestContext& ctx);
    void GetTagInfo(Kernel::HLERequestContext& ctx);
    void GetActivateEventHandle(Kernel::HLERequestContext& ctx);
    void GetDeactivateEventHandle(Kernel::HLERequestContext& ctx);
    void GetState(Kernel::HLERequestContext& ctx);
    void GetDeviceState(Kernel::HLERequestContext& ctx);
    void GetNpadId(Kernel::HLERequestContext& ctx);
    void GetAvailabilityChangeEventHandle(Kernel::HLERequestContext& ctx);

    std::optional<std::shared_ptr<NfcDevice>> GetNfcDevice(u64 handle);

    KernelHelpers::ServiceContext service_context;

    std::array<std::shared_ptr<NfcDevice>, 10> devices{};

    State state{State::NonInitialized};
    Kernel::KEvent* availability_change_event;
};

} // namespace Service::NFC
