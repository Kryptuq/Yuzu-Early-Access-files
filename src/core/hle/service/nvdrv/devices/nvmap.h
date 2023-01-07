// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/core/nvmap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service::Nvidia::NvCore {
class Container;
} // namespace Service::Nvidia::NvCore

namespace Service::Nvidia::Devices {

class nvmap final : public nvdevice {
public:
    explicit nvmap(Core::System& system_, NvCore::Container& container);
    ~nvmap() override;

    nvmap(const nvmap&) = delete;
    nvmap& operator=(const nvmap&) = delete;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                    std::span<const u8> inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input, std::vector<u8>& output,
                    std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

    enum class HandleParameterType : u32_le {
        Size = 1,
        Alignment = 2,
        Base = 3,
        Heap = 4,
        Kind = 5,
        IsSharedMemMapped = 6
    };

private:
    /// Id to use for the next handle that is created.
    u32 next_handle = 0;

    /// Id to use for the next object that is created.
    u32 next_id = 0;

    struct IocCreateParams {
        // Input
        u32_le size{};
        // Output
        u32_le handle{};
    };
    static_assert(sizeof(IocCreateParams) == 8, "IocCreateParams has wrong size");

    struct IocFromIdParams {
        // Input
        u32_le id{};
        // Output
        u32_le handle{};
    };
    static_assert(sizeof(IocFromIdParams) == 8, "IocFromIdParams has wrong size");

    struct IocAllocParams {
        // Input
        u32_le handle{};
        u32_le heap_mask{};
        NvCore::NvMap::Handle::Flags flags{};
        u32_le align{};
        u8 kind{};
        INSERT_PADDING_BYTES(7);
        u64_le address{};
    };
    static_assert(sizeof(IocAllocParams) == 32, "IocAllocParams has wrong size");

    struct IocFreeParams {
        u32_le handle{};
        INSERT_PADDING_BYTES(4);
        u64_le address{};
        u32_le size{};
        NvCore::NvMap::Handle::Flags flags{};
    };
    static_assert(sizeof(IocFreeParams) == 24, "IocFreeParams has wrong size");

    struct IocParamParams {
        // Input
        u32_le handle{};
        HandleParameterType param{};
        // Output
        u32_le result{};
    };
    static_assert(sizeof(IocParamParams) == 12, "IocParamParams has wrong size");

    struct IocGetIdParams {
        // Output
        u32_le id{};
        // Input
        u32_le handle{};
    };
    static_assert(sizeof(IocGetIdParams) == 8, "IocGetIdParams has wrong size");

    NvResult IocCreate(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocAlloc(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocGetId(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocFromId(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocParam(std::span<const u8> input, std::vector<u8>& output);
    NvResult IocFree(std::span<const u8> input, std::vector<u8>& output);

    NvCore::Container& container;
    NvCore::NvMap& file;
};

} // namespace Service::Nvidia::Devices
