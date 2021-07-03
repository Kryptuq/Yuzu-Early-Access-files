// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Tegra::NvdecCommon {

enum class VideoCodec : u64 {
    None = 0x0,
    H264 = 0x3,
    Vp8 = 0x5,
    H265 = 0x7,
    Vp9 = 0x9,
};

// NVDEC should use a 32-bit address space, but is mapped to 64-bit,
// doubling the sizes here is compensating for that.
struct NvdecRegisters {
    static constexpr std::size_t NUM_REGS = 0x178;

    union {
        struct {
            INSERT_PADDING_WORDS_NOINIT(256); ///< 0x0000
            VideoCodec set_codec_id;          ///< 0x0400
            INSERT_PADDING_WORDS_NOINIT(126); ///< 0x0408
            u64 execute;                      ///< 0x0600
            INSERT_PADDING_WORDS_NOINIT(126); ///< 0x0608
            struct {                          ///< 0x0800
                union {
                    BitField<0, 3, VideoCodec> codec;
                    BitField<4, 1, u64> gp_timer_on;
                    BitField<13, 1, u64> mb_timer_on;
                    BitField<14, 1, u64> intra_frame_pslc;
                    BitField<17, 1, u64> all_intra_frame;
                };
            } control_params;
            u64 picture_info_offset;                   ///< 0x0808
            u64 frame_bitstream_offset;                ///< 0x0810
            u64 frame_number;                          ///< 0x0818
            u64 h264_slice_data_offsets;               ///< 0x0820
            u64 h264_mv_dump_offset;                   ///< 0x0828
            INSERT_PADDING_WORDS_NOINIT(6);            ///< 0x0830
            u64 frame_stats_offset;                    ///< 0x0848
            u64 h264_last_surface_luma_offset;         ///< 0x0850
            u64 h264_last_surface_chroma_offset;       ///< 0x0858
            std::array<u64, 17> surface_luma_offset;   ///< 0x0860
            std::array<u64, 17> surface_chroma_offset; ///< 0x08E8
            INSERT_PADDING_WORDS_NOINIT(132);          ///< 0x0970
            u64 vp9_entropy_probs_offset;              ///< 0x0B80
            u64 vp9_backward_updates_offset;           ///< 0x0B88
            u64 vp9_last_frame_segmap_offset;          ///< 0x0B90
            u64 vp9_curr_frame_segmap_offset;          ///< 0x0B98
            INSERT_PADDING_WORDS_NOINIT(2);            ///< 0x0BA0
            u64 vp9_last_frame_mvs_offset;             ///< 0x0BA8
            u64 vp9_curr_frame_mvs_offset;             ///< 0x0BB0
            INSERT_PADDING_WORDS_NOINIT(2);            ///< 0x0BB8
        };
        std::array<u64, NUM_REGS> reg_array;
    };
};
static_assert(sizeof(NvdecRegisters) == (0xBC0), "NvdecRegisters is incorrect size");

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(NvdecRegisters, field_name) == position * sizeof(u64),                  \
                  "Field " #field_name " has invalid position")

ASSERT_REG_POSITION(set_codec_id, 0x80);
ASSERT_REG_POSITION(execute, 0xC0);
ASSERT_REG_POSITION(control_params, 0x100);
ASSERT_REG_POSITION(picture_info_offset, 0x101);
ASSERT_REG_POSITION(frame_bitstream_offset, 0x102);
ASSERT_REG_POSITION(frame_number, 0x103);
ASSERT_REG_POSITION(h264_slice_data_offsets, 0x104);
ASSERT_REG_POSITION(frame_stats_offset, 0x109);
ASSERT_REG_POSITION(h264_last_surface_luma_offset, 0x10A);
ASSERT_REG_POSITION(h264_last_surface_chroma_offset, 0x10B);
ASSERT_REG_POSITION(surface_luma_offset, 0x10C);
ASSERT_REG_POSITION(surface_chroma_offset, 0x11D);
ASSERT_REG_POSITION(vp9_entropy_probs_offset, 0x170);
ASSERT_REG_POSITION(vp9_backward_updates_offset, 0x171);
ASSERT_REG_POSITION(vp9_last_frame_segmap_offset, 0x172);
ASSERT_REG_POSITION(vp9_curr_frame_segmap_offset, 0x173);
ASSERT_REG_POSITION(vp9_last_frame_mvs_offset, 0x175);
ASSERT_REG_POSITION(vp9_curr_frame_mvs_offset, 0x176);

#undef ASSERT_REG_POSITION

} // namespace Tegra::NvdecCommon
