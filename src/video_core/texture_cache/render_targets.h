// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <span>

#include "common/bit_cast.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon {

/// Framebuffer properties used to lookup a framebuffer
struct RenderTargets {
    constexpr auto operator<=>(const RenderTargets&) const noexcept = default;

    constexpr bool Contains(std::span<const ImageViewId> elements) const noexcept {
        const auto contains = [elements](ImageViewId item) {
            return std::ranges::find(elements, item) != elements.end();
        };
        return std::ranges::any_of(color_buffer_ids, contains) || contains(depth_buffer_id);
    }

    std::array<ImageViewId, NUM_RT> color_buffer_ids{};
    ImageViewId depth_buffer_id{};
    std::array<u8, NUM_RT> draw_buffers{};
    Extent2D size{};
    bool is_rescaled{};
};

} // namespace VideoCommon

namespace std {

template <>
struct hash<VideoCommon::RenderTargets> {
    size_t operator()(const VideoCommon::RenderTargets& rt) const noexcept {
        using VideoCommon::ImageViewId;
        size_t value = std::hash<ImageViewId>{}(rt.depth_buffer_id);
        for (const ImageViewId color_buffer_id : rt.color_buffer_ids) {
            value ^= std::hash<ImageViewId>{}(color_buffer_id);
        }
        value ^= Common::BitCast<u64>(rt.draw_buffers);
        value ^= Common::BitCast<u64>(rt.size);
        return value;
    }
};

} // namespace std
