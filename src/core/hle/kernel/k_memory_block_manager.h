// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <functional>

#include "common/common_funcs.h"
#include "core/hle/kernel/k_dynamic_resource_manager.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_typed_address.h"

namespace Kernel {

class KMemoryBlockManagerUpdateAllocator {
public:
    static constexpr size_t MaxBlocks = 2;

private:
    std::array<KMemoryBlock*, MaxBlocks> m_blocks{};
    size_t m_index{MaxBlocks};
    KMemoryBlockSlabManager* m_slab_manager{};

private:
    Result Initialize(size_t num_blocks) {
        // Check num blocks.
        ASSERT(num_blocks <= MaxBlocks);

        // Set index.
        m_index = MaxBlocks - num_blocks;

        // Allocate the blocks.
        for (size_t i = 0; i < num_blocks && i < MaxBlocks; ++i) {
            m_blocks[m_index + i] = m_slab_manager->Allocate();
            R_UNLESS(m_blocks[m_index + i] != nullptr, ResultOutOfResource);
        }

        R_SUCCEED();
    }

public:
    KMemoryBlockManagerUpdateAllocator(Result* out_result, KMemoryBlockSlabManager* sm,
                                       size_t num_blocks = MaxBlocks)
        : m_slab_manager(sm) {
        *out_result = this->Initialize(num_blocks);
    }

    ~KMemoryBlockManagerUpdateAllocator() {
        for (const auto& block : m_blocks) {
            if (block != nullptr) {
                m_slab_manager->Free(block);
            }
        }
    }

    KMemoryBlock* Allocate() {
        ASSERT(m_index < MaxBlocks);
        ASSERT(m_blocks[m_index] != nullptr);
        KMemoryBlock* block = nullptr;
        std::swap(block, m_blocks[m_index++]);
        return block;
    }

    void Free(KMemoryBlock* block) {
        ASSERT(m_index <= MaxBlocks);
        ASSERT(block != nullptr);
        if (m_index == 0) {
            m_slab_manager->Free(block);
        } else {
            m_blocks[--m_index] = block;
        }
    }
};

class KMemoryBlockManager final {
public:
    using MemoryBlockTree =
        Common::IntrusiveRedBlackTreeBaseTraits<KMemoryBlock>::TreeType<KMemoryBlock>;
    using MemoryBlockLockFunction = void (KMemoryBlock::*)(KMemoryPermission new_perm, bool left,
                                                           bool right);
    using iterator = MemoryBlockTree::iterator;
    using const_iterator = MemoryBlockTree::const_iterator;

public:
    KMemoryBlockManager();

    using HostUnmapCallback = std::function<void(Common::ProcessAddress, u64)>;

    Result Initialize(KProcessAddress st, KProcessAddress nd,
                      KMemoryBlockSlabManager* slab_manager);
    void Finalize(KMemoryBlockSlabManager* slab_manager, HostUnmapCallback&& host_unmap_callback);

    iterator end() {
        return m_memory_block_tree.end();
    }
    const_iterator end() const {
        return m_memory_block_tree.end();
    }
    const_iterator cend() const {
        return m_memory_block_tree.cend();
    }

    KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages,
                                 size_t num_pages, size_t alignment, size_t offset,
                                 size_t guard_pages) const;

    void Update(KMemoryBlockManagerUpdateAllocator* allocator, KProcessAddress address,
                size_t num_pages, KMemoryState state, KMemoryPermission perm, KMemoryAttribute attr,
                KMemoryBlockDisableMergeAttribute set_disable_attr,
                KMemoryBlockDisableMergeAttribute clear_disable_attr);
    void UpdateLock(KMemoryBlockManagerUpdateAllocator* allocator, KProcessAddress address,
                    size_t num_pages, MemoryBlockLockFunction lock_func, KMemoryPermission perm);

    void UpdateIfMatch(KMemoryBlockManagerUpdateAllocator* allocator, KProcessAddress address,
                       size_t num_pages, KMemoryState test_state, KMemoryPermission test_perm,
                       KMemoryAttribute test_attr, KMemoryState state, KMemoryPermission perm,
                       KMemoryAttribute attr);

    iterator FindIterator(KProcessAddress address) const {
        return m_memory_block_tree.find(KMemoryBlock(
            address, 1, KMemoryState::Free, KMemoryPermission::None, KMemoryAttribute::None));
    }

    const KMemoryBlock* FindBlock(KProcessAddress address) const {
        if (const_iterator it = this->FindIterator(address); it != m_memory_block_tree.end()) {
            return std::addressof(*it);
        }

        return nullptr;
    }

    // Debug.
    bool CheckState() const;

private:
    void CoalesceForUpdate(KMemoryBlockManagerUpdateAllocator* allocator, KProcessAddress address,
                           size_t num_pages);

    MemoryBlockTree m_memory_block_tree;
    KProcessAddress m_start_address{};
    KProcessAddress m_end_address{};
};

class KScopedMemoryBlockManagerAuditor {
public:
    explicit KScopedMemoryBlockManagerAuditor(KMemoryBlockManager* m) : m_manager(m) {
        ASSERT(m_manager->CheckState());
    }
    explicit KScopedMemoryBlockManagerAuditor(KMemoryBlockManager& m)
        : KScopedMemoryBlockManagerAuditor(std::addressof(m)) {}
    ~KScopedMemoryBlockManagerAuditor() {
        ASSERT(m_manager->CheckState());
    }

private:
    KMemoryBlockManager* m_manager;
};

} // namespace Kernel
