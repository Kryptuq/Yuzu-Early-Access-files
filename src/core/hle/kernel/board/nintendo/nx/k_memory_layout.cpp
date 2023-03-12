// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/literals.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_system_control.h"
#include "core/hle/kernel/k_trace.h"

namespace Kernel {

namespace {

using namespace Common::Literals;

constexpr size_t CarveoutAlignment = 0x20000;
constexpr size_t CarveoutSizeMax = (512_MiB) - CarveoutAlignment;

bool SetupPowerManagementControllerMemoryRegion(KMemoryLayout& memory_layout) {
    // Above firmware 2.0.0, the PMC is not mappable.
    return memory_layout.GetPhysicalMemoryRegionTree().Insert(
               0x7000E000, 0x400, KMemoryRegionType_None | KMemoryRegionAttr_NoUserMap) &&
           memory_layout.GetPhysicalMemoryRegionTree().Insert(
               0x7000E400, 0xC00,
               KMemoryRegionType_PowerManagementController | KMemoryRegionAttr_NoUserMap);
}

void InsertPoolPartitionRegionIntoBothTrees(KMemoryLayout& memory_layout, size_t start, size_t size,
                                            KMemoryRegionType phys_type,
                                            KMemoryRegionType virt_type, u32& cur_attr) {
    const u32 attr = cur_attr++;
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(start, size,
                                                              static_cast<u32>(phys_type), attr));
    const KMemoryRegion* phys = memory_layout.GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(
        static_cast<u32>(phys_type), attr);
    ASSERT(phys != nullptr);
    ASSERT(phys->GetEndAddress() != 0);
    ASSERT(memory_layout.GetVirtualMemoryRegionTree().Insert(phys->GetPairAddress(), size,
                                                             static_cast<u32>(virt_type), attr));
}

} // namespace

namespace Init {

void SetupDevicePhysicalMemoryRegions(KMemoryLayout& memory_layout) {
    ASSERT(SetupPowerManagementControllerMemoryRegion(memory_layout));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x70019000, 0x1000, KMemoryRegionType_MemoryController | KMemoryRegionAttr_NoUserMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x7001C000, 0x1000, KMemoryRegionType_MemoryController0 | KMemoryRegionAttr_NoUserMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x7001D000, 0x1000, KMemoryRegionType_MemoryController1 | KMemoryRegionAttr_NoUserMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x50040000, 0x1000, KMemoryRegionType_None | KMemoryRegionAttr_NoUserMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x50041000, 0x1000,
        KMemoryRegionType_InterruptDistributor | KMemoryRegionAttr_ShouldKernelMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x50042000, 0x1000,
        KMemoryRegionType_InterruptCpuInterface | KMemoryRegionAttr_ShouldKernelMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x50043000, 0x1D000, KMemoryRegionType_None | KMemoryRegionAttr_NoUserMap));

    // Map IRAM unconditionally, to support debug-logging-to-iram build config.
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x40000000, 0x40000, KMemoryRegionType_LegacyLpsIram | KMemoryRegionAttr_ShouldKernelMap));

    // Above firmware 2.0.0, prevent mapping the bpmp exception vectors or the ipatch region.
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x6000F000, 0x1000, KMemoryRegionType_None | KMemoryRegionAttr_NoUserMap));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        0x6001DC00, 0x400, KMemoryRegionType_None | KMemoryRegionAttr_NoUserMap));
}

void SetupDramPhysicalMemoryRegions(KMemoryLayout& memory_layout) {
    const size_t intended_memory_size = KSystemControl::Init::GetIntendedMemorySize();
    const PAddr physical_memory_base_address =
        KSystemControl::Init::GetKernelPhysicalBaseAddress(DramPhysicalAddress);

    // Insert blocks into the tree.
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        physical_memory_base_address, intended_memory_size, KMemoryRegionType_Dram));
    ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
        physical_memory_base_address, ReservedEarlyDramSize, KMemoryRegionType_DramReservedEarly));

    // Insert the KTrace block at the end of Dram, if KTrace is enabled.
    static_assert(!IsKTraceEnabled || KTraceBufferSize > 0);
    if constexpr (IsKTraceEnabled) {
        const PAddr ktrace_buffer_phys_addr =
            physical_memory_base_address + intended_memory_size - KTraceBufferSize;
        ASSERT(memory_layout.GetPhysicalMemoryRegionTree().Insert(
            ktrace_buffer_phys_addr, KTraceBufferSize, KMemoryRegionType_KernelTraceBuffer));
    }
}

void SetupPoolPartitionMemoryRegions(KMemoryLayout& memory_layout) {
    // Start by identifying the extents of the DRAM memory region.
    const auto dram_extents = memory_layout.GetMainMemoryPhysicalExtents();
    ASSERT(dram_extents.GetEndAddress() != 0);

    // Determine the end of the pool region.
    const u64 pool_end = dram_extents.GetEndAddress() - KTraceBufferSize;

    // Find the start of the kernel DRAM region.
    const KMemoryRegion* kernel_dram_region =
        memory_layout.GetPhysicalMemoryRegionTree().FindFirstDerived(
            KMemoryRegionType_DramKernelBase);
    ASSERT(kernel_dram_region != nullptr);

    const u64 kernel_dram_start = kernel_dram_region->GetAddress();
    ASSERT(Common::IsAligned(kernel_dram_start, CarveoutAlignment));

    // Find the start of the pool partitions region.
    const KMemoryRegion* pool_partitions_region =
        memory_layout.GetPhysicalMemoryRegionTree().FindByTypeAndAttribute(
            KMemoryRegionType_DramPoolPartition, 0);
    ASSERT(pool_partitions_region != nullptr);
    const u64 pool_partitions_start = pool_partitions_region->GetAddress();

    // Setup the pool partition layouts.
    // On 5.0.0+, setup modern 4-pool-partition layout.

    // Get Application and Applet pool sizes.
    const size_t application_pool_size = KSystemControl::Init::GetApplicationPoolSize();
    const size_t applet_pool_size = KSystemControl::Init::GetAppletPoolSize();
    const size_t unsafe_system_pool_min_size =
        KSystemControl::Init::GetMinimumNonSecureSystemPoolSize();

    // Decide on starting addresses for our pools.
    const u64 application_pool_start = pool_end - application_pool_size;
    const u64 applet_pool_start = application_pool_start - applet_pool_size;
    const u64 unsafe_system_pool_start = std::min(
        kernel_dram_start + CarveoutSizeMax,
        Common::AlignDown(applet_pool_start - unsafe_system_pool_min_size, CarveoutAlignment));
    const size_t unsafe_system_pool_size = applet_pool_start - unsafe_system_pool_start;

    // We want to arrange application pool depending on where the middle of dram is.
    const u64 dram_midpoint = (dram_extents.GetAddress() + dram_extents.GetEndAddress()) / 2;
    u32 cur_pool_attr = 0;
    size_t total_overhead_size = 0;
    if (dram_extents.GetEndAddress() <= dram_midpoint || dram_midpoint <= application_pool_start) {
        InsertPoolPartitionRegionIntoBothTrees(
            memory_layout, application_pool_start, application_pool_size,
            KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool,
            cur_pool_attr);
        total_overhead_size +=
            KMemoryManager::CalculateManagementOverheadSize(application_pool_size);
    } else {
        const size_t first_application_pool_size = dram_midpoint - application_pool_start;
        const size_t second_application_pool_size =
            application_pool_start + application_pool_size - dram_midpoint;
        InsertPoolPartitionRegionIntoBothTrees(
            memory_layout, application_pool_start, first_application_pool_size,
            KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool,
            cur_pool_attr);
        InsertPoolPartitionRegionIntoBothTrees(
            memory_layout, dram_midpoint, second_application_pool_size,
            KMemoryRegionType_DramApplicationPool, KMemoryRegionType_VirtualDramApplicationPool,
            cur_pool_attr);
        total_overhead_size +=
            KMemoryManager::CalculateManagementOverheadSize(first_application_pool_size);
        total_overhead_size +=
            KMemoryManager::CalculateManagementOverheadSize(second_application_pool_size);
    }

    // Insert the applet pool.
    InsertPoolPartitionRegionIntoBothTrees(memory_layout, applet_pool_start, applet_pool_size,
                                           KMemoryRegionType_DramAppletPool,
                                           KMemoryRegionType_VirtualDramAppletPool, cur_pool_attr);
    total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(applet_pool_size);

    // Insert the nonsecure system pool.
    InsertPoolPartitionRegionIntoBothTrees(
        memory_layout, unsafe_system_pool_start, unsafe_system_pool_size,
        KMemoryRegionType_DramSystemNonSecurePool, KMemoryRegionType_VirtualDramSystemNonSecurePool,
        cur_pool_attr);
    total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(unsafe_system_pool_size);

    // Insert the pool management region.
    total_overhead_size += KMemoryManager::CalculateManagementOverheadSize(
        (unsafe_system_pool_start - pool_partitions_start) - total_overhead_size);
    const u64 pool_management_start = unsafe_system_pool_start - total_overhead_size;
    const size_t pool_management_size = total_overhead_size;
    u32 pool_management_attr = 0;
    InsertPoolPartitionRegionIntoBothTrees(
        memory_layout, pool_management_start, pool_management_size,
        KMemoryRegionType_DramPoolManagement, KMemoryRegionType_VirtualDramPoolManagement,
        pool_management_attr);

    // Insert the system pool.
    const u64 system_pool_size = pool_management_start - pool_partitions_start;
    InsertPoolPartitionRegionIntoBothTrees(memory_layout, pool_partitions_start, system_pool_size,
                                           KMemoryRegionType_DramSystemPool,
                                           KMemoryRegionType_VirtualDramSystemPool, cur_pool_attr);
}

} // namespace Init

} // namespace Kernel
