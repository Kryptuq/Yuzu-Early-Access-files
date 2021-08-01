// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"

struct SwsContext;

namespace Tegra {
class GPU;
class Nvdec;

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

    enum class VideoPixelFormat : u64_le {
        RGBA8 = 0x1f,
        BGRA8 = 0x20,
        Yuv420 = 0x44,
    };

    union VicConfig {
        u64_le raw{};
        BitField<0, 7, u64_le> pixel_format;
        BitField<7, 2, u64_le> chroma_loc_horiz;
        BitField<9, 2, u64_le> chroma_loc_vert;
        BitField<11, 4, u64_le> block_linear_kind;
        BitField<15, 4, u64_le> block_linear_height_log2;
        BitField<32, 14, u64_le> surface_width_minus1;
        BitField<46, 14, u64_le> surface_height_minus1;
    };

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
