// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Set {

class SET_CAL final : public ServiceFramework<SET_CAL> {
public:
    explicit SET_CAL(Core::System& system_);
    ~SET_CAL() override;
};

} // namespace Service::Set
