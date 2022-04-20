/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <initializer_list>
#include <map>
#include <type_traits>

#include <mcl/mp/metafunction/apply.hpp>
#include <mcl/mp/typelist/list.hpp>
#include <mcl/type_traits/is_instance_of_template.hpp>

#ifdef _MSC_VER
#    include <mcl/mp/typelist/head.hpp>
#endif

namespace Dynarmic::Common {

template<typename Function, typename... Values>
inline auto GenerateLookupTableFromList(Function f, mcl::mp::list<Values...>) {
#ifdef _MSC_VER
    using PairT = std::invoke_result_t<Function, mcl::mp::head<mcl::mp::list<Values...>>>;
#else
    using PairT = std::common_type_t<std::invoke_result_t<Function, Values>...>;
#endif
    using MapT = mcl::mp::apply<std::map, PairT>;

    static_assert(mcl::is_instance_of_template_v<std::pair, PairT>);

    const std::initializer_list<PairT> pair_array{f(Values{})...};
    return MapT(pair_array.begin(), pair_array.end());
}

}  // namespace Dynarmic::Common
