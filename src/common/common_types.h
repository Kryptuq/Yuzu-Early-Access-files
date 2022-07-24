// SPDX-FileCopyrightText: 2012 Gekko Emulator
// SPDX-FileContributor: ShizZy <shizzy247@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * Copyright (C) 2005-2012 Gekko Emulator
 *
 * @file    common_types.h
 * @author  ShizZy <shizzy247@gmail.com>
 * @date    2012-02-11
 * @brief   Common types used throughout the project
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Official project repository can be found at:
 * http://code.google.com/p/gekko-gc-emu/
 */

#pragma once

#include <array>
#include <cstdint>

using u8 = std::uint8_t;   ///< 8-bit unsigned byte
using u16 = std::uint16_t; ///< 16-bit unsigned short
using u32 = std::uint32_t; ///< 32-bit unsigned word
using u64 = std::uint64_t; ///< 64-bit unsigned int

using s8 = std::int8_t;   ///< 8-bit signed byte
using s16 = std::int16_t; ///< 16-bit signed short
using s32 = std::int32_t; ///< 32-bit signed word
using s64 = std::int64_t; ///< 64-bit signed int

using f32 = float;  ///< 32-bit floating point
using f64 = double; ///< 64-bit floating point

using VAddr = u64;    ///< Represents a pointer in the userspace virtual address space.
using PAddr = u64;    ///< Represents a pointer in the ARM11 physical address space.
using GPUVAddr = u64; ///< Represents a pointer in the GPU virtual address space.

using u128 = std::array<std::uint64_t, 2>;
static_assert(sizeof(u128) == 16, "u128 must be 128 bits wide");
