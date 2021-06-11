// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/caps/caps_ss.h"

namespace Service::Capture {

CAPS_SS::CAPS_SS(Core::System& system_) : ServiceFramework{system_, "caps:ss"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {201, nullptr, "SaveScreenShot"},
        {202, nullptr, "SaveEditedScreenShot"},
        {203, nullptr, "SaveScreenShotEx0"},
        {204, nullptr, "SaveEditedScreenShotEx0"},
        {206, nullptr, "Unknown206"},
        {208, nullptr, "SaveScreenShotOfMovieEx1"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

CAPS_SS::~CAPS_SS() = default;

} // namespace Service::Capture
