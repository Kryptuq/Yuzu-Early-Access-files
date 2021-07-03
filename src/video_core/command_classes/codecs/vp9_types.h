// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstring>
#include <vector>
#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Tegra {
class GPU;

namespace Decoder {
struct Vp9FrameDimensions {
    s16 width;
    s16 height;
    s16 luma_pitch;
    s16 chroma_pitch;
};
static_assert(sizeof(Vp9FrameDimensions) == 0x8, "Vp9 Vp9FrameDimensions is an invalid size");

enum FrameFlags : u32 {
    IsKeyFrame = 1 << 0,
    LastFrameIsKeyFrame = 1 << 1,
    FrameSizeChanged = 1 << 2,
    ErrorResilientMode = 1 << 3,
    LastShowFrame = 1 << 4,
    IntraOnly = 1 << 5,
};

enum class TxSize {
    Tx4x4 = 0,   // 4x4 transform
    Tx8x8 = 1,   // 8x8 transform
    Tx16x16 = 2, // 16x16 transform
    Tx32x32 = 3, // 32x32 transform
    TxSizes = 4
};

enum class TxMode {
    Only4X4 = 0,      // Only 4x4 transform used
    Allow8X8 = 1,     // Allow block transform size up to 8x8
    Allow16X16 = 2,   // Allow block transform size up to 16x16
    Allow32X32 = 3,   // Allow block transform size up to 32x32
    TxModeSelect = 4, // Transform specified for each block
    TxModes = 5
};

struct Segmentation {
    u8 enabled;
    u8 update_map;
    u8 temporal_update;
    u8 abs_delta;
    std::array<u32, 8> feature_mask;
    std::array<std::array<s16, 4>, 8> feature_data;
};
static_assert(sizeof(Segmentation) == 0x64, "Segmentation is an invalid size");

struct LoopFilter {
    u8 mode_ref_delta_enabled;
    std::array<s8, 4> ref_deltas;
    std::array<s8, 2> mode_deltas;
};
static_assert(sizeof(LoopFilter) == 0x7, "LoopFilter is an invalid size");

struct Vp9EntropyProbs {
    std::array<u8, 36> y_mode_prob;           ///< 0x0000
    std::array<u8, 64> partition_prob;        ///< 0x0024
    std::array<u8, 1728> coef_probs;          ///< 0x0064
    std::array<u8, 8> switchable_interp_prob; ///< 0x0724
    std::array<u8, 28> inter_mode_prob;       ///< 0x072C
    std::array<u8, 4> intra_inter_prob;       ///< 0x0748
    std::array<u8, 5> comp_inter_prob;        ///< 0x074C
    std::array<u8, 10> single_ref_prob;       ///< 0x0751
    std::array<u8, 5> comp_ref_prob;          ///< 0x075B
    std::array<u8, 6> tx_32x32_prob;          ///< 0x0760
    std::array<u8, 4> tx_16x16_prob;          ///< 0x0766
    std::array<u8, 2> tx_8x8_prob;            ///< 0x076A
    std::array<u8, 3> skip_probs;             ///< 0x076C
    std::array<u8, 3> joints;                 ///< 0x076F
    std::array<u8, 2> sign;                   ///< 0x0772
    std::array<u8, 20> classes;               ///< 0x0774
    std::array<u8, 2> class_0;                ///< 0x0788
    std::array<u8, 20> prob_bits;             ///< 0x078A
    std::array<u8, 12> class_0_fr;            ///< 0x079E
    std::array<u8, 6> fr;                     ///< 0x07AA
    std::array<u8, 2> class_0_hp;             ///< 0x07B0
    std::array<u8, 2> high_precision;         ///< 0x07B2
};
static_assert(sizeof(Vp9EntropyProbs) == 0x7B4, "Vp9EntropyProbs is an invalid size");

struct Vp9PictureInfo {
    bool is_key_frame;
    bool intra_only;
    bool last_frame_was_key;
    bool frame_size_changed;
    bool error_resilient_mode;
    bool last_frame_shown;
    bool show_frame;
    std::array<s8, 4> ref_frame_sign_bias;
    s32 base_q_index;
    s32 y_dc_delta_q;
    s32 uv_dc_delta_q;
    s32 uv_ac_delta_q;
    bool lossless;
    s32 transform_mode;
    bool allow_high_precision_mv;
    s32 interp_filter;
    s32 reference_mode;
    s8 comp_fixed_ref;
    std::array<s8, 2> comp_var_ref;
    s32 log2_tile_cols;
    s32 log2_tile_rows;
    bool segment_enabled;
    bool segment_map_update;
    bool segment_map_temporal_update;
    s32 segment_abs_delta;
    std::array<u32, 8> segment_feature_enable;
    std::array<std::array<s16, 4>, 8> segment_feature_data;
    bool mode_ref_delta_enabled;
    bool use_prev_in_find_mv_refs;
    std::array<s8, 4> ref_deltas;
    std::array<s8, 2> mode_deltas;
    Vp9EntropyProbs entropy;
    Vp9FrameDimensions frame_size;
    u8 first_level;
    u8 sharpness_level;
    u32 bitstream_size;
    std::array<u64, 4> frame_offsets;
    std::array<bool, 4> refresh_frame;
};

struct Vp9FrameContainer {
    Vp9PictureInfo info{};
    std::vector<u8> bit_stream;
};

struct PictureInfo {
    INSERT_PADDING_WORDS_NOINIT(12);       ///< 0x00
    u32 bitstream_size;                    ///< 0x30
    INSERT_PADDING_WORDS_NOINIT(5);        ///< 0x34
    Vp9FrameDimensions last_frame_size;    ///< 0x48
    Vp9FrameDimensions golden_frame_size;  ///< 0x50
    Vp9FrameDimensions alt_frame_size;     ///< 0x58
    Vp9FrameDimensions current_frame_size; ///< 0x60
    u32 vp9_flags;                         ///< 0x68
    std::array<s8, 4> ref_frame_sign_bias; ///< 0x6C
    u8 first_level;                        ///< 0x70
    u8 sharpness_level;                    ///< 0x71
    u8 base_q_index;                       ///< 0x72
    u8 y_dc_delta_q;                       ///< 0x73
    u8 uv_ac_delta_q;                      ///< 0x74
    u8 uv_dc_delta_q;                      ///< 0x75
    u8 lossless;                           ///< 0x76
    u8 tx_mode;                            ///< 0x77
    u8 allow_high_precision_mv;            ///< 0x78
    u8 interp_filter;                      ///< 0x79
    u8 reference_mode;                     ///< 0x7A
    s8 comp_fixed_ref;                     ///< 0x7B
    std::array<s8, 2> comp_var_ref;        ///< 0x7C
    u8 log2_tile_cols;                     ///< 0x7E
    u8 log2_tile_rows;                     ///< 0x7F
    Segmentation segmentation;             ///< 0x80
    LoopFilter loop_filter;                ///< 0xE4
    INSERT_PADDING_BYTES_NOINIT(5);        ///< 0xEB
    u32 surface_params;                    ///< 0xF0
    INSERT_PADDING_WORDS_NOINIT(3);        ///< 0xF4

    [[nodiscard]] Vp9PictureInfo Convert() const {
        return {
            .is_key_frame = (vp9_flags & FrameFlags::IsKeyFrame) != 0,
            .intra_only = (vp9_flags & FrameFlags::IntraOnly) != 0,
            .last_frame_was_key = (vp9_flags & FrameFlags::LastFrameIsKeyFrame) != 0,
            .frame_size_changed = (vp9_flags & FrameFlags::FrameSizeChanged) != 0,
            .error_resilient_mode = (vp9_flags & FrameFlags::ErrorResilientMode) != 0,
            .last_frame_shown = (vp9_flags & FrameFlags::LastShowFrame) != 0,
            .show_frame = false,
            .ref_frame_sign_bias = ref_frame_sign_bias,
            .base_q_index = base_q_index,
            .y_dc_delta_q = y_dc_delta_q,
            .uv_dc_delta_q = uv_dc_delta_q,
            .uv_ac_delta_q = uv_ac_delta_q,
            .lossless = lossless != 0,
            .transform_mode = tx_mode,
            .allow_high_precision_mv = allow_high_precision_mv != 0,
            .interp_filter = interp_filter,
            .reference_mode = reference_mode,
            .comp_fixed_ref = comp_fixed_ref,
            .comp_var_ref = comp_var_ref,
            .log2_tile_cols = log2_tile_cols,
            .log2_tile_rows = log2_tile_rows,
            .segment_enabled = segmentation.enabled != 0,
            .segment_map_update = segmentation.update_map != 0,
            .segment_map_temporal_update = segmentation.temporal_update != 0,
            .segment_abs_delta = segmentation.abs_delta,
            .segment_feature_enable = segmentation.feature_mask,
            .segment_feature_data = segmentation.feature_data,
            .mode_ref_delta_enabled = loop_filter.mode_ref_delta_enabled != 0,
            .use_prev_in_find_mv_refs = !(vp9_flags == (FrameFlags::ErrorResilientMode)) &&
                                        !(vp9_flags == (FrameFlags::FrameSizeChanged)) &&
                                        !(vp9_flags == (FrameFlags::IntraOnly)) &&
                                        (vp9_flags == (FrameFlags::LastShowFrame)) &&
                                        !(vp9_flags == (FrameFlags::LastFrameIsKeyFrame)),
            .ref_deltas = loop_filter.ref_deltas,
            .mode_deltas = loop_filter.mode_deltas,
            .entropy{},
            .frame_size = current_frame_size,
            .first_level = first_level,
            .sharpness_level = sharpness_level,
            .bitstream_size = bitstream_size,
            .frame_offsets{},
            .refresh_frame{},
        };
    }
};
static_assert(sizeof(PictureInfo) == 0x100, "PictureInfo is an invalid size");

struct EntropyProbs {
    INSERT_PADDING_BYTES_NOINIT(1024);                 ///< 0x0000
    std::array<u8, 28> inter_mode_prob;                ///< 0x0400
    std::array<u8, 4> intra_inter_prob;                ///< 0x041C
    INSERT_PADDING_BYTES_NOINIT(80);                   ///< 0x0420
    std::array<u8, 2> tx_8x8_prob;                     ///< 0x0470
    std::array<u8, 4> tx_16x16_prob;                   ///< 0x0472
    std::array<u8, 6> tx_32x32_prob;                   ///< 0x0476
    std::array<u8, 4> y_mode_prob_e8;                  ///< 0x047C
    std::array<std::array<u8, 8>, 4> y_mode_prob_e0e7; ///< 0x0480
    INSERT_PADDING_BYTES_NOINIT(64);                   ///< 0x04A0
    std::array<u8, 64> partition_prob;                 ///< 0x04E0
    INSERT_PADDING_BYTES_NOINIT(10);                   ///< 0x0520
    std::array<u8, 8> switchable_interp_prob;          ///< 0x052A
    std::array<u8, 5> comp_inter_prob;                 ///< 0x0532
    std::array<u8, 3> skip_probs;                      ///< 0x0537
    INSERT_PADDING_BYTES_NOINIT(1);                    ///< 0x053A
    std::array<u8, 3> joints;                          ///< 0x053B
    std::array<u8, 2> sign;                            ///< 0x053E
    std::array<u8, 2> class_0;                         ///< 0x0540
    std::array<u8, 6> fr;                              ///< 0x0542
    std::array<u8, 2> class_0_hp;                      ///< 0x0548
    std::array<u8, 2> high_precision;                  ///< 0x054A
    std::array<u8, 20> classes;                        ///< 0x054C
    std::array<u8, 12> class_0_fr;                     ///< 0x0560
    std::array<u8, 20> pred_bits;                      ///< 0x056C
    std::array<u8, 10> single_ref_prob;                ///< 0x0580
    std::array<u8, 5> comp_ref_prob;                   ///< 0x058A
    INSERT_PADDING_BYTES_NOINIT(17);                   ///< 0x058F
    std::array<u8, 2304> coef_probs;                   ///< 0x05A0

    void Convert(Vp9EntropyProbs& fc) {
        fc.inter_mode_prob = inter_mode_prob;
        fc.intra_inter_prob = intra_inter_prob;
        fc.tx_8x8_prob = tx_8x8_prob;
        fc.tx_16x16_prob = tx_16x16_prob;
        fc.tx_32x32_prob = tx_32x32_prob;

        for (std::size_t i = 0; i < 4; i++) {
            for (std::size_t j = 0; j < 9; j++) {
                fc.y_mode_prob[j + 9 * i] = j < 8 ? y_mode_prob_e0e7[i][j] : y_mode_prob_e8[i];
            }
        }

        fc.partition_prob = partition_prob;
        fc.switchable_interp_prob = switchable_interp_prob;
        fc.comp_inter_prob = comp_inter_prob;
        fc.skip_probs = skip_probs;
        fc.joints = joints;
        fc.sign = sign;
        fc.class_0 = class_0;
        fc.fr = fr;
        fc.class_0_hp = class_0_hp;
        fc.high_precision = high_precision;
        fc.classes = classes;
        fc.class_0_fr = class_0_fr;
        fc.prob_bits = pred_bits;
        fc.single_ref_prob = single_ref_prob;
        fc.comp_ref_prob = comp_ref_prob;

        // Skip the 4th element as it goes unused
        for (std::size_t i = 0; i < coef_probs.size(); i += 4) {
            const std::size_t j = i - i / 4;
            fc.coef_probs[j] = coef_probs[i];
            fc.coef_probs[j + 1] = coef_probs[i + 1];
            fc.coef_probs[j + 2] = coef_probs[i + 2];
        }
    }
};
static_assert(sizeof(EntropyProbs) == 0xEA0, "EntropyProbs is an invalid size");

enum class Ref { Last, Golden, AltRef };

struct RefPoolElement {
    s64 frame{};
    Ref ref{};
    bool refresh{};
};

struct FrameContexts {
    s64 from;
    bool adapted;
    Vp9EntropyProbs probs;
};

#define ASSERT_POSITION(field_name, position)                                                      \
    static_assert(offsetof(Vp9EntropyProbs, field_name) == position,                               \
                  "Field " #field_name " has invalid position")

ASSERT_POSITION(partition_prob, 0x0024);
ASSERT_POSITION(switchable_interp_prob, 0x0724);
ASSERT_POSITION(sign, 0x0772);
ASSERT_POSITION(class_0_fr, 0x079E);
ASSERT_POSITION(high_precision, 0x07B2);
#undef ASSERT_POSITION

#define ASSERT_POSITION(field_name, position)                                                      \
    static_assert(offsetof(PictureInfo, field_name) == position,                                   \
                  "Field " #field_name " has invalid position")

ASSERT_POSITION(bitstream_size, 0x30);
ASSERT_POSITION(last_frame_size, 0x48);
ASSERT_POSITION(first_level, 0x70);
ASSERT_POSITION(segmentation, 0x80);
ASSERT_POSITION(loop_filter, 0xE4);
ASSERT_POSITION(surface_params, 0xF0);
#undef ASSERT_POSITION

#define ASSERT_POSITION(field_name, position)                                                      \
    static_assert(offsetof(EntropyProbs, field_name) == position,                                  \
                  "Field " #field_name " has invalid position")

ASSERT_POSITION(inter_mode_prob, 0x400);
ASSERT_POSITION(tx_8x8_prob, 0x470);
ASSERT_POSITION(partition_prob, 0x4E0);
ASSERT_POSITION(class_0, 0x540);
ASSERT_POSITION(class_0_fr, 0x560);
ASSERT_POSITION(coef_probs, 0x5A0);
#undef ASSERT_POSITION

}; // namespace Decoder
}; // namespace Tegra
