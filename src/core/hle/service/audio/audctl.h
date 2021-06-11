// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Audio {

class AudCtl final : public ServiceFramework<AudCtl> {
public:
    explicit AudCtl(Core::System& system_);
    ~AudCtl() override;

private:
    void GetTargetVolumeMin(Kernel::HLERequestContext& ctx);
    void GetTargetVolumeMax(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Audio
