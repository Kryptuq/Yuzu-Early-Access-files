// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/nfp/nfp_types.h"

namespace Service::NFP {
class NfpDevice;

class IUser final : public ServiceFramework<IUser> {
public:
    explicit IUser(Core::System& system_);

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void Finalize(Kernel::HLERequestContext& ctx);
    void ListDevices(Kernel::HLERequestContext& ctx);
    void StartDetection(Kernel::HLERequestContext& ctx);
    void StopDetection(Kernel::HLERequestContext& ctx);
    void Mount(Kernel::HLERequestContext& ctx);
    void Unmount(Kernel::HLERequestContext& ctx);
    void OpenApplicationArea(Kernel::HLERequestContext& ctx);
    void GetApplicationArea(Kernel::HLERequestContext& ctx);
    void SetApplicationArea(Kernel::HLERequestContext& ctx);
    void Flush(Kernel::HLERequestContext& ctx);
    void Restore(Kernel::HLERequestContext& ctx);
    void CreateApplicationArea(Kernel::HLERequestContext& ctx);
    void GetTagInfo(Kernel::HLERequestContext& ctx);
    void GetRegisterInfo(Kernel::HLERequestContext& ctx);
    void GetCommonInfo(Kernel::HLERequestContext& ctx);
    void GetModelInfo(Kernel::HLERequestContext& ctx);
    void AttachActivateEvent(Kernel::HLERequestContext& ctx);
    void AttachDeactivateEvent(Kernel::HLERequestContext& ctx);
    void GetState(Kernel::HLERequestContext& ctx);
    void GetDeviceState(Kernel::HLERequestContext& ctx);
    void GetNpadId(Kernel::HLERequestContext& ctx);
    void GetApplicationAreaSize(Kernel::HLERequestContext& ctx);
    void AttachAvailabilityChangeEvent(Kernel::HLERequestContext& ctx);
    void RecreateApplicationArea(Kernel::HLERequestContext& ctx);

    std::optional<std::shared_ptr<NfpDevice>> GetNfpDevice(u64 handle);

    KernelHelpers::ServiceContext service_context;

    std::array<std::shared_ptr<NfpDevice>, 10> devices{};

    State state{State::NonInitialized};
    Kernel::KEvent* availability_change_event;
};

} // namespace Service::NFP
