// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <type_traits>

namespace Common {

// Represents a point within a 2D space.
template <typename T>
struct Point {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type!");

    T x{};
    T y{};

#define ARITHMETIC_OP(op, compound_op)                                                             \
    friend constexpr Point operator op(const Point& lhs, const Point& rhs) noexcept {              \
        return {                                                                                   \
            .x = static_cast<T>(lhs.x op rhs.x),                                                   \
            .y = static_cast<T>(lhs.y op rhs.y),                                                   \
        };                                                                                         \
    }                                                                                              \
    friend constexpr Point operator op(const Point& lhs, T value) noexcept {                       \
        return {                                                                                   \
            .x = static_cast<T>(lhs.x op value),                                                   \
            .y = static_cast<T>(lhs.y op value),                                                   \
        };                                                                                         \
    }                                                                                              \
    friend constexpr Point operator op(T value, const Point& rhs) noexcept {                       \
        return {                                                                                   \
            .x = static_cast<T>(value op rhs.x),                                                   \
            .y = static_cast<T>(value op rhs.y),                                                   \
        };                                                                                         \
    }                                                                                              \
    friend constexpr Point& operator compound_op(Point& lhs, const Point& rhs) noexcept {          \
        lhs.x = static_cast<T>(lhs.x op rhs.x);                                                    \
        lhs.y = static_cast<T>(lhs.y op rhs.y);                                                    \
        return lhs;                                                                                \
    }                                                                                              \
    friend constexpr Point& operator compound_op(Point& lhs, T value) noexcept {                   \
        lhs.x = static_cast<T>(lhs.x op value);                                                    \
        lhs.y = static_cast<T>(lhs.y op value);                                                    \
        return lhs;                                                                                \
    }
    ARITHMETIC_OP(+, +=)
    ARITHMETIC_OP(-, -=)
    ARITHMETIC_OP(*, *=)
    ARITHMETIC_OP(/, /=)
#undef ARITHMETIC_OP

    friend constexpr bool operator==(const Point&, const Point&) = default;
};

} // namespace Common
