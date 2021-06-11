// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstring>
#include "common/assert.h"
#include "core/crypto/xts_encryption_layer.h"

namespace Core::Crypto {

constexpr u64 XTS_SECTOR_SIZE = 0x4000;

XTSEncryptionLayer::XTSEncryptionLayer(FileSys::VirtualFile base_, Key256 key_)
    : EncryptionLayer(std::move(base_)), cipher(key_, Mode::XTS) {}

std::size_t XTSEncryptionLayer::Read(u8* data, std::size_t length, std::size_t offset) const {
    if (length == 0)
        return 0;

    const auto sector_offset = offset & 0x3FFF;
    if (sector_offset == 0) {
        if (length % XTS_SECTOR_SIZE == 0) {
            std::vector<u8> raw = base->ReadBytes(length, offset);
            cipher.XTSTranscode(raw.data(), raw.size(), data, offset / XTS_SECTOR_SIZE,
                                XTS_SECTOR_SIZE, Op::Decrypt);
            return raw.size();
        }
        if (length > XTS_SECTOR_SIZE) {
            const auto rem = length % XTS_SECTOR_SIZE;
            const auto read = length - rem;
            return Read(data, read, offset) + Read(data + read, rem, offset + read);
        }
        std::vector<u8> buffer = base->ReadBytes(XTS_SECTOR_SIZE, offset);
        if (buffer.size() < XTS_SECTOR_SIZE)
            buffer.resize(XTS_SECTOR_SIZE);
        cipher.XTSTranscode(buffer.data(), buffer.size(), buffer.data(), offset / XTS_SECTOR_SIZE,
                            XTS_SECTOR_SIZE, Op::Decrypt);
        std::memcpy(data, buffer.data(), std::min(buffer.size(), length));
        return std::min(buffer.size(), length);
    }

    // offset does not fall on block boundary (0x4000)
    std::vector<u8> block = base->ReadBytes(0x4000, offset - sector_offset);
    if (block.size() < XTS_SECTOR_SIZE)
        block.resize(XTS_SECTOR_SIZE);
    cipher.XTSTranscode(block.data(), block.size(), block.data(),
                        (offset - sector_offset) / XTS_SECTOR_SIZE, XTS_SECTOR_SIZE, Op::Decrypt);
    const std::size_t read = XTS_SECTOR_SIZE - sector_offset;

    if (length + sector_offset < XTS_SECTOR_SIZE) {
        std::memcpy(data, block.data() + sector_offset, std::min<u64>(length, read));
        return std::min<u64>(length, read);
    }
    std::memcpy(data, block.data() + sector_offset, read);
    return read + Read(data + read, length - read, offset + read);
}
} // namespace Core::Crypto
