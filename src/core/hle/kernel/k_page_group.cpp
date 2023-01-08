// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_dynamic_resource_manager.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_group.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

void KPageGroup::Finalize() {
    KBlockInfo* cur = m_first_block;
    while (cur != nullptr) {
        KBlockInfo* next = cur->GetNext();
        m_manager->Free(cur);
        cur = next;
    }

    m_first_block = nullptr;
    m_last_block = nullptr;
}

void KPageGroup::CloseAndReset() {
    auto& mm = m_kernel.MemoryManager();

    KBlockInfo* cur = m_first_block;
    while (cur != nullptr) {
        KBlockInfo* next = cur->GetNext();
        mm.Close(cur->GetAddress(), cur->GetNumPages());
        m_manager->Free(cur);
        cur = next;
    }

    m_first_block = nullptr;
    m_last_block = nullptr;
}

size_t KPageGroup::GetNumPages() const {
    size_t num_pages = 0;

    for (const auto& it : *this) {
        num_pages += it.GetNumPages();
    }

    return num_pages;
}

Result KPageGroup::AddBlock(KPhysicalAddress addr, size_t num_pages) {
    // Succeed immediately if we're adding no pages.
    R_SUCCEED_IF(num_pages == 0);

    // Check for overflow.
    ASSERT(addr < addr + num_pages * PageSize);

    // Try to just append to the last block.
    if (m_last_block != nullptr) {
        R_SUCCEED_IF(m_last_block->TryConcatenate(addr, num_pages));
    }

    // Allocate a new block.
    KBlockInfo* new_block = m_manager->Allocate();
    R_UNLESS(new_block != nullptr, ResultOutOfResource);

    // Initialize the block.
    new_block->Initialize(addr, num_pages);

    // Add the block to our list.
    if (m_last_block != nullptr) {
        m_last_block->SetNext(new_block);
    } else {
        m_first_block = new_block;
    }
    m_last_block = new_block;

    R_SUCCEED();
}

void KPageGroup::Open() const {
    auto& mm = m_kernel.MemoryManager();

    for (const auto& it : *this) {
        mm.Open(it.GetAddress(), it.GetNumPages());
    }
}

void KPageGroup::OpenFirst() const {
    auto& mm = m_kernel.MemoryManager();

    for (const auto& it : *this) {
        mm.OpenFirst(it.GetAddress(), it.GetNumPages());
    }
}

void KPageGroup::Close() const {
    auto& mm = m_kernel.MemoryManager();

    for (const auto& it : *this) {
        mm.Close(it.GetAddress(), it.GetNumPages());
    }
}

bool KPageGroup::IsEquivalentTo(const KPageGroup& rhs) const {
    auto lit = this->begin();
    auto rit = rhs.begin();
    auto lend = this->end();
    auto rend = rhs.end();

    while (lit != lend && rit != rend) {
        if (*lit != *rit) {
            return false;
        }

        ++lit;
        ++rit;
    }

    return lit == lend && rit == rend;
}

} // namespace Kernel
