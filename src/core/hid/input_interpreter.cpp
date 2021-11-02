// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hid/hid_types.h"
#include "core/hid/input_interpreter.h"
#include "core/hle/service/hid/controllers/npad.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/sm/sm.h"

InputInterpreter::InputInterpreter(Core::System& system)
    : npad{system.ServiceManager()
               .GetService<Service::HID::Hid>("hid")
               ->GetAppletResource()
               ->GetController<Service::HID::Controller_NPad>(Service::HID::HidController::NPad)} {
    ResetButtonStates();
}

InputInterpreter::~InputInterpreter() = default;

void InputInterpreter::PollInput() {
    const u64 button_state = npad.GetAndResetPressState();

    previous_index = current_index;
    current_index = (current_index + 1) % button_states.size();

    button_states[current_index] = button_state;
}

void InputInterpreter::ResetButtonStates() {
    previous_index = 0;
    current_index = 0;

    button_states[0] = 0xFFFFFFFFFFFFFFFF;

    for (std::size_t i = 1; i < button_states.size(); ++i) {
        button_states[i] = 0;
    }
}

bool InputInterpreter::IsButtonPressed(Core::HID::NpadButton button) const {
    return (button_states[current_index] & static_cast<u64>(button)) != 0;
}

bool InputInterpreter::IsButtonPressedOnce(Core::HID::NpadButton button) const {
    const bool current_press = (button_states[current_index] & static_cast<u64>(button)) != 0;
    const bool previous_press = (button_states[previous_index] & static_cast<u64>(button)) != 0;

    return current_press && !previous_press;
}

bool InputInterpreter::IsButtonHeld(Core::HID::NpadButton button) const {
    u64 held_buttons{button_states[0]};

    for (std::size_t i = 1; i < button_states.size(); ++i) {
        held_buttons &= button_states[i];
    }

    return (held_buttons & static_cast<u64>(button)) != 0;
}
