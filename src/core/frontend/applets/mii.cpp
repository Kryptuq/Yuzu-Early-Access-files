// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/frontend/applets/mii.h"

namespace Core::Frontend {

MiiApplet::~MiiApplet() = default;

void DefaultMiiApplet::ShowMii(
    const MiiParameters& parameters,
    const std::function<void(const Core::Frontend::MiiParameters& parameters)> callback) const {
    LOG_INFO(Service_HID, "(STUBBED) called");
    callback(parameters);
}

} // namespace Core::Frontend
