// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/mp/metavalue/value.hpp"

namespace mcl {

/// Is type T an instance of template class C?
template<template<class...> class, class>
struct is_instance_of_template : mp::false_type {};

template<template<class...> class C, class... As>
struct is_instance_of_template<C, C<As...>> : mp::true_type {};

/// Is type T an instance of template class C?
template<template<class...> class C, class T>
constexpr bool is_instance_of_template_v = is_instance_of_template<C, T>::value;

}  // namespace mcl
