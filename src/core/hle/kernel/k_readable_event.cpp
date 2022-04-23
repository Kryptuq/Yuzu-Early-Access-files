// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KReadableEvent::KReadableEvent(KernelCore& kernel_) : KSynchronizationObject{kernel_} {}

KReadableEvent::~KReadableEvent() = default;

bool KReadableEvent::IsSignaled() const {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    return is_signaled;
}

void KReadableEvent::Destroy() {
    if (parent) {
        parent->Close();
    }
}

ResultCode KReadableEvent::Signal() {
    KScopedSchedulerLock lk{kernel};

    if (!is_signaled) {
        is_signaled = true;
        NotifyAvailable();
    }

    return ResultSuccess;
}

ResultCode KReadableEvent::Clear() {
    Reset();

    return ResultSuccess;
}

ResultCode KReadableEvent::Reset() {
    KScopedSchedulerLock lk{kernel};

    if (!is_signaled) {
        return ResultInvalidState;
    }

    is_signaled = false;
    return ResultSuccess;
}

} // namespace Kernel
