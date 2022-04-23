// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Set {

class SET_FD final : public ServiceFramework<SET_FD> {
public:
    explicit SET_FD(Core::System& system_);
    ~SET_FD() override;
};

} // namespace Service::Set
