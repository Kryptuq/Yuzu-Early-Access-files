// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Account {

constexpr ResultCode ERR_ACCOUNTINFO_BAD_APPLICATION{ErrorModule::Account, 22};
constexpr ResultCode ERR_ACCOUNTINFO_ALREADY_INITIALIZED{ErrorModule::Account, 41};

} // namespace Service::Account
