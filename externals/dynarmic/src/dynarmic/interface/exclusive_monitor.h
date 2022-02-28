/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <dynarmic/common/spin_lock.h>

namespace Dynarmic {

using VAddr = std::uint64_t;
using Vector = std::array<std::uint64_t, 2>;

class ExclusiveMonitor {
public:
    /// @param processor_count Maximum number of processors using this global
    ///                        exclusive monitor. Each processor must have a
    ///                        unique id.
    explicit ExclusiveMonitor(size_t processor_count);

    size_t GetProcessorCount() const;

    /// Marks a region containing [address, address+size) to be exclusive to
    /// processor processor_id.
    template<typename T, typename Function>
    T ReadAndMark(size_t processor_id, VAddr address, Function op) {
        static_assert(std::is_trivially_copyable_v<T>);
        const VAddr masked_address = address & RESERVATION_GRANULE_MASK;

        Lock();
        exclusive_addresses[processor_id] = masked_address;
        const T value = op();
        std::memcpy(exclusive_values[processor_id].data(), &value, sizeof(T));
        Unlock();
        return value;
    }

    /// Checks to see if processor processor_id has exclusive access to the
    /// specified region. If it does, executes the operation then clears
    /// the exclusive state for processors if their exclusive region(s)
    /// contain [address, address+size).
    template<typename T, typename Function>
    bool DoExclusiveOperation(size_t processor_id, VAddr address, Function op) {
        static_assert(std::is_trivially_copyable_v<T>);
        if (!CheckAndClear(processor_id, address)) {
            return false;
        }

        T saved_value;
        std::memcpy(&saved_value, exclusive_values[processor_id].data(), sizeof(T));
        const bool result = op(saved_value);

        Unlock();
        return result;
    }

    /// Unmark everything.
    void Clear();
    /// Unmark processor id
    void ClearProcessor(size_t processor_id);

private:
    bool CheckAndClear(size_t processor_id, VAddr address);

    void Lock();
    void Unlock();

    friend volatile int* GetExclusiveMonitorLockPointer(ExclusiveMonitor*);
    friend size_t GetExclusiveMonitorProcessorCount(ExclusiveMonitor*);
    friend VAddr* GetExclusiveMonitorAddressPointer(ExclusiveMonitor*, size_t index);
    friend Vector* GetExclusiveMonitorValuePointer(ExclusiveMonitor*, size_t index);

    static constexpr VAddr RESERVATION_GRANULE_MASK = 0xFFFF'FFFF'FFFF'FFFFull;
    static constexpr VAddr INVALID_EXCLUSIVE_ADDRESS = 0xDEAD'DEAD'DEAD'DEADull;
    SpinLock lock;
    std::vector<VAddr> exclusive_addresses;
    std::vector<Vector> exclusive_values;
};

}  // namespace Dynarmic
