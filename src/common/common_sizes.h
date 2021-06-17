// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <limits>

#include "common/common_types.h"

namespace Common {

enum : u64 {
    Size_1_KB = 0x400ULL,
    Size_64_KB = 64ULL * Size_1_KB,
    Size_128_KB = 128ULL * Size_1_KB,
    Size_1_MB = 0x100000ULL,
    Size_2_MB = 2ULL * Size_1_MB,
    Size_4_MB = 4ULL * Size_1_MB,
    Size_5_MB = 5ULL * Size_1_MB,
    Size_14_MB = 14ULL * Size_1_MB,
    Size_32_MB = 32ULL * Size_1_MB,
    Size_33_MB = 33ULL * Size_1_MB,
    Size_128_MB = 128ULL * Size_1_MB,
    Size_448_MB = 448ULL * Size_1_MB,
    Size_507_MB = 507ULL * Size_1_MB,
    Size_512_MB = 512ULL * Size_1_MB,
    Size_562_MB = 562ULL * Size_1_MB,
    Size_1554_MB = 1554ULL * Size_1_MB,
    Size_2048_MB = 2048ULL * Size_1_MB,
    Size_2193_MB = 2193ULL * Size_1_MB,
    Size_3285_MB = 3285ULL * Size_1_MB,
    Size_4916_MB = 4916ULL * Size_1_MB,
    Size_1_GB = 0x40000000ULL,
    Size_2_GB = 2ULL * Size_1_GB,
    Size_4_GB = 4ULL * Size_1_GB,
    Size_6_GB = 6ULL * Size_1_GB,
    Size_8_GB = 8ULL * Size_1_GB,
    Size_64_GB = 64ULL * Size_1_GB,
    Size_512_GB = 512ULL * Size_1_GB,
    Size_Invalid = std::numeric_limits<u64>::max(),
};

} // namespace Common
