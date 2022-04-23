// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/audio/audrec_a.h"

namespace Service::Audio {

AudRecA::AudRecA(Core::System& system_) : ServiceFramework{system_, "audrec:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestSuspend"},
        {1, nullptr, "RequestResume"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudRecA::~AudRecA() = default;

} // namespace Service::Audio
