// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/hid/controllers/controller_base.h"

namespace Service::HID {

ControllerBase::ControllerBase(Core::HID::HIDCore& hid_core_) : hid_core(hid_core_) {}
ControllerBase::~ControllerBase() = default;

void ControllerBase::ActivateController() {
    if (is_activated) {
        return;
    }
    is_activated = true;
    OnInit();
}

void ControllerBase::DeactivateController() {
    if (is_activated) {
        OnRelease();
    }
    is_activated = false;
}

bool ControllerBase::IsControllerActivated() const {
    return is_activated;
}
} // namespace Service::HID
