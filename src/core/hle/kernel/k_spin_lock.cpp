// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_spin_lock.h"

namespace Kernel {

void KSpinLock::Lock() {
    lck.lock();
}

void KSpinLock::Unlock() {
    lck.unlock();
}

bool KSpinLock::TryLock() {
    return lck.try_lock();
}

} // namespace Kernel
