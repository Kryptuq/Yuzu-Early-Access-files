// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"

namespace FileSys {

enum class Mode : u32;

// An enumeration representing what can be at the end of a path in a VfsFilesystem
enum class VfsEntryType {
    None,
    File,
    Directory,
};

// A class representing an abstract filesystem. A default implementation given the root VirtualDir
// is provided for convenience, but if the Vfs implementation has any additional state or
// functionality, they will need to override.
class VfsFilesystem {
public:
    YUZU_NON_COPYABLE(VfsFilesystem);
    YUZU_NON_MOVEABLE(VfsFilesystem);

    explicit VfsFilesystem(VirtualDir root);
    virtual ~VfsFilesystem();

    // Gets the friendly name for the filesystem.
    virtual std::string GetName() const;

    // Return whether or not the user has read permissions on this filesystem.
    virtual bool IsReadable() const;
    // Return whether or not the user has write permission on this filesystem.
    virtual bool IsWritable() const;

    // Determine if the entry at path is non-existent, a file, or a directory.
    virtual VfsEntryType GetEntryType(std::string_view path) const;

    // Opens the file with path relative to root. If it doesn't exist, returns nullptr.
    virtual VirtualFile OpenFile(std::string_view path, Mode perms);
    // Creates a new, empty file at path
    virtual VirtualFile CreateFile(std::string_view path, Mode perms);
    // Copies the file from old_path to new_path, returning the new file on success and nullptr on
    // failure.
    virtual VirtualFile CopyFile(std::string_view old_path, std::string_view new_path);
    // Moves the file from old_path to new_path, returning the moved file on success and nullptr on
    // failure.
    virtual VirtualFile MoveFile(std::string_view old_path, std::string_view new_path);
    // Deletes the file with path relative to root, returning true on success.
    virtual bool DeleteFile(std::string_view path);

    // Opens the directory with path relative to root. If it doesn't exist, returns nullptr.
    virtual VirtualDir OpenDirectory(std::string_view path, Mode perms);
    // Creates a new, empty directory at path
    virtual VirtualDir CreateDirectory(std::string_view path, Mode perms);
    // Copies the directory from old_path to new_path, returning the new directory on success and
    // nullptr on failure.
    virtual VirtualDir CopyDirectory(std::string_view old_path, std::string_view new_path);
    // Moves the directory from old_path to new_path, returning the moved directory on success and
    // nullptr on failure.
    virtual VirtualDir MoveDirectory(std::string_view old_path, std::string_view new_path);
    // Deletes the directory with path relative to root, returning true on success.
    virtual bool DeleteDirectory(std::string_view path);

protected:
    // Root directory in default implementation.
    VirtualDir root;
};

// A class representing a file in an abstract filesystem.
class VfsFile {
public:
    YUZU_NON_COPYABLE(VfsFile);
    YUZU_NON_MOVEABLE(VfsFile);

    VfsFile() = default;
    virtual ~VfsFile();

    // Retrieves the file name.
    virtual std::string GetName() const = 0;
    // Retrieves the extension of the file name.
    virtual std::string GetExtension() const;
    // Retrieves the size of the file.
    virtual std::size_t GetSize() const = 0;
    // Resizes the file to new_size. Returns whether or not the operation was successful.
    virtual bool Resize(std::size_t new_size) = 0;
    // Gets a pointer to the directory containing this file, returning nullptr if there is none.
    virtual VirtualDir GetContainingDirectory() const = 0;

    // Returns whether or not the file can be written to.
    virtual bool IsWritable() const = 0;
    // Returns whether or not the file can be read from.
    virtual bool IsReadable() const = 0;

    // The primary method of reading from the file. Reads length bytes into data starting at offset
    // into file. Returns number of bytes successfully read.
    virtual std::size_t Read(u8* data, std::size_t length, std::size_t offset = 0) const = 0;
    // The primary method of writing to the file. Writes length bytes from data starting at offset
    // into file. Returns number of bytes successfully written.
    virtual std::size_t Write(const u8* data, std::size_t length, std::size_t offset = 0) = 0;

    // Reads exactly one byte at the offset provided, returning std::nullopt on error.
    virtual std::optional<u8> ReadByte(std::size_t offset = 0) const;
    // Reads size bytes starting at offset in file into a vector.
    virtual std::vector<u8> ReadBytes(std::size_t size, std::size_t offset = 0) const;
    // Reads all the bytes from the file into a vector. Equivalent to 'file->Read(file->GetSize(),
    // 0)'
    virtual std::vector<u8> ReadAllBytes() const;

    // Reads an array of type T, size number_elements starting at offset.
    // Returns the number of bytes (sizeof(T)*number_elements) read successfully.
    template <typename T>
    std::size_t ReadArray(T* data, std::size_t number_elements, std::size_t offset = 0) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        return Read(reinterpret_cast<u8*>(data), number_elements * sizeof(T), offset);
    }

    // Reads size bytes into the memory starting at data starting at offset into the file.
    // Returns the number of bytes read successfully.
    template <typename T>
    std::size_t ReadBytes(T* data, std::size_t size, std::size_t offset = 0) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return Read(reinterpret_cast<u8*>(data), size, offset);
    }

    // Reads one object of type T starting at offset in file.
    // Returns the number of bytes read successfully (sizeof(T)).
    template <typename T>
    std::size_t ReadObject(T* data, std::size_t offset = 0) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return Read(reinterpret_cast<u8*>(data), sizeof(T), offset);
    }

    // Writes exactly one byte to offset in file and returns whether or not the byte was written
    // successfully.
    virtual bool WriteByte(u8 data, std::size_t offset = 0);
    // Writes a vector of bytes to offset in file and returns the number of bytes successfully
    // written.
    virtual std::size_t WriteBytes(const std::vector<u8>& data, std::size_t offset = 0);

    // Writes an array of type T, size number_elements to offset in file.
    // Returns the number of bytes (sizeof(T)*number_elements) written successfully.
    template <typename T>
    std::size_t WriteArray(const T* data, std::size_t number_elements, std::size_t offset = 0) {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return Write(reinterpret_cast<const u8*>(data), number_elements * sizeof(T), offset);
    }

    // Writes size bytes starting at memory location data to offset in file.
    // Returns the number of bytes written successfully.
    template <typename T>
    std::size_t WriteBytes(const T* data, std::size_t size, std::size_t offset = 0) {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return Write(reinterpret_cast<const u8*>(data), size, offset);
    }

    // Writes one object of type T to offset in file.
    // Returns the number of bytes written successfully (sizeof(T)).
    template <typename T>
    std::size_t WriteObject(const T& data, std::size_t offset = 0) {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return Write(reinterpret_cast<const u8*>(&data), sizeof(T), offset);
    }

    // Renames the file to name. Returns whether or not the operation was successsful.
    virtual bool Rename(std::string_view name) = 0;

    // Returns the full path of this file as a string, recursively
    virtual std::string GetFullPath() const;
};

// A class representing a directory in an abstract filesystem.
class VfsDirectory {
public:
    YUZU_NON_COPYABLE(VfsDirectory);
    YUZU_NON_MOVEABLE(VfsDirectory);

    VfsDirectory() = default;
    virtual ~VfsDirectory();

    // Retrieves the file located at path as if the current directory was root. Returns nullptr if
    // not found.
    virtual VirtualFile GetFileRelative(std::string_view path) const;
    // Calls GetFileRelative(path) on the root of the current directory.
    virtual VirtualFile GetFileAbsolute(std::string_view path) const;

    // Retrieves the directory located at path as if the current directory was root. Returns nullptr
    // if not found.
    virtual VirtualDir GetDirectoryRelative(std::string_view path) const;
    // Calls GetDirectoryRelative(path) on the root of the current directory.
    virtual VirtualDir GetDirectoryAbsolute(std::string_view path) const;

    // Returns a vector containing all of the files in this directory.
    virtual std::vector<VirtualFile> GetFiles() const = 0;
    // Returns the file with filename matching name. Returns nullptr if directory doesn't have a
    // file with name.
    virtual VirtualFile GetFile(std::string_view name) const;

    // Returns a struct containing the file's timestamp.
    virtual FileTimeStampRaw GetFileTimeStamp(std::string_view path) const;

    // Returns a vector containing all of the subdirectories in this directory.
    virtual std::vector<VirtualDir> GetSubdirectories() const = 0;
    // Returns the directory with name matching name. Returns nullptr if directory doesn't have a
    // directory with name.
    virtual VirtualDir GetSubdirectory(std::string_view name) const;

    // Returns whether or not the directory can be written to.
    virtual bool IsWritable() const = 0;
    // Returns whether of not the directory can be read from.
    virtual bool IsReadable() const = 0;

    // Returns whether or not the directory is the root of the current file tree.
    virtual bool IsRoot() const;

    // Returns the name of the directory.
    virtual std::string GetName() const = 0;
    // Returns the total size of all files and subdirectories in this directory.
    virtual std::size_t GetSize() const;
    // Returns the parent directory of this directory. Returns nullptr if this directory is root or
    // has no parent.
    virtual VirtualDir GetParentDirectory() const = 0;

    // Creates a new subdirectory with name name. Returns a pointer to the new directory or nullptr
    // if the operation failed.
    virtual VirtualDir CreateSubdirectory(std::string_view name) = 0;
    // Creates a new file with name name. Returns a pointer to the new file or nullptr if the
    // operation failed.
    virtual VirtualFile CreateFile(std::string_view name) = 0;

    // Creates a new file at the path relative to this directory. Also creates directories if
    // they do not exist and is supported by this implementation. Returns nullptr on any failure.
    virtual VirtualFile CreateFileRelative(std::string_view path);

    // Creates a new file at the path relative to root of this directory. Also creates directories
    // if they do not exist and is supported by this implementation. Returns nullptr on any failure.
    virtual VirtualFile CreateFileAbsolute(std::string_view path);

    // Creates a new directory at the path relative to this directory. Also creates directories if
    // they do not exist and is supported by this implementation. Returns nullptr on any failure.
    virtual VirtualDir CreateDirectoryRelative(std::string_view path);

    // Creates a new directory at the path relative to root of this directory. Also creates
    // directories if they do not exist and is supported by this implementation. Returns nullptr on
    // any failure.
    virtual VirtualDir CreateDirectoryAbsolute(std::string_view path);

    // Deletes the subdirectory with the given name and returns true on success.
    virtual bool DeleteSubdirectory(std::string_view name) = 0;

    // Deletes all subdirectories and files within the provided directory and then deletes
    // the directory itself. Returns true on success.
    virtual bool DeleteSubdirectoryRecursive(std::string_view name);

    // Deletes all subdirectories and files within the provided directory.
    // Unlike DeleteSubdirectoryRecursive, this does not delete the provided directory.
    virtual bool CleanSubdirectoryRecursive(std::string_view name);

    // Returns whether or not the file with name name was deleted successfully.
    virtual bool DeleteFile(std::string_view name) = 0;

    // Returns whether or not this directory was renamed to name.
    virtual bool Rename(std::string_view name) = 0;

    // Returns whether or not the file with name src was successfully copied to a new file with name
    // dest.
    virtual bool Copy(std::string_view src, std::string_view dest);

    // Gets all of the entries directly in the directory (files and dirs), returning a map between
    // item name -> type.
    virtual std::map<std::string, VfsEntryType, std::less<>> GetEntries() const;

    // Returns the full path of this directory as a string, recursively
    virtual std::string GetFullPath() const;
};

// A convenience partial-implementation of VfsDirectory that stubs out methods that should only work
// if writable. This is to avoid redundant empty methods everywhere.
class ReadOnlyVfsDirectory : public VfsDirectory {
public:
    bool IsWritable() const override;
    bool IsReadable() const override;
    VirtualDir CreateSubdirectory(std::string_view name) override;
    VirtualFile CreateFile(std::string_view name) override;
    VirtualFile CreateFileAbsolute(std::string_view path) override;
    VirtualFile CreateFileRelative(std::string_view path) override;
    VirtualDir CreateDirectoryAbsolute(std::string_view path) override;
    VirtualDir CreateDirectoryRelative(std::string_view path) override;
    bool DeleteSubdirectory(std::string_view name) override;
    bool DeleteSubdirectoryRecursive(std::string_view name) override;
    bool CleanSubdirectoryRecursive(std::string_view name) override;
    bool DeleteFile(std::string_view name) override;
    bool Rename(std::string_view name) override;
};

// Compare the two files, byte-for-byte, in increments specified by block_size
bool DeepEquals(const VirtualFile& file1, const VirtualFile& file2,
                std::size_t block_size = 0x1000);

// A method that copies the raw data between two different implementations of VirtualFile. If you
// are using the same implementation, it is probably better to use the Copy method in the parent
// directory of src/dest.
bool VfsRawCopy(const VirtualFile& src, const VirtualFile& dest, std::size_t block_size = 0x1000);

// A method that performs a similar function to VfsRawCopy above, but instead copies entire
// directories. It suffers the same performance penalties as above and an implementation-specific
// Copy should always be preferred.
bool VfsRawCopyD(const VirtualDir& src, const VirtualDir& dest, std::size_t block_size = 0x1000);

// Checks if the directory at path relative to rel exists. If it does, returns that. If it does not
// it attempts to create it and returns the new dir or nullptr on failure.
VirtualDir GetOrCreateDirectoryRelative(const VirtualDir& rel, std::string_view path);

} // namespace FileSys
