// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Tegra::NvdecCommon {

struct NvdecRegisters {
    INSERT_PADDING_WORDS(256);
    u64 set_codec_id{};
    INSERT_PADDING_WORDS(254);
    u64 set_platform_id{};
    u64 picture_info_offset{};
    u64 frame_bitstream_offset{};
    u64 frame_number{};
    u64 h264_slice_data_offsets{};
    u64 h264_mv_dump_offset{};
    INSERT_PADDING_WORDS(6);
    u64 frame_stats_offset{};
    u64 h264_last_surface_luma_offset{};
    u64 h264_last_surface_chroma_offset{};
    std::array<u64, 17> surface_luma_offset{};
    std::array<u64, 17> surface_chroma_offset{};
    INSERT_PADDING_WORDS(132);
    u64 vp9_entropy_probs_offset{};
    u64 vp9_backward_updates_offset{};
    u64 vp9_last_frame_segmap_offset{};
    u64 vp9_curr_frame_segmap_offset{};
    INSERT_PADDING_WORDS(2);
    u64 vp9_last_frame_mvs_offset{};
    u64 vp9_curr_frame_mvs_offset{};
    INSERT_PADDING_WORDS(2);
};
static_assert(sizeof(NvdecRegisters) == (0xBC0), "NvdecRegisters is incorrect size");

enum class VideoCodec : u32 {
    None = 0x0,
    H264 = 0x3,
    Vp8 = 0x5,
    H265 = 0x7,
    Vp9 = 0x9,
};

} // namespace Tegra::NvdecCommon
