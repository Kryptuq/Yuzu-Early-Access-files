// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/assert.h"
#include "common/common_types.h"

#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace Kernel {

class KConditionVariable {
public:
    using ThreadTree = typename KThread::ConditionVariableThreadTreeType;

    explicit KConditionVariable(Core::System& system_);
    ~KConditionVariable();

    // Arbitration
    [[nodiscard]] ResultCode SignalToAddress(VAddr addr);
    [[nodiscard]] ResultCode WaitForAddress(Handle handle, VAddr addr, u32 value);

    // Condition variable
    void Signal(u64 cv_key, s32 count);
    [[nodiscard]] ResultCode Wait(VAddr addr, u64 key, u32 value, s64 timeout);

private:
    [[nodiscard]] KThread* SignalImpl(KThread* thread);

    ThreadTree thread_tree;

    Core::System& system;
    KernelCore& kernel;
};

inline void BeforeUpdatePriority(const KernelCore& kernel, KConditionVariable::ThreadTree* tree,
                                 KThread* thread) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    tree->erase(tree->iterator_to(*thread));
}

inline void AfterUpdatePriority(const KernelCore& kernel, KConditionVariable::ThreadTree* tree,
                                KThread* thread) {
    ASSERT(kernel.GlobalSchedulerContext().IsLocked());

    tree->insert(*thread);
}

} // namespace Kernel
