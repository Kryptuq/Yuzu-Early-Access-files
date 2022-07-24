// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::AM {

class TCAP final : public ServiceFramework<TCAP> {
public:
    explicit TCAP(Core::System& system_);
    ~TCAP() override;
};

} // namespace Service::AM
