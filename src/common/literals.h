// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Common::Literals {

constexpr u64 operator""_KiB(unsigned long long int x) {
    return 1024ULL * x;
}

constexpr u64 operator""_MiB(unsigned long long int x) {
    return 1024_KiB * x;
}

constexpr u64 operator""_GiB(unsigned long long int x) {
    return 1024_MiB * x;
}

constexpr u64 operator""_TiB(unsigned long long int x) {
    return 1024_GiB * x;
}

constexpr u64 operator""_PiB(unsigned long long int x) {
    return 1024_TiB * x;
}

} // namespace Common::Literals
