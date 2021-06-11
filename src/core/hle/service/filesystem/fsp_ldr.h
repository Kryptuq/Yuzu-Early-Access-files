// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::FileSystem {

class FSP_LDR final : public ServiceFramework<FSP_LDR> {
public:
    explicit FSP_LDR(Core::System& system_);
    ~FSP_LDR() override;
};

} // namespace Service::FileSystem
