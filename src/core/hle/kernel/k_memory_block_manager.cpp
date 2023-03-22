// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_memory_block_manager.h"

namespace Kernel {

KMemoryBlockManager::KMemoryBlockManager() = default;

Result KMemoryBlockManager::Initialize(KProcessAddress st, KProcessAddress nd,
                                       KMemoryBlockSlabManager* slab_manager) {
    // Allocate a block to encapsulate the address space, insert it into the tree.
    KMemoryBlock* start_block = slab_manager->Allocate();
    R_UNLESS(start_block != nullptr, ResultOutOfResource);

    // Set our start and end.
    m_start_address = st;
    m_end_address = nd;
    ASSERT(Common::IsAligned(GetInteger(m_start_address), PageSize));
    ASSERT(Common::IsAligned(GetInteger(m_end_address), PageSize));

    // Initialize and insert the block.
    start_block->Initialize(m_start_address, (m_end_address - m_start_address) / PageSize,
                            KMemoryState::Free, KMemoryPermission::None, KMemoryAttribute::None);
    m_memory_block_tree.insert(*start_block);

    R_SUCCEED();
}

void KMemoryBlockManager::Finalize(KMemoryBlockSlabManager* slab_manager,
                                   HostUnmapCallback&& host_unmap_callback) {
    // Erase every block until we have none left.
    auto it = m_memory_block_tree.begin();
    while (it != m_memory_block_tree.end()) {
        KMemoryBlock* block = std::addressof(*it);
        it = m_memory_block_tree.erase(it);
        slab_manager->Free(block);
        host_unmap_callback(block->GetAddress(), block->GetSize());
    }

    ASSERT(m_memory_block_tree.empty());
}

KProcessAddress KMemoryBlockManager::FindFreeArea(KProcessAddress region_start,
                                                  size_t region_num_pages, size_t num_pages,
                                                  size_t alignment, size_t offset,
                                                  size_t guard_pages) const {
    if (num_pages > 0) {
        const KProcessAddress region_end = region_start + region_num_pages * PageSize;
        const KProcessAddress region_last = region_end - 1;
        for (const_iterator it = this->FindIterator(region_start); it != m_memory_block_tree.cend();
             it++) {
            const KMemoryInfo info = it->GetMemoryInfo();
            if (region_last < info.GetAddress()) {
                break;
            }
            if (info.m_state != KMemoryState::Free) {
                continue;
            }

            KProcessAddress area =
                (info.GetAddress() <= GetInteger(region_start)) ? region_start : info.GetAddress();
            area += guard_pages * PageSize;

            const KProcessAddress offset_area =
                Common::AlignDown(GetInteger(area), alignment) + offset;
            area = (area <= offset_area) ? offset_area : offset_area + alignment;

            const KProcessAddress area_end = area + num_pages * PageSize + guard_pages * PageSize;
            const KProcessAddress area_last = area_end - 1;

            if (info.GetAddress() <= GetInteger(area) && area < area_last &&
                area_last <= region_last && area_last <= info.GetLastAddress()) {
                return area;
            }
        }
    }

    return {};
}

void KMemoryBlockManager::CoalesceForUpdate(KMemoryBlockManagerUpdateAllocator* allocator,
                                            KProcessAddress address, size_t num_pages) {
    // Find the iterator now that we've updated.
    iterator it = this->FindIterator(address);
    if (address != m_start_address) {
        it--;
    }

    // Coalesce blocks that we can.
    while (true) {
        iterator prev = it++;
        if (it == m_memory_block_tree.end()) {
            break;
        }

        if (prev->CanMergeWith(*it)) {
            KMemoryBlock* block = std::addressof(*it);
            m_memory_block_tree.erase(it);
            prev->Add(*block);
            allocator->Free(block);
            it = prev;
        }

        if (address + num_pages * PageSize < it->GetMemoryInfo().GetEndAddress()) {
            break;
        }
    }
}

void KMemoryBlockManager::Update(KMemoryBlockManagerUpdateAllocator* allocator,
                                 KProcessAddress address, size_t num_pages, KMemoryState state,
                                 KMemoryPermission perm, KMemoryAttribute attr,
                                 KMemoryBlockDisableMergeAttribute set_disable_attr,
                                 KMemoryBlockDisableMergeAttribute clear_disable_attr) {
    // Ensure for auditing that we never end up with an invalid tree.
    KScopedMemoryBlockManagerAuditor auditor(this);
    ASSERT(Common::IsAligned(GetInteger(address), PageSize));
    ASSERT((attr & (KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared)) ==
           KMemoryAttribute::None);

    KProcessAddress cur_address = address;
    size_t remaining_pages = num_pages;
    iterator it = this->FindIterator(address);

    while (remaining_pages > 0) {
        const size_t remaining_size = remaining_pages * PageSize;
        KMemoryInfo cur_info = it->GetMemoryInfo();
        if (it->HasProperties(state, perm, attr)) {
            // If we already have the right properties, just advance.
            if (cur_address + remaining_size < cur_info.GetEndAddress()) {
                remaining_pages = 0;
                cur_address += remaining_size;
            } else {
                remaining_pages =
                    (cur_address + remaining_size - cur_info.GetEndAddress()) / PageSize;
                cur_address = cur_info.GetEndAddress();
            }
        } else {
            // If we need to, create a new block before and insert it.
            if (cur_info.GetAddress() != cur_address) {
                KMemoryBlock* new_block = allocator->Allocate();

                it->Split(new_block, cur_address);
                it = m_memory_block_tree.insert(*new_block);
                it++;

                cur_info = it->GetMemoryInfo();
                cur_address = cur_info.GetAddress();
            }

            // If we need to, create a new block after and insert it.
            if (cur_info.GetSize() > remaining_size) {
                KMemoryBlock* new_block = allocator->Allocate();

                it->Split(new_block, cur_address + remaining_size);
                it = m_memory_block_tree.insert(*new_block);

                cur_info = it->GetMemoryInfo();
            }

            // Update block state.
            it->Update(state, perm, attr, cur_address == address, static_cast<u8>(set_disable_attr),
                       static_cast<u8>(clear_disable_attr));
            cur_address += cur_info.GetSize();
            remaining_pages -= cur_info.GetNumPages();
        }
        it++;
    }

    this->CoalesceForUpdate(allocator, address, num_pages);
}

void KMemoryBlockManager::UpdateIfMatch(KMemoryBlockManagerUpdateAllocator* allocator,
                                        KProcessAddress address, size_t num_pages,
                                        KMemoryState test_state, KMemoryPermission test_perm,
                                        KMemoryAttribute test_attr, KMemoryState state,
                                        KMemoryPermission perm, KMemoryAttribute attr) {
    // Ensure for auditing that we never end up with an invalid tree.
    KScopedMemoryBlockManagerAuditor auditor(this);
    ASSERT(Common::IsAligned(GetInteger(address), PageSize));
    ASSERT((attr & (KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared)) ==
           KMemoryAttribute::None);

    KProcessAddress cur_address = address;
    size_t remaining_pages = num_pages;
    iterator it = this->FindIterator(address);

    while (remaining_pages > 0) {
        const size_t remaining_size = remaining_pages * PageSize;
        KMemoryInfo cur_info = it->GetMemoryInfo();
        if (it->HasProperties(test_state, test_perm, test_attr) &&
            !it->HasProperties(state, perm, attr)) {
            // If we need to, create a new block before and insert it.
            if (cur_info.GetAddress() != cur_address) {
                KMemoryBlock* new_block = allocator->Allocate();

                it->Split(new_block, cur_address);
                it = m_memory_block_tree.insert(*new_block);
                it++;

                cur_info = it->GetMemoryInfo();
                cur_address = cur_info.GetAddress();
            }

            // If we need to, create a new block after and insert it.
            if (cur_info.GetSize() > remaining_size) {
                KMemoryBlock* new_block = allocator->Allocate();

                it->Split(new_block, cur_address + remaining_size);
                it = m_memory_block_tree.insert(*new_block);

                cur_info = it->GetMemoryInfo();
            }

            // Update block state.
            it->Update(state, perm, attr, false, 0, 0);
            cur_address += cur_info.GetSize();
            remaining_pages -= cur_info.GetNumPages();
        } else {
            // If we already have the right properties, just advance.
            if (cur_address + remaining_size < cur_info.GetEndAddress()) {
                remaining_pages = 0;
                cur_address += remaining_size;
            } else {
                remaining_pages =
                    (cur_address + remaining_size - cur_info.GetEndAddress()) / PageSize;
                cur_address = cur_info.GetEndAddress();
            }
        }
        it++;
    }

    this->CoalesceForUpdate(allocator, address, num_pages);
}

void KMemoryBlockManager::UpdateLock(KMemoryBlockManagerUpdateAllocator* allocator,
                                     KProcessAddress address, size_t num_pages,
                                     MemoryBlockLockFunction lock_func, KMemoryPermission perm) {
    // Ensure for auditing that we never end up with an invalid tree.
    KScopedMemoryBlockManagerAuditor auditor(this);
    ASSERT(Common::IsAligned(GetInteger(address), PageSize));

    KProcessAddress cur_address = address;
    size_t remaining_pages = num_pages;
    iterator it = this->FindIterator(address);

    const KProcessAddress end_address = address + (num_pages * PageSize);

    while (remaining_pages > 0) {
        const size_t remaining_size = remaining_pages * PageSize;
        KMemoryInfo cur_info = it->GetMemoryInfo();

        // If we need to, create a new block before and insert it.
        if (cur_info.m_address != cur_address) {
            KMemoryBlock* new_block = allocator->Allocate();

            it->Split(new_block, cur_address);
            it = m_memory_block_tree.insert(*new_block);
            it++;

            cur_info = it->GetMemoryInfo();
            cur_address = cur_info.GetAddress();
        }

        if (cur_info.GetSize() > remaining_size) {
            // If we need to, create a new block after and insert it.
            KMemoryBlock* new_block = allocator->Allocate();

            it->Split(new_block, cur_address + remaining_size);
            it = m_memory_block_tree.insert(*new_block);

            cur_info = it->GetMemoryInfo();
        }

        // Call the locked update function.
        (std::addressof(*it)->*lock_func)(perm, cur_info.GetAddress() == address,
                                          cur_info.GetEndAddress() == end_address);
        cur_address += cur_info.GetSize();
        remaining_pages -= cur_info.GetNumPages();
        it++;
    }

    this->CoalesceForUpdate(allocator, address, num_pages);
}

// Debug.
bool KMemoryBlockManager::CheckState() const {
    // Loop over every block, ensuring that we are sorted and coalesced.
    auto it = m_memory_block_tree.cbegin();
    auto prev = it++;
    while (it != m_memory_block_tree.cend()) {
        const KMemoryInfo prev_info = prev->GetMemoryInfo();
        const KMemoryInfo cur_info = it->GetMemoryInfo();

        // Sequential blocks which can be merged should be merged.
        if (prev->CanMergeWith(*it)) {
            return false;
        }

        // Sequential blocks should be sequential.
        if (prev_info.GetEndAddress() != cur_info.GetAddress()) {
            return false;
        }

        // If the block is ipc locked, it must have a count.
        if ((cur_info.m_attribute & KMemoryAttribute::IpcLocked) != KMemoryAttribute::None &&
            cur_info.m_ipc_lock_count == 0) {
            return false;
        }

        // If the block is device shared, it must have a count.
        if ((cur_info.m_attribute & KMemoryAttribute::DeviceShared) != KMemoryAttribute::None &&
            cur_info.m_device_use_count == 0) {
            return false;
        }

        // Advance the iterator.
        prev = it++;
    }

    // Our loop will miss checking the last block, potentially, so check it.
    if (prev != m_memory_block_tree.cend()) {
        const KMemoryInfo prev_info = prev->GetMemoryInfo();
        // If the block is ipc locked, it must have a count.
        if ((prev_info.m_attribute & KMemoryAttribute::IpcLocked) != KMemoryAttribute::None &&
            prev_info.m_ipc_lock_count == 0) {
            return false;
        }

        // If the block is device shared, it must have a count.
        if ((prev_info.m_attribute & KMemoryAttribute::DeviceShared) != KMemoryAttribute::None &&
            prev_info.m_device_use_count == 0) {
            return false;
        }
    }

    return true;
}

} // namespace Kernel
