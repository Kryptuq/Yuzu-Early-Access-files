// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/frontend/applets/controller.h"
#include "core/hle/service/hid/controllers/npad.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/sm/sm.h"

namespace Core::Frontend {

ControllerApplet::~ControllerApplet() = default;

DefaultControllerApplet::DefaultControllerApplet(Service::SM::ServiceManager& service_manager_)
    : service_manager{service_manager_} {}

DefaultControllerApplet::~DefaultControllerApplet() = default;

void DefaultControllerApplet::ReconfigureControllers(std::function<void()> callback,
                                                     const ControllerParameters& parameters) const {
    LOG_INFO(Service_HID, "called, deducing the best configuration based on the given parameters!");

    auto& npad =
        service_manager.GetService<Service::HID::Hid>("hid")
            ->GetAppletResource()
            ->GetController<Service::HID::Controller_NPad>(Service::HID::HidController::NPad);

    auto& players = Settings::values.players.GetValue();

    const std::size_t min_supported_players =
        parameters.enable_single_mode ? 1 : parameters.min_players;

    // Disconnect Handheld first.
    npad.DisconnectNpadAtIndex(8);

    // Deduce the best configuration based on the input parameters.
    for (std::size_t index = 0; index < players.size() - 2; ++index) {
        // First, disconnect all controllers regardless of the value of keep_controllers_connected.
        // This makes it easy to connect the desired controllers.
        npad.DisconnectNpadAtIndex(index);

        // Only connect the minimum number of required players.
        if (index >= min_supported_players) {
            continue;
        }

        // Connect controllers based on the following priority list from highest to lowest priority:
        // Pro Controller -> Dual Joycons -> Left Joycon/Right Joycon -> Handheld
        if (parameters.allow_pro_controller) {
            npad.AddNewControllerAt(
                npad.MapSettingsTypeToNPad(Settings::ControllerType::ProController), index);
        } else if (parameters.allow_dual_joycons) {
            npad.AddNewControllerAt(
                npad.MapSettingsTypeToNPad(Settings::ControllerType::DualJoyconDetached), index);
        } else if (parameters.allow_left_joycon && parameters.allow_right_joycon) {
            // Assign left joycons to even player indices and right joycons to odd player indices.
            // We do this since Captain Toad Treasure Tracker expects a left joycon for Player 1 and
            // a right Joycon for Player 2 in 2 Player Assist mode.
            if (index % 2 == 0) {
                npad.AddNewControllerAt(
                    npad.MapSettingsTypeToNPad(Settings::ControllerType::LeftJoycon), index);
            } else {
                npad.AddNewControllerAt(
                    npad.MapSettingsTypeToNPad(Settings::ControllerType::RightJoycon), index);
            }
        } else if (index == 0 && parameters.enable_single_mode && parameters.allow_handheld &&
                   !Settings::values.use_docked_mode.GetValue()) {
            // We should *never* reach here under any normal circumstances.
            npad.AddNewControllerAt(npad.MapSettingsTypeToNPad(Settings::ControllerType::Handheld),
                                    index);
        } else {
            UNREACHABLE_MSG("Unable to add a new controller based on the given parameters!");
        }
    }

    callback();
}

} // namespace Core::Frontend
