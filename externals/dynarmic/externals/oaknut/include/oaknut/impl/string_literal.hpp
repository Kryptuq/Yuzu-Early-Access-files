// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cstddef>

namespace oaknut {

template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    static constexpr std::size_t strlen = N - 1;
    static constexpr std::size_t size = N;

    char value[N];
};

}  // namespace oaknut
