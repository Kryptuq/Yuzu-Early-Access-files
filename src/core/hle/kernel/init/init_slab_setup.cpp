// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/hardware_properties.h"
#include "core/hle/kernel/init/init_slab_setup.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_system_control.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/kernel/memory_types.h"
#include "core/memory.h"

namespace Kernel::Init {

#define SLAB_COUNT(CLASS) kernel.SlabResourceCounts().num_##CLASS

#define FOREACH_SLAB_TYPE(HANDLER, ...)                                                            \
    HANDLER(KProcess, (SLAB_COUNT(KProcess)), ##__VA_ARGS__)                                       \
    HANDLER(KThread, (SLAB_COUNT(KThread)), ##__VA_ARGS__)                                         \
    HANDLER(KEvent, (SLAB_COUNT(KEvent)), ##__VA_ARGS__)                                           \
    HANDLER(KPort, (SLAB_COUNT(KPort)), ##__VA_ARGS__)                                             \
    HANDLER(KSharedMemory, (SLAB_COUNT(KSharedMemory)), ##__VA_ARGS__)                             \
    HANDLER(KTransferMemory, (SLAB_COUNT(KTransferMemory)), ##__VA_ARGS__)                         \
    HANDLER(KSession, (SLAB_COUNT(KSession)), ##__VA_ARGS__)                                       \
    HANDLER(KResourceLimit, (SLAB_COUNT(KResourceLimit)), ##__VA_ARGS__)

namespace {

#define DEFINE_SLAB_TYPE_ENUM_MEMBER(NAME, COUNT, ...) KSlabType_##NAME,

enum KSlabType : u32 {
    FOREACH_SLAB_TYPE(DEFINE_SLAB_TYPE_ENUM_MEMBER) KSlabType_Count,
};

#undef DEFINE_SLAB_TYPE_ENUM_MEMBER

// Constexpr counts.
constexpr size_t SlabCountKProcess = 80;
constexpr size_t SlabCountKThread = 800;
constexpr size_t SlabCountKEvent = 700;
constexpr size_t SlabCountKInterruptEvent = 100;
constexpr size_t SlabCountKPort = 256 + 0x20; // Extra 0x20 ports over Nintendo for homebrew.
constexpr size_t SlabCountKSharedMemory = 80;
constexpr size_t SlabCountKTransferMemory = 200;
constexpr size_t SlabCountKCodeMemory = 10;
constexpr size_t SlabCountKDeviceAddressSpace = 300;
constexpr size_t SlabCountKSession = 933;
constexpr size_t SlabCountKLightSession = 100;
constexpr size_t SlabCountKObjectName = 7;
constexpr size_t SlabCountKResourceLimit = 5;
constexpr size_t SlabCountKDebug = Core::Hardware::NUM_CPU_CORES;
constexpr size_t SlabCountKAlpha = 1;
constexpr size_t SlabCountKBeta = 6;

constexpr size_t SlabCountExtraKThread = 160;

template <typename T>
VAddr InitializeSlabHeap(Core::System& system, KMemoryLayout& memory_layout, VAddr address,
                         size_t num_objects) {
    // TODO(bunnei): This is just a place holder. We should initialize the appropriate KSlabHeap for
    // kernel object type T with the backing kernel memory pointer once we emulate kernel memory.

    const size_t size = Common::AlignUp(sizeof(T) * num_objects, alignof(void*));
    VAddr start = Common::AlignUp(address, alignof(T));

    // This is intentionally empty. Once KSlabHeap is fully implemented, we can replace this with
    // the pointer to emulated memory to pass along. Until then, KSlabHeap will just allocate/free
    // host memory.
    void* backing_kernel_memory{};

    if (size > 0) {
        const KMemoryRegion* region = memory_layout.FindVirtual(start + size - 1);
        ASSERT(region != nullptr);
        ASSERT(region->IsDerivedFrom(KMemoryRegionType_KernelSlab));
        T::InitializeSlabHeap(system.Kernel(), backing_kernel_memory, size);
    }

    return start + size;
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
        .num_KAlpha = SlabCountKAlpha,
        .num_KBeta = SlabCountKBeta,
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
    size += KernelSlabHeapGapsSize;

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
    const size_t total_gap_size = KernelSlabHeapGapsSize;
    std::array<size_t, slab_types.size()> slab_gaps;
    for (size_t i = 0; i < slab_gaps.size(); i++) {
        // Note: This is an off-by-one error from Nintendo's intention, because GenerateRandomRange
        // is inclusive. However, Nintendo also has the off-by-one error, and it's "harmless", so we
        // will include it ourselves.
        slab_gaps[i] = KSystemControl::GenerateRandomRange(0, total_gap_size);
    }

    // Sort the array, so that we can treat differences between values as offsets to the starts of
    // slabs.
    for (size_t i = 1; i < slab_gaps.size(); i++) {
        for (size_t j = i; j > 0 && slab_gaps[j - 1] > slab_gaps[j]; j--) {
            std::swap(slab_gaps[j], slab_gaps[j - 1]);
        }
    }

    for (size_t i = 0; i < slab_types.size(); i++) {
        // Add the random gap to the address.
        address += (i == 0) ? slab_gaps[0] : slab_gaps[i] - slab_gaps[i - 1];

#define INITIALIZE_SLAB_HEAP(NAME, COUNT, ...)                                                     \
    case KSlabType_##NAME:                                                                         \
        address = InitializeSlabHeap<NAME>(system, memory_layout, address, COUNT);                 \
        break;

        // Initialize the slabheap.
        switch (slab_types[i]) {
            // For each of the slab types, we want to initialize that heap.
            FOREACH_SLAB_TYPE(INITIALIZE_SLAB_HEAP)
            // If we somehow get an invalid type, abort.
        default:
            UNREACHABLE();
        }
    }
}

} // namespace Kernel::Init
