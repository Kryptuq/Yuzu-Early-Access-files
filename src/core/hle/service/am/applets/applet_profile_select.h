// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "common/common_funcs.h"
#include "common/uuid.h"
#include "core/hle/result.h"
#include "core/hle/service/am/applets/applets.h"

namespace Core {
class System;
}

namespace Service::AM::Applets {

enum class ProfileSelectAppletVersion : u32 {
    Version1 = 0x1,     // 1.0.0+
    Version2 = 0x10000, // 2.0.0+
    Version3 = 0x20000, // 6.0.0+
};

// This is nn::account::UiMode
enum class UiMode {
    UserSelector,
    UserCreator,
    EnsureNetworkServiceAccountAvailable,
    UserIconEditor,
    UserNicknameEditor,
    UserCreatorForStarter,
    NintendoAccountAuthorizationRequestContext,
    IntroduceExternalNetworkServiceAccount,
    IntroduceExternalNetworkServiceAccountForRegistration,
    NintendoAccountNnidLinker,
    LicenseRequirementsForNetworkService,
    LicenseRequirementsForNetworkServiceWithUserContextImpl,
    UserCreatorForImmediateNaLoginTest,
    UserQualificationPromoter,
};

// This is nn::account::UserSelectionPurpose
enum class UserSelectionPurpose {
    General,
    GameCardRegistration,
    EShopLaunch,
    EShopItemShow,
    PicturePost,
    NintendoAccountLinkage,
    SettingsUpdate,
    SaveDataDeletion,
    UserMigration,
    SaveDataTransfer,
};

// This is nn::account::NintendoAccountStartupDialogType
enum class NintendoAccountStartupDialogType {
    LoginAndCreate,
    Login,
    Create,
};

// This is nn::account::UserSelectionSettingsForSystemService
struct UserSelectionSettingsForSystemService {
    UserSelectionPurpose purpose;
    bool enable_user_creation;
    INSERT_PADDING_BYTES(0x3);
};
static_assert(sizeof(UserSelectionSettingsForSystemService) == 0x8,
              "UserSelectionSettingsForSystemService has incorrect size.");

struct UiSettingsDisplayOptions {
    bool is_network_service_account_required;
    bool is_skip_enabled;
    bool is_system_or_launcher;
    bool is_registration_permitted;
    bool show_skip_button;
    bool aditional_select;
    bool show_user_selector;
    bool is_unqualified_user_selectable;
};
static_assert(sizeof(UiSettingsDisplayOptions) == 0x8,
              "UiSettingsDisplayOptions has incorrect size.");

struct UiSettingsV1 {
    UiMode mode;
    INSERT_PADDING_BYTES(0x4);
    std::array<Common::UUID, 8> invalid_uid_list;
    u64 application_id;
    UiSettingsDisplayOptions display_options;
};
static_assert(sizeof(UiSettingsV1) == 0x98, "UiSettings has incorrect size.");

// This is nn::account::UiSettings
struct UiSettings {
    UiMode mode;
    INSERT_PADDING_BYTES(0x4);
    std::array<Common::UUID, 8> invalid_uid_list;
    u64 application_id;
    UiSettingsDisplayOptions display_options;
    UserSelectionPurpose purpose;
    INSERT_PADDING_BYTES(0x4);
};
static_assert(sizeof(UiSettings) == 0xA0, "UiSettings has incorrect size.");

// This is nn::account::UiReturnArg
struct UiReturnArg {
    u64 result;
    Common::UUID uuid_selected;
};
static_assert(sizeof(UiReturnArg) == 0x18, "UiReturnArg has incorrect size.");

class ProfileSelect final : public Applet {
public:
    explicit ProfileSelect(Core::System& system_, LibraryAppletMode applet_mode_,
                           const Core::Frontend::ProfileSelectApplet& frontend_);
    ~ProfileSelect() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    Result GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;
    Result RequestExit() override;

    void SelectionComplete(std::optional<Common::UUID> uuid);

private:
    const Core::Frontend::ProfileSelectApplet& frontend;

    UiSettings config;
    UiSettingsV1 config_old;
    ProfileSelectAppletVersion profile_select_version;

    bool complete = false;
    Result status = ResultSuccess;
    std::vector<u8> final_data;
    Core::System& system;
};

} // namespace Service::AM::Applets
