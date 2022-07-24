// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
