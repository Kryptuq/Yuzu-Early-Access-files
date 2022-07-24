// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/audio/audren_a.h"

namespace Service::Audio {

AudRenA::AudRenA(Core::System& system_) : ServiceFramework{system_, "audren:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestSuspend"},
        {1, nullptr, "RequestResume"},
        {2, nullptr, "GetProcessMasterVolume"},
        {3, nullptr, "SetProcessMasterVolume"},
        {4, nullptr, "RegisterAppletResourceUserId"},
        {5, nullptr, "UnregisterAppletResourceUserId"},
        {6, nullptr, "GetProcessRecordVolume"},
        {7, nullptr, "SetProcessRecordVolume"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudRenA::~AudRenA() = default;

} // namespace Service::Audio
