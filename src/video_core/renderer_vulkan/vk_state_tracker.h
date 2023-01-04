// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <limits>

#include "common/common_types.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/maxwell_3d.h"

namespace Tegra {
namespace Control {
struct ChannelState;
}
} // namespace Tegra

namespace Vulkan {

namespace Dirty {

enum : u8 {
    First = VideoCommon::Dirty::LastCommonEntry,

    VertexInput,
    VertexAttribute0,
    VertexAttribute31 = VertexAttribute0 + 31,
    VertexBinding0,
    VertexBinding31 = VertexBinding0 + 31,

    Viewports,
    Scissors,
    DepthBias,
    BlendConstants,
    DepthBounds,
    StencilProperties,
    StencilReference,
    StencilWriteMask,
    StencilCompare,
    LineWidth,

    CullMode,
    DepthBoundsEnable,
    DepthTestEnable,
    DepthWriteEnable,
    DepthCompareOp,
    FrontFace,
    StencilOp,
    StencilTestEnable,
    PrimitiveRestartEnable,
    RasterizerDiscardEnable,
    DepthBiasEnable,
    StateEnable,
    LogicOp,
    LogicOpEnable,
    DepthClampEnable,

    Blending,
    BlendEnable,
    BlendEquations,
    ColorMask,
    ViewportSwizzles,

    Last,
};
static_assert(Last <= std::numeric_limits<u8>::max());

} // namespace Dirty

class StateTracker {
    using Maxwell = Tegra::Engines::Maxwell3D::Regs;

public:
    explicit StateTracker();

    void InvalidateCommandBufferState() {
        (*flags) |= invalidation_flags;
        current_topology = INVALID_TOPOLOGY;
        stencil_reset = true;
    }

    void InvalidateViewports() {
        (*flags)[Dirty::Viewports] = true;
    }

    void InvalidateScissors() {
        (*flags)[Dirty::Scissors] = true;
    }

    bool TouchViewports() {
        const bool dirty_viewports = Exchange(Dirty::Viewports, false);
        const bool rescale_viewports = Exchange(VideoCommon::Dirty::RescaleViewports, false);
        return dirty_viewports || rescale_viewports;
    }

    bool TouchScissors() {
        const bool dirty_scissors = Exchange(Dirty::Scissors, false);
        const bool rescale_scissors = Exchange(VideoCommon::Dirty::RescaleScissors, false);
        return dirty_scissors || rescale_scissors;
    }

    bool TouchDepthBias() {
        return Exchange(Dirty::DepthBias, false) ||
               Exchange(VideoCommon::Dirty::DepthBiasGlobal, false);
    }

    bool TouchBlendConstants() {
        return Exchange(Dirty::BlendConstants, false);
    }

    bool TouchDepthBounds() {
        return Exchange(Dirty::DepthBounds, false);
    }

    bool TouchStencilProperties() {
        return Exchange(Dirty::StencilProperties, false);
    }

    bool TouchStencilReference() {
        return Exchange(Dirty::StencilReference, false);
    }

    bool TouchStencilWriteMask() {
        return Exchange(Dirty::StencilWriteMask, false);
    }

    bool TouchStencilCompare() {
        return Exchange(Dirty::StencilCompare, false);
    }

    template <typename T>
    bool ExchangeCheck(T& old_value, T new_value) {
        bool result = old_value != new_value;
        old_value = new_value;
        return result;
    }

    bool TouchStencilSide(bool two_sided_stencil_new) {
        return ExchangeCheck(two_sided_stencil, two_sided_stencil_new) || stencil_reset;
    }

    bool CheckStencilReferenceFront(u32 new_value) {
        return ExchangeCheck(front.ref, new_value) || stencil_reset;
    }

    bool CheckStencilReferenceBack(u32 new_value) {
        return ExchangeCheck(back.ref, new_value) || stencil_reset;
    }

    bool CheckStencilWriteMaskFront(u32 new_value) {
        return ExchangeCheck(front.write_mask, new_value) || stencil_reset;
    }

    bool CheckStencilWriteMaskBack(u32 new_value) {
        return ExchangeCheck(back.write_mask, new_value) || stencil_reset;
    }

    bool CheckStencilCompareMaskFront(u32 new_value) {
        return ExchangeCheck(front.compare_mask, new_value) || stencil_reset;
    }

    bool CheckStencilCompareMaskBack(u32 new_value) {
        return ExchangeCheck(back.compare_mask, new_value) || stencil_reset;
    }

    void ClearStencilReset() {
        stencil_reset = false;
    }

    bool TouchLineWidth() const {
        return Exchange(Dirty::LineWidth, false);
    }

    bool TouchCullMode() {
        return Exchange(Dirty::CullMode, false);
    }

    bool TouchStateEnable() {
        return Exchange(Dirty::StateEnable, false);
    }

    bool TouchDepthBoundsTestEnable() {
        return Exchange(Dirty::DepthBoundsEnable, false);
    }

    bool TouchDepthTestEnable() {
        return Exchange(Dirty::DepthTestEnable, false);
    }

    bool TouchDepthWriteEnable() {
        return Exchange(Dirty::DepthWriteEnable, false);
    }

    bool TouchPrimitiveRestartEnable() {
        return Exchange(Dirty::PrimitiveRestartEnable, false);
    }

    bool TouchRasterizerDiscardEnable() {
        return Exchange(Dirty::RasterizerDiscardEnable, false);
    }

    bool TouchDepthBiasEnable() {
        return Exchange(Dirty::DepthBiasEnable, false);
    }

    bool TouchLogicOpEnable() {
        return Exchange(Dirty::LogicOpEnable, false);
    }

    bool TouchDepthClampEnable() {
        return Exchange(Dirty::DepthClampEnable, false);
    }

    bool TouchDepthCompareOp() {
        return Exchange(Dirty::DepthCompareOp, false);
    }

    bool TouchFrontFace() {
        return Exchange(Dirty::FrontFace, false);
    }

    bool TouchStencilOp() {
        return Exchange(Dirty::StencilOp, false);
    }

    bool TouchBlending() {
        return Exchange(Dirty::Blending, false);
    }

    bool TouchBlendEnable() {
        return Exchange(Dirty::BlendEnable, false);
    }

    bool TouchBlendEquations() {
        return Exchange(Dirty::BlendEquations, false);
    }

    bool TouchColorMask() {
        return Exchange(Dirty::ColorMask, false);
    }

    bool TouchStencilTestEnable() {
        return Exchange(Dirty::StencilTestEnable, false);
    }

    bool TouchLogicOp() {
        return Exchange(Dirty::LogicOp, false);
    }

    bool ChangePrimitiveTopology(Maxwell::PrimitiveTopology new_topology) {
        const bool has_changed = current_topology != new_topology;
        current_topology = new_topology;
        return has_changed;
    }

    void SetupTables(Tegra::Control::ChannelState& channel_state);

    void ChangeChannel(Tegra::Control::ChannelState& channel_state);

    void InvalidateState();

private:
    static constexpr auto INVALID_TOPOLOGY = static_cast<Maxwell::PrimitiveTopology>(~0u);

    bool Exchange(std::size_t id, bool new_value) const noexcept {
        const bool is_dirty = (*flags)[id];
        (*flags)[id] = new_value;
        return is_dirty;
    }

    struct StencilProperties {
        u32 ref = 0;
        u32 write_mask = 0;
        u32 compare_mask = 0;
    };

    Tegra::Engines::Maxwell3D::DirtyState::Flags* flags;
    Tegra::Engines::Maxwell3D::DirtyState::Flags default_flags;
    Tegra::Engines::Maxwell3D::DirtyState::Flags invalidation_flags;
    Maxwell::PrimitiveTopology current_topology = INVALID_TOPOLOGY;
    bool two_sided_stencil = false;
    StencilProperties front{};
    StencilProperties back{};
    bool stencil_reset = false;
};

} // namespace Vulkan
