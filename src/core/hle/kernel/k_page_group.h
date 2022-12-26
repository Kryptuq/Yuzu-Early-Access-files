// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/result.h"

namespace Kernel {

class KBlockInfoManager;
class KernelCore;
class KPageGroup;

class KBlockInfo {
public:
    constexpr explicit KBlockInfo() : m_next(nullptr) {}

    constexpr void Initialize(KPhysicalAddress addr, size_t np) {
        ASSERT(Common::IsAligned(addr, PageSize));
        ASSERT(static_cast<u32>(np) == np);

        m_page_index = static_cast<u32>(addr / PageSize);
        m_num_pages = static_cast<u32>(np);
    }

    constexpr KPhysicalAddress GetAddress() const {
        return m_page_index * PageSize;
    }
    constexpr size_t GetNumPages() const {
        return m_num_pages;
    }
    constexpr size_t GetSize() const {
        return this->GetNumPages() * PageSize;
    }
    constexpr KPhysicalAddress GetEndAddress() const {
        return (m_page_index + m_num_pages) * PageSize;
    }
    constexpr KPhysicalAddress GetLastAddress() const {
        return this->GetEndAddress() - 1;
    }

    constexpr KBlockInfo* GetNext() const {
        return m_next;
    }

    constexpr bool IsEquivalentTo(const KBlockInfo& rhs) const {
        return m_page_index == rhs.m_page_index && m_num_pages == rhs.m_num_pages;
    }

    constexpr bool operator==(const KBlockInfo& rhs) const {
        return this->IsEquivalentTo(rhs);
    }

    constexpr bool operator!=(const KBlockInfo& rhs) const {
        return !(*this == rhs);
    }

    constexpr bool IsStrictlyBefore(KPhysicalAddress addr) const {
        const KPhysicalAddress end = this->GetEndAddress();

        if (m_page_index != 0 && end == 0) {
            return false;
        }

        return end < addr;
    }

    constexpr bool operator<(KPhysicalAddress addr) const {
        return this->IsStrictlyBefore(addr);
    }

    constexpr bool TryConcatenate(KPhysicalAddress addr, size_t np) {
        if (addr != 0 && addr == this->GetEndAddress()) {
            m_num_pages += static_cast<u32>(np);
            return true;
        }
        return false;
    }

private:
    constexpr void SetNext(KBlockInfo* next) {
        m_next = next;
    }

private:
    friend class KPageGroup;

    KBlockInfo* m_next{};
    u32 m_page_index{};
    u32 m_num_pages{};
};
static_assert(sizeof(KBlockInfo) <= 0x10);

class KPageGroup {
public:
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const KBlockInfo;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        constexpr explicit Iterator(pointer n) : m_node(n) {}

        constexpr bool operator==(const Iterator& rhs) const {
            return m_node == rhs.m_node;
        }
        constexpr bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }

        constexpr pointer operator->() const {
            return m_node;
        }
        constexpr reference operator*() const {
            return *m_node;
        }

        constexpr Iterator& operator++() {
            m_node = m_node->GetNext();
            return *this;
        }

        constexpr Iterator operator++(int) {
            const Iterator it{*this};
            ++(*this);
            return it;
        }

    private:
        pointer m_node{};
    };

    explicit KPageGroup(KernelCore& kernel, KBlockInfoManager* m)
        : m_kernel{kernel}, m_manager{m} {}
    ~KPageGroup() {
        this->Finalize();
    }

    void CloseAndReset();
    void Finalize();

    Iterator begin() const {
        return Iterator{m_first_block};
    }
    Iterator end() const {
        return Iterator{nullptr};
    }
    bool empty() const {
        return m_first_block == nullptr;
    }

    Result AddBlock(KPhysicalAddress addr, size_t num_pages);
    void Open() const;
    void OpenFirst() const;
    void Close() const;

    size_t GetNumPages() const;

    bool IsEquivalentTo(const KPageGroup& rhs) const;

    bool operator==(const KPageGroup& rhs) const {
        return this->IsEquivalentTo(rhs);
    }

    bool operator!=(const KPageGroup& rhs) const {
        return !(*this == rhs);
    }

private:
    KernelCore& m_kernel;
    KBlockInfo* m_first_block{};
    KBlockInfo* m_last_block{};
    KBlockInfoManager* m_manager{};
};

class KScopedPageGroup {
public:
    explicit KScopedPageGroup(const KPageGroup* gp) : m_pg(gp) {
        if (m_pg) {
            m_pg->Open();
        }
    }
    explicit KScopedPageGroup(const KPageGroup& gp) : KScopedPageGroup(std::addressof(gp)) {}
    ~KScopedPageGroup() {
        if (m_pg) {
            m_pg->Close();
        }
    }

    void CancelClose() {
        m_pg = nullptr;
    }

private:
    const KPageGroup* m_pg{};
};

} // namespace Kernel
