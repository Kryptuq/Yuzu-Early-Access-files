// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/bitsizeof.hpp"
#include "mcl/stdint.hpp"

namespace mcl::detail {

/// if MSB is 0, this is a full slot. remaining 7 bits is a partial hash of the key.
/// if MSB is 1, this is a non-full slot.
enum class meta_byte : u8 {
    empty = 0xff,
    tombstone = 0x80,
    end_sentinel = 0x88,
};

inline bool is_full(meta_byte mb)
{
    return (static_cast<u8>(mb) & 0x80) == 0;
}

inline meta_byte meta_byte_from_hash(size_t hash)
{
    return static_cast<meta_byte>(hash >> (bitsizeof<size_t> - 7));
}

inline size_t group_index_from_hash(size_t hash, size_t group_index_mask)
{
    return hash & group_index_mask;
}

}  // namespace mcl::detail
