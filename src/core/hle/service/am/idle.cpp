// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/am/idle.h"

namespace Service::AM {

IdleSys::IdleSys(Core::System& system_) : ServiceFramework{system_, "idle:sys"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetAutoPowerDownEvent"},
        {1, nullptr, "IsAutoPowerDownRequested"},
        {2, nullptr, "Unknown2"},
        {3, nullptr, "SetHandlingContext"},
        {4, nullptr, "LoadAndApplySettings"},
        {5, nullptr, "ReportUserIsActive"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IdleSys::~IdleSys() = default;

} // namespace Service::AM
