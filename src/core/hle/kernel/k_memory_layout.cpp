// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "common/alignment.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_system_control.h"

namespace Kernel {

namespace {

template <typename... Args>
KMemoryRegion* AllocateRegion(KMemoryRegionAllocator& memory_region_allocator, Args&&... args) {
    return memory_region_allocator.Allocate(std::forward<Args>(args)...);
}

} // namespace

KMemoryRegionTree::KMemoryRegionTree(KMemoryRegionAllocator& memory_region_allocator_)
    : memory_region_allocator{memory_region_allocator_} {}

void KMemoryRegionTree::InsertDirectly(u64 address, u64 last_address, u32 attr, u32 type_id) {
    this->insert(*AllocateRegion(memory_region_allocator, address, last_address, attr, type_id));
}

bool KMemoryRegionTree::Insert(u64 address, size_t size, u32 type_id, u32 new_attr, u32 old_attr) {
    // Locate the memory region that contains the address.
    KMemoryRegion* found = this->FindModifiable(address);

    // We require that the old attr is correct.
    if (found->GetAttributes() != old_attr) {
        return false;
    }

    // We further require that the region can be split from the old region.
    const u64 inserted_region_end = address + size;
    const u64 inserted_region_last = inserted_region_end - 1;
    if (found->GetLastAddress() < inserted_region_last) {
        return false;
    }

    // Further, we require that the type id is a valid transformation.
    if (!found->CanDerive(type_id)) {
        return false;
    }

    // Cache information from the region before we remove it.
    const u64 old_address = found->GetAddress();
    const u64 old_last = found->GetLastAddress();
    const u64 old_pair = found->GetPairAddress();
    const u32 old_type = found->GetType();

    // Erase the existing region from the tree.
    this->erase(this->iterator_to(*found));

    // Insert the new region into the tree.
    if (old_address == address) {
        // Reuse the old object for the new region, if we can.
        found->Reset(address, inserted_region_last, old_pair, new_attr, type_id);
        this->insert(*found);
    } else {
        // If we can't re-use, adjust the old region.
        found->Reset(old_address, address - 1, old_pair, old_attr, old_type);
        this->insert(*found);

        // Insert a new region for the split.
        const u64 new_pair = (old_pair != std::numeric_limits<u64>::max())
                                 ? old_pair + (address - old_address)
                                 : old_pair;
        this->insert(*AllocateRegion(memory_region_allocator, address, inserted_region_last,
                                     new_pair, new_attr, type_id));
    }

    // If we need to insert a region after the region, do so.
    if (old_last != inserted_region_last) {
        const u64 after_pair = (old_pair != std::numeric_limits<u64>::max())
                                   ? old_pair + (inserted_region_end - old_address)
                                   : old_pair;
        this->insert(*AllocateRegion(memory_region_allocator, inserted_region_end, old_last,
                                     after_pair, old_attr, old_type));
    }

    return true;
}

VAddr KMemoryRegionTree::GetRandomAlignedRegion(size_t size, size_t alignment, u32 type_id) {
    // We want to find the total extents of the type id.
    const auto extents = this->GetDerivedRegionExtents(static_cast<KMemoryRegionType>(type_id));

    // Ensure that our alignment is correct.
    ASSERT(Common::IsAligned(extents.GetAddress(), alignment));

    const u64 first_address = extents.GetAddress();
    const u64 last_address = extents.GetLastAddress();

    const u64 first_index = first_address / alignment;
    const u64 last_index = last_address / alignment;

    while (true) {
        const u64 candidate =
            KSystemControl::GenerateRandomRange(first_index, last_index) * alignment;

        // Ensure that the candidate doesn't overflow with the size.
        if (!(candidate < candidate + size)) {
            continue;
        }

        const u64 candidate_last = candidate + size - 1;

        // Ensure that the candidate fits within the region.
        if (candidate_last > last_address) {
            continue;
        }

        // Locate the candidate region, and ensure it fits and has the correct type id.
        if (const auto& candidate_region = *this->Find(candidate);
            !(candidate_last <= candidate_region.GetLastAddress() &&
              candidate_region.GetType() == type_id)) {
            continue;
        }

        return candidate;
    }
}

KMemoryLayout::KMemoryLayout()
    : virtual_tree{memory_region_allocator}, physical_tree{memory_region_allocator},
      virtual_linear_tree{memory_region_allocator}, physical_linear_tree{memory_region_allocator} {}

void KMemoryLayout::InitializeLinearMemoryRegionTrees(PAddr aligned_linear_phys_start,
                                                      VAddr linear_virtual_start) {
    // Set static differences.
    linear_phys_to_virt_diff = linear_virtual_start - aligned_linear_phys_start;
    linear_virt_to_phys_diff = aligned_linear_phys_start - linear_virtual_start;

    // Initialize linear trees.
    for (auto& region : GetPhysicalMemoryRegionTree()) {
        if (region.HasTypeAttribute(KMemoryRegionAttr_LinearMapped)) {
            GetPhysicalLinearMemoryRegionTree().InsertDirectly(
                region.GetAddress(), region.GetLastAddress(), region.GetAttributes(),
                region.GetType());
        }
    }

    for (auto& region : GetVirtualMemoryRegionTree()) {
        if (region.IsDerivedFrom(KMemoryRegionType_Dram)) {
            GetVirtualLinearMemoryRegionTree().InsertDirectly(
                region.GetAddress(), region.GetLastAddress(), region.GetAttributes(),
                region.GetType());
        }
    }
}

size_t KMemoryLayout::GetResourceRegionSizeForInit() {
    // Calculate resource region size based on whether we allow extra threads.
    const bool use_extra_resources = KSystemControl::Init::ShouldIncreaseThreadResourceLimit();
    size_t resource_region_size =
        KernelResourceSize + (use_extra_resources ? KernelSlabHeapAdditionalSize : 0);

    return resource_region_size;
}

} // namespace Kernel
