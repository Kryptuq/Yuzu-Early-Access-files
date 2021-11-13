// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/command_classes/nvdec.h"
#include "video_core/gpu.h"

namespace Tegra {

#define NVDEC_REG_INDEX(field_name)                                                                \
    (offsetof(NvdecCommon::NvdecRegisters, field_name) / sizeof(u64))

Nvdec::Nvdec(GPU& gpu_) : gpu(gpu_), state{}, codec(std::make_unique<Codec>(gpu, state)) {}

Nvdec::~Nvdec() = default;

void Nvdec::ProcessMethod(u32 method, u32 argument) {
    state.reg_array[method] = static_cast<u64>(argument) << 8;

    switch (method) {
    case NVDEC_REG_INDEX(set_codec_id):
        codec->SetTargetCodec(static_cast<NvdecCommon::VideoCodec>(argument));
        break;
    case NVDEC_REG_INDEX(execute):
        Execute();
        break;
    }
}

AVFramePtr Nvdec::GetFrame() {
    return codec->GetCurrentFrame();
}

void Nvdec::Execute() {
    switch (codec->GetCurrentCodec()) {
    case NvdecCommon::VideoCodec::H264:
    case NvdecCommon::VideoCodec::VP8:
    case NvdecCommon::VideoCodec::VP9:
        codec->Decode();
        break;
    default:
        UNIMPLEMENTED_MSG("Codec {}", codec->GetCurrentCodecName());
        break;
    }
}

} // namespace Tegra
