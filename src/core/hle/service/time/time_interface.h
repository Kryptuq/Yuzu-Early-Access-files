// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/time/time.h"

namespace Core {
class System;
}

namespace Service::Time {

class Time final : public Module::Interface {
public:
    explicit Time(std::shared_ptr<Module> time, Core::System& system_, const char* name_);
    ~Time() override;
};

} // namespace Service::Time
