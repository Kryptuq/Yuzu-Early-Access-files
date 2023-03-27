// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "core/frontend/applets/profile_select.h"
#include "core/hle/service/acc/profile_manager.h"

namespace Core::Frontend {

ProfileSelectApplet::~ProfileSelectApplet() = default;

void DefaultProfileSelectApplet::Close() const {}

void DefaultProfileSelectApplet::SelectProfile(SelectProfileCallback callback) const {
    Service::Account::ProfileManager manager;
    callback(manager.GetUser(Settings::values.current_user.GetValue()).value_or(Common::UUID{}));
    LOG_INFO(Service_ACC, "called, selecting current user instead of prompting...");
}

} // namespace Core::Frontend
