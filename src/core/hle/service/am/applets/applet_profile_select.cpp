// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/assert.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/frontend/applets/profile_select.h"
#include "core/hle/service/acc/errors.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applets/applet_profile_select.h"

namespace Service::AM::Applets {

ProfileSelect::ProfileSelect(Core::System& system_, LibraryAppletMode applet_mode_,
                             const Core::Frontend::ProfileSelectApplet& frontend_)
    : Applet{system_, applet_mode_}, frontend{frontend_}, system{system_} {}

ProfileSelect::~ProfileSelect() = default;

void ProfileSelect::Initialize() {
    complete = false;
    status = ResultSuccess;
    final_data.clear();

    Applet::Initialize();
    profile_select_version = ProfileSelectAppletVersion{common_args.library_version};

    const auto user_config_storage = broker.PopNormalDataToApplet();
    ASSERT(user_config_storage != nullptr);
    const auto& user_config = user_config_storage->GetData();

    LOG_INFO(Service_AM, "Initializing Profile Select Applet with version={}",
             profile_select_version);

    switch (profile_select_version) {
    case ProfileSelectAppletVersion::Version1:
        ASSERT(user_config.size() == sizeof(UiSettingsV1));
        std::memcpy(&config_old, user_config.data(), sizeof(UiSettingsV1));
        break;
    case ProfileSelectAppletVersion::Version2:
    case ProfileSelectAppletVersion::Version3:
        ASSERT(user_config.size() == sizeof(UiSettings));
        std::memcpy(&config, user_config.data(), sizeof(UiSettings));
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown profile_select_version = {}", profile_select_version);
        break;
    }
}

bool ProfileSelect::TransactionComplete() const {
    return complete;
}

Result ProfileSelect::GetStatus() const {
    return status;
}

void ProfileSelect::ExecuteInteractive() {
    ASSERT_MSG(false, "Attempted to call interactive execution on non-interactive applet.");
}

void ProfileSelect::Execute() {
    if (complete) {
        broker.PushNormalDataFromApplet(std::make_shared<IStorage>(system, std::move(final_data)));
        return;
    }

    Core::Frontend::ProfileSelectParameters parameters{};

    switch (profile_select_version) {
    case ProfileSelectAppletVersion::Version1:
        parameters = {
            .mode = config_old.mode,
            .invalid_uid_list = config_old.invalid_uid_list,
            .display_options = config_old.display_options,
            .purpose = UserSelectionPurpose::General,
        };
        break;
    case ProfileSelectAppletVersion::Version2:
    case ProfileSelectAppletVersion::Version3:
        parameters = {
            .mode = config.mode,
            .invalid_uid_list = config.invalid_uid_list,
            .display_options = config.display_options,
            .purpose = config.purpose,
        };
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown profile_select_version = {}", profile_select_version);
        break;
    }

    frontend.SelectProfile([this](std::optional<Common::UUID> uuid) { SelectionComplete(uuid); },
                           parameters);
}

void ProfileSelect::SelectionComplete(std::optional<Common::UUID> uuid) {
    UiReturnArg output{};

    if (uuid.has_value() && uuid->IsValid()) {
        output.result = 0;
        output.uuid_selected = *uuid;
    } else {
        status = Account::ResultCancelledByUser;
        output.result = Account::ResultCancelledByUser.raw;
        output.uuid_selected = Common::InvalidUUID;
    }

    final_data = std::vector<u8>(sizeof(UiReturnArg));
    std::memcpy(final_data.data(), &output, final_data.size());
    broker.PushNormalDataFromApplet(std::make_shared<IStorage>(system, std::move(final_data)));
    broker.SignalStateChanged();
}

Result ProfileSelect::RequestExit() {
    frontend.Close();
    R_SUCCEED();
}

} // namespace Service::AM::Applets
