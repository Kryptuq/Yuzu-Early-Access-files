// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <mutex>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

#include "common/common_types.h"
#include "common/literals.h"
#include "common/lru_cache.h"
#include "video_core/compatible_formats.h"
#include "video_core/delayed_destruction_ring.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/descriptor_table.h"
#include "video_core/texture_cache/image_base.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/image_view_base.h"
#include "video_core/texture_cache/render_targets.h"
#include "video_core/texture_cache/slot_vector.h"
#include "video_core/texture_cache/types.h"
#include "video_core/textures/texture.h"

namespace VideoCommon {

using Tegra::Texture::SwizzleSource;
using Tegra::Texture::TICEntry;
using Tegra::Texture::TSCEntry;
using VideoCore::Surface::GetFormatType;
using VideoCore::Surface::IsCopyCompatible;
using VideoCore::Surface::PixelFormat;
using VideoCore::Surface::PixelFormatFromDepthFormat;
using VideoCore::Surface::PixelFormatFromRenderTargetFormat;
using namespace Common::Literals;

struct ImageViewInOut {
    u32 index{};
    bool blacklist{};
    ImageViewId id{};
};

template <class P>
class TextureCache {
    /// Address shift for caching images into a hash table
    static constexpr u64 PAGE_BITS = 20;

    static constexpr u64 CPU_PAGE_BITS = 12;
    static constexpr u64 CPU_PAGE_SIZE = 1ULL << CPU_PAGE_BITS;

    /// Enables debugging features to the texture cache
    static constexpr bool ENABLE_VALIDATION = P::ENABLE_VALIDATION;
    /// Implement blits as copies between framebuffers
    static constexpr bool FRAMEBUFFER_BLITS = P::FRAMEBUFFER_BLITS;
    /// True when some copies have to be emulated
    static constexpr bool HAS_EMULATED_COPIES = P::HAS_EMULATED_COPIES;
    /// True when the API can provide info about the memory of the device.
    static constexpr bool HAS_DEVICE_MEMORY_INFO = P::HAS_DEVICE_MEMORY_INFO;

    static constexpr s64 TARGET_THRESHOLD = 4_GiB;
    static constexpr s64 DEFAULT_EXPECTED_MEMORY = 1_GiB + 125_MiB;
    static constexpr s64 DEFAULT_CRITICAL_MEMORY = 1_GiB + 625_MiB;
    static constexpr size_t GC_EMERGENCY_COUNTS = 2;

    using Runtime = typename P::Runtime;
    using Image = typename P::Image;
    using ImageAlloc = typename P::ImageAlloc;
    using ImageView = typename P::ImageView;
    using Sampler = typename P::Sampler;
    using Framebuffer = typename P::Framebuffer;

    struct BlitImages {
        ImageId dst_id;
        ImageId src_id;
        PixelFormat dst_format;
        PixelFormat src_format;
    };

    template <typename T>
    struct IdentityHash {
        [[nodiscard]] size_t operator()(T value) const noexcept {
            return static_cast<size_t>(value);
        }
    };

public:
    explicit TextureCache(Runtime&, VideoCore::RasterizerInterface&, Tegra::Engines::Maxwell3D&,
                          Tegra::Engines::KeplerCompute&, Tegra::MemoryManager&);

    /// Notify the cache that a new frame has been queued
    void TickFrame();

    /// Return a constant reference to the given image view id
    [[nodiscard]] const ImageView& GetImageView(ImageViewId id) const noexcept;

    /// Return a reference to the given image view id
    [[nodiscard]] ImageView& GetImageView(ImageViewId id) noexcept;

    /// Mark an image as modified from the GPU
    void MarkModification(ImageId id) noexcept;

    /// Fill image_view_ids with the graphics images in indices
    template <bool has_blacklists>
    void FillGraphicsImageViews(std::span<ImageViewInOut> views);

    /// Fill image_view_ids with the compute images in indices
    void FillComputeImageViews(std::span<ImageViewInOut> views);

    /// Get the sampler from the graphics descriptor table in the specified index
    Sampler* GetGraphicsSampler(u32 index);

    /// Get the sampler from the compute descriptor table in the specified index
    Sampler* GetComputeSampler(u32 index);

    /// Refresh the state for graphics image view and sampler descriptors
    void SynchronizeGraphicsDescriptors();

    /// Refresh the state for compute image view and sampler descriptors
    void SynchronizeComputeDescriptors();

    /// Updates the Render Targets if they can be rescaled
    /// @param is_clear True when the render targets are being used for clears
    /// @retval True if the Render Targets have been rescaled.
    bool RescaleRenderTargets(bool is_clear);

    /// Update bound render targets and upload memory if necessary
    /// @param is_clear True when the render targets are being used for clears
    void UpdateRenderTargets(bool is_clear);

    /// Find a framebuffer with the currently bound render targets
    /// UpdateRenderTargets should be called before this
    Framebuffer* GetFramebuffer();

    /// Mark images in a range as modified from the CPU
    void WriteMemory(VAddr cpu_addr, size_t size);

    /// Mark images in a range as modified from the CPU
    void CachedWriteMemory(VAddr cpu_addr, size_t size);

    /// Download contents of host images to guest memory in a region
    void DownloadMemory(VAddr cpu_addr, size_t size);

    /// Remove images in a region
    void UnmapMemory(VAddr cpu_addr, size_t size);

    /// Remove images in a region
    void UnmapGPUMemory(GPUVAddr gpu_addr, size_t size);

    void FlushCachedWrites();

    /// Blit an image with the given parameters
    void BlitImage(const Tegra::Engines::Fermi2D::Surface& dst,
                   const Tegra::Engines::Fermi2D::Surface& src,
                   const Tegra::Engines::Fermi2D::Config& copy);

    /// Try to find a cached image view in the given CPU address
    [[nodiscard]] ImageView* TryFindFramebufferImageView(VAddr cpu_addr);

    /// Return true when there are uncommitted images to be downloaded
    [[nodiscard]] bool HasUncommittedFlushes() const noexcept;

    /// Return true when the caller should wait for async downloads
    [[nodiscard]] bool ShouldWaitAsyncFlushes() const noexcept;

    /// Commit asynchronous downloads
    void CommitAsyncFlushes();

    /// Pop asynchronous downloads
    void PopAsyncFlushes();

    /// Return true when a CPU region is modified from the GPU
    [[nodiscard]] bool IsRegionGpuModified(VAddr addr, size_t size);

    [[nodiscard]] bool IsRescaling() const noexcept;

    [[nodiscard]] bool IsRescaling(const ImageViewBase& image_view) const noexcept;

    std::mutex mutex;

private:
    /// Iterate over all page indices in a range
    template <typename Func>
    static void ForEachCPUPage(VAddr addr, size_t size, Func&& func) {
        static constexpr bool RETURNS_BOOL = std::is_same_v<std::invoke_result<Func, u64>, bool>;
        const u64 page_end = (addr + size - 1) >> PAGE_BITS;
        for (u64 page = addr >> PAGE_BITS; page <= page_end; ++page) {
            if constexpr (RETURNS_BOOL) {
                if (func(page)) {
                    break;
                }
            } else {
                func(page);
            }
        }
    }

    template <typename Func>
    static void ForEachGPUPage(GPUVAddr addr, size_t size, Func&& func) {
        static constexpr bool RETURNS_BOOL = std::is_same_v<std::invoke_result<Func, u64>, bool>;
        const u64 page_end = (addr + size - 1) >> PAGE_BITS;
        for (u64 page = addr >> PAGE_BITS; page <= page_end; ++page) {
            if constexpr (RETURNS_BOOL) {
                if (func(page)) {
                    break;
                }
            } else {
                func(page);
            }
        }
    }

    /// Runs the Garbage Collector.
    void RunGarbageCollector();

    /// Fills image_view_ids in the image views in indices
    template <bool has_blacklists>
    void FillImageViews(DescriptorTable<TICEntry>& table,
                        std::span<ImageViewId> cached_image_view_ids,
                        std::span<ImageViewInOut> views);

    /// Find or create an image view in the guest descriptor table
    ImageViewId VisitImageView(DescriptorTable<TICEntry>& table,
                               std::span<ImageViewId> cached_image_view_ids, u32 index);

    /// Find or create a framebuffer with the given render target parameters
    FramebufferId GetFramebufferId(const RenderTargets& key);

    /// Refresh the contents (pixel data) of an image
    void RefreshContents(Image& image, ImageId image_id);

    /// Upload data from guest to an image
    template <typename StagingBuffer>
    void UploadImageContents(Image& image, StagingBuffer& staging_buffer);

    /// Find or create an image view from a guest descriptor
    [[nodiscard]] ImageViewId FindImageView(const TICEntry& config);

    /// Create a new image view from a guest descriptor
    [[nodiscard]] ImageViewId CreateImageView(const TICEntry& config);

    /// Find or create an image from the given parameters
    [[nodiscard]] ImageId FindOrInsertImage(const ImageInfo& info, GPUVAddr gpu_addr,
                                            RelaxedOptions options = RelaxedOptions{});

    /// Find an image from the given parameters
    [[nodiscard]] ImageId FindImage(const ImageInfo& info, GPUVAddr gpu_addr,
                                    RelaxedOptions options);

    /// Create an image from the given parameters
    [[nodiscard]] ImageId InsertImage(const ImageInfo& info, GPUVAddr gpu_addr,
                                      RelaxedOptions options);

    /// Create a new image and join perfectly matching existing images
    /// Remove joined images from the cache
    [[nodiscard]] ImageId JoinImages(const ImageInfo& info, GPUVAddr gpu_addr, VAddr cpu_addr);

    /// Return a blit image pair from the given guest blit parameters
    [[nodiscard]] BlitImages GetBlitImages(const Tegra::Engines::Fermi2D::Surface& dst,
                                           const Tegra::Engines::Fermi2D::Surface& src,
                                           const Tegra::Engines::Fermi2D::Config& copy);

    /// Find or create a sampler from a guest descriptor sampler
    [[nodiscard]] SamplerId FindSampler(const TSCEntry& config);

    /// Find or create an image view for the given color buffer index
    [[nodiscard]] ImageViewId FindColorBuffer(size_t index, bool is_clear);

    /// Find or create an image view for the depth buffer
    [[nodiscard]] ImageViewId FindDepthBuffer(bool is_clear);

    /// Find or create a view for a render target with the given image parameters
    [[nodiscard]] ImageViewId FindRenderTargetView(const ImageInfo& info, GPUVAddr gpu_addr,
                                                   bool is_clear);

    /// Iterates over all the images in a region calling func
    template <typename Func>
    void ForEachImageInRegion(VAddr cpu_addr, size_t size, Func&& func);

    template <typename Func>
    void ForEachImageInRegionGPU(GPUVAddr gpu_addr, size_t size, Func&& func);

    template <typename Func>
    void ForEachSparseImageInRegion(GPUVAddr gpu_addr, size_t size, Func&& func);

    /// Iterates over all the images in a region calling func
    template <typename Func>
    void ForEachSparseSegment(ImageBase& image, Func&& func);

    /// Find or create an image view in the given image with the passed parameters
    [[nodiscard]] ImageViewId FindOrEmplaceImageView(ImageId image_id, const ImageViewInfo& info);

    /// Register image in the page table
    void RegisterImage(ImageId image);

    /// Unregister image from the page table
    void UnregisterImage(ImageId image);

    /// Track CPU reads and writes for image
    void TrackImage(ImageBase& image, ImageId image_id);

    /// Stop tracking CPU reads and writes for image
    void UntrackImage(ImageBase& image, ImageId image_id);

    /// Delete image from the cache
    void DeleteImage(ImageId image, bool immediate_delete = false);

    /// Remove image views references from the cache
    void RemoveImageViewReferences(std::span<const ImageViewId> removed_views);

    /// Remove framebuffers using the given image views from the cache
    void RemoveFramebuffers(std::span<const ImageViewId> removed_views);

    /// Mark an image as modified from the GPU
    void MarkModification(ImageBase& image) noexcept;

    /// Synchronize image aliases, copying data if needed
    void SynchronizeAliases(ImageId image_id);

    /// Prepare an image to be used
    void PrepareImage(ImageId image_id, bool is_modification, bool invalidate);

    /// Prepare an image view to be used
    void PrepareImageView(ImageViewId image_view_id, bool is_modification, bool invalidate);

    /// Execute copies from one image to the other, even if they are incompatible
    void CopyImage(ImageId dst_id, ImageId src_id, std::vector<ImageCopy> copies);

    /// Bind an image view as render target, downloading resources preemtively if needed
    void BindRenderTarget(ImageViewId* old_id, ImageViewId new_id);

    /// Create a render target from a given image and image view parameters
    [[nodiscard]] std::pair<FramebufferId, ImageViewId> RenderTargetFromImage(
        ImageId, const ImageViewInfo& view_info);

    /// Returns true if the current clear parameters clear the whole image of a given image view
    [[nodiscard]] bool IsFullClear(ImageViewId id);

    bool ImageCanRescale(ImageBase& image);
    void InvalidateScale(Image& image);
    bool ScaleUp(Image& image);
    bool ScaleDown(Image& image);
    u64 GetScaledImageSizeBytes(ImageBase& image);

    Runtime& runtime;
    VideoCore::RasterizerInterface& rasterizer;
    Tegra::Engines::Maxwell3D& maxwell3d;
    Tegra::Engines::KeplerCompute& kepler_compute;
    Tegra::MemoryManager& gpu_memory;

    DescriptorTable<TICEntry> graphics_image_table{gpu_memory};
    DescriptorTable<TSCEntry> graphics_sampler_table{gpu_memory};
    std::vector<SamplerId> graphics_sampler_ids;
    std::vector<ImageViewId> graphics_image_view_ids;

    DescriptorTable<TICEntry> compute_image_table{gpu_memory};
    DescriptorTable<TSCEntry> compute_sampler_table{gpu_memory};
    std::vector<SamplerId> compute_sampler_ids;
    std::vector<ImageViewId> compute_image_view_ids;

    RenderTargets render_targets;

    std::unordered_map<TICEntry, ImageViewId> image_views;
    std::unordered_map<TSCEntry, SamplerId> samplers;
    std::unordered_map<RenderTargets, FramebufferId> framebuffers;

    std::unordered_map<u64, std::vector<ImageMapId>, IdentityHash<u64>> page_table;
    std::unordered_map<u64, std::vector<ImageId>, IdentityHash<u64>> gpu_page_table;
    std::unordered_map<u64, std::vector<ImageId>, IdentityHash<u64>> sparse_page_table;

    std::unordered_map<ImageId, std::vector<ImageViewId>> sparse_views;

    std::unordered_set<ImageId> cached_cpu_invalidate;

    VAddr virtual_invalid_space{};

    bool has_deleted_images = false;
    bool is_rescaling = false;
    u64 total_used_memory = 0;
    u64 minimum_memory;
    u64 expected_memory;
    u64 critical_memory;
    size_t critical_gc;

    SlotVector<Image> slot_images;
    SlotVector<ImageMapView> slot_map_views;
    SlotVector<ImageView> slot_image_views;
    SlotVector<ImageAlloc> slot_image_allocs;
    SlotVector<Sampler> slot_samplers;
    SlotVector<Framebuffer> slot_framebuffers;

    // TODO: This data structure is not optimal and it should be reworked
    std::vector<ImageId> uncommitted_downloads;
    std::queue<std::vector<ImageId>> committed_downloads;

    struct LRUItemParams {
        using ObjectType = ImageId;
        using TickType = u64;
    };
    Common::LeastRecentlyUsedCache<LRUItemParams> lru_cache;

    static constexpr size_t TICKS_TO_DESTROY = 6;
    DelayedDestructionRing<Image, TICKS_TO_DESTROY> sentenced_images;
    DelayedDestructionRing<ImageView, TICKS_TO_DESTROY> sentenced_image_view;
    DelayedDestructionRing<Framebuffer, TICKS_TO_DESTROY> sentenced_framebuffers;

    std::unordered_map<GPUVAddr, ImageAllocId> image_allocs_table;

    u64 modification_tick = 0;
    u64 frame_tick = 0;
};

} // namespace VideoCommon
