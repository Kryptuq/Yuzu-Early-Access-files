// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Account {

constexpr Result ResultCancelledByUser{ErrorModule::Account, 1};
constexpr Result ResultNoNotifications{ErrorModule::Account, 15};
constexpr Result ResultInvalidUserId{ErrorModule::Account, 20};
constexpr Result ResultInvalidApplication{ErrorModule::Account, 22};
constexpr Result ResultNullptr{ErrorModule::Account, 30};
constexpr Result ResultInvalidArrayLength{ErrorModule::Account, 32};
constexpr Result ResultApplicationInfoAlreadyInitialized{ErrorModule::Account, 41};
constexpr Result ResultAccountUpdateFailed{ErrorModule::Account, 100};

} // namespace Service::Account
