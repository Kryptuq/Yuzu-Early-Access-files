// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "common/common_types.h"

namespace FileSys {

class VfsDirectory;
class VfsFile;
class VfsFilesystem;

// Declarations for Vfs* pointer types

using VirtualDir = std::shared_ptr<VfsDirectory>;
using VirtualFile = std::shared_ptr<VfsFile>;
using VirtualFilesystem = std::shared_ptr<VfsFilesystem>;

struct FileTimeStampRaw {
    u64 created{};
    u64 accessed{};
    u64 modified{};
    u64 padding{};
};

} // namespace FileSys
