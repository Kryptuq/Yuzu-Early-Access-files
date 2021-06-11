/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 *
 * Original version of table by Lioncash.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/decoder/decoder_detail.h"
#include "dynarmic/frontend/decoder/matcher.h"

namespace Dynarmic::A32 {

template<typename Visitor>
using ArmMatcher = Decoder::Matcher<Visitor, u32>;

template<typename V>
std::vector<ArmMatcher<V>> GetArmDecodeTable() {
    std::vector<ArmMatcher<V>> table = {

#define INST(fn, name, bitstring) DYNARMIC_DECODER_GET_MATCHER(ArmMatcher, fn, name, Decoder::detail::StringToArray<32>(bitstring)),
#include "./arm.inc"
#undef INST

    };

    // If a matcher has more bits in its mask it is more specific, so it should come first.
    std::stable_sort(table.begin(), table.end(), [](const auto& matcher1, const auto& matcher2) {
        return Common::BitCount(matcher1.GetMask()) > Common::BitCount(matcher2.GetMask());
    });

    return table;
}

template<typename V>
std::optional<std::reference_wrapper<const ArmMatcher<V>>> DecodeArm(u32 instruction) {
    static const auto table = GetArmDecodeTable<V>();

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? std::optional<std::reference_wrapper<const ArmMatcher<V>>>(*iter) : std::nullopt;
}

}  // namespace Dynarmic::A32
