// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl::mp {

namespace detail {

template<template<class...> class F, class L>
struct apply_impl;

template<template<class...> class F, template<class...> class LT, class... Es>
struct apply_impl<F, LT<Es...>> {
    using type = F<Es...>;
};

}  // namespace detail

/// Invokes metafunction F where the arguments are all the members of list L
template<template<class...> class F, class L>
using apply = typename detail::apply_impl<F, L>::type;

}  // namespace mcl::mp
