// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace Service::AM::Applets {

enum class WebAppletVersion : u32_le {
    Version0 = 0x0,          // Only used by WifiWebAuthApplet
    Version131072 = 0x20000, // 1.0.0 - 2.3.0
    Version196608 = 0x30000, // 3.0.0 - 4.1.0
    Version327680 = 0x50000, // 5.0.0 - 5.1.0
    Version393216 = 0x60000, // 6.0.0 - 7.0.1
    Version524288 = 0x80000, // 8.0.0+
};

enum class ShimKind : u32 {
    Shop = 1,
    Login = 2,
    Offline = 3,
    Share = 4,
    Web = 5,
    Wifi = 6,
    Lobby = 7,
};

enum class WebExitReason : u32 {
    EndButtonPressed = 0,
    BackButtonPressed = 1,
    ExitRequested = 2,
    CallbackURL = 3,
    WindowClosed = 4,
    ErrorDialog = 7,
};

enum class WebArgInputTLVType : u16 {
    InitialURL = 0x1,
    CallbackURL = 0x3,
    CallbackableURL = 0x4,
    ApplicationID = 0x5,
    DocumentPath = 0x6,
    DocumentKind = 0x7,
    SystemDataID = 0x8,
    ShareStartPage = 0x9,
    Whitelist = 0xA,
    News = 0xB,
    UserID = 0xE,
    AlbumEntry0 = 0xF,
    ScreenShotEnabled = 0x10,
    EcClientCertEnabled = 0x11,
    PlayReportEnabled = 0x13,
    BootDisplayKind = 0x17,
    BackgroundKind = 0x18,
    FooterEnabled = 0x19,
    PointerEnabled = 0x1A,
    LeftStickMode = 0x1B,
    KeyRepeatFrame1 = 0x1C,
    KeyRepeatFrame2 = 0x1D,
    BootAsMediaPlayerInverted = 0x1E,
    DisplayURLKind = 0x1F,
    BootAsMediaPlayer = 0x21,
    ShopJumpEnabled = 0x22,
    MediaAutoPlayEnabled = 0x23,
    LobbyParameter = 0x24,
    ApplicationAlbumEntry = 0x26,
    JsExtensionEnabled = 0x27,
    AdditionalCommentText = 0x28,
    TouchEnabledOnContents = 0x29,
    UserAgentAdditionalString = 0x2A,
    AdditionalMediaData0 = 0x2B,
    MediaPlayerAutoCloseEnabled = 0x2C,
    PageCacheEnabled = 0x2D,
    WebAudioEnabled = 0x2E,
    YouTubeVideoWhitelist = 0x31,
    FooterFixedKind = 0x32,
    PageFadeEnabled = 0x33,
    MediaCreatorApplicationRatingAge = 0x34,
    BootLoadingIconEnabled = 0x35,
    PageScrollIndicatorEnabled = 0x36,
    MediaPlayerSpeedControlEnabled = 0x37,
    AlbumEntry1 = 0x38,
    AlbumEntry2 = 0x39,
    AlbumEntry3 = 0x3A,
    AdditionalMediaData1 = 0x3B,
    AdditionalMediaData2 = 0x3C,
    AdditionalMediaData3 = 0x3D,
    BootFooterButton = 0x3E,
    OverrideWebAudioVolume = 0x3F,
    OverrideMediaAudioVolume = 0x40,
    BootMode = 0x41,
    WebSessionEnabled = 0x42,
    MediaPlayerOfflineEnabled = 0x43,
};

enum class WebArgOutputTLVType : u16 {
    ShareExitReason = 0x1,
    LastURL = 0x2,
    LastURLSize = 0x3,
    SharePostResult = 0x4,
    PostServiceName = 0x5,
    PostServiceNameSize = 0x6,
    PostID = 0x7,
    PostIDSize = 0x8,
    MediaPlayerAutoClosedByCompletion = 0x9,
};

enum class DocumentKind : u32 {
    OfflineHtmlPage = 1,
    ApplicationLegalInformation = 2,
    SystemDataPage = 3,
};

enum class ShareStartPage : u32 {
    Default,
    Settings,
};

enum class BootDisplayKind : u32 {
    Default,
    White,
    Black,
};

enum class BackgroundKind : u32 {
    Default,
};

enum class LeftStickMode : u32 {
    Pointer,
    Cursor,
};

enum class WebSessionBootMode : u32 {
    AllForeground,
    AllForegroundInitiallyHidden,
};

struct WebArgHeader {
    u16 total_tlv_entries{};
    INSERT_PADDING_BYTES(2);
    ShimKind shim_kind{};
};
static_assert(sizeof(WebArgHeader) == 0x8, "WebArgHeader has incorrect size.");

struct WebArgInputTLV {
    WebArgInputTLVType input_tlv_type{};
    u16 arg_data_size{};
    INSERT_PADDING_WORDS(1);
};
static_assert(sizeof(WebArgInputTLV) == 0x8, "WebArgInputTLV has incorrect size.");

struct WebArgOutputTLV {
    WebArgOutputTLVType output_tlv_type{};
    u16 arg_data_size{};
    INSERT_PADDING_WORDS(1);
};
static_assert(sizeof(WebArgOutputTLV) == 0x8, "WebArgOutputTLV has incorrect size.");

struct WebCommonReturnValue {
    WebExitReason exit_reason{};
    INSERT_PADDING_WORDS(1);
    std::array<char, 0x1000> last_url{};
    u64 last_url_size{};
};
static_assert(sizeof(WebCommonReturnValue) == 0x1010, "WebCommonReturnValue has incorrect size.");

using WebArgInputTLVMap = std::unordered_map<WebArgInputTLVType, std::vector<u8>>;

} // namespace Service::AM::Applets
