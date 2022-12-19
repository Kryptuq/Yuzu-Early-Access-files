// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_spin_lock.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_timer_task.h"

namespace Kernel {

class KHardwareTimerBase {
public:
    explicit KHardwareTimerBase(KernelCore& kernel) : m_kernel{kernel} {}

    void CancelTask(KTimerTask* task) {
        KScopedDisableDispatch dd{m_kernel};
        KScopedSpinLock lk{m_lock};

        if (const s64 task_time = task->GetTime(); task_time > 0) {
            this->RemoveTaskFromTree(task);
        }
    }

protected:
    KSpinLock& GetLock() {
        return m_lock;
    }

    s64 DoInterruptTaskImpl(s64 cur_time) {
        // We want to handle all tasks, returning the next time that a task is scheduled.
        while (true) {
            // Get the next task. If there isn't one, return 0.
            KTimerTask* task = m_next_task;
            if (task == nullptr) {
                return 0;
            }

            // If the task needs to be done in the future, do it in the future and not now.
            if (const s64 task_time = task->GetTime(); task_time > cur_time) {
                return task_time;
            }

            // Remove the task from the tree of tasks, and update our next task.
            this->RemoveTaskFromTree(task);

            // Handle the task.
            task->OnTimer();
        }
    }

    bool RegisterAbsoluteTaskImpl(KTimerTask* task, s64 task_time) {
        ASSERT(task_time > 0);

        // Set the task's time, and insert it into our tree.
        task->SetTime(task_time);
        m_task_tree.insert(*task);

        // Update our next task if relevant.
        if (m_next_task != nullptr && m_next_task->GetTime() <= task_time) {
            return false;
        }
        m_next_task = task;
        return true;
    }

private:
    void RemoveTaskFromTree(KTimerTask* task) {
        // Erase from the tree.
        auto it = m_task_tree.erase(m_task_tree.iterator_to(*task));

        // Clear the task's scheduled time.
        task->SetTime(0);

        // Update our next task if relevant.
        if (m_next_task == task) {
            m_next_task = (it != m_task_tree.end()) ? std::addressof(*it) : nullptr;
        }
    }

protected:
    KernelCore& m_kernel;

private:
    using TimerTaskTree = Common::IntrusiveRedBlackTreeBaseTraits<KTimerTask>::TreeType<KTimerTask>;

    KSpinLock m_lock{};
    TimerTaskTree m_task_tree{};
    KTimerTask* m_next_task{};
};

} // namespace Kernel
