// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/audio/audout_a.h"

namespace Service::Audio {

AudOutA::AudOutA(Core::System& system_) : ServiceFramework{system_, "audout:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestSuspend"},
        {1, nullptr, "RequestResume"},
        {2, nullptr, "GetProcessMasterVolume"},
        {3, nullptr, "SetProcessMasterVolume"},
        {4, nullptr, "GetProcessRecordVolume"},
        {5, nullptr, "SetProcessRecordVolume"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudOutA::~AudOutA() = default;

} // namespace Service::Audio
