// SPDX-FileCopyrightText: Ryujinx Team and Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <span>
#include <vector>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/command_classes/nvdec_common.h"

namespace Tegra {
class GPU;
namespace Decoder {

class H264BitWriter {
public:
    H264BitWriter();
    ~H264BitWriter();

    /// The following Write methods are based on clause 9.1 in the H.264 specification.
    /// WriteSe and WriteUe write in the Exp-Golomb-coded syntax
    void WriteU(s32 value, s32 value_sz);
    void WriteSe(s32 value);
    void WriteUe(u32 value);

    /// Finalize the bitstream
    void End();

    /// append a bit to the stream, equivalent value to the state parameter
    void WriteBit(bool state);

    /// Based on section 7.3.2.1.1.1 and Table 7-4 in the H.264 specification
    /// Writes the scaling matrices of the sream
    void WriteScalingList(std::span<const u8> list, s32 start, s32 count);

    /// Return the bitstream as a vector.
    [[nodiscard]] std::vector<u8>& GetByteArray();
    [[nodiscard]] const std::vector<u8>& GetByteArray() const;

private:
    void WriteBits(s32 value, s32 bit_count);
    void WriteExpGolombCodedInt(s32 value);
    void WriteExpGolombCodedUInt(u32 value);
    [[nodiscard]] s32 GetFreeBufferBits();
    void Flush();

    s32 buffer_size{8};

    s32 buffer{};
    s32 buffer_pos{};
    std::vector<u8> byte_array;
};

class H264 {
public:
    explicit H264(GPU& gpu);
    ~H264();

    /// Compose the H264 frame for FFmpeg decoding
    [[nodiscard]] const std::vector<u8>& ComposeFrame(const NvdecCommon::NvdecRegisters& state,
                                                      bool is_first_frame = false);

private:
    std::vector<u8> frame;
    GPU& gpu;

    struct H264ParameterSet {
        s32 log2_max_pic_order_cnt_lsb_minus4; ///< 0x00
        s32 delta_pic_order_always_zero_flag;  ///< 0x04
        s32 frame_mbs_only_flag;               ///< 0x08
        u32 pic_width_in_mbs;                  ///< 0x0C
        u32 frame_height_in_map_units;         ///< 0x10
        union {                                ///< 0x14
            BitField<0, 2, u32> tile_format;
            BitField<2, 3, u32> gob_height;
        };
        u32 entropy_coding_mode_flag;               ///< 0x18
        s32 pic_order_present_flag;                 ///< 0x1C
        s32 num_refidx_l0_default_active;           ///< 0x20
        s32 num_refidx_l1_default_active;           ///< 0x24
        s32 deblocking_filter_control_present_flag; ///< 0x28
        s32 redundant_pic_cnt_present_flag;         ///< 0x2C
        u32 transform_8x8_mode_flag;                ///< 0x30
        u32 pitch_luma;                             ///< 0x34
        u32 pitch_chroma;                           ///< 0x38
        u32 luma_top_offset;                        ///< 0x3C
        u32 luma_bot_offset;                        ///< 0x40
        u32 luma_frame_offset;                      ///< 0x44
        u32 chroma_top_offset;                      ///< 0x48
        u32 chroma_bot_offset;                      ///< 0x4C
        u32 chroma_frame_offset;                    ///< 0x50
        u32 hist_buffer_size;                       ///< 0x54
        union {                                     ///< 0x58
            union {
                BitField<0, 1, u64> mbaff_frame;
                BitField<1, 1, u64> direct_8x8_inference;
                BitField<2, 1, u64> weighted_pred;
                BitField<3, 1, u64> constrained_intra_pred;
                BitField<4, 1, u64> ref_pic;
                BitField<5, 1, u64> field_pic;
                BitField<6, 1, u64> bottom_field;
                BitField<7, 1, u64> second_field;
            } flags;
            BitField<8, 4, u64> log2_max_frame_num_minus4;
            BitField<12, 2, u64> chroma_format_idc;
            BitField<14, 2, u64> pic_order_cnt_type;
            BitField<16, 6, s64> pic_init_qp_minus26;
            BitField<22, 5, s64> chroma_qp_index_offset;
            BitField<27, 5, s64> second_chroma_qp_index_offset;
            BitField<32, 2, u64> weighted_bipred_idc;
            BitField<34, 7, u64> curr_pic_idx;
            BitField<41, 5, u64> curr_col_idx;
            BitField<46, 16, u64> frame_number;
            BitField<62, 1, u64> frame_surfaces;
            BitField<63, 1, u64> output_memory_layout;
        };
    };
    static_assert(sizeof(H264ParameterSet) == 0x60, "H264ParameterSet is an invalid size");

    struct H264DecoderContext {
        INSERT_PADDING_WORDS_NOINIT(18);       ///< 0x0000
        u32 stream_len;                        ///< 0x0048
        INSERT_PADDING_WORDS_NOINIT(3);        ///< 0x004C
        H264ParameterSet h264_parameter_set;   ///< 0x0058
        INSERT_PADDING_WORDS_NOINIT(66);       ///< 0x00B8
        std::array<u8, 0x60> weight_scale;     ///< 0x01C0
        std::array<u8, 0x80> weight_scale_8x8; ///< 0x0220
    };
    static_assert(sizeof(H264DecoderContext) == 0x2A0, "H264DecoderContext is an invalid size");

#define ASSERT_POSITION(field_name, position)                                                      \
    static_assert(offsetof(H264ParameterSet, field_name) == position,                              \
                  "Field " #field_name " has invalid position")

    ASSERT_POSITION(log2_max_pic_order_cnt_lsb_minus4, 0x00);
    ASSERT_POSITION(delta_pic_order_always_zero_flag, 0x04);
    ASSERT_POSITION(frame_mbs_only_flag, 0x08);
    ASSERT_POSITION(pic_width_in_mbs, 0x0C);
    ASSERT_POSITION(frame_height_in_map_units, 0x10);
    ASSERT_POSITION(tile_format, 0x14);
    ASSERT_POSITION(entropy_coding_mode_flag, 0x18);
    ASSERT_POSITION(pic_order_present_flag, 0x1C);
    ASSERT_POSITION(num_refidx_l0_default_active, 0x20);
    ASSERT_POSITION(num_refidx_l1_default_active, 0x24);
    ASSERT_POSITION(deblocking_filter_control_present_flag, 0x28);
    ASSERT_POSITION(redundant_pic_cnt_present_flag, 0x2C);
    ASSERT_POSITION(transform_8x8_mode_flag, 0x30);
    ASSERT_POSITION(pitch_luma, 0x34);
    ASSERT_POSITION(pitch_chroma, 0x38);
    ASSERT_POSITION(luma_top_offset, 0x3C);
    ASSERT_POSITION(luma_bot_offset, 0x40);
    ASSERT_POSITION(luma_frame_offset, 0x44);
    ASSERT_POSITION(chroma_top_offset, 0x48);
    ASSERT_POSITION(chroma_bot_offset, 0x4C);
    ASSERT_POSITION(chroma_frame_offset, 0x50);
    ASSERT_POSITION(hist_buffer_size, 0x54);
    ASSERT_POSITION(flags, 0x58);
#undef ASSERT_POSITION

#define ASSERT_POSITION(field_name, position)                                                      \
    static_assert(offsetof(H264DecoderContext, field_name) == position,                            \
                  "Field " #field_name " has invalid position")

    ASSERT_POSITION(stream_len, 0x48);
    ASSERT_POSITION(h264_parameter_set, 0x58);
    ASSERT_POSITION(weight_scale, 0x1C0);
#undef ASSERT_POSITION
};

} // namespace Decoder
} // namespace Tegra
