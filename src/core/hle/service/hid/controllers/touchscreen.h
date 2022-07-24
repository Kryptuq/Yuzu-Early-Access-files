// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hid/hid_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedConsole;
} // namespace Core::HID

namespace Service::HID {
class Controller_Touchscreen final : public ControllerBase {
public:
    // This is nn::hid::TouchScreenModeForNx
    enum class TouchScreenModeForNx : u8 {
        UseSystemSetting,
        Finger,
        Heat2,
    };

    // This is nn::hid::TouchScreenConfigurationForNx
    struct TouchScreenConfigurationForNx {
        TouchScreenModeForNx mode{TouchScreenModeForNx::UseSystemSetting};
        INSERT_PADDING_BYTES_NOINIT(0x7);
        INSERT_PADDING_BYTES_NOINIT(0xF); // Reserved
    };
    static_assert(sizeof(TouchScreenConfigurationForNx) == 0x17,
                  "TouchScreenConfigurationForNx is an invalid size");

    explicit Controller_Touchscreen(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_);
    ~Controller_Touchscreen() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

private:
    static constexpr std::size_t MAX_FINGERS = 16;

    // This is nn::hid::TouchScreenState
    struct TouchScreenState {
        s64 sampling_number{};
        s32 entry_count{};
        INSERT_PADDING_BYTES(4); // Reserved
        std::array<Core::HID::TouchState, MAX_FINGERS> states{};
    };
    static_assert(sizeof(TouchScreenState) == 0x290, "TouchScreenState is an invalid size");

    struct TouchSharedMemory {
        // This is nn::hid::detail::TouchScreenLifo
        Lifo<TouchScreenState, hid_entry_count> touch_screen_lifo{};
        static_assert(sizeof(touch_screen_lifo) == 0x2C38, "touch_screen_lifo is an invalid size");
        INSERT_PADDING_WORDS(0xF2);
    };
    static_assert(sizeof(TouchSharedMemory) == 0x3000, "TouchSharedMemory is an invalid size");

    TouchScreenState next_state{};
    TouchSharedMemory* shared_memory = nullptr;
    Core::HID::EmulatedConsole* console = nullptr;

    std::array<Core::HID::TouchFinger, MAX_FINGERS> fingers{};
};
} // namespace Service::HID
