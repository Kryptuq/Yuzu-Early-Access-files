// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: 2021 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <bit>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "common/address_space.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Tegra {
class MemoryManager;
} // namespace Tegra

namespace Service::Nvidia {
class Module;
}

namespace Service::Nvidia::NvCore {
class Container;
class NvMap;
} // namespace Service::Nvidia::NvCore

namespace Service::Nvidia::Devices {

enum class MappingFlags : u32 {
    None = 0,
    Fixed = 1 << 0,
    Sparse = 1 << 1,
    Remap = 1 << 8,
};
DECLARE_ENUM_FLAG_OPERATORS(MappingFlags);

class nvhost_as_gpu final : public nvdevice {
public:
    explicit nvhost_as_gpu(Core::System& system_, Module& module, NvCore::Container& core);
    ~nvhost_as_gpu() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    const std::vector<u8>& inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output, std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

    Kernel::KEvent* QueryEvent(u32 event_id) override;

    struct VaRegion {
        u64 offset;
        u32 page_size;
        u32 _pad0_;
        u64 pages;
    };
    static_assert(sizeof(VaRegion) == 0x18);

private:
    struct IoctlAllocAsEx {
        u32_le flags{}; // usually passes 1
        s32_le as_fd{}; // ignored; passes 0
        u32_le big_page_size{};
        u32_le reserved{}; // ignored; passes 0
        u64_le va_range_start{};
        u64_le va_range_end{};
        u64_le va_range_split{};
    };
    static_assert(sizeof(IoctlAllocAsEx) == 40, "IoctlAllocAsEx is incorrect size");

    struct IoctlAllocSpace {
        u32_le pages{};
        u32_le page_size{};
        MappingFlags flags{};
        INSERT_PADDING_WORDS(1);
        union {
            u64_le offset;
            u64_le align;
        };
    };
    static_assert(sizeof(IoctlAllocSpace) == 24, "IoctlInitalizeEx is incorrect size");

    struct IoctlFreeSpace {
        u64_le offset{};
        u32_le pages{};
        u32_le page_size{};
    };
    static_assert(sizeof(IoctlFreeSpace) == 16, "IoctlFreeSpace is incorrect size");

    struct IoctlRemapEntry {
        u16 flags;
        u16 kind;
        NvCore::NvMap::Handle::Id handle;
        u32 handle_offset_big_pages;
        u32 as_offset_big_pages;
        u32 big_pages;
    };
    static_assert(sizeof(IoctlRemapEntry) == 20, "IoctlRemapEntry is incorrect size");

    struct IoctlMapBufferEx {
        MappingFlags flags{}; // bit0: fixed_offset, bit2: cacheable
        u32_le kind{};        // -1 is default
        NvCore::NvMap::Handle::Id handle;
        u32_le page_size{}; // 0 means don't care
        s64_le buffer_offset{};
        u64_le mapping_size{};
        s64_le offset{};
    };
    static_assert(sizeof(IoctlMapBufferEx) == 40, "IoctlMapBufferEx is incorrect size");

    struct IoctlUnmapBuffer {
        s64_le offset{};
    };
    static_assert(sizeof(IoctlUnmapBuffer) == 8, "IoctlUnmapBuffer is incorrect size");

    struct IoctlBindChannel {
        s32_le fd{};
    };
    static_assert(sizeof(IoctlBindChannel) == 4, "IoctlBindChannel is incorrect size");

    struct IoctlGetVaRegions {
        u64_le buf_addr{}; // (contained output user ptr on linux, ignored)
        u32_le buf_size{}; // forced to 2*sizeof(struct va_region)
        u32_le reserved{};
        std::array<VaRegion, 2> regions{};
    };
    static_assert(sizeof(IoctlGetVaRegions) == 16 + sizeof(VaRegion) * 2,
                  "IoctlGetVaRegions is incorrect size");

    NvResult AllocAsEx(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult AllocateSpace(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult Remap(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult MapBufferEx(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult UnmapBuffer(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult FreeSpace(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult BindChannel(const std::vector<u8>& input, std::vector<u8>& output);

    void GetVARegionsImpl(IoctlGetVaRegions& params);
    NvResult GetVARegions(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult GetVARegions(const std::vector<u8>& input, std::vector<u8>& output,
                          std::vector<u8>& inline_output);

    void FreeMappingLocked(u64 offset);

    Module& module;

    NvCore::Container& container;
    NvCore::NvMap& nvmap;

    struct Mapping {
        VAddr ptr;
        u64 offset;
        u64 size;
        bool fixed;
        bool big_page; // Only valid if fixed == false
        bool sparse_alloc;

        Mapping(VAddr ptr_, u64 offset_, u64 size_, bool fixed_, bool big_page_, bool sparse_alloc_)
            : ptr(ptr_), offset(offset_), size(size_), fixed(fixed_), big_page(big_page_),
              sparse_alloc(sparse_alloc_) {}
    };

    struct Allocation {
        u64 size;
        std::list<std::shared_ptr<Mapping>> mappings;
        u32 page_size;
        bool sparse;
        bool big_pages;
    };

    std::map<u64, std::shared_ptr<Mapping>>
        mapping_map; //!< This maps the base addresses of mapped buffers to their total sizes and
                     //!< mapping type, this is needed as what was originally a single buffer may
                     //!< have been split into multiple GPU side buffers with the remap flag.
    std::map<u64, Allocation> allocation_map; //!< Holds allocations created by AllocSpace from
                                              //!< which fixed buffers can be mapped into
    std::mutex mutex;                         //!< Locks all AS operations

    struct VM {
        static constexpr u32 PAGE_SIZE{0x1000};
        static constexpr u32 PAGE_SIZE_BITS{std::countr_zero(PAGE_SIZE)};

        static constexpr u32 SUPPORTED_BIG_PAGE_SIZES{0x30000};
        static constexpr u32 DEFAULT_BIG_PAGE_SIZE{0x20000};
        u32 big_page_size{DEFAULT_BIG_PAGE_SIZE};
        u32 big_page_size_bits{std::countr_zero(DEFAULT_BIG_PAGE_SIZE)};

        static constexpr u32 VA_START_SHIFT{10};
        static constexpr u64 DEFAULT_VA_SPLIT{1ULL << 34};
        static constexpr u64 DEFAULT_VA_RANGE{1ULL << 37};
        u64 va_range_start{DEFAULT_BIG_PAGE_SIZE << VA_START_SHIFT};
        u64 va_range_split{DEFAULT_VA_SPLIT};
        u64 va_range_end{DEFAULT_VA_RANGE};

        using Allocator = Common::FlatAllocator<u32, 0, 32>;

        std::unique_ptr<Allocator> big_page_allocator;
        std::shared_ptr<Allocator>
            small_page_allocator; //! Shared as this is also used by nvhost::GpuChannel

        bool initialised{};
    } vm;
    std::shared_ptr<Tegra::MemoryManager> gmmu;

    // s32 channel{};
    // u32 big_page_size{VM::DEFAULT_BIG_PAGE_SIZE};
};

} // namespace Service::Nvidia::Devices
