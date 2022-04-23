// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"

namespace FileSys {

enum class BisPartitionId : u32 {
    UserDataRoot = 20,
    CalibrationBinary = 27,
    CalibrationFile = 28,
    BootConfigAndPackage2Part1 = 21,
    BootConfigAndPackage2Part2 = 22,
    BootConfigAndPackage2Part3 = 23,
    BootConfigAndPackage2Part4 = 24,
    BootConfigAndPackage2Part5 = 25,
    BootConfigAndPackage2Part6 = 26,
    SafeMode = 29,
    System = 31,
    SystemProperEncryption = 32,
    SystemProperPartition = 33,
    User = 30,
};

class RegisteredCache;
class PlaceholderCache;

/// File system interface to the Built-In Storage
/// This is currently missing accessors to BIS partitions, but seemed like a good place for the NAND
/// registered caches.
class BISFactory {
public:
    explicit BISFactory(VirtualDir nand_root, VirtualDir load_root, VirtualDir dump_root);
    ~BISFactory();

    VirtualDir GetSystemNANDContentDirectory() const;
    VirtualDir GetUserNANDContentDirectory() const;

    RegisteredCache* GetSystemNANDContents() const;
    RegisteredCache* GetUserNANDContents() const;

    PlaceholderCache* GetSystemNANDPlaceholder() const;
    PlaceholderCache* GetUserNANDPlaceholder() const;

    VirtualDir GetModificationLoadRoot(u64 title_id) const;
    VirtualDir GetModificationDumpRoot(u64 title_id) const;

    VirtualDir OpenPartition(BisPartitionId id) const;
    VirtualFile OpenPartitionStorage(BisPartitionId id, VirtualFilesystem file_system) const;

    VirtualDir GetImageDirectory() const;

    u64 GetSystemNANDFreeSpace() const;
    u64 GetSystemNANDTotalSpace() const;
    u64 GetUserNANDFreeSpace() const;
    u64 GetUserNANDTotalSpace() const;
    u64 GetFullNANDTotalSpace() const;

    VirtualDir GetBCATDirectory(u64 title_id) const;

private:
    VirtualDir nand_root;
    VirtualDir load_root;
    VirtualDir dump_root;

    std::unique_ptr<RegisteredCache> sysnand_cache;
    std::unique_ptr<RegisteredCache> usrnand_cache;

    std::unique_ptr<PlaceholderCache> sysnand_placeholder;
    std::unique_ptr<PlaceholderCache> usrnand_placeholder;
};

} // namespace FileSys
