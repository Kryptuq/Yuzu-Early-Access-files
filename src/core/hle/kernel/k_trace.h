// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Kernel {

constexpr bool IsKTraceEnabled = false;
constexpr std::size_t KTraceBufferSize = IsKTraceEnabled ? 16 * 1024 * 1024 : 0;

} // namespace Kernel
