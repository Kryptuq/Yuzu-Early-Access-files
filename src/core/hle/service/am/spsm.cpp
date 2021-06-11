// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/am/spsm.h"

namespace Service::AM {

SPSM::SPSM(Core::System& system_) : ServiceFramework{system_, "spsm"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetState"},
        {1, nullptr, "EnterSleep"},
        {2, nullptr, "GetLastWakeReason"},
        {3, nullptr, "Shutdown"},
        {4, nullptr, "GetNotificationMessageEventHandle"},
        {5, nullptr, "ReceiveNotificationMessage"},
        {6, nullptr, "AnalyzeLogForLastSleepWakeSequence"},
        {7, nullptr, "ResetEventLog"},
        {8, nullptr, "AnalyzePerformanceLogForLastSleepWakeSequence"},
        {9, nullptr, "ChangeHomeButtonLongPressingTime"},
        {10, nullptr, "PutErrorState"},
        {11, nullptr, "InvalidateCurrentHomeButtonPressing"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SPSM::~SPSM() = default;

} // namespace Service::AM
