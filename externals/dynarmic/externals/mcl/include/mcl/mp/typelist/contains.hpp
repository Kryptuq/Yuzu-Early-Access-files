// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/value.hpp"

namespace mcl::mp {

/// Does list L contain an element which is same as type T?
template<class L, class T>
struct contains;

template<template<class...> class LT, class... Ts, class T>
struct contains<LT<Ts...>, T>
    : bool_value<(false || ... || std::is_same_v<Ts, T>)> {};

/// Does list L contain an element which is same as type T?
template<class L, class T>
constexpr bool contains_v = contains<L, T>::value;

}  // namespace mcl::mp
