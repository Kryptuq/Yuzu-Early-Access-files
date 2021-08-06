// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <random>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/uuid.h"

namespace Common {

namespace {

bool IsHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

u8 HexCharToByte(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<u8>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<u8>(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<u8>(c - 'A' + 10);
    }
    ASSERT_MSG(false, "{} is not a hexadecimal digit!", c);
    return u8{0};
}

} // Anonymous namespace

u128 HexStringToU128(std::string_view hex_string) {
    const size_t length = hex_string.length();

    // Detect "0x" prefix.
    const bool has_0x_prefix = length > 2 && hex_string[0] == '0' && hex_string[1] == 'x';
    const size_t offset = has_0x_prefix ? 2 : 0;

    // Check length.
    if (length > 32 + offset) {
        ASSERT_MSG(false, "hex_string has more than 32 hexadecimal characters!");
        return INVALID_UUID;
    }

    u64 lo = 0;
    u64 hi = 0;
    for (size_t i = 0; i < length - offset; ++i) {
        const char c = hex_string[length - 1 - i];
        if (!IsHexDigit(c)) {
            ASSERT_MSG(false, "{} is not a hexadecimal digit!", c);
            return INVALID_UUID;
        }
        if (i < 16) {
            lo |= u64{HexCharToByte(c)} << (i * 4);
        }
        if (i >= 16) {
            hi |= u64{HexCharToByte(c)} << ((i - 16) * 4);
        }
    }
    return u128{lo, hi};
}

UUID UUID::Generate() {
    std::random_device device;
    std::mt19937 gen(device());
    std::uniform_int_distribution<u64> distribution(1, std::numeric_limits<u64>::max());
    return UUID{distribution(gen), distribution(gen)};
}

std::string UUID::Format() const {
    return fmt::format("{:016x}{:016x}", uuid[1], uuid[0]);
}

std::string UUID::FormatSwitch() const {
    std::array<u8, 16> s{};
    std::memcpy(s.data(), uuid.data(), sizeof(u128));
    return fmt::format("{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{"
                       ":02x}{:02x}{:02x}{:02x}{:02x}",
                       s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11],
                       s[12], s[13], s[14], s[15]);
}

} // namespace Common
