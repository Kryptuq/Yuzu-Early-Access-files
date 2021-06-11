// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::AM {

class SPSM final : public ServiceFramework<SPSM> {
public:
    explicit SPSM(Core::System& system_);
    ~SPSM() override;
};

} // namespace Service::AM
