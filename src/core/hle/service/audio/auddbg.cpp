// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/audio/auddbg.h"

namespace Service::Audio {

AudDbg::AudDbg(Core::System& system_, const char* name) : ServiceFramework{system_, name} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestSuspendForDebug"},
        {1, nullptr, "RequestResumeForDebug"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudDbg::~AudDbg() = default;

} // namespace Service::Audio
