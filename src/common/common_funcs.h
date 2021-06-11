// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <array>
#include <string>

#if !defined(ARCHITECTURE_x86_64)
#include <cstdlib> // for exit
#endif
#include "common/common_types.h"

/// Textually concatenates two tokens. The double-expansion is required by the C preprocessor.
#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y

/// Helper macros to insert unused bytes or words to properly align structs. These values will be
/// zero-initialized.
#define INSERT_PADDING_BYTES(num_bytes)                                                            \
    std::array<u8, num_bytes> CONCAT2(pad, __LINE__) {}
#define INSERT_PADDING_WORDS(num_words)                                                            \
    std::array<u32, num_words> CONCAT2(pad, __LINE__) {}

/// These are similar to the INSERT_PADDING_* macros but do not zero-initialize the contents.
/// This keeps the structure trivial to construct.
#define INSERT_PADDING_BYTES_NOINIT(num_bytes) std::array<u8, num_bytes> CONCAT2(pad, __LINE__)
#define INSERT_PADDING_WORDS_NOINIT(num_words) std::array<u32, num_words> CONCAT2(pad, __LINE__)

#ifndef _MSC_VER

#ifdef ARCHITECTURE_x86_64
#define Crash() __asm__ __volatile__("int $3")
#else
#define Crash() exit(1)
#endif

#else // _MSC_VER

// Locale Cross-Compatibility
#define locale_t _locale_t

extern "C" {
__declspec(dllimport) void __stdcall DebugBreak(void);
}
#define Crash() DebugBreak()

#endif // _MSC_VER ndef

// Generic function to get last error message.
// Call directly after the command or use the error num.
// This function might change the error code.
// Defined in misc.cpp.
[[nodiscard]] std::string GetLastErrorMsg();

// Like GetLastErrorMsg(), but passing an explicit error code.
// Defined in misc.cpp.
[[nodiscard]] std::string NativeErrorToString(int e);

#define DECLARE_ENUM_FLAG_OPERATORS(type)                                                          \
    [[nodiscard]] constexpr type operator|(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) | static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator&(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) & static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator^(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) ^ static_cast<T>(b));                           \
    }                                                                                              \
    constexpr type& operator|=(type& a, type b) noexcept {                                         \
        a = a | b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator&=(type& a, type b) noexcept {                                         \
        a = a & b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator^=(type& a, type b) noexcept {                                         \
        a = a ^ b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    [[nodiscard]] constexpr type operator~(type key) noexcept {                                    \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(~static_cast<T>(key));                                            \
    }                                                                                              \
    [[nodiscard]] constexpr bool True(type key) noexcept {                                         \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<T>(key) != 0;                                                           \
    }                                                                                              \
    [[nodiscard]] constexpr bool False(type key) noexcept {                                        \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<T>(key) == 0;                                                           \
    }

#define YUZU_NON_COPYABLE(cls)                                                                     \
    cls(const cls&) = delete;                                                                      \
    cls& operator=(const cls&) = delete

#define YUZU_NON_MOVEABLE(cls)                                                                     \
    cls(cls&&) = delete;                                                                           \
    cls& operator=(cls&&) = delete

namespace Common {

[[nodiscard]] constexpr u32 MakeMagic(char a, char b, char c, char d) {
    return u32(a) | u32(b) << 8 | u32(c) << 16 | u32(d) << 24;
}

// std::size() does not support zero-size C arrays. We're fixing that.
template <class C>
constexpr auto Size(const C& c) -> decltype(c.size()) {
    return std::size(c);
}

template <class C>
constexpr std::size_t Size(const C& c) {
    if constexpr (sizeof(C) == 0) {
        return 0;
    } else {
        return std::size(c);
    }
}

} // namespace Common
