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

void KReadableEvent::Initialize(KEvent* parent) {
    m_is_signaled = false;
    m_parent = parent;

    if (m_parent != nullptr) {
        m_parent->Open();
    }
}

bool KReadableEvent::IsSignaled() const {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(kernel));

    return m_is_signaled;
}

void KReadableEvent::Destroy() {
    if (m_parent) {
        {
            KScopedSchedulerLock sl{kernel};
            m_parent->OnReadableEventDestroyed();
        }
        m_parent->Close();
    }
}

Result KReadableEvent::Signal() {
    KScopedSchedulerLock lk{kernel};

    if (!m_is_signaled) {
        m_is_signaled = true;
        this->NotifyAvailable();
    }

    return ResultSuccess;
}

Result KReadableEvent::Clear() {
    this->Reset();

    return ResultSuccess;
}

Result KReadableEvent::Reset() {
    KScopedSchedulerLock lk{kernel};

    if (!m_is_signaled) {
        return ResultInvalidState;
    }

    m_is_signaled = false;
    return ResultSuccess;
}

} // namespace Kernel
