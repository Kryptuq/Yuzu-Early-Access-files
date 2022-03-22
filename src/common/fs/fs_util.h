// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <concepts>
#include <filesystem>
#include <span>
#include <string>

#include "common/common_types.h"

namespace Common::FS {

template <typename T>
concept IsChar = std::same_as<T, char>;

/**
 * Converts a UTF-8 encoded std::string or std::string_view to a std::u8string.
 *
 * @param utf8_string UTF-8 encoded string
 *
 * @returns UTF-8 encoded std::u8string.
 */
[[nodiscard]] std::u8string ToU8String(std::string_view utf8_string);

/**
 * Converts a buffer of bytes to a UTF8-encoded std::u8string.
 * This converts from the start of the buffer until the first encountered null-terminator.
 * If no null-terminator is found, this converts the entire buffer instead.
 *
 * @param buffer Buffer of bytes
 *
 * @returns UTF-8 encoded std::u8string.
 */
[[nodiscard]] std::u8string BufferToU8String(std::span<const u8> buffer);

/**
 * Same as BufferToU8String, but returns a string view of the buffer.
 *
 * @param buffer Buffer of bytes
 *
 * @returns UTF-8 encoded std::u8string_view.
 */
[[nodiscard]] std::u8string_view BufferToU8StringView(std::span<const u8> buffer);

/**
 * Converts a std::u8string or std::u8string_view to a UTF-8 encoded std::string.
 *
 * @param u8_string UTF-8 encoded u8string
 *
 * @returns UTF-8 encoded std::string.
 */
[[nodiscard]] std::string ToUTF8String(std::u8string_view u8_string);

/**
 * Converts a buffer of bytes to a UTF8-encoded std::string.
 * This converts from the start of the buffer until the first encountered null-terminator.
 * If no null-terminator is found, this converts the entire buffer instead.
 *
 * @param buffer Buffer of bytes
 *
 * @returns UTF-8 encoded std::string.
 */
[[nodiscard]] std::string BufferToUTF8String(std::span<const u8> buffer);

/**
 * Same as BufferToUTF8String, but returns a string view of the buffer.
 *
 * @param buffer Buffer of bytes
 *
 * @returns UTF-8 encoded std::string_view.
 */
[[nodiscard]] std::string_view BufferToUTF8StringView(std::span<const u8> buffer);

/**
 * Converts a filesystem path to a UTF-8 encoded std::string.
 *
 * @param path Filesystem path
 *
 * @returns UTF-8 encoded std::string.
 */
[[nodiscard]] std::string PathToUTF8String(const std::filesystem::path& path);

} // namespace Common::FS
