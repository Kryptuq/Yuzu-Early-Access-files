// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/file_sys/vfs.h"

namespace Core::Crypto {

// Basically non-functional class that implements all of the methods that are irrelevant to an
// EncryptionLayer. Reduces duplicate code.
class EncryptionLayer : public FileSys::VfsFile {
public:
    explicit EncryptionLayer(FileSys::VirtualFile base);

    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override = 0;

    std::string GetName() const override;
    std::size_t GetSize() const override;
    bool Resize(std::size_t new_size) override;
    std::shared_ptr<FileSys::VfsDirectory> GetContainingDirectory() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override;
    bool Rename(std::string_view name) override;

protected:
    FileSys::VirtualFile base;
};

} // namespace Core::Crypto
