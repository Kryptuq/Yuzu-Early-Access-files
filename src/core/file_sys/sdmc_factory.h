// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/file_sys/vfs_types.h"
#include "core/hle/result.h"

namespace FileSys {

class RegisteredCache;
class PlaceholderCache;

/// File system interface to the SDCard archive
class SDMCFactory {
public:
    explicit SDMCFactory(VirtualDir sd_dir_, VirtualDir sd_mod_dir_);
    ~SDMCFactory();

    ResultVal<VirtualDir> Open() const;

    VirtualDir GetSDMCModificationLoadRoot(u64 title_id) const;
    VirtualDir GetSDMCContentDirectory() const;

    RegisteredCache* GetSDMCContents() const;
    PlaceholderCache* GetSDMCPlaceholder() const;

    VirtualDir GetImageDirectory() const;

    u64 GetSDMCFreeSpace() const;
    u64 GetSDMCTotalSpace() const;

private:
    VirtualDir sd_dir;
    VirtualDir sd_mod_dir;

    std::unique_ptr<RegisteredCache> contents;
    std::unique_ptr<PlaceholderCache> placeholder;
};

} // namespace FileSys
