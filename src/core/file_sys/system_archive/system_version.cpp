// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/system_archive/system_version.h"
#include "core/file_sys/vfs_vector.h"

namespace FileSys::SystemArchive {

namespace SystemVersionData {

// This section should reflect the best system version to describe yuzu's HLE api.
// TODO(DarkLordZach): Update when HLE gets better.

constexpr u8 VERSION_MAJOR = 11;
constexpr u8 VERSION_MINOR = 0;
constexpr u8 VERSION_MICRO = 1;

constexpr u8 REVISION_MAJOR = 1;
constexpr u8 REVISION_MINOR = 0;

constexpr char PLATFORM_STRING[] = "NX";
constexpr char VERSION_HASH[] = "69103fcb2004dace877094c2f8c29e6113be5dbf";
constexpr char DISPLAY_VERSION[] = "11.0.1";
constexpr char DISPLAY_TITLE[] = "NintendoSDK Firmware for NX 11.0.1-1.0";

} // namespace SystemVersionData

std::string GetLongDisplayVersion() {
    return SystemVersionData::DISPLAY_TITLE;
}

VirtualDir SystemVersion() {
    VirtualFile file = std::make_shared<VectorVfsFile>(std::vector<u8>(0x100), "file");
    file->WriteObject(SystemVersionData::VERSION_MAJOR, 0);
    file->WriteObject(SystemVersionData::VERSION_MINOR, 1);
    file->WriteObject(SystemVersionData::VERSION_MICRO, 2);
    file->WriteObject(SystemVersionData::REVISION_MAJOR, 4);
    file->WriteObject(SystemVersionData::REVISION_MINOR, 5);
    file->WriteArray(SystemVersionData::PLATFORM_STRING,
                     std::min<u64>(sizeof(SystemVersionData::PLATFORM_STRING), 0x20ULL), 0x8);
    file->WriteArray(SystemVersionData::VERSION_HASH,
                     std::min<u64>(sizeof(SystemVersionData::VERSION_HASH), 0x40ULL), 0x28);
    file->WriteArray(SystemVersionData::DISPLAY_VERSION,
                     std::min<u64>(sizeof(SystemVersionData::DISPLAY_VERSION), 0x18ULL), 0x68);
    file->WriteArray(SystemVersionData::DISPLAY_TITLE,
                     std::min<u64>(sizeof(SystemVersionData::DISPLAY_TITLE), 0x80ULL), 0x80);
    return std::make_shared<VectorVfsDirectory>(std::vector<VirtualFile>{file},
                                                std::vector<VirtualDir>{}, "data");
}

} // namespace FileSys::SystemArchive
