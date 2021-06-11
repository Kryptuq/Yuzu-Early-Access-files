// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace Service::Glue {

constexpr ResultCode ERR_INVALID_RESOURCE{ErrorModule::ARP, 30};
constexpr ResultCode ERR_INVALID_PROCESS_ID{ErrorModule::ARP, 31};
constexpr ResultCode ERR_INVALID_ACCESS{ErrorModule::ARP, 42};
constexpr ResultCode ERR_NOT_REGISTERED{ErrorModule::ARP, 102};

} // namespace Service::Glue
