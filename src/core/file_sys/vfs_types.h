// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

namespace FileSys {

class VfsDirectory;
class VfsFile;
class VfsFilesystem;

// Declarations for Vfs* pointer types

using VirtualDir = std::shared_ptr<VfsDirectory>;
using VirtualFile = std::shared_ptr<VfsFile>;
using VirtualFilesystem = std::shared_ptr<VfsFilesystem>;

} // namespace FileSys
