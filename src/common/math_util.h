// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <cstdlib>
#include <type_traits>

namespace Common {

constexpr float PI = 3.1415926535f;

template <class T>
struct Rectangle {
    T left{};
    T top{};
    T right{};
    T bottom{};

    constexpr Rectangle() = default;

    constexpr Rectangle(T width, T height) : right(width), bottom(height) {}

    constexpr Rectangle(T left_, T top_, T right_, T bottom_)
        : left(left_), top(top_), right(right_), bottom(bottom_) {}

    [[nodiscard]] constexpr T Left() const {
        return left;
    }

    [[nodiscard]] constexpr T Top() const {
        return top;
    }

    [[nodiscard]] constexpr T Right() const {
        return right;
    }

    [[nodiscard]] constexpr T Bottom() const {
        return bottom;
    }

    [[nodiscard]] constexpr bool IsEmpty() const {
        return (GetWidth() <= 0) || (GetHeight() <= 0);
    }

    [[nodiscard]] constexpr T GetWidth() const {
        if constexpr (std::is_floating_point_v<T>) {
            return std::abs(right - left);
        } else {
            return static_cast<T>(std::abs(static_cast<std::make_signed_t<T>>(right - left)));
        }
    }

    [[nodiscard]] constexpr T GetHeight() const {
        if constexpr (std::is_floating_point_v<T>) {
            return std::abs(bottom - top);
        } else {
            return static_cast<T>(std::abs(static_cast<std::make_signed_t<T>>(bottom - top)));
        }
    }

    [[nodiscard]] constexpr Rectangle<T> TranslateX(const T x) const {
        return Rectangle{left + x, top, right + x, bottom};
    }

    [[nodiscard]] constexpr Rectangle<T> TranslateY(const T y) const {
        return Rectangle{left, top + y, right, bottom + y};
    }

    [[nodiscard]] constexpr Rectangle<T> Scale(const float s) const {
        return Rectangle{left, top, static_cast<T>(static_cast<float>(left + GetWidth()) * s),
                         static_cast<T>(static_cast<float>(top + GetHeight()) * s)};
    }

    [[nodiscard]] constexpr bool operator==(const Rectangle<T>& rhs) const {
        return (left == rhs.left) && (top == rhs.top) && (right == rhs.right) &&
               (bottom == rhs.bottom);
    }

    [[nodiscard]] constexpr bool operator!=(const Rectangle<T>& rhs) const {
        return !operator==(rhs);
    }

    [[nodiscard]] constexpr bool Intersect(const Rectangle<T>& with, Rectangle<T>* result) const {
        result->left = std::max(left, with.left);
        result->top = std::max(top, with.top);
        result->right = std::min(right, with.right);
        result->bottom = std::min(bottom, with.bottom);
        return !result->IsEmpty();
    }
};

template <typename T>
Rectangle(T, T, T, T) -> Rectangle<T>;

} // namespace Common
