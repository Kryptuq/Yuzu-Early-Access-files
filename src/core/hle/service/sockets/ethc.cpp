// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/sockets/ethc.h"

namespace Service::Sockets {

ETHC_C::ETHC_C(Core::System& system_) : ServiceFramework{system_, "ethc:c"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "Initialize"},
        {1, nullptr, "Cancel"},
        {2, nullptr, "GetResult"},
        {3, nullptr, "GetMediaList"},
        {4, nullptr, "SetMediaType"},
        {5, nullptr, "GetMediaType"},
        {6, nullptr, "Unknown6"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ETHC_C::~ETHC_C() = default;

ETHC_I::ETHC_I(Core::System& system_) : ServiceFramework{system_, "ethc:i"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetReadableHandle"},
        {1, nullptr, "Cancel"},
        {2, nullptr, "GetResult"},
        {3, nullptr, "GetInterfaceList"},
        {4, nullptr, "GetInterfaceCount"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ETHC_I::~ETHC_I() = default;

} // namespace Service::Sockets
