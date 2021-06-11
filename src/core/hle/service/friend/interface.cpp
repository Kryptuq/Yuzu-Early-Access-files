// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/friend/interface.h"

namespace Service::Friend {

Friend::Friend(std::shared_ptr<Module> module_, Core::System& system_, const char* name)
    : Interface(std::move(module_), system_, name) {
    static const FunctionInfo functions[] = {
        {0, &Friend::CreateFriendService, "CreateFriendService"},
        {1, &Friend::CreateNotificationService, "CreateNotificationService"},
        {2, nullptr, "CreateDaemonSuspendSessionService"},
    };
    RegisterHandlers(functions);
}

Friend::~Friend() = default;

} // namespace Service::Friend
