/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <vector>

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/decoder/decoder_detail.h"
#include "dynarmic/frontend/decoder/matcher.h"

namespace Dynarmic::A32 {

template<typename Visitor>
using ASIMDMatcher = Decoder::Matcher<Visitor, u32>;

template<typename V>
std::vector<ASIMDMatcher<V>> GetASIMDDecodeTable() {
    std::vector<ASIMDMatcher<V>> table = {

#define INST(fn, name, bitstring) DYNARMIC_DECODER_GET_MATCHER(ASIMDMatcher, fn, name, Decoder::detail::StringToArray<32>(bitstring)),
#include "./asimd.inc"
#undef INST

    };

    // Exceptions to the rule of thumb.
    const std::set<std::string> comes_first{
        "VBIC, VMOV, VMVN, VORR (immediate)",
        "VEXT",
        "VTBL",
        "VTBX",
        "VDUP (scalar)",
    };
    const std::set<std::string> comes_last{
        "VMLA (scalar)",
        "VMLAL (scalar)",
        "VQDMLAL/VQDMLSL (scalar)",
        "VMUL (scalar)",
        "VMULL (scalar)",
        "VQDMULL (scalar)",
        "VQDMULH (scalar)",
        "VQRDMULH (scalar)",
    };
    const auto sort_begin = std::stable_partition(table.begin(), table.end(), [&](const auto& matcher) {
        return comes_first.count(matcher.GetName()) > 0;
    });
    const auto sort_end = std::stable_partition(table.begin(), table.end(), [&](const auto& matcher) {
        return comes_last.count(matcher.GetName()) == 0;
    });

    // If a matcher has more bits in its mask it is more specific, so it should come first.
    std::stable_sort(sort_begin, sort_end, [](const auto& matcher1, const auto& matcher2) {
        return Common::BitCount(matcher1.GetMask()) > Common::BitCount(matcher2.GetMask());
    });

    return table;
}

template<typename V>
std::optional<std::reference_wrapper<const ASIMDMatcher<V>>> DecodeASIMD(u32 instruction) {
    static const auto table = GetASIMDDecodeTable<V>();

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? std::optional<std::reference_wrapper<const ASIMDMatcher<V>>>(*iter) : std::nullopt;
}

}  // namespace Dynarmic::A32
