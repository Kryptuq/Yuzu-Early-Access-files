// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Common::FS {

enum class FileAccessMode {
    /**
     * If the file at path exists, it opens the file for reading.
     * If the file at path does not exist, it fails to open the file.
     */
    Read = 1 << 0,
    /**
     * If the file at path exists, the existing contents of the file are erased.
     * The empty file is then opened for writing.
     * If the file at path does not exist, it creates and opens a new empty file for writing.
     */
    Write = 1 << 1,
    /**
     * If the file at path exists, it opens the file for reading and writing.
     * If the file at path does not exist, it fails to open the file.
     */
    ReadWrite = Read | Write,
    /**
     * If the file at path exists, it opens the file for appending.
     * If the file at path does not exist, it creates and opens a new empty file for appending.
     */
    Append = 1 << 2,
    /**
     * If the file at path exists, it opens the file for both reading and appending.
     * If the file at path does not exist, it creates and opens a new empty file for both
     * reading and appending.
     */
    ReadAppend = Read | Append,
};

enum class FileType {
    BinaryFile,
    TextFile,
};

enum class FileShareFlag {
    ShareNone,      // Provides exclusive access to the file.
    ShareReadOnly,  // Provides read only shared access to the file.
    ShareWriteOnly, // Provides write only shared access to the file.
    ShareReadWrite, // Provides read and write shared access to the file.
};

enum class DirEntryFilter {
    File = 1 << 0,
    Directory = 1 << 1,
    All = File | Directory,
};
DECLARE_ENUM_FLAG_OPERATORS(DirEntryFilter);

/**
 * A callback function which takes in the path of a directory entry.
 *
 * @param path The path of a directory entry
 *
 * @returns A boolean value.
 *          Return true to indicate whether the callback is successful, false otherwise.
 */
using DirEntryCallable = std::function<bool(const std::filesystem::path& path)>;

} // namespace Common::FS
