// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
        {1000, nullptr, "Unknown1000"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

CAPS_SS::~CAPS_SS() = default;

} // namespace Service::Capture
