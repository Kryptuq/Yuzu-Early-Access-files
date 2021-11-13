// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string_view>
#include <queue>
#include "common/common_types.h"
#include "video_core/command_classes/nvdec_common.h"

extern "C" {
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <libavcodec/avcodec.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

namespace Tegra {
class GPU;

void AVFrameDeleter(AVFrame* ptr);
using AVFramePtr = std::unique_ptr<AVFrame, decltype(&AVFrameDeleter)>;

namespace Decoder {
class H264;
class VP8;
class VP9;
} // namespace Decoder

class Codec {
public:
    explicit Codec(GPU& gpu, const NvdecCommon::NvdecRegisters& regs);
    ~Codec();

    /// Initialize the codec, returning success or failure
    void Initialize();

    /// Sets NVDEC video stream codec
    void SetTargetCodec(NvdecCommon::VideoCodec codec);

    /// Call decoders to construct headers, decode AVFrame with ffmpeg
    void Decode();

    /// Returns next decoded frame
    [[nodiscard]] AVFramePtr GetCurrentFrame();

    /// Returns the value of current_codec
    [[nodiscard]] NvdecCommon::VideoCodec GetCurrentCodec() const;

    /// Return name of the current codec
    [[nodiscard]] std::string_view GetCurrentCodecName() const;

private:
    void InitializeAvCodecContext();

    void InitializeGpuDecoder();

    bool CreateGpuAvDevice();

    bool initialized{};
    NvdecCommon::VideoCodec current_codec{NvdecCommon::VideoCodec::None};

    AVCodec* av_codec{nullptr};
    AVCodecContext* av_codec_ctx{nullptr};
    AVBufferRef* av_gpu_decoder{nullptr};

    GPU& gpu;
    const NvdecCommon::NvdecRegisters& state;
    std::unique_ptr<Decoder::H264> h264_decoder;
    std::unique_ptr<Decoder::VP8> vp8_decoder;
    std::unique_ptr<Decoder::VP9> vp9_decoder;

    std::queue<AVFramePtr> av_frames{};
};

} // namespace Tegra
