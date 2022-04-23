// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"

struct SwsContext;

namespace Tegra {
class GPU;
class Nvdec;
union VicConfig;

class Vic {
public:
    enum class Method : u32 {
        Execute = 0xc0,
        SetControlParams = 0x1c1,
        SetConfigStructOffset = 0x1c2,
        SetOutputSurfaceLumaOffset = 0x1c8,
        SetOutputSurfaceChromaOffset = 0x1c9,
        SetOutputSurfaceChromaUnusedOffset = 0x1ca
    };

    explicit Vic(GPU& gpu, std::shared_ptr<Nvdec> nvdec_processor);

    ~Vic();

    /// Write to the device state.
    void ProcessMethod(Method method, u32 argument);

private:
    void Execute();

    void WriteRGBFrame(const AVFrame* frame, const VicConfig& config);

    void WriteYUVFrame(const AVFrame* frame, const VicConfig& config);

    GPU& gpu;
    std::shared_ptr<Tegra::Nvdec> nvdec_processor;

    /// Avoid reallocation of the following buffers every frame, as their
    /// size does not change during a stream
    using AVMallocPtr = std::unique_ptr<u8, decltype(&av_free)>;
    AVMallocPtr converted_frame_buffer;
    std::vector<u8> luma_buffer;
    std::vector<u8> chroma_buffer;

    GPUVAddr config_struct_address{};
    GPUVAddr output_surface_luma_address{};
    GPUVAddr output_surface_chroma_address{};

    SwsContext* scaler_ctx{};
    s32 scaler_width{};
    s32 scaler_height{};
};

} // namespace Tegra
