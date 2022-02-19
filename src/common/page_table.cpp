// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/page_table.h"

namespace Common {

PageTable::PageTable() = default;

PageTable::~PageTable() noexcept = default;

bool PageTable::BeginTraversal(TraversalEntry* out_entry, TraversalContext* out_context,
                               u64 address) const {
    // Setup invalid defaults.
    out_entry->phys_addr = 0;
    out_entry->block_size = page_size;
    out_context->next_page = 0;

    // Validate that we can read the actual entry.
    const auto page = address / page_size;
    if (page >= backing_addr.size()) {
        return false;
    }

    // Validate that the entry is mapped.
    const auto phys_addr = backing_addr[page];
    if (phys_addr == 0) {
        return false;
    }

    // Populate the results.
    out_entry->phys_addr = phys_addr + address;
    out_context->next_page = page + 1;
    out_context->next_offset = address + page_size;

    return true;
}

bool PageTable::ContinueTraversal(TraversalEntry* out_entry, TraversalContext* context) const {
    // Setup invalid defaults.
    out_entry->phys_addr = 0;
    out_entry->block_size = page_size;

    // Validate that we can read the actual entry.
    const auto page = context->next_page;
    if (page >= backing_addr.size()) {
        return false;
    }

    // Validate that the entry is mapped.
    const auto phys_addr = backing_addr[page];
    if (phys_addr == 0) {
        return false;
    }

    // Populate the results.
    out_entry->phys_addr = phys_addr + context->next_offset;
    context->next_page = page + 1;
    context->next_offset += page_size;

    return true;
}

void PageTable::Resize(std::size_t address_space_width_in_bits, std::size_t page_size_in_bits) {
    const std::size_t num_page_table_entries{1ULL
                                             << (address_space_width_in_bits - page_size_in_bits)};
    pointers.resize(num_page_table_entries);
    backing_addr.resize(num_page_table_entries);
    current_address_space_width_in_bits = address_space_width_in_bits;
    page_size = 1ULL << page_size_in_bits;
}

} // namespace Common
