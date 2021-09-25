// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This file references various implementation details from Atmosphere, an open-source firmware for
// the Nintendo Switch. Copyright 2018-2020 Atmosphere-NX.

#pragma once

#include "common/common_types.h"

namespace Kernel {

template <typename T>
concept KLockable = !std::is_reference_v<T> && requires(T & t) {
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
