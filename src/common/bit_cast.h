// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <version>

#ifdef __cpp_lib_bit_cast
#include <bit>
#endif

namespace Common {

template <typename To, typename From>
constexpr inline To BitCast(const From& from) {
#ifdef __cpp_lib_bit_cast
    return std::bit_cast<To>(from);
#else
    return __builtin_bit_cast(To, from);
#endif
}

} // namespace Common
