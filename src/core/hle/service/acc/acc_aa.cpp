// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/acc/acc_aa.h"

namespace Service::Account {

ACC_AA::ACC_AA(std::shared_ptr<Module> module_, std::shared_ptr<ProfileManager> profile_manager_,
               Core::System& system_)
    : Interface(std::move(module_), std::move(profile_manager_), system_, "acc:aa") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "EnsureCacheAsync"},
        {1, nullptr, "LoadCache"},
        {2, nullptr, "GetDeviceAccountId"},
        {50, nullptr, "RegisterNotificationTokenAsync"},   // 1.0.0 - 6.2.0
        {51, nullptr, "UnregisterNotificationTokenAsync"}, // 1.0.0 - 6.2.0
    };
    // clang-format on
    RegisterHandlers(functions);
}

ACC_AA::~ACC_AA() = default;

} // namespace Service::Account
