// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/hex_util.h"

namespace Common {

std::vector<u8> HexStringToVector(std::string_view str, bool little_endian) {
    std::vector<u8> out(str.size() / 2);
    if (little_endian) {
        for (std::size_t i = str.size() - 2; i <= str.size(); i -= 2)
            out[i / 2] = (ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]);
    } else {
        for (std::size_t i = 0; i < str.size(); i += 2)
            out[i / 2] = (ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]);
    }
    return out;
}

} // namespace Common
