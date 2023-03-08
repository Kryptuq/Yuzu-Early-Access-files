// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

namespace Service::Glue {

constexpr Result ResultInvalidProcessId{ErrorModule::ARP, 31};
constexpr Result ResultAlreadyBound{ErrorModule::ARP, 42};
constexpr Result ResultProcessIdNotRegistered{ErrorModule::ARP, 102};

} // namespace Service::Glue
