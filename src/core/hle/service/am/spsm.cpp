// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
