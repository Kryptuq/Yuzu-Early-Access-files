// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/svc.h"

namespace Kernel::Svc {
namespace {

constexpr bool IsValidSetMemoryPermission(MemoryPermission perm) {
    switch (perm) {
    case MemoryPermission::None:
    case MemoryPermission::Read:
    case MemoryPermission::ReadWrite:
        return true;
    default:
        return false;
    }
}

// Checks if address + size is greater than the given address
// This can return false if the size causes an overflow of a 64-bit type
// or if the given size is zero.
constexpr bool IsValidAddressRange(u64 address, u64 size) {
    return address + size > address;
}

// Helper function that performs the common sanity checks for svcMapMemory
// and svcUnmapMemory. This is doable, as both functions perform their sanitizing
// in the same order.
Result MapUnmapMemorySanityChecks(const KPageTable& manager, u64 dst_addr, u64 src_addr, u64 size) {
    if (!Common::Is4KBAligned(dst_addr)) {
        LOG_ERROR(Kernel_SVC, "Destination address is not aligned to 4KB, 0x{:016X}", dst_addr);
        R_THROW(ResultInvalidAddress);
    }

    if (!Common::Is4KBAligned(src_addr)) {
        LOG_ERROR(Kernel_SVC, "Source address is not aligned to 4KB, 0x{:016X}", src_addr);
        R_THROW(ResultInvalidSize);
    }

    if (size == 0) {
        LOG_ERROR(Kernel_SVC, "Size is 0");
        R_THROW(ResultInvalidSize);
    }

    if (!Common::Is4KBAligned(size)) {
        LOG_ERROR(Kernel_SVC, "Size is not aligned to 4KB, 0x{:016X}", size);
        R_THROW(ResultInvalidSize);
    }

    if (!IsValidAddressRange(dst_addr, size)) {
        LOG_ERROR(Kernel_SVC,
                  "Destination is not a valid address range, addr=0x{:016X}, size=0x{:016X}",
                  dst_addr, size);
        R_THROW(ResultInvalidCurrentMemory);
    }

    if (!IsValidAddressRange(src_addr, size)) {
        LOG_ERROR(Kernel_SVC, "Source is not a valid address range, addr=0x{:016X}, size=0x{:016X}",
                  src_addr, size);
        R_THROW(ResultInvalidCurrentMemory);
    }

    if (!manager.IsInsideAddressSpace(src_addr, size)) {
        LOG_ERROR(Kernel_SVC,
                  "Source is not within the address space, addr=0x{:016X}, size=0x{:016X}",
                  src_addr, size);
        R_THROW(ResultInvalidCurrentMemory);
    }

    if (manager.IsOutsideStackRegion(dst_addr, size)) {
        LOG_ERROR(Kernel_SVC,
                  "Destination is not within the stack region, addr=0x{:016X}, size=0x{:016X}",
                  dst_addr, size);
        R_THROW(ResultInvalidMemoryRegion);
    }

    if (manager.IsInsideHeapRegion(dst_addr, size)) {
        LOG_ERROR(Kernel_SVC,
                  "Destination does not fit within the heap region, addr=0x{:016X}, "
                  "size=0x{:016X}",
                  dst_addr, size);
        R_THROW(ResultInvalidMemoryRegion);
    }

    if (manager.IsInsideAliasRegion(dst_addr, size)) {
        LOG_ERROR(Kernel_SVC,
                  "Destination does not fit within the map region, addr=0x{:016X}, "
                  "size=0x{:016X}",
                  dst_addr, size);
        R_THROW(ResultInvalidMemoryRegion);
    }

    R_SUCCEED();
}

} // namespace

Result SetMemoryPermission(Core::System& system, u64 address, u64 size, MemoryPermission perm) {
    LOG_DEBUG(Kernel_SVC, "called, address=0x{:016X}, size=0x{:X}, perm=0x{:08X", address, size,
              perm);

    // Validate address / size.
    R_UNLESS(Common::IsAligned(address, PageSize), ResultInvalidAddress);
    R_UNLESS(Common::IsAligned(size, PageSize), ResultInvalidSize);
    R_UNLESS(size > 0, ResultInvalidSize);
    R_UNLESS((address < address + size), ResultInvalidCurrentMemory);

    // Validate the permission.
    R_UNLESS(IsValidSetMemoryPermission(perm), ResultInvalidNewMemoryPermission);

    // Validate that the region is in range for the current process.
    auto& page_table = GetCurrentProcess(system.Kernel()).PageTable();
    R_UNLESS(page_table.Contains(address, size), ResultInvalidCurrentMemory);

    // Set the memory attribute.
    R_RETURN(page_table.SetMemoryPermission(address, size, perm));
}

Result SetMemoryAttribute(Core::System& system, u64 address, u64 size, u32 mask, u32 attr) {
    LOG_DEBUG(Kernel_SVC,
              "called, address=0x{:016X}, size=0x{:X}, mask=0x{:08X}, attribute=0x{:08X}", address,
              size, mask, attr);

    // Validate address / size.
    R_UNLESS(Common::IsAligned(address, PageSize), ResultInvalidAddress);
    R_UNLESS(Common::IsAligned(size, PageSize), ResultInvalidSize);
    R_UNLESS(size > 0, ResultInvalidSize);
    R_UNLESS((address < address + size), ResultInvalidCurrentMemory);

    // Validate the attribute and mask.
    constexpr u32 SupportedMask = static_cast<u32>(MemoryAttribute::Uncached);
    R_UNLESS((mask | attr) == mask, ResultInvalidCombination);
    R_UNLESS((mask | attr | SupportedMask) == SupportedMask, ResultInvalidCombination);

    // Validate that the region is in range for the current process.
    auto& page_table{GetCurrentProcess(system.Kernel()).PageTable()};
    R_UNLESS(page_table.Contains(address, size), ResultInvalidCurrentMemory);

    // Set the memory attribute.
    R_RETURN(page_table.SetMemoryAttribute(address, size, mask, attr));
}

/// Maps a memory range into a different range.
Result MapMemory(Core::System& system, u64 dst_addr, u64 src_addr, u64 size) {
    LOG_TRACE(Kernel_SVC, "called, dst_addr=0x{:X}, src_addr=0x{:X}, size=0x{:X}", dst_addr,
              src_addr, size);

    auto& page_table{GetCurrentProcess(system.Kernel()).PageTable()};

    if (const Result result{MapUnmapMemorySanityChecks(page_table, dst_addr, src_addr, size)};
        result.IsError()) {
        return result;
    }

    R_RETURN(page_table.MapMemory(dst_addr, src_addr, size));
}

/// Unmaps a region that was previously mapped with svcMapMemory
Result UnmapMemory(Core::System& system, u64 dst_addr, u64 src_addr, u64 size) {
    LOG_TRACE(Kernel_SVC, "called, dst_addr=0x{:X}, src_addr=0x{:X}, size=0x{:X}", dst_addr,
              src_addr, size);

    auto& page_table{GetCurrentProcess(system.Kernel()).PageTable()};

    if (const Result result{MapUnmapMemorySanityChecks(page_table, dst_addr, src_addr, size)};
        result.IsError()) {
        return result;
    }

    R_RETURN(page_table.UnmapMemory(dst_addr, src_addr, size));
}

Result SetMemoryPermission64(Core::System& system, uint64_t address, uint64_t size,
                             MemoryPermission perm) {
    R_RETURN(SetMemoryPermission(system, address, size, perm));
}

Result SetMemoryAttribute64(Core::System& system, uint64_t address, uint64_t size, uint32_t mask,
                            uint32_t attr) {
    R_RETURN(SetMemoryAttribute(system, address, size, mask, attr));
}

Result MapMemory64(Core::System& system, uint64_t dst_address, uint64_t src_address,
                   uint64_t size) {
    R_RETURN(MapMemory(system, dst_address, src_address, size));
}

Result UnmapMemory64(Core::System& system, uint64_t dst_address, uint64_t src_address,
                     uint64_t size) {
    R_RETURN(UnmapMemory(system, dst_address, src_address, size));
}

Result SetMemoryPermission64From32(Core::System& system, uint32_t address, uint32_t size,
                                   MemoryPermission perm) {
    R_RETURN(SetMemoryPermission(system, address, size, perm));
}

Result SetMemoryAttribute64From32(Core::System& system, uint32_t address, uint32_t size,
                                  uint32_t mask, uint32_t attr) {
    R_RETURN(SetMemoryAttribute(system, address, size, mask, attr));
}

Result MapMemory64From32(Core::System& system, uint32_t dst_address, uint32_t src_address,
                         uint32_t size) {
    R_RETURN(MapMemory(system, dst_address, src_address, size));
}

Result UnmapMemory64From32(Core::System& system, uint32_t dst_address, uint32_t src_address,
                           uint32_t size) {
    R_RETURN(UnmapMemory(system, dst_address, src_address, size));
}

} // namespace Kernel::Svc
