// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/pctl/module.h"

namespace Core {
class System;
}

namespace Service::PCTL {

class PCTL final : public Module::Interface {
public:
    explicit PCTL(Core::System& system_, std::shared_ptr<Module> module_, const char* name,
                  Capability capability_);
    ~PCTL() override;
};

} // namespace Service::PCTL
