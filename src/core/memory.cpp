// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>

#include "common/assert.h"
#include "common/atomic_ops.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/page_table.h"
#include "common/settings.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/memory.h"
#include "video_core/gpu.h"

namespace Core::Memory {

// Implementation class used to keep the specifics of the memory subsystem hidden
// from outside classes. This also allows modification to the internals of the memory
// subsystem without needing to rebuild all files that make use of the memory interface.
struct Memory::Impl {
    explicit Impl(Core::System& system_) : system{system_} {}

    void SetCurrentPageTable(Kernel::KProcess& process, u32 core_id) {
        current_page_table = &process.PageTable().PageTableImpl();
        current_page_table->fastmem_arena = system.DeviceMemory().buffer.VirtualBasePointer();

        const std::size_t address_space_width = process.PageTable().GetAddressSpaceWidth();

        system.ArmInterface(core_id).PageTableChanged(*current_page_table, address_space_width);
    }

    void MapMemoryRegion(Common::PageTable& page_table, VAddr base, u64 size, PAddr target) {
        ASSERT_MSG((size & YUZU_PAGEMASK) == 0, "non-page aligned size: {:016X}", size);
        ASSERT_MSG((base & YUZU_PAGEMASK) == 0, "non-page aligned base: {:016X}", base);
        ASSERT_MSG(target >= DramMemoryMap::Base, "Out of bounds target: {:016X}", target);
        MapPages(page_table, base / YUZU_PAGESIZE, size / YUZU_PAGESIZE, target,
                 Common::PageType::Memory);

        if (Settings::IsFastmemEnabled()) {
            system.DeviceMemory().buffer.Map(base, target - DramMemoryMap::Base, size);
        }
    }

    void UnmapRegion(Common::PageTable& page_table, VAddr base, u64 size) {
        ASSERT_MSG((size & YUZU_PAGEMASK) == 0, "non-page aligned size: {:016X}", size);
        ASSERT_MSG((base & YUZU_PAGEMASK) == 0, "non-page aligned base: {:016X}", base);
        MapPages(page_table, base / YUZU_PAGESIZE, size / YUZU_PAGESIZE, 0,
                 Common::PageType::Unmapped);

        if (Settings::IsFastmemEnabled()) {
            system.DeviceMemory().buffer.Unmap(base, size);
        }
    }

    [[nodiscard]] u8* GetPointerFromRasterizerCachedMemory(VAddr vaddr) const {
        const PAddr paddr{current_page_table->backing_addr[vaddr >> YUZU_PAGEBITS]};

        if (!paddr) {
            return {};
        }

        return system.DeviceMemory().GetPointer<u8>(paddr) + vaddr;
    }

    [[nodiscard]] u8* GetPointerFromDebugMemory(VAddr vaddr) const {
        const PAddr paddr{current_page_table->backing_addr[vaddr >> YUZU_PAGEBITS]};

        if (paddr == 0) {
            return {};
        }

        return system.DeviceMemory().GetPointer<u8>(paddr) + vaddr;
    }

    u8 Read8(const VAddr addr) {
        return Read<u8>(addr);
    }

    u16 Read16(const VAddr addr) {
        if ((addr & 1) == 0) {
            return Read<u16_le>(addr);
        } else {
            const u32 a{Read<u8>(addr)};
            const u32 b{Read<u8>(addr + sizeof(u8))};
            return static_cast<u16>((b << 8) | a);
        }
    }

    u32 Read32(const VAddr addr) {
        if ((addr & 3) == 0) {
            return Read<u32_le>(addr);
        } else {
            const u32 a{Read16(addr)};
            const u32 b{Read16(addr + sizeof(u16))};
            return (b << 16) | a;
        }
    }

    u64 Read64(const VAddr addr) {
        if ((addr & 7) == 0) {
            return Read<u64_le>(addr);
        } else {
            const u32 a{Read32(addr)};
            const u32 b{Read32(addr + sizeof(u32))};
            return (static_cast<u64>(b) << 32) | a;
        }
    }

    void Write8(const VAddr addr, const u8 data) {
        Write<u8>(addr, data);
    }

    void Write16(const VAddr addr, const u16 data) {
        if ((addr & 1) == 0) {
            Write<u16_le>(addr, data);
        } else {
            Write<u8>(addr, static_cast<u8>(data));
            Write<u8>(addr + sizeof(u8), static_cast<u8>(data >> 8));
        }
    }

    void Write32(const VAddr addr, const u32 data) {
        if ((addr & 3) == 0) {
            Write<u32_le>(addr, data);
        } else {
            Write16(addr, static_cast<u16>(data));
            Write16(addr + sizeof(u16), static_cast<u16>(data >> 16));
        }
    }

    void Write64(const VAddr addr, const u64 data) {
        if ((addr & 7) == 0) {
            Write<u64_le>(addr, data);
        } else {
            Write32(addr, static_cast<u32>(data));
            Write32(addr + sizeof(u32), static_cast<u32>(data >> 32));
        }
    }

    bool WriteExclusive8(const VAddr addr, const u8 data, const u8 expected) {
        return WriteExclusive<u8>(addr, data, expected);
    }

    bool WriteExclusive16(const VAddr addr, const u16 data, const u16 expected) {
        return WriteExclusive<u16_le>(addr, data, expected);
    }

    bool WriteExclusive32(const VAddr addr, const u32 data, const u32 expected) {
        return WriteExclusive<u32_le>(addr, data, expected);
    }

    bool WriteExclusive64(const VAddr addr, const u64 data, const u64 expected) {
        return WriteExclusive<u64_le>(addr, data, expected);
    }

    std::string ReadCString(VAddr vaddr, std::size_t max_length) {
        std::string string;
        string.reserve(max_length);
        for (std::size_t i = 0; i < max_length; ++i) {
            const char c = Read<s8>(vaddr);
            if (c == '\0') {
                break;
            }
            string.push_back(c);
            ++vaddr;
        }
        string.shrink_to_fit();
        return string;
    }

    void WalkBlock(const Kernel::KProcess& process, const VAddr addr, const std::size_t size,
                   auto on_unmapped, auto on_memory, auto on_rasterizer, auto increment) {
        const auto& page_table = process.PageTable().PageTableImpl();
        std::size_t remaining_size = size;
        std::size_t page_index = addr >> YUZU_PAGEBITS;
        std::size_t page_offset = addr & YUZU_PAGEMASK;

        while (remaining_size) {
            const std::size_t copy_amount =
                std::min(static_cast<std::size_t>(YUZU_PAGESIZE) - page_offset, remaining_size);
            const auto current_vaddr =
                static_cast<VAddr>((page_index << YUZU_PAGEBITS) + page_offset);

            const auto [pointer, type] = page_table.pointers[page_index].PointerType();
            switch (type) {
            case Common::PageType::Unmapped: {
                on_unmapped(copy_amount, current_vaddr);
                break;
            }
            case Common::PageType::Memory: {
                u8* mem_ptr = pointer + page_offset + (page_index << YUZU_PAGEBITS);
                on_memory(copy_amount, mem_ptr);
                break;
            }
            case Common::PageType::DebugMemory: {
                u8* const mem_ptr{GetPointerFromDebugMemory(current_vaddr)};
                on_memory(copy_amount, mem_ptr);
                break;
            }
            case Common::PageType::RasterizerCachedMemory: {
                u8* const host_ptr{GetPointerFromRasterizerCachedMemory(current_vaddr)};
                on_rasterizer(current_vaddr, copy_amount, host_ptr);
                break;
            }
            default:
                UNREACHABLE();
            }

            page_index++;
            page_offset = 0;
            increment(copy_amount);
            remaining_size -= copy_amount;
        }
    }

    template <bool UNSAFE>
    void ReadBlockImpl(const Kernel::KProcess& process, const VAddr src_addr, void* dest_buffer,
                       const std::size_t size) {
        WalkBlock(
            process, src_addr, size,
            [src_addr, size, &dest_buffer](const std::size_t copy_amount,
                                           const VAddr current_vaddr) {
                LOG_ERROR(HW_Memory,
                          "Unmapped ReadBlock @ 0x{:016X} (start address = 0x{:016X}, size = {})",
                          current_vaddr, src_addr, size);
                std::memset(dest_buffer, 0, copy_amount);
            },
            [&](const std::size_t copy_amount, const u8* const src_ptr) {
                std::memcpy(dest_buffer, src_ptr, copy_amount);
            },
            [&](const VAddr current_vaddr, const std::size_t copy_amount,
                const u8* const host_ptr) {
                if constexpr (!UNSAFE) {
                    system.GPU().FlushRegion(current_vaddr, copy_amount);
                }
                std::memcpy(dest_buffer, host_ptr, copy_amount);
            },
            [&](const std::size_t copy_amount) {
                dest_buffer = static_cast<u8*>(dest_buffer) + copy_amount;
            });
    }

    void ReadBlock(const VAddr src_addr, void* dest_buffer, const std::size_t size) {
        ReadBlockImpl<false>(*system.CurrentProcess(), src_addr, dest_buffer, size);
    }

    void ReadBlockUnsafe(const VAddr src_addr, void* dest_buffer, const std::size_t size) {
        ReadBlockImpl<true>(*system.CurrentProcess(), src_addr, dest_buffer, size);
    }

    template <bool UNSAFE>
    void WriteBlockImpl(const Kernel::KProcess& process, const VAddr dest_addr,
                        const void* src_buffer, const std::size_t size) {
        WalkBlock(
            process, dest_addr, size,
            [dest_addr, size](const std::size_t copy_amount, const VAddr current_vaddr) {
                LOG_ERROR(HW_Memory,
                          "Unmapped WriteBlock @ 0x{:016X} (start address = 0x{:016X}, size = {})",
                          current_vaddr, dest_addr, size);
            },
            [&](const std::size_t copy_amount, u8* const dest_ptr) {
                std::memcpy(dest_ptr, src_buffer, copy_amount);
            },
            [&](const VAddr current_vaddr, const std::size_t copy_amount, u8* const host_ptr) {
                if constexpr (!UNSAFE) {
                    system.GPU().InvalidateRegion(current_vaddr, copy_amount);
                }
                std::memcpy(host_ptr, src_buffer, copy_amount);
            },
            [&](const std::size_t copy_amount) {
                src_buffer = static_cast<const u8*>(src_buffer) + copy_amount;
            });
    }

    void WriteBlock(const VAddr dest_addr, const void* src_buffer, const std::size_t size) {
        WriteBlockImpl<false>(*system.CurrentProcess(), dest_addr, src_buffer, size);
    }

    void WriteBlockUnsafe(const VAddr dest_addr, const void* src_buffer, const std::size_t size) {
        WriteBlockImpl<true>(*system.CurrentProcess(), dest_addr, src_buffer, size);
    }

    void ZeroBlock(const Kernel::KProcess& process, const VAddr dest_addr, const std::size_t size) {
        WalkBlock(
            process, dest_addr, size,
            [dest_addr, size](const std::size_t copy_amount, const VAddr current_vaddr) {
                LOG_ERROR(HW_Memory,
                          "Unmapped ZeroBlock @ 0x{:016X} (start address = 0x{:016X}, size = {})",
                          current_vaddr, dest_addr, size);
            },
            [](const std::size_t copy_amount, u8* const dest_ptr) {
                std::memset(dest_ptr, 0, copy_amount);
            },
            [&](const VAddr current_vaddr, const std::size_t copy_amount, u8* const host_ptr) {
                system.GPU().InvalidateRegion(current_vaddr, copy_amount);
                std::memset(host_ptr, 0, copy_amount);
            },
            [](const std::size_t copy_amount) {});
    }

    void CopyBlock(const Kernel::KProcess& process, VAddr dest_addr, VAddr src_addr,
                   const std::size_t size) {
        WalkBlock(
            process, dest_addr, size,
            [&](const std::size_t copy_amount, const VAddr current_vaddr) {
                LOG_ERROR(HW_Memory,
                          "Unmapped CopyBlock @ 0x{:016X} (start address = 0x{:016X}, size = {})",
                          current_vaddr, src_addr, size);
                ZeroBlock(process, dest_addr, copy_amount);
            },
            [&](const std::size_t copy_amount, const u8* const src_ptr) {
                WriteBlockImpl<false>(process, dest_addr, src_ptr, copy_amount);
            },
            [&](const VAddr current_vaddr, const std::size_t copy_amount, u8* const host_ptr) {
                system.GPU().FlushRegion(current_vaddr, copy_amount);
                WriteBlockImpl<false>(process, dest_addr, host_ptr, copy_amount);
            },
            [&](const std::size_t copy_amount) {
                dest_addr += static_cast<VAddr>(copy_amount);
                src_addr += static_cast<VAddr>(copy_amount);
            });
    }

    template <typename Callback>
    Result PerformCacheOperation(const Kernel::KProcess& process, VAddr dest_addr, std::size_t size,
                                 Callback&& cb) {
        class InvalidMemoryException : public std::exception {};

        try {
            WalkBlock(
                process, dest_addr, size,
                [&](const std::size_t block_size, const VAddr current_vaddr) {
                    LOG_ERROR(HW_Memory, "Unmapped cache maintenance @ {:#018X}", current_vaddr);
                    throw InvalidMemoryException();
                },
                [&](const std::size_t block_size, u8* const host_ptr) {},
                [&](const VAddr current_vaddr, const std::size_t block_size, u8* const host_ptr) {
                    cb(current_vaddr, block_size);
                },
                [](const std::size_t block_size) {});
        } catch (InvalidMemoryException&) {
            return Kernel::ResultInvalidCurrentMemory;
        }

        return ResultSuccess;
    }

    Result InvalidateDataCache(const Kernel::KProcess& process, VAddr dest_addr, std::size_t size) {
        auto on_rasterizer = [&](const VAddr current_vaddr, const std::size_t block_size) {
            // dc ivac: Invalidate to point of coherency
            // GPU flush -> CPU invalidate
            system.GPU().FlushRegion(current_vaddr, block_size);
        };
        return PerformCacheOperation(process, dest_addr, size, on_rasterizer);
    }

    Result StoreDataCache(const Kernel::KProcess& process, VAddr dest_addr, std::size_t size) {
        auto on_rasterizer = [&](const VAddr current_vaddr, const std::size_t block_size) {
            // dc cvac: Store to point of coherency
            // CPU flush -> GPU invalidate
            system.GPU().InvalidateRegion(current_vaddr, block_size);
        };
        return PerformCacheOperation(process, dest_addr, size, on_rasterizer);
    }

    Result FlushDataCache(const Kernel::KProcess& process, VAddr dest_addr, std::size_t size) {
        auto on_rasterizer = [&](const VAddr current_vaddr, const std::size_t block_size) {
            // dc civac: Store to point of coherency, and invalidate from cache
            // CPU flush -> GPU invalidate
            system.GPU().InvalidateRegion(current_vaddr, block_size);
        };
        return PerformCacheOperation(process, dest_addr, size, on_rasterizer);
    }

    void MarkRegionDebug(VAddr vaddr, u64 size, bool debug) {
        if (vaddr == 0) {
            return;
        }

        // Iterate over a contiguous CPU address space, marking/unmarking the region.
        // The region is at a granularity of CPU pages.

        const u64 num_pages = ((vaddr + size - 1) >> YUZU_PAGEBITS) - (vaddr >> YUZU_PAGEBITS) + 1;
        for (u64 i = 0; i < num_pages; ++i, vaddr += YUZU_PAGESIZE) {
            const Common::PageType page_type{
                current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Type()};
            if (debug) {
                // Switch page type to debug if now debug
                switch (page_type) {
                case Common::PageType::Unmapped:
                    ASSERT_MSG(false, "Attempted to mark unmapped pages as debug");
                    break;
                case Common::PageType::RasterizerCachedMemory:
                case Common::PageType::DebugMemory:
                    // Page is already marked.
                    break;
                case Common::PageType::Memory:
                    current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Store(
                        nullptr, Common::PageType::DebugMemory);
                    break;
                default:
                    UNREACHABLE();
                }
            } else {
                // Switch page type to non-debug if now non-debug
                switch (page_type) {
                case Common::PageType::Unmapped:
                    ASSERT_MSG(false, "Attempted to mark unmapped pages as non-debug");
                    break;
                case Common::PageType::RasterizerCachedMemory:
                case Common::PageType::Memory:
                    // Don't mess with already non-debug or rasterizer memory.
                    break;
                case Common::PageType::DebugMemory: {
                    u8* const pointer{GetPointerFromDebugMemory(vaddr & ~YUZU_PAGEMASK)};
                    current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Store(
                        pointer - (vaddr & ~YUZU_PAGEMASK), Common::PageType::Memory);
                    break;
                }
                default:
                    UNREACHABLE();
                }
            }
        }
    }

    void RasterizerMarkRegionCached(VAddr vaddr, u64 size, bool cached) {
        if (vaddr == 0) {
            return;
        }

        if (Settings::IsFastmemEnabled()) {
            const bool is_read_enable = !Settings::IsGPULevelExtreme() || !cached;
            system.DeviceMemory().buffer.Protect(vaddr, size, is_read_enable, !cached);
        }

        // Iterate over a contiguous CPU address space, which corresponds to the specified GPU
        // address space, marking the region as un/cached. The region is marked un/cached at a
        // granularity of CPU pages, hence why we iterate on a CPU page basis (note: GPU page size
        // is different). This assumes the specified GPU address region is contiguous as well.

        const u64 num_pages = ((vaddr + size - 1) >> YUZU_PAGEBITS) - (vaddr >> YUZU_PAGEBITS) + 1;
        for (u64 i = 0; i < num_pages; ++i, vaddr += YUZU_PAGESIZE) {
            const Common::PageType page_type{
                current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Type()};
            if (cached) {
                // Switch page type to cached if now cached
                switch (page_type) {
                case Common::PageType::Unmapped:
                    // It is not necessary for a process to have this region mapped into its address
                    // space, for example, a system module need not have a VRAM mapping.
                    break;
                case Common::PageType::DebugMemory:
                case Common::PageType::Memory:
                    current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Store(
                        nullptr, Common::PageType::RasterizerCachedMemory);
                    break;
                case Common::PageType::RasterizerCachedMemory:
                    // There can be more than one GPU region mapped per CPU region, so it's common
                    // that this area is already marked as cached.
                    break;
                default:
                    UNREACHABLE();
                }
            } else {
                // Switch page type to uncached if now uncached
                switch (page_type) {
                case Common::PageType::Unmapped: // NOLINT(bugprone-branch-clone)
                    // It is not necessary for a process to have this region mapped into its address
                    // space, for example, a system module need not have a VRAM mapping.
                    break;
                case Common::PageType::DebugMemory:
                case Common::PageType::Memory:
                    // There can be more than one GPU region mapped per CPU region, so it's common
                    // that this area is already unmarked as cached.
                    break;
                case Common::PageType::RasterizerCachedMemory: {
                    u8* const pointer{GetPointerFromRasterizerCachedMemory(vaddr & ~YUZU_PAGEMASK)};
                    if (pointer == nullptr) {
                        // It's possible that this function has been called while updating the
                        // pagetable after unmapping a VMA. In that case the underlying VMA will no
                        // longer exist, and we should just leave the pagetable entry blank.
                        current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Store(
                            nullptr, Common::PageType::Unmapped);
                    } else {
                        current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Store(
                            pointer - (vaddr & ~YUZU_PAGEMASK), Common::PageType::Memory);
                    }
                    break;
                }
                default:
                    UNREACHABLE();
                }
            }
        }
    }

    /**
     * Maps a region of pages as a specific type.
     *
     * @param page_table The page table to use to perform the mapping.
     * @param base       The base address to begin mapping at.
     * @param size       The total size of the range in bytes.
     * @param target     The target address to begin mapping from.
     * @param type       The page type to map the memory as.
     */
    void MapPages(Common::PageTable& page_table, VAddr base, u64 size, PAddr target,
                  Common::PageType type) {
        LOG_DEBUG(HW_Memory, "Mapping {:016X} onto {:016X}-{:016X}", target, base * YUZU_PAGESIZE,
                  (base + size) * YUZU_PAGESIZE);

        // During boot, current_page_table might not be set yet, in which case we need not flush
        if (system.IsPoweredOn()) {
            auto& gpu = system.GPU();
            for (u64 i = 0; i < size; i++) {
                const auto page = base + i;
                if (page_table.pointers[page].Type() == Common::PageType::RasterizerCachedMemory) {
                    gpu.FlushAndInvalidateRegion(page << YUZU_PAGEBITS, YUZU_PAGESIZE);
                }
            }
        }

        const VAddr end = base + size;
        ASSERT_MSG(end <= page_table.pointers.size(), "out of range mapping at {:016X}",
                   base + page_table.pointers.size());

        if (!target) {
            ASSERT_MSG(type != Common::PageType::Memory,
                       "Mapping memory page without a pointer @ {:016x}", base * YUZU_PAGESIZE);

            while (base != end) {
                page_table.pointers[base].Store(nullptr, type);
                page_table.backing_addr[base] = 0;

                base += 1;
            }
        } else {
            while (base != end) {
                page_table.pointers[base].Store(
                    system.DeviceMemory().GetPointer<u8>(target) - (base << YUZU_PAGEBITS), type);
                page_table.backing_addr[base] = target - (base << YUZU_PAGEBITS);

                ASSERT_MSG(page_table.pointers[base].Pointer(),
                           "memory mapping base yield a nullptr within the table");

                base += 1;
                target += YUZU_PAGESIZE;
            }
        }
    }

    [[nodiscard]] u8* GetPointerImpl(VAddr vaddr, auto on_unmapped, auto on_rasterizer) const {
        // AARCH64 masks the upper 16 bit of all memory accesses
        vaddr &= 0xffffffffffffULL;

        if (vaddr >= 1uLL << current_page_table->GetAddressSpaceBits()) {
            on_unmapped();
            return nullptr;
        }

        // Avoid adding any extra logic to this fast-path block
        const uintptr_t raw_pointer = current_page_table->pointers[vaddr >> YUZU_PAGEBITS].Raw();
        if (u8* const pointer = Common::PageTable::PageInfo::ExtractPointer(raw_pointer)) {
            return &pointer[vaddr];
        }
        switch (Common::PageTable::PageInfo::ExtractType(raw_pointer)) {
        case Common::PageType::Unmapped:
            on_unmapped();
            return nullptr;
        case Common::PageType::Memory:
            ASSERT_MSG(false, "Mapped memory page without a pointer @ 0x{:016X}", vaddr);
            return nullptr;
        case Common::PageType::DebugMemory:
            return GetPointerFromDebugMemory(vaddr);
        case Common::PageType::RasterizerCachedMemory: {
            u8* const host_ptr{GetPointerFromRasterizerCachedMemory(vaddr)};
            on_rasterizer();
            return host_ptr;
        }
        default:
            UNREACHABLE();
        }
        return nullptr;
    }

    [[nodiscard]] u8* GetPointer(const VAddr vaddr) const {
        return GetPointerImpl(
            vaddr, [vaddr]() { LOG_ERROR(HW_Memory, "Unmapped GetPointer @ 0x{:016X}", vaddr); },
            []() {});
    }

    [[nodiscard]] u8* GetPointerSilent(const VAddr vaddr) const {
        return GetPointerImpl(
            vaddr, []() {}, []() {});
    }

    /**
     * Reads a particular data type out of memory at the given virtual address.
     *
     * @param vaddr The virtual address to read the data type from.
     *
     * @tparam T The data type to read out of memory. This type *must* be
     *           trivially copyable, otherwise the behavior of this function
     *           is undefined.
     *
     * @returns The instance of T read from the specified virtual address.
     */
    template <typename T>
    T Read(VAddr vaddr) {
        T result = 0;
        const u8* const ptr = GetPointerImpl(
            vaddr,
            [vaddr]() {
                LOG_ERROR(HW_Memory, "Unmapped Read{} @ 0x{:016X}", sizeof(T) * 8, vaddr);
            },
            [&]() { system.GPU().FlushRegion(vaddr, sizeof(T)); });
        if (ptr) {
            std::memcpy(&result, ptr, sizeof(T));
        }
        return result;
    }

    /**
     * Writes a particular data type to memory at the given virtual address.
     *
     * @param vaddr The virtual address to write the data type to.
     *
     * @tparam T The data type to write to memory. This type *must* be
     *           trivially copyable, otherwise the behavior of this function
     *           is undefined.
     */
    template <typename T>
    void Write(VAddr vaddr, const T data) {
        u8* const ptr = GetPointerImpl(
            vaddr,
            [vaddr, data]() {
                LOG_ERROR(HW_Memory, "Unmapped Write{} @ 0x{:016X} = 0x{:016X}", sizeof(T) * 8,
                          vaddr, static_cast<u64>(data));
            },
            [&]() { system.GPU().InvalidateRegion(vaddr, sizeof(T)); });
        if (ptr) {
            std::memcpy(ptr, &data, sizeof(T));
        }
    }

    template <typename T>
    bool WriteExclusive(VAddr vaddr, const T data, const T expected) {
        u8* const ptr = GetPointerImpl(
            vaddr,
            [vaddr, data]() {
                LOG_ERROR(HW_Memory, "Unmapped WriteExclusive{} @ 0x{:016X} = 0x{:016X}",
                          sizeof(T) * 8, vaddr, static_cast<u64>(data));
            },
            [&]() { system.GPU().InvalidateRegion(vaddr, sizeof(T)); });
        if (ptr) {
            const auto volatile_pointer = reinterpret_cast<volatile T*>(ptr);
            return Common::AtomicCompareAndSwap(volatile_pointer, data, expected);
        }
        return true;
    }

    bool WriteExclusive128(VAddr vaddr, const u128 data, const u128 expected) {
        u8* const ptr = GetPointerImpl(
            vaddr,
            [vaddr, data]() {
                LOG_ERROR(HW_Memory, "Unmapped WriteExclusive128 @ 0x{:016X} = 0x{:016X}{:016X}",
                          vaddr, static_cast<u64>(data[1]), static_cast<u64>(data[0]));
            },
            [&]() { system.GPU().InvalidateRegion(vaddr, sizeof(u128)); });
        if (ptr) {
            const auto volatile_pointer = reinterpret_cast<volatile u64*>(ptr);
            return Common::AtomicCompareAndSwap(volatile_pointer, data, expected);
        }
        return true;
    }

    Common::PageTable* current_page_table = nullptr;
    Core::System& system;
};

Memory::Memory(Core::System& system_) : system{system_} {
    Reset();
}

Memory::~Memory() = default;

void Memory::Reset() {
    impl = std::make_unique<Impl>(system);
}

void Memory::SetCurrentPageTable(Kernel::KProcess& process, u32 core_id) {
    impl->SetCurrentPageTable(process, core_id);
}

void Memory::MapMemoryRegion(Common::PageTable& page_table, VAddr base, u64 size, PAddr target) {
    impl->MapMemoryRegion(page_table, base, size, target);
}

void Memory::UnmapRegion(Common::PageTable& page_table, VAddr base, u64 size) {
    impl->UnmapRegion(page_table, base, size);
}

bool Memory::IsValidVirtualAddress(const VAddr vaddr) const {
    const Kernel::KProcess& process = *system.CurrentProcess();
    const auto& page_table = process.PageTable().PageTableImpl();
    const size_t page = vaddr >> YUZU_PAGEBITS;
    if (page >= page_table.pointers.size()) {
        return false;
    }
    const auto [pointer, type] = page_table.pointers[page].PointerType();
    return pointer != nullptr || type == Common::PageType::RasterizerCachedMemory ||
           type == Common::PageType::DebugMemory;
}

bool Memory::IsValidVirtualAddressRange(VAddr base, u64 size) const {
    VAddr end = base + size;
    VAddr page = Common::AlignDown(base, YUZU_PAGESIZE);

    for (; page < end; page += YUZU_PAGESIZE) {
        if (!IsValidVirtualAddress(page)) {
            return false;
        }
    }

    return true;
}

u8* Memory::GetPointer(VAddr vaddr) {
    return impl->GetPointer(vaddr);
}

u8* Memory::GetPointerSilent(VAddr vaddr) {
    return impl->GetPointerSilent(vaddr);
}

const u8* Memory::GetPointer(VAddr vaddr) const {
    return impl->GetPointer(vaddr);
}

u8 Memory::Read8(const VAddr addr) {
    return impl->Read8(addr);
}

u16 Memory::Read16(const VAddr addr) {
    return impl->Read16(addr);
}

u32 Memory::Read32(const VAddr addr) {
    return impl->Read32(addr);
}

u64 Memory::Read64(const VAddr addr) {
    return impl->Read64(addr);
}

void Memory::Write8(VAddr addr, u8 data) {
    impl->Write8(addr, data);
}

void Memory::Write16(VAddr addr, u16 data) {
    impl->Write16(addr, data);
}

void Memory::Write32(VAddr addr, u32 data) {
    impl->Write32(addr, data);
}

void Memory::Write64(VAddr addr, u64 data) {
    impl->Write64(addr, data);
}

bool Memory::WriteExclusive8(VAddr addr, u8 data, u8 expected) {
    return impl->WriteExclusive8(addr, data, expected);
}

bool Memory::WriteExclusive16(VAddr addr, u16 data, u16 expected) {
    return impl->WriteExclusive16(addr, data, expected);
}

bool Memory::WriteExclusive32(VAddr addr, u32 data, u32 expected) {
    return impl->WriteExclusive32(addr, data, expected);
}

bool Memory::WriteExclusive64(VAddr addr, u64 data, u64 expected) {
    return impl->WriteExclusive64(addr, data, expected);
}

bool Memory::WriteExclusive128(VAddr addr, u128 data, u128 expected) {
    return impl->WriteExclusive128(addr, data, expected);
}

std::string Memory::ReadCString(VAddr vaddr, std::size_t max_length) {
    return impl->ReadCString(vaddr, max_length);
}

void Memory::ReadBlock(const Kernel::KProcess& process, const VAddr src_addr, void* dest_buffer,
                       const std::size_t size) {
    impl->ReadBlockImpl<false>(process, src_addr, dest_buffer, size);
}

void Memory::ReadBlock(const VAddr src_addr, void* dest_buffer, const std::size_t size) {
    impl->ReadBlock(src_addr, dest_buffer, size);
}

void Memory::ReadBlockUnsafe(const VAddr src_addr, void* dest_buffer, const std::size_t size) {
    impl->ReadBlockUnsafe(src_addr, dest_buffer, size);
}

void Memory::WriteBlock(const Kernel::KProcess& process, VAddr dest_addr, const void* src_buffer,
                        std::size_t size) {
    impl->WriteBlockImpl<false>(process, dest_addr, src_buffer, size);
}

void Memory::WriteBlock(const VAddr dest_addr, const void* src_buffer, const std::size_t size) {
    impl->WriteBlock(dest_addr, src_buffer, size);
}

void Memory::WriteBlockUnsafe(const VAddr dest_addr, const void* src_buffer,
                              const std::size_t size) {
    impl->WriteBlockUnsafe(dest_addr, src_buffer, size);
}

void Memory::CopyBlock(const Kernel::KProcess& process, VAddr dest_addr, VAddr src_addr,
                       const std::size_t size) {
    impl->CopyBlock(process, dest_addr, src_addr, size);
}

void Memory::ZeroBlock(const Kernel::KProcess& process, VAddr dest_addr, const std::size_t size) {
    impl->ZeroBlock(process, dest_addr, size);
}

Result Memory::InvalidateDataCache(const Kernel::KProcess& process, VAddr dest_addr,
                                   const std::size_t size) {
    return impl->InvalidateDataCache(process, dest_addr, size);
}

Result Memory::StoreDataCache(const Kernel::KProcess& process, VAddr dest_addr,
                              const std::size_t size) {
    return impl->StoreDataCache(process, dest_addr, size);
}

Result Memory::FlushDataCache(const Kernel::KProcess& process, VAddr dest_addr,
                              const std::size_t size) {
    return impl->FlushDataCache(process, dest_addr, size);
}

void Memory::RasterizerMarkRegionCached(VAddr vaddr, u64 size, bool cached) {
    impl->RasterizerMarkRegionCached(vaddr, size, cached);
}

void Memory::MarkRegionDebug(VAddr vaddr, u64 size, bool debug) {
    impl->MarkRegionDebug(vaddr, size, debug);
}

} // namespace Core::Memory
