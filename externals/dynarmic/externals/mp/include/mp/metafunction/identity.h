/* This file is part of the mp project.
 * Copyright (c) 2017 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace mp {

namespace detail {

template<class T>
struct identity_impl {
    using type = T;
};

} // namespace detail

/// Identity metafunction
template<class T>
using identity = typename identity_impl<T>::type;

} // namespace mp
