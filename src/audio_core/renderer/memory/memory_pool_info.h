// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * CPU pools are mapped in user memory with the supplied process_handle (see PoolMapper).
 */
class MemoryPoolInfo {
public:
    /**
     * The location of this pool.
     * CPU pools are mapped in user memory with the supplied process_handle (see PoolMapper).
     * DSP pools are mapped in the current process sysmodule.
     */
    enum class Location {
        CPU = 1,
        DSP = 2,
    };

    /**
     * Current state of the pool
     */
    enum class State {
        Invalid,
        Acquired,
        RequestDetach,
        Detached,
        RequestAttach,
        Attached,
        Released,
    };

    /**
     * Result code for updating the pool (See InfoUpdater::Update)
     */
    enum class ResultState {
        Success,
        BadParam,
        MapFailed,
        InUse,
    };

    /**
     * Input parameters coming from the game which are used to update current pools
     * (See InfoUpdater::Update)
     */
    struct InParameter {
        /* 0x00 */ u64 address;
        /* 0x08 */ u64 size;
        /* 0x10 */ State state;
        /* 0x14 */ bool in_use;
        /* 0x18 */ char unk18[0x8];
    };
    static_assert(sizeof(InParameter) == 0x20, "MemoryPoolInfo::InParameter has the wrong size!");

    /**
     * Output status sent back to the game on update (See InfoUpdater::Update)
     */
    struct OutStatus {
        /* 0x00 */ State state;
        /* 0x04 */ char unk04[0xC];
    };
    static_assert(sizeof(OutStatus) == 0x10, "MemoryPoolInfo::OutStatus has the wrong size!");

    MemoryPoolInfo() = default;
    MemoryPoolInfo(Location location_) : location{location_} {}

    /**
     * Get the CPU address for this pool.
     *
     * @return The CPU address of this pool.
     */
    CpuAddr GetCpuAddress() const;

    /**
     * Get the DSP address for this pool.
     *
     * @return The DSP address of this pool.
     */
    CpuAddr GetDspAddress() const;

    /**
     * Get the size of this pool.
     *
     * @return The size of this pool.
     */
    u64 GetSize() const;

    /**
     * Get the location of this pool.
     *
     * @return The location for the pool (see MemoryPoolInfo::Location).
     */
    Location GetLocation() const;

    /**
     * Set the CPU address for this pool.
     *
     * @param address - The new CPU address for this pool.
     * @param size    - The new size for this pool.
     */
    void SetCpuAddress(CpuAddr address, u64 size);

    /**
     * Set the DSP address for this pool.
     *
     * @param address - The new DSP address for this pool.
     */
    void SetDspAddress(CpuAddr address);

    /**
     * Check whether the pool contains a given range.
     *
     * @param address - The buffer address to look for.
     * @param size    - The size of the given buffer.
     * @return True if the range is within this pool, otherwise false.
     */
    bool Contains(CpuAddr address, u64 size) const;

    /**
     * Check whether this pool is mapped, which is when the dsp address is set.
     *
     * @return True if the pool is mapped, otherwise false.
     */
    bool IsMapped() const;

    /**
     * Translates a given CPU range into a relative offset for the DSP.
     *
     * @param address - The buffer address to look for.
     * @param size    - The size of the given buffer.
     * @return Pointer to the DSP-mapped memory.
     */
    CpuAddr Translate(CpuAddr address, u64 size) const;

    /**
     * Set or unset whether this memory pool is in use.
     *
     * @param used - Use state for this pool.
     */
    void SetUsed(bool used);

    /**
     * Get whether this pool is in use.
     *
     * @return True if in use, otherwise false.
     */
    bool IsUsed() const;

private:
    /// Base address for the CPU-side memory
    CpuAddr cpu_address{};
    /// Base address for the DSP-side memory
    CpuAddr dsp_address{};
    /// Size of this pool
    u64 size{};
    /// Location of this pool, either CPU or DSP
    Location location{Location::DSP};
    /// If this pool is in use
    bool in_use{};
};

} // namespace AudioCore::AudioRenderer
