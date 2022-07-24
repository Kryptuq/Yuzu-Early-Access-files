// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/common_types.h"

// This is to consolidate system-wide constants that are used by multiple components of yuzu.
// This is especially to prevent the case of something in frontend duplicating a constexpr array or
// directly including some service header for the sole purpose of data.
namespace Core::Constants {

// ACC Service - Blank JPEG used as user icon in absentia of real one.
extern const std::array<u8, 107> ACCOUNT_BACKUP_JPEG;

} // namespace Core::Constants
