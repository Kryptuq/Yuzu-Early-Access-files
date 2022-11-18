// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hardware_properties.h"
#include "core/hle/kernel/init/init_slab_setup.h"
#include "core/hle/kernel/k_code_memory.h"
#include "core/hle/kernel/k_debug.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_event_info.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_buffer.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/k_session_request.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_shared_memory_info.h"
#include "core/hle/kernel/k_system_control.h"
#include "core/hle/kernel/k_system_resource.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_thread_local_page.h"
#include "core/hle/kernel/k_transfer_memory.h"

namespace Kernel::Init {

#define SLAB_COUNT(CLASS) kernel.SlabResourceCounts().num_##CLASS

#define FOREACH_SLAB_TYPE(HANDLER, ...)                                                            \
    HANDLER(KProcess, (SLAB_COUNT(KProcess)), ##__VA_ARGS__)                                       \
    HANDLER(KThread, (SLAB_COUNT(KThread)), ##__VA_ARGS__)                                         \
    HANDLER(KEvent, (SLAB_COUNT(KEvent)), ##__VA_ARGS__)                                           \
    HANDLER(KPort, (SLAB_COUNT(KPort)), ##__VA_ARGS__)                                             \
    HANDLER(KSessionRequest, (SLAB_COUNT(KSession) * 2), ##__VA_ARGS__)                            \
    HANDLER(KSharedMemory, (SLAB_COUNT(KSharedMemory)), ##__VA_ARGS__)                             \
    HANDLER(KSharedMemoryInfo, (SLAB_COUNT(KSharedMemory) * 8), ##__VA_ARGS__)                     \
    HANDLER(KTransferMemory, (SLAB_COUNT(KTransferMemory)), ##__VA_ARGS__)                         \
    HANDLER(KCodeMemory, (SLAB_COUNT(KCodeMemory)), ##__VA_ARGS__)                                 \
    HANDLER(KSession, (SLAB_COUNT(KSession)), ##__VA_ARGS__)                                       \
    HANDLER(KThreadLocalPage,                                                                      \
            (SLAB_COUNT(KProcess) + (SLAB_COUNT(KProcess) + SLAB_COUNT(KThread)) / 8),             \
            ##__VA_ARGS__)                                                                         \
    HANDLER(KResourceLimit, (SLAB_COUNT(KResourceLimit)), ##__VA_ARGS__)                           \
    HANDLER(KEventInfo, (SLAB_COUNT(KThread) + SLAB_COUNT(KDebug)), ##__VA_ARGS__)                 \
    HANDLER(KDebug, (SLAB_COUNT(KDebug)), ##__VA_ARGS__)                                           \
    HANDLER(KSecureSystemResource, (SLAB_COUNT(KProcess)), ##__VA_ARGS__)

namespace {

#define DEFINE_SLAB_TYPE_ENUM_MEMBER(NAME, COUNT, ...) KSlabType_##NAME,

enum KSlabType : u32 {
    FOREACH_SLAB_TYPE(DEFINE_SLAB_TYPE_ENUM_MEMBER) KSlabType_Count,
};

#undef DEFINE_SLAB_TYPE_ENUM_MEMBER

// Constexpr counts.
constexpr size_t SlabCountKProcess = 80;
constexpr size_t SlabCountKThread = 800;
constexpr size_t SlabCountKEvent = 900;
constexpr size_t SlabCountKInterruptEvent = 100;
constexpr size_t SlabCountKPort = 384;
constexpr size_t SlabCountKSharedMemory = 80;
constexpr size_t SlabCountKTransferMemory = 200;
constexpr size_t SlabCountKCodeMemory = 10;
constexpr size_t SlabCountKDeviceAddressSpace = 300;
constexpr size_t SlabCountKSession = 1133;
constexpr size_t SlabCountKLightSession = 100;
constexpr size_t SlabCountKObjectName = 7;
constexpr size_t SlabCountKResourceLimit = 5;
constexpr size_t SlabCountKDebug = Core::Hardware::NUM_CPU_CORES;
constexpr size_t SlabCountKIoPool = 1;
constexpr size_t SlabCountKIoRegion = 6;
constexpr size_t SlabcountKSessionRequestMappings = 40;

constexpr size_t SlabCountExtraKThread = (1024 + 256 + 256) - SlabCountKThread;

namespace test {

static_assert(KernelPageBufferHeapSize ==
              2 * PageSize + (SlabCountKProcess + SlabCountKThread +
                              (SlabCountKProcess + SlabCountKThread) / 8) *
                                 PageSize);
static_assert(KernelPageBufferAdditionalSize ==
              (SlabCountExtraKThread + (SlabCountExtraKThread / 8)) * PageSize);

} // namespace test

/// Helper function to translate from the slab virtual address to the reserved location in physical
/// memory.
static PAddr TranslateSlabAddrToPhysical(KMemoryLayout& memory_layout, VAddr slab_addr) {
    slab_addr -= memory_layout.GetSlabRegionAddress();
    return slab_addr + Core::DramMemoryMap::SlabHeapBase;
}

template <typename T>
VAddr InitializeSlabHeap(Core::System& system, KMemoryLayout& memory_layout, VAddr address,
                         size_t num_objects) {

    const size_t size = Common::AlignUp(sizeof(T) * num_objects, alignof(void*));
    VAddr start = Common::AlignUp(address, alignof(T));

    // This should use the virtual memory address passed in, but currently, we do not setup the
    // kernel virtual memory layout. Instead, we simply map these at a region of physical memory
    // that we reserve for the slab heaps.
    // TODO(bunnei): Fix this once we support the kernel virtual memory layout.

    if (size > 0) {
        void* backing_kernel_memory{system.DeviceMemory().GetPointer<void>(
            TranslateSlabAddrToPhysical(memory_layout, start))};

        const KMemoryRegion* region = memory_layout.FindVirtual(start + size - 1);
        ASSERT(region != nullptr);
        ASSERT(region->IsDerivedFrom(KMemoryRegionType_KernelSlab));
        T::InitializeSlabHeap(system.Kernel(), backing_kernel_memory, size);
    }

    return start + size;
}

size_t CalculateSlabHeapGapSize() {
    constexpr size_t KernelSlabHeapGapSize = 2_MiB - 320_KiB;
    static_assert(KernelSlabHeapGapSize <= KernelSlabHeapGapsSizeMax);
    return KernelSlabHeapGapSize;
}

} // namespace

KSlabResourceCounts KSlabResourceCounts::CreateDefault() {
    return {
        .num_KProcess = SlabCountKProcess,
        .num_KThread = SlabCountKThread,
        .num_KEvent = SlabCountKEvent,
        .num_KInterruptEvent = SlabCountKInterruptEvent,
        .num_KPort = SlabCountKPort,
        .num_KSharedMemory = SlabCountKSharedMemory,
        .num_KTransferMemory = SlabCountKTransferMemory,
        .num_KCodeMemory = SlabCountKCodeMemory,
        .num_KDeviceAddressSpace = SlabCountKDeviceAddressSpace,
        .num_KSession = SlabCountKSession,
        .num_KLightSession = SlabCountKLightSession,
        .num_KObjectName = SlabCountKObjectName,
        .num_KResourceLimit = SlabCountKResourceLimit,
        .num_KDebug = SlabCountKDebug,
        .num_KIoPool = SlabCountKIoPool,
        .num_KIoRegion = SlabCountKIoRegion,
        .num_KSessionRequestMappings = SlabcountKSessionRequestMappings,
    };
}

void InitializeSlabResourceCounts(KernelCore& kernel) {
    kernel.SlabResourceCounts() = KSlabResourceCounts::CreateDefault();
    if (KSystemControl::Init::ShouldIncreaseThreadResourceLimit()) {
        kernel.SlabResourceCounts().num_KThread += SlabCountExtraKThread;
    }
}

size_t CalculateTotalSlabHeapSize(const KernelCore& kernel) {
    size_t size = 0;

#define ADD_SLAB_SIZE(NAME, COUNT, ...)                                                            \
    {                                                                                              \
        size += alignof(NAME);                                                                     \
        size += Common::AlignUp(sizeof(NAME) * (COUNT), alignof(void*));                           \
    };

    // Add the size required for each slab.
    FOREACH_SLAB_TYPE(ADD_SLAB_SIZE)

#undef ADD_SLAB_SIZE

    // Add the reserved size.
    size += CalculateSlabHeapGapSize();

    return size;
}

void InitializeSlabHeaps(Core::System& system, KMemoryLayout& memory_layout) {
    auto& kernel = system.Kernel();

    // Get the start of the slab region, since that's where we'll be working.
    VAddr address = memory_layout.GetSlabRegionAddress();

    // Initialize slab type array to be in sorted order.
    std::array<KSlabType, KSlabType_Count> slab_types;
    for (size_t i = 0; i < slab_types.size(); i++) {
        slab_types[i] = static_cast<KSlabType>(i);
    }

    // N shuffles the slab type array with the following simple algorithm.
    for (size_t i = 0; i < slab_types.size(); i++) {
        const size_t rnd = KSystemControl::GenerateRandomRange(0, slab_types.size() - 1);
        std::swap(slab_types[i], slab_types[rnd]);
    }

    // Create an array to represent the gaps between the slabs.
    const size_t total_gap_size = CalculateSlabHeapGapSize();
    std::array<size_t, slab_types.size()> slab_gaps;
    for (auto& slab_gap : slab_gaps) {
        // Note: This is an off-by-one error from Nintendo's intention, because GenerateRandomRange
        // is inclusive. However, Nintendo also has the off-by-one error, and it's "harmless", so we
        // will include it ourselves.
        slab_gap = KSystemControl::GenerateRandomRange(0, total_gap_size);
    }

    // Sort the array, so that we can treat differences between values as offsets to the starts of
    // slabs.
    for (size_t i = 1; i < slab_gaps.size(); i++) {
        for (size_t j = i; j > 0 && slab_gaps[j - 1] > slab_gaps[j]; j--) {
            std::swap(slab_gaps[j], slab_gaps[j - 1]);
        }
    }

    // Track the gaps, so that we can free them to the unused slab tree.
    VAddr gap_start = address;
    size_t gap_size = 0;

    for (size_t i = 0; i < slab_gaps.size(); i++) {
        // Add the random gap to the address.
        const auto cur_gap = (i == 0) ? slab_gaps[0] : slab_gaps[i] - slab_gaps[i - 1];
        address += cur_gap;
        gap_size += cur_gap;

#define INITIALIZE_SLAB_HEAP(NAME, COUNT, ...)                                                     \
    case KSlabType_##NAME:                                                                         \
        if (COUNT > 0) {                                                                           \
            address = InitializeSlabHeap<NAME>(system, memory_layout, address, COUNT);             \
        }                                                                                          \
        break;

        // Initialize the slabheap.
        switch (slab_types[i]) {
            // For each of the slab types, we want to initialize that heap.
            FOREACH_SLAB_TYPE(INITIALIZE_SLAB_HEAP)
            // If we somehow get an invalid type, abort.
        default:
            ASSERT_MSG(false, "Unknown slab type: {}", slab_types[i]);
            break;
        }

        // If we've hit the end of a gap, free it.
        if (gap_start + gap_size != address) {
            gap_start = address;
            gap_size = 0;
        }
    }
}

} // namespace Kernel::Init

namespace Kernel {

void KPageBufferSlabHeap::Initialize(Core::System& system) {
    auto& kernel = system.Kernel();
    const auto& counts = kernel.SlabResourceCounts();
    const size_t num_pages =
        counts.num_KProcess + counts.num_KThread + (counts.num_KProcess + counts.num_KThread) / 8;
    const size_t slab_size = num_pages * PageSize;

    // Reserve memory from the system resource limit.
    ASSERT(
        kernel.GetSystemResourceLimit()->Reserve(LimitableResource::PhysicalMemoryMax, slab_size));

    // Allocate memory for the slab.
    constexpr auto AllocateOption = KMemoryManager::EncodeOption(
        KMemoryManager::Pool::System, KMemoryManager::Direction::FromFront);
    const PAddr slab_address =
        kernel.MemoryManager().AllocateAndOpenContinuous(num_pages, 1, AllocateOption);
    ASSERT(slab_address != 0);

    // Initialize the slabheap.
    KPageBuffer::InitializeSlabHeap(kernel, system.DeviceMemory().GetPointer<void>(slab_address),
                                    slab_size);
}

} // namespace Kernel
