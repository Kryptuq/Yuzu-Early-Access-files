/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Adapted by DarkLordZach for use/interaction with yuzu
 *
 * Modifications Copyright 2018 yuzu emulator team
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include "common/common_types.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

struct RomFSBuildDirectoryContext;
struct RomFSBuildFileContext;
struct RomFSDirectoryEntry;
struct RomFSFileEntry;

class RomFSBuildContext {
public:
    explicit RomFSBuildContext(VirtualDir base, VirtualDir ext = nullptr);
    ~RomFSBuildContext();

    // This finalizes the context.
    std::multimap<u64, VirtualFile> Build();

private:
    VirtualDir base;
    VirtualDir ext;
    std::shared_ptr<RomFSBuildDirectoryContext> root;
    std::map<std::string, std::shared_ptr<RomFSBuildDirectoryContext>, std::less<>> directories;
    std::map<std::string, std::shared_ptr<RomFSBuildFileContext>, std::less<>> files;
    u64 num_dirs = 0;
    u64 num_files = 0;
    u64 dir_table_size = 0;
    u64 file_table_size = 0;
    u64 dir_hash_table_size = 0;
    u64 file_hash_table_size = 0;
    u64 file_partition_size = 0;

    void VisitDirectory(VirtualDir filesys, VirtualDir ext_dir,
                        std::shared_ptr<RomFSBuildDirectoryContext> parent);

    bool AddDirectory(std::shared_ptr<RomFSBuildDirectoryContext> parent_dir_ctx,
                      std::shared_ptr<RomFSBuildDirectoryContext> dir_ctx);
    bool AddFile(std::shared_ptr<RomFSBuildDirectoryContext> parent_dir_ctx,
                 std::shared_ptr<RomFSBuildFileContext> file_ctx);
};

} // namespace FileSys
