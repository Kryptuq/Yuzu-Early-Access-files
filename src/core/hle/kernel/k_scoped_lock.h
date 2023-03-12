// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

namespace Kernel {

template <typename T>
concept KLockable = !
std::is_reference_v<T>&& requires(T& t) {
                             { t.Lock() } -> std::same_as<void>;
                             { t.Unlock() } -> std::same_as<void>;
                         };

template <typename T>
    requires KLockable<T>
class KScopedLock {
public:
    explicit KScopedLock(T* l) : m_lock(*l) {}
    explicit KScopedLock(T& l) : m_lock(l) {
        m_lock.Lock();
    }

    ~KScopedLock() {
        m_lock.Unlock();
    }

    KScopedLock(const KScopedLock&) = delete;
    KScopedLock& operator=(const KScopedLock&) = delete;

    KScopedLock(KScopedLock&&) = delete;
    KScopedLock& operator=(KScopedLock&&) = delete;

private:
    T& m_lock;
};

} // namespace Kernel
