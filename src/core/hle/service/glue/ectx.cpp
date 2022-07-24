// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/glue/ectx.h"

namespace Service::Glue {

ECTX_AW::ECTX_AW(Core::System& system_) : ServiceFramework{system_, "ectx:aw"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CreateContextRegistrar"},
        {1, nullptr, "CommitContext"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ECTX_AW::~ECTX_AW() = default;

} // namespace Service::Glue
