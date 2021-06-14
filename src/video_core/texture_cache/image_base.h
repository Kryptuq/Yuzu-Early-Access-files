// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <optional>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/image_view_info.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon {

enum class ImageFlagBits : u32 {
    AcceleratedUpload = 1 << 0, ///< Upload can be accelerated in the GPU
    Converted = 1 << 1,   ///< Guest format is not supported natively and it has to be converted
    CpuModified = 1 << 2, ///< Contents have been modified from the CPU
    GpuModified = 1 << 3, ///< Contents have been modified from the GPU
    Tracked = 1 << 4,     ///< Writes and reads are being hooked from the CPU JIT
    Strong = 1 << 5,      ///< Exists in the image table, the dimensions are can be trusted
    Registered = 1 << 6,  ///< True when the image is registered
    Picked = 1 << 7,      ///< Temporary flag to mark the image as picked

    // Garbage Collection Flags
    BadOverlap = 1 << 8, ///< This image overlaps other but doesn't fit, has higher
                         ///< garbage collection priority
    Alias = 1 << 9,      ///< This image has aliases and has priority on garbage
                         ///< collection
};
DECLARE_ENUM_FLAG_OPERATORS(ImageFlagBits)

struct ImageViewInfo;

struct AliasedImage {
    std::vector<ImageCopy> copies;
    ImageId id;
};

struct ImageBase {
    explicit ImageBase(const ImageInfo& info, GPUVAddr gpu_addr, VAddr cpu_addr);

    [[nodiscard]] std::optional<SubresourceBase> TryFindBase(GPUVAddr other_addr) const noexcept;

    [[nodiscard]] ImageViewId FindView(const ImageViewInfo& view_info) const noexcept;

    void InsertView(const ImageViewInfo& view_info, ImageViewId image_view_id);

    [[nodiscard]] bool IsSafeDownload() const noexcept;

    [[nodiscard]] bool Overlaps(VAddr overlap_cpu_addr, size_t overlap_size) const noexcept {
        const VAddr overlap_end = overlap_cpu_addr + overlap_size;
        return cpu_addr < overlap_end && overlap_cpu_addr < cpu_addr_end;
    }

    void CheckBadOverlapState();
    void CheckAliasState();

    ImageInfo info;

    u32 guest_size_bytes = 0;
    u32 unswizzled_size_bytes = 0;
    u32 converted_size_bytes = 0;
    ImageFlagBits flags = ImageFlagBits::CpuModified;

    GPUVAddr gpu_addr = 0;
    VAddr cpu_addr = 0;
    VAddr cpu_addr_end = 0;

    u64 modification_tick = 0;
    u64 frame_tick = 0;

    std::array<u32, MAX_MIP_LEVELS> mip_level_offsets{};

    std::vector<ImageViewInfo> image_view_infos;
    std::vector<ImageViewId> image_view_ids;

    std::vector<u32> slice_offsets;
    std::vector<SubresourceBase> slice_subresources;

    std::vector<AliasedImage> aliased_images;
    std::vector<ImageId> overlapping_images;
};

struct ImageAllocBase {
    std::vector<ImageId> images;
};

void AddImageAlias(ImageBase& lhs, ImageBase& rhs, ImageId lhs_id, ImageId rhs_id);

} // namespace VideoCommon
