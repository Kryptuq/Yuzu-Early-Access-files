// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service::Nvidia::Devices {

constexpr u32 DEFAULT_BIG_PAGE_SIZE = 1 << 16;
constexpr u32 DEFAULT_SMALL_PAGE_SIZE = 1 << 12;

class nvmap;

enum class AddressSpaceFlags : u32 {
    None = 0x0,
    FixedOffset = 0x1,
    Remap = 0x100,
};
DECLARE_ENUM_FLAG_OPERATORS(AddressSpaceFlags);

class nvhost_as_gpu final : public nvdevice {
public:
    explicit nvhost_as_gpu(Core::System& system_, std::shared_ptr<nvmap> nvmap_dev_);
    ~nvhost_as_gpu() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    const std::vector<u8>& inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output, std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

private:
    class BufferMap final {
    public:
        constexpr BufferMap() = default;

        constexpr BufferMap(GPUVAddr start_addr_, std::size_t size_)
            : start_addr{start_addr_}, end_addr{start_addr_ + size_} {}

        constexpr BufferMap(GPUVAddr start_addr_, std::size_t size_, VAddr cpu_addr_,
                            bool is_allocated_)
            : start_addr{start_addr_}, end_addr{start_addr_ + size_}, cpu_addr{cpu_addr_},
              is_allocated{is_allocated_} {}

        constexpr VAddr StartAddr() const {
            return start_addr;
        }

        constexpr VAddr EndAddr() const {
            return end_addr;
        }

        constexpr std::size_t Size() const {
            return end_addr - start_addr;
        }

        constexpr VAddr CpuAddr() const {
            return cpu_addr;
        }

        constexpr bool IsAllocated() const {
            return is_allocated;
        }

    private:
        GPUVAddr start_addr{};
        GPUVAddr end_addr{};
        VAddr cpu_addr{};
        bool is_allocated{};
    };

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
        AddressSpaceFlags flags{};
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
        u16_le flags{};
        u16_le kind{};
        u32_le nvmap_handle{};
        u32_le map_offset{};
        u32_le offset{};
        u32_le pages{};
    };
    static_assert(sizeof(IoctlRemapEntry) == 20, "IoctlRemapEntry is incorrect size");

    struct IoctlMapBufferEx {
        AddressSpaceFlags flags{}; // bit0: fixed_offset, bit2: cacheable
        u32_le kind{};             // -1 is default
        u32_le nvmap_handle{};
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

    struct IoctlVaRegion {
        u64_le offset{};
        u32_le page_size{};
        INSERT_PADDING_WORDS(1);
        u64_le pages{};
    };
    static_assert(sizeof(IoctlVaRegion) == 24, "IoctlVaRegion is incorrect size");

    struct IoctlGetVaRegions {
        u64_le buf_addr{}; // (contained output user ptr on linux, ignored)
        u32_le buf_size{}; // forced to 2*sizeof(struct va_region)
        u32_le reserved{};
        IoctlVaRegion small{};
        IoctlVaRegion big{};
    };
    static_assert(sizeof(IoctlGetVaRegions) == 16 + sizeof(IoctlVaRegion) * 2,
                  "IoctlGetVaRegions is incorrect size");

    s32 channel{};
    u32 big_page_size{DEFAULT_BIG_PAGE_SIZE};

    NvResult AllocAsEx(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult AllocateSpace(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult Remap(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult MapBufferEx(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult UnmapBuffer(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult FreeSpace(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult BindChannel(const std::vector<u8>& input, std::vector<u8>& output);

    NvResult GetVARegions(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult GetVARegions(const std::vector<u8>& input, std::vector<u8>& output,
                          std::vector<u8>& inline_output);

    std::optional<BufferMap> FindBufferMap(GPUVAddr gpu_addr) const;
    void AddBufferMap(GPUVAddr gpu_addr, std::size_t size, VAddr cpu_addr, bool is_allocated);
    std::optional<std::size_t> RemoveBufferMap(GPUVAddr gpu_addr);

    std::shared_ptr<nvmap> nvmap_dev;

    // This is expected to be ordered, therefore we must use a map, not unordered_map
    std::map<GPUVAddr, BufferMap> buffer_mappings;
};

} // namespace Service::Nvidia::Devices
