// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
#if defined(LIBVA_FOUND)
// Hardware acceleration code from FFmpeg/doc/examples/hw_decode.c originally under MIT license
namespace {
constexpr std::array<const char*, 2> VAAPI_DRIVERS = {
    "i915",
    "amdgpu",
};

AVPixelFormat GetHwFormat(AVCodecContext*, const AVPixelFormat* pix_fmts) {
    for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == AV_PIX_FMT_VAAPI) {
            return AV_PIX_FMT_VAAPI;
        }
    }
    LOG_INFO(Service_NVDRV, "Could not find compatible GPU AV format, falling back to CPU");
    return *pix_fmts;
}

bool CreateVaapiHwdevice(AVBufferRef** av_hw_device) {
    AVDictionary* hwdevice_options = nullptr;
    av_dict_set(&hwdevice_options, "connection_type", "drm", 0);
    for (const auto& driver : VAAPI_DRIVERS) {
        av_dict_set(&hwdevice_options, "kernel_driver", driver, 0);
        const int hwdevice_error = av_hwdevice_ctx_create(av_hw_device, AV_HWDEVICE_TYPE_VAAPI,
                                                          nullptr, hwdevice_options, 0);
        if (hwdevice_error >= 0) {
            LOG_INFO(Service_NVDRV, "Using VA-API with {}", driver);
            av_dict_free(&hwdevice_options);
            return true;
        }
        LOG_DEBUG(Service_NVDRV, "VA-API av_hwdevice_ctx_create failed {}", hwdevice_error);
    }
    LOG_DEBUG(Service_NVDRV, "VA-API av_hwdevice_ctx_create failed for all drivers");
    av_dict_free(&hwdevice_options);
    return false;
}
} // namespace
#endif

void AVFrameDeleter(AVFrame* ptr) {
    av_frame_free(&ptr);
}

Codec::Codec(GPU& gpu_, const NvdecCommon::NvdecRegisters& regs)
    : gpu(gpu_), state{regs}, h264_decoder(std::make_unique<Decoder::H264>(gpu)),
      vp9_decoder(std::make_unique<Decoder::VP9>(gpu)) {}

Codec::~Codec() {
    if (!initialized) {
        return;
    }
    // Free libav memory
    avcodec_send_packet(av_codec_ctx, nullptr);
    AVFrame* av_frame = av_frame_alloc();
    avcodec_receive_frame(av_codec_ctx, av_frame);
    avcodec_flush_buffers(av_codec_ctx);
    av_frame_free(&av_frame);
    avcodec_close(av_codec_ctx);
    av_buffer_unref(&av_hw_device);
}

void Codec::InitializeHwdec() {
    // Prioritize integrated GPU to mitigate bandwidth bottlenecks
#if defined(LIBVA_FOUND)
    if (CreateVaapiHwdevice(&av_hw_device)) {
        const auto hw_device_ctx = av_buffer_ref(av_hw_device);
        ASSERT_MSG(hw_device_ctx, "av_buffer_ref failed");
        av_codec_ctx->hw_device_ctx = hw_device_ctx;
        av_codec_ctx->get_format = GetHwFormat;
        return;
    }
#endif
    // TODO more GPU accelerated decoders
}

void Codec::Initialize() {
    AVCodecID codec;
    switch (current_codec) {
    case NvdecCommon::VideoCodec::H264:
        codec = AV_CODEC_ID_H264;
        break;
    case NvdecCommon::VideoCodec::Vp9:
        codec = AV_CODEC_ID_VP9;
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown codec {}", current_codec);
        return;
    }
    av_codec = avcodec_find_decoder(codec);
    av_codec_ctx = avcodec_alloc_context3(av_codec);
    av_opt_set(av_codec_ctx->priv_data, "tune", "zerolatency", 0);
    InitializeHwdec();
    if (!av_codec_ctx->hw_device_ctx) {
        LOG_INFO(Service_NVDRV, "Using FFmpeg software decoding");
    }
    const auto av_error = avcodec_open2(av_codec_ctx, av_codec, nullptr);
    if (av_error < 0) {
        LOG_ERROR(Service_NVDRV, "avcodec_open2() Failed.");
        avcodec_close(av_codec_ctx);
        av_buffer_unref(&av_hw_device);
        return;
    }
    initialized = true;
}

void Codec::SetTargetCodec(NvdecCommon::VideoCodec codec) {
    if (current_codec != codec) {
        current_codec = codec;
        LOG_INFO(Service_NVDRV, "NVDEC video codec initialized to {}", GetCurrentCodecName());
    }
}

void Codec::Decode() {
    const bool is_first_frame = !initialized;
    if (is_first_frame) {
        Initialize();
    }
    bool vp9_hidden_frame = false;
    std::vector<u8> frame_data;
    if (current_codec == NvdecCommon::VideoCodec::H264) {
        frame_data = h264_decoder->ComposeFrameHeader(state, is_first_frame);
    } else if (current_codec == NvdecCommon::VideoCodec::Vp9) {
        frame_data = vp9_decoder->ComposeFrameHeader(state);
        vp9_hidden_frame = vp9_decoder->WasFrameHidden();
    }
    AVPacket packet{};
    av_init_packet(&packet);
    packet.data = frame_data.data();
    packet.size = static_cast<s32>(frame_data.size());
    if (const int ret = avcodec_send_packet(av_codec_ctx, &packet); ret) {
        LOG_DEBUG(Service_NVDRV, "avcodec_send_packet error {}", ret);
        return;
    }
    // Only receive/store visible frames
    if (vp9_hidden_frame) {
        return;
    }
    AVFrame* hw_frame = av_frame_alloc();
    AVFrame* sw_frame = hw_frame;
    ASSERT_MSG(hw_frame, "av_frame_alloc hw_frame failed");
    if (const int ret = avcodec_receive_frame(av_codec_ctx, hw_frame); ret) {
        LOG_DEBUG(Service_NVDRV, "avcodec_receive_frame error {}", ret);
        av_frame_free(&hw_frame);
        return;
    }
    if (!hw_frame->width || !hw_frame->height) {
        LOG_WARNING(Service_NVDRV, "Zero width or height in frame");
        av_frame_free(&hw_frame);
        return;
    }
#if defined(LIBVA_FOUND)
    // Hardware acceleration code from FFmpeg/doc/examples/hw_decode.c under MIT license
    if (hw_frame->format == AV_PIX_FMT_VAAPI) {
        sw_frame = av_frame_alloc();
        ASSERT_MSG(sw_frame, "av_frame_alloc sw_frame failed");
        // Can't use AV_PIX_FMT_YUV420P and share code with software decoding in vic.cpp
        // because Intel drivers crash unless using AV_PIX_FMT_NV12
        sw_frame->format = AV_PIX_FMT_NV12;
        const int transfer_data_ret = av_hwframe_transfer_data(sw_frame, hw_frame, 0);
        ASSERT_MSG(!transfer_data_ret, "av_hwframe_transfer_data error {}", transfer_data_ret);
        av_frame_free(&hw_frame);
    }
#endif
    if (sw_frame->format != AV_PIX_FMT_YUV420P && sw_frame->format != AV_PIX_FMT_NV12) {
        UNIMPLEMENTED_MSG("Unexpected video format from host graphics: {}", sw_frame->format);
        av_frame_free(&sw_frame);
        return;
    }
    av_frames.push(AVFramePtr{sw_frame, AVFrameDeleter});
    if (av_frames.size() > 10) {
        LOG_TRACE(Service_NVDRV, "av_frames.push overflow dropped frame");
        av_frames.pop();
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
}
} // namespace Tegra
