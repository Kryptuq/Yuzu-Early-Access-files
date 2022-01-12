// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/service/hid/hidbus/starlink.h"

namespace Service::HID {
constexpr u8 DEVICE_ID = 0x19;

Starlink::Starlink(Core::HID::HIDCore& hid_core_, KernelHelpers::ServiceContext& service_context_)
    : HidbusBase(service_context_) {}
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
    if (!polling_mode_enabled || !is_transfer_memory_set) {
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

bool Starlink::SetCommand(const std::vector<u8>& data) {
    LOG_ERROR(Service_HID, "Command not implemented");
    return false;
}

} // namespace Service::HID
