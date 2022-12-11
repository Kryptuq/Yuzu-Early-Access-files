// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/dirty_flags.h"
#include "video_core/engines/draw_manager.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra::Engines {
DrawManager::DrawManager(Maxwell3D* maxwell3d_) : maxwell3d(maxwell3d_) {}

void DrawManager::ProcessMethodCall(u32 method, u32 argument) {
    const auto& regs{maxwell3d->regs};
    switch (method) {
    case MAXWELL3D_REG_INDEX(clear_surface):
        return Clear(1);
    case MAXWELL3D_REG_INDEX(draw.begin):
        return DrawBegin();
    case MAXWELL3D_REG_INDEX(draw.end):
        return DrawEnd();
    case MAXWELL3D_REG_INDEX(vertex_buffer.first):
    case MAXWELL3D_REG_INDEX(vertex_buffer.count):
    case MAXWELL3D_REG_INDEX(index_buffer.first):
        break;
    case MAXWELL3D_REG_INDEX(index_buffer.count):
        draw_state.draw_indexed = true;
        break;
    case MAXWELL3D_REG_INDEX(index_buffer32_subsequent):
    case MAXWELL3D_REG_INDEX(index_buffer16_subsequent):
    case MAXWELL3D_REG_INDEX(index_buffer8_subsequent):
        draw_state.instance_count++;
        [[fallthrough]];
    case MAXWELL3D_REG_INDEX(index_buffer32_first):
    case MAXWELL3D_REG_INDEX(index_buffer16_first):
    case MAXWELL3D_REG_INDEX(index_buffer8_first):
        return DrawIndexSmall(argument);
    case MAXWELL3D_REG_INDEX(draw_inline_index):
        SetInlineIndexBuffer(argument);
        break;
    case MAXWELL3D_REG_INDEX(inline_index_2x16.even):
        SetInlineIndexBuffer(regs.inline_index_2x16.even);
        SetInlineIndexBuffer(regs.inline_index_2x16.odd);
        break;
    case MAXWELL3D_REG_INDEX(inline_index_4x8.index0):
        SetInlineIndexBuffer(regs.inline_index_4x8.index0);
        SetInlineIndexBuffer(regs.inline_index_4x8.index1);
        SetInlineIndexBuffer(regs.inline_index_4x8.index2);
        SetInlineIndexBuffer(regs.inline_index_4x8.index3);
        break;
    case MAXWELL3D_REG_INDEX(vertex_array_instance_first):
    case MAXWELL3D_REG_INDEX(vertex_array_instance_subsequent): {
        LOG_WARNING(HW_GPU, "(STUBBED) called");
        break;
    }
    default:
        break;
    }
}

void DrawManager::Clear(u32 layer_count) {
    if (maxwell3d->ShouldExecute()) {
        maxwell3d->rasterizer->Clear(layer_count);
    }
}

void DrawManager::DrawDeferred() {
    if (draw_state.draw_mode != DrawMode::Instance || draw_state.instance_count == 0) {
        return;
    }
    DrawEnd(draw_state.instance_count + 1, true);
    draw_state.instance_count = 0;
}

void DrawManager::DrawArray(PrimitiveTopology topology, u32 vertex_first, u32 vertex_count,
                            u32 base_instance, u32 num_instances) {
    draw_state.topology = topology;
    draw_state.vertex_buffer.first = vertex_first;
    draw_state.vertex_buffer.count = vertex_count;
    draw_state.base_instance = base_instance;
    ProcessDraw(false, num_instances);
}

void DrawManager::DrawIndex(PrimitiveTopology topology, u32 index_first, u32 index_count,
                            u32 base_index, u32 base_instance, u32 num_instances) {
    const auto& regs{maxwell3d->regs};
    draw_state.topology = topology;
    draw_state.index_buffer = regs.index_buffer;
    draw_state.index_buffer.first = index_first;
    draw_state.index_buffer.count = index_count;
    draw_state.base_index = base_index;
    draw_state.base_instance = base_instance;
    ProcessDraw(true, num_instances);
}

void DrawManager::SetInlineIndexBuffer(u32 index) {
    draw_state.inline_index_draw_indexes.push_back(static_cast<u8>(index & 0x000000ff));
    draw_state.inline_index_draw_indexes.push_back(static_cast<u8>((index & 0x0000ff00) >> 8));
    draw_state.inline_index_draw_indexes.push_back(static_cast<u8>((index & 0x00ff0000) >> 16));
    draw_state.inline_index_draw_indexes.push_back(static_cast<u8>((index & 0xff000000) >> 24));
    draw_state.draw_mode = DrawMode::InlineIndex;
}

void DrawManager::DrawBegin() {
    const auto& regs{maxwell3d->regs};
    auto reset_instance_count = regs.draw.instance_id == Maxwell3D::Regs::Draw::InstanceId::First;
    auto increment_instance_count =
        regs.draw.instance_id == Maxwell3D::Regs::Draw::InstanceId::Subsequent;
    if (reset_instance_count) {
        DrawDeferred();
        draw_state.instance_count = 0;
        draw_state.draw_mode = DrawMode::General;
    } else if (increment_instance_count) {
        draw_state.instance_count++;
        draw_state.draw_mode = DrawMode::Instance;
    }

    draw_state.topology = regs.draw.topology;
}

void DrawManager::DrawEnd(u32 instance_count, bool force_draw) {
    const auto& regs{maxwell3d->regs};
    switch (draw_state.draw_mode) {
    case DrawMode::Instance:
        if (!force_draw) {
            break;
        }
        [[fallthrough]];
    case DrawMode::General:
        draw_state.base_instance = regs.global_base_instance_index;
        draw_state.base_index = regs.global_base_vertex_index;
        if (draw_state.draw_indexed) {
            draw_state.index_buffer = regs.index_buffer;
            ProcessDraw(true, instance_count);
        } else {
            draw_state.vertex_buffer = regs.vertex_buffer;
            ProcessDraw(false, instance_count);
        }
        draw_state.draw_indexed = false;
        break;
    case DrawMode::InlineIndex:
        draw_state.base_instance = regs.global_base_instance_index;
        draw_state.base_index = regs.global_base_vertex_index;
        draw_state.index_buffer = regs.index_buffer;
        draw_state.index_buffer.count =
            static_cast<u32>(draw_state.inline_index_draw_indexes.size() / 4);
        draw_state.index_buffer.format = Maxwell3D::Regs::IndexFormat::UnsignedInt;
        ProcessDraw(true, instance_count);
        draw_state.inline_index_draw_indexes.clear();
        break;
    }
}

void DrawManager::DrawIndexSmall(u32 argument) {
    const auto& regs{maxwell3d->regs};
    IndexBufferSmall index_small_params{argument};
    draw_state.base_instance = regs.global_base_instance_index;
    draw_state.base_index = regs.global_base_vertex_index;
    draw_state.index_buffer = regs.index_buffer;
    draw_state.index_buffer.first = index_small_params.first;
    draw_state.index_buffer.count = index_small_params.count;
    draw_state.topology = index_small_params.topology;
    maxwell3d->dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
    ProcessDraw(true, 1);
}

void DrawManager::UpdateTopology() {
    const auto& regs{maxwell3d->regs};
    switch (regs.primitive_topology_control) {
    case PrimitiveTopologyControl::UseInBeginMethods:
        break;
    case PrimitiveTopologyControl::UseSeparateState:
        switch (regs.topology_override) {
        case PrimitiveTopologyOverride::None:
            break;
        case PrimitiveTopologyOverride::Points:
            draw_state.topology = PrimitiveTopology::Points;
            break;
        case PrimitiveTopologyOverride::Lines:
            draw_state.topology = PrimitiveTopology::Lines;
            break;
        case PrimitiveTopologyOverride::LineStrip:
            draw_state.topology = PrimitiveTopology::LineStrip;
            break;
        default:
            draw_state.topology = static_cast<PrimitiveTopology>(regs.topology_override);
            break;
        }
        break;
    }
}

void DrawManager::ProcessDraw(bool draw_indexed, u32 instance_count) {
    LOG_TRACE(HW_GPU, "called, topology={}, count={}", draw_state.topology,
              draw_indexed ? draw_state.index_buffer.count : draw_state.vertex_buffer.count);

    UpdateTopology();

    if (maxwell3d->ShouldExecute()) {
        maxwell3d->rasterizer->Draw(draw_indexed, instance_count);
    }
}
} // namespace Tegra::Engines
