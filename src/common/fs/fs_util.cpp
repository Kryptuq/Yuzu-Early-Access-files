// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include "common/fs/fs_util.h"

namespace Common::FS {

std::u8string ToU8String(std::string_view utf8_string) {
    return std::u8string{utf8_string.begin(), utf8_string.end()};
}

std::u8string BufferToU8String(std::span<const u8> buffer) {
    return std::u8string{buffer.begin(), std::ranges::find(buffer, u8{0})};
}

std::string ToUTF8String(std::u8string_view u8_string) {
    return std::string{u8_string.begin(), u8_string.end()};
}

std::string BufferToUTF8String(std::span<const u8> buffer) {
    return std::string{buffer.begin(), std::ranges::find(buffer, u8{0})};
}

std::string PathToUTF8String(const std::filesystem::path& path) {
    return ToUTF8String(path.u8string());
}

} // namespace Common::FS
