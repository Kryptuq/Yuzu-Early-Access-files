// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Glue {

constexpr Result ERR_INVALID_RESOURCE{ErrorModule::ARP, 30};
constexpr Result ERR_INVALID_PROCESS_ID{ErrorModule::ARP, 31};
constexpr Result ERR_INVALID_ACCESS{ErrorModule::ARP, 42};
constexpr Result ERR_NOT_REGISTERED{ErrorModule::ARP, 102};

} // namespace Service::Glue
