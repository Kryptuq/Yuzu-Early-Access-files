// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_writable_event.h"

namespace Kernel {

KWritableEvent::KWritableEvent(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_} {}

KWritableEvent::~KWritableEvent() = default;

void KWritableEvent::Initialize(KEvent* parent_event_, std::string&& name_) {
    parent = parent_event_;
    name = std::move(name_);
    parent->GetReadableEvent().Open();
}

ResultCode KWritableEvent::Signal() {
    return parent->GetReadableEvent().Signal();
}

ResultCode KWritableEvent::Clear() {
    return parent->GetReadableEvent().Clear();
}

void KWritableEvent::Destroy() {
    // Close our references.
    parent->GetReadableEvent().Close();
    parent->Close();
}

} // namespace Kernel
