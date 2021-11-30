// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/result.h"

namespace Kernel {

class KernelCore;
class Synchronization;
class KThread;

/// Class that represents a Kernel object that a thread can be waiting on
class KSynchronizationObject : public KAutoObjectWithList {
    KERNEL_AUTOOBJECT_TRAITS(KSynchronizationObject, KAutoObject);

public:
    struct ThreadListNode {
        ThreadListNode* next{};
        KThread* thread{};
    };

    [[nodiscard]] static ResultCode Wait(KernelCore& kernel, s32* out_index,
                                         KSynchronizationObject** objects, const s32 num_objects,
                                         s64 timeout);

    void Finalize() override;

    [[nodiscard]] virtual bool IsSignaled() const = 0;

    [[nodiscard]] std::vector<KThread*> GetWaitingThreadsForDebugging() const;

protected:
    explicit KSynchronizationObject(KernelCore& kernel);
    ~KSynchronizationObject() override;

    virtual void OnFinalizeSynchronizationObject() {}

    void NotifyAvailable(ResultCode result);
    void NotifyAvailable() {
        return this->NotifyAvailable(ResultSuccess);
    }

private:
    ThreadListNode* thread_list_head{};
    ThreadListNode* thread_list_tail{};
};

} // namespace Kernel
