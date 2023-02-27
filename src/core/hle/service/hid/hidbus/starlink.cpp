// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/hidbus/starlink.h"

namespace Service::HID {
constexpr u8 DEVICE_ID = 0x28;

Starlink::Starlink(Core::System& system_, KernelHelpers::ServiceContext& service_context_)
    : HidbusBase(system_, service_context_) {}
Starlink::~Starlink() = default;

void Starlink::OnInit() {
    return;
}

void Starlink::OnRelease() {
    return;
};

void Starlink::OnUpdate() {
    if (!is_activated) {
        return;
    }
    if (!device_enabled) {
        return;
    }
    if (!polling_mode_enabled || transfer_memory == 0) {
        return;
    }

    LOG_ERROR(Service_HID, "Polling mode not supported {}", polling_mode);
}

u8 Starlink::GetDeviceId() const {
    return DEVICE_ID;
}

std::vector<u8> Starlink::GetReply() const {
    return {};
}

bool Starlink::SetCommand(std::span<const u8> data) {
    LOG_ERROR(Service_HID, "Command not implemented");
    return false;
}

} // namespace Service::HID
