// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "common/algorithm.h"
#include "common/assert.h"
#include "video_core/engines/engine_upload.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/textures/decoders.h"

namespace Tegra::Engines::Upload {

State::State(MemoryManager& memory_manager_, Registers& regs_)
    : regs{regs_}, memory_manager{memory_manager_} {}

State::~State() = default;

void State::BindRasterizer(VideoCore::RasterizerInterface* rasterizer_) {
    rasterizer = rasterizer_;
}

void State::ProcessExec(const bool is_linear_) {
    write_offset = 0;
    copy_size = regs.line_length_in * regs.line_count;
    inner_buffer.resize(copy_size);
    is_linear = is_linear_;
}

void State::ProcessData(const u32 data, const bool is_last_call) {
    const u32 sub_copy_size = std::min(4U, copy_size - write_offset);
    std::memcpy(&inner_buffer[write_offset], &data, sub_copy_size);
    write_offset += sub_copy_size;
    if (!is_last_call) {
        return;
    }
    ProcessData(inner_buffer);
}

void State::ProcessData(const u32* data, size_t num_data) {
    std::span<const u8> read_buffer(reinterpret_cast<const u8*>(data), num_data * sizeof(u32));
    ProcessData(read_buffer);
}

void State::ProcessData(std::span<const u8> read_buffer) {
    const GPUVAddr address{regs.dest.Address()};
    if (is_linear) {
        if (regs.line_count == 1) {
            rasterizer->AccelerateInlineToMemory(address, copy_size, read_buffer);
        } else {
            for (u32 line = 0; line < regs.line_count; ++line) {
                const GPUVAddr dest_line = address + static_cast<size_t>(line) * regs.dest.pitch;
                std::span<const u8> buffer(read_buffer.data() +
                                               static_cast<size_t>(line) * regs.line_length_in,
                                           regs.line_length_in);
                rasterizer->AccelerateInlineToMemory(dest_line, regs.line_length_in, buffer);
            }
        }
    } else {
        u32 width = regs.dest.width;
        u32 x_elements = regs.line_length_in;
        u32 x_offset = regs.dest.x;
        const u32 bpp_shift = Common::FoldRight(
            4U, [](u32 x, u32 y) { return std::min(x, static_cast<u32>(std::countr_zero(y))); },
            width, x_elements, x_offset, static_cast<u32>(address));
        width >>= bpp_shift;
        x_elements >>= bpp_shift;
        x_offset >>= bpp_shift;
        const u32 bytes_per_pixel = 1U << bpp_shift;
        const std::size_t dst_size = Tegra::Texture::CalculateSize(
            true, bytes_per_pixel, width, regs.dest.height, regs.dest.depth,
            regs.dest.BlockHeight(), regs.dest.BlockDepth());
        tmp_buffer.resize(dst_size);
        memory_manager.ReadBlock(address, tmp_buffer.data(), dst_size);
        Tegra::Texture::SwizzleSubrect(tmp_buffer, read_buffer, bytes_per_pixel, width,
                                       regs.dest.height, regs.dest.depth, x_offset, regs.dest.y,
                                       x_elements, regs.line_count, regs.dest.BlockHeight(),
                                       regs.dest.BlockDepth(), regs.line_length_in);
        memory_manager.WriteBlock(address, tmp_buffer.data(), dst_size);
    }
}

} // namespace Tegra::Engines::Upload
