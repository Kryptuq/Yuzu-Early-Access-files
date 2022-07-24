// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "core/file_sys/vfs.h"

namespace FileSys {

// Class that stacks multiple VfsDirectories on top of each other, attempting to read from the first
// one and falling back to the one after. The highest priority directory (overwrites all others)
// should be element 0 in the dirs vector.
class LayeredVfsDirectory : public VfsDirectory {
    explicit LayeredVfsDirectory(std::vector<VirtualDir> dirs_, std::string name_);

public:
    ~LayeredVfsDirectory() override;

    /// Wrapper function to allow for more efficient handling of dirs.size() == 0, 1 cases.
    static VirtualDir MakeLayeredDirectory(std::vector<VirtualDir> dirs, std::string name = "");

    VirtualFile GetFileRelative(std::string_view path) const override;
    VirtualDir GetDirectoryRelative(std::string_view path) const override;
    VirtualFile GetFile(std::string_view file_name) const override;
    VirtualDir GetSubdirectory(std::string_view subdir_name) const override;
    std::string GetFullPath() const override;

    std::vector<VirtualFile> GetFiles() const override;
    std::vector<VirtualDir> GetSubdirectories() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::string GetName() const override;
    VirtualDir GetParentDirectory() const override;
    VirtualDir CreateSubdirectory(std::string_view subdir_name) override;
    VirtualFile CreateFile(std::string_view file_name) override;
    bool DeleteSubdirectory(std::string_view subdir_name) override;
    bool DeleteFile(std::string_view file_name) override;
    bool Rename(std::string_view new_name) override;

private:
    std::vector<VirtualDir> dirs;
    std::string name;
};

} // namespace FileSys
