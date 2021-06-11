// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::Account {

constexpr ResultCode ERR_ACCOUNTINFO_BAD_APPLICATION{ErrorModule::Account, 22};
constexpr ResultCode ERR_ACCOUNTINFO_ALREADY_INITIALIZED{ErrorModule::Account, 41};

} // namespace Service::Account
