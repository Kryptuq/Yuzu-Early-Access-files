// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/system_archive/system_version.h"
#include "core/file_sys/vfs_vector.h"
#include "core/hle/api_version.h"

namespace FileSys::SystemArchive {

std::string GetLongDisplayVersion() {
    return HLE::ApiVersion::DISPLAY_TITLE;
}

VirtualDir SystemVersion() {
    VirtualFile file = std::make_shared<VectorVfsFile>(std::vector<u8>(0x100), "file");
    file->WriteObject(HLE::ApiVersion::HOS_VERSION_MAJOR, 0);
    file->WriteObject(HLE::ApiVersion::HOS_VERSION_MINOR, 1);
    file->WriteObject(HLE::ApiVersion::HOS_VERSION_MICRO, 2);
    file->WriteObject(HLE::ApiVersion::SDK_REVISION_MAJOR, 4);
    file->WriteObject(HLE::ApiVersion::SDK_REVISION_MINOR, 5);
    file->WriteArray(HLE::ApiVersion::PLATFORM_STRING,
                     std::min<u64>(sizeof(HLE::ApiVersion::PLATFORM_STRING), 0x20ULL), 0x8);
    file->WriteArray(HLE::ApiVersion::VERSION_HASH,
                     std::min<u64>(sizeof(HLE::ApiVersion::VERSION_HASH), 0x40ULL), 0x28);
    file->WriteArray(HLE::ApiVersion::DISPLAY_VERSION,
                     std::min<u64>(sizeof(HLE::ApiVersion::DISPLAY_VERSION), 0x18ULL), 0x68);
    file->WriteArray(HLE::ApiVersion::DISPLAY_TITLE,
                     std::min<u64>(sizeof(HLE::ApiVersion::DISPLAY_TITLE), 0x80ULL), 0x80);
    return std::make_shared<VectorVfsDirectory>(std::vector<VirtualFile>{file},
                                                std::vector<VirtualDir>{}, "data");
}

} // namespace FileSys::SystemArchive
