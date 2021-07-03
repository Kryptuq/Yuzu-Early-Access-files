// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <fstream>
#include <vector>
#include "common/assert.h"
#include "video_core/command_classes/codecs/codec.h"
#include "video_core/command_classes/codecs/h264.h"
#include "video_core/command_classes/codecs/vp9.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

extern "C" {
#include <libavutil/opt.h>
}

namespace Tegra {

void AVFrameDeleter(AVFrame* ptr) {
    av_frame_unref(ptr);
    av_free(ptr);
}

Codec::Codec(GPU& gpu_, const NvdecCommon::NvdecRegisters& regs)
    : gpu(gpu_), state{regs}, h264_decoder(std::make_unique<Decoder::H264>(gpu)),
      vp9_decoder(std::make_unique<Decoder::VP9>(gpu)) {}

Codec::~Codec() {
    if (!initialized) {
        return;
    }
    // Free libav memory
    AVFrame* av_frame{nullptr};
    avcodec_send_packet(av_codec_ctx, nullptr);
    av_frame = av_frame_alloc();
    avcodec_receive_frame(av_codec_ctx, av_frame);
    avcodec_flush_buffers(av_codec_ctx);

    av_frame_unref(av_frame);
    av_free(av_frame);
    avcodec_close(av_codec_ctx);
}

void Codec::Initialize() {
    AVCodecID codec{AV_CODEC_ID_NONE};
    switch (current_codec) {
    case NvdecCommon::VideoCodec::H264:
        codec = AV_CODEC_ID_H264;
        break;
    case NvdecCommon::VideoCodec::Vp9:
        codec = AV_CODEC_ID_VP9;
        break;
    default:
        return;
    }
    av_codec = avcodec_find_decoder(codec);
    av_codec_ctx = avcodec_alloc_context3(av_codec);
    av_opt_set(av_codec_ctx->priv_data, "tune", "zerolatency", 0);

    // TODO(ameerj): libavcodec gpu hw acceleration

    const auto av_error = avcodec_open2(av_codec_ctx, av_codec, nullptr);
    if (av_error < 0) {
        LOG_ERROR(Service_NVDRV, "avcodec_open2() Failed.");
        avcodec_close(av_codec_ctx);
        return;
    }
    initialized = true;
    return;
}

void Codec::SetTargetCodec(NvdecCommon::VideoCodec codec) {
    if (current_codec != codec) {
        current_codec = codec;
        LOG_INFO(Service_NVDRV, "NVDEC video codec initialized to {}", GetCurrentCodecName());
    }
}

void Codec::Decode() {
    const bool is_first_frame = !initialized;
    if (!initialized) {
        Initialize();
    }

    bool vp9_hidden_frame = false;
    AVPacket packet{};
    av_init_packet(&packet);
    std::vector<u8> frame_data;

    if (current_codec == NvdecCommon::VideoCodec::H264) {
        frame_data = h264_decoder->ComposeFrameHeader(state, is_first_frame);
    } else if (current_codec == NvdecCommon::VideoCodec::Vp9) {
        frame_data = vp9_decoder->ComposeFrameHeader(state);
        vp9_hidden_frame = vp9_decoder->WasFrameHidden();
    }

    packet.data = frame_data.data();
    packet.size = static_cast<s32>(frame_data.size());

    avcodec_send_packet(av_codec_ctx, &packet);

    if (!vp9_hidden_frame) {
        // Only receive/store visible frames
        AVFramePtr frame = AVFramePtr{av_frame_alloc(), AVFrameDeleter};
        avcodec_receive_frame(av_codec_ctx, frame.get());
        av_frames.push(std::move(frame));
        // Limit queue to 10 frames. Workaround for ZLA decode and queue spam
        if (av_frames.size() > 10) {
            av_frames.pop();
        }
    }
}

AVFramePtr Codec::GetCurrentFrame() {
    // Sometimes VIC will request more frames than have been decoded.
    // in this case, return a nullptr and don't overwrite previous frame data
    if (av_frames.empty()) {
        return AVFramePtr{nullptr, AVFrameDeleter};
    }

    AVFramePtr frame = std::move(av_frames.front());
    av_frames.pop();
    return frame;
}

NvdecCommon::VideoCodec Codec::GetCurrentCodec() const {
    return current_codec;
}

std::string_view Codec::GetCurrentCodecName() const {
    switch (current_codec) {
    case NvdecCommon::VideoCodec::None:
        return "None";
    case NvdecCommon::VideoCodec::H264:
        return "H264";
    case NvdecCommon::VideoCodec::Vp8:
        return "VP8";
    case NvdecCommon::VideoCodec::H265:
        return "H265";
    case NvdecCommon::VideoCodec::Vp9:
        return "VP9";
    default:
        return "Unknown";
    }
};

} // namespace Tegra
