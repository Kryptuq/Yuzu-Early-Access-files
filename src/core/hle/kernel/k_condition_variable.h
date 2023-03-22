// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"

#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_typed_address.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace Kernel {

class KConditionVariable {
public:
    using ThreadTree = typename KThread::ConditionVariableThreadTreeType;

    explicit KConditionVariable(Core::System& system);
    ~KConditionVariable();

    // Arbitration
    Result SignalToAddress(KProcessAddress addr);
    Result WaitForAddress(Handle handle, KProcessAddress addr, u32 value);

    // Condition variable
    void Signal(u64 cv_key, s32 count);
    Result Wait(KProcessAddress addr, u64 key, u32 value, s64 timeout);

private:
    void SignalImpl(KThread* thread);

private:
    Core::System& m_system;
    KernelCore& m_kernel;
    ThreadTree m_tree{};
};

inline void BeforeUpdatePriority(KernelCore& kernel, KConditionVariable::ThreadTree* tree,
                                 KThread* thread) {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(kernel));

    tree->erase(tree->iterator_to(*thread));
}

inline void AfterUpdatePriority(KernelCore& kernel, KConditionVariable::ThreadTree* tree,
                                KThread* thread) {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(kernel));

    tree->insert(*thread);
}

} // namespace Kernel
