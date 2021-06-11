// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <span>
#include <type_traits>
#include "common/common_types.h"
#include "core/file_sys/vfs.h"

namespace Core::Crypto {

struct CipherContext;

enum class Mode {
    CTR = 11,
    ECB = 2,
    XTS = 70,
};

enum class Op {
    Encrypt,
    Decrypt,
};

template <typename Key, std::size_t KeySize = sizeof(Key)>
class AESCipher {
    static_assert(std::is_same_v<Key, std::array<u8, KeySize>>, "Key must be std::array of u8.");
    static_assert(KeySize == 0x10 || KeySize == 0x20, "KeySize must be 128 or 256.");

public:
    AESCipher(Key key, Mode mode);
    ~AESCipher();

    void SetIV(std::span<const u8> data);

    template <typename Source, typename Dest>
    void Transcode(const Source* src, std::size_t size, Dest* dest, Op op) const {
        static_assert(std::is_trivially_copyable_v<Source> && std::is_trivially_copyable_v<Dest>,
                      "Transcode source and destination types must be trivially copyable.");
        Transcode(reinterpret_cast<const u8*>(src), size, reinterpret_cast<u8*>(dest), op);
    }

    void Transcode(const u8* src, std::size_t size, u8* dest, Op op) const;

    template <typename Source, typename Dest>
    void XTSTranscode(const Source* src, std::size_t size, Dest* dest, std::size_t sector_id,
                      std::size_t sector_size, Op op) {
        static_assert(std::is_trivially_copyable_v<Source> && std::is_trivially_copyable_v<Dest>,
                      "XTSTranscode source and destination types must be trivially copyable.");
        XTSTranscode(reinterpret_cast<const u8*>(src), size, reinterpret_cast<u8*>(dest), sector_id,
                     sector_size, op);
    }

    void XTSTranscode(const u8* src, std::size_t size, u8* dest, std::size_t sector_id,
                      std::size_t sector_size, Op op);

private:
    std::unique_ptr<CipherContext> ctx;
};
} // namespace Core::Crypto
