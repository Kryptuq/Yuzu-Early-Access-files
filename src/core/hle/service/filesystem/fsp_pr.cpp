// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/filesystem/fsp_pr.h"

namespace Service::FileSystem {

FSP_PR::FSP_PR(Core::System& system_) : ServiceFramework{system_, "fsp:pr"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RegisterProgram"},
        {1, nullptr, "UnregisterProgram"},
        {2, nullptr, "SetCurrentProcess"},
        {256, nullptr, "SetEnabledProgramVerification"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

FSP_PR::~FSP_PR() = default;

} // namespace Service::FileSystem
