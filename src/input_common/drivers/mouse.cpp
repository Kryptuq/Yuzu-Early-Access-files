// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include <stop_token>
#include <thread>
#include <fmt/format.h>

#include "common/param_package.h"
#include "common/settings.h"
#include "common/thread.h"
#include "input_common/drivers/mouse.h"

namespace InputCommon {
constexpr int mouse_axis_x = 0;
constexpr int mouse_axis_y = 1;
constexpr int wheel_axis_x = 2;
constexpr int wheel_axis_y = 3;
constexpr int motion_wheel_y = 4;
constexpr int touch_axis_x = 10;
constexpr int touch_axis_y = 11;
constexpr PadIdentifier identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 0,
    .pad = 0,
};

Mouse::Mouse(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    PreSetController(identifier);
    PreSetAxis(identifier, mouse_axis_x);
    PreSetAxis(identifier, mouse_axis_y);
    PreSetAxis(identifier, wheel_axis_x);
    PreSetAxis(identifier, wheel_axis_y);
    PreSetAxis(identifier, motion_wheel_y);
    PreSetAxis(identifier, touch_axis_x);
    PreSetAxis(identifier, touch_axis_y);
    update_thread = std::jthread([this](std::stop_token stop_token) { UpdateThread(stop_token); });
}

void Mouse::UpdateThread(std::stop_token stop_token) {
    Common::SetCurrentThreadName("yuzu:input:Mouse");
    constexpr int update_time = 10;
    while (!stop_token.stop_requested()) {
        if (Settings::values.mouse_panning && !Settings::values.mouse_enabled) {
            // Slow movement by 4%
            last_mouse_change *= 0.96f;
            const float sensitivity =
                Settings::values.mouse_panning_sensitivity.GetValue() * 0.022f;
            SetAxis(identifier, mouse_axis_x, last_mouse_change.x * sensitivity);
            SetAxis(identifier, mouse_axis_y, -last_mouse_change.y * sensitivity);
        }

        SetAxis(identifier, motion_wheel_y, 0.0f);

        if (mouse_panning_timout++ > 20) {
            StopPanning();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(update_time));
    }
}

void Mouse::MouseMove(int x, int y, f32 touch_x, f32 touch_y, int center_x, int center_y) {
    // If native mouse is enabled just set the screen coordinates
    if (Settings::values.mouse_enabled) {
        SetAxis(identifier, mouse_axis_x, touch_x);
        SetAxis(identifier, mouse_axis_y, touch_y);
        return;
    }

    SetAxis(identifier, touch_axis_x, touch_x);
    SetAxis(identifier, touch_axis_y, touch_y);

    if (Settings::values.mouse_panning) {
        auto mouse_change =
            (Common::MakeVec(x, y) - Common::MakeVec(center_x, center_y)).Cast<float>();
        mouse_panning_timout = 0;

        const auto move_distance = mouse_change.Length();
        if (move_distance == 0) {
            return;
        }

        // Make slow movements at least 3 units on lenght
        if (move_distance < 3.0f) {
            // Normalize value
            mouse_change /= move_distance;
            mouse_change *= 3.0f;
        }

        // Average mouse movements
        last_mouse_change = (last_mouse_change * 0.91f) + (mouse_change * 0.09f);

        const auto last_move_distance = last_mouse_change.Length();

        // Make fast movements clamp to 8 units on lenght
        if (last_move_distance > 8.0f) {
            // Normalize value
            last_mouse_change /= last_move_distance;
            last_mouse_change *= 8.0f;
        }

        // Ignore average if it's less than 1 unit and use current movement value
        if (last_move_distance < 1.0f) {
            last_mouse_change = mouse_change / mouse_change.Length();
        }

        return;
    }

    if (button_pressed) {
        const auto mouse_move = Common::MakeVec<int>(x, y) - mouse_origin;
        const float sensitivity = Settings::values.mouse_panning_sensitivity.GetValue() * 0.0012f;
        SetAxis(identifier, mouse_axis_x, static_cast<float>(mouse_move.x) * sensitivity);
        SetAxis(identifier, mouse_axis_y, static_cast<float>(-mouse_move.y) * sensitivity);
    }
}

void Mouse::PressButton(int x, int y, f32 touch_x, f32 touch_y, MouseButton button) {
    SetAxis(identifier, touch_axis_x, touch_x);
    SetAxis(identifier, touch_axis_y, touch_y);
    SetButton(identifier, static_cast<int>(button), true);
    // Set initial analog parameters
    mouse_origin = {x, y};
    last_mouse_position = {x, y};
    button_pressed = true;
}

void Mouse::ReleaseButton(MouseButton button) {
    SetButton(identifier, static_cast<int>(button), false);

    if (!Settings::values.mouse_panning && !Settings::values.mouse_enabled) {
        SetAxis(identifier, mouse_axis_x, 0);
        SetAxis(identifier, mouse_axis_y, 0);
    }
    button_pressed = false;
}

void Mouse::MouseWheelChange(int x, int y) {
    wheel_position.x += x;
    wheel_position.y += y;
    SetAxis(identifier, wheel_axis_x, static_cast<f32>(wheel_position.x));
    SetAxis(identifier, wheel_axis_y, static_cast<f32>(wheel_position.y));
    SetAxis(identifier, motion_wheel_y, static_cast<f32>(y) / 100.0f);
}

void Mouse::ReleaseAllButtons() {
    ResetButtonState();
    button_pressed = false;
}

void Mouse::StopPanning() {
    last_mouse_change = {};
}

std::vector<Common::ParamPackage> Mouse::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices;
    devices.emplace_back(Common::ParamPackage{
        {"engine", GetEngineName()},
        {"display", "Keyboard/Mouse"},
    });
    return devices;
}

AnalogMapping Mouse::GetAnalogMappingForDevice(
    [[maybe_unused]] const Common::ParamPackage& params) {
    // Only overwrite different buttons from default
    AnalogMapping mapping = {};
    Common::ParamPackage right_analog_params;
    right_analog_params.Set("engine", GetEngineName());
    right_analog_params.Set("axis_x", 0);
    right_analog_params.Set("axis_y", 1);
    right_analog_params.Set("threshold", 0.5f);
    right_analog_params.Set("range", 1.0f);
    right_analog_params.Set("deadzone", 0.0f);
    mapping.insert_or_assign(Settings::NativeAnalog::RStick, std::move(right_analog_params));
    return mapping;
}

Common::Input::ButtonNames Mouse::GetUIButtonName(const Common::ParamPackage& params) const {
    const auto button = static_cast<MouseButton>(params.Get("button", 0));
    switch (button) {
    case MouseButton::Left:
        return Common::Input::ButtonNames::ButtonLeft;
    case MouseButton::Right:
        return Common::Input::ButtonNames::ButtonRight;
    case MouseButton::Wheel:
        return Common::Input::ButtonNames::ButtonMouseWheel;
    case MouseButton::Backward:
        return Common::Input::ButtonNames::ButtonBackward;
    case MouseButton::Forward:
        return Common::Input::ButtonNames::ButtonForward;
    case MouseButton::Task:
        return Common::Input::ButtonNames::ButtonTask;
    case MouseButton::Extra:
        return Common::Input::ButtonNames::ButtonExtra;
    case MouseButton::Undefined:
    default:
        return Common::Input::ButtonNames::Undefined;
    }
}

Common::Input::ButtonNames Mouse::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        return GetUIButtonName(params);
    }
    if (params.Has("axis")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("axis_x") && params.Has("axis_y") && params.Has("axis_z")) {
        return Common::Input::ButtonNames::Engine;
    }

    return Common::Input::ButtonNames::Invalid;
}

} // namespace InputCommon
