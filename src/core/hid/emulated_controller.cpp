// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "core/hid/emulated_controller.h"
#include "core/hid/input_converter.h"

namespace Core::HID {
constexpr s32 HID_JOYSTICK_MAX = 0x7fff;
constexpr s32 HID_TRIGGER_MAX = 0x7fff;

EmulatedController::EmulatedController(NpadIdType npad_id_type_) : npad_id_type(npad_id_type_) {}

EmulatedController::~EmulatedController() = default;

NpadStyleIndex EmulatedController::MapSettingsTypeToNPad(Settings::ControllerType type) {
    switch (type) {
    case Settings::ControllerType::ProController:
        return NpadStyleIndex::ProController;
    case Settings::ControllerType::DualJoyconDetached:
        return NpadStyleIndex::JoyconDual;
    case Settings::ControllerType::LeftJoycon:
        return NpadStyleIndex::JoyconLeft;
    case Settings::ControllerType::RightJoycon:
        return NpadStyleIndex::JoyconRight;
    case Settings::ControllerType::Handheld:
        return NpadStyleIndex::Handheld;
    case Settings::ControllerType::GameCube:
        return NpadStyleIndex::GameCube;
    case Settings::ControllerType::Pokeball:
        return NpadStyleIndex::Pokeball;
    case Settings::ControllerType::NES:
        return NpadStyleIndex::NES;
    case Settings::ControllerType::SNES:
        return NpadStyleIndex::SNES;
    case Settings::ControllerType::N64:
        return NpadStyleIndex::N64;
    case Settings::ControllerType::SegaGenesis:
        return NpadStyleIndex::SegaGenesis;
    default:
        return NpadStyleIndex::ProController;
    }
}

Settings::ControllerType EmulatedController::MapNPadToSettingsType(NpadStyleIndex type) {
    switch (type) {
    case NpadStyleIndex::ProController:
        return Settings::ControllerType::ProController;
    case NpadStyleIndex::JoyconDual:
        return Settings::ControllerType::DualJoyconDetached;
    case NpadStyleIndex::JoyconLeft:
        return Settings::ControllerType::LeftJoycon;
    case NpadStyleIndex::JoyconRight:
        return Settings::ControllerType::RightJoycon;
    case NpadStyleIndex::Handheld:
        return Settings::ControllerType::Handheld;
    case NpadStyleIndex::GameCube:
        return Settings::ControllerType::GameCube;
    case NpadStyleIndex::Pokeball:
        return Settings::ControllerType::Pokeball;
    case NpadStyleIndex::NES:
        return Settings::ControllerType::NES;
    case NpadStyleIndex::SNES:
        return Settings::ControllerType::SNES;
    case NpadStyleIndex::N64:
        return Settings::ControllerType::N64;
    case NpadStyleIndex::SegaGenesis:
        return Settings::ControllerType::SegaGenesis;
    default:
        return Settings::ControllerType::ProController;
    }
}

void EmulatedController::ReloadFromSettings() {
    const auto player_index = NpadIdTypeToIndex(npad_id_type);
    const auto& player = Settings::values.players.GetValue()[player_index];

    for (std::size_t index = 0; index < player.buttons.size(); ++index) {
        button_params[index] = Common::ParamPackage(player.buttons[index]);
    }
    for (std::size_t index = 0; index < player.analogs.size(); ++index) {
        stick_params[index] = Common::ParamPackage(player.analogs[index]);
    }
    for (std::size_t index = 0; index < player.motions.size(); ++index) {
        motion_params[index] = Common::ParamPackage(player.motions[index]);
    }

    controller.colors_state.left = {
        .body = player.body_color_left,
        .button = player.button_color_left,
    };

    controller.colors_state.right = {
        .body = player.body_color_right,
        .button = player.button_color_right,
    };

    controller.colors_state.fullkey = controller.colors_state.left;

    // Other or debug controller should always be a pro controller
    if (npad_id_type != NpadIdType::Other) {
        SetNpadStyleIndex(MapSettingsTypeToNPad(player.controller_type));
    } else {
        SetNpadStyleIndex(NpadStyleIndex::ProController);
    }

    if (player.connected) {
        Connect();
    } else {
        Disconnect();
    }

    ReloadInput();
}

void EmulatedController::LoadDevices() {
    // TODO(german77): Use more buttons to detect the correct device
    const auto left_joycon = button_params[Settings::NativeButton::DRight];
    const auto right_joycon = button_params[Settings::NativeButton::A];

    // Triggers for GC controllers
    trigger_params[LeftIndex] = button_params[Settings::NativeButton::ZL];
    trigger_params[RightIndex] = button_params[Settings::NativeButton::ZR];

    battery_params[LeftIndex] = left_joycon;
    battery_params[RightIndex] = right_joycon;
    battery_params[LeftIndex].Set("battery", true);
    battery_params[RightIndex].Set("battery", true);

    output_params[LeftIndex] = left_joycon;
    output_params[RightIndex] = right_joycon;
    output_params[LeftIndex].Set("output", true);
    output_params[RightIndex].Set("output", true);

    LoadTASParams();

    std::transform(button_params.begin() + Settings::NativeButton::BUTTON_HID_BEGIN,
                   button_params.begin() + Settings::NativeButton::BUTTON_NS_END,
                   button_devices.begin(), Common::Input::CreateDevice<Common::Input::InputDevice>);
    std::transform(stick_params.begin() + Settings::NativeAnalog::STICK_HID_BEGIN,
                   stick_params.begin() + Settings::NativeAnalog::STICK_HID_END,
                   stick_devices.begin(), Common::Input::CreateDevice<Common::Input::InputDevice>);
    std::transform(motion_params.begin() + Settings::NativeMotion::MOTION_HID_BEGIN,
                   motion_params.begin() + Settings::NativeMotion::MOTION_HID_END,
                   motion_devices.begin(), Common::Input::CreateDevice<Common::Input::InputDevice>);
    std::transform(trigger_params.begin(), trigger_params.end(), trigger_devices.begin(),
                   Common::Input::CreateDevice<Common::Input::InputDevice>);
    std::transform(battery_params.begin(), battery_params.begin(), battery_devices.end(),
                   Common::Input::CreateDevice<Common::Input::InputDevice>);
    std::transform(output_params.begin(), output_params.end(), output_devices.begin(),
                   Common::Input::CreateDevice<Common::Input::OutputDevice>);

    // Initialize TAS devices
    std::transform(tas_button_params.begin(), tas_button_params.end(), tas_button_devices.begin(),
                   Common::Input::CreateDevice<Common::Input::InputDevice>);
    std::transform(tas_stick_params.begin(), tas_stick_params.end(), tas_stick_devices.begin(),
                   Common::Input::CreateDevice<Common::Input::InputDevice>);
}

void EmulatedController::LoadTASParams() {
    const auto player_index = NpadIdTypeToIndex(npad_id_type);
    Common::ParamPackage common_params{};
    common_params.Set("engine", "tas");
    common_params.Set("port", static_cast<int>(player_index));
    for (auto& param : tas_button_params) {
        param = common_params;
    }
    for (auto& param : tas_stick_params) {
        param = common_params;
    }

    // TODO(german77): Replace this with an input profile or something better
    tas_button_params[Settings::NativeButton::A].Set("button", 0);
    tas_button_params[Settings::NativeButton::B].Set("button", 1);
    tas_button_params[Settings::NativeButton::X].Set("button", 2);
    tas_button_params[Settings::NativeButton::Y].Set("button", 3);
    tas_button_params[Settings::NativeButton::LStick].Set("button", 4);
    tas_button_params[Settings::NativeButton::RStick].Set("button", 5);
    tas_button_params[Settings::NativeButton::L].Set("button", 6);
    tas_button_params[Settings::NativeButton::R].Set("button", 7);
    tas_button_params[Settings::NativeButton::ZL].Set("button", 8);
    tas_button_params[Settings::NativeButton::ZR].Set("button", 9);
    tas_button_params[Settings::NativeButton::Plus].Set("button", 10);
    tas_button_params[Settings::NativeButton::Minus].Set("button", 11);
    tas_button_params[Settings::NativeButton::DLeft].Set("button", 12);
    tas_button_params[Settings::NativeButton::DUp].Set("button", 13);
    tas_button_params[Settings::NativeButton::DRight].Set("button", 14);
    tas_button_params[Settings::NativeButton::DDown].Set("button", 15);
    tas_button_params[Settings::NativeButton::SL].Set("button", 16);
    tas_button_params[Settings::NativeButton::SR].Set("button", 17);
    tas_button_params[Settings::NativeButton::Home].Set("button", 18);
    tas_button_params[Settings::NativeButton::Screenshot].Set("button", 19);

    tas_stick_params[Settings::NativeAnalog::LStick].Set("axis_x", 0);
    tas_stick_params[Settings::NativeAnalog::LStick].Set("axis_y", 1);
    tas_stick_params[Settings::NativeAnalog::RStick].Set("axis_x", 2);
    tas_stick_params[Settings::NativeAnalog::RStick].Set("axis_y", 3);
}

void EmulatedController::ReloadInput() {
    // If you load any device here add the equivalent to the UnloadInput() function
    LoadDevices();
    for (std::size_t index = 0; index < button_devices.size(); ++index) {
        if (!button_devices[index]) {
            continue;
        }
        const auto uuid = Common::UUID{button_params[index].Get("guid", "")};
        button_devices[index]->SetCallback({
            .on_change =
                [this, index, uuid](const Common::Input::CallbackStatus& callback) {
                    SetButton(callback, index, uuid);
                },
        });
        button_devices[index]->ForceUpdate();
    }

    for (std::size_t index = 0; index < stick_devices.size(); ++index) {
        if (!stick_devices[index]) {
            continue;
        }
        const auto uuid = Common::UUID{stick_params[index].Get("guid", "")};
        stick_devices[index]->SetCallback({
            .on_change =
                [this, index, uuid](const Common::Input::CallbackStatus& callback) {
                    SetStick(callback, index, uuid);
                },
        });
        stick_devices[index]->ForceUpdate();
    }

    for (std::size_t index = 0; index < trigger_devices.size(); ++index) {
        if (!trigger_devices[index]) {
            continue;
        }
        const auto uuid = Common::UUID{trigger_params[index].Get("guid", "")};
        trigger_devices[index]->SetCallback({
            .on_change =
                [this, index, uuid](const Common::Input::CallbackStatus& callback) {
                    SetTrigger(callback, index, uuid);
                },
        });
        trigger_devices[index]->ForceUpdate();
    }

    for (std::size_t index = 0; index < battery_devices.size(); ++index) {
        if (!battery_devices[index]) {
            continue;
        }
        battery_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetBattery(callback, index);
                },
        });
        battery_devices[index]->ForceUpdate();
    }

    for (std::size_t index = 0; index < motion_devices.size(); ++index) {
        if (!motion_devices[index]) {
            continue;
        }
        motion_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetMotion(callback, index);
                },
        });
        motion_devices[index]->ForceUpdate();
    }

    // Use a common UUID for TAS
    const auto tas_uuid = Common::UUID{0x0, 0x7A5};

    // Register TAS devices. No need to force update
    for (std::size_t index = 0; index < tas_button_devices.size(); ++index) {
        if (!tas_button_devices[index]) {
            continue;
        }
        tas_button_devices[index]->SetCallback({
            .on_change =
                [this, index, tas_uuid](const Common::Input::CallbackStatus& callback) {
                    SetButton(callback, index, tas_uuid);
                },
        });
    }

    for (std::size_t index = 0; index < tas_stick_devices.size(); ++index) {
        if (!tas_stick_devices[index]) {
            continue;
        }
        tas_stick_devices[index]->SetCallback({
            .on_change =
                [this, index, tas_uuid](const Common::Input::CallbackStatus& callback) {
                    SetStick(callback, index, tas_uuid);
                },
        });
    }
}

void EmulatedController::UnloadInput() {
    for (auto& button : button_devices) {
        button.reset();
    }
    for (auto& stick : stick_devices) {
        stick.reset();
    }
    for (auto& motion : motion_devices) {
        motion.reset();
    }
    for (auto& trigger : trigger_devices) {
        trigger.reset();
    }
    for (auto& battery : battery_devices) {
        battery.reset();
    }
    for (auto& output : output_devices) {
        output.reset();
    }
    for (auto& button : tas_button_devices) {
        button.reset();
    }
    for (auto& stick : tas_stick_devices) {
        stick.reset();
    }
}

void EmulatedController::EnableConfiguration() {
    is_configuring = true;
    tmp_is_connected = is_connected;
    tmp_npad_type = npad_type;
}

void EmulatedController::DisableConfiguration() {
    is_configuring = false;

    // Apply temporary npad type to the real controller
    if (tmp_npad_type != npad_type) {
        if (is_connected) {
            Disconnect();
        }
        SetNpadStyleIndex(tmp_npad_type);
    }

    // Apply temporary connected status to the real controller
    if (tmp_is_connected != is_connected) {
        if (tmp_is_connected) {
            Connect();
            return;
        }
        Disconnect();
    }
}

void EmulatedController::EnableSystemButtons() {
    system_buttons_enabled = true;
}

void EmulatedController::DisableSystemButtons() {
    system_buttons_enabled = false;
}

void EmulatedController::ResetSystemButtons() {
    controller.home_button_state.home.Assign(false);
    controller.capture_button_state.capture.Assign(false);
}

bool EmulatedController::IsConfiguring() const {
    return is_configuring;
}

void EmulatedController::SaveCurrentConfig() {
    const auto player_index = NpadIdTypeToIndex(npad_id_type);
    auto& player = Settings::values.players.GetValue()[player_index];
    player.connected = is_connected;
    player.controller_type = MapNPadToSettingsType(npad_type);
    for (std::size_t index = 0; index < player.buttons.size(); ++index) {
        player.buttons[index] = button_params[index].Serialize();
    }
    for (std::size_t index = 0; index < player.analogs.size(); ++index) {
        player.analogs[index] = stick_params[index].Serialize();
    }
    for (std::size_t index = 0; index < player.motions.size(); ++index) {
        player.motions[index] = motion_params[index].Serialize();
    }
}

void EmulatedController::RestoreConfig() {
    if (!is_configuring) {
        return;
    }
    ReloadFromSettings();
}

std::vector<Common::ParamPackage> EmulatedController::GetMappedDevices(
    EmulatedDeviceIndex device_index) const {
    std::vector<Common::ParamPackage> devices;
    for (const auto& param : button_params) {
        if (!param.Has("engine")) {
            continue;
        }
        const auto devices_it = std::find_if(
            devices.begin(), devices.end(), [param](const Common::ParamPackage param_) {
                return param.Get("engine", "") == param_.Get("engine", "") &&
                       param.Get("guid", "") == param_.Get("guid", "") &&
                       param.Get("port", 0) == param_.Get("port", 0);
            });
        if (devices_it != devices.end()) {
            continue;
        }
        Common::ParamPackage device{};
        device.Set("engine", param.Get("engine", ""));
        device.Set("guid", param.Get("guid", ""));
        device.Set("port", param.Get("port", 0));
        devices.push_back(device);
    }

    for (const auto& param : stick_params) {
        if (!param.Has("engine")) {
            continue;
        }
        if (param.Get("engine", "") == "analog_from_button") {
            continue;
        }
        const auto devices_it = std::find_if(
            devices.begin(), devices.end(), [param](const Common::ParamPackage param_) {
                return param.Get("engine", "") == param_.Get("engine", "") &&
                       param.Get("guid", "") == param_.Get("guid", "") &&
                       param.Get("port", 0) == param_.Get("port", 0);
            });
        if (devices_it != devices.end()) {
            continue;
        }
        Common::ParamPackage device{};
        device.Set("engine", param.Get("engine", ""));
        device.Set("guid", param.Get("guid", ""));
        device.Set("port", param.Get("port", 0));
        devices.push_back(device);
    }
    return devices;
}

Common::ParamPackage EmulatedController::GetButtonParam(std::size_t index) const {
    if (index >= button_params.size()) {
        return {};
    }
    return button_params[index];
}

Common::ParamPackage EmulatedController::GetStickParam(std::size_t index) const {
    if (index >= stick_params.size()) {
        return {};
    }
    return stick_params[index];
}

Common::ParamPackage EmulatedController::GetMotionParam(std::size_t index) const {
    if (index >= motion_params.size()) {
        return {};
    }
    return motion_params[index];
}

void EmulatedController::SetButtonParam(std::size_t index, Common::ParamPackage param) {
    if (index >= button_params.size()) {
        return;
    }
    button_params[index] = std::move(param);
    ReloadInput();
}

void EmulatedController::SetStickParam(std::size_t index, Common::ParamPackage param) {
    if (index >= stick_params.size()) {
        return;
    }
    stick_params[index] = std::move(param);
    ReloadInput();
}

void EmulatedController::SetMotionParam(std::size_t index, Common::ParamPackage param) {
    if (index >= motion_params.size()) {
        return;
    }
    motion_params[index] = std::move(param);
    ReloadInput();
}

void EmulatedController::SetButton(const Common::Input::CallbackStatus& callback, std::size_t index,
                                   Common::UUID uuid) {
    if (index >= controller.button_values.size()) {
        return;
    }
    {
        std::lock_guard lock{mutex};
        bool value_changed = false;
        const auto new_status = TransformToButton(callback);
        auto& current_status = controller.button_values[index];

        // Only read button values that have the same uuid or are pressed once
        if (current_status.uuid != uuid) {
            if (!new_status.value) {
                return;
            }
        }

        current_status.toggle = new_status.toggle;
        current_status.uuid = uuid;

        // Update button status with current
        if (!current_status.toggle) {
            current_status.locked = false;
            if (current_status.value != new_status.value) {
                current_status.value = new_status.value;
                value_changed = true;
            }
        } else {
            // Toggle button and lock status
            if (new_status.value && !current_status.locked) {
                current_status.locked = true;
                current_status.value = !current_status.value;
                value_changed = true;
            }

            // Unlock button ready for next press
            if (!new_status.value && current_status.locked) {
                current_status.locked = false;
            }
        }

        if (!value_changed) {
            return;
        }

        if (is_configuring) {
            controller.npad_button_state.raw = NpadButton::None;
            controller.debug_pad_button_state.raw = 0;
            TriggerOnChange(ControllerTriggerType::Button, false);
            return;
        }

        switch (index) {
        case Settings::NativeButton::A:
            controller.npad_button_state.a.Assign(current_status.value);
            controller.debug_pad_button_state.a.Assign(current_status.value);
            break;
        case Settings::NativeButton::B:
            controller.npad_button_state.b.Assign(current_status.value);
            controller.debug_pad_button_state.b.Assign(current_status.value);
            break;
        case Settings::NativeButton::X:
            controller.npad_button_state.x.Assign(current_status.value);
            controller.debug_pad_button_state.x.Assign(current_status.value);
            break;
        case Settings::NativeButton::Y:
            controller.npad_button_state.y.Assign(current_status.value);
            controller.debug_pad_button_state.y.Assign(current_status.value);
            break;
        case Settings::NativeButton::LStick:
            controller.npad_button_state.stick_l.Assign(current_status.value);
            break;
        case Settings::NativeButton::RStick:
            controller.npad_button_state.stick_r.Assign(current_status.value);
            break;
        case Settings::NativeButton::L:
            controller.npad_button_state.l.Assign(current_status.value);
            controller.debug_pad_button_state.l.Assign(current_status.value);
            break;
        case Settings::NativeButton::R:
            controller.npad_button_state.r.Assign(current_status.value);
            controller.debug_pad_button_state.r.Assign(current_status.value);
            break;
        case Settings::NativeButton::ZL:
            controller.npad_button_state.zl.Assign(current_status.value);
            controller.debug_pad_button_state.zl.Assign(current_status.value);
            break;
        case Settings::NativeButton::ZR:
            controller.npad_button_state.zr.Assign(current_status.value);
            controller.debug_pad_button_state.zr.Assign(current_status.value);
            break;
        case Settings::NativeButton::Plus:
            controller.npad_button_state.plus.Assign(current_status.value);
            controller.debug_pad_button_state.plus.Assign(current_status.value);
            break;
        case Settings::NativeButton::Minus:
            controller.npad_button_state.minus.Assign(current_status.value);
            controller.debug_pad_button_state.minus.Assign(current_status.value);
            break;
        case Settings::NativeButton::DLeft:
            controller.npad_button_state.left.Assign(current_status.value);
            controller.debug_pad_button_state.d_left.Assign(current_status.value);
            break;
        case Settings::NativeButton::DUp:
            controller.npad_button_state.up.Assign(current_status.value);
            controller.debug_pad_button_state.d_up.Assign(current_status.value);
            break;
        case Settings::NativeButton::DRight:
            controller.npad_button_state.right.Assign(current_status.value);
            controller.debug_pad_button_state.d_right.Assign(current_status.value);
            break;
        case Settings::NativeButton::DDown:
            controller.npad_button_state.down.Assign(current_status.value);
            controller.debug_pad_button_state.d_down.Assign(current_status.value);
            break;
        case Settings::NativeButton::SL:
            controller.npad_button_state.left_sl.Assign(current_status.value);
            controller.npad_button_state.right_sl.Assign(current_status.value);
            break;
        case Settings::NativeButton::SR:
            controller.npad_button_state.left_sr.Assign(current_status.value);
            controller.npad_button_state.right_sr.Assign(current_status.value);
            break;
        case Settings::NativeButton::Home:
            if (!system_buttons_enabled) {
                break;
            }
            controller.home_button_state.home.Assign(current_status.value);
            break;
        case Settings::NativeButton::Screenshot:
            if (!system_buttons_enabled) {
                break;
            }
            controller.capture_button_state.capture.Assign(current_status.value);
            break;
        }
    }
    if (!is_connected) {
        if (npad_id_type == NpadIdType::Player1 && npad_type != NpadStyleIndex::Handheld) {
            Connect();
        }
        if (npad_id_type == NpadIdType::Handheld && npad_type == NpadStyleIndex::Handheld) {
            Connect();
        }
    }
    TriggerOnChange(ControllerTriggerType::Button, true);
}

void EmulatedController::SetStick(const Common::Input::CallbackStatus& callback, std::size_t index,
                                  Common::UUID uuid) {
    if (index >= controller.stick_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
    const auto stick_value = TransformToStick(callback);

    // Only read stick values that have the same uuid or are over the threshold to avoid flapping
    if (controller.stick_values[index].uuid != uuid) {
        if (!stick_value.down && !stick_value.up && !stick_value.left && !stick_value.right) {
            return;
        }
    }

    controller.stick_values[index] = stick_value;
    controller.stick_values[index].uuid = uuid;

    if (is_configuring) {
        controller.analog_stick_state.left = {};
        controller.analog_stick_state.right = {};
        TriggerOnChange(ControllerTriggerType::Stick, false);
        return;
    }

    const AnalogStickState stick{
        .x = static_cast<s32>(controller.stick_values[index].x.value * HID_JOYSTICK_MAX),
        .y = static_cast<s32>(controller.stick_values[index].y.value * HID_JOYSTICK_MAX),
    };

    switch (index) {
    case Settings::NativeAnalog::LStick:
        controller.analog_stick_state.left = stick;
        controller.npad_button_state.stick_l_left.Assign(controller.stick_values[index].left);
        controller.npad_button_state.stick_l_up.Assign(controller.stick_values[index].up);
        controller.npad_button_state.stick_l_right.Assign(controller.stick_values[index].right);
        controller.npad_button_state.stick_l_down.Assign(controller.stick_values[index].down);
        break;
    case Settings::NativeAnalog::RStick:
        controller.analog_stick_state.right = stick;
        controller.npad_button_state.stick_r_left.Assign(controller.stick_values[index].left);
        controller.npad_button_state.stick_r_up.Assign(controller.stick_values[index].up);
        controller.npad_button_state.stick_r_right.Assign(controller.stick_values[index].right);
        controller.npad_button_state.stick_r_down.Assign(controller.stick_values[index].down);
        break;
    }

    TriggerOnChange(ControllerTriggerType::Stick, true);
}

void EmulatedController::SetTrigger(const Common::Input::CallbackStatus& callback,
                                    std::size_t index, Common::UUID uuid) {
    if (index >= controller.trigger_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
    const auto trigger_value = TransformToTrigger(callback);

    // Only read trigger values that have the same uuid or are pressed once
    if (controller.trigger_values[index].uuid != uuid) {
        if (!trigger_value.pressed.value) {
            return;
        }
    }

    controller.trigger_values[index] = trigger_value;
    controller.trigger_values[index].uuid = uuid;

    if (is_configuring) {
        controller.gc_trigger_state.left = 0;
        controller.gc_trigger_state.right = 0;
        TriggerOnChange(ControllerTriggerType::Trigger, false);
        return;
    }

    const auto& trigger = controller.trigger_values[index];

    switch (index) {
    case Settings::NativeTrigger::LTrigger:
        controller.gc_trigger_state.left = static_cast<s32>(trigger.analog.value * HID_TRIGGER_MAX);
        controller.npad_button_state.zl.Assign(trigger.pressed.value);
        break;
    case Settings::NativeTrigger::RTrigger:
        controller.gc_trigger_state.right =
            static_cast<s32>(trigger.analog.value * HID_TRIGGER_MAX);
        controller.npad_button_state.zr.Assign(trigger.pressed.value);
        break;
    }

    TriggerOnChange(ControllerTriggerType::Trigger, true);
}

void EmulatedController::SetMotion(const Common::Input::CallbackStatus& callback,
                                   std::size_t index) {
    if (index >= controller.motion_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
    auto& raw_status = controller.motion_values[index].raw_status;
    auto& emulated = controller.motion_values[index].emulated;

    raw_status = TransformToMotion(callback);
    emulated.SetAcceleration(Common::Vec3f{
        raw_status.accel.x.value,
        raw_status.accel.y.value,
        raw_status.accel.z.value,
    });
    emulated.SetGyroscope(Common::Vec3f{
        raw_status.gyro.x.value,
        raw_status.gyro.y.value,
        raw_status.gyro.z.value,
    });
    emulated.UpdateRotation(raw_status.delta_timestamp);
    emulated.UpdateOrientation(raw_status.delta_timestamp);
    force_update_motion = raw_status.force_update;

    if (is_configuring) {
        TriggerOnChange(ControllerTriggerType::Motion, false);
        return;
    }

    auto& motion = controller.motion_state[index];
    motion.accel = emulated.GetAcceleration();
    motion.gyro = emulated.GetGyroscope();
    motion.rotation = emulated.GetRotations();
    motion.orientation = emulated.GetOrientation();
    motion.is_at_rest = !emulated.IsMoving(motion_sensitivity);

    TriggerOnChange(ControllerTriggerType::Motion, true);
}

void EmulatedController::SetBattery(const Common::Input::CallbackStatus& callback,
                                    std::size_t index) {
    if (index >= controller.battery_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
    controller.battery_values[index] = TransformToBattery(callback);

    if (is_configuring) {
        TriggerOnChange(ControllerTriggerType::Battery, false);
        return;
    }

    bool is_charging = false;
    bool is_powered = false;
    NpadBatteryLevel battery_level = 0;
    switch (controller.battery_values[index]) {
    case Common::Input::BatteryLevel::Charging:
        is_charging = true;
        is_powered = true;
        battery_level = 6;
        break;
    case Common::Input::BatteryLevel::Medium:
        battery_level = 6;
        break;
    case Common::Input::BatteryLevel::Low:
        battery_level = 4;
        break;
    case Common::Input::BatteryLevel::Critical:
        battery_level = 2;
        break;
    case Common::Input::BatteryLevel::Empty:
        battery_level = 0;
        break;
    case Common::Input::BatteryLevel::None:
    case Common::Input::BatteryLevel::Full:
    default:
        is_powered = true;
        battery_level = 8;
        break;
    }

    switch (index) {
    case LeftIndex:
        controller.battery_state.left = {
            .is_powered = is_powered,
            .is_charging = is_charging,
            .battery_level = battery_level,
        };
        break;
    case RightIndex:
        controller.battery_state.right = {
            .is_powered = is_powered,
            .is_charging = is_charging,
            .battery_level = battery_level,
        };
        break;
    case DualIndex:
        controller.battery_state.dual = {
            .is_powered = is_powered,
            .is_charging = is_charging,
            .battery_level = battery_level,
        };
        break;
    }
    TriggerOnChange(ControllerTriggerType::Battery, true);
}

bool EmulatedController::SetVibration(std::size_t device_index, VibrationValue vibration) {
    if (device_index >= output_devices.size()) {
        return false;
    }
    if (!output_devices[device_index]) {
        return false;
    }
    const auto player_index = NpadIdTypeToIndex(npad_id_type);
    const auto& player = Settings::values.players.GetValue()[player_index];
    const f32 strength = static_cast<f32>(player.vibration_strength) / 100.0f;

    if (!player.vibration_enabled) {
        return false;
    }

    // Exponential amplification is too strong at low amplitudes. Switch to a linear
    // amplification if strength is set below 0.7f
    const Common::Input::VibrationAmplificationType type =
        strength > 0.7f ? Common::Input::VibrationAmplificationType::Exponential
                        : Common::Input::VibrationAmplificationType::Linear;

    const Common::Input::VibrationStatus status = {
        .low_amplitude = std::min(vibration.low_amplitude * strength, 1.0f),
        .low_frequency = vibration.low_frequency,
        .high_amplitude = std::min(vibration.high_amplitude * strength, 1.0f),
        .high_frequency = vibration.high_frequency,
        .type = type,
    };
    return output_devices[device_index]->SetVibration(status) ==
           Common::Input::VibrationError::None;
}

bool EmulatedController::TestVibration(std::size_t device_index) {
    static constexpr VibrationValue test_vibration = {
        .low_amplitude = 0.001f,
        .low_frequency = 160.0f,
        .high_amplitude = 0.001f,
        .high_frequency = 320.0f,
    };

    // Send a slight vibration to test for rumble support
    SetVibration(device_index, test_vibration);

    // Stop any vibration and return the result
    return SetVibration(device_index, DEFAULT_VIBRATION_VALUE);
}

void EmulatedController::SetLedPattern() {
    for (auto& device : output_devices) {
        if (!device) {
            continue;
        }

        const LedPattern pattern = GetLedPattern();
        const Common::Input::LedStatus status = {
            .led_1 = pattern.position1 != 0,
            .led_2 = pattern.position2 != 0,
            .led_3 = pattern.position3 != 0,
            .led_4 = pattern.position4 != 0,
        };
        device->SetLED(status);
    }
}

void EmulatedController::SetSupportedNpadStyleTag(NpadStyleTag supported_styles) {
    supported_style_tag = supported_styles;
    if (!is_connected) {
        return;
    }
    if (!IsControllerSupported()) {
        LOG_ERROR(Service_HID, "Controller type {} is not supported. Disconnecting controller",
                  npad_type);
        Disconnect();
    }
}

bool EmulatedController::IsControllerSupported(bool use_temporary_value) const {
    const auto type = is_configuring && use_temporary_value ? tmp_npad_type : npad_type;
    switch (type) {
    case NpadStyleIndex::ProController:
        return supported_style_tag.fullkey;
    case NpadStyleIndex::Handheld:
        return supported_style_tag.handheld;
    case NpadStyleIndex::JoyconDual:
        return supported_style_tag.joycon_dual;
    case NpadStyleIndex::JoyconLeft:
        return supported_style_tag.joycon_left;
    case NpadStyleIndex::JoyconRight:
        return supported_style_tag.joycon_right;
    case NpadStyleIndex::GameCube:
        return supported_style_tag.gamecube;
    case NpadStyleIndex::Pokeball:
        return supported_style_tag.palma;
    case NpadStyleIndex::NES:
        return supported_style_tag.lark;
    case NpadStyleIndex::SNES:
        return supported_style_tag.lucia;
    case NpadStyleIndex::N64:
        return supported_style_tag.lagoon;
    case NpadStyleIndex::SegaGenesis:
        return supported_style_tag.lager;
    default:
        return false;
    }
}

void EmulatedController::Connect(bool use_temporary_value) {
    if (!IsControllerSupported(use_temporary_value)) {
        const auto type = is_configuring && use_temporary_value ? tmp_npad_type : npad_type;
        LOG_ERROR(Service_HID, "Controller type {} is not supported", type);
        return;
    }
    {
        std::lock_guard lock{mutex};
        if (is_configuring) {
            tmp_is_connected = true;
            TriggerOnChange(ControllerTriggerType::Connected, false);
            return;
        }

        if (is_connected) {
            return;
        }
        is_connected = true;
    }
    TriggerOnChange(ControllerTriggerType::Connected, true);
}

void EmulatedController::Disconnect() {
    {
        std::lock_guard lock{mutex};
        if (is_configuring) {
            tmp_is_connected = false;
            TriggerOnChange(ControllerTriggerType::Disconnected, false);
            return;
        }

        if (!is_connected) {
            return;
        }
        is_connected = false;
    }
    TriggerOnChange(ControllerTriggerType::Disconnected, true);
}

bool EmulatedController::IsConnected(bool get_temporary_value) const {
    if (get_temporary_value && is_configuring) {
        return tmp_is_connected;
    }
    return is_connected;
}

bool EmulatedController::IsVibrationEnabled() const {
    const auto player_index = NpadIdTypeToIndex(npad_id_type);
    const auto& player = Settings::values.players.GetValue()[player_index];
    return player.vibration_enabled;
}

NpadIdType EmulatedController::GetNpadIdType() const {
    return npad_id_type;
}

NpadStyleIndex EmulatedController::GetNpadStyleIndex(bool get_temporary_value) const {
    if (get_temporary_value && is_configuring) {
        return tmp_npad_type;
    }
    return npad_type;
}

void EmulatedController::SetNpadStyleIndex(NpadStyleIndex npad_type_) {
    {
        std::lock_guard lock{mutex};

        if (is_configuring) {
            if (tmp_npad_type == npad_type_) {
                return;
            }
            tmp_npad_type = npad_type_;
            TriggerOnChange(ControllerTriggerType::Type, false);
            return;
        }

        if (npad_type == npad_type_) {
            return;
        }
        if (is_connected) {
            LOG_WARNING(Service_HID, "Controller {} type changed while it's connected",
                        NpadIdTypeToIndex(npad_id_type));
        }
        npad_type = npad_type_;
    }
    TriggerOnChange(ControllerTriggerType::Type, true);
}

LedPattern EmulatedController::GetLedPattern() const {
    switch (npad_id_type) {
    case NpadIdType::Player1:
        return LedPattern{1, 0, 0, 0};
    case NpadIdType::Player2:
        return LedPattern{1, 1, 0, 0};
    case NpadIdType::Player3:
        return LedPattern{1, 1, 1, 0};
    case NpadIdType::Player4:
        return LedPattern{1, 1, 1, 1};
    case NpadIdType::Player5:
        return LedPattern{1, 0, 0, 1};
    case NpadIdType::Player6:
        return LedPattern{1, 0, 1, 0};
    case NpadIdType::Player7:
        return LedPattern{1, 0, 1, 1};
    case NpadIdType::Player8:
        return LedPattern{0, 1, 1, 0};
    default:
        return LedPattern{0, 0, 0, 0};
    }
}

ButtonValues EmulatedController::GetButtonsValues() const {
    return controller.button_values;
}

SticksValues EmulatedController::GetSticksValues() const {
    return controller.stick_values;
}

TriggerValues EmulatedController::GetTriggersValues() const {
    return controller.trigger_values;
}

ControllerMotionValues EmulatedController::GetMotionValues() const {
    return controller.motion_values;
}

ColorValues EmulatedController::GetColorsValues() const {
    return controller.color_values;
}

BatteryValues EmulatedController::GetBatteryValues() const {
    return controller.battery_values;
}

HomeButtonState EmulatedController::GetHomeButtons() const {
    if (is_configuring) {
        return {};
    }
    return controller.home_button_state;
}

CaptureButtonState EmulatedController::GetCaptureButtons() const {
    if (is_configuring) {
        return {};
    }
    return controller.capture_button_state;
}

NpadButtonState EmulatedController::GetNpadButtons() const {
    if (is_configuring) {
        return {};
    }
    return controller.npad_button_state;
}

DebugPadButton EmulatedController::GetDebugPadButtons() const {
    if (is_configuring) {
        return {};
    }
    return controller.debug_pad_button_state;
}

AnalogSticks EmulatedController::GetSticks() const {
    if (is_configuring) {
        return {};
    }
    // Some drivers like stick from buttons need constant refreshing
    for (auto& device : stick_devices) {
        if (!device) {
            continue;
        }
        device->SoftUpdate();
    }
    return controller.analog_stick_state;
}

NpadGcTriggerState EmulatedController::GetTriggers() const {
    if (is_configuring) {
        return {};
    }
    return controller.gc_trigger_state;
}

MotionState EmulatedController::GetMotions() const {
    if (force_update_motion) {
        for (auto& device : motion_devices) {
            if (!device) {
                continue;
            }
            device->ForceUpdate();
        }
    }
    return controller.motion_state;
}

ControllerColors EmulatedController::GetColors() const {
    return controller.colors_state;
}

BatteryLevelState EmulatedController::GetBattery() const {
    return controller.battery_state;
}

void EmulatedController::TriggerOnChange(ControllerTriggerType type, bool is_npad_service_update) {
    for (const auto& poller_pair : callback_list) {
        const ControllerUpdateCallback& poller = poller_pair.second;
        if (!is_npad_service_update && poller.is_npad_service) {
            continue;
        }
        if (poller.on_change) {
            poller.on_change(type);
        }
    }
}

int EmulatedController::SetCallback(ControllerUpdateCallback update_callback) {
    std::lock_guard lock{mutex};
    callback_list.insert_or_assign(last_callback_key, std::move(update_callback));
    return last_callback_key++;
}

void EmulatedController::DeleteCallback(int key) {
    std::lock_guard lock{mutex};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}
} // namespace Core::HID
