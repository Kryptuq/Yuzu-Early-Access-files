/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace mp {

/// Binds the first sizeof...(A) arguments of metafunction F with arguments A
template<template<class...> class F, class... As>
struct bind {
    template<class... Rs>
    using type = F<As..., Rs...>;
};

} // namespace mp

#define MM_MP_BIND(...) ::mp::bind<__VA_ARGS__>::template type
