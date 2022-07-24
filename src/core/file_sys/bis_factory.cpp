// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include "common/fs/path_util.h"
#include "core/file_sys/bis_factory.h"
#include "core/file_sys/mode.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

constexpr u64 NAND_USER_SIZE = 0x680000000;  // 26624 MiB
constexpr u64 NAND_SYSTEM_SIZE = 0xA0000000; // 2560 MiB
constexpr u64 NAND_TOTAL_SIZE = 0x747C00000; // 29820 MiB

BISFactory::BISFactory(VirtualDir nand_root_, VirtualDir load_root_, VirtualDir dump_root_)
    : nand_root(std::move(nand_root_)), load_root(std::move(load_root_)),
      dump_root(std::move(dump_root_)),
      sysnand_cache(std::make_unique<RegisteredCache>(
          GetOrCreateDirectoryRelative(nand_root, "/system/Contents/registered"))),
      usrnand_cache(std::make_unique<RegisteredCache>(
          GetOrCreateDirectoryRelative(nand_root, "/user/Contents/registered"))),
      sysnand_placeholder(std::make_unique<PlaceholderCache>(
          GetOrCreateDirectoryRelative(nand_root, "/system/Contents/placehld"))),
      usrnand_placeholder(std::make_unique<PlaceholderCache>(
          GetOrCreateDirectoryRelative(nand_root, "/user/Contents/placehld"))) {}

BISFactory::~BISFactory() = default;

VirtualDir BISFactory::GetSystemNANDContentDirectory() const {
    return GetOrCreateDirectoryRelative(nand_root, "/system/Contents");
}

VirtualDir BISFactory::GetUserNANDContentDirectory() const {
    return GetOrCreateDirectoryRelative(nand_root, "/user/Contents");
}

RegisteredCache* BISFactory::GetSystemNANDContents() const {
    return sysnand_cache.get();
}

RegisteredCache* BISFactory::GetUserNANDContents() const {
    return usrnand_cache.get();
}

PlaceholderCache* BISFactory::GetSystemNANDPlaceholder() const {
    return sysnand_placeholder.get();
}

PlaceholderCache* BISFactory::GetUserNANDPlaceholder() const {
    return usrnand_placeholder.get();
}

VirtualDir BISFactory::GetModificationLoadRoot(u64 title_id) const {
    // LayeredFS doesn't work on updates and title id-less homebrew
    if (title_id == 0 || (title_id & 0xFFF) == 0x800)
        return nullptr;
    return GetOrCreateDirectoryRelative(load_root, fmt::format("/{:016X}", title_id));
}

VirtualDir BISFactory::GetModificationDumpRoot(u64 title_id) const {
    if (title_id == 0)
        return nullptr;
    return GetOrCreateDirectoryRelative(dump_root, fmt::format("/{:016X}", title_id));
}

VirtualDir BISFactory::OpenPartition(BisPartitionId id) const {
    switch (id) {
    case BisPartitionId::CalibrationFile:
        return GetOrCreateDirectoryRelative(nand_root, "/prodinfof");
    case BisPartitionId::SafeMode:
        return GetOrCreateDirectoryRelative(nand_root, "/safe");
    case BisPartitionId::System:
        return GetOrCreateDirectoryRelative(nand_root, "/system");
    case BisPartitionId::User:
        return GetOrCreateDirectoryRelative(nand_root, "/user");
    default:
        return nullptr;
    }
}

VirtualFile BISFactory::OpenPartitionStorage(BisPartitionId id,
                                             VirtualFilesystem file_system) const {
    auto& keys = Core::Crypto::KeyManager::Instance();
    Core::Crypto::PartitionDataManager pdm{file_system->OpenDirectory(
        Common::FS::GetYuzuPathString(Common::FS::YuzuPath::NANDDir), Mode::Read)};
    keys.PopulateFromPartitionData(pdm);

    switch (id) {
    case BisPartitionId::CalibrationBinary:
        return pdm.GetDecryptedProdInfo();
    case BisPartitionId::BootConfigAndPackage2Part1:
    case BisPartitionId::BootConfigAndPackage2Part2:
    case BisPartitionId::BootConfigAndPackage2Part3:
    case BisPartitionId::BootConfigAndPackage2Part4:
    case BisPartitionId::BootConfigAndPackage2Part5:
    case BisPartitionId::BootConfigAndPackage2Part6: {
        const auto new_id = static_cast<u8>(id) -
                            static_cast<u8>(BisPartitionId::BootConfigAndPackage2Part1) +
                            static_cast<u8>(Core::Crypto::Package2Type::NormalMain);
        return pdm.GetPackage2Raw(static_cast<Core::Crypto::Package2Type>(new_id));
    }
    default:
        return nullptr;
    }
}

VirtualDir BISFactory::GetImageDirectory() const {
    return GetOrCreateDirectoryRelative(nand_root, "/user/Album");
}

u64 BISFactory::GetSystemNANDFreeSpace() const {
    const auto sys_dir = GetOrCreateDirectoryRelative(nand_root, "/system");
    if (sys_dir == nullptr) {
        return GetSystemNANDTotalSpace();
    }

    return GetSystemNANDTotalSpace() - sys_dir->GetSize();
}

u64 BISFactory::GetSystemNANDTotalSpace() const {
    return NAND_SYSTEM_SIZE;
}

u64 BISFactory::GetUserNANDFreeSpace() const {
    // For some reason games such as BioShock 1 checks whether this is exactly 0x680000000 bytes.
    // Set the free space to be 1 MiB less than the total as a workaround to this issue.
    return GetUserNANDTotalSpace() - 0x100000;
}

u64 BISFactory::GetUserNANDTotalSpace() const {
    return NAND_USER_SIZE;
}

u64 BISFactory::GetFullNANDTotalSpace() const {
    return NAND_TOTAL_SIZE;
}

VirtualDir BISFactory::GetBCATDirectory(u64 title_id) const {
    return GetOrCreateDirectoryRelative(nand_root,
                                        fmt::format("/system/save/bcat/{:016X}", title_id));
}

} // namespace FileSys
