// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "input_common/input_engine.h"
#include "input_common/input_mapping.h"

namespace InputCommon {

MappingFactory::MappingFactory() = default;

void MappingFactory::BeginMapping(Polling::InputType type) {
    is_enabled = true;
    input_type = type;
    input_queue.Clear();
    first_axis = -1;
    second_axis = -1;
}

Common::ParamPackage MappingFactory::GetNextInput() {
    Common::ParamPackage input;
    input_queue.Pop(input);
    return input;
}

void MappingFactory::RegisterInput(const MappingData& data) {
    if (!is_enabled) {
        return;
    }
    if (!IsDriverValid(data)) {
        return;
    }

    switch (input_type) {
    case Polling::InputType::Button:
        RegisterButton(data);
        return;
    case Polling::InputType::Stick:
        RegisterStick(data);
        return;
    case Polling::InputType::Motion:
        RegisterMotion(data);
        return;
    default:
        return;
    }
}

void MappingFactory::StopMapping() {
    is_enabled = false;
    input_type = Polling::InputType::None;
    input_queue.Clear();
}

void MappingFactory::RegisterButton(const MappingData& data) {
    Common::ParamPackage new_input;
    new_input.Set("engine", data.engine);
    if (data.pad.guid.IsValid()) {
        new_input.Set("guid", data.pad.guid.RawString());
    }
    new_input.Set("port", static_cast<int>(data.pad.port));
    new_input.Set("pad", static_cast<int>(data.pad.pad));

    switch (data.type) {
    case EngineInputType::Button:
        // Workaround for old compatibility
        if (data.engine == "keyboard") {
            new_input.Set("code", data.index);
            break;
        }
        new_input.Set("button", data.index);
        break;
    case EngineInputType::HatButton:
        new_input.Set("hat", data.index);
        new_input.Set("direction", data.hat_name);
        break;
    case EngineInputType::Analog:
        // Ignore mouse axis when mapping buttons
        if (data.engine == "mouse" && data.index != 4) {
            return;
        }
        new_input.Set("axis", data.index);
        new_input.Set("threshold", 0.5f);
        break;
    default:
        return;
    }
    input_queue.Push(new_input);
}

void MappingFactory::RegisterStick(const MappingData& data) {
    Common::ParamPackage new_input;
    new_input.Set("engine", data.engine);
    if (data.pad.guid.IsValid()) {
        new_input.Set("guid", data.pad.guid.RawString());
    }
    new_input.Set("port", static_cast<int>(data.pad.port));
    new_input.Set("pad", static_cast<int>(data.pad.pad));

    // If engine is mouse map the mouse position as a joystick
    if (data.engine == "mouse") {
        new_input.Set("axis_x", 0);
        new_input.Set("axis_y", 1);
        new_input.Set("threshold", 0.5f);
        new_input.Set("range", 1.0f);
        new_input.Set("deadzone", 0.0f);
        input_queue.Push(new_input);
        return;
    }

    switch (data.type) {
    case EngineInputType::Button:
    case EngineInputType::HatButton:
        RegisterButton(data);
        return;
    case EngineInputType::Analog:
        if (first_axis == data.index) {
            return;
        }
        if (first_axis == -1) {
            first_axis = data.index;
            return;
        }
        new_input.Set("axis_x", first_axis);
        new_input.Set("axis_y", data.index);
        new_input.Set("threshold", 0.5f);
        new_input.Set("range", 0.95f);
        new_input.Set("deadzone", 0.15f);
        break;
    default:
        return;
    }
    input_queue.Push(new_input);
}

void MappingFactory::RegisterMotion(const MappingData& data) {
    Common::ParamPackage new_input;
    new_input.Set("engine", data.engine);
    if (data.pad.guid.IsValid()) {
        new_input.Set("guid", data.pad.guid.RawString());
    }
    new_input.Set("port", static_cast<int>(data.pad.port));
    new_input.Set("pad", static_cast<int>(data.pad.pad));

    // If engine is mouse map the mouse position as 3 axis motion
    if (data.engine == "mouse") {
        new_input.Set("axis_x", 1);
        new_input.Set("invert_x", "-");
        new_input.Set("axis_y", 0);
        new_input.Set("axis_z", 4);
        new_input.Set("range", 1.0f);
        new_input.Set("deadzone", 0.0f);
        input_queue.Push(new_input);
        return;
    }

    switch (data.type) {
    case EngineInputType::Button:
    case EngineInputType::HatButton:
        RegisterButton(data);
        return;
    case EngineInputType::Analog:
        if (first_axis == data.index) {
            return;
        }
        if (second_axis == data.index) {
            return;
        }
        if (first_axis == -1) {
            first_axis = data.index;
            return;
        }
        if (second_axis == -1) {
            second_axis = data.index;
            return;
        }
        new_input.Set("axis_x", first_axis);
        new_input.Set("axis_y", second_axis);
        new_input.Set("axis_z", data.index);
        new_input.Set("range", 1.0f);
        new_input.Set("deadzone", 0.20f);
        break;
    case EngineInputType::Motion:
        new_input.Set("motion", data.index);
        break;
    default:
        return;
    }
    input_queue.Push(new_input);
}

bool MappingFactory::IsDriverValid(const MappingData& data) const {
    // Only port 0 can be mapped on the keyboard
    if (data.engine == "keyboard" && data.pad.port != 0) {
        return false;
    }
    // To prevent mapping with two devices we disable any UDP except motion
    if (!Settings::values.enable_udp_controller && data.engine == "cemuhookudp" &&
        data.type != EngineInputType::Motion) {
        return false;
    }
    // The following drivers don't need to be mapped
    if (data.engine == "touch_from_button") {
        return false;
    }
    if (data.engine == "analog_from_button") {
        return false;
    }
    return true;
}

} // namespace InputCommon
