// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl::mp {

namespace detail {

template<class L>
struct tail_impl;

template<template<class...> class LT, class E1, class... Es>
struct tail_impl<LT<E1, Es...>> {
    using type = LT<Es...>;
};

}  // namespace detail

/// Gets the first type of list L
template<class L>
using tail = typename detail::tail_impl<L>::type;

}  // namespace mcl::mp
