// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <memory>
#include <string_view>
#include "core/file_sys/vfs.h"

namespace FileSys {

// Class that wraps multiple vfs files and concatenates them, making reads seamless. Currently
// read-only.
class ConcatenatedVfsFile : public VfsFile {
    explicit ConcatenatedVfsFile(std::vector<VirtualFile> files, std::string name_);
    explicit ConcatenatedVfsFile(std::multimap<u64, VirtualFile> files, std::string name_);

public:
    ~ConcatenatedVfsFile() override;

    /// Wrapper function to allow for more efficient handling of files.size() == 0, 1 cases.
    static VirtualFile MakeConcatenatedFile(std::vector<VirtualFile> files, std::string name);

    /// Convenience function that turns a map of offsets to files into a concatenated file, filling
    /// gaps with a given filler byte.
    static VirtualFile MakeConcatenatedFile(u8 filler_byte, std::multimap<u64, VirtualFile> files,
                                            std::string name);

    std::string GetName() const override;
    std::size_t GetSize() const override;
    bool Resize(std::size_t new_size) override;
    VirtualDir GetContainingDirectory() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override;
    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override;
    bool Rename(std::string_view new_name) override;

private:
    // Maps starting offset to file -- more efficient.
    std::multimap<u64, VirtualFile> files;
    std::string name;
};

} // namespace FileSys
