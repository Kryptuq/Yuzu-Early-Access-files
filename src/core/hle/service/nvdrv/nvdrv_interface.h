// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/service.h"

namespace Kernel {
class KWritableEvent;
}

namespace Service::Nvidia {

class NVDRV final : public ServiceFramework<NVDRV> {
public:
    explicit NVDRV(Core::System& system_, std::shared_ptr<Module> nvdrv_, const char* name);
    ~NVDRV() override;

private:
    void Open(Kernel::HLERequestContext& ctx);
    void Ioctl1(Kernel::HLERequestContext& ctx);
    void Ioctl2(Kernel::HLERequestContext& ctx);
    void Ioctl3(Kernel::HLERequestContext& ctx);
    void Close(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);
    void QueryEvent(Kernel::HLERequestContext& ctx);
    void SetAruid(Kernel::HLERequestContext& ctx);
    void SetGraphicsFirmwareMemoryMarginEnabled(Kernel::HLERequestContext& ctx);
    void GetStatus(Kernel::HLERequestContext& ctx);
    void DumpGraphicsMemoryInfo(Kernel::HLERequestContext& ctx);

    void ServiceError(Kernel::HLERequestContext& ctx, NvResult result);

    std::shared_ptr<Module> nvdrv;

    u64 pid{};
    bool is_initialized{};
};

} // namespace Service::Nvidia
