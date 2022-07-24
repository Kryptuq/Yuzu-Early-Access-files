// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>

#include "video_core/command_classes/codecs/vp8.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

namespace Tegra::Decoder {
VP8::VP8(GPU& gpu_) : gpu(gpu_) {}

VP8::~VP8() = default;

const std::vector<u8>& VP8::ComposeFrame(const NvdecCommon::NvdecRegisters& state) {
    VP8PictureInfo info;
    gpu.MemoryManager().ReadBlock(state.picture_info_offset, &info, sizeof(VP8PictureInfo));

    const bool is_key_frame = info.key_frame == 1u;
    const auto bitstream_size = static_cast<size_t>(info.vld_buffer_size);
    const size_t header_size = is_key_frame ? 10u : 3u;
    frame.resize(header_size + bitstream_size);

    // Based on page 30 of the VP8 specification.
    // https://datatracker.ietf.org/doc/rfc6386/
    frame[0] = is_key_frame ? 0u : 1u; // 1-bit frame type (0: keyframe, 1: interframes).
    frame[0] |= static_cast<u8>((info.version & 7u) << 1u); // 3-bit version number
    frame[0] |= static_cast<u8>(1u << 4u);                  // 1-bit show_frame flag

    // The next 19-bits are the first partition size
    frame[0] |= static_cast<u8>((info.first_part_size & 7u) << 5u);
    frame[1] = static_cast<u8>((info.first_part_size & 0x7f8u) >> 3u);
    frame[2] = static_cast<u8>((info.first_part_size & 0x7f800u) >> 11u);

    if (is_key_frame) {
        frame[3] = 0x9du;
        frame[4] = 0x01u;
        frame[5] = 0x2au;
        // TODO(ameerj): Horizontal/Vertical Scale
        // 16 bits: (2 bits Horizontal Scale << 14) | Width (14 bits)
        frame[6] = static_cast<u8>(info.frame_width & 0xff);
        frame[7] = static_cast<u8>(((info.frame_width >> 8) & 0x3f));
        // 16 bits:(2 bits Vertical Scale << 14) | Height (14 bits)
        frame[8] = static_cast<u8>(info.frame_height & 0xff);
        frame[9] = static_cast<u8>(((info.frame_height >> 8) & 0x3f));
    }
    const u64 bitstream_offset = state.frame_bitstream_offset;
    gpu.MemoryManager().ReadBlock(bitstream_offset, frame.data() + header_size, bitstream_size);

    return frame;
}

} // namespace Tegra::Decoder
