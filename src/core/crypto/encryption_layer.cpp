// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/crypto/encryption_layer.h"

namespace Core::Crypto {

EncryptionLayer::EncryptionLayer(FileSys::VirtualFile base_) : base(std::move(base_)) {}

std::string EncryptionLayer::GetName() const {
    return base->GetName();
}

std::size_t EncryptionLayer::GetSize() const {
    return base->GetSize();
}

bool EncryptionLayer::Resize(std::size_t new_size) {
    return false;
}

std::shared_ptr<FileSys::VfsDirectory> EncryptionLayer::GetContainingDirectory() const {
    return base->GetContainingDirectory();
}

bool EncryptionLayer::IsWritable() const {
    return false;
}

bool EncryptionLayer::IsReadable() const {
    return true;
}

std::size_t EncryptionLayer::Write(const u8* data, std::size_t length, std::size_t offset) {
    return 0;
}

bool EncryptionLayer::Rename(std::string_view name) {
    return base->Rename(name);
}
} // namespace Core::Crypto
