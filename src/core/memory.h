// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include "common/typed_address.h"
#include "core/hle/result.h"

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
constexpr std::size_t YUZU_PAGEBITS = 12;
constexpr u64 YUZU_PAGESIZE = 1ULL << YUZU_PAGEBITS;
constexpr u64 YUZU_PAGEMASK = YUZU_PAGESIZE - 1;

/// Virtual user-space memory regions
enum : u64 {
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
    void MapMemoryRegion(Common::PageTable& page_table, Common::ProcessAddress base, u64 size,
                         Common::PhysicalAddress target);

    /**
     * Unmaps a region of the emulated process address space.
     *
     * @param page_table The page table of the emulated process.
     * @param base       The address to begin unmapping at.
     * @param size       The amount of bytes to unmap.
     */
    void UnmapRegion(Common::PageTable& page_table, Common::ProcessAddress base, u64 size);

    /**
     * Checks whether or not the supplied address is a valid virtual
     * address for the current process.
     *
     * @param vaddr The virtual address to check the validity of.
     *
     * @returns True if the given virtual address is valid, false otherwise.
     */
    [[nodiscard]] bool IsValidVirtualAddress(Common::ProcessAddress vaddr) const;

    /**
     * Checks whether or not the supplied range of addresses are all valid
     * virtual addresses for the current process.
     *
     * @param base The address to begin checking.
     * @param size The amount of bytes to check.
     *
     * @returns True if all bytes in the given range are valid, false otherwise.
     */
    [[nodiscard]] bool IsValidVirtualAddressRange(Common::ProcessAddress base, u64 size) const;

    /**
     * Gets a pointer to the given address.
     *
     * @param vaddr Virtual address to retrieve a pointer to.
     *
     * @returns The pointer to the given address, if the address is valid.
     *          If the address is not valid, nullptr will be returned.
     */
    u8* GetPointer(Common::ProcessAddress vaddr);
    u8* GetPointerSilent(Common::ProcessAddress vaddr);

    template <typename T>
    T* GetPointer(Common::ProcessAddress vaddr) {
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
    [[nodiscard]] const u8* GetPointer(Common::ProcessAddress vaddr) const;

    template <typename T>
    const T* GetPointer(Common::ProcessAddress vaddr) const {
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
    u8 Read8(Common::ProcessAddress addr);

    /**
     * Reads a 16-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 16-bit value from.
     *
     * @returns the read 16-bit unsigned value.
     */
    u16 Read16(Common::ProcessAddress addr);

    /**
     * Reads a 32-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 32-bit value from.
     *
     * @returns the read 32-bit unsigned value.
     */
    u32 Read32(Common::ProcessAddress addr);

    /**
     * Reads a 64-bit unsigned value from the current process' address space
     * at the given virtual address.
     *
     * @param addr The virtual address to read the 64-bit value from.
     *
     * @returns the read 64-bit value.
     */
    u64 Read64(Common::ProcessAddress addr);

    /**
     * Writes an 8-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 8-bit unsigned integer to.
     * @param data The 8-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory at the given virtual address contains the specified data value.
     */
    void Write8(Common::ProcessAddress addr, u8 data);

    /**
     * Writes a 16-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 16-bit unsigned integer to.
     * @param data The 16-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    void Write16(Common::ProcessAddress addr, u16 data);

    /**
     * Writes a 32-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 32-bit unsigned integer to.
     * @param data The 32-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    void Write32(Common::ProcessAddress addr, u32 data);

    /**
     * Writes a 64-bit unsigned integer to the given virtual address in
     * the current process' address space.
     *
     * @param addr The virtual address to write the 64-bit unsigned integer to.
     * @param data The 64-bit unsigned integer to write to the given virtual address.
     *
     * @post The memory range [addr, sizeof(data)) contains the given data value.
     */
    void Write64(Common::ProcessAddress addr, u64 data);

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
    bool WriteExclusive8(Common::ProcessAddress addr, u8 data, u8 expected);

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
    bool WriteExclusive16(Common::ProcessAddress addr, u16 data, u16 expected);

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
    bool WriteExclusive32(Common::ProcessAddress addr, u32 data, u32 expected);

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
    bool WriteExclusive64(Common::ProcessAddress addr, u64 data, u64 expected);

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
    bool WriteExclusive128(Common::ProcessAddress addr, u128 data, u128 expected);

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
    std::string ReadCString(Common::ProcessAddress vaddr, std::size_t max_length);

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
    void ReadBlock(const Kernel::KProcess& process, Common::ProcessAddress src_addr,
                   void* dest_buffer, std::size_t size);

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
    void ReadBlock(Common::ProcessAddress src_addr, void* dest_buffer, std::size_t size);

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
    void ReadBlockUnsafe(Common::ProcessAddress src_addr, void* dest_buffer, std::size_t size);

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
    void WriteBlock(const Kernel::KProcess& process, Common::ProcessAddress dest_addr,
                    const void* src_buffer, std::size_t size);

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
    void WriteBlock(Common::ProcessAddress dest_addr, const void* src_buffer, std::size_t size);

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
    void WriteBlockUnsafe(Common::ProcessAddress dest_addr, const void* src_buffer,
                          std::size_t size);

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
    void CopyBlock(const Kernel::KProcess& process, Common::ProcessAddress dest_addr,
                   Common::ProcessAddress src_addr, std::size_t size);

    /**
     * Zeros a range of bytes within the current process' address space at the specified
     * virtual address.
     *
     * @param process   The process that will have data zeroed within its address space.
     * @param dest_addr The destination virtual address to zero the data from.
     * @param size      The size of the range to zero out, in bytes.
     *
     * @post The range [dest_addr, size) within the process' address space contains the
     *       value 0.
     */
    void ZeroBlock(const Kernel::KProcess& process, Common::ProcessAddress dest_addr,
                   std::size_t size);

    /**
     * Invalidates a range of bytes within the current process' address space at the specified
     * virtual address.
     *
     * @param process   The process that will have data invalidated within its address space.
     * @param dest_addr The destination virtual address to invalidate the data from.
     * @param size      The size of the range to invalidate, in bytes.
     *
     */
    Result InvalidateDataCache(const Kernel::KProcess& process, Common::ProcessAddress dest_addr,
                               std::size_t size);

    /**
     * Stores a range of bytes within the current process' address space at the specified
     * virtual address.
     *
     * @param process   The process that will have data stored within its address space.
     * @param dest_addr The destination virtual address to store the data from.
     * @param size      The size of the range to store, in bytes.
     *
     */
    Result StoreDataCache(const Kernel::KProcess& process, Common::ProcessAddress dest_addr,
                          std::size_t size);

    /**
     * Flushes a range of bytes within the current process' address space at the specified
     * virtual address.
     *
     * @param process   The process that will have data flushed within its address space.
     * @param dest_addr The destination virtual address to flush the data from.
     * @param size      The size of the range to flush, in bytes.
     *
     */
    Result FlushDataCache(const Kernel::KProcess& process, Common::ProcessAddress dest_addr,
                          std::size_t size);

    /**
     * Marks each page within the specified address range as cached or uncached.
     *
     * @param vaddr  The virtual address indicating the start of the address range.
     * @param size   The size of the address range in bytes.
     * @param cached Whether or not any pages within the address range should be
     *               marked as cached or uncached.
     */
    void RasterizerMarkRegionCached(Common::ProcessAddress vaddr, u64 size, bool cached);

    /**
     * Marks each page within the specified address range as debug or non-debug.
     * Debug addresses are not accessible from fastmem pointers.
     *
     * @param vaddr The virtual address indicating the start of the address range.
     * @param size  The size of the address range in bytes.
     * @param debug Whether or not any pages within the address range should be
     *              marked as debug or non-debug.
     */
    void MarkRegionDebug(Common::ProcessAddress vaddr, u64 size, bool debug);

private:
    Core::System& system;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Core::Memory
