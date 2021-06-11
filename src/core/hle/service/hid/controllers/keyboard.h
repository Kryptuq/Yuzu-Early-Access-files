// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/settings.h"
#include "common/swap.h"
#include "core/frontend/input.h"
#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {
class Controller_Keyboard final : public ControllerBase {
public:
    explicit Controller_Keyboard(Core::System& system_);
    ~Controller_Keyboard() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // Called when input devices should be loaded
    void OnLoadInputDevices() override;

private:
    struct Modifiers {
        union {
            u32_le raw{};
            BitField<0, 1, u32> control;
            BitField<1, 1, u32> shift;
            BitField<2, 1, u32> left_alt;
            BitField<3, 1, u32> right_alt;
            BitField<4, 1, u32> gui;
            BitField<8, 1, u32> caps_lock;
            BitField<9, 1, u32> scroll_lock;
            BitField<10, 1, u32> num_lock;
            BitField<11, 1, u32> katakana;
            BitField<12, 1, u32> hiragana;
        };
    };
    static_assert(sizeof(Modifiers) == 0x4, "Modifiers is an invalid size");

    struct KeyboardState {
        s64_le sampling_number;
        s64_le sampling_number2;

        Modifiers modifier;
        std::array<u8, 32> key;
    };
    static_assert(sizeof(KeyboardState) == 0x38, "KeyboardState is an invalid size");

    struct SharedMemory {
        CommonHeader header;
        std::array<KeyboardState, 17> pad_states;
        INSERT_PADDING_BYTES(0x28);
    };
    static_assert(sizeof(SharedMemory) == 0x400, "SharedMemory is an invalid size");
    SharedMemory shared_memory{};

    std::array<std::unique_ptr<Input::ButtonDevice>, Settings::NativeKeyboard::NumKeyboardKeys>
        keyboard_keys;
    std::array<std::unique_ptr<Input::ButtonDevice>, Settings::NativeKeyboard::NumKeyboardMods>
        keyboard_mods;
};
} // namespace Service::HID
