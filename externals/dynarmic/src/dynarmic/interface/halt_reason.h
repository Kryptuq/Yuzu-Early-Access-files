/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstdint>

namespace Dynarmic {

enum class HaltReason : std::uint32_t {
    Step = 0x00000001,
    CacheInvalidation = 0x00000002,
    UserDefined1 = 0x01000000,
    UserDefined2 = 0x02000000,
    UserDefined3 = 0x04000000,
    UserDefined4 = 0x08000000,
    UserDefined5 = 0x10000000,
    UserDefined6 = 0x20000000,
    UserDefined7 = 0x40000000,
    UserDefined8 = 0x80000000,
};

constexpr HaltReason operator~(HaltReason hr) {
    return static_cast<HaltReason>(~static_cast<std::uint32_t>(hr));
}

constexpr HaltReason operator|(HaltReason hr1, HaltReason hr2) {
    return static_cast<HaltReason>(static_cast<std::uint32_t>(hr1) | static_cast<std::uint32_t>(hr2));
}

constexpr HaltReason operator&(HaltReason hr1, HaltReason hr2) {
    return static_cast<HaltReason>(static_cast<std::uint32_t>(hr1) & static_cast<std::uint32_t>(hr2));
}

constexpr HaltReason operator|=(HaltReason& result, HaltReason hr) {
    return result = (result | hr);
}

constexpr HaltReason operator&=(HaltReason& result, HaltReason hr) {
    return result = (result & hr);
}

constexpr bool operator!(HaltReason hr) {
    return static_cast<std::uint32_t>(hr) == 0;
}

constexpr bool Has(HaltReason hr1, HaltReason hr2) {
    return (static_cast<std::uint32_t>(hr1) & static_cast<std::uint32_t>(hr2)) != 0;
}

}  // namespace Dynarmic
