// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
