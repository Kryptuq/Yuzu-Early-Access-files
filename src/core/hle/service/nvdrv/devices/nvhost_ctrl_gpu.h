// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service::Nvidia::Devices {

class nvhost_ctrl_gpu final : public nvdevice {
public:
    explicit nvhost_ctrl_gpu(Core::System& system_);
    ~nvhost_ctrl_gpu() override;

    NvResult Ioctl1(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    const std::vector<u8>& inline_input, std::vector<u8>& output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, const std::vector<u8>& input,
                    std::vector<u8>& output, std::vector<u8>& inline_output) override;

    void OnOpen(DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;

private:
    struct IoctlGpuCharacteristics {
        u32_le arch;                       // 0x120 (NVGPU_GPU_ARCH_GM200)
        u32_le impl;                       // 0xB (NVGPU_GPU_IMPL_GM20B)
        u32_le rev;                        // 0xA1 (Revision A1)
        u32_le num_gpc;                    // 0x1
        u64_le l2_cache_size;              // 0x40000
        u64_le on_board_video_memory_size; // 0x0 (not used)
        u32_le num_tpc_per_gpc;            // 0x2
        u32_le bus_type;                   // 0x20 (NVGPU_GPU_BUS_TYPE_AXI)
        u32_le big_page_size;              // 0x20000
        u32_le compression_page_size;      // 0x20000
        u32_le pde_coverage_bit_count;     // 0x1B
        u32_le available_big_page_sizes;   // 0x30000
        u32_le gpc_mask;                   // 0x1
        u32_le sm_arch_sm_version;         // 0x503 (Maxwell Generation 5.0.3?)
        u32_le sm_arch_spa_version;        // 0x503 (Maxwell Generation 5.0.3?)
        u32_le sm_arch_warp_count;         // 0x80
        u32_le gpu_va_bit_count;           // 0x28
        u32_le reserved;                   // NULL
        u64_le flags;                      // 0x55
        u32_le twod_class;                 // 0x902D (FERMI_TWOD_A)
        u32_le threed_class;               // 0xB197 (MAXWELL_B)
        u32_le compute_class;              // 0xB1C0 (MAXWELL_COMPUTE_B)
        u32_le gpfifo_class;               // 0xB06F (MAXWELL_CHANNEL_GPFIFO_A)
        u32_le inline_to_memory_class;     // 0xA140 (KEPLER_INLINE_TO_MEMORY_B)
        u32_le dma_copy_class;             // 0xB0B5 (MAXWELL_DMA_COPY_A)
        u32_le max_fbps_count;             // 0x1
        u32_le fbp_en_mask;                // 0x0 (disabled)
        u32_le max_ltc_per_fbp;            // 0x2
        u32_le max_lts_per_ltc;            // 0x1
        u32_le max_tex_per_tpc;            // 0x0 (not supported)
        u32_le max_gpc_count;              // 0x1
        u32_le rop_l2_en_mask_0;           // 0x21D70 (fuse_status_opt_rop_l2_fbp_r)
        u32_le rop_l2_en_mask_1;           // 0x0
        u64_le chipname;                   // 0x6230326D67 ("gm20b")
        u64_le gr_compbit_store_base_hw;   // 0x0 (not supported)
    };
    static_assert(sizeof(IoctlGpuCharacteristics) == 160,
                  "IoctlGpuCharacteristics is incorrect size");

    struct IoctlCharacteristics {
        u64_le gpu_characteristics_buf_size; // must not be NULL, but gets overwritten with
                                             // 0xA0=max_size
        u64_le gpu_characteristics_buf_addr; // ignored, but must not be NULL
        IoctlGpuCharacteristics gc;
    };
    static_assert(sizeof(IoctlCharacteristics) == 16 + sizeof(IoctlGpuCharacteristics),
                  "IoctlCharacteristics is incorrect size");

    struct IoctlGpuGetTpcMasksArgs {
        u32_le mask_buffer_size{};
        INSERT_PADDING_WORDS(1);
        u64_le mask_buffer_address{};
        u32_le tcp_mask{};
        INSERT_PADDING_WORDS(1);
    };
    static_assert(sizeof(IoctlGpuGetTpcMasksArgs) == 24,
                  "IoctlGpuGetTpcMasksArgs is incorrect size");

    struct IoctlActiveSlotMask {
        u32_le slot; // always 0x07
        u32_le mask;
    };
    static_assert(sizeof(IoctlActiveSlotMask) == 8, "IoctlActiveSlotMask is incorrect size");

    struct IoctlZcullGetCtxSize {
        u32_le size;
    };
    static_assert(sizeof(IoctlZcullGetCtxSize) == 4, "IoctlZcullGetCtxSize is incorrect size");

    struct IoctlNvgpuGpuZcullGetInfoArgs {
        u32_le width_align_pixels;
        u32_le height_align_pixels;
        u32_le pixel_squares_by_aliquots;
        u32_le aliquot_total;
        u32_le region_byte_multiplier;
        u32_le region_header_size;
        u32_le subregion_header_size;
        u32_le subregion_width_align_pixels;
        u32_le subregion_height_align_pixels;
        u32_le subregion_count;
    };
    static_assert(sizeof(IoctlNvgpuGpuZcullGetInfoArgs) == 40,
                  "IoctlNvgpuGpuZcullGetInfoArgs is incorrect size");

    struct IoctlZbcSetTable {
        u32_le color_ds[4];
        u32_le color_l2[4];
        u32_le depth;
        u32_le format;
        u32_le type;
    };
    static_assert(sizeof(IoctlZbcSetTable) == 44, "IoctlZbcSetTable is incorrect size");

    struct IoctlZbcQueryTable {
        u32_le color_ds[4];
        u32_le color_l2[4];
        u32_le depth;
        u32_le ref_cnt;
        u32_le format;
        u32_le type;
        u32_le index_size;
    };
    static_assert(sizeof(IoctlZbcQueryTable) == 52, "IoctlZbcQueryTable is incorrect size");

    struct IoctlFlushL2 {
        u32_le flush; // l2_flush | l2_invalidate << 1 | fb_flush << 2
        u32_le reserved;
    };
    static_assert(sizeof(IoctlFlushL2) == 8, "IoctlFlushL2 is incorrect size");

    struct IoctlGetGpuTime {
        u64_le gpu_time{};
        INSERT_PADDING_WORDS(2);
    };
    static_assert(sizeof(IoctlGetGpuTime) == 0x10, "IoctlGetGpuTime is incorrect size");

    NvResult GetCharacteristics(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult GetCharacteristics(const std::vector<u8>& input, std::vector<u8>& output,
                                std::vector<u8>& inline_output);

    NvResult GetTPCMasks(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult GetTPCMasks(const std::vector<u8>& input, std::vector<u8>& output,
                         std::vector<u8>& inline_output);

    NvResult GetActiveSlotMask(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ZCullGetCtxSize(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ZCullGetInfo(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ZBCSetTable(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult ZBCQueryTable(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult FlushL2(const std::vector<u8>& input, std::vector<u8>& output);
    NvResult GetGpuTime(const std::vector<u8>& input, std::vector<u8>& output);
};

} // namespace Service::Nvidia::Devices
