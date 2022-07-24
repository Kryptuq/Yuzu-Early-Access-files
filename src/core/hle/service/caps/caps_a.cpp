// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/caps/caps_a.h"

namespace Service::Capture {

class IAlbumAccessorSession final : public ServiceFramework<IAlbumAccessorSession> {
public:
    explicit IAlbumAccessorSession(Core::System& system_)
        : ServiceFramework{system_, "IAlbumAccessorSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {2001, nullptr, "OpenAlbumMovieReadStream"},
            {2002, nullptr, "CloseAlbumMovieReadStream"},
            {2003, nullptr, "GetAlbumMovieReadStreamMovieDataSize"},
            {2004, nullptr, "ReadMovieDataFromAlbumMovieReadStream"},
            {2005, nullptr, "GetAlbumMovieReadStreamBrokenReason"},
            {2006, nullptr, "GetAlbumMovieReadStreamImageDataSize"},
            {2007, nullptr, "ReadImageDataFromAlbumMovieReadStream"},
            {2008, nullptr, "ReadFileAttributeFromAlbumMovieReadStream"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

CAPS_A::CAPS_A(Core::System& system_) : ServiceFramework{system_, "caps:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetAlbumFileCount"},
        {1, nullptr, "GetAlbumFileList"},
        {2, nullptr, "LoadAlbumFile"},
        {3, nullptr, "DeleteAlbumFile"},
        {4, nullptr, "StorageCopyAlbumFile"},
        {5, nullptr, "IsAlbumMounted"},
        {6, nullptr, "GetAlbumUsage"},
        {7, nullptr, "GetAlbumFileSize"},
        {8, nullptr, "LoadAlbumFileThumbnail"},
        {9, nullptr, "LoadAlbumScreenShotImage"},
        {10, nullptr, "LoadAlbumScreenShotThumbnailImage"},
        {11, nullptr, "GetAlbumEntryFromApplicationAlbumEntry"},
        {12, nullptr, "LoadAlbumScreenShotImageEx"},
        {13, nullptr, "LoadAlbumScreenShotThumbnailImageEx"},
        {14, nullptr, "LoadAlbumScreenShotImageEx0"},
        {15, nullptr, "GetAlbumUsage3"},
        {16, nullptr, "GetAlbumMountResult"},
        {17, nullptr, "GetAlbumUsage16"},
        {18, nullptr, "Unknown18"},
        {19, nullptr, "Unknown19"},
        {100, nullptr, "GetAlbumFileCountEx0"},
        {101, nullptr, "GetAlbumFileListEx0"},
        {202, nullptr, "SaveEditedScreenShot"},
        {301, nullptr, "GetLastThumbnail"},
        {302, nullptr, "GetLastOverlayMovieThumbnail"},
        {401, nullptr, "GetAutoSavingStorage"},
        {501, nullptr, "GetRequiredStorageSpaceSizeToCopyAll"},
        {1001, nullptr, "LoadAlbumScreenShotThumbnailImageEx0"},
        {1002, nullptr, "LoadAlbumScreenShotImageEx1"},
        {1003, nullptr, "LoadAlbumScreenShotThumbnailImageEx1"},
        {8001, nullptr, "ForceAlbumUnmounted"},
        {8002, nullptr, "ResetAlbumMountStatus"},
        {8011, nullptr, "RefreshAlbumCache"},
        {8012, nullptr, "GetAlbumCache"},
        {8013, nullptr, "GetAlbumCacheEx"},
        {8021, nullptr, "GetAlbumEntryFromApplicationAlbumEntryAruid"},
        {10011, nullptr, "SetInternalErrorConversionEnabled"},
        {50000, nullptr, "LoadMakerNoteInfoForDebug"},
        {60002, nullptr, "OpenAccessorSession"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

CAPS_A::~CAPS_A() = default;

} // namespace Service::Capture
