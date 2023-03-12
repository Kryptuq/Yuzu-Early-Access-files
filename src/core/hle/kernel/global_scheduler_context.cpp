// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>

#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/global_scheduler_context.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

GlobalSchedulerContext::GlobalSchedulerContext(KernelCore& kernel)
    : m_kernel{kernel}, m_scheduler_lock{kernel} {}

GlobalSchedulerContext::~GlobalSchedulerContext() = default;

void GlobalSchedulerContext::AddThread(KThread* thread) {
    std::scoped_lock lock{m_global_list_guard};
    m_thread_list.push_back(thread);
}

void GlobalSchedulerContext::RemoveThread(KThread* thread) {
    std::scoped_lock lock{m_global_list_guard};
    std::erase(m_thread_list, thread);
}

void GlobalSchedulerContext::PreemptThreads() {
    // The priority levels at which the global scheduler preempts threads every 10 ms. They are
    // ordered from Core 0 to Core 3.
    static constexpr std::array<u32, Core::Hardware::NUM_CPU_CORES> preemption_priorities{
        59,
        59,
        59,
        63,
    };

    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(m_kernel));
    for (u32 core_id = 0; core_id < Core::Hardware::NUM_CPU_CORES; core_id++) {
        const u32 priority = preemption_priorities[core_id];
        KScheduler::RotateScheduledQueue(m_kernel, core_id, priority);
    }
}

bool GlobalSchedulerContext::IsLocked() const {
    return m_scheduler_lock.IsLockedByCurrentThread();
}

void GlobalSchedulerContext::RegisterDummyThreadForWakeup(KThread* thread) {
    ASSERT(this->IsLocked());

    m_woken_dummy_threads.insert(thread);
}

void GlobalSchedulerContext::UnregisterDummyThreadForWakeup(KThread* thread) {
    ASSERT(this->IsLocked());

    m_woken_dummy_threads.erase(thread);
}

void GlobalSchedulerContext::WakeupWaitingDummyThreads() {
    ASSERT(this->IsLocked());

    for (auto* thread : m_woken_dummy_threads) {
        thread->DummyThreadEndWait();
    }

    m_woken_dummy_threads.clear();
}

} // namespace Kernel
