// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/uuid.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/service/time/clock_types.h"

namespace Service::Time {

class SharedMemory final {
public:
    explicit SharedMemory(Core::System& system_);
    ~SharedMemory();

    // TODO(ogniK): We have to properly simulate memory barriers, how are we going to do this?
    template <typename T, std::size_t Offset>
    struct MemoryBarrier {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        u32_le read_attempt{};
        std::array<T, 2> data{};

        // These are not actually memory barriers at the moment as we don't have multicore and all
        // HLE is mutexed. This will need to properly be implemented when we start updating the time
        // points on threads. As of right now, we'll be updated both values synchronously and just
        // incrementing the read_attempt to indicate that we waited.
        void StoreData(u8* shared_memory, T data_to_store) {
            std::memcpy(this, shared_memory + Offset, sizeof(*this));
            read_attempt++;
            data[read_attempt & 1] = data_to_store;
            std::memcpy(shared_memory + Offset, this, sizeof(*this));
        }

        // For reading we're just going to read the last stored value. If there was no value stored
        // it will just end up reading an empty value as intended.
        T ReadData(u8* shared_memory) {
            std::memcpy(this, shared_memory + Offset, sizeof(*this));
            return data[(read_attempt - 1) & 1];
        }
    };

    // Shared memory format
    struct Format {
        MemoryBarrier<Clock::SteadyClockContext, 0x0> standard_steady_clock_timepoint;
        MemoryBarrier<Clock::SystemClockContext, 0x38> standard_local_system_clock_context;
        MemoryBarrier<Clock::SystemClockContext, 0x80> standard_network_system_clock_context;
        MemoryBarrier<bool, 0xc8> standard_user_system_clock_automatic_correction;
        u32_le format_version;
    };
    static_assert(sizeof(Format) == 0xd8, "Format is an invalid size");

    void SetupStandardSteadyClock(const Common::UUID& clock_source_id,
                                  Clock::TimeSpanType current_time_point);
    void UpdateLocalSystemClockContext(const Clock::SystemClockContext& context);
    void UpdateNetworkSystemClockContext(const Clock::SystemClockContext& context);
    void SetAutomaticCorrectionEnabled(bool is_enabled);

private:
    Core::System& system;
    Format shared_memory_format{};
};

} // namespace Service::Time
