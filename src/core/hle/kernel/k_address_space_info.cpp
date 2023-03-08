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
   { .bit_width = 36, .address = Size_Invalid, .size = 8_GiB            , .type = KAddressSpaceInfo::Type::Heap,     },
   { .bit_width = 36, .address = Size_Invalid, .size = 6_GiB            , .type = KAddressSpaceInfo::Type::Alias,    },
   { .bit_width = 39, .address = 128_MiB     , .size = 512_GiB - 128_MiB, .type = KAddressSpaceInfo::Type::Map39Bit, },
   { .bit_width = 39, .address = Size_Invalid, .size = 64_GiB           , .type = KAddressSpaceInfo::Type::MapSmall  },
   { .bit_width = 39, .address = Size_Invalid, .size = 8_GiB            , .type = KAddressSpaceInfo::Type::Heap,     },
   { .bit_width = 39, .address = Size_Invalid, .size = 64_GiB           , .type = KAddressSpaceInfo::Type::Alias,    },
   { .bit_width = 39, .address = Size_Invalid, .size = 2_GiB            , .type = KAddressSpaceInfo::Type::Stack,    },
}};
// clang-format on

const KAddressSpaceInfo& GetAddressSpaceInfo(size_t width, KAddressSpaceInfo::Type type) {
    for (auto& info : AddressSpaceInfos) {
        if (info.bit_width == width && info.type == type) {
            return info;
        }
    }
    UNREACHABLE_MSG("Could not find AddressSpaceInfo");
}

} // namespace

std::size_t KAddressSpaceInfo::GetAddressSpaceStart(size_t width, KAddressSpaceInfo::Type type) {
    return GetAddressSpaceInfo(width, type).address;
}

std::size_t KAddressSpaceInfo::GetAddressSpaceSize(size_t width, KAddressSpaceInfo::Type type) {
    return GetAddressSpaceInfo(width, type).size;
}

} // namespace Kernel
