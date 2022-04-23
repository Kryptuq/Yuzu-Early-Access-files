// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/caps/caps_c.h"

namespace Service::Capture {

class IAlbumControlSession final : public ServiceFramework<IAlbumControlSession> {
public:
    explicit IAlbumControlSession(Core::System& system_)
        : ServiceFramework{system_, "IAlbumControlSession"} {
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
            {2401, nullptr, "OpenAlbumMovieWriteStream"},
            {2402, nullptr, "FinishAlbumMovieWriteStream"},
            {2403, nullptr, "CommitAlbumMovieWriteStream"},
            {2404, nullptr, "DiscardAlbumMovieWriteStream"},
            {2405, nullptr, "DiscardAlbumMovieWriteStreamNoDelete"},
            {2406, nullptr, "CommitAlbumMovieWriteStreamEx"},
            {2411, nullptr, "StartAlbumMovieWriteStreamDataSection"},
            {2412, nullptr, "EndAlbumMovieWriteStreamDataSection"},
            {2413, nullptr, "StartAlbumMovieWriteStreamMetaSection"},
            {2414, nullptr, "EndAlbumMovieWriteStreamMetaSection"},
            {2421, nullptr, "ReadDataFromAlbumMovieWriteStream"},
            {2422, nullptr, "WriteDataToAlbumMovieWriteStream"},
            {2424, nullptr, "WriteMetaToAlbumMovieWriteStream"},
            {2431, nullptr, "GetAlbumMovieWriteStreamBrokenReason"},
            {2433, nullptr, "GetAlbumMovieWriteStreamDataSize"},
            {2434, nullptr, "SetAlbumMovieWriteStreamDataSize"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

CAPS_C::CAPS_C(Core::System& system_) : ServiceFramework{system_, "caps:c"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {1, nullptr, "CaptureRawImage"},
        {2, nullptr, "CaptureRawImageWithTimeout"},
        {33, &CAPS_C::SetShimLibraryVersion, "SetShimLibraryVersion"},
        {1001, nullptr, "RequestTakingScreenShot"},
        {1002, nullptr, "RequestTakingScreenShotWithTimeout"},
        {1011, nullptr, "NotifyTakingScreenShotRefused"},
        {2001, nullptr, "NotifyAlbumStorageIsAvailable"},
        {2002, nullptr, "NotifyAlbumStorageIsUnavailable"},
        {2011, nullptr, "RegisterAppletResourceUserId"},
        {2012, nullptr, "UnregisterAppletResourceUserId"},
        {2013, nullptr, "GetApplicationIdFromAruid"},
        {2014, nullptr, "CheckApplicationIdRegistered"},
        {2101, nullptr, "GenerateCurrentAlbumFileId"},
        {2102, nullptr, "GenerateApplicationAlbumEntry"},
        {2201, nullptr, "SaveAlbumScreenShotFile"},
        {2202, nullptr, "SaveAlbumScreenShotFileEx"},
        {2301, nullptr, "SetOverlayScreenShotThumbnailData"},
        {2302, nullptr, "SetOverlayMovieThumbnailData"},
        {60001, nullptr, "OpenControlSession"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

CAPS_C::~CAPS_C() = default;

void CAPS_C::SetShimLibraryVersion(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto library_version{rp.Pop<u64>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_Capture, "(STUBBED) called. library_version={}, applet_resource_user_id={}",
                library_version, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

} // namespace Service::Capture
