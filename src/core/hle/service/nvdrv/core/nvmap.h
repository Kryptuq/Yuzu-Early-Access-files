// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-FileCopyrightText: 2022 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <assert.h>

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hle/service/nvdrv/nvdata.h"

namespace Tegra {

namespace Host1x {
class Host1x;
} // namespace Host1x

} // namespace Tegra

namespace Service::Nvidia::NvCore {
/**
 * @brief The nvmap core class holds the global state for nvmap and provides methods to manage
 * handles
 */
class NvMap {
public:
    /**
     * @brief A handle to a contiguous block of memory in an application's address space
     */
    struct Handle {
        std::mutex mutex;

        u64 align{};      //!< The alignment to use when pinning the handle onto the SMMU
        u64 size;         //!< Page-aligned size of the memory the handle refers to
        u64 aligned_size; //!< `align`-aligned size of the memory the handle refers to
        u64 orig_size;    //!< Original unaligned size of the memory this handle refers to

        s32 dupes{1};          //!< How many guest references there are to this handle
        s32 internal_dupes{0}; //!< How many emulator-internal references there are to this handle

        using Id = u32;
        Id id; //!< A globally unique identifier for this handle

        s32 pins{};
        u32 pin_virt_address{};
        std::optional<typename std::list<std::shared_ptr<Handle>>::iterator> unmap_queue_entry{};

        union Flags {
            u32 raw;
            BitField<0, 1, u32> map_uncached; //!< If the handle should be mapped as uncached
            BitField<2, 1, u32> keep_uncached_after_free; //!< Only applicable when the handle was
                                                          //!< allocated with a fixed address
            BitField<4, 1, u32> _unk0_;                   //!< Passed to IOVMM for pins
        } flags{};
        static_assert(sizeof(Flags) == sizeof(u32));

        u64 address{}; //!< The memory location in the guest's AS that this handle corresponds to,
                       //!< this can also be in the nvdrv tmem
        bool is_shared_mem_mapped{}; //!< If this nvmap has been mapped with the MapSharedMem IPC
                                     //!< call

        u8 kind{};        //!< Used for memory compression
        bool allocated{}; //!< If the handle has been allocated with `Alloc`

        u64 dma_map_addr{}; //! remove me after implementing pinning.

        Handle(u64 size, Id id);

        /**
         * @brief Sets up the handle with the given memory config, can allocate memory from the tmem
         * if a 0 address is passed
         */
        [[nodiscard]] NvResult Alloc(Flags pFlags, u32 pAlign, u8 pKind, u64 pAddress);

        /**
         * @brief Increases the dupe counter of the handle for the given session
         */
        [[nodiscard]] NvResult Duplicate(bool internal_session);

        /**
         * @brief Obtains a pointer to the handle's memory and marks the handle it as having been
         * mapped
         */
        u8* GetPointer() {
            if (!address) {
                return nullptr;
            }

            is_shared_mem_mapped = true;
            return reinterpret_cast<u8*>(address);
        }
    };

private:
    std::list<std::shared_ptr<Handle>> unmap_queue{};
    std::mutex unmap_queue_lock{}; //!< Protects access to `unmap_queue`

    std::unordered_map<Handle::Id, std::shared_ptr<Handle>>
        handles{};           //!< Main owning map of handles
    std::mutex handles_lock; //!< Protects access to `handles`

    static constexpr u32 HandleIdIncrement{
        4}; //!< Each new handle ID is an increment of 4 from the previous
    std::atomic<u32> next_handle_id{HandleIdIncrement};
    Tegra::Host1x::Host1x& host1x;

    void AddHandle(std::shared_ptr<Handle> handle);

    /**
     * @brief Unmaps and frees the SMMU memory region a handle is mapped to
     * @note Both `unmap_queue_lock` and `handle_description.mutex` MUST be locked when calling this
     */
    void UnmapHandle(Handle& handle_description);

    /**
     * @brief Removes a handle from the map taking its dupes into account
     * @note handle_description.mutex MUST be locked when calling this
     * @return If the handle was removed from the map
     */
    bool TryRemoveHandle(const Handle& handle_description);

public:
    /**
     * @brief Encapsulates the result of a FreeHandle operation
     */
    struct FreeInfo {
        u64 address;       //!< Address the handle referred to before deletion
        u64 size;          //!< Page-aligned handle size
        bool was_uncached; //!< If the handle was allocated as uncached
    };

    NvMap(Tegra::Host1x::Host1x& host1x);

    /**
     * @brief Creates an unallocated handle of the given size
     */
    [[nodiscard]] NvResult CreateHandle(u64 size, std::shared_ptr<NvMap::Handle>& result_out);

    std::shared_ptr<Handle> GetHandle(Handle::Id handle);

    VAddr GetHandleAddress(Handle::Id handle);

    /**
     * @brief Maps a handle into the SMMU address space
     * @note This operation is refcounted, the number of calls to this must eventually match the
     * number of calls to `UnpinHandle`
     * @return The SMMU virtual address that the handle has been mapped to
     */
    u32 PinHandle(Handle::Id handle);

    /**
     * @brief When this has been called an equal number of times to `PinHandle` for the supplied
     * handle it will be added to a list of handles to be freed when necessary
     */
    void UnpinHandle(Handle::Id handle);

    /**
     * @brief Tries to duplicate a handle
     */
    void DuplicateHandle(Handle::Id handle, bool internal_session = false);

    /**
     * @brief Tries to free a handle and remove a single dupe
     * @note If a handle has no dupes left and has no other users a FreeInfo struct will be returned
     * describing the prior state of the handle
     */
    std::optional<FreeInfo> FreeHandle(Handle::Id handle, bool internal_session);
};
} // namespace Service::Nvidia::NvCore
