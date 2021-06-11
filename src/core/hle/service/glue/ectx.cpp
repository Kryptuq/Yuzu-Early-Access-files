// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
