// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"

namespace Kernel {

class KHardwareTimer;

class KThreadQueue {
public:
    explicit KThreadQueue(KernelCore& kernel) : m_kernel{kernel}, m_hardware_timer{} {}
    virtual ~KThreadQueue() = default;

    void SetHardwareTimer(KHardwareTimer* timer) {
        m_hardware_timer = timer;
    }

    virtual void NotifyAvailable(KThread* waiting_thread, KSynchronizationObject* signaled_object,
                                 Result wait_result);
    virtual void EndWait(KThread* waiting_thread, Result wait_result);
    virtual void CancelWait(KThread* waiting_thread, Result wait_result, bool cancel_timer_task);

private:
    KernelCore& m_kernel;
    KHardwareTimer* m_hardware_timer{};
};

class KThreadQueueWithoutEndWait : public KThreadQueue {
public:
    explicit KThreadQueueWithoutEndWait(KernelCore& kernel) : KThreadQueue(kernel) {}

    void EndWait(KThread* waiting_thread, Result wait_result) override final;
};

} // namespace Kernel
