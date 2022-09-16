// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/memory/memory_pool_info.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {

/**
 * Represents a region of mapped or unmapped memory.
 */
class AddressInfo {
public:
    AddressInfo() = default;
    AddressInfo(CpuAddr cpu_address_, u64 size_) : cpu_address{cpu_address_}, size{size_} {}

    /**
     * Setup a new AddressInfo.
     *
     * @param cpu_address_ - The CPU address of this region.
     * @param size_        - The size of this region.
     */
    void Setup(CpuAddr cpu_address_, u64 size_) {
        cpu_address = cpu_address_;
        size = size_;
        memory_pool = nullptr;
        dsp_address = 0;
    }

    /**
     * Get the CPU address.
     *
     * @return The CpuAddr address
     */
    CpuAddr GetCpuAddr() const {
        return cpu_address;
    }

    /**
     * Assign this region to a memory pool.
     *
     * @param memory_pool_ - Memory pool to assign.
     */
    void SetPool(MemoryPoolInfo* memory_pool_) {
        memory_pool = memory_pool_;
    }

    /**
     * Get the size of this region.
     *
     * @return The size of this region.
     */
    u64 GetSize() const {
        return size;
    }

    /**
     * Get the ADSP address for this region.
     *
     * @return The ADSP address for this region.
     */
    CpuAddr GetForceMappedDspAddr() const {
        return dsp_address;
    }

    /**
     * Set the ADSP address for this region.
     *
     * @param dsp_addr - The new ADSP address for this region.
     */
    void SetForceMappedDspAddr(CpuAddr dsp_addr) {
        dsp_address = dsp_addr;
    }

    /**
     * Check whether this region has an active memory pool.
     *
     * @return True if this region has a mapped memory pool, otherwise false.
     */
    bool HasMappedMemoryPool() const {
        return memory_pool != nullptr && memory_pool->GetDspAddress() != 0;
    }

    /**
     * Check whether this region is mapped to the ADSP.
     *
     * @return True if this region is mapped, otherwise false.
     */
    bool IsMapped() const {
        return HasMappedMemoryPool() || dsp_address != 0;
    }

    /**
     * Get a usable reference to this region of memory.
     *
     * @param mark_in_use - Whether this region should be marked as being in use.
     * @return A valid memory address if valid, otherwise 0.
     */
    CpuAddr GetReference(bool mark_in_use) {
        if (!HasMappedMemoryPool()) {
            return dsp_address;
        }

        if (mark_in_use) {
            memory_pool->SetUsed(true);
        }

        return memory_pool->Translate(cpu_address, size);
    }

private:
    /// CPU address of this region
    CpuAddr cpu_address;
    /// Size of this region
    u64 size;
    /// The memory this region is mapped to
    MemoryPoolInfo* memory_pool;
    /// ADSP address of this region
    CpuAddr dsp_address;
};

} // namespace AudioCore::AudioRenderer
