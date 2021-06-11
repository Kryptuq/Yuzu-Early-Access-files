// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/nvmemp.h"

namespace Service::Nvidia {

NVMEMP::NVMEMP(Core::System& system_) : ServiceFramework{system_, "nvmemp"} {
    static const FunctionInfo functions[] = {
        {0, &NVMEMP::Open, "Open"},
        {1, &NVMEMP::GetAruid, "GetAruid"},
    };
    RegisterHandlers(functions);
}

NVMEMP::~NVMEMP() = default;

void NVMEMP::Open(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED();
}

void NVMEMP::GetAruid(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED();
}

} // namespace Service::Nvidia
