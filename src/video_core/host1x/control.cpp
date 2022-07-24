// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/assert.h"
#include "video_core/host1x/control.h"
#include "video_core/host1x/host1x.h"

namespace Tegra::Host1x {

Control::Control(Host1x& host1x_) : host1x(host1x_) {}

Control::~Control() = default;

void Control::ProcessMethod(Method method, u32 argument) {
    switch (method) {
    case Method::LoadSyncptPayload32:
        syncpoint_value = argument;
        break;
    case Method::WaitSyncpt:
    case Method::WaitSyncpt32:
        Execute(argument);
        break;
    default:
        UNIMPLEMENTED_MSG("Control method 0x{:X}", static_cast<u32>(method));
        break;
    }
}

void Control::Execute(u32 data) {
    host1x.GetSyncpointManager().WaitHost(data, syncpoint_value);
}

} // namespace Tegra::Host1x
