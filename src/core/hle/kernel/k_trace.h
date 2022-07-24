// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Kernel {

using namespace Common::Literals;

constexpr bool IsKTraceEnabled = false;
constexpr std::size_t KTraceBufferSize = IsKTraceEnabled ? 16_MiB : 0;

} // namespace Kernel
