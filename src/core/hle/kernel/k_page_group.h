// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/result.h"

namespace Kernel {

class KPageGroup;

class KBlockInfo {
private:
    friend class KPageGroup;

public:
    constexpr KBlockInfo() = default;

    constexpr void Initialize(PAddr addr, size_t np) {
        ASSERT(Common::IsAligned(addr, PageSize));
        ASSERT(static_cast<u32>(np) == np);

        m_page_index = static_cast<u32>(addr) / PageSize;
        m_num_pages = static_cast<u32>(np);
    }

    constexpr PAddr GetAddress() const {
        return m_page_index * PageSize;
    }
    constexpr size_t GetNumPages() const {
        return m_num_pages;
    }
    constexpr size_t GetSize() const {
        return this->GetNumPages() * PageSize;
    }
    constexpr PAddr GetEndAddress() const {
        return (m_page_index + m_num_pages) * PageSize;
    }
    constexpr PAddr GetLastAddress() const {
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

    constexpr bool IsStrictlyBefore(PAddr addr) const {
        const PAddr end = this->GetEndAddress();

        if (m_page_index != 0 && end == 0) {
            return false;
        }

        return end < addr;
    }

    constexpr bool operator<(PAddr addr) const {
        return this->IsStrictlyBefore(addr);
    }

    constexpr bool TryConcatenate(PAddr addr, size_t np) {
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
    KBlockInfo* m_next{};
    u32 m_page_index{};
    u32 m_num_pages{};
};
static_assert(sizeof(KBlockInfo) <= 0x10);

class KPageGroup final {
public:
    class Node final {
    public:
        constexpr Node(u64 addr_, std::size_t num_pages_) : addr{addr_}, num_pages{num_pages_} {}

        constexpr u64 GetAddress() const {
            return addr;
        }

        constexpr std::size_t GetNumPages() const {
            return num_pages;
        }

        constexpr std::size_t GetSize() const {
            return GetNumPages() * PageSize;
        }

    private:
        u64 addr{};
        std::size_t num_pages{};
    };

public:
    KPageGroup() = default;
    KPageGroup(u64 address, u64 num_pages) {
        ASSERT(AddBlock(address, num_pages).IsSuccess());
    }

    constexpr std::list<Node>& Nodes() {
        return nodes;
    }

    constexpr const std::list<Node>& Nodes() const {
        return nodes;
    }

    std::size_t GetNumPages() const {
        std::size_t num_pages = 0;
        for (const Node& node : nodes) {
            num_pages += node.GetNumPages();
        }
        return num_pages;
    }

    bool IsEqual(KPageGroup& other) const {
        auto this_node = nodes.begin();
        auto other_node = other.nodes.begin();
        while (this_node != nodes.end() && other_node != other.nodes.end()) {
            if (this_node->GetAddress() != other_node->GetAddress() ||
                this_node->GetNumPages() != other_node->GetNumPages()) {
                return false;
            }
            this_node = std::next(this_node);
            other_node = std::next(other_node);
        }

        return this_node == nodes.end() && other_node == other.nodes.end();
    }

    Result AddBlock(u64 address, u64 num_pages) {
        if (!num_pages) {
            return ResultSuccess;
        }
        if (!nodes.empty()) {
            const auto node = nodes.back();
            if (node.GetAddress() + node.GetNumPages() * PageSize == address) {
                address = node.GetAddress();
                num_pages += node.GetNumPages();
                nodes.pop_back();
            }
        }
        nodes.push_back({address, num_pages});
        return ResultSuccess;
    }

    bool Empty() const {
        return nodes.empty();
    }

    void Finalize() {}

private:
    std::list<Node> nodes;
};

} // namespace Kernel
