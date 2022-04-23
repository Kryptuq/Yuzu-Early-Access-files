// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/spl/csrng.h"

namespace Service::SPL {

CSRNG::CSRNG(Core::System& system_, std::shared_ptr<Module> module_)
    : Interface(system_, std::move(module_), "csrng") {
    static const FunctionInfo functions[] = {
        {0, &CSRNG::GenerateRandomBytes, "GenerateRandomBytes"},
    };
    RegisterHandlers(functions);
}

CSRNG::~CSRNG() = default;

} // namespace Service::SPL
