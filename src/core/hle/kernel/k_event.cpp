// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"

namespace Kernel {

KEvent::KEvent(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, readable_event{kernel_}, writable_event{
                                                                                 kernel_} {}

KEvent::~KEvent() = default;

void KEvent::Initialize(std::string&& name_, KProcess* owner_) {
    // Increment reference count.
    // Because reference count is one on creation, this will result
    // in a reference count of two. Thus, when both readable and
    // writable events are closed this object will be destroyed.
    Open();

    // Create our sub events.
    KAutoObject::Create(std::addressof(readable_event));
    KAutoObject::Create(std::addressof(writable_event));

    // Initialize our sub sessions.
    readable_event.Initialize(this, name_ + ":Readable");
    writable_event.Initialize(this, name_ + ":Writable");

    // Set our owner process.
    owner = owner_;
    owner->Open();

    // Mark initialized.
    name = std::move(name_);
    initialized = true;
}

void KEvent::Finalize() {
    KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList>::Finalize();
}

void KEvent::PostDestroy(uintptr_t arg) {
    // Release the event count resource the owner process holds.
    KProcess* owner = reinterpret_cast<KProcess*>(arg);
    owner->GetResourceLimit()->Release(LimitableResource::Events, 1);
    owner->Close();
}

} // namespace Kernel
