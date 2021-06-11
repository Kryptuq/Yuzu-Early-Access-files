// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Kernel {

struct KAddressSpaceInfo final {
    enum class Type : u32 {
        MapSmall = 0,
        MapLarge = 1,
        Map39Bit = 2,
        Heap = 3,
        Stack = 4,
        Alias = 5,
        Count,
    };

    static u64 GetAddressSpaceStart(std::size_t width, Type type);
    static std::size_t GetAddressSpaceSize(std::size_t width, Type type);

    const std::size_t bit_width{};
    const std::size_t address{};
    const std::size_t size{};
    const Type type{};
};

} // namespace Kernel
