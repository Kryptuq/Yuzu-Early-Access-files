// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#include <random>
#include <type_traits>

template<typename T>
T RandInt(T min, T max)
{
    static_assert(std::is_integral_v<T>, "T must be an integral type.");
    static_assert(!std::is_same_v<T, signed char> && !std::is_same_v<T, unsigned char>,
                  "Using char with uniform_int_distribution is undefined behavior.");

    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_int_distribution<T> rand(min, max);
    return rand(mt);
}
