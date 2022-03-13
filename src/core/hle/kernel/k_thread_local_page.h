// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <array>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/intrusive_red_black_tree.h"
#include "core/hle/kernel/k_page_buffer.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Kernel {

class KernelCore;
class KProcess;

class KThreadLocalPage final : public Common::IntrusiveRedBlackTreeBaseNode<KThreadLocalPage>,
                               public KSlabAllocated<KThreadLocalPage> {
public:
    static constexpr size_t RegionsPerPage = PageSize / Svc::ThreadLocalRegionSize;
    static_assert(RegionsPerPage > 0);

public:
    explicit KThreadLocalPage(VAddr addr = {}) : m_virt_addr(addr) {
        m_is_region_free.fill(true);
    }

    constexpr VAddr GetAddress() const {
        return m_virt_addr;
    }

    ResultCode Initialize(KernelCore& kernel, KProcess* process);
    ResultCode Finalize();

    VAddr Reserve();
    void Release(VAddr addr);

    bool IsAllUsed() const {
        return std::ranges::all_of(m_is_region_free.begin(), m_is_region_free.end(),
                                   [](bool is_free) { return !is_free; });
    }

    bool IsAllFree() const {
        return std::ranges::all_of(m_is_region_free.begin(), m_is_region_free.end(),
                                   [](bool is_free) { return is_free; });
    }

    bool IsAnyUsed() const {
        return !this->IsAllFree();
    }

    bool IsAnyFree() const {
        return !this->IsAllUsed();
    }

public:
    using RedBlackKeyType = VAddr;

    static constexpr RedBlackKeyType GetRedBlackKey(const RedBlackKeyType& v) {
        return v;
    }
    static constexpr RedBlackKeyType GetRedBlackKey(const KThreadLocalPage& v) {
        return v.GetAddress();
    }

    template <typename T>
    requires(std::same_as<T, KThreadLocalPage> ||
             std::same_as<T, RedBlackKeyType>) static constexpr int Compare(const T& lhs,
                                                                            const KThreadLocalPage&
                                                                                rhs) {
        const VAddr lval = GetRedBlackKey(lhs);
        const VAddr rval = GetRedBlackKey(rhs);

        if (lval < rval) {
            return -1;
        } else if (lval == rval) {
            return 0;
        } else {
            return 1;
        }
    }

private:
    constexpr VAddr GetRegionAddress(size_t i) {
        return this->GetAddress() + i * Svc::ThreadLocalRegionSize;
    }

    constexpr bool Contains(VAddr addr) {
        return this->GetAddress() <= addr && addr < this->GetAddress() + PageSize;
    }

    constexpr size_t GetRegionIndex(VAddr addr) {
        ASSERT(Common::IsAligned(addr, Svc::ThreadLocalRegionSize));
        ASSERT(this->Contains(addr));
        return (addr - this->GetAddress()) / Svc::ThreadLocalRegionSize;
    }

private:
    VAddr m_virt_addr{};
    KProcess* m_owner{};
    KernelCore* m_kernel{};
    std::array<bool, RegionsPerPage> m_is_region_free{};
};

} // namespace Kernel
