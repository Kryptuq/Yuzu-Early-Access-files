/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mp/metavalue/value.h>

namespace mp {

/// Is type T an instance of template class C?
template <template <class...> class, class>
struct is_instance_of_template : false_type {};

template <template <class...> class C, class... As>
struct is_instance_of_template<C, C<As...>> : true_type {};

/// Is type T an instance of template class C?
template<template <class...> class C, class T>
constexpr bool is_instance_of_template_v = is_instance_of_template<C, T>::value;

} // namespace mp
