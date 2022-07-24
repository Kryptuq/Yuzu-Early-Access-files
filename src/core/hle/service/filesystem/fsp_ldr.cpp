// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/filesystem/fsp_ldr.h"

namespace Service::FileSystem {

FSP_LDR::FSP_LDR(Core::System& system_) : ServiceFramework{system_, "fsp:ldr"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "OpenCodeFileSystem"},
        {1, nullptr, "IsArchivedProgram"},
        {2, nullptr, "SetCurrentProcess"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

FSP_LDR::~FSP_LDR() = default;

} // namespace Service::FileSystem
