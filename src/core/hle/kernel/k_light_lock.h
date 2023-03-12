// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

#include "core/hle/kernel/k_scoped_lock.h"

namespace Kernel {

class KernelCore;

class KLightLock {
public:
    explicit KLightLock(KernelCore& kernel) : m_kernel{kernel} {}

    void Lock();

    void Unlock();

    bool LockSlowPath(uintptr_t owner, uintptr_t cur_thread);

    void UnlockSlowPath(uintptr_t cur_thread);

    bool IsLocked() const {
        return m_tag.load() != 0;
    }

    bool IsLockedByCurrentThread() const;

private:
    std::atomic<uintptr_t> m_tag{};
    KernelCore& m_kernel;
};

using KScopedLightLock = KScopedLock<KLightLock>;

} // namespace Kernel
