// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <bit>
#include <optional>
#include <random>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/tiny_mt.h"
#include "common/uuid.h"

namespace Common {

namespace {

constexpr size_t RawStringSize = sizeof(UUID) * 2;
constexpr size_t FormattedStringSize = RawStringSize + 4;

std::optional<u8> HexCharToByte(char c) {
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
    return std::nullopt;
}

std::array<u8, 0x10> ConstructFromRawString(std::string_view raw_string) {
    std::array<u8, 0x10> uuid;

    for (size_t i = 0; i < RawStringSize; i += 2) {
        const auto upper = HexCharToByte(raw_string[i]);
        const auto lower = HexCharToByte(raw_string[i + 1]);
        if (!upper || !lower) {
            return {};
        }
        uuid[i / 2] = static_cast<u8>((*upper << 4) | *lower);
    }

    return uuid;
}

std::array<u8, 0x10> ConstructFromFormattedString(std::string_view formatted_string) {
    std::array<u8, 0x10> uuid;

    size_t i = 0;

    // Process the first 8 characters.
    const auto* str = formatted_string.data();

    for (; i < 4; ++i) {
        const auto upper = HexCharToByte(*(str++));
        const auto lower = HexCharToByte(*(str++));
        if (!upper || !lower) {
            return {};
        }
        uuid[i] = static_cast<u8>((*upper << 4) | *lower);
    }

    // Process the next 4 characters.
    ++str;

    for (; i < 6; ++i) {
        const auto upper = HexCharToByte(*(str++));
        const auto lower = HexCharToByte(*(str++));
        if (!upper || !lower) {
            return {};
        }
        uuid[i] = static_cast<u8>((*upper << 4) | *lower);
    }

    // Process the next 4 characters.
    ++str;

    for (; i < 8; ++i) {
        const auto upper = HexCharToByte(*(str++));
        const auto lower = HexCharToByte(*(str++));
        if (!upper || !lower) {
            return {};
        }
        uuid[i] = static_cast<u8>((*upper << 4) | *lower);
    }

    // Process the next 4 characters.
    ++str;

    for (; i < 10; ++i) {
        const auto upper = HexCharToByte(*(str++));
        const auto lower = HexCharToByte(*(str++));
        if (!upper || !lower) {
            return {};
        }
        uuid[i] = static_cast<u8>((*upper << 4) | *lower);
    }

    // Process the last 12 characters.
    ++str;

    for (; i < 16; ++i) {
        const auto upper = HexCharToByte(*(str++));
        const auto lower = HexCharToByte(*(str++));
        if (!upper || !lower) {
            return {};
        }
        uuid[i] = static_cast<u8>((*upper << 4) | *lower);
    }

    return uuid;
}

std::array<u8, 0x10> ConstructUUID(std::string_view uuid_string) {
    const auto length = uuid_string.length();

    if (length == 0) {
        return {};
    }

    // Check if the input string contains 32 hexadecimal characters.
    if (length == RawStringSize) {
        return ConstructFromRawString(uuid_string);
    }

    // Check if the input string has the length of a RFC 4122 formatted UUID string.
    if (length == FormattedStringSize) {
        return ConstructFromFormattedString(uuid_string);
    }

    ASSERT_MSG(false, "UUID string has an invalid length of {} characters!", length);

    return {};
}

} // Anonymous namespace

UUID::UUID(std::string_view uuid_string) : uuid{ConstructUUID(uuid_string)} {}

std::string UUID::RawString() const {
    return fmt::format("{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}"
                       "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                       uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
                       uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
                       uuid[15]);
}

std::string UUID::FormattedString() const {
    return fmt::format("{:02x}{:02x}{:02x}{:02x}"
                       "-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-"
                       "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                       uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
                       uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
                       uuid[15]);
}

size_t UUID::Hash() const noexcept {
    u64 hash;
    u64 temp;

    std::memcpy(&hash, uuid.data(), sizeof(u64));
    std::memcpy(&temp, uuid.data() + 8, sizeof(u64));

    return hash ^ std::rotl(temp, 1);
}

u128 UUID::AsU128() const {
    u128 uuid_old;
    std::memcpy(&uuid_old, uuid.data(), sizeof(UUID));
    return uuid_old;
}

UUID UUID::MakeRandom() {
    std::random_device device;

    return MakeRandomWithSeed(device());
}

UUID UUID::MakeRandomWithSeed(u32 seed) {
    // Create and initialize our RNG.
    TinyMT rng;
    rng.Initialize(seed);

    UUID uuid;

    // Populate the UUID with random bytes.
    rng.GenerateRandomBytes(uuid.uuid.data(), sizeof(UUID));

    return uuid;
}

UUID UUID::MakeRandomRFC4122V4() {
    auto uuid = MakeRandom();

    // According to Proposed Standard RFC 4122 Section 4.4, we must:

    // 1. Set the two most significant bits (bits 6 and 7) of the
    //    clock_seq_hi_and_reserved to zero and one, respectively.
    uuid.uuid[8] = 0x80 | (uuid.uuid[8] & 0x3F);

    // 2. Set the four most significant bits (bits 12 through 15) of the
    //    time_hi_and_version field to the 4-bit version number from Section 4.1.3.
    uuid.uuid[6] = 0x40 | (uuid.uuid[6] & 0xF);

    return uuid;
}

} // namespace Common
