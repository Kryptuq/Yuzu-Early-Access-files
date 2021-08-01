// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

// This file contains yuzu's HLE API version constants.

namespace HLE::ApiVersion {

// Horizon OS version constants.

constexpr u8 HOS_VERSION_MAJOR = 12;
constexpr u8 HOS_VERSION_MINOR = 1;
constexpr u8 HOS_VERSION_MICRO = 0;

// NintendoSDK version constants.

constexpr u8 SDK_REVISION_MAJOR = 1;
constexpr u8 SDK_REVISION_MINOR = 0;

constexpr char PLATFORM_STRING[] = "NX";
constexpr char VERSION_HASH[] = "76b10c2dab7d3aa73fc162f8dff1655e6a21caf4";
constexpr char DISPLAY_VERSION[] = "12.1.0";
constexpr char DISPLAY_TITLE[] = "NintendoSDK Firmware for NX 12.1.0-1.0";

// Atmosphere version constants.

constexpr u8 ATMOSPHERE_RELEASE_VERSION_MAJOR = 0;
constexpr u8 ATMOSPHERE_RELEASE_VERSION_MINOR = 19;
constexpr u8 ATMOSPHERE_RELEASE_VERSION_MICRO = 5;

constexpr u32 GetTargetFirmware() {
    return u32{HOS_VERSION_MAJOR} << 24 | u32{HOS_VERSION_MINOR} << 16 |
           u32{HOS_VERSION_MICRO} << 8 | 0U;
}

} // namespace HLE::ApiVersion
