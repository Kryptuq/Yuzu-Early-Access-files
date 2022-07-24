// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using uptr = std::uintptr_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using sptr = std::intptr_t;

using size_t = std::size_t;

using f32 = float;
using f64 = double;
static_assert(sizeof(f32) == sizeof(u32), "f32 must be 32 bits wide");
static_assert(sizeof(f64) == sizeof(u64), "f64 must be 64 bits wide");
