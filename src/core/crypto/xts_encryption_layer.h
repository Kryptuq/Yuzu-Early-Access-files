// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/crypto/aes_util.h"
#include "core/crypto/encryption_layer.h"
#include "core/crypto/key_manager.h"

namespace Core::Crypto {

// Sits on top of a VirtualFile and provides XTS-mode AES description.
class XTSEncryptionLayer : public EncryptionLayer {
public:
    XTSEncryptionLayer(FileSys::VirtualFile base, Key256 key);

    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override;

private:
    // Must be mutable as operations modify cipher contexts.
    mutable AESCipher<Key256> cipher;
};

} // namespace Core::Crypto
