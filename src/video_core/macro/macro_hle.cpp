// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <vector>
#include "common/scope_exit.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/macro/macro.h"
#include "video_core/macro/macro_hle.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra {
namespace {

using HLEFunction = void (*)(Engines::Maxwell3D& maxwell3d, const std::vector<u32>& parameters);

// HLE'd functions
void HLE_771BB18C62444DA0(Engines::Maxwell3D& maxwell3d, const std::vector<u32>& parameters) {
    const u32 instance_count = parameters[2] & maxwell3d.GetRegisterValue(0xD1B);

    maxwell3d.regs.draw.topology.Assign(
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0] & 0x3ffffff));
    maxwell3d.regs.global_base_instance_index = parameters[5];
    maxwell3d.regs.global_base_vertex_index = parameters[3];
    maxwell3d.regs.index_buffer.count = parameters[1];
    maxwell3d.regs.index_buffer.first = parameters[4];

    if (maxwell3d.ShouldExecute()) {
        maxwell3d.Rasterizer().Draw(true, instance_count);
    }
    maxwell3d.regs.index_buffer.count = 0;
}

void HLE_0D61FC9FAAC9FCAD(Engines::Maxwell3D& maxwell3d, const std::vector<u32>& parameters) {
    const u32 instance_count = (maxwell3d.GetRegisterValue(0xD1B) & parameters[2]);

    maxwell3d.regs.vertex_buffer.first = parameters[3];
    maxwell3d.regs.vertex_buffer.count = parameters[1];
    maxwell3d.regs.global_base_instance_index = parameters[4];
    maxwell3d.regs.draw.topology.Assign(
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0]));

    if (maxwell3d.ShouldExecute()) {
        maxwell3d.Rasterizer().Draw(false, instance_count);
    }
    maxwell3d.regs.vertex_buffer.count = 0;
}

void HLE_0217920100488FF7(Engines::Maxwell3D& maxwell3d, const std::vector<u32>& parameters) {
    const u32 instance_count = (maxwell3d.GetRegisterValue(0xD1B) & parameters[2]);
    const u32 element_base = parameters[4];
    const u32 base_instance = parameters[5];
    maxwell3d.regs.index_buffer.first = parameters[3];
    maxwell3d.regs.vertex_id_base = element_base;
    maxwell3d.regs.index_buffer.count = parameters[1];
    maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
    maxwell3d.regs.global_base_vertex_index = element_base;
    maxwell3d.regs.global_base_instance_index = base_instance;
    maxwell3d.CallMethod(0x8e3, 0x640, true);
    maxwell3d.CallMethod(0x8e4, element_base, true);
    maxwell3d.CallMethod(0x8e5, base_instance, true);
    maxwell3d.regs.draw.topology.Assign(
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[0]));
    if (maxwell3d.ShouldExecute()) {
        maxwell3d.Rasterizer().Draw(true, instance_count);
    }
    maxwell3d.regs.vertex_id_base = 0x0;
    maxwell3d.regs.index_buffer.count = 0;
    maxwell3d.regs.global_base_vertex_index = 0x0;
    maxwell3d.regs.global_base_instance_index = 0x0;
    maxwell3d.CallMethod(0x8e3, 0x640, true);
    maxwell3d.CallMethod(0x8e4, 0x0, true);
    maxwell3d.CallMethod(0x8e5, 0x0, true);
}

// Multidraw Indirect
void HLE_3F5E74B9C9A50164(Engines::Maxwell3D& maxwell3d, const std::vector<u32>& parameters) {
    SCOPE_EXIT({
        // Clean everything.
        maxwell3d.regs.vertex_id_base = 0x0;
        maxwell3d.regs.index_buffer.count = 0;
        maxwell3d.regs.global_base_vertex_index = 0x0;
        maxwell3d.regs.global_base_instance_index = 0x0;
        maxwell3d.CallMethod(0x8e3, 0x640, true);
        maxwell3d.CallMethod(0x8e4, 0x0, true);
        maxwell3d.CallMethod(0x8e5, 0x0, true);
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
    });
    const u32 start_indirect = parameters[0];
    const u32 end_indirect = parameters[1];
    if (start_indirect >= end_indirect) {
        // Nothing to do.
        return;
    }
    const auto topology =
        static_cast<Tegra::Engines::Maxwell3D::Regs::PrimitiveTopology>(parameters[2]);
    maxwell3d.regs.draw.topology.Assign(topology);
    const u32 padding = parameters[3];
    const std::size_t max_draws = parameters[4];

    const u32 indirect_words = 5 + padding;
    const std::size_t first_draw = start_indirect;
    const std::size_t effective_draws = end_indirect - start_indirect;
    const std::size_t last_draw = start_indirect + std::min(effective_draws, max_draws);

    for (std::size_t index = first_draw; index < last_draw; index++) {
        const std::size_t base = index * indirect_words + 5;
        const u32 num_vertices = parameters[base];
        const u32 instance_count = parameters[base + 1];
        const u32 first_index = parameters[base + 2];
        const u32 base_vertex = parameters[base + 3];
        const u32 base_instance = parameters[base + 4];
        maxwell3d.regs.index_buffer.first = first_index;
        maxwell3d.regs.vertex_id_base = base_vertex;
        maxwell3d.regs.index_buffer.count = num_vertices;
        maxwell3d.regs.global_base_vertex_index = base_vertex;
        maxwell3d.regs.global_base_instance_index = base_instance;
        maxwell3d.CallMethod(0x8e3, 0x640, true);
        maxwell3d.CallMethod(0x8e4, base_vertex, true);
        maxwell3d.CallMethod(0x8e5, base_instance, true);
        maxwell3d.dirty.flags[VideoCommon::Dirty::IndexBuffer] = true;
        if (maxwell3d.ShouldExecute()) {
            maxwell3d.Rasterizer().Draw(true, instance_count);
        }
    }
}

// Multi-layer Clear
void HLE_EAD26C3E2109B06B(Engines::Maxwell3D& maxwell3d, const std::vector<u32>& parameters) {
    ASSERT(parameters.size() == 1);

    const Engines::Maxwell3D::Regs::ClearSurface clear_params{parameters[0]};
    const u32 rt_index = clear_params.RT;
    const u32 num_layers = maxwell3d.regs.rt[rt_index].depth;
    ASSERT(clear_params.layer == 0);

    maxwell3d.regs.clear_surface.raw = clear_params.raw;
    maxwell3d.ProcessClearBuffers(num_layers);
}

constexpr std::array<std::pair<u64, HLEFunction>, 5> hle_funcs{{
    {0x771BB18C62444DA0, &HLE_771BB18C62444DA0},
    {0x0D61FC9FAAC9FCAD, &HLE_0D61FC9FAAC9FCAD},
    {0x0217920100488FF7, &HLE_0217920100488FF7},
    {0x3F5E74B9C9A50164, &HLE_3F5E74B9C9A50164},
    {0xEAD26C3E2109B06B, &HLE_EAD26C3E2109B06B},
}};

class HLEMacroImpl final : public CachedMacro {
public:
    explicit HLEMacroImpl(Engines::Maxwell3D& maxwell3d_, HLEFunction func_)
        : maxwell3d{maxwell3d_}, func{func_} {}

    void Execute(const std::vector<u32>& parameters, u32 method) override {
        func(maxwell3d, parameters);
    }

private:
    Engines::Maxwell3D& maxwell3d;
    HLEFunction func;
};

} // Anonymous namespace

HLEMacro::HLEMacro(Engines::Maxwell3D& maxwell3d_) : maxwell3d{maxwell3d_} {}
HLEMacro::~HLEMacro() = default;

std::unique_ptr<CachedMacro> HLEMacro::GetHLEProgram(u64 hash) const {
    const auto it = std::find_if(hle_funcs.cbegin(), hle_funcs.cend(),
                                 [hash](const auto& pair) { return pair.first == hash; });
    if (it == hle_funcs.end()) {
        return nullptr;
    }
    return std::make_unique<HLEMacroImpl>(maxwell3d, it->second);
}

} // namespace Tegra
