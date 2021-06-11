// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <stdexcept>
#include "common/common_types.h"
#include "common/stream.h"

namespace Common {

Stream::Stream() = default;
Stream::~Stream() = default;

void Stream::Seek(s32 offset, SeekOrigin origin) {
    if (origin == SeekOrigin::SetOrigin) {
        if (offset < 0) {
            position = 0;
        } else if (position >= buffer.size()) {
            position = buffer.size();
        } else {
            position = offset;
        }
    } else if (origin == SeekOrigin::FromCurrentPos) {
        Seek(static_cast<s32>(position) + offset, SeekOrigin::SetOrigin);
    } else if (origin == SeekOrigin::FromEnd) {
        Seek(static_cast<s32>(buffer.size()) - offset, SeekOrigin::SetOrigin);
    }
}

u8 Stream::ReadByte() {
    if (position < buffer.size()) {
        return buffer[position++];
    } else {
        throw std::out_of_range("Attempting to read a byte not within the buffer range");
    }
}

void Stream::WriteByte(u8 byte) {
    if (position == buffer.size()) {
        buffer.push_back(byte);
        position++;
    } else {
        buffer.insert(buffer.begin() + position, byte);
    }
}

} // namespace Common
