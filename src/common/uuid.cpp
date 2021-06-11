// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <random>

#include <fmt/format.h>

#include "common/uuid.h"

namespace Common {

UUID UUID::Generate() {
    std::random_device device;
    std::mt19937 gen(device());
    std::uniform_int_distribution<u64> distribution(1, std::numeric_limits<u64>::max());
    return UUID{distribution(gen), distribution(gen)};
}

std::string UUID::Format() const {
    return fmt::format("0x{:016X}{:016X}", uuid[1], uuid[0]);
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
