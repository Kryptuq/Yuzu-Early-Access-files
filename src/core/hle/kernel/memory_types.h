// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/common_types.h"

namespace Kernel {

constexpr std::size_t PageBits{12};
constexpr std::size_t PageSize{1 << PageBits};

using Page = std::array<u8, PageSize>;

using KPhysicalAddress = PAddr;
using KProcessAddress = VAddr;

} // namespace Kernel
