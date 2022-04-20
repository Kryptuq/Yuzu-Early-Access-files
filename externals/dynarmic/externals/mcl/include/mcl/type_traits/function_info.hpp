// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <tuple>

#include "mcl/mp/typelist/list.hpp"

namespace mcl {

template<class F>
struct function_info : function_info<decltype(&F::operator())> {};

template<class R, class... As>
struct function_info<R(As...)> {
    using return_type = R;
    using parameter_list = mp::list<As...>;
    static constexpr std::size_t parameter_count = sizeof...(As);

    using equivalent_function_type = R(As...);

    template<std::size_t I>
    struct parameter {
        static_assert(I < parameter_count, "Non-existent parameter");
        using type = std::tuple_element_t<I, std::tuple<As...>>;
    };
};

template<class R, class... As>
struct function_info<R (*)(As...)> : function_info<R(As...)> {};

template<class C, class R, class... As>
struct function_info<R (C::*)(As...)> : function_info<R(As...)> {
    using class_type = C;

    using equivalent_function_type_with_class = R(C*, As...);
};

template<class C, class R, class... As>
struct function_info<R (C::*)(As...) const> : function_info<R(As...)> {
    using class_type = C;

    using equivalent_function_type_with_class = R(C*, As...);
};

template<class F>
constexpr size_t parameter_count_v = function_info<F>::parameter_count;

template<class F>
using parameter_list = typename function_info<F>::parameter_list;

template<class F, std::size_t I>
using get_parameter = typename function_info<F>::template parameter<I>::type;

template<class F>
using equivalent_function_type = typename function_info<F>::equivalent_function_type;

template<class F>
using equivalent_function_type_with_class = typename function_info<F>::equivalent_function_type_with_class;

template<class F>
using return_type = typename function_info<F>::return_type;

template<class F>
using class_type = typename function_info<F>::class_type;

}  // namespace mcl
