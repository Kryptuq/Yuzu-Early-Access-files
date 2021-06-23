// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/sdmc_factory.h"
#include "core/file_sys/vfs.h"
#include "core/file_sys/xts_archive.h"

namespace FileSys {

constexpr u64 SDMC_TOTAL_SIZE = 0x10000000000; // 1 TiB

SDMCFactory::SDMCFactory(VirtualDir dir_)
    : dir(std::move(dir_)), contents(std::make_unique<RegisteredCache>(
                                GetOrCreateDirectoryRelative(dir, "/Nintendo/Contents/registered"),
                                [](const VirtualFile& file, const NcaID& id) {
                                    return NAX{file, id}.GetDecrypted();
                                })),
      placeholder(std::make_unique<PlaceholderCache>(
          GetOrCreateDirectoryRelative(dir, "/Nintendo/Contents/placehld"))) {}

SDMCFactory::~SDMCFactory() = default;

ResultVal<VirtualDir> SDMCFactory::Open() const {
    return MakeResult<VirtualDir>(dir);
}

VirtualDir SDMCFactory::GetSDMCModificationLoadRoot(u64 title_id) const {
    // LayeredFS doesn't work on updates and title id-less homebrew
    if (title_id == 0 || (title_id & 0xFFF) == 0x800) {
        return nullptr;
    }
    return GetOrCreateDirectoryRelative(dir, fmt::format("/atmosphere/contents/{:016X}", title_id));
}

VirtualDir SDMCFactory::GetSDMCContentDirectory() const {
    return GetOrCreateDirectoryRelative(dir, "/Nintendo/Contents");
}

RegisteredCache* SDMCFactory::GetSDMCContents() const {
    return contents.get();
}

PlaceholderCache* SDMCFactory::GetSDMCPlaceholder() const {
    return placeholder.get();
}

VirtualDir SDMCFactory::GetImageDirectory() const {
    return GetOrCreateDirectoryRelative(dir, "/Nintendo/Album");
}

u64 SDMCFactory::GetSDMCFreeSpace() const {
    return GetSDMCTotalSpace() - dir->GetSize();
}

u64 SDMCFactory::GetSDMCTotalSpace() const {
    return SDMC_TOTAL_SIZE;
}

} // namespace FileSys
