/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>
#include <type_traits>

namespace mp {

namespace detail {

template<size_t N, class L>
struct drop_impl;

template<size_t N, template<class...> class LT>
struct drop_impl<N, LT<>> {
    using type = LT<>;
};

template<size_t N, template<class...> class LT, class E1, class... Es>
struct drop_impl<N, LT<E1, Es...>> {
    using type = std::conditional_t<N == 0, LT<E1, Es...>, typename drop_impl<N - 1, LT<Es...>>::type>;
};

} // namespace detail

/// Drops the first N elements of list L
template<std::size_t N, class L>
using drop = typename detail::drop_impl<N, L>::type;

} // namespace mp
