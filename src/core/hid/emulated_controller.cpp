// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/thread.h"
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

    controller.colors_state.fullkey = {
        .body = GetNpadColor(player.body_color_left),
        .button = GetNpadColor(player.button_color_left),
    };
    controller.colors_state.left = {
        .body = GetNpadColor(player.body_color_left),
        .button = GetNpadColor(player.button_color_left),
    };
    controller.colors_state.right = {
        .body = GetNpadColor(player.body_color_right),
        .button = GetNpadColor(player.button_color_right),
    };

    // Other or debug controller should always be a pro controller
    if (npad_id_type != NpadIdType::Other) {
        SetNpadStyleIndex(MapSettingsTypeToNPad(player.controller_type));
        original_npad_type = npad_type;
    } else {
        SetNpadStyleIndex(NpadStyleIndex::ProController);
        original_npad_type = npad_type;
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

    camera_params = Common::ParamPackage{"engine:camera,camera:1"};
    nfc_params = Common::ParamPackage{"engine:virtual_amiibo,nfc:1"};

    output_params[LeftIndex] = left_joycon;
    output_params[RightIndex] = right_joycon;
    output_params[2] = camera_params;
    output_params[3] = nfc_params;
    output_params[LeftIndex].Set("output", true);
    output_params[RightIndex].Set("output", true);
    output_params[2].Set("output", true);
    output_params[3].Set("output", true);

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
    std::transform(battery_params.begin(), battery_params.end(), battery_devices.begin(),
                   Common::Input::CreateDevice<Common::Input::InputDevice>);
    camera_devices = Common::Input::CreateDevice<Common::Input::InputDevice>(camera_params);
    nfc_devices = Common::Input::CreateDevice<Common::Input::InputDevice>(nfc_params);
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

    if (camera_devices) {
        camera_devices->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback) { SetCamera(callback); },
        });
        camera_devices->ForceUpdate();
    }

    if (nfc_devices) {
        if (npad_id_type == NpadIdType::Handheld || npad_id_type == NpadIdType::Player1) {
            nfc_devices->SetCallback({
                .on_change =
                    [this](const Common::Input::CallbackStatus& callback) { SetNfc(callback); },
            });
            nfc_devices->ForceUpdate();
        }
    }

    // Use a common UUID for TAS
    static constexpr Common::UUID TAS_UUID = Common::UUID{
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0xA5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

    // Register TAS devices. No need to force update
    for (std::size_t index = 0; index < tas_button_devices.size(); ++index) {
        if (!tas_button_devices[index]) {
            continue;
        }
        tas_button_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetButton(callback, index, TAS_UUID);
                },
        });
    }

    for (std::size_t index = 0; index < tas_stick_devices.size(); ++index) {
        if (!tas_stick_devices[index]) {
            continue;
        }
        tas_stick_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetStick(callback, index, TAS_UUID);
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
    camera_devices.reset();
    nfc_devices.reset();
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
        original_npad_type = tmp_npad_type;
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
    std::scoped_lock lock{mutex};
    system_buttons_enabled = true;
}

void EmulatedController::DisableSystemButtons() {
    std::scoped_lock lock{mutex};
    system_buttons_enabled = false;
}

void EmulatedController::ResetSystemButtons() {
    std::scoped_lock lock{mutex};
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
                       param.Get("port", 0) == param_.Get("port", 0) &&
                       param.Get("pad", 0) == param_.Get("pad", 0);
            });
        if (devices_it != devices.end()) {
            continue;
        }
        Common::ParamPackage device{};
        device.Set("engine", param.Get("engine", ""));
        device.Set("guid", param.Get("guid", ""));
        device.Set("port", param.Get("port", 0));
        device.Set("pad", param.Get("pad", 0));
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
                       param.Get("port", 0) == param_.Get("port", 0) &&
                       param.Get("pad", 0) == param_.Get("pad", 0);
            });
        if (devices_it != devices.end()) {
            continue;
        }
        Common::ParamPackage device{};
        device.Set("engine", param.Get("engine", ""));
        device.Set("guid", param.Get("guid", ""));
        device.Set("port", param.Get("port", 0));
        device.Set("pad", param.Get("pad", 0));
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
    std::unique_lock lock{mutex};
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
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::Button, false);
        return;
    }

    // GC controllers have triggers not buttons
    if (npad_type == NpadStyleIndex::GameCube) {
        if (index == Settings::NativeButton::ZR) {
            return;
        }
        if (index == Settings::NativeButton::ZL) {
            return;
        }
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

    lock.unlock();

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
    std::unique_lock lock{mutex};
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
        lock.unlock();
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

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Stick, true);
}

void EmulatedController::SetTrigger(const Common::Input::CallbackStatus& callback,
                                    std::size_t index, Common::UUID uuid) {
    if (index >= controller.trigger_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
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
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::Trigger, false);
        return;
    }

    // Only GC controllers have analog triggers
    if (npad_type != NpadStyleIndex::GameCube) {
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

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Trigger, true);
}

void EmulatedController::SetMotion(const Common::Input::CallbackStatus& callback,
                                   std::size_t index) {
    if (index >= controller.motion_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
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
    emulated.SetGyroThreshold(raw_status.gyro.x.properties.threshold);
    emulated.UpdateRotation(raw_status.delta_timestamp);
    emulated.UpdateOrientation(raw_status.delta_timestamp);
    force_update_motion = raw_status.force_update;

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::Motion, false);
        return;
    }

    auto& motion = controller.motion_state[index];
    motion.accel = emulated.GetAcceleration();
    motion.gyro = emulated.GetGyroscope();
    motion.rotation = emulated.GetRotations();
    motion.orientation = emulated.GetOrientation();
    motion.is_at_rest = !emulated.IsMoving(motion_sensitivity);

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Motion, true);
}

void EmulatedController::SetBattery(const Common::Input::CallbackStatus& callback,
                                    std::size_t index) {
    if (index >= controller.battery_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
    controller.battery_values[index] = TransformToBattery(callback);

    if (is_configuring) {
        lock.unlock();
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

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Battery, true);
}

void EmulatedController::SetCamera(const Common::Input::CallbackStatus& callback) {
    std::unique_lock lock{mutex};
    controller.camera_values = TransformToCamera(callback);

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::IrSensor, false);
        return;
    }

    controller.camera_state.sample++;
    controller.camera_state.format =
        static_cast<Core::IrSensor::ImageTransferProcessorFormat>(controller.camera_values.format);
    controller.camera_state.data = controller.camera_values.data;

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::IrSensor, true);
}

void EmulatedController::SetNfc(const Common::Input::CallbackStatus& callback) {
    std::unique_lock lock{mutex};
    controller.nfc_values = TransformToNfc(callback);

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::Nfc, false);
        return;
    }

    controller.nfc_state = {
        controller.nfc_values.state,
        controller.nfc_values.data,
    };

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Nfc, true);
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

bool EmulatedController::IsVibrationEnabled(std::size_t device_index) {
    const auto player_index = NpadIdTypeToIndex(npad_id_type);
    const auto& player = Settings::values.players.GetValue()[player_index];

    if (!player.vibration_enabled) {
        return false;
    }

    if (device_index >= output_devices.size()) {
        return false;
    }

    if (!output_devices[device_index]) {
        return false;
    }

    return output_devices[device_index]->IsVibrationEnabled();
}

bool EmulatedController::SetPollingMode(Common::Input::PollingMode polling_mode) {
    LOG_INFO(Service_HID, "Set polling mode {}", polling_mode);
    auto& output_device = output_devices[static_cast<std::size_t>(DeviceIndex::Right)];
    auto& nfc_output_device = output_devices[3];

    const auto virtual_nfc_result = nfc_output_device->SetPollingMode(polling_mode);
    const auto mapped_nfc_result = output_device->SetPollingMode(polling_mode);

    return virtual_nfc_result == Common::Input::PollingError::None ||
           mapped_nfc_result == Common::Input::PollingError::None;
}

bool EmulatedController::SetCameraFormat(
    Core::IrSensor::ImageTransferProcessorFormat camera_format) {
    LOG_INFO(Service_HID, "Set camera format {}", camera_format);

    auto& right_output_device = output_devices[static_cast<std::size_t>(DeviceIndex::Right)];
    auto& camera_output_device = output_devices[2];

    if (right_output_device->SetCameraFormat(static_cast<Common::Input::CameraFormat>(
            camera_format)) == Common::Input::CameraError::None) {
        return true;
    }

    // Fallback to Qt camera if native device doesn't have support
    return camera_output_device->SetCameraFormat(static_cast<Common::Input::CameraFormat>(
               camera_format)) == Common::Input::CameraError::None;
}

bool EmulatedController::HasNfc() const {
    const auto& nfc_output_device = output_devices[3];

    switch (npad_type) {
    case NpadStyleIndex::JoyconRight:
    case NpadStyleIndex::JoyconDual:
    case NpadStyleIndex::ProController:
        break;
    default:
        return false;
    }

    const bool has_virtual_nfc =
        npad_id_type == NpadIdType::Player1 || npad_id_type == NpadIdType::Handheld;
    const bool is_virtual_nfc_supported =
        nfc_output_device->SupportsNfc() != Common::Input::NfcState::NotSupported;

    return is_connected && (has_virtual_nfc && is_virtual_nfc_supported);
}

bool EmulatedController::WriteNfc(const std::vector<u8>& data) {
    auto& nfc_output_device = output_devices[3];

    return nfc_output_device->WriteNfcData(data) == Common::Input::NfcState::Success;
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

    // Attempt to reconnect with the original type
    if (npad_type != original_npad_type) {
        Disconnect();
        const auto current_npad_type = npad_type;
        SetNpadStyleIndex(original_npad_type);
        if (IsControllerSupported()) {
            Connect();
            return;
        }
        SetNpadStyleIndex(current_npad_type);
        Connect();
    }

    if (IsControllerSupported()) {
        return;
    }

    Disconnect();

    // Fallback Fullkey controllers to Pro controllers
    if (IsControllerFullkey() && supported_style_tag.fullkey) {
        LOG_WARNING(Service_HID, "Reconnecting controller type {} as Pro controller", npad_type);
        SetNpadStyleIndex(NpadStyleIndex::ProController);
        Connect();
        return;
    }

    // Fallback Dual joycon controllers to Pro controllers
    if (npad_type == NpadStyleIndex::JoyconDual && supported_style_tag.fullkey) {
        LOG_WARNING(Service_HID, "Reconnecting controller type {} as Pro controller", npad_type);
        SetNpadStyleIndex(NpadStyleIndex::ProController);
        Connect();
        return;
    }

    // Fallback Pro controllers to Dual joycon
    if (npad_type == NpadStyleIndex::ProController && supported_style_tag.joycon_dual) {
        LOG_WARNING(Service_HID, "Reconnecting controller type {} as Dual Joycons", npad_type);
        SetNpadStyleIndex(NpadStyleIndex::JoyconDual);
        Connect();
        return;
    }

    LOG_ERROR(Service_HID, "Controller type {} is not supported. Disconnecting controller",
              npad_type);
}

bool EmulatedController::IsControllerFullkey(bool use_temporary_value) const {
    std::scoped_lock lock{mutex};
    const auto type = is_configuring && use_temporary_value ? tmp_npad_type : npad_type;
    switch (type) {
    case NpadStyleIndex::ProController:
    case NpadStyleIndex::GameCube:
    case NpadStyleIndex::NES:
    case NpadStyleIndex::SNES:
    case NpadStyleIndex::N64:
    case NpadStyleIndex::SegaGenesis:
        return true;
    default:
        return false;
    }
}

bool EmulatedController::IsControllerSupported(bool use_temporary_value) const {
    std::scoped_lock lock{mutex};
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

    std::unique_lock lock{mutex};
    if (is_configuring) {
        tmp_is_connected = true;
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::Connected, false);
        return;
    }

    if (is_connected) {
        return;
    }
    is_connected = true;

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Connected, true);
}

void EmulatedController::Disconnect() {
    std::unique_lock lock{mutex};
    if (is_configuring) {
        tmp_is_connected = false;
        lock.unlock();
        TriggerOnChange(ControllerTriggerType::Disconnected, false);
        return;
    }

    if (!is_connected) {
        return;
    }
    is_connected = false;

    lock.unlock();
    TriggerOnChange(ControllerTriggerType::Disconnected, true);
}

bool EmulatedController::IsConnected(bool get_temporary_value) const {
    std::scoped_lock lock{mutex};
    if (get_temporary_value && is_configuring) {
        return tmp_is_connected;
    }
    return is_connected;
}

NpadIdType EmulatedController::GetNpadIdType() const {
    std::scoped_lock lock{mutex};
    return npad_id_type;
}

NpadStyleIndex EmulatedController::GetNpadStyleIndex(bool get_temporary_value) const {
    std::scoped_lock lock{mutex};
    if (get_temporary_value && is_configuring) {
        return tmp_npad_type;
    }
    return npad_type;
}

void EmulatedController::SetNpadStyleIndex(NpadStyleIndex npad_type_) {
    std::unique_lock lock{mutex};

    if (is_configuring) {
        if (tmp_npad_type == npad_type_) {
            return;
        }
        tmp_npad_type = npad_type_;
        lock.unlock();
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

    lock.unlock();
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
    std::scoped_lock lock{mutex};
    return controller.button_values;
}

SticksValues EmulatedController::GetSticksValues() const {
    std::scoped_lock lock{mutex};
    return controller.stick_values;
}

TriggerValues EmulatedController::GetTriggersValues() const {
    std::scoped_lock lock{mutex};
    return controller.trigger_values;
}

ControllerMotionValues EmulatedController::GetMotionValues() const {
    std::scoped_lock lock{mutex};
    return controller.motion_values;
}

ColorValues EmulatedController::GetColorsValues() const {
    std::scoped_lock lock{mutex};
    return controller.color_values;
}

BatteryValues EmulatedController::GetBatteryValues() const {
    std::scoped_lock lock{mutex};
    return controller.battery_values;
}

CameraValues EmulatedController::GetCameraValues() const {
    std::scoped_lock lock{mutex};
    return controller.camera_values;
}

HomeButtonState EmulatedController::GetHomeButtons() const {
    std::scoped_lock lock{mutex};
    if (is_configuring) {
        return {};
    }
    return controller.home_button_state;
}

CaptureButtonState EmulatedController::GetCaptureButtons() const {
    std::scoped_lock lock{mutex};
    if (is_configuring) {
        return {};
    }
    return controller.capture_button_state;
}

NpadButtonState EmulatedController::GetNpadButtons() const {
    std::scoped_lock lock{mutex};
    if (is_configuring) {
        return {};
    }
    return controller.npad_button_state;
}

DebugPadButton EmulatedController::GetDebugPadButtons() const {
    std::scoped_lock lock{mutex};
    if (is_configuring) {
        return {};
    }
    return controller.debug_pad_button_state;
}

AnalogSticks EmulatedController::GetSticks() const {
    std::unique_lock lock{mutex};

    if (is_configuring) {
        return {};
    }

    // Some drivers like stick from buttons need constant refreshing
    for (auto& device : stick_devices) {
        if (!device) {
            continue;
        }
        lock.unlock();
        device->SoftUpdate();
        lock.lock();
    }

    return controller.analog_stick_state;
}

NpadGcTriggerState EmulatedController::GetTriggers() const {
    std::scoped_lock lock{mutex};
    if (is_configuring) {
        return {};
    }
    return controller.gc_trigger_state;
}

MotionState EmulatedController::GetMotions() const {
    std::unique_lock lock{mutex};

    // Some drivers like mouse motion need constant refreshing
    if (force_update_motion) {
        for (auto& device : motion_devices) {
            if (!device) {
                continue;
            }
            lock.unlock();
            device->ForceUpdate();
            lock.lock();
        }
    }

    return controller.motion_state;
}

ControllerColors EmulatedController::GetColors() const {
    std::scoped_lock lock{mutex};
    return controller.colors_state;
}

BatteryLevelState EmulatedController::GetBattery() const {
    std::scoped_lock lock{mutex};
    return controller.battery_state;
}

const CameraState& EmulatedController::GetCamera() const {
    std::scoped_lock lock{mutex};
    return controller.camera_state;
}

const NfcState& EmulatedController::GetNfc() const {
    std::scoped_lock lock{mutex};
    return controller.nfc_state;
}

NpadColor EmulatedController::GetNpadColor(u32 color) {
    return {
        .r = static_cast<u8>((color >> 16) & 0xFF),
        .g = static_cast<u8>((color >> 8) & 0xFF),
        .b = static_cast<u8>(color & 0xFF),
        .a = 0xff,
    };
}

void EmulatedController::TriggerOnChange(ControllerTriggerType type, bool is_npad_service_update) {
    std::scoped_lock lock{callback_mutex};
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
    std::scoped_lock lock{callback_mutex};
    callback_list.insert_or_assign(last_callback_key, std::move(update_callback));
    return last_callback_key++;
}

void EmulatedController::DeleteCallback(int key) {
    std::scoped_lock lock{callback_mutex};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}
} // namespace Core::HID
