// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_spin_lock.h"

namespace Kernel {

void KSpinLock::Lock() {
    m_lock.lock();
}

void KSpinLock::Unlock() {
    m_lock.unlock();
}

bool KSpinLock::TryLock() {
    return m_lock.try_lock();
}

} // namespace Kernel
