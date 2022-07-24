// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/frontend/applets/mii.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applets/applet_mii.h"
#include "core/reporter.h"

namespace Service::AM::Applets {

Mii::Mii(Core::System& system_, LibraryAppletMode applet_mode_,
         const Core::Frontend::MiiApplet& frontend_)
    : Applet{system_, applet_mode_}, frontend{frontend_}, system{system_} {}

Mii::~Mii() = default;

void Mii::Initialize() {
    is_complete = false;

    const auto storage = broker.PopNormalDataToApplet();
    ASSERT(storage != nullptr);

    const auto data = storage->GetData();
    ASSERT(data.size() == sizeof(MiiAppletInput));

    std::memcpy(&input_data, data.data(), sizeof(MiiAppletInput));
}

bool Mii::TransactionComplete() const {
    return is_complete;
}

ResultCode Mii::GetStatus() const {
    return ResultSuccess;
}

void Mii::ExecuteInteractive() {
    UNREACHABLE_MSG("Unexpected interactive applet data!");
}

void Mii::Execute() {
    if (is_complete) {
        return;
    }

    const auto callback = [this](const Core::Frontend::MiiParameters& parameters) {
        DisplayCompleted(parameters);
    };

    switch (input_data.applet_mode) {
    case MiiAppletMode::ShowMiiEdit: {
        Service::Mii::MiiManager manager;
        Core::Frontend::MiiParameters params{
            .is_editable = false,
            .mii_data = input_data.mii_char_info.mii_data,
        };
        frontend.ShowMii(params, callback);
        break;
    }
    case MiiAppletMode::EditMii: {
        Service::Mii::MiiManager manager;
        Core::Frontend::MiiParameters params{
            .is_editable = true,
            .mii_data = input_data.mii_char_info.mii_data,
        };
        frontend.ShowMii(params, callback);
        break;
    }
    case MiiAppletMode::CreateMii: {
        Service::Mii::MiiManager manager;
        Core::Frontend::MiiParameters params{
            .is_editable = true,
            .mii_data = manager.BuildDefault(0),
        };
        frontend.ShowMii(params, callback);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unimplemented LibAppletMiiEdit mode={:02X}!", input_data.applet_mode);
    }
}

void Mii::DisplayCompleted(const Core::Frontend::MiiParameters& parameters) {
    is_complete = true;

    std::vector<u8> reply(sizeof(AppletOutputForCharInfoEditing));
    output_data = {
        .result = ResultSuccess,
        .mii_data = parameters.mii_data,
    };

    std::memcpy(reply.data(), &output_data, sizeof(AppletOutputForCharInfoEditing));
    broker.PushNormalDataFromApplet(std::make_shared<IStorage>(system, std::move(reply)));
    broker.SignalStateChanged();
}

} // namespace Service::AM::Applets
