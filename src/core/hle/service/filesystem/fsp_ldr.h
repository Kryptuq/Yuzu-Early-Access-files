// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
