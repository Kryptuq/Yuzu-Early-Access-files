// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_memory_block_manager.h"
#include "core/hle/kernel/memory_types.h"

namespace Kernel {

KMemoryBlockManager::KMemoryBlockManager(VAddr start_addr_, VAddr end_addr_)
    : start_addr{start_addr_}, end_addr{end_addr_} {
    const u64 num_pages{(end_addr - start_addr) / PageSize};
    memory_block_tree.emplace_back(start_addr, num_pages, KMemoryState::Free,
                                   KMemoryPermission::None, KMemoryAttribute::None);
}

KMemoryBlockManager::iterator KMemoryBlockManager::FindIterator(VAddr addr) {
    auto node{memory_block_tree.begin()};
    while (node != end()) {
        const VAddr node_end_addr{node->GetNumPages() * PageSize + node->GetAddress()};
        if (node->GetAddress() <= addr && node_end_addr - 1 >= addr) {
            return node;
        }
        node = std::next(node);
    }
    return end();
}

VAddr KMemoryBlockManager::FindFreeArea(VAddr region_start, std::size_t region_num_pages,
                                        std::size_t num_pages, std::size_t align,
                                        std::size_t offset, std::size_t guard_pages) {
    if (num_pages == 0) {
        return {};
    }

    const VAddr region_end{region_start + region_num_pages * PageSize};
    const VAddr region_last{region_end - 1};
    for (auto it{FindIterator(region_start)}; it != memory_block_tree.cend(); it++) {
        const auto info{it->GetMemoryInfo()};
        if (region_last < info.GetAddress()) {
            break;
        }

        if (info.state != KMemoryState::Free) {
            continue;
        }

        VAddr area{(info.GetAddress() <= region_start) ? region_start : info.GetAddress()};
        area += guard_pages * PageSize;

        const VAddr offset_area{Common::AlignDown(area, align) + offset};
        area = (area <= offset_area) ? offset_area : offset_area + align;

        const VAddr area_end{area + num_pages * PageSize + guard_pages * PageSize};
        const VAddr area_last{area_end - 1};

        if (info.GetAddress() <= area && area < area_last && area_last <= region_last &&
            area_last <= info.GetLastAddress()) {
            return area;
        }
    }

    return {};
}

void KMemoryBlockManager::Update(VAddr addr, std::size_t num_pages, KMemoryState prev_state,
                                 KMemoryPermission prev_perm, KMemoryAttribute prev_attribute,
                                 KMemoryState state, KMemoryPermission perm,
                                 KMemoryAttribute attribute) {
    const VAddr update_end_addr{addr + num_pages * PageSize};
    iterator node{memory_block_tree.begin()};

    prev_attribute |= KMemoryAttribute::IpcAndDeviceMapped;

    while (node != memory_block_tree.end()) {
        KMemoryBlock* block{&(*node)};
        iterator next_node{std::next(node)};
        const VAddr cur_addr{block->GetAddress()};
        const VAddr cur_end_addr{block->GetNumPages() * PageSize + cur_addr};

        if (addr < cur_end_addr && cur_addr < update_end_addr) {
            if (!block->HasProperties(prev_state, prev_perm, prev_attribute)) {
                node = next_node;
                continue;
            }

            iterator new_node{node};
            if (addr > cur_addr) {
                memory_block_tree.insert(node, block->Split(addr));
            }

            if (update_end_addr < cur_end_addr) {
                new_node = memory_block_tree.insert(node, block->Split(update_end_addr));
            }

            new_node->Update(state, perm, attribute);

            MergeAdjacent(new_node, next_node);
        }

        if (cur_end_addr - 1 >= update_end_addr - 1) {
            break;
        }

        node = next_node;
    }
}

void KMemoryBlockManager::Update(VAddr addr, std::size_t num_pages, KMemoryState state,
                                 KMemoryPermission perm, KMemoryAttribute attribute) {
    const VAddr update_end_addr{addr + num_pages * PageSize};
    iterator node{memory_block_tree.begin()};

    while (node != memory_block_tree.end()) {
        KMemoryBlock* block{&(*node)};
        iterator next_node{std::next(node)};
        const VAddr cur_addr{block->GetAddress()};
        const VAddr cur_end_addr{block->GetNumPages() * PageSize + cur_addr};

        if (addr < cur_end_addr && cur_addr < update_end_addr) {
            iterator new_node{node};

            if (addr > cur_addr) {
                memory_block_tree.insert(node, block->Split(addr));
            }

            if (update_end_addr < cur_end_addr) {
                new_node = memory_block_tree.insert(node, block->Split(update_end_addr));
            }

            new_node->Update(state, perm, attribute);

            MergeAdjacent(new_node, next_node);
        }

        if (cur_end_addr - 1 >= update_end_addr - 1) {
            break;
        }

        node = next_node;
    }
}

void KMemoryBlockManager::UpdateLock(VAddr addr, std::size_t num_pages, LockFunc&& lock_func,
                                     KMemoryPermission perm) {
    const VAddr update_end_addr{addr + num_pages * PageSize};
    iterator node{memory_block_tree.begin()};

    while (node != memory_block_tree.end()) {
        KMemoryBlock* block{&(*node)};
        iterator next_node{std::next(node)};
        const VAddr cur_addr{block->GetAddress()};
        const VAddr cur_end_addr{block->GetNumPages() * PageSize + cur_addr};

        if (addr < cur_end_addr && cur_addr < update_end_addr) {
            iterator new_node{node};

            if (addr > cur_addr) {
                memory_block_tree.insert(node, block->Split(addr));
            }

            if (update_end_addr < cur_end_addr) {
                new_node = memory_block_tree.insert(node, block->Split(update_end_addr));
            }

            lock_func(new_node, perm);

            MergeAdjacent(new_node, next_node);
        }

        if (cur_end_addr - 1 >= update_end_addr - 1) {
            break;
        }

        node = next_node;
    }
}

void KMemoryBlockManager::IterateForRange(VAddr start, VAddr end, IterateFunc&& func) {
    const_iterator it{FindIterator(start)};
    KMemoryInfo info{};
    do {
        info = it->GetMemoryInfo();
        func(info);
        it = std::next(it);
    } while (info.addr + info.size - 1 < end - 1 && it != cend());
}

void KMemoryBlockManager::MergeAdjacent(iterator it, iterator& next_it) {
    KMemoryBlock* block{&(*it)};

    auto EraseIt = [&](const iterator it_to_erase) {
        if (next_it == it_to_erase) {
            next_it = std::next(next_it);
        }
        memory_block_tree.erase(it_to_erase);
    };

    if (it != memory_block_tree.begin()) {
        KMemoryBlock* prev{&(*std::prev(it))};

        if (block->HasSameProperties(*prev)) {
            const iterator prev_it{std::prev(it)};

            prev->Add(block->GetNumPages());
            EraseIt(it);

            it = prev_it;
            block = prev;
        }
    }

    if (it != cend()) {
        const KMemoryBlock* const next{&(*std::next(it))};

        if (block->HasSameProperties(*next)) {
            block->Add(next->GetNumPages());
            EraseIt(std::next(it));
        }
    }
}

} // namespace Kernel
