// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/frontend/input_interpreter.h"
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
    const u32 button_state = npad.GetAndResetPressState();

    previous_index = current_index;
    current_index = (current_index + 1) % button_states.size();

    button_states[current_index] = button_state;
}

void InputInterpreter::ResetButtonStates() {
    previous_index = 0;
    current_index = 0;

    button_states[0] = 0xFFFFFFFF;

    for (std::size_t i = 1; i < button_states.size(); ++i) {
        button_states[i] = 0;
    }
}

bool InputInterpreter::IsButtonPressed(HIDButton button) const {
    return (button_states[current_index] & (1U << static_cast<u8>(button))) != 0;
}

bool InputInterpreter::IsButtonPressedOnce(HIDButton button) const {
    const bool current_press =
        (button_states[current_index] & (1U << static_cast<u8>(button))) != 0;
    const bool previous_press =
        (button_states[previous_index] & (1U << static_cast<u8>(button))) != 0;

    return current_press && !previous_press;
}

bool InputInterpreter::IsButtonHeld(HIDButton button) const {
    u32 held_buttons{button_states[0]};

    for (std::size_t i = 1; i < button_states.size(); ++i) {
        held_buttons &= button_states[i];
    }

    return (held_buttons & (1U << static_cast<u8>(button))) != 0;
}
