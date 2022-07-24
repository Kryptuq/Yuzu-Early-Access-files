// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hle/service/mii/types.h"

namespace Service::AM::Applets {

enum class MiiEditAppletVersion : s32 {
    Version3 = 0x3, // 1.0.0 - 10.1.1
    Version4 = 0x4, // 10.2.0+
};

// This is nn::mii::AppletMode
enum class MiiEditAppletMode : u32 {
    ShowMiiEdit = 0,
    AppendMii = 1,
    AppendMiiImage = 2,
    UpdateMiiImage = 3,
    CreateMii = 4,
    EditMii = 5,
};

enum class MiiEditResult : u32 {
    Success,
    Cancel,
};

struct MiiEditCharInfo {
    Service::Mii::MiiInfo mii_info{};
};
static_assert(sizeof(MiiEditCharInfo) == 0x58, "MiiEditCharInfo has incorrect size.");

struct MiiEditAppletInputCommon {
    MiiEditAppletVersion version{};
    MiiEditAppletMode applet_mode{};
};
static_assert(sizeof(MiiEditAppletInputCommon) == 0x8,
              "MiiEditAppletInputCommon has incorrect size.");

struct MiiEditAppletInputV3 {
    u32 special_mii_key_code{};
    std::array<Common::UUID, 8> valid_uuids{};
    Common::UUID used_uuid{};
    INSERT_PADDING_BYTES(0x64);
};
static_assert(sizeof(MiiEditAppletInputV3) == 0x100 - sizeof(MiiEditAppletInputCommon),
              "MiiEditAppletInputV3 has incorrect size.");

struct MiiEditAppletInputV4 {
    u32 special_mii_key_code{};
    MiiEditCharInfo char_info{};
    INSERT_PADDING_BYTES(0x28);
    Common::UUID used_uuid{};
    INSERT_PADDING_BYTES(0x64);
};
static_assert(sizeof(MiiEditAppletInputV4) == 0x100 - sizeof(MiiEditAppletInputCommon),
              "MiiEditAppletInputV4 has incorrect size.");

// This is nn::mii::AppletOutput
struct MiiEditAppletOutput {
    MiiEditResult result{};
    s32 index{};
    INSERT_PADDING_BYTES(0x18);
};
static_assert(sizeof(MiiEditAppletOutput) == 0x20, "MiiEditAppletOutput has incorrect size.");

// This is nn::mii::AppletOutputForCharInfoEditing
struct MiiEditAppletOutputForCharInfoEditing {
    MiiEditResult result{};
    MiiEditCharInfo char_info{};
    INSERT_PADDING_BYTES(0x24);
};
static_assert(sizeof(MiiEditAppletOutputForCharInfoEditing) == 0x80,
              "MiiEditAppletOutputForCharInfoEditing has incorrect size.");

} // namespace Service::AM::Applets
