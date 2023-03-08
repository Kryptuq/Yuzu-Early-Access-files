// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

    static std::size_t GetAddressSpaceStart(std::size_t width, Type type);
    static std::size_t GetAddressSpaceSize(std::size_t width, Type type);

    const std::size_t bit_width{};
    const std::size_t address{};
    const std::size_t size{};
    const Type type{};
};

} // namespace Kernel
