// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hid/hid_core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/service/hid/hidbus/hidbus_base.h"
#include "core/hle/service/kernel_helpers.h"

namespace Service::HID {

HidbusBase::HidbusBase(Core::System& system_, KernelHelpers::ServiceContext& service_context_)
    : system(system_), service_context(service_context_) {
    send_command_async_event = service_context.CreateEvent("hidbus:SendCommandAsyncEvent");
}
HidbusBase::~HidbusBase() = default;

void HidbusBase::ActivateDevice() {
    if (is_activated) {
        return;
    }
    is_activated = true;
    OnInit();
}

void HidbusBase::DeactivateDevice() {
    if (is_activated) {
        OnRelease();
    }
    is_activated = false;
}

bool HidbusBase::IsDeviceActivated() const {
    return is_activated;
}

void HidbusBase::Enable(bool enable) {
    device_enabled = enable;
}

bool HidbusBase::IsEnabled() const {
    return device_enabled;
}

bool HidbusBase::IsPollingMode() const {
    return polling_mode_enabled;
}

JoyPollingMode HidbusBase::GetPollingMode() const {
    return polling_mode;
}

void HidbusBase::SetPollingMode(JoyPollingMode mode) {
    polling_mode = mode;
    polling_mode_enabled = true;
}

void HidbusBase::DisablePollingMode() {
    polling_mode_enabled = false;
}

void HidbusBase::SetTransferMemoryAddress(Common::ProcessAddress t_mem) {
    transfer_memory = t_mem;
}

Kernel::KReadableEvent& HidbusBase::GetSendCommandAsycEvent() const {
    return send_command_async_event->GetReadableEvent();
}

} // namespace Service::HID
