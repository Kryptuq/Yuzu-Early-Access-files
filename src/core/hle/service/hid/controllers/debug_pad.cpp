// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/common_types.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hle/service/hid/controllers/debug_pad.h"

namespace Service::HID {

constexpr s32 HID_JOYSTICK_MAX = 0x7fff;
[[maybe_unused]] constexpr s32 HID_JOYSTICK_MIN = -0x7fff;
enum class JoystickId : std::size_t { Joystick_Left, Joystick_Right };

Controller_DebugPad::Controller_DebugPad(Core::System& system_) : ControllerBase{system_} {}
Controller_DebugPad::~Controller_DebugPad() = default;

void Controller_DebugPad::OnInit() {}

void Controller_DebugPad::OnRelease() {}

void Controller_DebugPad::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                   std::size_t size) {
    shared_memory.header.timestamp = core_timing.GetCPUTicks();
    shared_memory.header.total_entry_count = 17;

    if (!IsControllerActivated()) {
        shared_memory.header.entry_count = 0;
        shared_memory.header.last_entry_index = 0;
        return;
    }
    shared_memory.header.entry_count = 16;

    const auto& last_entry = shared_memory.pad_states[shared_memory.header.last_entry_index];
    shared_memory.header.last_entry_index = (shared_memory.header.last_entry_index + 1) % 17;
    auto& cur_entry = shared_memory.pad_states[shared_memory.header.last_entry_index];

    cur_entry.sampling_number = last_entry.sampling_number + 1;
    cur_entry.sampling_number2 = cur_entry.sampling_number;

    if (Settings::values.debug_pad_enabled) {
        cur_entry.attribute.connected.Assign(1);
        auto& pad = cur_entry.pad_state;

        using namespace Settings::NativeButton;
        pad.a.Assign(buttons[A - BUTTON_HID_BEGIN]->GetStatus());
        pad.b.Assign(buttons[B - BUTTON_HID_BEGIN]->GetStatus());
        pad.x.Assign(buttons[X - BUTTON_HID_BEGIN]->GetStatus());
        pad.y.Assign(buttons[Y - BUTTON_HID_BEGIN]->GetStatus());
        pad.l.Assign(buttons[L - BUTTON_HID_BEGIN]->GetStatus());
        pad.r.Assign(buttons[R - BUTTON_HID_BEGIN]->GetStatus());
        pad.zl.Assign(buttons[ZL - BUTTON_HID_BEGIN]->GetStatus());
        pad.zr.Assign(buttons[ZR - BUTTON_HID_BEGIN]->GetStatus());
        pad.plus.Assign(buttons[Plus - BUTTON_HID_BEGIN]->GetStatus());
        pad.minus.Assign(buttons[Minus - BUTTON_HID_BEGIN]->GetStatus());
        pad.d_left.Assign(buttons[DLeft - BUTTON_HID_BEGIN]->GetStatus());
        pad.d_up.Assign(buttons[DUp - BUTTON_HID_BEGIN]->GetStatus());
        pad.d_right.Assign(buttons[DRight - BUTTON_HID_BEGIN]->GetStatus());
        pad.d_down.Assign(buttons[DDown - BUTTON_HID_BEGIN]->GetStatus());

        const auto [stick_l_x_f, stick_l_y_f] =
            analogs[static_cast<std::size_t>(JoystickId::Joystick_Left)]->GetStatus();
        const auto [stick_r_x_f, stick_r_y_f] =
            analogs[static_cast<std::size_t>(JoystickId::Joystick_Right)]->GetStatus();
        cur_entry.l_stick.x = static_cast<s32>(stick_l_x_f * HID_JOYSTICK_MAX);
        cur_entry.l_stick.y = static_cast<s32>(stick_l_y_f * HID_JOYSTICK_MAX);
        cur_entry.r_stick.x = static_cast<s32>(stick_r_x_f * HID_JOYSTICK_MAX);
        cur_entry.r_stick.y = static_cast<s32>(stick_r_y_f * HID_JOYSTICK_MAX);
    }

    std::memcpy(data, &shared_memory, sizeof(SharedMemory));
}

void Controller_DebugPad::OnLoadInputDevices() {
    std::transform(Settings::values.debug_pad_buttons.begin(),
                   Settings::values.debug_pad_buttons.begin() +
                       Settings::NativeButton::NUM_BUTTONS_HID,
                   buttons.begin(), Input::CreateDevice<Input::ButtonDevice>);
    std::transform(Settings::values.debug_pad_analogs.begin(),
                   Settings::values.debug_pad_analogs.end(), analogs.begin(),
                   Input::CreateDevice<Input::AnalogDevice>);
}
} // namespace Service::HID
