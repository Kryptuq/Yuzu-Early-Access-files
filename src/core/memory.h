// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include "common/common_types.h"

namespace Common {
struct PageTable;
}

namespace Core {
class System;
}

namespace Kernel {
class PhysicalMemory;
class KProcess;
} // namespace Kernel

namespace Core::Memory {

/**
 * Page size used by the ARM architecture. This is the smallest granularity with which memory can
 * be mapped.
 */
constexpr std::size_t PAGE_BITS = 12;
constexpr u64 PAGE_SIZE = 1ULL << PAGE_BITS;
constexpr u64 PAGE_MASK = PAGE_SIZE - 1;

/// Virtual user-space memory regions
enum : VAddr {
    /// TLS (Thread-Local Storage) related.
    TLS_ENTRY_SIZE = 0x200,

    /// Application stack
    DEFAULT_STACK_SIZE = 0x100000,
};

/// Central class that handles all memory operations and state.
class Memory {
public:
    explicit Memory(Core::System& system);
    ~Memory();

    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    Memory(Memory&&) = default;
    Memory& operator=(Memory&&) = delete;

    /**
     * Resets the state of the Memory system.
     */
    void Reset();

    /**
     * Changes the currently active page table to that of the given process instance.
     *
     * @param process The process to use the page table of.
     */
    void SetCurrentPageTable(Kernel::KProcess& process, u32 core_id);

    /**
     * Maps an allocated buffer onto a region of the emulated process address space.
     *
     * @param page_table The page table of the emulated process.
     * @param base       The address to start mapping at. Must be page-aligned.
     * @param size       The amount of bytes to map. Must be page-aligned.
     * @param target     Buffer with the memory backing the mapping. Must be of length at least
     *                   `size`.
     */
    void MapMemoryRegion(Common::PageTable& page_table, VAddr base, u64 size, PAddr target);

    /**
     * Unmaps a region of the emulated process address space.
     *
     * @param page_table The page table of the emulated process.
     * @param base       The address to begin unmapping at.
     * @param size       The amount of bytes to unmap.
     */
    void UnmapRegion(Common::PageTable& page_table, VAddr base, u64 size);

    /**
     * Checks whether or not the supplied address is a valid virtual
     * address for the current process.
     *
     * @param vaddr The virtual address to check the validity of.
     *
     * @returns True if the given virtual address is valid, false otherwise.
     */
    [[nodiscard]] bool IsValidVirtualAddress(VAddr vaddr) const;

    /**
     * Gets a pointer to the given address.
     *
     * @param vaddr Virtual address to retrieve a pointer to.
     *
     * @returns The pointer to the given address, if the address is valid.
     *          If the address is not valid, nullptr will be returned.
     */
    u8* GetPointer(VAddr vaddr);

    template <typename T>
    T* GetPointer(VAddr vaddr) {
        return reinterpret_cast<T*>(GetPointer(vaddr));
    }

    /**
     * Gets a pointer to the given address.
     *
     * @param vaddr Virtual address to retrieve a pointer to.
     *
     * @returns The pointer to the given address, if the address is valid.
     *          If the address is not valid, nullptr will be returned.
     */
    [[nodiscard]] const u8* GetPointer(VAddr vaddr) const;

    template <typename T>
    const T* GetPointer(VAddr vaddr) const {
        return reinterpret_cast<T*>(GetPointer(vaddr));
    }

    /**
     * Reads an 8-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 8-bit value from.
     *
     * @returns the read 8-bit unsigned value.
     */
    u8 Read8(VAddr addr);

    /**
     * Reads a 16-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 16-bit value from.
     *
     * @returns the read 16-bit unsigned value.
     */
    u16 Read16(VAddr addr);

    /**
     * Reads a 32-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 32-bit value from.
     *
     * @returns the read 32-bit unsigned value.
     */
    u32 Read32(VAddr addr);

    /**
     * Reads a 64-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 64-bit value from.
     *
     * @returns the read 64-bit value.
     */
    u64 Read64(VAddr addr);

    /**
     * Writes an 8-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 8-bit unsigned integer to.
     * @param data The 8-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory at the given virtual address contains the specified data value.
     */
    void Write8(VAddr addr, u8 data);

    /**
     * Writes a 16-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 16-bit unsigned integer to.
     * @param data The 16-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    void Write16(VAddr addr, u16 data);

    /**
     * Writes a 32-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 32-bit unsigned integer to.
     * @param data The 32-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    void Write32(VAddr addr, u32 data);

    /**
     * Writes a 64-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 64-bit unsigned integer to.
     * @param data The 64-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    void Write64(VAddr addr, u64 data);

    /**
     * Writes a 8-bit unsigned integer to the given virtual address in
     * the current process' address space if and only if the address contains
     * the expected value. This operation is atomic.
     *
     * @param addr The virtual address to write the 8-bit unsigned integer to.
     * @param data The 8-bit unsigned integer to write to the given virtual address.
     * @param expected The 8-bit unsigned integer to check against the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    bool WriteExclusive8(VAddr addr, u8 data, u8 expected);

    /**
     * Writes a 16-bit unsigned integer to the given virtual address in
     * the current process' address space if and only if the address contains
     * the expected value. This operation is atomic.
     *
     * @param addr The virtual address to write the 16-bit unsigned integer to.
     * @param data The 16-bit unsigned integer to write to the given virtual address.
     * @param expected The 16-bit unsigned integer to check against the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    bool WriteExclusive16(VAddr addr, u16 data, u16 expected);

    /**
     * Writes a 32-bit unsigned integer to the given virtual address in
     * the current process' address space if and only if the address contains
     * the expected value. This operation is atomic.
     *
     * @param addr The virtual address to write the 32-bit unsigned integer to.
     * @param data The 32-bit unsigned integer to write to the given virtual address.
     * @param expected The 32-bit unsigned integer to check against the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    bool WriteExclusive32(VAddr addr, u32 data, u32 expected);

    /**
     * Writes a 64-bit unsigned integer to the given virtual address in
     * the current process' address space if and only if the address contains
     * the expected value. This operation is atomic.
     *
     * @param addr The virtual address to write the 64-bit unsigned integer to.
     * @param data The 64-bit unsigned integer to write to the given virtual address.
     * @param expected The 64-bit unsigned integer to check against the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    bool WriteExclusive64(VAddr addr, u64 data, u64 expected);

    /**
     * Writes a 128-bit unsigned integer to the given virtual address in
     * the current process' address space if and only if the address contains
     * the expected value. This operation is atomic.
     *
     * @param addr The virtual address to write the 128-bit unsigned integer to.
     * @param data The 128-bit unsigned integer to write to the given virtual address.
     * @param expected The 128-bit unsigned integer to check against the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    bool WriteExclusive128(VAddr addr, u128 data, u128 expected);

    /**
     * Reads a null-terminated string from the given virtual address.
     * This function will continually read characters until either:
     *
     * - A null character ('\0') is reached.
     * - max_length characters have been read.
     *
     * @note The final null-terminating character (if found) is not included
     *       in the returned string.
     *
     * @param vaddr      The address to begin reading the string from.
     * @param max_length The maximum length of the string to read in characters.
     *
     * @returns The read string.
     */
    std::string ReadCString(VAddr vaddr, std::size_t max_length);

    /**
     * Reads a contiguous block of bytes from a specified process' address space.
     *
     * @param process     The process to read the data from.
     * @param src_addr    The virtual address to begin reading from.
     * @param dest_buffer The buffer to place the read bytes into.
     * @param size        The amount of data to read, in bytes.
     *
     * @note If a size of 0 is specified, then this function reads nothing and
     *       no attempts to access memory are made at all.
     *
     * @pre dest_buffer must be at least size bytes in length, otherwise a
     *      buffer overrun will occur.
     *
     * @post The range [dest_buffer, size) contains the read bytes from the
     *       process' address space.
     */
    void ReadBlock(const Kernel::KProcess& process, VAddr src_addr, void* dest_buffer,
                   std::size_t size);

    /**
     * Reads a contiguous block of bytes from the current process' address space.
     *
     * @param src_addr    The virtual address to begin reading from.
     * @param dest_buffer The buffer to place the read bytes into.
     * @param size        The amount of data to read, in bytes.
     *
     * @note If a size of 0 is specified, then this function reads nothing and
     *       no attempts to access memory are made at all.
     *
     * @pre dest_buffer must be at least size bytes in length, otherwise a
     *      buffer overrun will occur.
     *
     * @post The range [dest_buffer, size) contains the read bytes from the
     *       current process' address space.
     */
    void ReadBlock(VAddr src_addr, void* dest_buffer, std::size_t size);

    /**
     * Reads a contiguous block of bytes from the current process' address space.
     * This unsafe version does not trigger GPU flushing.
     *
     * @param src_addr    The virtual address to begin reading from.
     * @param dest_buffer The buffer to place the read bytes into.
     * @param size        The amount of data to read, in bytes.
     *
     * @note If a size of 0 is specified, then this function reads nothing and
     *       no attempts to access memory are made at all.
     *
     * @pre dest_buffer must be at least size bytes in length, otherwise a
     *      buffer overrun will occur.
     *
     * @post The range [dest_buffer, size) contains the read bytes from the
     *       current process' address space.
     */
    void ReadBlockUnsafe(VAddr src_addr, void* dest_buffer, std::size_t size);

    /**
     * Writes a range of bytes into a given process' address space at the specified
     * virtual address.
     *
     * @param process    The process to write data into the address space of.
     * @param dest_addr  The destination virtual address to begin writing the data at.
     * @param src_buffer The data to write into the process' address space.
     * @param size       The size of the data to write, in bytes.
     *
     * @post The address range [dest_addr, size) in the process' address space
     *       contains the data that was within src_buffer.
     *
     * @post If an attempt is made to write into an unmapped region of memory, the writes
     *       will be ignored and an error will be logged.
     *
     * @post If a write is performed into a region of memory that is considered cached
     *       rasterizer memory, will cause the currently active rasterizer to be notified
     *       and will mark that region as invalidated to caches that the active
     *       graphics backend may be maintaining over the course of execution.
     */
    void WriteBlock(const Kernel::KProcess& process, VAddr dest_addr, const void* src_buffer,
                    std::size_t size);

    /**
     * Writes a range of bytes into the current process' address space at the specified
     * virtual address.
     *
     * @param dest_addr  The destination virtual address to begin writing the data at.
     * @param src_buffer The data to write into the current process' address space.
     * @param size       The size of the data to write, in bytes.
     *
     * @post The address range [dest_addr, size) in the current process' address space
     *       contains the data that was within src_buffer.
     *
     * @post If an attempt is made to write into an unmapped region of memory, the writes
     *       will be ignored and an error will be logged.
     *
     * @post If a write is performed into a region of memory that is considered cached
     *       rasterizer memory, will cause the currently active rasterizer to be notified
     *       and will mark that region as invalidated to caches that the active
     *       graphics backend may be maintaining over the course of execution.
     */
    void WriteBlock(VAddr dest_addr, const void* src_buffer, std::size_t size);

    /**
     * Writes a range of bytes into the current process' address space at the specified
     * virtual address.
     * This unsafe version does not invalidate GPU Memory.
     *
     * @param dest_addr  The destination virtual address to begin writing the data at.
     * @param src_buffer The data to write into the current process' address space.
     * @param size       The size of the data to write, in bytes.
     *
     * @post The address range [dest_addr, size) in the current process' address space
     *       contains the data that was within src_buffer.
     *
     * @post If an attempt is made to write into an unmapped region of memory, the writes
     *       will be ignored and an error will be logged.
     *
     */
    void WriteBlockUnsafe(VAddr dest_addr, const void* src_buffer, std::size_t size);

    /**
     * Copies data within a process' address space to another location within the
     * same address space.
     *
     * @param process   The process that will have data copied within its address space.
     * @param dest_addr The destination virtual address to begin copying the data into.
     * @param src_addr  The source virtual address to begin copying the data from.
     * @param size      The size of the data to copy, in bytes.
     *
     * @post The range [dest_addr, size) within the process' address space contains the
     *       same data within the range [src_addr, size).
     */
    void CopyBlock(const Kernel::KProcess& process, VAddr dest_addr, VAddr src_addr,
                   std::size_t size);

    /**
     * Marks each page within the specified address range as cached or uncached.
     *
     * @param vaddr  The virtual address indicating the start of the address range.
     * @param size   The size of the address range in bytes.
     * @param cached Whether or not any pages within the address range should be
     *               marked as cached or uncached.
     */
    void RasterizerMarkRegionCached(VAddr vaddr, u64 size, bool cached);

private:
    Core::System& system;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Core::Memory
