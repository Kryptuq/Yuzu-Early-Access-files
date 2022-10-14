// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"

namespace Kernel {

KEvent::KEvent(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, m_readable_event{kernel_} {}

KEvent::~KEvent() = default;

void KEvent::Initialize(KProcess* owner) {
    // Create our readable event.
    KAutoObject::Create(std::addressof(m_readable_event));

    // Initialize our readable event.
    m_readable_event.Initialize(this);

    // Set our owner process.
    m_owner = owner;
    m_owner->Open();

    // Mark initialized.
    m_initialized = true;
}

void KEvent::Finalize() {
    KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList>::Finalize();
}

Result KEvent::Signal() {
    KScopedSchedulerLock sl{kernel};

    R_SUCCEED_IF(m_readable_event_destroyed);

    return m_readable_event.Signal();
}

Result KEvent::Clear() {
    KScopedSchedulerLock sl{kernel};

    R_SUCCEED_IF(m_readable_event_destroyed);

    return m_readable_event.Clear();
}

void KEvent::PostDestroy(uintptr_t arg) {
    // Release the event count resource the owner process holds.
    KProcess* owner = reinterpret_cast<KProcess*>(arg);
    owner->GetResourceLimit()->Release(LimitableResource::Events, 1);
    owner->Close();
}

} // namespace Kernel
