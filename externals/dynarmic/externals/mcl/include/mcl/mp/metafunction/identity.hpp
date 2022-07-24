// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace mcl::mp {

namespace detail {

template<class T>
struct identity_impl {
    using type = T;
};

}  // namespace detail

/// Identity metafunction
template<class T>
using identity = typename identity_impl<T>::type;

}  // namespace mcl::mp
