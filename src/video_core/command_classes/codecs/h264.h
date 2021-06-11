// MIT License
//
// Copyright (c) Ryujinx Team and Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#pragma once

#include <vector>
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
    void WriteScalingList(const std::vector<u8>& list, s32 start, s32 count);

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

    /// Compose the H264 header of the frame for FFmpeg decoding
    [[nodiscard]] const std::vector<u8>& ComposeFrameHeader(
        const NvdecCommon::NvdecRegisters& state, bool is_first_frame = false);

private:
    struct H264ParameterSet {
        u32 log2_max_pic_order_cnt{};
        u32 delta_pic_order_always_zero_flag{};
        u32 frame_mbs_only_flag{};
        u32 pic_width_in_mbs{};
        u32 pic_height_in_map_units{};
        INSERT_PADDING_WORDS(1);
        u32 entropy_coding_mode_flag{};
        u32 bottom_field_pic_order_flag{};
        u32 num_refidx_l0_default_active{};
        u32 num_refidx_l1_default_active{};
        u32 deblocking_filter_control_flag{};
        u32 redundant_pic_count_flag{};
        u32 transform_8x8_mode_flag{};
        INSERT_PADDING_WORDS(9);
        u64 flags{};
        u32 frame_number{};
        u32 frame_number2{};
    };
    static_assert(sizeof(H264ParameterSet) == 0x68, "H264ParameterSet is an invalid size");

    struct H264DecoderContext {
        INSERT_PADDING_BYTES(0x48);
        u32 frame_data_size{};
        INSERT_PADDING_BYTES(0xc);
        H264ParameterSet h264_parameter_set{};
        INSERT_PADDING_BYTES(0x100);
        std::array<u8, 0x60> scaling_matrix_4;
        std::array<u8, 0x80> scaling_matrix_8;
    };
    static_assert(sizeof(H264DecoderContext) == 0x2a0, "H264DecoderContext is an invalid size");

    std::vector<u8> frame;
    GPU& gpu;
};

} // namespace Decoder
} // namespace Tegra
