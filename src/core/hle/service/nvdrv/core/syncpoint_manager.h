// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-FileCopyrightText: 2022 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <mutex>

#include "common/common_types.h"
#include "core/hle/service/nvdrv/nvdata.h"

namespace Tegra::Host1x {
class Host1x;
} // namespace Tegra::Host1x

namespace Service::Nvidia::NvCore {

enum class ChannelType : u32 {
    MsEnc = 0,
    VIC = 1,
    GPU = 2,
    NvDec = 3,
    Display = 4,
    NvJpg = 5,
    TSec = 6,
    Max = 7
};

/**
 * @brief SyncpointManager handles allocating and accessing host1x syncpoints, these are cached
 * versions of the HW syncpoints which are intermittently synced
 * @note Refer to Chapter 14 of the Tegra X1 TRM for an exhaustive overview of them
 * @url https://http.download.nvidia.com/tegra-public-appnotes/host1x.html
 * @url
 * https://github.com/Jetson-TX1-AndroidTV/android_kernel_jetson_tx1_hdmi_primary/blob/jetson-tx1/drivers/video/tegra/host/nvhost_syncpt.c
 */
class SyncpointManager final {
public:
    explicit SyncpointManager(Tegra::Host1x::Host1x& host1x);
    ~SyncpointManager();

    /**
     * @brief Checks if the given syncpoint is both allocated and below the number of HW syncpoints
     */
    bool IsSyncpointAllocated(u32 id);

    /**
     * @brief Finds a free syncpoint and reserves it
     * @return The ID of the reserved syncpoint
     */
    u32 AllocateSyncpoint(bool client_managed);

    /**
     * @url
     * https://github.com/Jetson-TX1-AndroidTV/android_kernel_jetson_tx1_hdmi_primary/blob/8f74a72394efb871cb3f886a3de2998cd7ff2990/drivers/gpu/host1x/syncpt.c#L259
     */
    bool HasSyncpointExpired(u32 id, u32 threshold) const;

    bool IsFenceSignalled(NvFence fence) const {
        return HasSyncpointExpired(fence.id, fence.value);
    }

    /**
     * @brief Atomically increments the maximum value of a syncpoint by the given amount
     * @return The new max value of the syncpoint
     */
    u32 IncrementSyncpointMaxExt(u32 id, u32 amount);

    /**
     * @return The minimum value of the syncpoint
     */
    u32 ReadSyncpointMinValue(u32 id);

    /**
     * @brief Synchronises the minimum value of the syncpoint to with the GPU
     * @return The new minimum value of the syncpoint
     */
    u32 UpdateMin(u32 id);

    /**
     * @brief Frees the usage of a syncpoint.
     */
    void FreeSyncpoint(u32 id);

    /**
     * @return A fence that will be signalled once this syncpoint hits its maximum value
     */
    NvFence GetSyncpointFence(u32 id);

    static constexpr std::array<u32, static_cast<u32>(ChannelType::Max)> channel_syncpoints{
        0x0,  // `MsEnc` is unimplemented
        0xC,  // `VIC`
        0x0,  // `GPU` syncpoints are allocated per-channel instead
        0x36, // `NvDec`
        0x0,  // `Display` is unimplemented
        0x37, // `NvJpg`
        0x0,  // `TSec` is unimplemented
    };        //!< Maps each channel ID to a constant syncpoint

private:
    /**
     * @note reservation_lock should be locked when calling this
     */
    u32 ReserveSyncpoint(u32 id, bool client_managed);

    /**
     * @return The ID of the first free syncpoint
     */
    u32 FindFreeSyncpoint();

    struct SyncpointInfo {
        std::atomic<u32> counter_min; //!< The least value the syncpoint can be (The value it was
                                      //!< when it was last synchronized with host1x)
        std::atomic<u32> counter_max; //!< The maximum value the syncpoint can reach according to
                                      //!< the current usage
        bool interface_managed; //!< If the syncpoint is managed by a host1x client interface, a
                                //!< client interface is a HW block that can handle host1x
                                //!< transactions on behalf of a host1x client (Which would
                                //!< otherwise need to be manually synced using PIO which is
                                //!< synchronous and requires direct cooperation of the CPU)
        bool reserved; //!< If the syncpoint is reserved or not, not to be confused with a reserved
                       //!< value
    };

    constexpr static std::size_t SyncpointCount{192};
    std::array<SyncpointInfo, SyncpointCount> syncpoints{};
    std::mutex reservation_lock;

    Tegra::Host1x::Host1x& host1x;
};

} // namespace Service::Nvidia::NvCore
