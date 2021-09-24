// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/settings.h"
#include "core/frontend/applets/profile_select.h"
#include "core/hle/service/acc/profile_manager.h"

namespace Core::Frontend {

ProfileSelectApplet::~ProfileSelectApplet() = default;

void DefaultProfileSelectApplet::SelectProfile(
    std::function<void(std::optional<Common::UUID>)> callback) const {
    Service::Account::ProfileManager manager;
    callback(manager.GetUser(Settings::values.current_user.GetValue())
                 .value_or(Common::UUID{Common::INVALID_UUID}));
    LOG_INFO(Service_ACC, "called, selecting current user instead of prompting...");
}

} // namespace Core::Frontend
