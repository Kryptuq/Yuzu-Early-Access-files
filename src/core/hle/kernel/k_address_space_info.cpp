// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "common/assert.h"
#include "common/literals.h"
#include "core/hle/kernel/k_address_space_info.h"

namespace Kernel {

namespace {

using namespace Common::Literals;

constexpr u64 Size_Invalid = UINT64_MAX;

// clang-format off
constexpr std::array<KAddressSpaceInfo, 13> AddressSpaceInfos{{
   { .bit_width = 32, .address = 2_MiB       , .size = 1_GiB   - 2_MiB  , .type = KAddressSpaceInfo::Type::MapSmall, },
   { .bit_width = 32, .address = 1_GiB       , .size = 4_GiB   - 1_GiB  , .type = KAddressSpaceInfo::Type::MapLarge, },
   { .bit_width = 32, .address = Size_Invalid, .size = 1_GiB            , .type = KAddressSpaceInfo::Type::Alias,    },
   { .bit_width = 32, .address = Size_Invalid, .size = 1_GiB            , .type = KAddressSpaceInfo::Type::Heap,     },
   { .bit_width = 36, .address = 128_MiB     , .size = 2_GiB   - 128_MiB, .type = KAddressSpaceInfo::Type::MapSmall, },
   { .bit_width = 36, .address = 2_GiB       , .size = 64_GiB  - 2_GiB  , .type = KAddressSpaceInfo::Type::MapLarge, },
   { .bit_width = 36, .address = Size_Invalid, .size = 6_GiB            , .type = KAddressSpaceInfo::Type::Heap,     },
   { .bit_width = 36, .address = Size_Invalid, .size = 6_GiB            , .type = KAddressSpaceInfo::Type::Alias,    },
   { .bit_width = 39, .address = 128_MiB     , .size = 512_GiB - 128_MiB, .type = KAddressSpaceInfo::Type::Map39Bit, },
   { .bit_width = 39, .address = Size_Invalid, .size = 64_GiB           , .type = KAddressSpaceInfo::Type::MapSmall  },
   { .bit_width = 39, .address = Size_Invalid, .size = 6_GiB            , .type = KAddressSpaceInfo::Type::Heap,     },
   { .bit_width = 39, .address = Size_Invalid, .size = 64_GiB           , .type = KAddressSpaceInfo::Type::Alias,    },
   { .bit_width = 39, .address = Size_Invalid, .size = 2_GiB            , .type = KAddressSpaceInfo::Type::Stack,    },
}};
// clang-format on

constexpr bool IsAllowedIndexForAddress(std::size_t index) {
    return index < AddressSpaceInfos.size() && AddressSpaceInfos[index].address != Size_Invalid;
}

using IndexArray =
    std::array<std::size_t, static_cast<std::size_t>(KAddressSpaceInfo::Type::Count)>;

constexpr IndexArray AddressSpaceIndices32Bit{
    0, 1, 0, 2, 0, 3,
};

constexpr IndexArray AddressSpaceIndices36Bit{
    4, 5, 4, 6, 4, 7,
};

constexpr IndexArray AddressSpaceIndices39Bit{
    9, 8, 8, 10, 12, 11,
};

constexpr bool IsAllowed32BitType(KAddressSpaceInfo::Type type) {
    return type < KAddressSpaceInfo::Type::Count && type != KAddressSpaceInfo::Type::Map39Bit &&
           type != KAddressSpaceInfo::Type::Stack;
}

constexpr bool IsAllowed36BitType(KAddressSpaceInfo::Type type) {
    return type < KAddressSpaceInfo::Type::Count && type != KAddressSpaceInfo::Type::Map39Bit &&
           type != KAddressSpaceInfo::Type::Stack;
}

constexpr bool IsAllowed39BitType(KAddressSpaceInfo::Type type) {
    return type < KAddressSpaceInfo::Type::Count && type != KAddressSpaceInfo::Type::MapLarge;
}

} // namespace

u64 KAddressSpaceInfo::GetAddressSpaceStart(std::size_t width, Type type) {
    const std::size_t index{static_cast<std::size_t>(type)};
    switch (width) {
    case 32:
        ASSERT(IsAllowed32BitType(type));
        ASSERT(IsAllowedIndexForAddress(AddressSpaceIndices32Bit[index]));
        return AddressSpaceInfos[AddressSpaceIndices32Bit[index]].address;
    case 36:
        ASSERT(IsAllowed36BitType(type));
        ASSERT(IsAllowedIndexForAddress(AddressSpaceIndices36Bit[index]));
        return AddressSpaceInfos[AddressSpaceIndices36Bit[index]].address;
    case 39:
        ASSERT(IsAllowed39BitType(type));
        ASSERT(IsAllowedIndexForAddress(AddressSpaceIndices39Bit[index]));
        return AddressSpaceInfos[AddressSpaceIndices39Bit[index]].address;
    }
    ASSERT(false);
    return 0;
}

std::size_t KAddressSpaceInfo::GetAddressSpaceSize(std::size_t width, Type type) {
    const std::size_t index{static_cast<std::size_t>(type)};
    switch (width) {
    case 32:
        ASSERT(IsAllowed32BitType(type));
        return AddressSpaceInfos[AddressSpaceIndices32Bit[index]].size;
    case 36:
        ASSERT(IsAllowed36BitType(type));
        return AddressSpaceInfos[AddressSpaceIndices36Bit[index]].size;
    case 39:
        ASSERT(IsAllowed39BitType(type));
        return AddressSpaceInfos[AddressSpaceIndices39Bit[index]].size;
    }
    ASSERT(false);
    return 0;
}

} // namespace Kernel
