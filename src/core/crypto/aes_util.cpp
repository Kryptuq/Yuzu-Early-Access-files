// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <mbedtls/cipher.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/crypto/aes_util.h"
#include "core/crypto/key_manager.h"

namespace Core::Crypto {
namespace {
using NintendoTweak = std::array<u8, 16>;

NintendoTweak CalculateNintendoTweak(std::size_t sector_id) {
    NintendoTweak out{};
    for (std::size_t i = 0xF; i <= 0xF; --i) {
        out[i] = sector_id & 0xFF;
        sector_id >>= 8;
    }
    return out;
}
} // Anonymous namespace

static_assert(static_cast<std::size_t>(Mode::CTR) ==
                  static_cast<std::size_t>(MBEDTLS_CIPHER_AES_128_CTR),
              "CTR has incorrect value.");
static_assert(static_cast<std::size_t>(Mode::ECB) ==
                  static_cast<std::size_t>(MBEDTLS_CIPHER_AES_128_ECB),
              "ECB has incorrect value.");
static_assert(static_cast<std::size_t>(Mode::XTS) ==
                  static_cast<std::size_t>(MBEDTLS_CIPHER_AES_128_XTS),
              "XTS has incorrect value.");

// Structure to hide mbedtls types from header file
struct CipherContext {
    mbedtls_cipher_context_t encryption_context;
    mbedtls_cipher_context_t decryption_context;
};

template <typename Key, std::size_t KeySize>
Crypto::AESCipher<Key, KeySize>::AESCipher(Key key, Mode mode)
    : ctx(std::make_unique<CipherContext>()) {
    mbedtls_cipher_init(&ctx->encryption_context);
    mbedtls_cipher_init(&ctx->decryption_context);

    ASSERT_MSG((mbedtls_cipher_setup(
                    &ctx->encryption_context,
                    mbedtls_cipher_info_from_type(static_cast<mbedtls_cipher_type_t>(mode))) ||
                mbedtls_cipher_setup(
                    &ctx->decryption_context,
                    mbedtls_cipher_info_from_type(static_cast<mbedtls_cipher_type_t>(mode)))) == 0,
               "Failed to initialize mbedtls ciphers.");

    ASSERT(
        !mbedtls_cipher_setkey(&ctx->encryption_context, key.data(), KeySize * 8, MBEDTLS_ENCRYPT));
    ASSERT(
        !mbedtls_cipher_setkey(&ctx->decryption_context, key.data(), KeySize * 8, MBEDTLS_DECRYPT));
    //"Failed to set key on mbedtls ciphers.");
}

template <typename Key, std::size_t KeySize>
AESCipher<Key, KeySize>::~AESCipher() {
    mbedtls_cipher_free(&ctx->encryption_context);
    mbedtls_cipher_free(&ctx->decryption_context);
}

template <typename Key, std::size_t KeySize>
void AESCipher<Key, KeySize>::Transcode(const u8* src, std::size_t size, u8* dest, Op op) const {
    auto* const context = op == Op::Encrypt ? &ctx->encryption_context : &ctx->decryption_context;

    mbedtls_cipher_reset(context);

    std::size_t written = 0;
    if (mbedtls_cipher_get_cipher_mode(context) == MBEDTLS_MODE_XTS) {
        mbedtls_cipher_update(context, src, size, dest, &written);
        if (written != size) {
            LOG_WARNING(Crypto, "Not all data was decrypted requested={:016X}, actual={:016X}.",
                        size, written);
        }
    } else {
        const auto block_size = mbedtls_cipher_get_block_size(context);
        if (size < block_size) {
            std::vector<u8> block(block_size);
            std::memcpy(block.data(), src, size);
            Transcode(block.data(), block.size(), block.data(), op);
            std::memcpy(dest, block.data(), size);
            return;
        }

        for (std::size_t offset = 0; offset < size; offset += block_size) {
            auto length = std::min<std::size_t>(block_size, size - offset);
            mbedtls_cipher_update(context, src + offset, length, dest + offset, &written);
            if (written != length) {
                if (length < block_size) {
                    std::vector<u8> block(block_size);
                    std::memcpy(block.data(), src + offset, length);
                    Transcode(block.data(), block.size(), block.data(), op);
                    std::memcpy(dest + offset, block.data(), length);
                    return;
                }
                LOG_WARNING(Crypto, "Not all data was decrypted requested={:016X}, actual={:016X}.",
                            length, written);
            }
        }
    }
}

template <typename Key, std::size_t KeySize>
void AESCipher<Key, KeySize>::XTSTranscode(const u8* src, std::size_t size, u8* dest,
                                           std::size_t sector_id, std::size_t sector_size, Op op) {
    ASSERT_MSG(size % sector_size == 0, "XTS decryption size must be a multiple of sector size.");

    for (std::size_t i = 0; i < size; i += sector_size) {
        SetIV(CalculateNintendoTweak(sector_id++));
        Transcode(src + i, sector_size, dest + i, op);
    }
}

template <typename Key, std::size_t KeySize>
void AESCipher<Key, KeySize>::SetIV(std::span<const u8> data) {
    ASSERT_MSG((mbedtls_cipher_set_iv(&ctx->encryption_context, data.data(), data.size()) ||
                mbedtls_cipher_set_iv(&ctx->decryption_context, data.data(), data.size())) == 0,
               "Failed to set IV on mbedtls ciphers.");
}

template class AESCipher<Key128>;
template class AESCipher<Key256>;
} // namespace Core::Crypto
