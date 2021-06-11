/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#include "stream.h"

namespace Sirit {

Stream::Stream(std::vector<u32>& words_) : words{words_} {}

Stream::~Stream() = default;

void Stream::Write(std::string_view string) {
    constexpr std::size_t word_size = 4;
    const auto size = string.size();
    const auto read = [string, size](std::size_t offset) {
        return offset < size ? static_cast<u8>(string[offset]) : u8(0);
    };

    words.reserve(words.size() + size / word_size + 1);
    for (std::size_t i = 0; i < size; i += word_size) {
        Write(read(i), read(i + 1), read(i + 2), read(i + 3));
    }
    if (size % word_size == 0) {
        Write(u32(0));
    }
}

void Stream::Write(u64 value) {
    const u32 dword[] = {static_cast<u32>(value), static_cast<u32>(value >> 32)};
    words.insert(std::begin(words), std::cbegin(dword), std::cend(dword));
}

void Stream::Write(u32 value) {
    words.push_back(value);
}

void Stream::Write(u16 first, u16 second) {
    const u32 word = static_cast<u32>(first) | static_cast<u32>(second) << 16;
    Write(word);
}

void Stream::Write(u8 first, u8 second, u8 third, u8 fourth) {
    const u32 word = static_cast<u32>(first) | static_cast<u32>(second) << 8 |
                     static_cast<u32>(third) << 16 | static_cast<u32>(fourth) << 24;
    Write(word);
}

} // namespace Sirit
