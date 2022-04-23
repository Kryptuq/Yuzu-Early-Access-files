// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::HID {

class IRS final : public ServiceFramework<IRS> {
public:
    explicit IRS(Core::System& system_);
    ~IRS() override;

private:
    void ActivateIrsensor(Kernel::HLERequestContext& ctx);
    void DeactivateIrsensor(Kernel::HLERequestContext& ctx);
    void GetIrsensorSharedMemoryHandle(Kernel::HLERequestContext& ctx);
    void StopImageProcessor(Kernel::HLERequestContext& ctx);
    void RunMomentProcessor(Kernel::HLERequestContext& ctx);
    void RunClusteringProcessor(Kernel::HLERequestContext& ctx);
    void RunImageTransferProcessor(Kernel::HLERequestContext& ctx);
    void GetImageTransferProcessorState(Kernel::HLERequestContext& ctx);
    void RunTeraPluginProcessor(Kernel::HLERequestContext& ctx);
    void GetNpadIrCameraHandle(Kernel::HLERequestContext& ctx);
    void RunPointingProcessor(Kernel::HLERequestContext& ctx);
    void SuspendImageProcessor(Kernel::HLERequestContext& ctx);
    void CheckFirmwareVersion(Kernel::HLERequestContext& ctx);
    void SetFunctionLevel(Kernel::HLERequestContext& ctx);
    void RunImageTransferExProcessor(Kernel::HLERequestContext& ctx);
    void RunIrLedProcessor(Kernel::HLERequestContext& ctx);
    void StopImageProcessorAsync(Kernel::HLERequestContext& ctx);
    void ActivateIrsensorWithFunctionLevel(Kernel::HLERequestContext& ctx);

    const u32 device_handle{0xABCD};
};

class IRS_SYS final : public ServiceFramework<IRS_SYS> {
public:
    explicit IRS_SYS(Core::System& system);
    ~IRS_SYS() override;
};

} // namespace Service::HID
