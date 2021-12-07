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

    void LinkNode(ThreadListNode* node_) {
        // Link the node to the list.
        if (thread_list_tail == nullptr) {
            thread_list_head = node_;
        } else {
            thread_list_tail->next = node_;
        }

        thread_list_tail = node_;
    }

    void UnlinkNode(ThreadListNode* node_) {
        // Unlink the node from the list.
        ThreadListNode* prev_ptr =
            reinterpret_cast<ThreadListNode*>(std::addressof(thread_list_head));
        ThreadListNode* prev_val = nullptr;
        ThreadListNode *prev, *tail_prev;

        do {
            prev = prev_ptr;
            prev_ptr = prev_ptr->next;
            tail_prev = prev_val;
            prev_val = prev_ptr;
        } while (prev_ptr != node_);

        if (thread_list_tail == node_) {
            thread_list_tail = tail_prev;
        }

        prev->next = node_->next;
    }

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
