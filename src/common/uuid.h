// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <string_view>

#include "common/common_types.h"

namespace Common {

constexpr u128 INVALID_UUID{{0, 0}};

/**
 * Converts a hex string to a 128-bit unsigned integer.
 *
 * The hex string can be formatted in lowercase or uppercase, with or without the "0x" prefix.
 *
 * This function will assert and return INVALID_UUID under the following conditions:
 * - If the hex string is more than 32 characters long
 * - If the hex string contains non-hexadecimal characters
 *
 * @param hex_string Hexadecimal string
 *
 * @returns A 128-bit unsigned integer if successfully converted, INVALID_UUID otherwise.
 */
[[nodiscard]] u128 HexStringToU128(std::string_view hex_string);

struct UUID {
    // UUIDs which are 0 are considered invalid!
    u128 uuid;
    UUID() = default;
    constexpr explicit UUID(const u128& id) : uuid{id} {}
    constexpr explicit UUID(const u64 lo, const u64 hi) : uuid{{lo, hi}} {}
    explicit UUID(std::string_view hex_string) {
        uuid = HexStringToU128(hex_string);
    }

    [[nodiscard]] constexpr explicit operator bool() const {
        return uuid != INVALID_UUID;
    }

    [[nodiscard]] constexpr bool operator==(const UUID& rhs) const {
        return uuid == rhs.uuid;
    }

    [[nodiscard]] constexpr bool operator!=(const UUID& rhs) const {
        return !operator==(rhs);
    }

    // TODO(ogniK): Properly generate uuids based on RFC-4122
    [[nodiscard]] static UUID Generate();

    // Set the UUID to {0,0} to be considered an invalid user
    constexpr void Invalidate() {
        uuid = INVALID_UUID;
    }

    [[nodiscard]] constexpr bool IsInvalid() const {
        return uuid == INVALID_UUID;
    }
    [[nodiscard]] constexpr bool IsValid() const {
        return !IsInvalid();
    }

    // TODO(ogniK): Properly generate a Nintendo ID
    [[nodiscard]] constexpr u64 GetNintendoID() const {
        return uuid[0];
    }

    [[nodiscard]] std::string Format() const;
    [[nodiscard]] std::string FormatSwitch() const;
};
static_assert(sizeof(UUID) == 16, "UUID is an invalid size!");

} // namespace Common

namespace std {

template <>
struct hash<Common::UUID> {
    size_t operator()(const Common::UUID& uuid) const noexcept {
        return uuid.uuid[1] ^ uuid.uuid[0];
    }
};

} // namespace std
