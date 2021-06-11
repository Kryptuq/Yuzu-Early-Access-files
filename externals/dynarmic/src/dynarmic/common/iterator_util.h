/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <iterator>

namespace Dynarmic::Common {
namespace detail {

template<typename T>
struct ReverseAdapter {
    T& iterable;

    constexpr auto begin() {
        using namespace std;
        return rbegin(iterable);
    }

    constexpr auto end() {
        using namespace std;
        return rend(iterable);
    }
};

}  // namespace detail

template<typename T>
constexpr detail::ReverseAdapter<T> Reverse(T&& iterable) {
    return detail::ReverseAdapter<T>{iterable};
}

}  // namespace Dynarmic::Common
