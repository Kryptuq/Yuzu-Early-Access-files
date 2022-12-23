// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <type_traits>

namespace Common {

template <class T>
requires(!std::is_array_v<T>) std::unique_ptr<T> make_unique_for_overwrite() {
    return std::unique_ptr<T>(new T);
}

template <class T>
requires std::is_unbounded_array_v<T> std::unique_ptr<T> make_unique_for_overwrite(std::size_t n) {
    return std::unique_ptr<T>(new std::remove_extent_t<T>[n]);
}

template <class T, class... Args>
requires std::is_bounded_array_v<T>
void make_unique_for_overwrite(Args&&...) = delete;

} // namespace Common
