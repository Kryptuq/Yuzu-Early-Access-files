// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Nvidia {

class NVMEMP final : public ServiceFramework<NVMEMP> {
public:
    explicit NVMEMP(Core::System& system_);
    ~NVMEMP() override;

private:
    void Open(Kernel::HLERequestContext& ctx);
    void GetAruid(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Nvidia
