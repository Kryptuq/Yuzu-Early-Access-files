// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/host1x/nvdec_common.h"

namespace Tegra {

namespace Host1x {
class Host1x;
} // namespace Host1x

namespace Decoder {

class VP8 {
public:
    explicit VP8(Host1x::Host1x& host1x);
    ~VP8();

    /// Compose the VP8 frame for FFmpeg decoding
    [[nodiscard]] const std::vector<u8>& ComposeFrame(
        const Host1x::NvdecCommon::NvdecRegisters& state);

private:
    std::vector<u8> frame;
    Host1x::Host1x& host1x;

    struct VP8PictureInfo {
        INSERT_PADDING_WORDS_NOINIT(14);
        u16 frame_width;  // actual frame width
        u16 frame_height; // actual frame height
        u8 key_frame;
        u8 version;
        union {
            u8 raw;
            BitField<0, 2, u8> tile_format;
            BitField<2, 3, u8> gob_height;
            BitField<5, 3, u8> reserverd_surface_format;
        };
        u8 error_conceal_on;  // 1: error conceal on; 0: off
        u32 first_part_size;  // the size of first partition(frame header and mb header partition)
        u32 hist_buffer_size; // in units of 256
        u32 vld_buffer_size;  // in units of 1
        // Current frame buffers
        std::array<u32, 2> frame_stride; // [y_c]
        u32 luma_top_offset;             // offset of luma top field in units of 256
        u32 luma_bot_offset;             // offset of luma bottom field in units of 256
        u32 luma_frame_offset;           // offset of luma frame in units of 256
        u32 chroma_top_offset;           // offset of chroma top field in units of 256
        u32 chroma_bot_offset;           // offset of chroma bottom field in units of 256
        u32 chroma_frame_offset;         // offset of chroma frame in units of 256

        INSERT_PADDING_BYTES_NOINIT(0x1c); // NvdecDisplayParams

        // Decode picture buffer related
        s8 current_output_memory_layout;
        // output NV12/NV24 setting. index 0: golden; 1: altref; 2: last
        std::array<s8, 3> output_memory_layout;

        u8 segmentation_feature_data_update;
        INSERT_PADDING_BYTES_NOINIT(3);

        // ucode return result
        u32 result_value;
        std::array<u32, 8> partition_offset;
        INSERT_PADDING_WORDS_NOINIT(3);
    };
    static_assert(sizeof(VP8PictureInfo) == 0xc0, "PictureInfo is an invalid size");
};

} // namespace Decoder
} // namespace Tegra
