// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/uuid.h"
#include "core/hle/service/glue/glue_manager.h"
#include "core/hle/service/service.h"

namespace Service::Account {

class ProfileManager;

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> module_,
                           std::shared_ptr<ProfileManager> profile_manager_, Core::System& system_,
                           const char* name);
        ~Interface() override;

        void GetUserCount(HLERequestContext& ctx);
        void GetUserExistence(HLERequestContext& ctx);
        void ListAllUsers(HLERequestContext& ctx);
        void ListOpenUsers(HLERequestContext& ctx);
        void GetLastOpenedUser(HLERequestContext& ctx);
        void GetProfile(HLERequestContext& ctx);
        void InitializeApplicationInfo(HLERequestContext& ctx);
        void InitializeApplicationInfoRestricted(HLERequestContext& ctx);
        void GetBaasAccountManagerForApplication(HLERequestContext& ctx);
        void IsUserRegistrationRequestPermitted(HLERequestContext& ctx);
        void TrySelectUserWithoutInteraction(HLERequestContext& ctx);
        void IsUserAccountSwitchLocked(HLERequestContext& ctx);
        void InitializeApplicationInfoV2(HLERequestContext& ctx);
        void GetProfileEditor(HLERequestContext& ctx);
        void ListQualifiedUsers(HLERequestContext& ctx);
        void ListOpenContextStoredUsers(HLERequestContext& ctx);
        void StoreSaveDataThumbnailApplication(HLERequestContext& ctx);
        void StoreSaveDataThumbnailSystem(HLERequestContext& ctx);

    private:
        Result InitializeApplicationInfoBase();
        void StoreSaveDataThumbnail(HLERequestContext& ctx, const Common::UUID& uuid,
                                    const u64 tid);

        enum class ApplicationType : u32_le {
            GameCard = 0,
            Digital = 1,
            Unknown = 3,
        };

        struct ApplicationInfo {
            Service::Glue::ApplicationLaunchProperty launch_property;
            ApplicationType application_type;

            constexpr explicit operator bool() const {
                return launch_property.title_id != 0x0;
            }
        };

        ApplicationInfo application_info{};

    protected:
        std::shared_ptr<Module> module;
        std::shared_ptr<ProfileManager> profile_manager;
    };
};

void LoopProcess(Core::System& system);

} // namespace Service::Account
