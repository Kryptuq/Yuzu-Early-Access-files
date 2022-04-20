// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl::mp {

namespace detail {

template<class... L>
struct append_impl;

template<template<class...> class LT, class... E1s, class... E2s>
struct append_impl<LT<E1s...>, E2s...> {
    using type = LT<E1s..., E2s...>;
};

}  // namespace detail

/// Append items E to list L
template<class L, class... Es>
using append = typename detail::append_impl<L, Es...>::type;

}  // namespace mcl::mp
