// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/common_types.h"
#include "input_common/input_engine.h"
#include "input_common/input_mapping.h"

namespace InputCommon {

MappingFactory::MappingFactory() {}

void MappingFactory::BeginMapping(Polling::InputType type) {
    is_enabled = true;
    input_type = type;
    input_queue.Clear();
    first_axis = -1;
    second_axis = -1;
}

[[nodiscard]] const Common::ParamPackage MappingFactory::GetNextInput() {
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
    if (data.pad.guid != Common::UUID{}) {
        new_input.Set("guid", data.pad.guid.Format());
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
    if (data.pad.guid != Common::UUID{}) {
        new_input.Set("guid", data.pad.guid.Format());
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
    if (data.pad.guid != Common::UUID{}) {
        new_input.Set("guid", data.pad.guid.Format());
    }
    new_input.Set("port", static_cast<int>(data.pad.port));
    new_input.Set("pad", static_cast<int>(data.pad.pad));
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
    // The following drivers don't need to be mapped
    if (data.engine == "tas") {
        return false;
    }
    if (data.engine == "touch") {
        return false;
    }
    if (data.engine == "touch_from_button") {
        return false;
    }
    if (data.engine == "analog_from_button") {
        return false;
    }
    return true;
}

} // namespace InputCommon
