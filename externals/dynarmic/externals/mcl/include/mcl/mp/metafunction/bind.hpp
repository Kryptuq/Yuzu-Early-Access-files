// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl::mp {

/// Binds the first sizeof...(A) arguments of metafunction F with arguments A
template<template<class...> class F, class... As>
struct bind {
    template<class... Rs>
    using type = F<As..., Rs...>;
};

}  // namespace mcl::mp

#define MCL_MP_BIND(...) ::mcl::mp::bind<__VA_ARGS__>::template type
