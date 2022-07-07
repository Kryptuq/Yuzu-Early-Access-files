// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/memory/memory_pool_info.h"
#include "common/common_types.h"
#include "core/hle/service/audio/errors.h"

namespace AudioCore::AudioRenderer {
class AddressInfo;

/**
 * Utility functions for managing MemoryPoolInfos
 */
class PoolMapper {
public:
    explicit PoolMapper(u32 process_handle, bool force_map);
    explicit PoolMapper(u32 process_handle, std::span<MemoryPoolInfo> pool_infos, u32 pool_count,
                        bool force_map);

    /**
     * Clear the usage state for all given pools.
     *
     * @param pools - The memory pools to clear.
     * @param count - The number of pools.
     */
    static void ClearUseState(std::span<MemoryPoolInfo> pools, u32 count);

    /**
     * Find the memory pool containing the given address and size from a given list of pools.
     *
     * @param pools   - The memory pools to search within.
     * @param count   - The number of pools.
     * @param address - The address of the region to find.
     * @param size    - The size of the region to find.
     * @return Pointer to the memory pool if found, otherwise nullptr.
     */
    MemoryPoolInfo* FindMemoryPool(MemoryPoolInfo* pools, u64 count, CpuAddr address,
                                   u64 size) const;

    /**
     * Find the memory pool containing the given address and size from the PoolMapper's memory pool.
     *
     * @param address - The address of the region to find.
     * @param size    - The size of the region to find.
     * @return Pointer to the memory pool if found, otherwise nullptr.
     */
    MemoryPoolInfo* FindMemoryPool(CpuAddr address, u64 size) const;

    /**
     * Set the PoolMapper's memory pool to one in the given list of pools, which contains
     * address_info.
     *
     * @param address_info - The expected region to find within pools.
     * @param pools        - The list of pools to search within.
     * @param count        - The number of pools given.
     * @return True if successfully mapped, otherwise false.
     */
    bool FillDspAddr(AddressInfo& address_info, MemoryPoolInfo* pools, u32 count) const;

    /**
     * Set the PoolMapper's memory pool to the one containing address_info.
     *
     * @param address_info - The address to find the memory pool for.
     * @return True if successfully mapped, otherwise false.
     */
    bool FillDspAddr(AddressInfo& address_info) const;

    /**
     * Try to attach a {address, size} region to the given address_info, and map it. Fills in the
     * given error_info and address_info.
     *
     * @param error_info   - Output error info.
     * @param address_info - Output address info, initialized with the given {address, size} and
     *                       attempted to map.
     * @param address      - Address of the region to map.
     * @param size         - Size of the region to map.
     * @return True if successfully attached, otherwise false.
     */
    bool TryAttachBuffer(BehaviorInfo::ErrorInfo& error_info, AddressInfo& address_info,
                         CpuAddr address, u64 size) const;

    /**
     * Return whether force mapping is enabled.
     *
     * @return True if force mapping is enabled, otherwise false.
     */
    bool IsForceMapEnabled() const;

    /**
     * Get the process handle, depending on location.
     *
     * @param pool - The pool to check the location of.
     * @return CurrentProcessHandle if location == DSP,
     *         the PoolMapper's process_handle if location == CPU
     */
    u32 GetProcessHandle(const MemoryPoolInfo* pool) const;

    /**
     * Map the given region with the given handle. This is a no-op.
     *
     * @param handle   - The process handle to map to.
     * @param cpu_addr - Address to map.
     * @param size     - Size to map.
     * @return True if successfully mapped, otherwise false.
     */
    bool Map(u32 handle, CpuAddr cpu_addr, u64 size) const;

    /**
     * Map the given memory pool.
     *
     * @param pool - The pool to map.
     * @return True if successfully mapped, otherwise false.
     */
    bool Map(MemoryPoolInfo& pool) const;

    /**
     * Unmap the given region with the given handle.
     *
     * @param handle   - The process handle to unmap to.
     * @param cpu_addr - Address to unmap.
     * @param size     - Size to unmap.
     * @return True if successfully unmapped, otherwise false.
     */
    bool Unmap(u32 handle, CpuAddr cpu_addr, u64 size) const;

    /**
     * Unmap the given memory pool.
     *
     * @param pool - The pool to unmap.
     * @return True if successfully unmapped, otherwise false.
     */
    bool Unmap(MemoryPoolInfo& pool) const;

    /**
     * Forcibly unmap the given region.
     *
     * @param address_info - The region to unmap.
     */
    void ForceUnmapPointer(const AddressInfo& address_info) const;

    /**
     * Update the given memory pool.
     *
     * @param pool       - Pool to update.
     * @param in_params  - Input parameters for the update.
     * @param out_params - Output parameters for the update.
     * @return The result of the update. See MemoryPoolInfo::ResultState
     */
    MemoryPoolInfo::ResultState Update(MemoryPoolInfo& pool,
                                       const MemoryPoolInfo::InParameter& in_params,
                                       MemoryPoolInfo::OutStatus& out_params) const;

    /**
     * Initialize the PoolMapper's memory pool.
     *
     * @param pool   - Input pool to initialize.
     * @param memory - Pointer to the memory region for the pool.
     * @param size   - Size of the memory region for the pool.
     * @return True if initialized successfully, otherwise false.
     */
    bool InitializeSystemPool(MemoryPoolInfo& pool, const u8* memory, u64 size) const;

private:
    /// Process handle for this mapper, used when location == CPU
    u32 process_handle;
    /// List of memory pools assigned to this mapper
    MemoryPoolInfo* pool_infos{};
    /// The number of pools
    u64 pool_count{};
    /// Is forced mapping enabled
    bool force_map;
};

} // namespace AudioCore::AudioRenderer
