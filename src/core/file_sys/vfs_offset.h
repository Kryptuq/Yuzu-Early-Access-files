// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "core/file_sys/vfs.h"

namespace FileSys {

// An implementation of VfsFile that wraps around another VfsFile at a certain offset.
// Similar to seeking to an offset.
// If the file is writable, operations that would write past the end of the offset file will expand
// the size of this wrapper.
class OffsetVfsFile : public VfsFile {
public:
    OffsetVfsFile(VirtualFile file, std::size_t size, std::size_t offset = 0,
                  std::string new_name = "", VirtualDir new_parent = nullptr);
    ~OffsetVfsFile() override;

    std::string GetName() const override;
    std::size_t GetSize() const override;
    bool Resize(std::size_t new_size) override;
    VirtualDir GetContainingDirectory() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override;
    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override;
    std::optional<u8> ReadByte(std::size_t offset) const override;
    std::vector<u8> ReadBytes(std::size_t size, std::size_t offset) const override;
    std::vector<u8> ReadAllBytes() const override;
    bool WriteByte(u8 data, std::size_t offset) override;
    std::size_t WriteBytes(const std::vector<u8>& data, std::size_t offset) override;

    bool Rename(std::string_view new_name) override;

    std::size_t GetOffset() const;

private:
    std::size_t TrimToFit(std::size_t r_size, std::size_t r_offset) const;

    VirtualFile file;
    std::size_t offset;
    std::size_t size;
    std::string name;
    VirtualDir parent;
};

} // namespace FileSys
