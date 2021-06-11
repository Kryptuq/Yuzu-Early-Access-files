// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <list>

#include "common/common_types.h"
#include "core/hle/kernel/k_memory_block.h"

namespace Kernel {

class KMemoryBlockManager final {
public:
    using MemoryBlockTree = std::list<KMemoryBlock>;
    using iterator = MemoryBlockTree::iterator;
    using const_iterator = MemoryBlockTree::const_iterator;

public:
    KMemoryBlockManager(VAddr start_addr_, VAddr end_addr_);

    iterator end() {
        return memory_block_tree.end();
    }
    const_iterator end() const {
        return memory_block_tree.end();
    }
    const_iterator cend() const {
        return memory_block_tree.cend();
    }

    iterator FindIterator(VAddr addr);

    VAddr FindFreeArea(VAddr region_start, std::size_t region_num_pages, std::size_t num_pages,
                       std::size_t align, std::size_t offset, std::size_t guard_pages);

    void Update(VAddr addr, std::size_t num_pages, KMemoryState prev_state,
                KMemoryPermission prev_perm, KMemoryAttribute prev_attribute, KMemoryState state,
                KMemoryPermission perm, KMemoryAttribute attribute);

    void Update(VAddr addr, std::size_t num_pages, KMemoryState state,
                KMemoryPermission perm = KMemoryPermission::None,
                KMemoryAttribute attribute = KMemoryAttribute::None);

    using LockFunc = std::function<void(iterator, KMemoryPermission)>;
    void UpdateLock(VAddr addr, std::size_t num_pages, LockFunc&& lock_func,
                    KMemoryPermission perm);

    using IterateFunc = std::function<void(const KMemoryInfo&)>;
    void IterateForRange(VAddr start, VAddr end, IterateFunc&& func);

    KMemoryBlock& FindBlock(VAddr addr) {
        return *FindIterator(addr);
    }

private:
    void MergeAdjacent(iterator it, iterator& next_it);

    [[maybe_unused]] const VAddr start_addr;
    [[maybe_unused]] const VAddr end_addr;

    MemoryBlockTree memory_block_tree;
};

} // namespace Kernel
