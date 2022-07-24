// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "common/common_types.h"
#include "common/stream.h"
#include "video_core/command_classes/codecs/vp9_types.h"
#include "video_core/command_classes/nvdec_common.h"

namespace Tegra {
class GPU;
namespace Decoder {

/// The VpxRangeEncoder, and VpxBitStreamWriter classes are used to compose the
/// VP9 header bitstreams.

class VpxRangeEncoder {
public:
    VpxRangeEncoder();
    ~VpxRangeEncoder();

    VpxRangeEncoder(const VpxRangeEncoder&) = delete;
    VpxRangeEncoder& operator=(const VpxRangeEncoder&) = delete;

    VpxRangeEncoder(VpxRangeEncoder&&) = default;
    VpxRangeEncoder& operator=(VpxRangeEncoder&&) = default;

    /// Writes the rightmost value_size bits from value into the stream
    void Write(s32 value, s32 value_size);

    /// Writes a single bit with half probability
    void Write(bool bit);

    /// Writes a bit to the base_stream encoded with probability
    void Write(bool bit, s32 probability);

    /// Signal the end of the bitstream
    void End();

    [[nodiscard]] std::vector<u8>& GetBuffer() {
        return base_stream.GetBuffer();
    }

    [[nodiscard]] const std::vector<u8>& GetBuffer() const {
        return base_stream.GetBuffer();
    }

private:
    u8 PeekByte();
    Common::Stream base_stream{};
    u32 low_value{};
    u32 range{0xff};
    s32 count{-24};
    s32 half_probability{128};
};

class VpxBitStreamWriter {
public:
    VpxBitStreamWriter();
    ~VpxBitStreamWriter();

    VpxBitStreamWriter(const VpxBitStreamWriter&) = delete;
    VpxBitStreamWriter& operator=(const VpxBitStreamWriter&) = delete;

    VpxBitStreamWriter(VpxBitStreamWriter&&) = default;
    VpxBitStreamWriter& operator=(VpxBitStreamWriter&&) = default;

    /// Write an unsigned integer value
    void WriteU(u32 value, u32 value_size);

    /// Write a signed integer value
    void WriteS(s32 value, u32 value_size);

    /// Based on 6.2.10 of VP9 Spec, writes a delta coded value
    void WriteDeltaQ(u32 value);

    /// Write a single bit.
    void WriteBit(bool state);

    /// Pushes current buffer into buffer_array, resets buffer
    void Flush();

    /// Returns byte_array
    [[nodiscard]] std::vector<u8>& GetByteArray();

    /// Returns const byte_array
    [[nodiscard]] const std::vector<u8>& GetByteArray() const;

private:
    /// Write bit_count bits from value into buffer
    void WriteBits(u32 value, u32 bit_count);

    /// Gets next available position in buffer, invokes Flush() if buffer is full
    s32 GetFreeBufferBits();

    s32 buffer_size{8};

    s32 buffer{};
    s32 buffer_pos{};
    std::vector<u8> byte_array;
};

class VP9 {
public:
    explicit VP9(GPU& gpu_);
    ~VP9();

    VP9(const VP9&) = delete;
    VP9& operator=(const VP9&) = delete;

    VP9(VP9&&) = default;
    VP9& operator=(VP9&&) = delete;

    /// Composes the VP9 frame from the GPU state information.
    /// Based on the official VP9 spec documentation
    void ComposeFrame(const NvdecCommon::NvdecRegisters& state);

    /// Returns true if the most recent frame was a hidden frame.
    [[nodiscard]] bool WasFrameHidden() const {
        return !current_frame_info.show_frame;
    }

    /// Returns a const reference to the composed frame data.
    [[nodiscard]] const std::vector<u8>& GetFrameBytes() const {
        return frame;
    }

private:
    /// Generates compressed header probability updates in the bitstream writer
    template <typename T, std::size_t N>
    void WriteProbabilityUpdate(VpxRangeEncoder& writer, const std::array<T, N>& new_prob,
                                const std::array<T, N>& old_prob);

    /// Generates compressed header probability updates in the bitstream writer
    /// If probs are not equal, WriteProbabilityDelta is invoked
    void WriteProbabilityUpdate(VpxRangeEncoder& writer, u8 new_prob, u8 old_prob);

    /// Generates compressed header probability deltas in the bitstream writer
    void WriteProbabilityDelta(VpxRangeEncoder& writer, u8 new_prob, u8 old_prob);

    /// Inverse of 6.3.4 Decode term subexp
    void EncodeTermSubExp(VpxRangeEncoder& writer, s32 value);

    /// Writes if the value is less than the test value
    bool WriteLessThan(VpxRangeEncoder& writer, s32 value, s32 test);

    /// Writes probability updates for the Coef probabilities
    void WriteCoefProbabilityUpdate(VpxRangeEncoder& writer, s32 tx_mode,
                                    const std::array<u8, 1728>& new_prob,
                                    const std::array<u8, 1728>& old_prob);

    /// Write probabilities for 4-byte aligned structures
    template <typename T, std::size_t N>
    void WriteProbabilityUpdateAligned4(VpxRangeEncoder& writer, const std::array<T, N>& new_prob,
                                        const std::array<T, N>& old_prob);

    /// Write motion vector probability updates. 6.3.17 in the spec
    void WriteMvProbabilityUpdate(VpxRangeEncoder& writer, u8 new_prob, u8 old_prob);

    /// Returns VP9 information from NVDEC provided offset and size
    [[nodiscard]] Vp9PictureInfo GetVp9PictureInfo(const NvdecCommon::NvdecRegisters& state);

    /// Read and convert NVDEC provided entropy probs to Vp9EntropyProbs struct
    void InsertEntropy(u64 offset, Vp9EntropyProbs& dst);

    /// Returns frame to be decoded after buffering
    [[nodiscard]] Vp9FrameContainer GetCurrentFrame(const NvdecCommon::NvdecRegisters& state);

    /// Use NVDEC providied information to compose the headers for the current frame
    [[nodiscard]] std::vector<u8> ComposeCompressedHeader();
    [[nodiscard]] VpxBitStreamWriter ComposeUncompressedHeader();

    GPU& gpu;
    std::vector<u8> frame;

    std::array<s8, 4> loop_filter_ref_deltas{};
    std::array<s8, 2> loop_filter_mode_deltas{};

    Vp9FrameContainer next_frame{};
    std::array<Vp9EntropyProbs, 4> frame_ctxs{};
    bool swap_ref_indices{};

    Vp9PictureInfo current_frame_info{};
    Vp9EntropyProbs prev_frame_probs{};
};

} // namespace Decoder
} // namespace Tegra
