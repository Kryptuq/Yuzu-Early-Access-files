// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstring>
#include <type_traits>

namespace mcl {

/// Reinterpret objects of one type as another by bit-casting between object representations.
template<class Dest, class Source>
inline Dest bit_cast(const Source& source) noexcept {
    static_assert(sizeof(Dest) == sizeof(Source), "size of destination and source objects must be equal");
    static_assert(std::is_trivially_copyable_v<Dest>, "destination type must be trivially copyable.");
    static_assert(std::is_trivially_copyable_v<Source>, "source type must be trivially copyable");

    std::aligned_storage_t<sizeof(Dest), alignof(Dest)> dest;
    std::memcpy(&dest, &source, sizeof(dest));
    return reinterpret_cast<Dest&>(dest);
}

/// Reinterpret objects of any arbitrary type as another type by bit-casting between object representations.
/// Note that here we do not verify if source pointed to by source_ptr has enough bytes to read from.
template<class Dest, class SourcePtr>
inline Dest bit_cast_pointee(const SourcePtr source_ptr) noexcept {
    static_assert(sizeof(SourcePtr) == sizeof(void*), "source pointer must have size of a pointer");
    static_assert(std::is_trivially_copyable_v<Dest>, "destination type must be trivially copyable.");

    std::aligned_storage_t<sizeof(Dest), alignof(Dest)> dest;
    std::memcpy(&dest, bit_cast<void*>(source_ptr), sizeof(dest));
    return reinterpret_cast<Dest&>(dest);
}

}  // namespace mcl
