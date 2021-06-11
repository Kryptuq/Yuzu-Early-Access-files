// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::Capture {

enum class AlbumImageOrientation {
    Orientation0 = 0,
    Orientation1 = 1,
    Orientation2 = 2,
    Orientation3 = 3,
};

enum class AlbumReportOption {
    Disable = 0,
    Enable = 1,
};

enum class ContentType : u8 {
    Screenshot = 0,
    Movie = 1,
    ExtraMovie = 3,
};

enum class AlbumStorage : u8 {
    NAND = 0,
    SD = 1,
};

struct AlbumFileDateTime {
    s16 year{};
    s8 month{};
    s8 day{};
    s8 hour{};
    s8 minute{};
    s8 second{};
    s8 uid{};
};
static_assert(sizeof(AlbumFileDateTime) == 0x8, "AlbumFileDateTime has incorrect size.");

struct AlbumEntry {
    u64 size{};
    u64 application_id{};
    AlbumFileDateTime datetime{};
    AlbumStorage storage{};
    ContentType content{};
    INSERT_PADDING_BYTES(6);
};
static_assert(sizeof(AlbumEntry) == 0x20, "AlbumEntry has incorrect size.");

struct AlbumFileEntry {
    u64 size{}; // Size of the entry
    u64 hash{}; // AES256 with hardcoded key over AlbumEntry
    AlbumFileDateTime datetime{};
    AlbumStorage storage{};
    ContentType content{};
    INSERT_PADDING_BYTES(5);
    u8 unknown{1}; // Set to 1 on official SW
};
static_assert(sizeof(AlbumFileEntry) == 0x20, "AlbumFileEntry has incorrect size.");

struct ApplicationAlbumEntry {
    u64 size{}; // Size of the entry
    u64 hash{}; // AES256 with hardcoded key over AlbumEntry
    AlbumFileDateTime datetime{};
    AlbumStorage storage{};
    ContentType content{};
    INSERT_PADDING_BYTES(5);
    u8 unknown{1}; // Set to 1 on official SW
};
static_assert(sizeof(ApplicationAlbumEntry) == 0x20, "ApplicationAlbumEntry has incorrect size.");

struct ApplicationAlbumFileEntry {
    ApplicationAlbumEntry entry{};
    AlbumFileDateTime datetime{};
    u64 unknown{};
};
static_assert(sizeof(ApplicationAlbumFileEntry) == 0x30,
              "ApplicationAlbumFileEntry has incorrect size.");

/// Registers all Capture services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& sm, Core::System& system);

} // namespace Service::Capture
