// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <lz4hc.h>

#include "common/assert.h"
#include "common/lz4_compression.h"

namespace Common::Compression {

std::vector<u8> CompressDataLZ4(const u8* source, std::size_t source_size) {
    ASSERT_MSG(source_size <= LZ4_MAX_INPUT_SIZE, "Source size exceeds LZ4 maximum input size");

    const auto source_size_int = static_cast<int>(source_size);
    const auto max_compressed_size = static_cast<std::size_t>(LZ4_compressBound(source_size_int));
    std::vector<u8> compressed(max_compressed_size);

    const int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(source), reinterpret_cast<char*>(compressed.data()),
        source_size_int, static_cast<int>(max_compressed_size));

    if (compressed_size <= 0) {
        // Compression failed
        return {};
    }

    compressed.resize(static_cast<std::size_t>(compressed_size));

    return compressed;
}

std::vector<u8> CompressDataLZ4HC(const u8* source, std::size_t source_size,
                                  s32 compression_level) {
    ASSERT_MSG(source_size <= LZ4_MAX_INPUT_SIZE, "Source size exceeds LZ4 maximum input size");

    compression_level = std::clamp(compression_level, LZ4HC_CLEVEL_MIN, LZ4HC_CLEVEL_MAX);

    const auto source_size_int = static_cast<int>(source_size);
    const auto max_compressed_size = static_cast<std::size_t>(LZ4_compressBound(source_size_int));
    std::vector<u8> compressed(max_compressed_size);

    const int compressed_size = LZ4_compress_HC(
        reinterpret_cast<const char*>(source), reinterpret_cast<char*>(compressed.data()),
        source_size_int, static_cast<int>(max_compressed_size), compression_level);

    if (compressed_size <= 0) {
        // Compression failed
        return {};
    }

    compressed.resize(static_cast<std::size_t>(compressed_size));

    return compressed;
}

std::vector<u8> CompressDataLZ4HCMax(const u8* source, std::size_t source_size) {
    return CompressDataLZ4HC(source, source_size, LZ4HC_CLEVEL_MAX);
}

std::vector<u8> DecompressDataLZ4(std::span<const u8> compressed, std::size_t uncompressed_size) {
    std::vector<u8> uncompressed(uncompressed_size);
    const int size_check = LZ4_decompress_safe(reinterpret_cast<const char*>(compressed.data()),
                                               reinterpret_cast<char*>(uncompressed.data()),
                                               static_cast<int>(compressed.size()),
                                               static_cast<int>(uncompressed.size()));
    if (static_cast<int>(uncompressed_size) != size_check) {
        // Decompression failed
        return {};
    }
    return uncompressed;
}

} // namespace Common::Compression
