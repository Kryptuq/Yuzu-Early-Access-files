// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/set/set_fd.h"

namespace Service::Set {

SET_FD::SET_FD(Core::System& system_) : ServiceFramework{system_, "set:fd"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {2, nullptr, "SetSettingsItemValue"},
        {3, nullptr, "ResetSettingsItemValue"},
        {4, nullptr, "CreateSettingsItemKeyIterator"},
        {10, nullptr, "ReadSettings"},
        {11, nullptr, "ResetSettings"},
        {20, nullptr, "SetWebInspectorFlag"},
        {21, nullptr, "SetAllowedSslHosts"},
        {22, nullptr, "SetHostFsMountPoint"},
        {23, nullptr, "SetMemoryUsageRateFlag"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

SET_FD::~SET_FD() = default;

} // namespace Service::Set
