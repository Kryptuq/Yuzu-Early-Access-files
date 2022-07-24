// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/service/time/errors.h"
#include "core/hle/service/time/system_clock_context_update_callback.h"

namespace Service::Time::Clock {

SystemClockContextUpdateCallback::SystemClockContextUpdateCallback() = default;
SystemClockContextUpdateCallback::~SystemClockContextUpdateCallback() = default;

bool SystemClockContextUpdateCallback::NeedUpdate(const SystemClockContext& value) const {
    if (has_context) {
        return context.offset != value.offset ||
               context.steady_time_point.clock_source_id != value.steady_time_point.clock_source_id;
    }

    return true;
}

void SystemClockContextUpdateCallback::RegisterOperationEvent(
    std::shared_ptr<Kernel::KWritableEvent>&& writable_event) {
    operation_event_list.emplace_back(std::move(writable_event));
}

void SystemClockContextUpdateCallback::BroadcastOperationEvent() {
    for (const auto& writable_event : operation_event_list) {
        writable_event->Signal();
    }
}

Result SystemClockContextUpdateCallback::Update(const SystemClockContext& value) {
    Result result{ResultSuccess};

    if (NeedUpdate(value)) {
        context = value;
        has_context = true;

        result = Update();

        if (result == ResultSuccess) {
            BroadcastOperationEvent();
        }
    }

    return result;
}

Result SystemClockContextUpdateCallback::Update() {
    return ResultSuccess;
}

} // namespace Service::Time::Clock
