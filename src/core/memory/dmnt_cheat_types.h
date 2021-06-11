/*
 * Copyright (c) 2018-2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Adapted by DarkLordZach for use/interaction with yuzu
 *
 * Modifications Copyright 2019 yuzu emulator team
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

#pragma once

#include "common/common_types.h"

namespace Core::Memory {

struct MemoryRegionExtents {
    u64 base{};
    u64 size{};
};

struct CheatProcessMetadata {
    u64 process_id{};
    u64 title_id{};
    MemoryRegionExtents main_nso_extents{};
    MemoryRegionExtents heap_extents{};
    MemoryRegionExtents alias_extents{};
    MemoryRegionExtents address_space_extents{};
    std::array<u8, 0x20> main_nso_build_id{};
};

struct CheatDefinition {
    std::array<char, 0x40> readable_name{};
    u32 num_opcodes{};
    std::array<u32, 0x100> opcodes{};
};

struct CheatEntry {
    bool enabled{};
    u32 cheat_id{};
    CheatDefinition definition{};
};

} // namespace Core::Memory
