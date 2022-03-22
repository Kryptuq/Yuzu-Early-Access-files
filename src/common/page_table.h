// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>

#include "common/common_types.h"
#include "common/virtual_buffer.h"

namespace Common {

enum class PageType : u8 {
    /// Page is unmapped and should cause an access error.
    Unmapped,
    /// Page is mapped to regular memory. This is the only type you can get pointers to.
    Memory,
    /// Page is mapped to regular memory, but also needs to check for rasterizer cache flushing and
    /// invalidation
    RasterizerCachedMemory,
};

/**
 * A (reasonably) fast way of allowing switchable and remappable process address spaces. It loosely
 * mimics the way a real CPU page table works.
 */
struct PageTable {
    struct TraversalEntry {
        u64 phys_addr{};
        std::size_t block_size{};
    };

    struct TraversalContext {
        u64 next_page{};
        u64 next_offset{};
    };

    /// Number of bits reserved for attribute tagging.
    /// This can be at most the guaranteed alignment of the pointers in the page table.
    static constexpr int ATTRIBUTE_BITS = 2;

    /**
     * Pair of host pointer and page type attribute.
     * This uses the lower bits of a given pointer to store the attribute tag.
     * Writing and reading the pointer attribute pair is guaranteed to be atomic for the same method
     * call. In other words, they are guaranteed to be synchronized at all times.
     */
    class PageInfo {
    public:
        /// Returns the page pointer
        [[nodiscard]] u8* Pointer() const noexcept {
            return ExtractPointer(raw.load(std::memory_order_relaxed));
        }

        /// Returns the page type attribute
        [[nodiscard]] PageType Type() const noexcept {
            return ExtractType(raw.load(std::memory_order_relaxed));
        }

        /// Returns the page pointer and attribute pair, extracted from the same atomic read
        [[nodiscard]] std::pair<u8*, PageType> PointerType() const noexcept {
            const uintptr_t non_atomic_raw = raw.load(std::memory_order_relaxed);
            return {ExtractPointer(non_atomic_raw), ExtractType(non_atomic_raw)};
        }

        /// Returns the raw representation of the page information.
        /// Use ExtractPointer and ExtractType to unpack the value.
        [[nodiscard]] uintptr_t Raw() const noexcept {
            return raw.load(std::memory_order_relaxed);
        }

        /// Write a page pointer and type pair atomically
        void Store(u8* pointer, PageType type) noexcept {
            raw.store(reinterpret_cast<uintptr_t>(pointer) | static_cast<uintptr_t>(type));
        }

        /// Unpack a pointer from a page info raw representation
        [[nodiscard]] static u8* ExtractPointer(uintptr_t raw) noexcept {
            return reinterpret_cast<u8*>(raw & (~uintptr_t{0} << ATTRIBUTE_BITS));
        }

        /// Unpack a page type from a page info raw representation
        [[nodiscard]] static PageType ExtractType(uintptr_t raw) noexcept {
            return static_cast<PageType>(raw & ((uintptr_t{1} << ATTRIBUTE_BITS) - 1));
        }

    private:
        std::atomic<uintptr_t> raw;
    };

    PageTable();
    ~PageTable() noexcept;

    PageTable(const PageTable&) = delete;
    PageTable& operator=(const PageTable&) = delete;

    PageTable(PageTable&&) noexcept = default;
    PageTable& operator=(PageTable&&) noexcept = default;

    bool BeginTraversal(TraversalEntry& out_entry, TraversalContext& out_context,
                        u64 address) const;
    bool ContinueTraversal(TraversalEntry& out_entry, TraversalContext& context) const;

    /**
     * Resizes the page table to be able to accommodate enough pages within
     * a given address space.
     *
     * @param address_space_width_in_bits The address size width in bits.
     * @param page_size_in_bits           The page size in bits.
     */
    void Resize(std::size_t address_space_width_in_bits, std::size_t page_size_in_bits);

    std::size_t GetAddressSpaceBits() const {
        return current_address_space_width_in_bits;
    }

    /**
     * Vector of memory pointers backing each page. An entry can only be non-null if the
     * corresponding attribute element is of type `Memory`.
     */
    VirtualBuffer<PageInfo> pointers;

    VirtualBuffer<u64> backing_addr;

    std::size_t current_address_space_width_in_bits{};

    u8* fastmem_arena{};

    std::size_t page_size{};
};

} // namespace Common
