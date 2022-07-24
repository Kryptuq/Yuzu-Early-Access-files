// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/pctl/pctl.h"

namespace Service::PCTL {

PCTL::PCTL(Core::System& system_, std::shared_ptr<Module> module_, const char* name,
           Capability capability_)
    : Interface{system_, std::move(module_), name, capability_} {
    static const FunctionInfo functions[] = {
        {0, &PCTL::CreateService, "CreateService"},
        {1, &PCTL::CreateServiceWithoutInitialize, "CreateServiceWithoutInitialize"},
    };
    RegisterHandlers(functions);
}

PCTL::~PCTL() = default;
} // namespace Service::PCTL
