// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Sockets {

class ETHC_C final : public ServiceFramework<ETHC_C> {
public:
    explicit ETHC_C(Core::System& system_);
    ~ETHC_C() override;
};

class ETHC_I final : public ServiceFramework<ETHC_I> {
public:
    explicit ETHC_I(Core::System& system_);
    ~ETHC_I() override;
};

} // namespace Service::Sockets
