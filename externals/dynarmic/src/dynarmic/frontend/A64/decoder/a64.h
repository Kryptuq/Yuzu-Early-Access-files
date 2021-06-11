/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/decoder/decoder_detail.h"
#include "dynarmic/frontend/decoder/matcher.h"

namespace Dynarmic::A64 {

template<typename Visitor>
using Matcher = Decoder::Matcher<Visitor, u32>;

template<typename Visitor>
using DecodeTable = std::array<std::vector<Matcher<Visitor>>, 0x1000>;

namespace detail {
inline size_t ToFastLookupIndex(u32 instruction) {
    return ((instruction >> 10) & 0x00F) | ((instruction >> 18) & 0xFF0);
}
}  // namespace detail

template<typename V>
DecodeTable<V> GetDecodeTable() {
    std::vector<Matcher<V>> list = {
#define INST(fn, name, bitstring) DYNARMIC_DECODER_GET_MATCHER(Matcher, fn, name, Decoder::detail::StringToArray<32>(bitstring)),
#include "./a64.inc"
#undef INST
    };

    std::stable_sort(list.begin(), list.end(), [](const auto& matcher1, const auto& matcher2) {
        // If a matcher has more bits in its mask it is more specific, so it should come first.
        return Common::BitCount(matcher1.GetMask()) > Common::BitCount(matcher2.GetMask());
    });

    // Exceptions to the above rule of thumb.
    const std::set<std::string> comes_first{
        "MOVI, MVNI, ORR, BIC (vector, immediate)",
        "FMOV (vector, immediate)",
        "Unallocated SIMD modified immediate",
    };

    std::stable_partition(list.begin(), list.end(), [&](const auto& matcher) {
        return comes_first.count(matcher.GetName()) > 0;
    });

    DecodeTable<V> table{};
    for (size_t i = 0; i < table.size(); ++i) {
        for (auto matcher : list) {
            const auto expect = detail::ToFastLookupIndex(matcher.GetExpected());
            const auto mask = detail::ToFastLookupIndex(matcher.GetMask());
            if ((i & mask) == expect) {
                table[i].push_back(matcher);
            }
        }
    }
    return table;
}

template<typename V>
std::optional<std::reference_wrapper<const Matcher<V>>> Decode(u32 instruction) {
    static const auto table = GetDecodeTable<V>();

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    const auto& subtable = table[detail::ToFastLookupIndex(instruction)];
    auto iter = std::find_if(subtable.begin(), subtable.end(), matches_instruction);
    return iter != subtable.end() ? std::optional<std::reference_wrapper<const Matcher<V>>>(*iter) : std::nullopt;
}

}  // namespace Dynarmic::A64
