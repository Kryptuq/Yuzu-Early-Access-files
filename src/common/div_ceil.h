// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <type_traits>

namespace Common {

/// Ceiled integer division.
template <typename N, typename D>
requires std::is_integral_v<N> && std::is_unsigned_v<D>
[[nodiscard]] constexpr N DivCeil(N number, D divisor) {
    return static_cast<N>((static_cast<D>(number) + divisor - 1) / divisor);
}

/// Ceiled integer division with logarithmic divisor in base 2
template <typename N, typename D>
requires std::is_integral_v<N> && std::is_unsigned_v<D>
[[nodiscard]] constexpr N DivCeilLog2(N value, D alignment_log2) {
    return static_cast<N>((static_cast<D>(value) + (D(1) << alignment_log2) - 1) >> alignment_log2);
}

} // namespace Common
