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
constexpr int touch_axis_x = 10;
constexpr int touch_axis_y = 11;
constexpr PadIdentifier identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 0,
    .pad = 0,
};

Mouse::Mouse(const std::string input_engine_) : InputEngine(input_engine_) {
    PreSetController(identifier);
    update_thread = std::jthread([this](std::stop_token stop_token) { UpdateThread(stop_token); });
}

void Mouse::UpdateThread(std::stop_token stop_token) {
    Common::SetCurrentThreadName("yuzu:input:Mouse");
    constexpr int update_time = 10;
    while (!stop_token.stop_requested()) {
        if (Settings::values.mouse_panning) {
            // Slow movement by 4%
            last_mouse_change *= 0.96f;
            const float sensitivity =
                Settings::values.mouse_panning_sensitivity.GetValue() * 0.022f;
            SetAxis(identifier, 0, last_mouse_change.x * sensitivity);
            SetAxis(identifier, 1, -last_mouse_change.y * sensitivity);
        }

        if (mouse_panning_timout++ > 20) {
            StopPanning();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(update_time));
    }
}

void Mouse::MouseMove(int x, int y, f32 touch_x, f32 touch_y, int center_x, int center_y) {
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
        SetAxis(identifier, 0, static_cast<float>(mouse_move.x) * sensitivity);
        SetAxis(identifier, 1, static_cast<float>(-mouse_move.y) * sensitivity);
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

    if (!Settings::values.mouse_panning) {
        SetAxis(identifier, 0, 0);
        SetAxis(identifier, 1, 0);
    }
    button_pressed = false;
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

std::string Mouse::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        return fmt::format("Mouse {}", params.Get("button", 0));
    }

    return "Bad Mouse";
}

} // namespace InputCommon
