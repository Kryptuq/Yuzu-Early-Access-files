// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/command_classes/nvdec.h"
#include "video_core/gpu.h"

namespace Tegra {

Nvdec::Nvdec(GPU& gpu_) : gpu(gpu_), codec(std::make_unique<Codec>(gpu)) {}

Nvdec::~Nvdec() = default;

void Nvdec::ProcessMethod(Method method, u32 argument) {
    if (method == Method::SetVideoCodec) {
        codec->StateWrite(static_cast<u32>(method), argument);
    } else {
        codec->StateWrite(static_cast<u32>(method), static_cast<u64>(argument) << 8);
    }

    switch (method) {
    case Method::SetVideoCodec:
        codec->SetTargetCodec(static_cast<NvdecCommon::VideoCodec>(argument));
        break;
    case Method::Execute:
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
    case NvdecCommon::VideoCodec::Vp9:
        codec->Decode();
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown codec {}", static_cast<u32>(codec->GetCurrentCodec()));
        break;
    }
}

} // namespace Tegra
