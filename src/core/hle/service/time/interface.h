// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
