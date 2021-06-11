/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <tuple>

namespace mp {

/// Converts a list of metavalues to a tuple.
template<class L>
struct lower_to_tuple;

template<template<class...> class LT, class... Es>
struct lower_to_tuple<LT<Es...>> {
    static constexpr auto value = std::make_tuple(static_cast<typename Es::value_type>(Es::value)...);
};

/// Converts a list of metavalues to a tuple.
template<class L>
constexpr auto lower_to_tuple_v = lower_to_tuple<L>::value;

} // namespace mp
