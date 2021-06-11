// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/settings_input.h"

namespace Settings {
namespace NativeButton {
const std::array<const char*, NumButtons> mapping = {{
    "button_a",      "button_b",     "button_x",     "button_y",    "button_lstick",
    "button_rstick", "button_l",     "button_r",     "button_zl",   "button_zr",
    "button_plus",   "button_minus", "button_dleft", "button_dup",  "button_dright",
    "button_ddown",  "button_sl",    "button_sr",    "button_home", "button_screenshot",
}};
}

namespace NativeAnalog {
const std::array<const char*, NumAnalogs> mapping = {{
    "lstick",
    "rstick",
}};
}

namespace NativeVibration {
const std::array<const char*, NumVibrations> mapping = {{
    "left_vibration_device",
    "right_vibration_device",
}};
}

namespace NativeMotion {
const std::array<const char*, NumMotions> mapping = {{
    "motionleft",
    "motionright",
}};
}

namespace NativeMouseButton {
const std::array<const char*, NumMouseButtons> mapping = {{
    "left",
    "right",
    "middle",
    "forward",
    "back",
}};
}
} // namespace Settings
