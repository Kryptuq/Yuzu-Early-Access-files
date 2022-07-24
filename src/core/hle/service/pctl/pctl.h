// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/pctl/pctl_module.h"

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
