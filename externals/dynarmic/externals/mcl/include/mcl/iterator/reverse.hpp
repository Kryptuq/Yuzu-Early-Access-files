// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <iterator>

namespace mcl::iterator {
namespace detail {

template<typename T>
struct reverse_adapter {
    T& iterable;

    constexpr auto begin()
    {
        using namespace std;
        return rbegin(iterable);
    }

    constexpr auto end()
    {
        using namespace std;
        return rend(iterable);
    }
};

}  // namespace detail

template<typename T>
constexpr detail::reverse_adapter<T> reverse(T&& iterable)
{
    return detail::reverse_adapter<T>{iterable};
}

}  // namespace mcl::iterator
