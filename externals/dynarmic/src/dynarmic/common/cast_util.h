/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstring>
#include <type_traits>

#include <mp/traits/function_info.h>

namespace Dynarmic::Common {

/// Reinterpret objects of one type as another by bit-casting between object representations.
template<class Dest, class Source>
inline Dest BitCast(const Source& source) noexcept {
    static_assert(sizeof(Dest) == sizeof(Source), "size of destination and source objects must be equal");
    static_assert(std::is_trivially_copyable_v<Dest>, "destination type must be trivially copyable.");
    static_assert(std::is_trivially_copyable_v<Source>, "source type must be trivially copyable");

    std::aligned_storage_t<sizeof(Dest), alignof(Dest)> dest;
    std::memcpy(&dest, &source, sizeof(dest));
    return reinterpret_cast<Dest&>(dest);
}

/// Reinterpret objects of any arbitrary type as another type by bit-casting between object representations.
/// Note that here we do not verify if source has enough bytes to read from.
template<class Dest, class SourcePtr>
inline Dest BitCastPointee(const SourcePtr source) noexcept {
    static_assert(sizeof(SourcePtr) == sizeof(void*), "source pointer must have size of a pointer");
    static_assert(std::is_trivially_copyable_v<Dest>, "destination type must be trivially copyable.");

    std::aligned_storage_t<sizeof(Dest), alignof(Dest)> dest;
    std::memcpy(&dest, BitCast<void*>(source), sizeof(dest));
    return reinterpret_cast<Dest&>(dest);
}

/// Cast a lambda into an equivalent function pointer.
template<class Function>
inline auto FptrCast(Function f) noexcept {
    return static_cast<mp::equivalent_function_type<Function>*>(f);
}

}  // namespace Dynarmic::Common
