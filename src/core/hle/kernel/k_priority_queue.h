// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bit>
#include <concepts>

#include "common/assert.h"
#include "common/bit_set.h"
#include "common/common_types.h"
#include "common/concepts.h"

namespace Kernel {

class KThread;

template <typename T>
concept KPriorityQueueAffinityMask = !std::is_reference_v<T> && requires(T & t) {
    { t.GetAffinityMask() } -> Common::ConvertibleTo<u64>;
    {t.SetAffinityMask(0)};

    { t.GetAffinity(0) } -> std::same_as<bool>;
    {t.SetAffinity(0, false)};
    {t.SetAll()};
};

template <typename T>
concept KPriorityQueueMember = !std::is_reference_v<T> && requires(T & t) {
    {typename T::QueueEntry()};
    {(typename T::QueueEntry()).Initialize()};
    {(typename T::QueueEntry()).SetPrev(std::addressof(t))};
    {(typename T::QueueEntry()).SetNext(std::addressof(t))};
    { (typename T::QueueEntry()).GetNext() } -> std::same_as<T*>;
    { (typename T::QueueEntry()).GetPrev() } -> std::same_as<T*>;
    { t.GetPriorityQueueEntry(0) } -> std::same_as<typename T::QueueEntry&>;

    {t.GetAffinityMask()};
    { std::remove_cvref_t<decltype(t.GetAffinityMask())>() } -> KPriorityQueueAffinityMask;

    { t.GetActiveCore() } -> Common::ConvertibleTo<s32>;
    { t.GetPriority() } -> Common::ConvertibleTo<s32>;
    { t.IsDummyThread() } -> Common::ConvertibleTo<bool>;
};

template <typename Member, size_t NumCores_, int LowestPriority, int HighestPriority>
requires KPriorityQueueMember<Member>
class KPriorityQueue {
public:
    using AffinityMaskType = std::remove_cv_t<
        std::remove_reference_t<decltype(std::declval<Member>().GetAffinityMask())>>;

    static_assert(LowestPriority >= 0);
    static_assert(HighestPriority >= 0);
    static_assert(LowestPriority >= HighestPriority);
    static constexpr size_t NumPriority = LowestPriority - HighestPriority + 1;
    static constexpr size_t NumCores = NumCores_;

    static constexpr bool IsValidCore(s32 core) {
        return 0 <= core && core < static_cast<s32>(NumCores);
    }

    static constexpr bool IsValidPriority(s32 priority) {
        return HighestPriority <= priority && priority <= LowestPriority + 1;
    }

private:
    using Entry = typename Member::QueueEntry;

public:
    class KPerCoreQueue {
    private:
        std::array<Entry, NumCores> root{};

    public:
        constexpr KPerCoreQueue() {
            for (auto& per_core_root : root) {
                per_core_root.Initialize();
            }
        }

        constexpr bool PushBack(s32 core, Member* member) {
            // Get the entry associated with the member.
            Entry& member_entry = member->GetPriorityQueueEntry(core);

            // Get the entry associated with the end of the queue.
            Member* tail = this->root[core].GetPrev();
            Entry& tail_entry =
                (tail != nullptr) ? tail->GetPriorityQueueEntry(core) : this->root[core];

            // Link the entries.
            member_entry.SetPrev(tail);
            member_entry.SetNext(nullptr);
            tail_entry.SetNext(member);
            this->root[core].SetPrev(member);

            return tail == nullptr;
        }

        constexpr bool PushFront(s32 core, Member* member) {
            // Get the entry associated with the member.
            Entry& member_entry = member->GetPriorityQueueEntry(core);

            // Get the entry associated with the front of the queue.
            Member* head = this->root[core].GetNext();
            Entry& head_entry =
                (head != nullptr) ? head->GetPriorityQueueEntry(core) : this->root[core];

            // Link the entries.
            member_entry.SetPrev(nullptr);
            member_entry.SetNext(head);
            head_entry.SetPrev(member);
            this->root[core].SetNext(member);

            return (head == nullptr);
        }

        constexpr bool Remove(s32 core, Member* member) {
            // Get the entry associated with the member.
            Entry& member_entry = member->GetPriorityQueueEntry(core);

            // Get the entries associated with next and prev.
            Member* prev = member_entry.GetPrev();
            Member* next = member_entry.GetNext();
            Entry& prev_entry =
                (prev != nullptr) ? prev->GetPriorityQueueEntry(core) : this->root[core];
            Entry& next_entry =
                (next != nullptr) ? next->GetPriorityQueueEntry(core) : this->root[core];

            // Unlink.
            prev_entry.SetNext(next);
            next_entry.SetPrev(prev);

            return (this->GetFront(core) == nullptr);
        }

        constexpr Member* GetFront(s32 core) const {
            return this->root[core].GetNext();
        }
    };

    class KPriorityQueueImpl {
    public:
        constexpr KPriorityQueueImpl() = default;

        constexpr void PushBack(s32 priority, s32 core, Member* member) {
            ASSERT(IsValidCore(core));
            ASSERT(IsValidPriority(priority));

            if (priority > LowestPriority) {
                return;
            }

            if (this->queues[priority].PushBack(core, member)) {
                this->available_priorities[core].SetBit(priority);
            }
        }

        constexpr void PushFront(s32 priority, s32 core, Member* member) {
            ASSERT(IsValidCore(core));
            ASSERT(IsValidPriority(priority));

            if (priority > LowestPriority) {
                return;
            }

            if (this->queues[priority].PushFront(core, member)) {
                this->available_priorities[core].SetBit(priority);
            }
        }

        constexpr void Remove(s32 priority, s32 core, Member* member) {
            ASSERT(IsValidCore(core));
            ASSERT(IsValidPriority(priority));

            if (priority > LowestPriority) {
                return;
            }

            if (this->queues[priority].Remove(core, member)) {
                this->available_priorities[core].ClearBit(priority);
            }
        }

        constexpr Member* GetFront(s32 core) const {
            ASSERT(IsValidCore(core));

            const s32 priority =
                static_cast<s32>(this->available_priorities[core].CountLeadingZero());
            if (priority <= LowestPriority) {
                return this->queues[priority].GetFront(core);
            } else {
                return nullptr;
            }
        }

        constexpr Member* GetFront(s32 priority, s32 core) const {
            ASSERT(IsValidCore(core));
            ASSERT(IsValidPriority(priority));

            if (priority <= LowestPriority) {
                return this->queues[priority].GetFront(core);
            } else {
                return nullptr;
            }
        }

        constexpr Member* GetNext(s32 core, const Member* member) const {
            ASSERT(IsValidCore(core));

            Member* next = member->GetPriorityQueueEntry(core).GetNext();
            if (next == nullptr) {
                const s32 priority = static_cast<s32>(
                    this->available_priorities[core].GetNextSet(member->GetPriority()));
                if (priority <= LowestPriority) {
                    next = this->queues[priority].GetFront(core);
                }
            }
            return next;
        }

        constexpr void MoveToFront(s32 priority, s32 core, Member* member) {
            ASSERT(IsValidCore(core));
            ASSERT(IsValidPriority(priority));

            if (priority <= LowestPriority) {
                this->queues[priority].Remove(core, member);
                this->queues[priority].PushFront(core, member);
            }
        }

        constexpr Member* MoveToBack(s32 priority, s32 core, Member* member) {
            ASSERT(IsValidCore(core));
            ASSERT(IsValidPriority(priority));

            if (priority <= LowestPriority) {
                this->queues[priority].Remove(core, member);
                this->queues[priority].PushBack(core, member);
                return this->queues[priority].GetFront(core);
            } else {
                return nullptr;
            }
        }

    private:
        std::array<KPerCoreQueue, NumPriority> queues{};
        std::array<Common::BitSet64<NumPriority>, NumCores> available_priorities{};
    };

private:
    KPriorityQueueImpl scheduled_queue;
    KPriorityQueueImpl suggested_queue;

private:
    constexpr void ClearAffinityBit(u64& affinity, s32 core) {
        affinity &= ~(UINT64_C(1) << core);
    }

    constexpr s32 GetNextCore(u64& affinity) {
        const s32 core = std::countr_zero(affinity);
        ClearAffinityBit(affinity, core);
        return core;
    }

    constexpr void PushBack(s32 priority, Member* member) {
        ASSERT(IsValidPriority(priority));

        // Push onto the scheduled queue for its core, if we can.
        u64 affinity = member->GetAffinityMask().GetAffinityMask();
        if (const s32 core = member->GetActiveCore(); core >= 0) {
            this->scheduled_queue.PushBack(priority, core, member);
            ClearAffinityBit(affinity, core);
        }

        // And suggest the thread for all other cores.
        while (affinity) {
            this->suggested_queue.PushBack(priority, GetNextCore(affinity), member);
        }
    }

    constexpr void PushFront(s32 priority, Member* member) {
        ASSERT(IsValidPriority(priority));

        // Push onto the scheduled queue for its core, if we can.
        u64 affinity = member->GetAffinityMask().GetAffinityMask();
        if (const s32 core = member->GetActiveCore(); core >= 0) {
            this->scheduled_queue.PushFront(priority, core, member);
            ClearAffinityBit(affinity, core);
        }

        // And suggest the thread for all other cores.
        // Note: Nintendo pushes onto the back of the suggested queue, not the front.
        while (affinity) {
            this->suggested_queue.PushBack(priority, GetNextCore(affinity), member);
        }
    }

    constexpr void Remove(s32 priority, Member* member) {
        ASSERT(IsValidPriority(priority));

        // Remove from the scheduled queue for its core.
        u64 affinity = member->GetAffinityMask().GetAffinityMask();
        if (const s32 core = member->GetActiveCore(); core >= 0) {
            this->scheduled_queue.Remove(priority, core, member);
            ClearAffinityBit(affinity, core);
        }

        // Remove from the suggested queue for all other cores.
        while (affinity) {
            this->suggested_queue.Remove(priority, GetNextCore(affinity), member);
        }
    }

public:
    constexpr KPriorityQueue() = default;

    // Getters.
    constexpr Member* GetScheduledFront(s32 core) const {
        return this->scheduled_queue.GetFront(core);
    }

    constexpr Member* GetScheduledFront(s32 core, s32 priority) const {
        return this->scheduled_queue.GetFront(priority, core);
    }

    constexpr Member* GetSuggestedFront(s32 core) const {
        return this->suggested_queue.GetFront(core);
    }

    constexpr Member* GetSuggestedFront(s32 core, s32 priority) const {
        return this->suggested_queue.GetFront(priority, core);
    }

    constexpr Member* GetScheduledNext(s32 core, const Member* member) const {
        return this->scheduled_queue.GetNext(core, member);
    }

    constexpr Member* GetSuggestedNext(s32 core, const Member* member) const {
        return this->suggested_queue.GetNext(core, member);
    }

    constexpr Member* GetSamePriorityNext(s32 core, const Member* member) const {
        return member->GetPriorityQueueEntry(core).GetNext();
    }

    // Mutators.
    constexpr void PushBack(Member* member) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return;
        }

        this->PushBack(member->GetPriority(), member);
    }

    constexpr void Remove(Member* member) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return;
        }

        this->Remove(member->GetPriority(), member);
    }

    constexpr void MoveToScheduledFront(Member* member) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return;
        }

        this->scheduled_queue.MoveToFront(member->GetPriority(), member->GetActiveCore(), member);
    }

    constexpr KThread* MoveToScheduledBack(Member* member) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return {};
        }

        return this->scheduled_queue.MoveToBack(member->GetPriority(), member->GetActiveCore(),
                                                member);
    }

    // First class fancy operations.
    constexpr void ChangePriority(s32 prev_priority, bool is_running, Member* member) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return;
        }

        ASSERT(IsValidPriority(prev_priority));

        // Remove the member from the queues.
        const s32 new_priority = member->GetPriority();
        this->Remove(prev_priority, member);

        // And enqueue. If the member is running, we want to keep it running.
        if (is_running) {
            this->PushFront(new_priority, member);
        } else {
            this->PushBack(new_priority, member);
        }
    }

    constexpr void ChangeAffinityMask(s32 prev_core, const AffinityMaskType& prev_affinity,
                                      Member* member) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return;
        }

        // Get the new information.
        const s32 priority = member->GetPriority();
        const AffinityMaskType& new_affinity = member->GetAffinityMask();
        const s32 new_core = member->GetActiveCore();

        // Remove the member from all queues it was in before.
        for (s32 core = 0; core < static_cast<s32>(NumCores); core++) {
            if (prev_affinity.GetAffinity(core)) {
                if (core == prev_core) {
                    this->scheduled_queue.Remove(priority, core, member);
                } else {
                    this->suggested_queue.Remove(priority, core, member);
                }
            }
        }

        // And add the member to all queues it should be in now.
        for (s32 core = 0; core < static_cast<s32>(NumCores); core++) {
            if (new_affinity.GetAffinity(core)) {
                if (core == new_core) {
                    this->scheduled_queue.PushBack(priority, core, member);
                } else {
                    this->suggested_queue.PushBack(priority, core, member);
                }
            }
        }
    }

    constexpr void ChangeCore(s32 prev_core, Member* member, bool to_front = false) {
        // This is for host (dummy) threads that we do not want to enter the priority queue.
        if (member->IsDummyThread()) {
            return;
        }

        // Get the new information.
        const s32 new_core = member->GetActiveCore();
        const s32 priority = member->GetPriority();

        // We don't need to do anything if the core is the same.
        if (prev_core != new_core) {
            // Remove from the scheduled queue for the previous core.
            if (prev_core >= 0) {
                this->scheduled_queue.Remove(priority, prev_core, member);
            }

            // Remove from the suggested queue and add to the scheduled queue for the new core.
            if (new_core >= 0) {
                this->suggested_queue.Remove(priority, new_core, member);
                if (to_front) {
                    this->scheduled_queue.PushFront(priority, new_core, member);
                } else {
                    this->scheduled_queue.PushBack(priority, new_core, member);
                }
            }

            // Add to the suggested queue for the previous core.
            if (prev_core >= 0) {
                this->suggested_queue.PushBack(priority, prev_core, member);
            }
        }
    }
};

} // namespace Kernel
