/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/value.h>

namespace mp {

/// Does list L contain an element which is same as type T?
template<class L, class T>
struct contains;

template<template<class...> class LT, class... Ts, class T>
struct contains<LT<Ts...>, T>
    : bool_value<(false || ... || std::is_same_v<Ts, T>)>
{};

/// Does list L contain an element which is same as type T?
template<class L, class T>
constexpr bool contains_v = contains<L, T>::value;

} // namespace mp
