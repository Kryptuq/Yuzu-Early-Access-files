// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"

namespace Kernel {

class KThreadQueue {
public:
    explicit KThreadQueue(KernelCore& kernel_) : kernel{kernel_} {}
    virtual ~KThreadQueue() = default;

    virtual void NotifyAvailable(KThread* waiting_thread, KSynchronizationObject* signaled_object,
                                 ResultCode wait_result);
    virtual void EndWait(KThread* waiting_thread, ResultCode wait_result);
    virtual void CancelWait(KThread* waiting_thread, ResultCode wait_result,
                            bool cancel_timer_task);

private:
    KernelCore& kernel;
    KThread::WaiterList wait_list{};
};

class KThreadQueueWithoutEndWait : public KThreadQueue {
public:
    explicit KThreadQueueWithoutEndWait(KernelCore& kernel_) : KThreadQueue(kernel_) {}

    void EndWait(KThread* waiting_thread, ResultCode wait_result) override final;
};

} // namespace Kernel
