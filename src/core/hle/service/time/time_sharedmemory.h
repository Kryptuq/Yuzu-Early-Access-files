// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/uuid.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/service/time/clock_types.h"

namespace Service::Time {

// Note: this type is not safe for concurrent writes.
template <typename T>
struct LockFreeAtomicType {
    u32 counter_;
    std::array<T, 2> value_;
};

template <typename T>
static inline void StoreToLockFreeAtomicType(LockFreeAtomicType<T>* p, const T& value) {
    // Get the current counter.
    auto counter = p->counter_;

    // Increment the counter.
    ++counter;

    // Store the updated value.
    p->value_[counter % 2] = value;

    // Fence memory.
    std::atomic_thread_fence(std::memory_order_release);

    // Set the updated counter.
    p->counter_ = counter;
}

template <typename T>
static inline T LoadFromLockFreeAtomicType(const LockFreeAtomicType<T>* p) {
    while (true) {
        // Get the counter.
        auto counter = p->counter_;

        // Get the value.
        auto value = p->value_[counter % 2];

        // Fence memory.
        std::atomic_thread_fence(std::memory_order_acquire);

        // Check that the counter matches.
        if (counter == p->counter_) {
            return value;
        }
    }
}

class SharedMemory final {
public:
    explicit SharedMemory(Core::System& system_);
    ~SharedMemory();

    // Shared memory format
    struct Format {
        LockFreeAtomicType<Clock::StandardSteadyClockTimePointType> standard_steady_clock_timepoint;
        LockFreeAtomicType<Clock::SystemClockContext> standard_local_system_clock_context;
        LockFreeAtomicType<Clock::SystemClockContext> standard_network_system_clock_context;
        LockFreeAtomicType<bool> is_standard_user_system_clock_automatic_correction_enabled;
        u32 format_version;
    };
    static_assert(offsetof(Format, standard_steady_clock_timepoint) == 0x0);
    static_assert(offsetof(Format, standard_local_system_clock_context) == 0x38);
    static_assert(offsetof(Format, standard_network_system_clock_context) == 0x80);
    static_assert(offsetof(Format, is_standard_user_system_clock_automatic_correction_enabled) ==
                  0xc8);
    static_assert(sizeof(Format) == 0xd8, "Format is an invalid size");

    void SetupStandardSteadyClock(const Common::UUID& clock_source_id,
                                  Clock::TimeSpanType current_time_point);
    void UpdateLocalSystemClockContext(const Clock::SystemClockContext& context);
    void UpdateNetworkSystemClockContext(const Clock::SystemClockContext& context);
    void SetAutomaticCorrectionEnabled(bool is_enabled);
    Format* GetFormat();

private:
    Core::System& system;
};

} // namespace Service::Time
