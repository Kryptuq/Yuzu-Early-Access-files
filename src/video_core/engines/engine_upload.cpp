// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>

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
    const GPUVAddr address{regs.dest.Address()};
    if (is_linear) {
        rasterizer->AccelerateInline2Memory(address, copy_size, inner_buffer);
    } else {
        UNIMPLEMENTED_IF(regs.dest.z != 0);
        UNIMPLEMENTED_IF(regs.dest.depth != 1);
        UNIMPLEMENTED_IF(regs.dest.BlockWidth() != 0);
        UNIMPLEMENTED_IF(regs.dest.BlockDepth() != 0);
        const std::size_t dst_size = Tegra::Texture::CalculateSize(
            true, 1, regs.dest.width, regs.dest.height, 1, regs.dest.BlockHeight(), 0);
        tmp_buffer.resize(dst_size);
        memory_manager.ReadBlock(address, tmp_buffer.data(), dst_size);
        Tegra::Texture::SwizzleKepler(regs.dest.width, regs.dest.height, regs.dest.x, regs.dest.y,
                                      regs.dest.BlockHeight(), copy_size, inner_buffer.data(),
                                      tmp_buffer.data());
        memory_manager.WriteBlock(address, tmp_buffer.data(), dst_size);
    }
}

} // namespace Tegra::Engines::Upload
