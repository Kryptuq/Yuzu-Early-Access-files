// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/file_sys/vfs.h"

namespace FileSys {

enum class RomFSExtractionType {
    Full,          // Includes data directory
    Truncated,     // Traverses into data directory
    SingleDiscard, // Traverses into the first subdirectory of root
};

// Converts a RomFS binary blob to VFS Filesystem
// Returns nullptr on failure
VirtualDir ExtractRomFS(VirtualFile file,
                        RomFSExtractionType type = RomFSExtractionType::Truncated);

// Converts a VFS filesystem into a RomFS binary
// Returns nullptr on failure
VirtualFile CreateRomFS(VirtualDir dir, VirtualDir ext = nullptr);

} // namespace FileSys
