// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
