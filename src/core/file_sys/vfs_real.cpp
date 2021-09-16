// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include "common/assert.h"
#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "core/file_sys/vfs_real.h"

// For FileTimeStampRaw
#include <sys/stat.h>

#ifdef _MSC_VER
#define stat _stat64
#endif

namespace FileSys {

namespace FS = Common::FS;

namespace {

constexpr FS::FileAccessMode ModeFlagsToFileAccessMode(Mode mode) {
    switch (mode) {
    case Mode::Read:
        return FS::FileAccessMode::Read;
    case Mode::Write:
    case Mode::ReadWrite:
    case Mode::Append:
    case Mode::ReadAppend:
    case Mode::WriteAppend:
    case Mode::All:
        return FS::FileAccessMode::ReadWrite;
    default:
        return {};
    }
}

} // Anonymous namespace

RealVfsFilesystem::RealVfsFilesystem() : VfsFilesystem(nullptr) {}
RealVfsFilesystem::~RealVfsFilesystem() = default;

std::string RealVfsFilesystem::GetName() const {
    return "Real";
}

bool RealVfsFilesystem::IsReadable() const {
    return true;
}

bool RealVfsFilesystem::IsWritable() const {
    return true;
}

VfsEntryType RealVfsFilesystem::GetEntryType(std::string_view path_) const {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);
    if (!FS::Exists(path)) {
        return VfsEntryType::None;
    }
    if (FS::IsDir(path)) {
        return VfsEntryType::Directory;
    }

    return VfsEntryType::File;
}

VirtualFile RealVfsFilesystem::OpenFile(std::string_view path_, Mode perms) {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);

    if (const auto weak_iter = cache.find(path); weak_iter != cache.cend()) {
        const auto& weak = weak_iter->second;

        if (!weak.expired()) {
            return std::shared_ptr<RealVfsFile>(new RealVfsFile(*this, weak.lock(), path, perms));
        }
    }

    auto backing = FS::FileOpen(path, ModeFlagsToFileAccessMode(perms), FS::FileType::BinaryFile);

    if (!backing) {
        return nullptr;
    }

    cache.insert_or_assign(path, std::move(backing));

    // Cannot use make_shared as RealVfsFile constructor is private
    return std::shared_ptr<RealVfsFile>(new RealVfsFile(*this, backing, path, perms));
}

VirtualFile RealVfsFilesystem::CreateFile(std::string_view path_, Mode perms) {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);
    // Current usages of CreateFile expect to delete the contents of an existing file.
    if (FS::IsFile(path)) {
        FS::IOFile temp{path, FS::FileAccessMode::Write, FS::FileType::BinaryFile};

        if (!temp.IsOpen()) {
            return nullptr;
        }

        temp.Close();

        return OpenFile(path, perms);
    }

    if (!FS::NewFile(path)) {
        return nullptr;
    }

    return OpenFile(path, perms);
}

VirtualFile RealVfsFilesystem::CopyFile(std::string_view old_path_, std::string_view new_path_) {
    // Unused
    return nullptr;
}

VirtualFile RealVfsFilesystem::MoveFile(std::string_view old_path_, std::string_view new_path_) {
    const auto old_path = FS::SanitizePath(old_path_, FS::DirectorySeparator::PlatformDefault);
    const auto new_path = FS::SanitizePath(new_path_, FS::DirectorySeparator::PlatformDefault);
    const auto cached_file_iter = cache.find(old_path);

    if (cached_file_iter != cache.cend()) {
        auto file = cached_file_iter->second.lock();

        if (!cached_file_iter->second.expired()) {
            file->Close();
        }

        if (!FS::RenameFile(old_path, new_path)) {
            return nullptr;
        }

        cache.erase(old_path);
        file->Open(new_path, FS::FileAccessMode::Read, FS::FileType::BinaryFile);
        if (file->IsOpen()) {
            cache.insert_or_assign(new_path, std::move(file));
        } else {
            LOG_ERROR(Service_FS, "Failed to open path {} in order to re-cache it", new_path);
        }
    } else {
        UNREACHABLE();
        return nullptr;
    }

    return OpenFile(new_path, Mode::ReadWrite);
}

bool RealVfsFilesystem::DeleteFile(std::string_view path_) {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);
    const auto cached_iter = cache.find(path);

    if (cached_iter != cache.cend()) {
        if (!cached_iter->second.expired()) {
            cached_iter->second.lock()->Close();
        }
        cache.erase(path);
    }

    return FS::RemoveFile(path);
}

VirtualDir RealVfsFilesystem::OpenDirectory(std::string_view path_, Mode perms) {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);
    // Cannot use make_shared as RealVfsDirectory constructor is private
    return std::shared_ptr<RealVfsDirectory>(new RealVfsDirectory(*this, path, perms));
}

VirtualDir RealVfsFilesystem::CreateDirectory(std::string_view path_, Mode perms) {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);
    if (!FS::CreateDirs(path)) {
        return nullptr;
    }
    // Cannot use make_shared as RealVfsDirectory constructor is private
    return std::shared_ptr<RealVfsDirectory>(new RealVfsDirectory(*this, path, perms));
}

VirtualDir RealVfsFilesystem::CopyDirectory(std::string_view old_path_,
                                            std::string_view new_path_) {
    // Unused
    return nullptr;
}

VirtualDir RealVfsFilesystem::MoveDirectory(std::string_view old_path_,
                                            std::string_view new_path_) {
    const auto old_path = FS::SanitizePath(old_path_, FS::DirectorySeparator::PlatformDefault);
    const auto new_path = FS::SanitizePath(new_path_, FS::DirectorySeparator::PlatformDefault);

    if (!FS::RenameDir(old_path, new_path)) {
        return nullptr;
    }

    for (auto& kv : cache) {
        // If the path in the cache doesn't start with old_path, then bail on this file.
        if (kv.first.rfind(old_path, 0) != 0) {
            continue;
        }

        const auto file_old_path =
            FS::SanitizePath(kv.first, FS::DirectorySeparator::PlatformDefault);
        auto file_new_path = FS::SanitizePath(new_path + '/' + kv.first.substr(old_path.size()),
                                              FS::DirectorySeparator::PlatformDefault);
        const auto& cached = cache[file_old_path];

        if (cached.expired()) {
            continue;
        }

        auto file = cached.lock();
        cache.erase(file_old_path);
        file->Open(file_new_path, FS::FileAccessMode::Read, FS::FileType::BinaryFile);
        if (file->IsOpen()) {
            cache.insert_or_assign(std::move(file_new_path), std::move(file));
        } else {
            LOG_ERROR(Service_FS, "Failed to open path {} in order to re-cache it", file_new_path);
        }
    }

    return OpenDirectory(new_path, Mode::ReadWrite);
}

bool RealVfsFilesystem::DeleteDirectory(std::string_view path_) {
    const auto path = FS::SanitizePath(path_, FS::DirectorySeparator::PlatformDefault);

    for (auto& kv : cache) {
        // If the path in the cache doesn't start with path, then bail on this file.
        if (kv.first.rfind(path, 0) != 0) {
            continue;
        }

        const auto& entry = cache[kv.first];
        if (!entry.expired()) {
            entry.lock()->Close();
        }

        cache.erase(kv.first);
    }

    return FS::RemoveDirRecursively(path);
}

RealVfsFile::RealVfsFile(RealVfsFilesystem& base_, std::shared_ptr<FS::IOFile> backing_,
                         const std::string& path_, Mode perms_)
    : base(base_), backing(std::move(backing_)), path(path_), parent_path(FS::GetParentPath(path_)),
      path_components(FS::SplitPathComponents(path_)), perms(perms_) {}

RealVfsFile::~RealVfsFile() = default;

std::string RealVfsFile::GetName() const {
    return path_components.back();
}

std::size_t RealVfsFile::GetSize() const {
    return backing->GetSize();
}

bool RealVfsFile::Resize(std::size_t new_size) {
    return backing->SetSize(new_size);
}

VirtualDir RealVfsFile::GetContainingDirectory() const {
    return base.OpenDirectory(parent_path, perms);
}

bool RealVfsFile::IsWritable() const {
    return True(perms & Mode::Write);
}

bool RealVfsFile::IsReadable() const {
    return True(perms & Mode::Read);
}

std::size_t RealVfsFile::Read(u8* data, std::size_t length, std::size_t offset) const {
    if (!backing->Seek(static_cast<s64>(offset))) {
        return 0;
    }
    return backing->ReadSpan(std::span{data, length});
}

std::size_t RealVfsFile::Write(const u8* data, std::size_t length, std::size_t offset) {
    if (!backing->Seek(static_cast<s64>(offset))) {
        return 0;
    }
    return backing->WriteSpan(std::span{data, length});
}

bool RealVfsFile::Rename(std::string_view name) {
    return base.MoveFile(path, parent_path + '/' + std::string(name)) != nullptr;
}

void RealVfsFile::Close() {
    backing->Close();
}

// TODO(DarkLordZach): MSVC would not let me combine the following two functions using 'if
// constexpr' because there is a compile error in the branch not used.

template <>
std::vector<VirtualFile> RealVfsDirectory::IterateEntries<RealVfsFile, VfsFile>() const {
    if (perms == Mode::Append) {
        return {};
    }

    std::vector<VirtualFile> out;

    const FS::DirEntryCallable callback = [this, &out](const std::filesystem::path& full_path) {
        const auto full_path_string = FS::PathToUTF8String(full_path);

        out.emplace_back(base.OpenFile(full_path_string, perms));

        return true;
    };

    FS::IterateDirEntries(path, callback, FS::DirEntryFilter::File);

    return out;
}

template <>
std::vector<VirtualDir> RealVfsDirectory::IterateEntries<RealVfsDirectory, VfsDirectory>() const {
    if (perms == Mode::Append) {
        return {};
    }

    std::vector<VirtualDir> out;

    const FS::DirEntryCallable callback = [this, &out](const std::filesystem::path& full_path) {
        const auto full_path_string = FS::PathToUTF8String(full_path);

        out.emplace_back(base.OpenDirectory(full_path_string, perms));

        return true;
    };

    FS::IterateDirEntries(path, callback, FS::DirEntryFilter::Directory);

    return out;
}

RealVfsDirectory::RealVfsDirectory(RealVfsFilesystem& base_, const std::string& path_, Mode perms_)
    : base(base_), path(FS::RemoveTrailingSlash(path_)), parent_path(FS::GetParentPath(path)),
      path_components(FS::SplitPathComponents(path)), perms(perms_) {
    if (!FS::Exists(path) && True(perms & Mode::Write)) {
        void(FS::CreateDirs(path));
    }
}

RealVfsDirectory::~RealVfsDirectory() = default;

VirtualFile RealVfsDirectory::GetFileRelative(std::string_view relative_path) const {
    const auto full_path = FS::SanitizePath(path + '/' + std::string(relative_path));
    if (!FS::Exists(full_path) || FS::IsDir(full_path)) {
        return nullptr;
    }
    return base.OpenFile(full_path, perms);
}

VirtualDir RealVfsDirectory::GetDirectoryRelative(std::string_view relative_path) const {
    const auto full_path = FS::SanitizePath(path + '/' + std::string(relative_path));
    if (!FS::Exists(full_path) || !FS::IsDir(full_path)) {
        return nullptr;
    }
    return base.OpenDirectory(full_path, perms);
}

VirtualFile RealVfsDirectory::GetFile(std::string_view name) const {
    return GetFileRelative(name);
}

VirtualDir RealVfsDirectory::GetSubdirectory(std::string_view name) const {
    return GetDirectoryRelative(name);
}

VirtualFile RealVfsDirectory::CreateFileRelative(std::string_view relative_path) {
    const auto full_path = FS::SanitizePath(path + '/' + std::string(relative_path));
    if (!FS::CreateParentDirs(full_path)) {
        return nullptr;
    }
    return base.CreateFile(full_path, perms);
}

VirtualDir RealVfsDirectory::CreateDirectoryRelative(std::string_view relative_path) {
    const auto full_path = FS::SanitizePath(path + '/' + std::string(relative_path));
    return base.CreateDirectory(full_path, perms);
}

bool RealVfsDirectory::DeleteSubdirectoryRecursive(std::string_view name) {
    const auto full_path = FS::SanitizePath(this->path + '/' + std::string(name));
    return base.DeleteDirectory(full_path);
}

std::vector<VirtualFile> RealVfsDirectory::GetFiles() const {
    return IterateEntries<RealVfsFile, VfsFile>();
}

FileTimeStampRaw RealVfsDirectory::GetFileTimeStamp(std::string_view path_) const {
    const auto full_path = FS::SanitizePath(path + '/' + std::string(path_));
    const auto fs_path = std::filesystem::path{FS::ToU8String(full_path)};
    struct stat file_status;

#ifdef _WIN32
    const auto stat_result = _wstat64(fs_path.c_str(), &file_status);
#else
    const auto stat_result = stat(fs_path.c_str(), &file_status);
#endif

    if (stat_result != 0) {
        return {};
    }

    return {
        .created{static_cast<u64>(file_status.st_ctime)},
        .accessed{static_cast<u64>(file_status.st_atime)},
        .modified{static_cast<u64>(file_status.st_mtime)},
    };
}

std::vector<VirtualDir> RealVfsDirectory::GetSubdirectories() const {
    return IterateEntries<RealVfsDirectory, VfsDirectory>();
}

bool RealVfsDirectory::IsWritable() const {
    return True(perms & Mode::Write);
}

bool RealVfsDirectory::IsReadable() const {
    return True(perms & Mode::Read);
}

std::string RealVfsDirectory::GetName() const {
    return path_components.back();
}

VirtualDir RealVfsDirectory::GetParentDirectory() const {
    if (path_components.size() <= 1) {
        return nullptr;
    }

    return base.OpenDirectory(parent_path, perms);
}

VirtualDir RealVfsDirectory::CreateSubdirectory(std::string_view name) {
    const std::string subdir_path = (path + '/').append(name);
    return base.CreateDirectory(subdir_path, perms);
}

VirtualFile RealVfsDirectory::CreateFile(std::string_view name) {
    const std::string file_path = (path + '/').append(name);
    return base.CreateFile(file_path, perms);
}

bool RealVfsDirectory::DeleteSubdirectory(std::string_view name) {
    const std::string subdir_path = (path + '/').append(name);
    return base.DeleteDirectory(subdir_path);
}

bool RealVfsDirectory::DeleteFile(std::string_view name) {
    const std::string file_path = (path + '/').append(name);
    return base.DeleteFile(file_path);
}

bool RealVfsDirectory::Rename(std::string_view name) {
    const std::string new_name = (parent_path + '/').append(name);
    return base.MoveFile(path, new_name) != nullptr;
}

std::string RealVfsDirectory::GetFullPath() const {
    auto out = path;
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}

std::map<std::string, VfsEntryType, std::less<>> RealVfsDirectory::GetEntries() const {
    if (perms == Mode::Append) {
        return {};
    }

    std::map<std::string, VfsEntryType, std::less<>> out;

    const FS::DirEntryCallable callback = [&out](const std::filesystem::path& full_path) {
        const auto filename = FS::PathToUTF8String(full_path.filename());

        out.insert_or_assign(filename,
                             FS::IsDir(full_path) ? VfsEntryType::Directory : VfsEntryType::File);

        return true;
    };

    FS::IterateDirEntries(path, callback);

    return out;
}

} // namespace FileSys
