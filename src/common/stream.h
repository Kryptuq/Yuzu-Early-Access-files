// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/common_types.h"

namespace Common {

enum class SeekOrigin {
    SetOrigin,
    FromCurrentPos,
    FromEnd,
};

class Stream {
public:
    /// Stream creates a bitstream and provides common functionality on the stream.
    explicit Stream();
    ~Stream();

    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    Stream(Stream&&) = default;
    Stream& operator=(Stream&&) = default;

    /// Reposition bitstream "cursor" to the specified offset from origin
    void Seek(s32 offset, SeekOrigin origin);

    /// Reads next byte in the stream buffer and increments position
    u8 ReadByte();

    /// Writes byte at current position
    void WriteByte(u8 byte);

    [[nodiscard]] std::size_t GetPosition() const {
        return position;
    }

    [[nodiscard]] std::vector<u8>& GetBuffer() {
        return buffer;
    }

    [[nodiscard]] const std::vector<u8>& GetBuffer() const {
        return buffer;
    }

private:
    std::vector<u8> buffer;
    std::size_t position{0};
};

} // namespace Common
