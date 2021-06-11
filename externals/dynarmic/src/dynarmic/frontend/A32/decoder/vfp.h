/* This file is part of the dynarmic project.
 * Copyright (c) 2032 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/decoder/decoder_detail.h"
#include "dynarmic/frontend/decoder/matcher.h"

namespace Dynarmic::A32 {

template<typename Visitor>
using VFPMatcher = Decoder::Matcher<Visitor, u32>;

template<typename V>
std::optional<std::reference_wrapper<const VFPMatcher<V>>> DecodeVFP(u32 instruction) {
    using Table = std::vector<VFPMatcher<V>>;

    static const struct Tables {
        Table unconditional;
        Table conditional;
    } tables = [] {
        Table list = {

#define INST(fn, name, bitstring) DYNARMIC_DECODER_GET_MATCHER(VFPMatcher, fn, name, Decoder::detail::StringToArray<32>(bitstring)),
#include "./vfp.inc"
#undef INST

        };

        const auto division = std::stable_partition(list.begin(), list.end(), [&](const auto& matcher) {
            return (matcher.GetMask() & 0xF0000000) == 0xF0000000;
        });

        return Tables{
            Table{list.begin(), division},
            Table{division, list.end()},
        };
    }();

    const bool is_unconditional = (instruction & 0xF0000000) == 0xF0000000;
    const Table& table = is_unconditional ? tables.unconditional : tables.conditional;

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? std::optional<std::reference_wrapper<const VFPMatcher<V>>>(*iter) : std::nullopt;
}

}  // namespace Dynarmic::A32
