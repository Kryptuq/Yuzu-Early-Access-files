// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
