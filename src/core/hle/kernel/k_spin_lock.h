// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>

#include "core/hle/kernel/k_scoped_lock.h"

namespace Kernel {

class KSpinLock {
public:
    KSpinLock() = default;

    KSpinLock(const KSpinLock&) = delete;
    KSpinLock& operator=(const KSpinLock&) = delete;

    KSpinLock(KSpinLock&&) = delete;
    KSpinLock& operator=(KSpinLock&&) = delete;

    void Lock();
    void Unlock();
    [[nodiscard]] bool TryLock();

private:
    std::mutex lck;
};

// TODO(bunnei): Alias for now, in case we want to implement these accurately in the future.
using KAlignedSpinLock = KSpinLock;
using KNotAlignedSpinLock = KSpinLock;

using KScopedSpinLock = KScopedLock<KSpinLock>;
using KScopedAlignedSpinLock = KScopedLock<KAlignedSpinLock>;
using KScopedNotAlignedSpinLock = KScopedLock<KNotAlignedSpinLock>;

} // namespace Kernel
