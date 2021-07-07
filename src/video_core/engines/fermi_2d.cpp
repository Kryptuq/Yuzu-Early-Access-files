// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/surface.h"

using VideoCore::Surface::BytesPerBlock;
using VideoCore::Surface::PixelFormatFromRenderTargetFormat;

namespace Tegra::Engines {

Fermi2D::Fermi2D() {
    // Nvidia's OpenGL driver seems to assume these values
    regs.src.depth = 1;
    regs.dst.depth = 1;
}

Fermi2D::~Fermi2D() = default;

void Fermi2D::BindRasterizer(VideoCore::RasterizerInterface* rasterizer_) {
    rasterizer = rasterizer_;
}

void Fermi2D::CallMethod(u32 method, u32 method_argument, bool is_last_call) {
    ASSERT_MSG(method < Regs::NUM_REGS,
               "Invalid Fermi2D register, increase the size of the Regs structure");
    regs.reg_array[method] = method_argument;

    if (method == FERMI2D_REG_INDEX(pixels_from_memory.src_y0) + 1) {
        Blit();
    }
}

void Fermi2D::CallMultiMethod(u32 method, const u32* base_start, u32 amount, u32 methods_pending) {
    for (u32 i = 0; i < amount; ++i) {
        CallMethod(method, base_start[i], methods_pending - i <= 1);
    }
}

void Fermi2D::Blit() {
    LOG_DEBUG(HW_GPU, "called. source address=0x{:x}, destination address=0x{:x}",
              regs.src.Address(), regs.dst.Address());

    UNIMPLEMENTED_IF_MSG(regs.operation != Operation::SrcCopy, "Operation is not copy");
    UNIMPLEMENTED_IF_MSG(regs.src.layer != 0, "Source layer is not zero");
    UNIMPLEMENTED_IF_MSG(regs.dst.layer != 0, "Destination layer is not zero");
    UNIMPLEMENTED_IF_MSG(regs.src.depth != 1, "Source depth is not one");
    UNIMPLEMENTED_IF_MSG(regs.clip_enable != 0, "Clipped blit enabled");

    const auto& args = regs.pixels_from_memory;
    Config config{
        .operation = regs.operation,
        .filter = args.sample_mode.filter,
        .dst_x0 = args.dst_x0,
        .dst_y0 = args.dst_y0,
        .dst_x1 = args.dst_x0 + args.dst_width,
        .dst_y1 = args.dst_y0 + args.dst_height,
        .src_x0 = static_cast<s32>(args.src_x0 >> 32),
        .src_y0 = static_cast<s32>(args.src_y0 >> 32),
        .src_x1 = static_cast<s32>((args.du_dx * args.dst_width + args.src_x0) >> 32),
        .src_y1 = static_cast<s32>((args.dv_dy * args.dst_height + args.src_y0) >> 32),
    };
    Surface src = regs.src;
    const auto bytes_per_pixel = BytesPerBlock(PixelFormatFromRenderTargetFormat(src.format));
    const auto need_align_to_pitch =
        src.linear == Tegra::Engines::Fermi2D::MemoryLayout::Pitch &&
        static_cast<s32>(src.width) == config.src_x1 &&
        config.src_x1 > static_cast<s32>(src.pitch / bytes_per_pixel) && config.src_x0 > 0;
    if (need_align_to_pitch) {
        auto address = src.Address() + config.src_x0 * bytes_per_pixel;
        src.addr_upper = static_cast<u32>(address >> 32);
        src.addr_lower = static_cast<u32>(address);
        src.width -= config.src_x0;
        config.src_x1 -= config.src_x0;
        config.src_x0 = 0;
    }
    if (!rasterizer->AccelerateSurfaceCopy(src, regs.dst, config)) {
        UNIMPLEMENTED();
    }
}

} // namespace Tegra::Engines
