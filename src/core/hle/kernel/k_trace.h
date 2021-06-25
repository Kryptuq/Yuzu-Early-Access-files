// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"

namespace Kernel {

using namespace Common::Literals;

constexpr bool IsKTraceEnabled = false;
constexpr std::size_t KTraceBufferSize = IsKTraceEnabled ? 16_MiB : 0;

} // namespace Kernel
