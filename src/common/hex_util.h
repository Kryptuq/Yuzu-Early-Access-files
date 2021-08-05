// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
#include <fmt/format.h>
#include "common/common_types.h"

namespace Common {

[[nodiscard]] constexpr u8 ToHexNibble(char c) {
    if (c >= 65 && c <= 70) {
        return static_cast<u8>(c - 55);
    }

    if (c >= 97 && c <= 102) {
        return static_cast<u8>(c - 87);
    }

    return static_cast<u8>(c - 48);
}

[[nodiscard]] std::vector<u8> HexStringToVector(std::string_view str, bool little_endian);

template <std::size_t Size, bool le = false>
[[nodiscard]] constexpr std::array<u8, Size> HexStringToArray(std::string_view str) {
    std::array<u8, Size> out{};
    if constexpr (le) {
        for (std::size_t i = 2 * Size - 2; i <= 2 * Size; i -= 2) {
            out[i / 2] = static_cast<u8>((ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]));
        }
    } else {
        for (std::size_t i = 0; i < 2 * Size; i += 2) {
            out[i / 2] = static_cast<u8>((ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]));
        }
    }
    return out;
}

template <typename ContiguousContainer>
[[nodiscard]] std::string HexToString(const ContiguousContainer& data, bool upper = true) {
    static_assert(std::is_same_v<typename ContiguousContainer::value_type, u8>,
                  "Underlying type within the contiguous container must be u8.");

    constexpr std::size_t pad_width = 2;

    std::string out;
    out.reserve(std::size(data) * pad_width);

    const auto format_str = fmt::runtime(upper ? "{:02X}" : "{:02x}");
    for (const u8 c : data) {
        out += fmt::format(format_str, c);
    }

    return out;
}

[[nodiscard]] constexpr std::array<u8, 16> AsArray(const char (&data)[33]) {
    return HexStringToArray<16>(data);
}

[[nodiscard]] constexpr std::array<u8, 32> AsArray(const char (&data)[65]) {
    return HexStringToArray<32>(data);
}

} // namespace Common
