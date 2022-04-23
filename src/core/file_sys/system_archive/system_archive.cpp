// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/system_archive/mii_model.h"
#include "core/file_sys/system_archive/ng_word.h"
#include "core/file_sys/system_archive/shared_font.h"
#include "core/file_sys/system_archive/system_archive.h"
#include "core/file_sys/system_archive/system_version.h"
#include "core/file_sys/system_archive/time_zone_binary.h"

namespace FileSys::SystemArchive {

constexpr u64 SYSTEM_ARCHIVE_BASE_TITLE_ID = 0x0100000000000800;
constexpr std::size_t SYSTEM_ARCHIVE_COUNT = 0x28;

using SystemArchiveSupplier = VirtualDir (*)();

struct SystemArchiveDescriptor {
    u64 title_id;
    const char* name;
    SystemArchiveSupplier supplier;
};

constexpr std::array<SystemArchiveDescriptor, SYSTEM_ARCHIVE_COUNT> SYSTEM_ARCHIVES{{
    {0x0100000000000800, "CertStore", nullptr},
    {0x0100000000000801, "ErrorMessage", nullptr},
    {0x0100000000000802, "MiiModel", &MiiModel},
    {0x0100000000000803, "BrowserDll", nullptr},
    {0x0100000000000804, "Help", nullptr},
    {0x0100000000000805, "SharedFont", nullptr},
    {0x0100000000000806, "NgWord", &NgWord1},
    {0x0100000000000807, "SsidList", nullptr},
    {0x0100000000000808, "Dictionary", nullptr},
    {0x0100000000000809, "SystemVersion", &SystemVersion},
    {0x010000000000080A, "AvatarImage", nullptr},
    {0x010000000000080B, "LocalNews", nullptr},
    {0x010000000000080C, "Eula", nullptr},
    {0x010000000000080D, "UrlBlackList", nullptr},
    {0x010000000000080E, "TimeZoneBinary", &TimeZoneBinary},
    {0x010000000000080F, "CertStoreCruiser", nullptr},
    {0x0100000000000810, "FontNintendoExtension", &FontNintendoExtension},
    {0x0100000000000811, "FontStandard", &FontStandard},
    {0x0100000000000812, "FontKorean", &FontKorean},
    {0x0100000000000813, "FontChineseTraditional", &FontChineseTraditional},
    {0x0100000000000814, "FontChineseSimple", &FontChineseSimple},
    {0x0100000000000815, "FontBfcpx", nullptr},
    {0x0100000000000816, "SystemUpdate", nullptr},
    {0x0100000000000817, "0100000000000817", nullptr},
    {0x0100000000000818, "FirmwareDebugSettings", nullptr},
    {0x0100000000000819, "BootImagePackage", nullptr},
    {0x010000000000081A, "BootImagePackageSafe", nullptr},
    {0x010000000000081B, "BootImagePackageExFat", nullptr},
    {0x010000000000081C, "BootImagePackageExFatSafe", nullptr},
    {0x010000000000081D, "FatalMessage", nullptr},
    {0x010000000000081E, "ControllerIcon", nullptr},
    {0x010000000000081F, "PlatformConfigIcosa", nullptr},
    {0x0100000000000820, "PlatformConfigCopper", nullptr},
    {0x0100000000000821, "PlatformConfigHoag", nullptr},
    {0x0100000000000822, "ControllerFirmware", nullptr},
    {0x0100000000000823, "NgWord2", &NgWord2},
    {0x0100000000000824, "PlatformConfigIcosaMariko", nullptr},
    {0x0100000000000825, "ApplicationBlackList", nullptr},
    {0x0100000000000826, "RebootlessSystemUpdateVersion", nullptr},
    {0x0100000000000827, "ContentActionTable", nullptr},
}};

VirtualFile SynthesizeSystemArchive(const u64 title_id) {
    if (title_id < SYSTEM_ARCHIVES.front().title_id || title_id > SYSTEM_ARCHIVES.back().title_id)
        return nullptr;

    const auto& desc = SYSTEM_ARCHIVES[title_id - SYSTEM_ARCHIVE_BASE_TITLE_ID];

    LOG_INFO(Service_FS, "Synthesizing system archive '{}' (0x{:016X}).", desc.name, desc.title_id);

    if (desc.supplier == nullptr)
        return nullptr;

    const auto dir = desc.supplier();

    if (dir == nullptr)
        return nullptr;

    const auto romfs = CreateRomFS(dir);

    if (romfs == nullptr)
        return nullptr;

    LOG_INFO(Service_FS, "    - System archive generation successful!");
    return romfs;
}
} // namespace FileSys::SystemArchive
