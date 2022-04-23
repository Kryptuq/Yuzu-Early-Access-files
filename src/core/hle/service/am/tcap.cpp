// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/am/tcap.h"

namespace Service::AM {

TCAP::TCAP(Core::System& system_) : ServiceFramework{system_, "tcap"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetContinuousHighSkinTemperatureEvent"},
        {1, nullptr, "SetOperationMode"},
        {2, nullptr, "LoadAndApplySettings"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

TCAP::~TCAP() = default;

} // namespace Service::AM
