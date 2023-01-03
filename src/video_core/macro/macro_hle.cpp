// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <vector>
#include "common/assert.h"
#include "common/scope_exit.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/draw_manager.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/macro/macro.h"
#include "video_core/macro/macro_hle.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra {

using Maxwell = Engines::Maxwell3D;

namespace {

bool IsTopologySafe(Maxwell::Regs::PrimitiveTopology topology) {
    switch (topology) {
    case Maxwell::Regs::PrimitiveTopology::Points:
    case Maxwell::Regs::PrimitiveTopology::Lines:
    case Maxwell::Regs::PrimitiveTopology::LineLoop:
    case Maxwell::Regs::PrimitiveTopology::LineStrip:
    case Maxwell::Regs::PrimitiveTopology::Triangles:
    case Maxwell::Regs::PrimitiveTopology::TriangleStrip:
    case Maxwell::Regs::PrimitiveTopology::TriangleFan:
    case Maxwell::Regs::PrimitiveTopology::LinesAdjacency:
    case Maxwell::Regs::PrimitiveTopology::LineStripAdjacency:
    case Maxwell::Regs::PrimitiveTopology::TrianglesAdjacency:
    case Maxwell::Regs::PrimitiveTopology::TriangleStripAdjacency:
    case Maxwell::Regs::PrimitiveTopology::Patches:
        return true;
    case Maxwell::Regs::PrimitiveTopology::Quads:
    case Maxwell::Regs::PrimitiveTopology::QuadStrip:
    case Maxwell::Regs::PrimitiveTopology::Polygon:
    default:
        return false;
    }
}

class HLEMacroImpl : public CachedMacro {
public:
    explicit HLEMacroImpl(Engines::Maxwell3D& maxwell3d_) : maxwell3d{maxwell3d_} {}

protected:
    Engines::Maxwell3D& maxwell3d;
};

class HLE_771BB18C62444DA0 final : public HLEMacroImpl {
public:
    explicit HLE_771BB18C62444DA0(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        maxwell3d.RefreshParameters();
        const u32 instance_count = parameters[2] & maxwell3d.GetRegisterValue(0xD1B);
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        maxwell3d.draw_manager->DrawIndex(
            static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0] &
                                                                            0x3ffffff),
            parameters[4], parameters[1], parameters[3], parameters[5], instance_count);
    }
};

class HLE_DrawArraysIndirect final : public HLEMacroImpl {
public:
    explicit HLE_DrawArraysIndirect(Engines::Maxwell3D& maxwell3d_, bool extended_ = false)
        : HLEMacroImpl(maxwell3d_), extended(extended_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        auto topology = static_cast<Maxwell::Regs::PrimitiveTopology>(parameters[0]);
        if (!maxwell3d.AnyParametersDirty() || !IsTopologySafe(topology)) {
            Fallback(parameters);
            return;
        }

        auto& params = maxwell3d.draw_manager->GetIndirectParams();
        params.is_indexed = false;
        params.include_count = false;
        params.count_start_address = 0;
        params.indirect_start_address = maxwell3d.GetMacroAddress(1);
        params.buffer_size = 4 * sizeof(u32);
        params.max_draw_counts = 1;
        params.stride = 0;

        if (extended) {
            maxwell3d.engine_state = Maxwell::EngineHint::OnHLEMacro;
            maxwell3d.SetHLEReplacementAttributeType(
                0, 0x640, Maxwell::HLEReplacementAttributeType::BaseInstance);
        }

        maxwell3d.draw_manager->DrawArrayIndirect(topology);

        if (extended) {
            maxwell3d.engine_state = Maxwell::EngineHint::None;
            maxwell3d.replace_table.clear();
        }
    }

private:
    void Fallback(const std::vector<u32>& parameters) {
        SCOPE_EXIT({
            if (extended) {
                maxwell3d.engine_state = Maxwell::EngineHint::None;
                maxwell3d.replace_table.clear();
            }
        });
        maxwell3d.RefreshParameters();
        const u32 instance_count = (maxwell3d.GetRegisterValue(0xD1B) & parameters[2]);

        auto topology = static_cast<Maxwell::Regs::PrimitiveTopology>(parameters[0]);
        const u32 vertex_first = parameters[3];
        const u32 vertex_count = parameters[1];

        if (!IsTopologySafe(topology) &&
            static_cast<size_t>(maxwell3d.GetMaxCurrentVertices()) <
                static_cast<size_t>(vertex_first) + static_cast<size_t>(vertex_count)) {
            ASSERT_MSG(false, "Faulty draw!");
            return;
        }

        const u32 base_instance = parameters[4];
        if (extended) {
            maxwell3d.regs.global_base_instance_index = base_instance;
            maxwell3d.engine_state = Maxwell::EngineHint::OnHLEMacro;
            maxwell3d.SetHLEReplacementAttributeType(
                0, 0x640, Maxwell::HLEReplacementAttributeType::BaseInstance);
        }

        maxwell3d.draw_manager->DrawArray(topology, vertex_first, vertex_count, base_instance,
                                          instance_count);

        if (extended) {
            maxwell3d.regs.global_base_instance_index = 0;
            maxwell3d.engine_state = Maxwell::EngineHint::None;
            maxwell3d.replace_table.clear();
        }
    }

    bool extended;
};

class HLE_DrawIndexedIndirect final : public HLEMacroImpl {
public:
    explicit HLE_DrawIndexedIndirect(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        auto topology = static_cast<Maxwell::Regs::PrimitiveTopology>(parameters[0]);
        if (!maxwell3d.AnyParametersDirty() || !IsTopologySafe(topology)) {
            Fallback(parameters);
            return;
        }

        const u32 estimate = static_cast<u32>(maxwell3d.EstimateIndexBufferSize());
        const u32 element_base = parameters[4];
        const u32 base_instance = parameters[5];
        maxwell3d.regs.vertex_id_base = element_base;
        maxwell3d.regs.global_base_vertex_index = element_base;
        maxwell3d.regs.global_base_instance_index = base_instance;
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        maxwell3d.engine_state = Maxwell::EngineHint::OnHLEMacro;
        maxwell3d.SetHLEReplacementAttributeType(0, 0x640,
                                                 Maxwell::HLEReplacementAttributeType::BaseVertex);
        maxwell3d.SetHLEReplacementAttributeType(
            0, 0x644, Maxwell::HLEReplacementAttributeType::BaseInstance);
        auto& params = maxwell3d.draw_manager->GetIndirectParams();
        params.is_indexed = true;
        params.include_count = false;
        params.count_start_address = 0;
        params.indirect_start_address = maxwell3d.GetMacroAddress(1);
        params.buffer_size = 5 * sizeof(u32);
        params.max_draw_counts = 1;
        params.stride = 0;
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        maxwell3d.draw_manager->DrawIndexedIndirect(topology, 0, estimate);
        maxwell3d.engine_state = Maxwell::EngineHint::None;
        maxwell3d.replace_table.clear();
        maxwell3d.regs.vertex_id_base = 0x0;
        maxwell3d.regs.global_base_vertex_index = 0x0;
        maxwell3d.regs.global_base_instance_index = 0x0;
    }

private:
    void Fallback(const std::vector<u32>& parameters) {
        maxwell3d.RefreshParameters();
        const u32 instance_count = (maxwell3d.GetRegisterValue(0xD1B) & parameters[2]);
        const u32 element_base = parameters[4];
        const u32 base_instance = parameters[5];
        maxwell3d.regs.vertex_id_base = element_base;
        maxwell3d.regs.global_base_vertex_index = element_base;
        maxwell3d.regs.global_base_instance_index = base_instance;
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        maxwell3d.engine_state = Maxwell::EngineHint::OnHLEMacro;
        maxwell3d.SetHLEReplacementAttributeType(0, 0x640,
                                                 Maxwell::HLEReplacementAttributeType::BaseVertex);
        maxwell3d.SetHLEReplacementAttributeType(
            0, 0x644, Maxwell::HLEReplacementAttributeType::BaseInstance);

        maxwell3d.draw_manager->DrawIndex(
            static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0]),
            parameters[3], parameters[1], element_base, base_instance, instance_count);

        maxwell3d.regs.vertex_id_base = 0x0;
        maxwell3d.regs.global_base_vertex_index = 0x0;
        maxwell3d.regs.global_base_instance_index = 0x0;
        maxwell3d.engine_state = Maxwell::EngineHint::None;
        maxwell3d.replace_table.clear();
    }
};

class HLE_MultiLayerClear final : public HLEMacroImpl {
public:
    explicit HLE_MultiLayerClear(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        maxwell3d.RefreshParameters();
        ASSERT(parameters.size() == 1);

        const Engines::Maxwell3D::Regs::ClearSurface clear_params{parameters[0]};
        const u32 rt_index = clear_params.RT;
        const u32 num_layers = maxwell3d.regs.rt[rt_index].depth;
        ASSERT(clear_params.layer == 0);

        maxwell3d.regs.clear_surface.raw = clear_params.raw;
        maxwell3d.draw_manager->Clear(num_layers);
    }
};

class HLE_MultiDrawIndexedIndirectCount final : public HLEMacroImpl {
public:
    explicit HLE_MultiDrawIndexedIndirectCount(Engines::Maxwell3D& maxwell3d_)
        : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        const auto topology = static_cast<Maxwell::Regs::PrimitiveTopology>(parameters[2]);
        if (!IsTopologySafe(topology)) {
            Fallback(parameters);
            return;
        }

        const u32 start_indirect = parameters[0];
        const u32 end_indirect = parameters[1];
        if (start_indirect >= end_indirect) {
            // Nothing to do.
            return;
        }

        const u32 padding = parameters[3]; // padding is in words

        // size of each indirect segment
        const u32 indirect_words = 5 + padding;
        const u32 stride = indirect_words * sizeof(u32);
        const std::size_t draw_count = end_indirect - start_indirect;
        const u32 estimate = static_cast<u32>(maxwell3d.EstimateIndexBufferSize());
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        auto& params = maxwell3d.draw_manager->GetIndirectParams();
        params.is_indexed = true;
        params.include_count = true;
        params.count_start_address = maxwell3d.GetMacroAddress(4);
        params.indirect_start_address = maxwell3d.GetMacroAddress(5);
        params.buffer_size = stride * draw_count;
        params.max_draw_counts = draw_count;
        params.stride = stride;
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        maxwell3d.engine_state = Maxwell::EngineHint::OnHLEMacro;
        maxwell3d.SetHLEReplacementAttributeType(0, 0x640,
                                                 Maxwell::HLEReplacementAttributeType::BaseVertex);
        maxwell3d.SetHLEReplacementAttributeType(
            0, 0x644, Maxwell::HLEReplacementAttributeType::BaseInstance);
        maxwell3d.draw_manager->DrawIndexedIndirect(topology, 0, estimate);
        maxwell3d.engine_state = Maxwell::EngineHint::None;
        maxwell3d.replace_table.clear();
    }

private:
    void Fallback(const std::vector<u32>& parameters) {
        SCOPE_EXIT({
            // Clean everything.
            // Clean everything.
            maxwell3d.regs.vertex_id_base = 0x0;
            maxwell3d.engine_state = Maxwell::EngineHint::None;
            maxwell3d.replace_table.clear();
        });
        maxwell3d.RefreshParameters();
        const u32 start_indirect = parameters[0];
        const u32 end_indirect = parameters[1];
        if (start_indirect >= end_indirect) {
            // Nothing to do.
            return;
        }
        const auto topology = static_cast<Maxwell::Regs::PrimitiveTopology>(parameters[2]);
        const u32 padding = parameters[3];
        const std::size_t max_draws = parameters[4];

        const u32 indirect_words = 5 + padding;
        const std::size_t first_draw = start_indirect;
        const std::size_t effective_draws = end_indirect - start_indirect;
        const std::size_t last_draw = start_indirect + std::min(effective_draws, max_draws);

        for (std::size_t index = first_draw; index < last_draw; index++) {
            const std::size_t base = index * indirect_words + 5;
            const u32 base_vertex = parameters[base + 3];
            const u32 base_instance = parameters[base + 4];
            maxwell3d.regs.vertex_id_base = base_vertex;
            maxwell3d.engine_state = Maxwell::EngineHint::OnHLEMacro;
            maxwell3d.SetHLEReplacementAttributeType(
                0, 0x640, Maxwell::HLEReplacementAttributeType::BaseVertex);
            maxwell3d.SetHLEReplacementAttributeType(
                0, 0x644, Maxwell::HLEReplacementAttributeType::BaseInstance);
            maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
            maxwell3d.draw_manager->DrawIndex(topology, parameters[base + 2], parameters[base],
                                              base_vertex, base_instance, parameters[base + 1]);
        }
    }
};

class HLE_C713C83D8F63CCF3 final : public HLEMacroImpl {
public:
    explicit HLE_C713C83D8F63CCF3(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        maxwell3d.RefreshParameters();
        const u32 offset = (parameters[0] & 0x3FFFFFFF) << 2;
        const u32 address = maxwell3d.regs.shadow_scratch[24];
        auto& const_buffer = maxwell3d.regs.const_buffer;
        const_buffer.size = 0x7000;
        const_buffer.address_high = (address >> 24) & 0xFF;
        const_buffer.address_low = address << 8;
        const_buffer.offset = offset;
    }
};

class HLE_D7333D26E0A93EDE final : public HLEMacroImpl {
public:
    explicit HLE_D7333D26E0A93EDE(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        maxwell3d.RefreshParameters();
        const size_t index = parameters[0];
        const u32 address = maxwell3d.regs.shadow_scratch[42 + index];
        const u32 size = maxwell3d.regs.shadow_scratch[47 + index];
        auto& const_buffer = maxwell3d.regs.const_buffer;
        const_buffer.size = size;
        const_buffer.address_high = (address >> 24) & 0xFF;
        const_buffer.address_low = address << 8;
    }
};

class HLE_BindShader final : public HLEMacroImpl {
public:
    explicit HLE_BindShader(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        maxwell3d.RefreshParameters();
        auto& regs = maxwell3d.regs;
        const u32 index = parameters[0];
        if ((parameters[1] - regs.shadow_scratch[28 + index]) == 0) {
            return;
        }

        regs.pipelines[index & 0xF].offset = parameters[2];
        maxwell3d.dirty.flags[VideoCommon::Dirty::Shaders] = true;
        regs.shadow_scratch[28 + index] = parameters[1];
        regs.shadow_scratch[34 + index] = parameters[2];

        const u32 address = parameters[4];
        auto& const_buffer = regs.const_buffer;
        const_buffer.size = 0x10000;
        const_buffer.address_high = (address >> 24) & 0xFF;
        const_buffer.address_low = address << 8;

        const size_t bind_group_id = parameters[3] & 0x7F;
        auto& bind_group = regs.bind_groups[bind_group_id];
        bind_group.raw_config = 0x11;
        maxwell3d.ProcessCBBind(bind_group_id);
    }
};

class HLE_SetRasterBoundingBox final : public HLEMacroImpl {
public:
    explicit HLE_SetRasterBoundingBox(Engines::Maxwell3D& maxwell3d_) : HLEMacroImpl(maxwell3d_) {}

    void Execute(const std::vector<u32>& parameters, [[maybe_unused]] u32 method) override {
        maxwell3d.RefreshParameters();
        const u32 raster_mode = parameters[0];
        auto& regs = maxwell3d.regs;
        const u32 raster_enabled = maxwell3d.regs.conservative_raster_enable;
        const u32 scratch_data = maxwell3d.regs.shadow_scratch[52];
        regs.raster_bounding_box.raw = raster_mode & 0xFFFFF00F;
        regs.raster_bounding_box.pad.Assign(scratch_data & raster_enabled);
    }
};

} // Anonymous namespace

HLEMacro::HLEMacro(Engines::Maxwell3D& maxwell3d_) : maxwell3d{maxwell3d_} {
    builders.emplace(0x771BB18C62444DA0ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_771BB18C62444DA0>(maxwell3d__);
                         }));
    builders.emplace(0x0D61FC9FAAC9FCADULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_DrawArraysIndirect>(maxwell3d__);
                         }));
    builders.emplace(0x8A4D173EB99A8603ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_DrawArraysIndirect>(maxwell3d__, true);
                         }));
    builders.emplace(0x0217920100488FF7ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_DrawIndexedIndirect>(maxwell3d__);
                         }));
    builders.emplace(0x3F5E74B9C9A50164ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_MultiDrawIndexedIndirectCount>(
                                 maxwell3d__);
                         }));
    builders.emplace(0xEAD26C3E2109B06BULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_MultiLayerClear>(maxwell3d__);
                         }));
    builders.emplace(0xC713C83D8F63CCF3ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_C713C83D8F63CCF3>(maxwell3d__);
                         }));
    builders.emplace(0xD7333D26E0A93EDEULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_D7333D26E0A93EDE>(maxwell3d__);
                         }));
    builders.emplace(0xEB29B2A09AA06D38ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_BindShader>(maxwell3d__);
                         }));
    builders.emplace(0xDB1341DBEB4C8AF7ULL,
                     std::function<std::unique_ptr<CachedMacro>(Engines::Maxwell3D&)>(
                         [](Engines::Maxwell3D& maxwell3d__) -> std::unique_ptr<CachedMacro> {
                             return std::make_unique<HLE_SetRasterBoundingBox>(maxwell3d__);
                         }));
}

HLEMacro::~HLEMacro() = default;

std::unique_ptr<CachedMacro> HLEMacro::GetHLEProgram(u64 hash) const {
    const auto it = builders.find(hash);
    if (it == builders.end()) {
        return nullptr;
    }
    return it->second(maxwell3d);
}

} // namespace Tegra
