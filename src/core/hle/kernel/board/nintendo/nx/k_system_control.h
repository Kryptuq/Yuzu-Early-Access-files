// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Kernel::Board::Nintendo::Nx {

class KSystemControl {
public:
    class Init {
    public:
        // Initialization.
        static std::size_t GetRealMemorySize();
        static std::size_t GetIntendedMemorySize();
        static PAddr GetKernelPhysicalBaseAddress(u64 base_address);
        static bool ShouldIncreaseThreadResourceLimit();
        static std::size_t GetApplicationPoolSize();
        static std::size_t GetAppletPoolSize();
        static std::size_t GetMinimumNonSecureSystemPoolSize();
    };

    static u64 GenerateRandomRange(u64 min, u64 max);
    static u64 GenerateRandomU64();
};

} // namespace Kernel::Board::Nintendo::Nx
