// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>

namespace Common {

/**
 * SpinLock class
 * a lock similar to mutex that forces a thread to spin wait instead calling the
 * supervisor. Should be used on short sequences of code.
 */
class SpinLock {
public:
    SpinLock() = default;

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    void lock();
    void unlock();
    [[nodiscard]] bool try_lock();

private:
    std::atomic_flag lck = ATOMIC_FLAG_INIT;
};

} // namespace Common
