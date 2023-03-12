// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_worker_task.h"
#include "core/hle/kernel/k_worker_task_manager.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

KWorkerTask::KWorkerTask(KernelCore& kernel) : KSynchronizationObject{kernel} {}

void KWorkerTask::DoWorkerTask() {
    if (auto* const thread = this->DynamicCast<KThread*>(); thread != nullptr) {
        return thread->DoWorkerTaskImpl();
    } else {
        auto* const process = this->DynamicCast<KProcess*>();
        ASSERT(process != nullptr);

        return process->DoWorkerTaskImpl();
    }
}

KWorkerTaskManager::KWorkerTaskManager() : m_waiting_thread(1, "KWorkerTaskManager") {}

void KWorkerTaskManager::AddTask(KernelCore& kernel, WorkerType type, KWorkerTask* task) {
    ASSERT(type <= WorkerType::Count);
    kernel.WorkerTaskManager().AddTask(kernel, task);
}

void KWorkerTaskManager::AddTask(KernelCore& kernel, KWorkerTask* task) {
    KScopedSchedulerLock sl(kernel);
    m_waiting_thread.QueueWork([task]() {
        // Do the task.
        task->DoWorkerTask();
    });
}

} // namespace Kernel
