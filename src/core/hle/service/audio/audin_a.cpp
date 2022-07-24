// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/audio/audin_a.h"

namespace Service::Audio {

AudInA::AudInA(Core::System& system_) : ServiceFramework{system_, "audin:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestSuspend"},
        {1, nullptr, "RequestResume"},
        {2, nullptr, "GetProcessMasterVolume"},
        {3, nullptr, "SetProcessMasterVolume"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudInA::~AudInA() = default;

} // namespace Service::Audio
