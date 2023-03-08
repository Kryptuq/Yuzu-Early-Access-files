// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>
#include "bit_cast.h"

namespace Common {

template <typename T>
    requires(std::is_integral_v<T> && std::is_signed_v<T>)
inline T WrappingAdd(T lhs, T rhs) {
    using U = std::make_unsigned_t<T>;

    U lhs_u = BitCast<U>(lhs);
    U rhs_u = BitCast<U>(rhs);

    return BitCast<T>(lhs_u + rhs_u);
}

} // namespace Common
