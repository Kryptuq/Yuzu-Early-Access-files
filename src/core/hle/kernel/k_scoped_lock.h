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
class [[nodiscard]] KScopedLock {
public:
    explicit KScopedLock(T* l) : lock_ptr(l) {
        this->lock_ptr->Lock();
    }
    explicit KScopedLock(T& l) : KScopedLock(std::addressof(l)) {}

    ~KScopedLock() {
        this->lock_ptr->Unlock();
    }

    KScopedLock(const KScopedLock&) = delete;
    KScopedLock& operator=(const KScopedLock&) = delete;

    KScopedLock(KScopedLock&&) = delete;
    KScopedLock& operator=(KScopedLock&&) = delete;

private:
    T* lock_ptr;
};

} // namespace Kernel
