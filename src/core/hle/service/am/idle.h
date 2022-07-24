// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::AM {

class IdleSys final : public ServiceFramework<IdleSys> {
public:
    explicit IdleSys(Core::System& system_);
    ~IdleSys() override;
};

} // namespace Service::AM
