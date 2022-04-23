// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <boost/container/flat_map.hpp>
#include "core/file_sys/mode.h"
#include "core/file_sys/vfs.h"

namespace Common::FS {
class IOFile;
}

namespace FileSys {

class RealVfsFilesystem : public VfsFilesystem {
public:
    RealVfsFilesystem();
    ~RealVfsFilesystem() override;

    std::string GetName() const override;
    bool IsReadable() const override;
    bool IsWritable() const override;
    VfsEntryType GetEntryType(std::string_view path) const override;
    VirtualFile OpenFile(std::string_view path, Mode perms = Mode::Read) override;
    VirtualFile CreateFile(std::string_view path, Mode perms = Mode::ReadWrite) override;
    VirtualFile CopyFile(std::string_view old_path, std::string_view new_path) override;
    VirtualFile MoveFile(std::string_view old_path, std::string_view new_path) override;
    bool DeleteFile(std::string_view path) override;
    VirtualDir OpenDirectory(std::string_view path, Mode perms = Mode::Read) override;
    VirtualDir CreateDirectory(std::string_view path, Mode perms = Mode::ReadWrite) override;
    VirtualDir CopyDirectory(std::string_view old_path, std::string_view new_path) override;
    VirtualDir MoveDirectory(std::string_view old_path, std::string_view new_path) override;
    bool DeleteDirectory(std::string_view path) override;

private:
    boost::container::flat_map<std::string, std::weak_ptr<Common::FS::IOFile>> cache;
};

// An implmentation of VfsFile that represents a file on the user's computer.
class RealVfsFile : public VfsFile {
    friend class RealVfsDirectory;
    friend class RealVfsFilesystem;

public:
    ~RealVfsFile() override;

    std::string GetName() const override;
    std::size_t GetSize() const override;
    bool Resize(std::size_t new_size) override;
    VirtualDir GetContainingDirectory() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override;
    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override;
    bool Rename(std::string_view name) override;

private:
    RealVfsFile(RealVfsFilesystem& base, std::shared_ptr<Common::FS::IOFile> backing,
                const std::string& path, Mode perms = Mode::Read);

    void Close();

    RealVfsFilesystem& base;
    std::shared_ptr<Common::FS::IOFile> backing;
    std::string path;
    std::string parent_path;
    std::vector<std::string> path_components;
    Mode perms;
};

// An implementation of VfsDirectory that represents a directory on the user's computer.
class RealVfsDirectory : public VfsDirectory {
    friend class RealVfsFilesystem;

public:
    ~RealVfsDirectory() override;

    VirtualFile GetFileRelative(std::string_view relative_path) const override;
    VirtualDir GetDirectoryRelative(std::string_view relative_path) const override;
    VirtualFile GetFile(std::string_view name) const override;
    VirtualDir GetSubdirectory(std::string_view name) const override;
    VirtualFile CreateFileRelative(std::string_view relative_path) override;
    VirtualDir CreateDirectoryRelative(std::string_view relative_path) override;
    bool DeleteSubdirectoryRecursive(std::string_view name) override;
    std::vector<VirtualFile> GetFiles() const override;
    FileTimeStampRaw GetFileTimeStamp(std::string_view path) const override;
    std::vector<VirtualDir> GetSubdirectories() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::string GetName() const override;
    VirtualDir GetParentDirectory() const override;
    VirtualDir CreateSubdirectory(std::string_view name) override;
    VirtualFile CreateFile(std::string_view name) override;
    bool DeleteSubdirectory(std::string_view name) override;
    bool DeleteFile(std::string_view name) override;
    bool Rename(std::string_view name) override;
    std::string GetFullPath() const override;
    std::map<std::string, VfsEntryType, std::less<>> GetEntries() const override;

private:
    RealVfsDirectory(RealVfsFilesystem& base, const std::string& path, Mode perms = Mode::Read);

    template <typename T, typename R>
    std::vector<std::shared_ptr<R>> IterateEntries() const;

    RealVfsFilesystem& base;
    std::string path;
    std::string parent_path;
    std::vector<std::string> path_components;
    Mode perms;
};

} // namespace FileSys
