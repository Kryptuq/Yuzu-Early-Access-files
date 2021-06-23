// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/texture_cache/slot_vector.h"

namespace VideoCommon {

constexpr size_t NUM_RT = 8;
constexpr size_t MAX_MIP_LEVELS = 14;

constexpr SlotId CORRUPT_ID{0xfffffffe};

using ImageId = SlotId;
using ImageMapId = SlotId;
using ImageViewId = SlotId;
using ImageAllocId = SlotId;
using SamplerId = SlotId;
using FramebufferId = SlotId;

enum class ImageType : u32 {
    e1D,
    e2D,
    e3D,
    Linear,
    Buffer,
};

enum class ImageViewType : u32 {
    e1D,
    e2D,
    Cube,
    e3D,
    e1DArray,
    e2DArray,
    CubeArray,
    Rect,
    Buffer,
};
constexpr size_t NUM_IMAGE_VIEW_TYPES = 9;

enum class RelaxedOptions : u32 {
    Size = 1 << 0,
    Format = 1 << 1,
    Samples = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(RelaxedOptions)

struct Offset2D {
    constexpr auto operator<=>(const Offset2D&) const noexcept = default;

    s32 x;
    s32 y;
};

struct Offset3D {
    constexpr auto operator<=>(const Offset3D&) const noexcept = default;

    s32 x;
    s32 y;
    s32 z;
};

struct Region2D {
    constexpr auto operator<=>(const Region2D&) const noexcept = default;

    Offset2D start;
    Offset2D end;
};

struct Extent2D {
    constexpr auto operator<=>(const Extent2D&) const noexcept = default;

    u32 width;
    u32 height;
};

struct Extent3D {
    constexpr auto operator<=>(const Extent3D&) const noexcept = default;

    u32 width;
    u32 height;
    u32 depth;
};

struct SubresourceLayers {
    s32 base_level = 0;
    s32 base_layer = 0;
    s32 num_layers = 1;
};

struct SubresourceBase {
    constexpr auto operator<=>(const SubresourceBase&) const noexcept = default;

    s32 level = 0;
    s32 layer = 0;
};

struct SubresourceExtent {
    constexpr auto operator<=>(const SubresourceExtent&) const noexcept = default;

    s32 levels = 1;
    s32 layers = 1;
};

struct SubresourceRange {
    constexpr auto operator<=>(const SubresourceRange&) const noexcept = default;

    SubresourceBase base;
    SubresourceExtent extent;
};

struct ImageCopy {
    SubresourceLayers src_subresource;
    SubresourceLayers dst_subresource;
    Offset3D src_offset;
    Offset3D dst_offset;
    Extent3D extent;
};

struct BufferImageCopy {
    size_t buffer_offset;
    size_t buffer_size;
    u32 buffer_row_length;
    u32 buffer_image_height;
    SubresourceLayers image_subresource;
    Offset3D image_offset;
    Extent3D image_extent;
};

struct BufferCopy {
    size_t src_offset;
    size_t dst_offset;
    size_t size;
};

struct SwizzleParameters {
    Extent3D num_tiles;
    Extent3D block;
    size_t buffer_offset;
    s32 level;
};

} // namespace VideoCommon
