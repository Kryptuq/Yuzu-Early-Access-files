// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bitset>
#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/math_util.h"
#include "video_core/engines/const_buffer_info.h"
#include "video_core/engines/engine_interface.h"
#include "video_core/engines/engine_upload.h"
#include "video_core/gpu.h"
#include "video_core/macro/macro.h"
#include "video_core/textures/texture.h"

namespace Core {
class System;
}

namespace Tegra {
class MemoryManager;
}

namespace VideoCore {
class RasterizerInterface;
}

namespace Tegra::Engines {

class DrawManager;

/**
 * This Engine is known as GF100_3D. Documentation can be found in:
 * https://github.com/NVIDIA/open-gpu-doc/blob/master/classes/3d/clb197.h
 * https://github.com/envytools/envytools/blob/master/rnndb/graph/gf100_3d.xml
 * https://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/nouveau/nvc0/nvc0_3d.xml.h
 *
 * Note: nVidia have confirmed that their open docs have had parts redacted, so this list is
 * currently incomplete, and the gaps are still worth exploring.
 */

#define MAXWELL3D_REG_INDEX(field_name) (offsetof(Maxwell3D::Regs, field_name) / sizeof(u32))

class Maxwell3D final : public EngineInterface {
public:
    explicit Maxwell3D(Core::System& system, MemoryManager& memory_manager);
    ~Maxwell3D();

    /// Binds a rasterizer to this engine.
    void BindRasterizer(VideoCore::RasterizerInterface* rasterizer);

    /// Register structure of the Maxwell3D engine.
    struct Regs {
        static constexpr std::size_t NUM_REGS = 0xE00;

        static constexpr std::size_t NumRenderTargets = 8;
        static constexpr std::size_t NumViewports = 16;
        static constexpr std::size_t NumCBData = 16;
        static constexpr std::size_t NumVertexArrays = 32;
        static constexpr std::size_t NumVertexAttributes = 32;
        static constexpr std::size_t NumVaryings = 31;
        static constexpr std::size_t NumImages = 8; // TODO(Rodrigo): Investigate this number
        static constexpr std::size_t NumClipDistances = 8;
        static constexpr std::size_t NumTransformFeedbackBuffers = 4;
        static constexpr std::size_t MaxShaderProgram = 6;
        static constexpr std::size_t MaxShaderStage = 5;
        // Maximum number of const buffers per shader stage.
        static constexpr std::size_t MaxConstBuffers = 18;
        static constexpr std::size_t MaxConstBufferSize = 0x10000;

        struct ID {
            union {
                BitField<0, 16, u32> cls;
                BitField<16, 5, u32> engine;
            };
        };

        struct LoadMME {
            u32 instruction_ptr;
            u32 instruction;
            u32 start_address_ptr;
            u32 start_address;
        };

        struct Notify {
            u32 address_high;
            u32 address_low;
            u32 type;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct PeerSemaphore {
            u32 address_high;
            u32 address_low;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct GlobalRender {
            enum class Mode : u32 {
                False = 0,
                True = 1,
                Conditional = 2,
                IfEqual = 3,
                IfNotEqual = 4,
            };
            u32 offset_high;
            u32 offset_low;
            Mode mode;

            GPUVAddr Address() const {
                return (GPUVAddr{offset_high} << 32) | GPUVAddr{offset_low};
            }
        };

        enum class ReductionOp : u32 {
            RedAdd = 0,
            RedMin = 1,
            RedMax = 2,
            RedInc = 3,
            RedDec = 4,
            RedAnd = 5,
            RedOr = 6,
            RedXor = 7,
        };

        struct LaunchDMA {
            enum class Layout : u32 {
                Blocklinear = 0,
                Pitch = 1,
            };

            enum class CompletionType : u32 {
                FlushDisable = 0,
                FlushOnly = 1,
                Release = 2,
            };

            union {
                BitField<0, 1, Layout> memory_layout;
                BitField<4, 2, CompletionType> completion_type;
                BitField<8, 2, u32> interrupt_type;
                BitField<12, 1, u32> sem_size;
                BitField<1, 1, u32> reduction_enable;
                BitField<13, 3, ReductionOp> reduction_op;
                BitField<2, 2, u32> reduction_format;
                BitField<6, 1, u32> sysmembar_disable;
            };
        };

        struct I2M {
            u32 address_high;
            u32 address_low;
            u32 payload;
            INSERT_PADDING_BYTES_NOINIT(0x8);
            u32 nop0;
            u32 nop1;
            u32 nop2;
            u32 nop3;
        };

        struct OpportunisticEarlyZ {
            BitField<0, 5, u32> threshold;

            u32 Threshold() const {
                switch (threshold) {
                case 0x0:
                    return 0;
                case 0x1F:
                    return 0x1F;
                default:
                    // Thresholds begin at 0x10 (1 << 4)
                    // Threshold is in the range 0x1 to 0x13
                    return 1U << (4 + threshold.Value() - 1);
                }
            }
        };

        struct GeometryShaderDmFifo {
            union {
                BitField<0, 13, u32> raster_on;
                BitField<16, 13, u32> raster_off;
                BitField<31, 1, u32> spill_enabled;
            };
        };

        struct L2CacheControl {
            enum class EvictPolicy : u32 {
                First = 0,
                Normal = 1,
                Last = 2,
            };

            union {
                BitField<4, 2, EvictPolicy> policy;
            };
        };

        struct InvalidateShaderCache {
            union {
                BitField<0, 1, u32> instruction;
                BitField<4, 1, u32> data;
                BitField<12, 1, u32> constant;
                BitField<1, 1, u32> locks;
                BitField<2, 1, u32> flush_data;
            };
        };

        struct SyncInfo {
            enum class Condition : u32 {
                StreamOutWritesDone = 0,
                RopWritesDone = 1,
            };

            union {
                BitField<0, 16, u32> sync_point;
                BitField<16, 1, u32> clean_l2;
                BitField<20, 1, Condition> condition;
            };
        };

        struct SurfaceClipBlockId {
            union {
                BitField<0, 4, u32> block_width;
                BitField<4, 4, u32> block_height;
                BitField<8, 4, u32> block_depth;
            };
        };

        struct DecompressSurface {
            union {
                BitField<0, 3, u32> mrt_select;
                BitField<4, 16, u32> rt_array_index;
            };
        };

        struct ZCullRopBypass {
            union {
                BitField<0, 1, u32> enable;
                BitField<4, 1, u32> no_stall;
                BitField<8, 1, u32> cull_everything;
                BitField<12, 4, u32> threshold;
            };
        };

        struct ZCullSubregion {
            union {
                BitField<0, 1, u32> enable;
                BitField<4, 24, u32> normalized_aliquots;
            };
        };

        struct RasterBoundingBox {
            enum class Mode : u32 {
                BoundingBox = 0,
                FullViewport = 1,
            };

            union {
                BitField<0, 1, Mode> mode;
                BitField<4, 8, u32> pad;
            };
        };

        struct IteratedBlendOptimization {
            enum class Noop : u32 {
                Never = 0,
                SourceRGBA0000 = 1,
                SourceAlpha = 2,
                SourceRGBA0001 = 3,
            };

            union {
                BitField<0, 1, Noop> noop;
            };
        };

        struct ZCullSubregionAllocation {
            enum class Format : u32 {
                Z_16x16x2_4x4 = 0,
                ZS_16x16_4x4 = 1,
                Z_16x16_4x2 = 2,
                Z_16x16_2x4 = 3,
                Z_16x8_4x4 = 4,
                Z_8x8_4x2 = 5,
                Z_8x8_2x4 = 6,
                Z_16x16_4x8 = 7,
                Z_4x8_2x2 = 8,
                ZS_16x8_4x2 = 9,
                ZS_16x8_2x4 = 10,
                ZS_8x8_2x2 = 11,
                Z_4x8_1x1 = 12,
                None = 15,
            };

            union {
                BitField<0, 8, u32> id;
                BitField<8, 16, u32> aliquots;
                BitField<24, 4, Format> format;
            };
        };

        enum class ZCullSubregionAlgorithm : u32 {
            Static = 0,
            Adaptive = 1,
        };

        struct PixelShaderOutputSampleMaskUsage {
            union {
                BitField<0, 1, u32> enable;
                BitField<1, 1, u32> qualify_by_aa;
            };
        };

        struct L1Configuration {
            enum class AddressableMemory : u32 {
                Size16Kb = 0,
                Size48Kb = 3,
            };
            union {
                BitField<0, 3, AddressableMemory> direct_addressable_memory;
            };
        };

        struct SPAVersion {
            union {
                BitField<0, 8, u32> minor;
                BitField<8, 8, u32> major;
            };
        };

        struct SnapGrid {
            enum class Location : u32 {
                Pixel2x2 = 1,
                Pixel4x4 = 2,
                Pixel8x8 = 3,
                Pixel16x16 = 4,
                Pixel32x32 = 5,
                Pixel64x64 = 6,
                Pixel128x128 = 7,
                Pixel256x256 = 8,
            };

            enum class Mode : u32 {
                RTNE = 0,
                Tesla = 1,
            };

            struct {
                union {
                    BitField<0, 4, Location> location;
                    BitField<8, 1, Mode> rounding_mode;
                };
            } line;

            struct {
                union {
                    BitField<0, 4, Location> location;
                    BitField<8, 1, Mode> rounding_mode;
                };
            } non_line;
        };

        struct Tessellation {
            enum class DomainType : u32 {
                Isolines = 0,
                Triangles = 1,
                Quads = 2,
            };

            enum class Spacing : u32 {
                Integer = 0,
                FractionalOdd = 1,
                FractionalEven = 2,
            };

            enum class OutputPrimitives : u32 {
                Points = 0,
                Lines = 1,
                Triangles_CW = 2,
                Triangles_CCW = 3,
            };

            struct Parameters {
                union {
                    BitField<0, 2, DomainType> domain_type;
                    BitField<4, 2, Spacing> spacing;
                    BitField<8, 2, OutputPrimitives> output_primitives;
                };
            } params;

            struct LOD {
                std::array<f32, 4> outer;
                std::array<f32, 2> inner;
            } lod;

            std::array<u32, 9> reserved;
        };

        struct SubTilingPerf {
            struct {
                union {
                    BitField<0, 8, u32> spm_triangle_register_file_per;
                    BitField<8, 8, u32> spm_pixel_output_buffer_per;
                    BitField<16, 8, u32> spm_triangle_ram_per;
                    BitField<24, 8, u32> max_quads_per;
                };
            } knob_a;

            struct {
                union {
                    BitField<0, 8, u32> max_primitives_per;
                };
            } knob_b;

            u32 knob_c;
        };

        struct ZCullSubregionReport {
            enum class ReportType : u32 {
                DepthTest = 0,
                DepthTestNoAccept = 1,
                DepthTestLateZ = 2,
                StencilTest = 3,
            };

            union {
                BitField<0, 1, u32> enabled;
                BitField<4, 8, u32> subregion_id;
            } to_report;

            union {
                BitField<0, 1, u32> enabled;
                BitField<4, 3, ReportType> type;
            } report_type;
        };

        struct BalancedPrimitiveWorkload {
            union {
                BitField<0, 1, u32> unpartitioned_mode;
                BitField<4, 1, u32> timesliced_mode;
            };
        };

        struct TransformFeedback {
            struct Buffer {
                u32 enable;
                u32 address_high;
                u32 address_low;
                s32 size;
                s32 start_offset;
                INSERT_PADDING_BYTES_NOINIT(0xC);

                GPUVAddr Address() const {
                    return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
                }
            };
            static_assert(sizeof(Buffer) == 0x20);

            struct Control {
                u32 stream;
                u32 varying_count;
                u32 stride;
                INSERT_PADDING_BYTES_NOINIT(0x4);
            };
            static_assert(sizeof(Control) == 0x10);

            std::array<TransformFeedback::Buffer, NumTransformFeedbackBuffers> buffers;

            INSERT_PADDING_BYTES_NOINIT(0x300);

            std::array<TransformFeedback::Control, NumTransformFeedbackBuffers> controls;
        };

        struct HybridAntiAliasControl {
            enum class Centroid : u32 {
                PerFragment = 0,
                PerPass = 1,
            };
            union {
                BitField<0, 4, u32> passes;
                BitField<4, 1, Centroid> centroid;
                BitField<5, 1, u32> passes_extended;
            };
        };

        struct ShaderLocalMemory {
            u32 base_address;
            INSERT_PADDING_BYTES_NOINIT(0x10);
            u32 address_high;
            u32 address_low;
            u32 size_high;
            u32 size_low;
            u32 default_size_per_warp;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }

            u64 Size() const {
                return (u64{size_high} << 32) | u64{size_low};
            }
        };

        struct ZCullRegion {
            u32 width;
            u32 height;
            u32 depth;
            u32 offset;
            INSERT_PADDING_BYTES_NOINIT(0xC);
            u32 fetch_streams_once;
            union {
                BitField<0, 16, u32> start_aliquot;
                BitField<16, 16, u32> aliquot_count;
            } location;
            u32 aliquots_per_layer;
            u32 storage_address_high;
            u32 storage_address_low;
            u32 storage_limit_address_high;
            u32 storage_limit_address_low;

            GPUVAddr StorageAddress() const {
                return (GPUVAddr{storage_address_high} << 32) | GPUVAddr{storage_address_low};
            }
            GPUVAddr StorageLimitAddress() const {
                return (GPUVAddr{storage_limit_address_high} << 32) |
                       GPUVAddr{storage_limit_address_low};
            }
        };

        struct ZetaReadOnly {
            union {
                BitField<0, 1, u32> enable_z;
                BitField<4, 1, u32> enable_stencil;
            };
        };

        struct VertexAttribute {
            enum class Size : u32 {
                Invalid = 0x0,
                Size_R32_G32_B32_A32 = 0x01,
                Size_R32_G32_B32 = 0x02,
                Size_R16_G16_B16_A16 = 0x03,
                Size_R32_G32 = 0x04,
                Size_R16_G16_B16 = 0x05,
                Size_R8_G8_B8_A8 = 0x0A,
                Size_R16_G16 = 0x0F,
                Size_R32 = 0x12,
                Size_R8_G8_B8 = 0x13,
                Size_R8_G8 = 0x18,
                Size_R16 = 0x1B,
                Size_R8 = 0x1D,
                Size_A2_B10_G10_R10 = 0x30,
                Size_B10_G11_R11 = 0x31,
                Size_G8_R8 = 0x32,
                Size_X8_B8_G8_R8 = 0x33,
                Size_A8 = 0x34,
            };

            enum class Type : u32 {
                UnusedEnumDoNotUseBecauseItWillGoAway = 0,
                SNorm = 1,
                UNorm = 2,
                SInt = 3,
                UInt = 4,
                UScaled = 5,
                SScaled = 6,
                Float = 7,
            };

            union {
                BitField<0, 5, u32> buffer;
                BitField<6, 1, u32> constant;
                BitField<7, 14, u32> offset;
                BitField<21, 6, Size> size;
                BitField<27, 3, Type> type;
                BitField<31, 1, u32> bgra;
                u32 hex;
            };

            u32 ComponentCount() const {
                switch (size) {
                case Size::Size_R32_G32_B32_A32:
                    return 4;
                case Size::Size_R32_G32_B32:
                    return 3;
                case Size::Size_R16_G16_B16_A16:
                    return 4;
                case Size::Size_R32_G32:
                    return 2;
                case Size::Size_R16_G16_B16:
                    return 3;
                case Size::Size_R8_G8_B8_A8:
                case Size::Size_X8_B8_G8_R8:
                    return 4;
                case Size::Size_R16_G16:
                    return 2;
                case Size::Size_R32:
                    return 1;
                case Size::Size_R8_G8_B8:
                    return 3;
                case Size::Size_R8_G8:
                case Size::Size_G8_R8:
                    return 2;
                case Size::Size_R16:
                    return 1;
                case Size::Size_R8:
                case Size::Size_A8:
                    return 1;
                case Size::Size_A2_B10_G10_R10:
                    return 4;
                case Size::Size_B10_G11_R11:
                    return 3;
                default:
                    ASSERT(false);
                    return 1;
                }
            }

            u32 SizeInBytes() const {
                switch (size) {
                case Size::Size_R32_G32_B32_A32:
                    return 16;
                case Size::Size_R32_G32_B32:
                    return 12;
                case Size::Size_R16_G16_B16_A16:
                    return 8;
                case Size::Size_R32_G32:
                    return 8;
                case Size::Size_R16_G16_B16:
                    return 6;
                case Size::Size_R8_G8_B8_A8:
                case Size::Size_X8_B8_G8_R8:
                    return 4;
                case Size::Size_R16_G16:
                    return 4;
                case Size::Size_R32:
                    return 4;
                case Size::Size_R8_G8_B8:
                    return 3;
                case Size::Size_R8_G8:
                case Size::Size_G8_R8:
                    return 2;
                case Size::Size_R16:
                    return 2;
                case Size::Size_R8:
                case Size::Size_A8:
                    return 1;
                case Size::Size_A2_B10_G10_R10:
                    return 4;
                case Size::Size_B10_G11_R11:
                    return 4;
                default:
                    ASSERT(false);
                    return 1;
                }
            }

            std::string SizeString() const {
                switch (size) {
                case Size::Size_R32_G32_B32_A32:
                    return "32_32_32_32";
                case Size::Size_R32_G32_B32:
                    return "32_32_32";
                case Size::Size_R16_G16_B16_A16:
                    return "16_16_16_16";
                case Size::Size_R32_G32:
                    return "32_32";
                case Size::Size_R16_G16_B16:
                    return "16_16_16";
                case Size::Size_R8_G8_B8_A8:
                    return "8_8_8_8";
                case Size::Size_R16_G16:
                    return "16_16";
                case Size::Size_R32:
                    return "32";
                case Size::Size_R8_G8_B8:
                    return "8_8_8";
                case Size::Size_R8_G8:
                case Size::Size_G8_R8:
                    return "8_8";
                case Size::Size_R16:
                    return "16";
                case Size::Size_R8:
                case Size::Size_A8:
                    return "8";
                case Size::Size_A2_B10_G10_R10:
                    return "2_10_10_10";
                case Size::Size_B10_G11_R11:
                    return "10_11_11";
                default:
                    ASSERT(false);
                    return {};
                }
            }

            std::string TypeString() const {
                switch (type) {
                case Type::UnusedEnumDoNotUseBecauseItWillGoAway:
                    return "Unused";
                case Type::SNorm:
                    return "SNORM";
                case Type::UNorm:
                    return "UNORM";
                case Type::SInt:
                    return "SINT";
                case Type::UInt:
                    return "UINT";
                case Type::UScaled:
                    return "USCALED";
                case Type::SScaled:
                    return "SSCALED";
                case Type::Float:
                    return "FLOAT";
                }
                ASSERT(false);
                return {};
            }

            bool IsNormalized() const {
                return (type == Type::SNorm) || (type == Type::UNorm);
            }

            bool IsValid() const {
                return size != Size::Invalid;
            }

            bool operator<(const VertexAttribute& other) const {
                return hex < other.hex;
            }
        };
        static_assert(sizeof(VertexAttribute) == 0x4);

        struct MsaaSampleLocation {
            union {
                BitField<0, 4, u32> x0;
                BitField<4, 4, u32> y0;
                BitField<8, 4, u32> x1;
                BitField<12, 4, u32> y1;
                BitField<16, 4, u32> x2;
                BitField<20, 4, u32> y2;
                BitField<24, 4, u32> x3;
                BitField<28, 4, u32> y3;
            };

            constexpr std::pair<u32, u32> Location(int index) const {
                switch (index) {
                case 0:
                    return {x0, y0};
                case 1:
                    return {x1, y1};
                case 2:
                    return {x2, y2};
                case 3:
                    return {x3, y3};
                default:
                    ASSERT(false);
                    return {0, 0};
                }
            }
        };

        struct MultisampleCoverageToColor {
            union {
                BitField<0, 1, u32> enable;
                BitField<4, 3, u32> target;
            };
        };

        struct DecompressZetaSurface {
            union {
                BitField<0, 1, u32> z_enable;
                BitField<4, 1, u32> stencil_enable;
            };
        };

        struct ZetaSparse {
            enum class UnmappedCompare : u32 {
                Unmapped = 0,
                FailAlways = 1,
            };
            union {
                BitField<0, 1, u32> enable;
                BitField<1, 1, UnmappedCompare> unmapped_compare;
            };
        };

        struct RtControl {
            union {
                BitField<0, 4, u32> count;
                BitField<4, 3, u32> target0;
                BitField<7, 3, u32> target1;
                BitField<10, 3, u32> target2;
                BitField<13, 3, u32> target3;
                BitField<16, 3, u32> target4;
                BitField<19, 3, u32> target5;
                BitField<22, 3, u32> target6;
                BitField<25, 3, u32> target7;
            };

            u32 Map(std::size_t index) const {
                const std::array<u32, NumRenderTargets> maps{target0, target1, target2, target3,
                                                             target4, target5, target6, target7};
                ASSERT(index < maps.size());
                return maps[index];
            }
        };

        struct CompressionThresholdSamples {
            u32 samples;

            u32 Samples() const {
                if (samples == 0) {
                    return 0;
                }
                return 1U << (samples - 1);
            }
        };

        struct PixelShaderInterlockControl {
            enum class TileMode : u32 {
                NoConflictDetect = 0,
                DetectSampleConflict = 1,
                DetectPixelConflict = 2,
            };
            enum class TileSize : u32 {
                Size_16x16 = 0,
                Size_8x8 = 1,
            };
            enum class FragmentOrder : u32 {
                FragmentOrdered = 0,
                FragmentUnordered = 1,
            };
            union {
                BitField<0, 2, TileMode> tile_mode;
                BitField<2, 1, TileSize> tile_size;
                BitField<3, 1, FragmentOrder> fragment_order;
            };
        };

        struct ZetaSize {
            enum class DimensionControl : u32 {
                DepthDefinesArray = 0,
                ArraySizeOne = 1,
            };

            u32 width;
            u32 height;
            union {
                BitField<0, 16, u32> depth;
                BitField<16, 1, DimensionControl> dim_control;
            };
        };

        enum class PrimitiveTopology : u32 {
            Points = 0x0,
            Lines = 0x1,
            LineLoop = 0x2,
            LineStrip = 0x3,
            Triangles = 0x4,
            TriangleStrip = 0x5,
            TriangleFan = 0x6,
            Quads = 0x7,
            QuadStrip = 0x8,
            Polygon = 0x9,
            LinesAdjacency = 0xA,
            LineStripAdjacency = 0xB,
            TrianglesAdjacency = 0xC,
            TriangleStripAdjacency = 0xD,
            Patches = 0xE,
        };

        struct VertexArray {
            union {
                BitField<0, 16, u32> start;
                BitField<16, 12, u32> count;
                BitField<28, 3, PrimitiveTopology> topology;
            };
        };

        enum class PrimitiveTopologyOverride : u32 {
            None = 0x0,
            Points = 0x1,
            Lines = 0x2,
            LineStrip = 0x3,
            Triangles = 0x4,
            TriangleStrip = 0x5,
            LinesAdjacency = 0xA,
            LineStripAdjacency = 0xB,
            TrianglesAdjacency = 0xC,
            TriangleStripAdjacency = 0xD,
            Patches = 0xE,

            LegacyPoints = 0x1001,
            LegacyIndexedLines = 0x1002,
            LegacyIndexedTriangles = 0x1003,
            LegacyLines = 0x100F,
            LegacyLineStrip = 0x1010,
            LegacyIndexedLineStrip = 0x1011,
            LegacyTriangles = 0x1012,
            LegacyTriangleStrip = 0x1013,
            LegacyIndexedTriangleStrip = 0x1014,
            LegacyTriangleFan = 0x1015,
            LegacyIndexedTriangleFan = 0x1016,
            LegacyTriangleFanImm = 0x1017,
            LegacyLinesImm = 0x1018,
            LegacyIndexedTriangles2 = 0x101A,
            LegacyIndexedLines2 = 0x101B,
        };

        enum class DepthMode : u32 {
            MinusOneToOne = 0,
            ZeroToOne = 1,
        };

        enum class IndexFormat : u32 {
            UnsignedByte = 0x0,
            UnsignedShort = 0x1,
            UnsignedInt = 0x2,
        };

        enum class ComparisonOp : u32 {
            Never_D3D = 1,
            Less_D3D = 2,
            Equal_D3D = 3,
            LessEqual_D3D = 4,
            Greater_D3D = 5,
            NotEqual_D3D = 6,
            GreaterEqual_D3D = 7,
            Always_D3D = 8,

            Never_GL = 0x200,
            Less_GL = 0x201,
            Equal_GL = 0x202,
            LessEqual_GL = 0x203,
            Greater_GL = 0x204,
            NotEqual_GL = 0x205,
            GreaterEqual_GL = 0x206,
            Always_GL = 0x207,
        };

        enum class ClearReport : u32 {
            ZPassPixelCount = 0x01,
            ZCullStats = 0x02,
            StreamingPrimitvesNeededMinusSucceeded = 0x03,
            AlphaBetaClocks = 0x04,
            StreamingPrimitivesSucceeded = 0x10,
            StreamingPrimitivesNeeded = 0x11,
            VerticesGenerated = 0x12,
            PrimitivesGenerated = 0x13,
            VertexShaderInvocations = 0x15,
            TessellationInitInvocations = 0x16,
            TessellationShaderInvocations = 0x17,
            TessellationShaderPrimitivesGenerated = 0x18,
            GeometryShaderInvocations = 0x1A,
            GeometryShaderPrimitivesGenerated = 0x1B,
            ClipperInvocations = 0x1C,
            ClipperPrimitivesGenerated = 0x1D,
            PixelShaderInvocations = 0x1E,
            VtgPrimitivesOut = 0x1F,
        };

        enum class FrontFace : u32 {
            ClockWise = 0x900,
            CounterClockWise = 0x901,
        };

        enum class CullFace : u32 {
            Front = 0x404,
            Back = 0x405,
            FrontAndBack = 0x408,
        };

        struct Blend {
            enum class Equation : u32 {
                Add_D3D = 1,
                Subtract_D3D = 2,
                ReverseSubtract_D3D = 3,
                Min_D3D = 4,
                Max_D3D = 5,

                Add_GL = 0x8006,
                Min_GL = 0x8007,
                Max_GL = 0x8008,
                Subtract_GL = 0x800A,
                ReverseSubtract_GL = 0x800B
            };

            enum class Factor : u32 {
                Zero_D3D = 0x1,
                One_D3D = 0x2,
                SourceColor_D3D = 0x3,
                OneMinusSourceColor_D3D = 0x4,
                SourceAlpha_D3D = 0x5,
                OneMinusSourceAlpha_D3D = 0x6,
                DestAlpha_D3D = 0x7,
                OneMinusDestAlpha_D3D = 0x8,
                DestColor_D3D = 0x9,
                OneMinusDestColor_D3D = 0xA,
                SourceAlphaSaturate_D3D = 0xB,
                BothSourceAlpha_D3D = 0xC,
                OneMinusBothSourceAlpha_D3D = 0xD,
                BlendFactor_D3D = 0xE,
                OneMinusBlendFactor_D3D = 0xF,
                Source1Color_D3D = 0x10,
                OneMinusSource1Color_D3D = 0x11,
                Source1Alpha_D3D = 0x12,
                OneMinusSource1Alpha_D3D = 0x13,

                Zero_GL = 0x4000,
                One_GL = 0x4001,
                SourceColor_GL = 0x4300,
                OneMinusSourceColor_GL = 0x4301,
                SourceAlpha_GL = 0x4302,
                OneMinusSourceAlpha_GL = 0x4303,
                DestAlpha_GL = 0x4304,
                OneMinusDestAlpha_GL = 0x4305,
                DestColor_GL = 0x4306,
                OneMinusDestColor_GL = 0x4307,
                SourceAlphaSaturate_GL = 0x4308,
                ConstantColor_GL = 0xC001,
                OneMinusConstantColor_GL = 0xC002,
                ConstantAlpha_GL = 0xC003,
                OneMinusConstantAlpha_GL = 0xC004,
                Source1Color_GL = 0xC900,
                OneMinusSource1Color_GL = 0xC901,
                Source1Alpha_GL = 0xC902,
                OneMinusSource1Alpha_GL = 0xC903,
            };

            u32 separate_alpha;
            Equation color_op;
            Factor color_source;
            Factor color_dest;
            Equation alpha_op;
            Factor alpha_source;
            u32 enable_global_color_key;
            Factor alpha_dest;

            u32 single_rop_control_enable;
            u32 enable[NumRenderTargets];
        };

        struct BlendPerTarget {
            u32 separate_alpha;
            Blend::Equation color_op;
            Blend::Factor color_source;
            Blend::Factor color_dest;
            Blend::Equation alpha_op;
            Blend::Factor alpha_source;
            Blend::Factor alpha_dest;
            INSERT_PADDING_BYTES_NOINIT(0x4);
        };
        static_assert(sizeof(BlendPerTarget) == 0x20);

        enum class PolygonMode : u32 {
            Point = 0x1B00,
            Line = 0x1B01,
            Fill = 0x1B02,
        };

        enum class ShadowRamControl : u32 {
            // write value to shadow ram
            Track = 0,
            // write value to shadow ram ( with validation ??? )
            TrackWithFilter = 1,
            // only write to real hw register
            Passthrough = 2,
            // write value from shadow ram to real hw register
            Replay = 3,
        };

        enum class ViewportSwizzle : u32 {
            PositiveX = 0,
            NegativeX = 1,
            PositiveY = 2,
            NegativeY = 3,
            PositiveZ = 4,
            NegativeZ = 5,
            PositiveW = 6,
            NegativeW = 7,
        };

        enum class SamplerBinding : u32 {
            Independently = 0,
            ViaHeaderBinding = 1,
        };

        struct TileMode {
            enum class DimensionControl : u32 {
                DepthDefinesArray = 0,
                DepthDefinesDepth = 1,
            };
            union {
                BitField<0, 4, u32> block_width;
                BitField<4, 4, u32> block_height;
                BitField<8, 4, u32> block_depth;
                BitField<12, 1, u32> is_pitch_linear;
                BitField<16, 1, DimensionControl> dim_control;
            };
        };
        static_assert(sizeof(TileMode) == 4);

        struct RenderTargetConfig {
            u32 address_high;
            u32 address_low;
            u32 width;
            u32 height;
            Tegra::RenderTargetFormat format;
            TileMode tile_mode;
            union {
                BitField<0, 16, u32> depth;
                BitField<16, 1, u32> volume;
            };
            u32 array_pitch;
            u32 base_layer;
            u32 mark_ieee_clean;
            INSERT_PADDING_BYTES_NOINIT(0x18);

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };
        static_assert(sizeof(RenderTargetConfig) == 0x40);

        struct ColorMask {
            union {
                u32 raw;
                BitField<0, 1, u32> R;
                BitField<4, 1, u32> G;
                BitField<8, 1, u32> B;
                BitField<12, 1, u32> A;
            };
        };

        struct ViewportTransform {
            f32 scale_x;
            f32 scale_y;
            f32 scale_z;
            f32 translate_x;
            f32 translate_y;
            f32 translate_z;

            union {
                u32 raw;
                BitField<0, 3, ViewportSwizzle> x;
                BitField<4, 3, ViewportSwizzle> y;
                BitField<8, 3, ViewportSwizzle> z;
                BitField<12, 3, ViewportSwizzle> w;
            } swizzle;

            union {
                BitField<0, 5, u32> x;
                BitField<8, 5, u32> y;
            } snap_grid_precision;

            Common::Rectangle<f32> GetRect() const {
                return {
                    GetX(),               // left
                    GetY() + GetHeight(), // top
                    GetX() + GetWidth(),  // right
                    GetY()                // bottom
                };
            }

            f32 GetX() const {
                return std::max(0.0f, translate_x - std::fabs(scale_x));
            }

            f32 GetY() const {
                return std::max(0.0f, translate_y - std::fabs(scale_y));
            }

            f32 GetWidth() const {
                return translate_x + std::fabs(scale_x) - GetX();
            }

            f32 GetHeight() const {
                return translate_y + std::fabs(scale_y) - GetY();
            }
        };
        static_assert(sizeof(ViewportTransform) == 0x20);

        struct Viewport {
            enum class PixelCenter : u32 {
                HalfIntegers = 0,
                Integers = 1,
            };

            union {
                BitField<0, 16, u32> x;
                BitField<16, 16, u32> width;
            };
            union {
                BitField<0, 16, u32> y;
                BitField<16, 16, u32> height;
            };
            float depth_range_near;
            float depth_range_far;
        };
        static_assert(sizeof(Viewport) == 0x10);

        struct Window {
            union {
                BitField<0, 16, u32> x_min;
                BitField<16, 16, u32> x_max;
            };
            union {
                BitField<0, 16, u32> y_min;
                BitField<16, 16, u32> y_max;
            };
        };
        static_assert(sizeof(Window) == 0x8);

        struct ClipIdExtent {
            union {
                BitField<0, 16, u32> x;
                BitField<16, 16, u32> width;
            };
            union {
                BitField<0, 16, u32> y;
                BitField<16, 16, u32> height;
            };
        };
        static_assert(sizeof(ClipIdExtent) == 0x8);

        enum class VisibleCallLimit : u32 {
            Limit0 = 0,
            Limit1 = 1,
            Limit2 = 2,
            Limit4 = 3,
            Limit8 = 4,
            Limit16 = 5,
            Limit32 = 6,
            Limit64 = 7,
            Limit128 = 8,
            None = 15,
        };

        struct StatisticsCounter {
            union {
                BitField<0, 1, u32> da_vertices;
                BitField<1, 1, u32> da_primitives;
                BitField<2, 1, u32> vs_invocations;
                BitField<3, 1, u32> gs_invocations;
                BitField<4, 1, u32> gs_primitives;
                BitField<5, 1, u32> streaming_primitives_succeeded;
                BitField<6, 1, u32> streaming_primitives_needed;
                BitField<7, 1, u32> clipper_invocations;
                BitField<8, 1, u32> clipper_primitives;
                BitField<9, 1, u32> ps_invocations;
                BitField<11, 1, u32> ti_invocations;
                BitField<12, 1, u32> ts_invocations;
                BitField<13, 1, u32> ts_primitives;
                BitField<14, 1, u32> total_streaming_primitives_needed_succeeded;
                BitField<10, 1, u32> vtg_primitives_out;
                BitField<15, 1, u32> alpha_beta_clocks;
            };
        };

        struct ClearRect {
            union {
                BitField<0, 16, u32> x_min;
                BitField<16, 16, u32> x_max;
            };
            union {
                BitField<0, 16, u32> y_min;
                BitField<16, 16, u32> y_max;
            };
        };

        struct VertexBuffer {
            u32 first;
            u32 count;
        };

        struct InvalidateShaderCacheNoWFI {
            union {
                BitField<0, 1, u32> instruction;
                BitField<4, 1, u32> global_data;
                BitField<12, 1, u32> constant;
            };
        };

        struct ZCullSerialization {
            enum class Applied : u32 {
                Always = 0,
                LateZ = 1,
                OutOfGamutZ = 2,
                LateZOrOutOfGamutZ = 3,
            };
            union {
                BitField<0, 1, u32> enable;
                BitField<4, 2, Applied> applied;
            };
        };

        struct ZCullDirFormat {
            enum class Zdir : u32 {
                Less = 0,
                Greater = 1,
            };
            enum class Zformat : u32 {
                MSB = 0,
                FP = 1,
                Ztrick = 2,
                Zf32 = 3,
            };

            union {
                BitField<0, 16, Zdir> dir;
                BitField<16, 16, Zformat> format;
            };
        };

        struct IteratedBlend {
            union {
                BitField<0, 1, u32> enable;
                BitField<1, 1, u32> enable_alpha;
            };
            u32 pass_count;
        };

        struct ZCullCriterion {
            enum class Sfunc : u32 {
                Never = 0,
                Less = 1,
                Equal = 2,
                LessOrEqual = 3,
                Greater = 4,
                NotEqual = 5,
                GreaterOrEqual = 6,
                Always = 7,
            };

            union {
                BitField<0, 8, Sfunc> sfunc;
                BitField<8, 1, u32> no_invalidate;
                BitField<9, 1, u32> force_match;
                BitField<16, 8, u32> sref;
                BitField<24, 8, u32> smask;
            };
        };

        struct LoadIteratedBlend {
            enum class Test : u32 {
                False = 0,
                True = 1,
                Equal = 2,
                NotEqual = 3,
                LessThan = 4,
                LessOrEqual = 5,
                Greater = 6,
                GreaterOrEqual = 7,
            };
            enum class Operation : u32 {
                AddProducts = 0,
                SubProducts = 1,
                Min = 2,
                Max = 3,
                Reciprocal = 4,
                Add = 5,
                Sub = 6,
            };
            enum class OperandA : u32 {
                SrcRGB = 0,
                DstRGB = 1,
                SrcAAA = 2,
                DstAAA = 3,
                Temp0_RGB = 4,
                Temp1_RGB = 5,
                Temp2_RGB = 6,
                PBR_RGB = 7,
            };
            enum class OperandB : u32 {
                Zero = 0,
                One = 1,
                SrcRGB = 2,
                SrcAAA = 3,
                OneMinusSrcAAA = 4,
                DstRGB = 5,
                DstAAA = 6,
                OneMinusDstAAA = 7,
                Temp0_RGB = 9,
                Temp1_RGB = 10,
                Temp2_RGB = 11,
                PBR_RGB = 12,
                ConstRGB = 13,
                ZeroATimesB = 14,
            };
            enum class Swizzle : u32 {
                RGB = 0,
                GBR = 1,
                RRR = 2,
                GGG = 3,
                BBB = 4,
                RToA = 5,
            };
            enum class WriteMask : u32 {
                RGB = 0,
                ROnly = 1,
                GOnly = 2,
                BOnly = 3,
            };
            enum class Pass : u32 {
                Temp0 = 0,
                Temp1 = 1,
                Temp2 = 2,
                None = 3,
            };

            u32 instruction_ptr;
            union {
                BitField<0, 3, Test> test;
                BitField<3, 3, Operation> operation;
                BitField<6, 3, u32> const_input;
                BitField<9, 3, OperandA> operand_a;
                BitField<12, 4, OperandB> operand_b;
                BitField<16, 3, OperandA> operand_c;
                BitField<19, 4, OperandB> operand_d;
                BitField<23, 3, Swizzle> output_swizzle;
                BitField<26, 2, WriteMask> output_mask;
                BitField<28, 2, Pass> output_pass;
                BitField<31, 1, u32> test_enabled;
            };
        };

        struct ScissorTest {
            u32 enable;
            union {
                BitField<0, 16, u32> min_x;
                BitField<16, 16, u32> max_x;
            };
            union {
                BitField<0, 16, u32> min_y;
                BitField<16, 16, u32> max_y;
            };
            INSERT_PADDING_BYTES_NOINIT(0x4);
        };
        static_assert(sizeof(ScissorTest) == 0x10);

        struct VPCPerf {
            union {
                BitField<0, 8, u32> culled_small_lines;
                BitField<8, 8, u32> culled_small_triangles;
                BitField<16, 8, u32> nonculled_lines_and_points;
                BitField<24, 8, u32> nonculled_triangles;
            };
        };

        struct ConstantColorRendering {
            u32 enabled;
            u32 red;
            u32 green;
            u32 blue;
            u32 alpha;
        };

        struct VertexStreamSubstitute {
            u32 address_high;
            u32 address_low;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct VTGWarpWatermarks {
            union {
                BitField<0, 16, u32> low;
                BitField<16, 16, u32> high;
            };
        };

        struct SampleMask {
            struct Target {
                union {
                    BitField<0, 1, u32> raster_out;
                    BitField<4, 1, u32> color_target;
                };
                u32 target;
            };
            struct Pos {
                u32 x0_y0;
                u32 x1_y0;
                u32 x0_y1;
                u32 x1_y1;
            };
        };

        enum class NonMultisampledZ : u32 {
            PerSample = 0,
            PixelCenter = 1,
        };

        enum class TIRMode : u32 {
            Disabled = 0,
            RasterNTargetM = 1,
        };

        enum class AntiAliasRaster : u32 {
            Mode1x1 = 0,
            Mode2x2 = 2,
            Mode4x2_D3D = 4,
            Mode2x1_D3D = 5,
            Mode4x4 = 6,
        };

        struct SurfaceClipIDMemory {
            u32 address_high;
            u32 address_low;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct TIRModulation {
            enum class Component : u32 {
                None = 0,
                RGB = 1,
                AlphaOnly = 2,
                RGBA = 3,
            };
            enum class Function : u32 {
                Linear = 0,
                Table = 1,
            };
            Component component;
            Function function;
        };

        struct Zeta {
            u32 address_high;
            u32 address_low;
            Tegra::DepthFormat format;
            TileMode tile_mode;
            u32 array_pitch;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct SurfaceClip {
            union {
                BitField<0, 16, u32> x;
                BitField<16, 16, u32> width;
            };
            union {
                BitField<0, 16, u32> y;
                BitField<16, 16, u32> height;
            };
        };

        enum class L2CacheControlPolicy : u32 {
            First = 0,
            Normal = 1,
            Last = 2,
        };

        struct L2CacheVAFRequests {
            union {
                BitField<0, 1, u32> system_memory_volatile;
                BitField<4, 2, L2CacheControlPolicy> policy;
            };
        };

        enum class ViewportMulticast : u32 {
            ViewportOrder = 0,
            PrimitiveOrder = 1,
        };

        struct TIRModulationCoeff {
            union {
                BitField<0, 8, u32> table_v0;
                BitField<8, 8, u32> table_v1;
                BitField<16, 8, u32> table_v2;
                BitField<24, 8, u32> table_v3;
            };
        };
        static_assert(sizeof(TIRModulationCoeff) == 0x4);

        struct ReduceColorThreshold {
            union {
                BitField<0, 8, u32> all_hit_once;
                BitField<16, 8, u32> all_covered;
            };
        };

        struct ClearControl {
            union {
                BitField<0, 1, u32> respect_stencil_mask;
                BitField<4, 1, u32> use_clear_rect;
                BitField<8, 1, u32> use_scissor;
                BitField<12, 1, u32> use_viewport_clip0;
            };
        };

        struct L2CacheRopNonInterlockedReads {
            union {
                BitField<4, 2, L2CacheControlPolicy> policy;
            };
        };

        struct VertexOutputAttributeSkipMasks {
            struct Attributes {
                union {
                    BitField<0, 1, u32> attribute0_comp0;
                    BitField<1, 1, u32> attribute0_comp1;
                    BitField<2, 1, u32> attribute0_comp2;
                    BitField<3, 1, u32> attribute0_comp3;
                    BitField<4, 1, u32> attribute1_comp0;
                    BitField<5, 1, u32> attribute1_comp1;
                    BitField<6, 1, u32> attribute1_comp2;
                    BitField<7, 1, u32> attribute1_comp3;
                    BitField<8, 1, u32> attribute2_comp0;
                    BitField<9, 1, u32> attribute2_comp1;
                    BitField<10, 1, u32> attribute2_comp2;
                    BitField<11, 1, u32> attribute2_comp3;
                    BitField<12, 1, u32> attribute3_comp0;
                    BitField<13, 1, u32> attribute3_comp1;
                    BitField<14, 1, u32> attribute3_comp2;
                    BitField<15, 1, u32> attribute3_comp3;
                    BitField<16, 1, u32> attribute4_comp0;
                    BitField<17, 1, u32> attribute4_comp1;
                    BitField<18, 1, u32> attribute4_comp2;
                    BitField<19, 1, u32> attribute4_comp3;
                    BitField<20, 1, u32> attribute5_comp0;
                    BitField<21, 1, u32> attribute5_comp1;
                    BitField<22, 1, u32> attribute5_comp2;
                    BitField<23, 1, u32> attribute5_comp3;
                    BitField<24, 1, u32> attribute6_comp0;
                    BitField<25, 1, u32> attribute6_comp1;
                    BitField<26, 1, u32> attribute6_comp2;
                    BitField<27, 1, u32> attribute6_comp3;
                    BitField<28, 1, u32> attribute7_comp0;
                    BitField<29, 1, u32> attribute7_comp1;
                    BitField<30, 1, u32> attribute7_comp2;
                    BitField<31, 1, u32> attribute7_comp3;
                };
            };

            std::array<Attributes, 2> a;
            std::array<Attributes, 2> b;
        };

        struct TIRControl {
            union {
                BitField<0, 1, u32> z_pass_pixel_count_use_raster_samples;
                BitField<4, 1, u32> alpha_coverage_use_raster_samples;
                BitField<1, 1, u32> reduce_coverage;
            };
        };

        enum class FillViaTriangleMode : u32 {
            Disabled = 0,
            FillAll = 1,
            FillBoundingBox = 2,
        };

        struct PsTicketDispenserValue {
            union {
                BitField<0, 8, u32> index;
                BitField<8, 16, u32> value;
            };
        };

        struct RegisterWatermarks {
            union {
                BitField<0, 16, u32> low;
                BitField<16, 16, u32> high;
            };
        };

        enum class InvalidateCacheLines : u32 {
            All = 0,
            One = 1,
        };

        struct InvalidateTextureDataCacheNoWfi {
            union {
                BitField<0, 1, InvalidateCacheLines> lines;
                BitField<4, 22, u32> tag;
            };
        };

        struct ZCullRegionEnable {
            union {
                BitField<0, 1, u32> enable_z;
                BitField<4, 1, u32> enable_stencil;
                BitField<1, 1, u32> rect_clear;
                BitField<2, 1, u32> use_rt_array_index;
                BitField<5, 16, u32> rt_array_index;
                BitField<3, 1, u32> make_conservative;
            };
        };

        enum class FillMode : u32 {
            Point = 1,
            Wireframe = 2,
            Solid = 3,
        };

        enum class ShadeMode : u32 {
            Flat = 0x1,
            Gouraud = 0x2,
            GL_Flat = 0x1D00,
            GL_Smooth = 0x1D01,
        };

        enum class AlphaToCoverageDither : u32 {
            Footprint_1x1 = 0,
            Footprint_2x2 = 1,
            Footprint_1x1_Virtual = 2,
        };

        struct InlineIndex4x8 {
            union {
                BitField<0, 30, u32> count;
                BitField<30, 2, u32> start;
            };
            union {
                BitField<0, 8, u32> index0;
                BitField<8, 8, u32> index1;
                BitField<16, 8, u32> index2;
                BitField<24, 8, u32> index3;
            };
        };

        enum class D3DCullMode : u32 {
            None = 0,
            CW = 1,
            CCW = 2,
        };

        struct BlendColor {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };

        struct StencilOp {
            enum class Op : u32 {
                Keep_D3D = 1,
                Zero_D3D = 2,
                Replace_D3D = 3,
                IncrSaturate_D3D = 4,
                DecrSaturate_D3D = 5,
                Invert_D3D = 6,
                Incr_D3D = 7,
                Decr_D3D = 8,

                Keep_GL = 0x1E00,
                Zero_GL = 0,
                Replace_GL = 0x1E01,
                IncrSaturate_GL = 0x1E02,
                DecrSaturate_GL = 0x1E03,
                Invert_GL = 0x150A,
                Incr_GL = 0x8507,
                Decr_GL = 0x8508,
            };

            Op fail;
            Op zfail;
            Op zpass;
            ComparisonOp func;
        };

        struct PsSaturate {
            // Opposite of DepthMode
            enum class Depth : u32 {
                ZeroToOne = 0,
                MinusOneToOne = 1,
            };

            union {
                BitField<0, 1, u32> output0_enable;
                BitField<1, 1, Depth> output0_range;
                BitField<4, 1, u32> output1_enable;
                BitField<5, 1, Depth> output1_range;
                BitField<8, 1, u32> output2_enable;
                BitField<9, 1, Depth> output2_range;
                BitField<12, 1, u32> output3_enable;
                BitField<13, 1, Depth> output3_range;
                BitField<16, 1, u32> output4_enable;
                BitField<17, 1, Depth> output4_range;
                BitField<20, 1, u32> output5_enable;
                BitField<21, 1, Depth> output5_range;
                BitField<24, 1, u32> output6_enable;
                BitField<25, 1, Depth> output6_range;
                BitField<28, 1, u32> output7_enable;
                BitField<29, 1, Depth> output7_range;
            };

            bool AnyEnabled() const {
                return output0_enable || output1_enable || output2_enable || output3_enable ||
                       output4_enable || output5_enable || output6_enable || output7_enable;
            }
        };

        struct WindowOrigin {
            enum class Mode : u32 {
                UpperLeft = 0,
                LowerLeft = 1,
            };
            union {
                BitField<0, 1, Mode> mode;
                BitField<4, 1, u32> flip_y;
            };
        };

        struct IteratedBlendConstants {
            u32 r;
            u32 g;
            u32 b;
            INSERT_PADDING_BYTES_NOINIT(0x4);
        };
        static_assert(sizeof(IteratedBlendConstants) == 0x10);

        struct UserClip {
            struct Enable {
                union {
                    u32 raw;
                    BitField<0, 1, u32> plane0;
                    BitField<1, 1, u32> plane1;
                    BitField<2, 1, u32> plane2;
                    BitField<3, 1, u32> plane3;
                    BitField<4, 1, u32> plane4;
                    BitField<5, 1, u32> plane5;
                    BitField<6, 1, u32> plane6;
                    BitField<7, 1, u32> plane7;
                };

                bool AnyEnabled() const {
                    return plane0 || plane1 || plane2 || plane3 || plane4 || plane5 || plane6 ||
                           plane7;
                }
            };

            struct Op {
                enum class ClipOrCull : u32 {
                    Clip = 0,
                    Cull = 1,
                };

                union {
                    u32 raw;
                    BitField<0, 1, ClipOrCull> plane0;
                    BitField<4, 1, ClipOrCull> plane1;
                    BitField<8, 1, ClipOrCull> plane2;
                    BitField<12, 1, ClipOrCull> plane3;
                    BitField<16, 1, ClipOrCull> plane4;
                    BitField<20, 1, ClipOrCull> plane5;
                    BitField<24, 1, ClipOrCull> plane6;
                    BitField<28, 1, ClipOrCull> plane7;
                };
            };
        };

        struct AntiAliasAlphaControl {
            union {
                BitField<0, 1, u32> alpha_to_coverage;
                BitField<4, 1, u32> alpha_to_one;
            };
        };

        struct RenderEnable {
            enum class Override : u32 {
                UseRenderEnable = 0,
                AlwaysRender = 1,
                NeverRender = 2,
            };

            enum class Mode : u32 {
                False = 0,
                True = 1,
                Conditional = 2,
                IfEqual = 3,
                IfNotEqual = 4,
            };

            u32 address_high;
            u32 address_low;
            Mode mode;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct TexSampler {
            u32 address_high;
            u32 address_low;
            u32 limit;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct TexHeader {
            u32 address_high;
            u32 address_low;
            u32 limit;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        enum class ZCullRegionFormat : u32 {
            Z_4x4 = 0,
            ZS_4x4 = 1,
            Z_4x2 = 2,
            Z_2x4 = 3,
            Z_16x8_4x4 = 4,
            Z_8x8_4x2 = 5,
            Z_8x8_2x4 = 6,
            Z_16x16_4x8 = 7,
            Z_4x8_2x2 = 8,
            ZS_16x8_4x2 = 9,
            ZS_16x8_2x4 = 10,
            ZS_8x8_2x2 = 11,
            Z_4x8_1x1 = 12,
        };

        struct RtLayer {
            enum class Control {
                LayerSelectsLayer = 0,
                GeometryShaderSelectsLayer = 1,
            };

            union {
                BitField<0, 16, u32> layer;
                BitField<16, 1, u32> control;
            };
        };

        struct InlineIndex2x16 {
            union {
                BitField<0, 31, u32> count;
                BitField<31, 1, u32> start_odd;
            };
            union {
                BitField<0, 16, u32> even;
                BitField<16, 16, u32> odd;
            };
        };

        struct VertexGlobalBaseOffset {
            u32 address_high;
            u32 address_low;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct ZCullRegionPixelOffset {
            u32 width;
            u32 height;
        };

        struct PointSprite {
            enum class RMode : u32 {
                Zero = 0,
                FromR = 1,
                FromS = 2,
            };
            enum class Origin : u32 {
                Bottom = 0,
                Top = 1,
            };
            enum class Texture : u32 {
                Passthrough = 0,
                Generate = 1,
            };

            union {
                BitField<0, 2, RMode> rmode;
                BitField<2, 1, Origin> origin;
                BitField<3, 1, Texture> texture0;
                BitField<4, 1, Texture> texture1;
                BitField<5, 1, Texture> texture2;
                BitField<6, 1, Texture> texture3;
                BitField<7, 1, Texture> texture4;
                BitField<8, 1, Texture> texture5;
                BitField<9, 1, Texture> texture6;
                BitField<10, 1, Texture> texture7;
                BitField<11, 1, Texture> texture8;
                BitField<12, 1, Texture> texture9;
            };
        };

        struct ProgramRegion {
            u32 address_high;
            u32 address_low;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct DefaultAttributes {
            enum class Diffuse : u32 {
                Vector_0001 = 0,
                Vector_1111 = 1,
            };
            enum class Specular : u32 {
                Vector_0000 = 0,
                Vector_0001 = 1,
            };
            enum class Vector : u32 {
                Vector_0000 = 0,
                Vector_0001 = 1,
            };
            enum class FixedFncTexture : u32 {
                Vector_0000 = 0,
                Vector_0001 = 1,
            };
            enum class DX9Color0 : u32 {
                Vector_0000 = 0,
                Vector_1111 = 1,
            };
            enum class DX9Color1To15 : u32 {
                Vector_0000 = 0,
                Vector_0001 = 1,
            };

            union {
                BitField<0, 1, Diffuse> color_front_diffuse;
                BitField<1, 1, Specular> color_front_specular;
                BitField<2, 1, Vector> generic_vector;
                BitField<3, 1, FixedFncTexture> fixed_fnc_texture;
                BitField<4, 1, DX9Color0> dx9_color0;
                BitField<5, 1, DX9Color1To15> dx9_color1_to_15;
            };
        };

        struct Draw {
            enum class PrimitiveId : u32 {
                First = 0,
                Unchanged = 1,
            };
            enum class InstanceId : u32 {
                First = 0,
                Subsequent = 1,
                Unchanged = 2,
            };
            enum class SplitMode : u32 {
                NormalBeginNormal = 0,
                NormalBeginOpen = 1,
                OpenBeginOpen = 2,
                OpenBeginNormal = 3,
            };

            u32 end;
            union {
                u32 begin;
                BitField<0, 16, PrimitiveTopology> topology;
                BitField<24, 1, PrimitiveId> primitive_id;
                BitField<26, 2, InstanceId> instance_id;
                BitField<29, 2, SplitMode> split_mode;
            };
        };

        struct VertexIdCopy {
            union {
                BitField<0, 1, u32> enable;
                BitField<4, 8, u32> attribute_slot;
            };
        };

        struct ShaderBasedCull {
            union {
                BitField<1, 1, u32> batch_cull_enable;
                BitField<0, 1, u32> before_fetch_enable;
            };
        };

        struct ClassVersion {
            union {
                BitField<0, 16, u32> current;
                BitField<16, 16, u32> oldest_supported;
            };
        };

        struct PrimitiveRestart {
            u32 enabled;
            u32 index;
        };

        struct OutputVertexId {
            union {
                BitField<12, 1, u32> uses_array_start;
            };
        };

        enum class PointCenterMode : u32 {
            GL = 0,
            D3D = 1,
        };

        enum class LineSmoothParams : u32 {
            Falloff_1_00 = 0,
            Falloff_1_33 = 1,
            Falloff_1_66 = 2,
        };

        struct LineSmoothEdgeTable {
            union {
                BitField<0, 8, u32> v0;
                BitField<8, 8, u32> v1;
                BitField<16, 8, u32> v2;
                BitField<24, 8, u32> v3;
            };
        };

        struct LineStippleParams {
            union {
                BitField<0, 8, u32> factor;
                BitField<8, 16, u32> pattern;
            };
        };

        enum class ProvokingVertex : u32 {
            First = 0,
            Last = 1,
        };

        struct ShaderControl {
            enum class Partial : u32 {
                Zero = 0,
                Infinity = 1,
            };
            enum class FP32NanBehavior : u32 {
                Legacy = 0,
                FP64Compatible = 1,
            };
            enum class FP32F2INanBehavior : u32 {
                PassZero = 0,
                PassIndefinite = 1,
            };

            union {
                BitField<0, 1, Partial> default_partial;
                BitField<1, 1, FP32NanBehavior> fp32_nan_behavior;
                BitField<2, 1, FP32F2INanBehavior> fp32_f2i_nan_behavior;
            };
        };

        struct SphVersion {
            union {
                BitField<0, 16, u32> current;
                BitField<16, 16, u32> oldest_supported;
            };
        };

        struct AlphaToCoverageOverride {
            union {
                BitField<0, 1, u32> qualify_by_anti_alias_enable;
                BitField<1, 1, u32> qualify_by_ps_sample_mask_enable;
            };
        };

        struct AamVersion {
            union {
                BitField<0, 16, u32> current;
                BitField<16, 16, u32> oldest_supported;
            };
        };

        struct IndexBuffer {
            u32 start_addr_high;
            u32 start_addr_low;
            u32 limit_addr_high;
            u32 limit_addr_low;
            IndexFormat format;
            u32 first;
            u32 count;

            unsigned FormatSizeInBytes() const {
                switch (format) {
                case IndexFormat::UnsignedByte:
                    return 1;
                case IndexFormat::UnsignedShort:
                    return 2;
                case IndexFormat::UnsignedInt:
                    return 4;
                }
                ASSERT(false);
                return 1;
            }

            GPUVAddr StartAddress() const {
                return (GPUVAddr{start_addr_high} << 32) | GPUVAddr{start_addr_low};
            }

            GPUVAddr EndAddress() const {
                return (GPUVAddr{limit_addr_high} << 32) | GPUVAddr{limit_addr_low};
            }

            /// Adjust the index buffer offset so it points to the first desired index.
            GPUVAddr IndexStart() const {
                return StartAddress() + size_t{first} * size_t{FormatSizeInBytes()};
            }
        };

        struct IndexBufferSmall {
            union {
                u32 raw;
                BitField<0, 16, u32> first;
                BitField<16, 12, u32> count;
                BitField<28, 4, PrimitiveTopology> topology;
            };
        };

        struct VertexStreamInstances {
            std::array<u32, NumVertexArrays> is_instanced;

            /// Returns whether the vertex array specified by index is supposed to be
            /// accessed per instance or not.
            bool IsInstancingEnabled(std::size_t index) const {
                return is_instanced[index];
            }
        };

        struct AttributePointSize {
            union {
                BitField<0, 1, u32> enabled;
                BitField<4, 8, u32> slot;
            };
        };

        struct ViewportClipControl {
            enum class GeometryGuardband : u32 {
                Scale256 = 0,
                Scale1 = 1,
            };
            enum class GeometryClip : u32 {
                WZero = 0,
                Passthrough = 1,
                FrustumXY = 2,
                FrustumXYZ = 3,
                WZeroNoZCull = 4,
                FrustumZ = 5,
                WZeroTriFillOrClip = 6,
            };
            enum class GeometryGuardbandZ : u32 {
                SameAsXY = 0,
                Scale256 = 1,
                Scale1 = 2,
            };

            union {
                BitField<0, 1, u32> depth_0_to_1;
                BitField<3, 1, u32> pixel_min_z;
                BitField<4, 1, u32> pixel_max_z;
                BitField<7, 1, GeometryGuardband> geometry_guardband;
                BitField<11, 3, GeometryClip> geometry_clip;
                BitField<1, 2, GeometryGuardbandZ> geometry_guardband_z;
            };
        };

        enum class PrimitiveTopologyControl : u32 {
            UseInBeginMethods = 0,
            UseSeparateState = 1,
        };

        struct WindowClip {
            enum class Type : u32 {
                Inclusive = 0,
                Exclusive = 1,
                ClipAll = 2,
            };

            u32 enable;
            Type type;
        };

        enum class InvalidateZCull : u32 {
            Invalidate = 0,
        };

        struct ZCull {
            union {
                BitField<0, 1, u32> z_enable;
                BitField<1, 1, u32> stencil_enable;
            };
            union {
                BitField<0, 1, u32> z_min_enbounded;
                BitField<1, 1, u32> z_max_unbounded;
            };
        };

        struct LogicOp {
            enum class Op : u32 {
                Clear = 0x1500,
                And = 0x1501,
                AndReverse = 0x1502,
                Copy = 0x1503,
                AndInverted = 0x1504,
                NoOp = 0x1505,
                Xor = 0x1506,
                Or = 0x1507,
                Nor = 0x1508,
                Equiv = 0x1509,
                Invert = 0x150A,
                OrReverse = 0x150B,
                CopyInverted = 0x150C,
                OrInverted = 0x150D,
                Nand = 0x150E,
                Set = 0x150F,
            };

            u32 enable;
            Op op;
        };

        struct ClearSurface {
            union {
                u32 raw;
                BitField<0, 1, u32> Z;
                BitField<1, 1, u32> S;
                BitField<2, 1, u32> R;
                BitField<3, 1, u32> G;
                BitField<4, 1, u32> B;
                BitField<5, 1, u32> A;
                BitField<6, 4, u32> RT;
                BitField<10, 16, u32> layer;
            };
        };

        struct ReportSemaphore {
            struct Compare {
                u32 initial_sequence;
                u32 initial_mode;
                u32 unknown1;
                u32 unknown2;
                u32 current_sequence;
                u32 current_mode;
            };

            enum class Operation : u32 {
                Release = 0,
                Acquire = 1,
                ReportOnly = 2,
                Trap = 3,
            };

            enum class Release : u32 {
                AfterAllPreceedingReads = 0,
                AfterAllPreceedingWrites = 1,
            };

            enum class Acquire : u32 {
                BeforeAnyFollowingWrites = 0,
                BeforeAnyFollowingReads = 1,
            };

            enum class Location : u32 {
                None = 0,
                VertexFetch = 1,
                VertexShader = 2,
                VPC = 4,
                StreamingOutput = 5,
                GeometryShader = 6,
                ZCull = 7,
                TessellationInit = 8,
                TessellationShader = 9,
                PixelShader = 10,
                DepthTest = 12,
                All = 15,
            };

            enum class Comparison : u32 {
                NotEqual = 0,
                GreaterOrEqual = 1,
            };

            enum class Report : u32 {
                Payload = 0, // "None" in docs, but confirmed via hardware to return the payload
                VerticesGenerated = 1,
                ZPassPixelCount = 2,
                PrimitivesGenerated = 3,
                AlphaBetaClocks = 4,
                VertexShaderInvocations = 5,
                StreamingPrimitivesNeededMinusSucceeded = 6,
                GeometryShaderInvocations = 7,
                GeometryShaderPrimitivesGenerated = 9,
                ZCullStats0 = 10,
                StreamingPrimitivesSucceeded = 11,
                ZCullStats1 = 12,
                StreamingPrimitivesNeeded = 13,
                ZCullStats2 = 14,
                ClipperInvocations = 15,
                ZCullStats3 = 16,
                ClipperPrimitivesGenerated = 17,
                VtgPrimitivesOut = 18,
                PixelShaderInvocations = 19,
                ZPassPixelCount64 = 21,
                IEEECleanColorTarget = 24,
                IEEECleanZetaTarget = 25,
                StreamingByteCount = 26,
                TessellationInitInvocations = 27,
                BoundingRectangle = 28,
                TessellationShaderInvocations = 29,
                TotalStreamingPrimitivesNeededMinusSucceeded = 30,
                TessellationShaderPrimitivesGenerated = 31,
            };

            u32 address_high;
            u32 address_low;
            u32 payload;
            union {
                u32 raw;
                BitField<0, 2, Operation> operation;
                BitField<4, 1, Release> release;
                BitField<8, 1, Acquire> acquire;
                BitField<12, 4, Location> location;
                BitField<16, 1, Comparison> comparison;
                BitField<20, 1, u32> awaken_enable;
                BitField<23, 5, Report> report;
                BitField<28, 1, u32> short_query;
                BitField<5, 3, u32> sub_report;
                BitField<21, 1, u32> dword_number;
                BitField<2, 1, u32> disable_flush;
                BitField<3, 1, u32> reduction_enable;
                BitField<9, 3, ReductionOp> reduction_op;
                BitField<17, 2, u32> format_signed;
            } query;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct VertexStream {
            union {
                BitField<0, 12, u32> stride;
                BitField<12, 1, u32> enable;
            };
            u32 address_high;
            u32 address_low;
            u32 frequency;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }

            bool IsEnabled() const {
                return enable != 0 && Address() != 0;
            }
        };
        static_assert(sizeof(VertexStream) == 0x10);

        struct VertexStreamLimit {
            u32 address_high;
            u32 address_low;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };
        static_assert(sizeof(VertexStreamLimit) == 0x8);

        enum class ShaderType : u32 {
            VertexA = 0,
            VertexB = 1,
            TessellationInit = 2,
            Tessellation = 3,
            Geometry = 4,
            Pixel = 5,
        };

        struct Pipeline {
            union {
                BitField<0, 1, u32> enable;
                BitField<4, 4, ShaderType> program;
            };
            u32 offset;
            u32 reservedA;
            u32 register_count;
            u32 binding_group;
            std::array<u32, 4> reserved;
            INSERT_PADDING_BYTES_NOINIT(0x1C);
        };
        static_assert(sizeof(Pipeline) == 0x40);

        bool IsShaderConfigEnabled(std::size_t index) const {
            // The VertexB is always enabled.
            if (index == static_cast<std::size_t>(ShaderType::VertexB)) {
                return true;
            }
            return pipelines[index].enable != 0;
        }

        bool IsShaderConfigEnabled(ShaderType type) const {
            return IsShaderConfigEnabled(static_cast<std::size_t>(type));
        }

        struct ConstantBuffer {
            u32 size;
            u32 address_high;
            u32 address_low;
            u32 offset;
            std::array<u32, NumCBData> buffer;

            GPUVAddr Address() const {
                return (GPUVAddr{address_high} << 32) | GPUVAddr{address_low};
            }
        };

        struct BindGroup {
            std::array<u32, 4> reserved;
            union {
                u32 raw_config;
                BitField<0, 1, u32> valid;
                BitField<4, 5, u32> shader_slot;
            };
            INSERT_PADDING_BYTES_NOINIT(0xC);
        };
        static_assert(sizeof(BindGroup) == 0x20);

        struct StreamOutLayout {
            union {
                BitField<0, 8, u32> attribute0;
                BitField<8, 8, u32> attribute1;
                BitField<16, 8, u32> attribute2;
                BitField<24, 8, u32> attribute3;
            };
        };

        struct ShaderPerformance {
            struct ControlA {
                union {
                    BitField<0, 2, u32> event0;
                    BitField<2, 3, u32> bit0;
                    BitField<5, 2, u32> event1;
                    BitField<7, 3, u32> bit1;
                    BitField<10, 2, u32> event2;
                    BitField<12, 3, u32> bit2;
                    BitField<15, 2, u32> event3;
                    BitField<17, 3, u32> bit3;
                    BitField<20, 2, u32> event4;
                    BitField<22, 3, u32> bit4;
                    BitField<25, 2, u32> event5;
                    BitField<27, 3, u32> bit5;
                    BitField<30, 2, u32> spare;
                };
            };

            struct ControlB {
                union {
                    BitField<0, 1, u32> edge;
                    BitField<1, 2, u32> mode;
                    BitField<3, 1, u32> windowed;
                    BitField<4, 16, u32> func;
                };
            };

            std::array<u32, 8> values_upper;
            std::array<u32, 8> values;
            std::array<u32, 8> events;
            std::array<ControlA, 8> control_a;
            std::array<ControlB, 8> control_b;
            u32 trap_control_mask;
            u32 start_shader_mask;
            u32 stop_shader_mask;
        };

        // clang-format off
        union {
            struct {
                ID object_id;                                                          ///< 0x0000
                INSERT_PADDING_BYTES_NOINIT(0xFC);
                u32 nop;                                                               ///< 0x0100
                Notify notify;                                                         ///< 0x0104
                u32 wait_for_idle;                                                     ///< 0x0110
                LoadMME load_mme;                                                      ///< 0x0114
                ShadowRamControl shadow_ram_control;                                   ///< 0x0124
                PeerSemaphore peer;                                                    ///< 0x0128
                GlobalRender global_render;                                            ///< 0x0130
                u32 go_idle;                                                           ///< 0x013C
                u32 trigger;                                                           ///< 0x0140
                u32 trigger_wfi;                                                       ///< 0x0144
                INSERT_PADDING_BYTES_NOINIT(0x8);
                u32 instrumentation_method_header;                                     ///< 0x0150
                u32 instrumentation_method_data;                                       ///< 0x0154
                INSERT_PADDING_BYTES_NOINIT(0x28);
                Upload::Registers upload;                                              ///< 0x0180
                LaunchDMA launch_dma;                                                  ///< 0x01B0
                u32 inline_data;                                                       ///< 0x01B4
                INSERT_PADDING_BYTES_NOINIT(0x24);
                I2M i2m;                                                               ///< 0x01DC
                u32 run_ds_now;                                                        ///< 0x0200
                OpportunisticEarlyZ opportunistic_early_z;                             ///< 0x0204
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 aliased_line_width_enabled;                                        ///< 0x020C
                u32 mandated_early_z;                                                  ///< 0x0210
                GeometryShaderDmFifo gs_dm_fifo;                                       ///< 0x0214
                L2CacheControl l2_cache_control;                                       ///< 0x0218
                InvalidateShaderCache invalidate_shader_cache;                         ///< 0x021C
                INSERT_PADDING_BYTES_NOINIT(0xA8);
                SyncInfo sync_info;                                                    ///< 0x02C8
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 prim_circular_buffer_throttle;                                     ///< 0x02D0
                u32 flush_invalidate_rop_mini_cache;                                   ///< 0x02D4
                SurfaceClipBlockId surface_clip_block_id;                              ///< 0x02D8
                u32 alpha_circular_buffer_size;                                        ///< 0x02DC
                DecompressSurface decompress_surface;                                  ///< 0x02E0
                ZCullRopBypass zcull_rop_bypass;                                       ///< 0x02E4
                ZCullSubregion zcull_subregion;                                        ///< 0x02E8
                RasterBoundingBox raster_bounding_box;                                 ///< 0x02EC
                u32 peer_semaphore_release;                                            ///< 0x02F0
                u32 iterated_blend_optimization;                                       ///< 0x02F4
                ZCullSubregionAllocation zcull_subregion_allocation;                   ///< 0x02F8
                ZCullSubregionAlgorithm zcull_subregion_algorithm;                     ///< 0x02FC
                PixelShaderOutputSampleMaskUsage ps_output_sample_mask_usage;          ///< 0x0300
                u32 draw_zero_index;                                                   ///< 0x0304
                L1Configuration l1_configuration;                                      ///< 0x0308
                u32 render_enable_control_load_const_buffer;                           ///< 0x030C
                SPAVersion spa_version;                                                ///< 0x0310
                u32 ieee_clean_update;                                                 ///< 0x0314
                SnapGrid snap_grid;                                                    ///< 0x0318
                Tessellation tessellation;                                             ///< 0x0320
                SubTilingPerf sub_tiling_perf;                                         ///< 0x0360
                ZCullSubregionReport zcull_subregion_report;                           ///< 0x036C
                BalancedPrimitiveWorkload balanced_primitive_workload;                 ///< 0x0374
                u32 max_patches_per_batch;                                             ///< 0x0378
                u32 rasterize_enable;                                                  ///< 0x037C
                TransformFeedback transform_feedback;                                  ///< 0x0380
                u32 raster_input;                                                      ///< 0x0740
                u32 transform_feedback_enabled;                                        ///< 0x0744
                u32 primitive_restart_topology_change_enable;                          ///< 0x0748
                u32 alpha_fraction;                                                    ///< 0x074C
                INSERT_PADDING_BYTES_NOINIT(0x4);
                HybridAntiAliasControl hybrid_aa_control;                              ///< 0x0754
                INSERT_PADDING_BYTES_NOINIT(0x24);
                ShaderLocalMemory shader_local_memory;                                 ///< 0x077C
                u32 color_zero_bandwidth_clear;                                        ///< 0x07A4
                u32 z_zero_bandwidth_clear;                                            ///< 0x07A8
                u32 isbe_save_restore_program_offset;                                  ///< 0x07AC
                INSERT_PADDING_BYTES_NOINIT(0x10);
                ZCullRegion zcull_region;                                              ///< 0x07C0
                ZetaReadOnly zeta_read_only;                                           ///< 0x07F8
                INSERT_PADDING_BYTES_NOINIT(0x4);
                std::array<RenderTargetConfig, NumRenderTargets> rt;                   ///< 0x0800
                std::array<ViewportTransform, NumViewports> viewport_transform;        ///< 0x0A00
                std::array<Viewport, NumViewports> viewports;                          ///< 0x0C00
                std::array<Window, 8> windows;                                         ///< 0x0D00
                std::array<ClipIdExtent, 4> clip_id_extent;                            ///< 0x0D40
                u32 max_geometry_instances_per_task;                                   ///< 0x0D60
                VisibleCallLimit visible_call_limit;                                   ///< 0x0D64
                StatisticsCounter statistics_count;                                    ///< 0x0D68
                ClearRect clear_rect;                                                  ///< 0x0D6C
                VertexBuffer vertex_buffer;                                            ///< 0x0D74
                DepthMode depth_mode;                                                  ///< 0x0D7C
                std::array<f32, 4> clear_color;                                        ///< 0x0D80
                f32 clear_depth;                                                       ///< 0x0D90
                u32 shader_cache_icache_prefetch;                                      ///< 0x0D94
                u32 force_transition_to_beta;                                          ///< 0x0D98
                u32 reduce_colour_thresholds;                                          ///< 0x0D9C
                s32 clear_stencil;                                                     ///< 0x0DA0
                InvalidateShaderCacheNoWFI invalidate_shader_cache_no_wfi;             ///< 0x0DA4
                ZCullSerialization zcull_serialization;                                ///< 0x0DA8
                PolygonMode polygon_mode_front;                                        ///< 0x0DAC
                PolygonMode polygon_mode_back;                                         ///< 0x0DB0
                u32 polygon_smooth;                                                    ///< 0x0DB4
                u32 zeta_mark_clean_ieee;                                              ///< 0x0DB8
                ZCullDirFormat zcull_dir_format;                                       ///< 0x0DBC
                u32 polygon_offset_point_enable;                                       ///< 0x0DC0
                u32 polygon_offset_line_enable;                                        ///< 0x0DC4
                u32 polygon_offset_fill_enable;                                        ///< 0x0DC8
                u32 patch_vertices;                                                    ///< 0x0DCC
                IteratedBlend iterated_blend;                                          ///< 0x0DD0
                ZCullCriterion zcull_criteria;                                         ///< 0x0DD8
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 fragment_barrier;                                                  ///< 0x0DE0
                u32 sm_timeout;                                                        ///< 0x0DE4
                u32 primitive_restart_array;                                           ///< 0x0DE8
                INSERT_PADDING_BYTES_NOINIT(0x4);
                LoadIteratedBlend load_iterated_blend;                                 ///< 0x0DF0
                u32 window_offset_x;                                                   ///< 0x0DF8
                u32 window_offset_y;                                                   ///< 0x0DFC
                std::array<ScissorTest, NumViewports> scissor_test;                    ///< 0x0E00
                INSERT_PADDING_BYTES_NOINIT(0x10);
                u32 select_texture_headers;                                            ///< 0x0F10
                VPCPerf vpc_perf;                                                      ///< 0x0F14
                u32 pm_local_trigger;                                                  ///< 0x0F18
                u32 post_z_pixel_imask;                                                ///< 0x0F1C
                INSERT_PADDING_BYTES_NOINIT(0x20);
                ConstantColorRendering const_color_rendering;                          ///< 0x0F40
                s32 stencil_back_ref;                                                  ///< 0x0F54
                u32 stencil_back_mask;                                                 ///< 0x0F58
                u32 stencil_back_func_mask;                                            ///< 0x0F5C
                INSERT_PADDING_BYTES_NOINIT(0x14);
                u32 invalidate_texture_data_cache;                                     ///< 0x0F74 Assumed - Not in official docs.
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 tiled_cache_barrier;                                               ///< 0x0F7C Assumed - Not in official docs.
                INSERT_PADDING_BYTES_NOINIT(0x4);
                VertexStreamSubstitute vertex_stream_substitute;                       ///< 0x0F84
                u32 line_mode_clip_generated_edge_do_not_draw;                         ///< 0x0F8C
                u32 color_mask_common;                                                 ///< 0x0F90
                INSERT_PADDING_BYTES_NOINIT(0x4);
                VTGWarpWatermarks vtg_warp_watermarks;                                 ///< 0x0F98
                f32 depth_bounds[2];                                                   ///< 0x0F9C
                SampleMask::Target sample_mask_target;                                 ///< 0x0FA4
                u32 color_target_mrt_enable;                                           ///< 0x0FAC
                NonMultisampledZ non_multisampled_z;                                   ///< 0x0FB0
                TIRMode tir_mode;                                                      ///< 0x0FB4
                AntiAliasRaster anti_alias_raster;                                     ///< 0x0FB8
                SampleMask::Pos sample_mask_pos;                                       ///< 0x0FBC
                SurfaceClipIDMemory surface_clip_id_memory;                            ///< 0x0FCC
                TIRModulation tir_modulation;                                          ///< 0x0FD4
                u32 blend_control_allow_float_pixel_kills;                             ///< 0x0FDC
                Zeta zeta;                                                             ///< 0x0FE0
                SurfaceClip surface_clip;                                              ///< 0x0FF4
                u32 tiled_cache_treat_heavy_as_light;                                  ///< 0x0FFC
                L2CacheVAFRequests l2_cache_vaf;                                       ///< 0x1000
                ViewportMulticast viewport_multicast;                                  ///< 0x1004
                u32 tessellation_cut_height;                                           ///< 0x1008
                u32 max_gs_instances_per_task;                                         ///< 0x100C
                u32 max_gs_output_vertices_per_task;                                   ///< 0x1010
                u32 reserved_sw_method0;                                               ///< 0x1014
                u32 gs_output_cb_storage_multiplier;                                   ///< 0x1018
                u32 beta_cb_storage_constant;                                          ///< 0x101C
                u32 ti_output_cb_storage_multiplier;                                   ///< 0x1020
                u32 alpha_cb_storage_constraint;                                       ///< 0x1024
                u32 reserved_sw_method1;                                               ///< 0x1028
                u32 reserved_sw_method2;                                               ///< 0x102C
                std::array<TIRModulationCoeff, 5> tir_modulation_coeff;                ///< 0x1030
                std::array<u32, 15> spare_nop;                                         ///< 0x1044
                INSERT_PADDING_BYTES_NOINIT(0x30);
                std::array<u32, 7> reserved_sw_method3_to_7;                           ///< 0x10B0
                ReduceColorThreshold reduce_color_thresholds_unorm8;                   ///< 0x10CC
                std::array<u32, 4> reserved_sw_method10_to_13;                         ///< 0x10D0
                ReduceColorThreshold reduce_color_thresholds_unorm10;                  ///< 0x10E0
                ReduceColorThreshold reduce_color_thresholds_unorm16;                  ///< 0x10E4
                ReduceColorThreshold reduce_color_thresholds_fp11;                     ///< 0x10E8
                ReduceColorThreshold reduce_color_thresholds_fp16;                     ///< 0x10EC
                ReduceColorThreshold reduce_color_thresholds_srgb8;                    ///< 0x10F0
                u32 unbind_all_constant_buffers;                                       ///< 0x10F4
                ClearControl clear_control;                                            ///< 0x10F8
                L2CacheRopNonInterlockedReads l2_cache_rop_non_interlocked_reads;      ///< 0x10FC
                u32 reserved_sw_method14;                                              ///< 0x1100
                u32 reserved_sw_method15;                                              ///< 0x1104
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 no_operation_data_high;                                            ///< 0x110C
                u32 depth_bias_control;                                                ///< 0x1110
                u32 pm_trigger_end;                                                    ///< 0x1114
                u32 vertex_id_base;                                                    ///< 0x1118
                u32 stencil_compression_enabled;                                       ///< 0x111C
                VertexOutputAttributeSkipMasks vertex_output_attribute_skip_masks;     ///< 0x1120
                TIRControl tir_control;                                                ///< 0x1130
                u32 mutable_method_treat_mutable_as_heavy;                             ///< 0x1134
                u32 post_ps_use_pre_ps_coverage;                                       ///< 0x1138
                FillViaTriangleMode fill_via_triangle_mode;                            ///< 0x113C
                u32 blend_per_format_snorm8_unorm16_snorm16_enabled;                   ///< 0x1140
                u32 flush_pending_writes_sm_gloal_store;                               ///< 0x1144
                u32 conservative_raster_enable;                                        ///< 0x1148 Assumed - Not in official docs.
                INSERT_PADDING_BYTES_NOINIT(0x14);
                std::array<VertexAttribute, NumVertexAttributes> vertex_attrib_format; ///< 0x1160
                std::array<MsaaSampleLocation, 4> multisample_sample_locations;        ///< 0x11E0
                u32 offset_render_target_index_by_viewport_index;                      ///< 0x11F0
                u32 force_heavy_method_sync;                                           ///< 0x11F4
                MultisampleCoverageToColor multisample_coverage_to_color;              ///< 0x11F8
                DecompressZetaSurface decompress_zeta_surface;                         ///< 0x11FC
                INSERT_PADDING_BYTES_NOINIT(0x8);
                ZetaSparse zeta_sparse;                                                ///< 0x1208
                u32 invalidate_sampler_cache;                                          ///< 0x120C
                u32 invalidate_texture_header_cache;                                   ///< 0x1210
                VertexArray vertex_array_instance_first;                               ///< 0x1214
                VertexArray vertex_array_instance_subsequent;                          ///< 0x1218
                RtControl rt_control;                                                  ///< 0x121C
                CompressionThresholdSamples compression_threshold_samples;             ///< 0x1220
                PixelShaderInterlockControl ps_interlock_control;                      ///< 0x1224
                ZetaSize zeta_size;                                                    ///< 0x1228
                SamplerBinding sampler_binding;                                        ///< 0x1234
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 draw_auto_byte_count;                                              ///< 0x123C
                std::array<u32, 8> post_vtg_shader_attrib_skip_mask;                   ///< 0x1240
                PsTicketDispenserValue ps_ticket_dispenser_value;                      ///< 0x1260
                INSERT_PADDING_BYTES_NOINIT(0x1C);
                u32 circular_buffer_size;                                              ///< 0x1280
                RegisterWatermarks vtg_register_watermarks;                            ///< 0x1284
                InvalidateTextureDataCacheNoWfi invalidate_texture_cache_no_wfi;       ///< 0x1288
                INSERT_PADDING_BYTES_NOINIT(0x4);
                L2CacheRopNonInterlockedReads l2_cache_rop_interlocked_reads;          ///< 0x1290
                INSERT_PADDING_BYTES_NOINIT(0x10);
                u32 primitive_restart_topology_change_index;                           ///< 0x12A4
                INSERT_PADDING_BYTES_NOINIT(0x20);
                ZCullRegionEnable zcull_region_enable;                                 ///< 0x12C8
                u32 depth_test_enable;                                                 ///< 0x12CC
                FillMode fill_mode;                                                    ///< 0x12D0
                ShadeMode shade_mode;                                                  ///< 0x12D4
                L2CacheRopNonInterlockedReads l2_cache_rop_non_interlocked_writes;     ///< 0x12D8
                L2CacheRopNonInterlockedReads l2_cache_rop_interlocked_writes;         ///< 0x12DC
                AlphaToCoverageDither alpha_to_coverage_dither;                        ///< 0x12E0
                u32 blend_per_target_enabled;                                          ///< 0x12E4
                u32 depth_write_enabled;                                               ///< 0x12E8
                u32 alpha_test_enabled;                                                ///< 0x12EC
                INSERT_PADDING_BYTES_NOINIT(0x10);
                InlineIndex4x8 inline_index_4x8;                                       ///< 0x1300
                D3DCullMode d3d_cull_mode;                                             ///< 0x1308
                ComparisonOp depth_test_func;                                          ///< 0x130C
                f32 alpha_test_ref;                                                    ///< 0x1310
                ComparisonOp alpha_test_func;                                          ///< 0x1314
                u32 draw_auto_stride;                                                  ///< 0x1318
                BlendColor blend_color;                                                ///< 0x131C
                INSERT_PADDING_BYTES_NOINIT(0x4);
                InvalidateCacheLines invalidate_sampler_cache_lines;                   ///< 0x1330
                InvalidateCacheLines invalidate_texture_header_cache_lines;            ///< 0x1334
                InvalidateCacheLines invalidate_texture_data_cache_lines;              ///< 0x1338
                Blend blend;                                                           ///< 0x133C
                u32 stencil_enable;                                                    ///< 0x1380
                StencilOp stencil_front_op;                                            ///< 0x1384
                s32 stencil_front_ref;                                                 ///< 0x1394
                s32 stencil_front_func_mask;                                           ///< 0x1398
                s32 stencil_front_mask;                                                ///< 0x139C
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 draw_auto_start_byte_count;                                        ///< 0x13A4
                PsSaturate frag_color_clamp;                                           ///< 0x13A8
                WindowOrigin window_origin;                                            ///< 0x13AC
                f32 line_width_smooth;                                                 ///< 0x13B0
                f32 line_width_aliased;                                                ///< 0x13B4
                INSERT_PADDING_BYTES_NOINIT(0x60);
                u32 line_override_multisample;                                         ///< 0x1418
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 alpha_hysteresis_rounds;                                           ///< 0x1420
                InvalidateCacheLines invalidate_sampler_cache_no_wfi;                  ///< 0x1424
                InvalidateCacheLines invalidate_texture_header_cache_no_wfi;           ///< 0x1428
                INSERT_PADDING_BYTES_NOINIT(0x8);
                u32 global_base_vertex_index;                                          ///< 0x1434
                u32 global_base_instance_index;                                        ///< 0x1438
                INSERT_PADDING_BYTES_NOINIT(0x14);
                RegisterWatermarks ps_warp_watermarks;                                 ///< 0x1450
                RegisterWatermarks ps_regster_watermarks;                              ///< 0x1454
                INSERT_PADDING_BYTES_NOINIT(0xC);
                u32 store_zcull;                                                       ///< 0x1464
                INSERT_PADDING_BYTES_NOINIT(0x18);
                std::array<IteratedBlendConstants, NumRenderTargets>
                    iterated_blend_constants;                                          ///< 0x1480
                u32 load_zcull;                                                        ///< 0x1500
                u32 surface_clip_id_height;                                            ///< 0x1504
                Window surface_clip_id_clear_rect;                                     ///< 0x1508
                UserClip::Enable user_clip_enable;                                     ///< 0x1510
                u32 zpass_pixel_count_enable;                                          ///< 0x1514
                f32 point_size;                                                        ///< 0x1518
                u32 zcull_stats_enable;                                                ///< 0x151C
                u32 point_sprite_enable;                                               ///< 0x1520
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 shader_exceptions_enable;                                          ///< 0x1528
                INSERT_PADDING_BYTES_NOINIT(0x4);
                ClearReport clear_report_value;                                        ///< 0x1530
                u32 anti_alias_enable;                                                 ///< 0x1534
                u32 zeta_enable;                                                       ///< 0x1538
                AntiAliasAlphaControl anti_alias_alpha_control;                        ///< 0x153C
                INSERT_PADDING_BYTES_NOINIT(0x10);
                RenderEnable render_enable;                                            ///< 0x1550
                TexSampler tex_sampler;                                                ///< 0x155C
                INSERT_PADDING_BYTES_NOINIT(0x4);
                f32 slope_scale_depth_bias;                                            ///< 0x156C
                u32 line_anti_alias_enable;                                            ///< 0x1570
                TexHeader tex_header;                                                  ///< 0x1574
                INSERT_PADDING_BYTES_NOINIT(0x10);
                u32 active_zcull_region_id;                                            ///< 0x1590
                u32 stencil_two_side_enable;                                           ///< 0x1594
                StencilOp stencil_back_op;                                             ///< 0x1598
                INSERT_PADDING_BYTES_NOINIT(0x10);
                u32 framebuffer_srgb;                                                  ///< 0x15B8
                f32 depth_bias;                                                        ///< 0x15BC
                INSERT_PADDING_BYTES_NOINIT(0x8);
                ZCullRegionFormat zcull_region_format;                                 ///< 0x15C8
                RtLayer rt_layer;                                                      ///< 0x15CC
                Tegra::Texture::MsaaMode anti_alias_samples_mode;                      ///< 0x15D0
                INSERT_PADDING_BYTES_NOINIT(0x10);
                u32 edge_flag;                                                         ///< 0x15E4
                u32 draw_inline_index;                                                 ///< 0x15E8
                InlineIndex2x16 inline_index_2x16;                                     ///< 0x15EC
                VertexGlobalBaseOffset vertex_global_base_offset;                      ///< 0x15F4
                ZCullRegionPixelOffset zcull_region_pixel_offset;                      ///< 0x15FC
                PointSprite point_sprite;                                              ///< 0x1604
                ProgramRegion program_region;                                          ///< 0x1608
                DefaultAttributes default_attributes;                                  ///< 0x1610
                Draw draw;                                                             ///< 0x1614
                VertexIdCopy vertex_id_copy;                                           ///< 0x161C
                u32 add_to_primitive_id;                                               ///< 0x1620
                u32 load_to_primitive_id;                                              ///< 0x1624
                INSERT_PADDING_BYTES_NOINIT(0x4);
                ShaderBasedCull shader_based_cull;                                     ///< 0x162C
                INSERT_PADDING_BYTES_NOINIT(0x8);
                ClassVersion class_version;                                            ///< 0x1638
                INSERT_PADDING_BYTES_NOINIT(0x8);
                PrimitiveRestart primitive_restart;                                    ///< 0x1644
                OutputVertexId output_vertex_id;                                       ///< 0x164C
                INSERT_PADDING_BYTES_NOINIT(0x8);
                u32 anti_alias_point_enable;                                           ///< 0x1658
                PointCenterMode point_center_mode;                                     ///< 0x165C
                INSERT_PADDING_BYTES_NOINIT(0x8);
                LineSmoothParams line_smooth_params;                                   ///< 0x1668
                u32 line_stipple_enable;                                               ///< 0x166C
                std::array<LineSmoothEdgeTable, 4> line_smooth_edge_table;             ///< 0x1670
                LineStippleParams line_stipple_params;                                 ///< 0x1680
                ProvokingVertex provoking_vertex;                                      ///< 0x1684
                u32 two_sided_light_enabled;                                           ///< 0x1688
                u32 polygon_stipple_enabled;                                           ///< 0x168C
                ShaderControl shader_control;                                          ///< 0x1690
                INSERT_PADDING_BYTES_NOINIT(0xC);
                ClassVersion class_version_check;                                      ///< 0x16A0
                SphVersion sph_version;                                                ///< 0x16A4
                SphVersion sph_version_check;                                          ///< 0x16A8
                INSERT_PADDING_BYTES_NOINIT(0x8);
                AlphaToCoverageOverride alpha_to_coverage_override;                    ///< 0x16B4
                INSERT_PADDING_BYTES_NOINIT(0x48);
                std::array<u32, 32> polygon_stipple_pattern;                           ///< 0x1700
                INSERT_PADDING_BYTES_NOINIT(0x10);
                AamVersion aam_version;                                                ///< 0x1790
                AamVersion aam_version_check;                                          ///< 0x1794
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 zeta_layer_offset;                                                 ///< 0x179C
                INSERT_PADDING_BYTES_NOINIT(0x28);
                IndexBuffer index_buffer;                                              ///< 0x17C8
                IndexBufferSmall index_buffer32_first;                                 ///< 0x17E4
                IndexBufferSmall index_buffer16_first;                                 ///< 0x17E8
                IndexBufferSmall index_buffer8_first;                                  ///< 0x17EC
                IndexBufferSmall index_buffer32_subsequent;                            ///< 0x17F0
                IndexBufferSmall index_buffer16_subsequent;                            ///< 0x17F4
                IndexBufferSmall index_buffer8_subsequent;                             ///< 0x17F8
                INSERT_PADDING_BYTES_NOINIT(0x80);
                f32 depth_bias_clamp;                                                  ///< 0x187C
                VertexStreamInstances vertex_stream_instances;                         ///< 0x1880
                INSERT_PADDING_BYTES_NOINIT(0x10);
                AttributePointSize point_size_attribute;                               ///< 0x1910
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 gl_cull_test_enabled;                                              ///< 0x1918
                FrontFace gl_front_face;                                               ///< 0x191C
                CullFace gl_cull_face;                                                 ///< 0x1920
                Viewport::PixelCenter viewport_pixel_center;                           ///< 0x1924
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 viewport_scale_offset_enabled;                                     ///< 0x192C
                INSERT_PADDING_BYTES_NOINIT(0xC);
                ViewportClipControl viewport_clip_control;                             ///< 0x193C
                UserClip::Op user_clip_op;                                             ///< 0x1940
                RenderEnable::Override render_enable_override;                         ///< 0x1944
                PrimitiveTopologyControl primitive_topology_control;                   ///< 0x1948
                WindowClip window_clip_enable;                                         ///< 0x194C
                INSERT_PADDING_BYTES_NOINIT(0x4);
                InvalidateZCull invalidate_zcull;                                      ///< 0x1958
                INSERT_PADDING_BYTES_NOINIT(0xC);
                ZCull zcull;                                                           ///< 0x1968
                PrimitiveTopologyOverride topology_override;                           ///< 0x1970
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 zcull_sync;                                                        ///< 0x1978
                u32 clip_id_test_enable;                                               ///< 0x197C
                u32 surface_clip_id_width;                                             ///< 0x1980
                u32 clip_id;                                                           ///< 0x1984
                INSERT_PADDING_BYTES_NOINIT(0x34);
                u32 depth_bounds_enable;                                               ///< 0x19BC
                u32 blend_float_zero_times_anything_is_zero;                           ///< 0x19C0
                LogicOp logic_op;                                                      ///< 0x19C4
                u32 z_compression_enable;                                              ///< 0x19CC
                ClearSurface clear_surface;                                            ///< 0x19D0
                u32 clear_clip_id_surface;                                             ///< 0x19D4
                INSERT_PADDING_BYTES_NOINIT(0x8);
                std::array<u32, NumRenderTargets> color_compression_enable;            ///< 0x19E0
                std::array<ColorMask, NumRenderTargets> color_mask;                    ///< 0x1A00
                INSERT_PADDING_BYTES_NOINIT(0xC);
                u32 pipe_nop;                                                          ///< 0x1A2C
                std::array<u32, 4> spare;                                              ///< 0x1A30
                INSERT_PADDING_BYTES_NOINIT(0xC0);
                ReportSemaphore report_semaphore;                                      ///< 0x1B00
                INSERT_PADDING_BYTES_NOINIT(0xF0);
                std::array<VertexStream, NumVertexArrays> vertex_streams;              ///< 0x1C00
                BlendPerTarget blend_per_target[NumRenderTargets];                     ///< 0x1E00
                std::array<VertexStreamLimit, NumVertexArrays> vertex_stream_limits;   ///< 0x1F00
                std::array<Pipeline, MaxShaderProgram> pipelines;                      ///< 0x2000
                INSERT_PADDING_BYTES_NOINIT(0x180);
                u32 falcon[32];                                                        ///< 0x2300
                ConstantBuffer const_buffer;                                           ///< 0x2380
                INSERT_PADDING_BYTES_NOINIT(0x30);
                BindGroup bind_groups[MaxShaderStage];                                 ///< 0x2400
                INSERT_PADDING_BYTES_NOINIT(0x160);
                u32 color_clamp_enable;                                                ///< 0x2600
                INSERT_PADDING_BYTES_NOINIT(0x4);
                u32 bindless_texture_const_buffer_slot;                                ///< 0x2608
                u32 trap_handler;                                                      ///< 0x260C
                INSERT_PADDING_BYTES_NOINIT(0x1F0);
                std::array<std::array<StreamOutLayout, 32>, NumTransformFeedbackBuffers>
                    stream_out_layout;                                                 ///< 0x2800
                INSERT_PADDING_BYTES_NOINIT(0x93C);
                ShaderPerformance shader_performance;                                  ///< 0x333C
                INSERT_PADDING_BYTES_NOINIT(0x18);
                std::array<u32, 0x100> shadow_scratch;                                 ///< 0x3400
            };
            std::array<u32, NUM_REGS> reg_array;
        };
    };
    // clang-format on

    Regs regs{};

    /// Store temporary hw register values, used by some calls to restore state after a operation
    Regs shadow_state;

    static_assert(sizeof(Regs) == Regs::NUM_REGS * sizeof(u32), "Maxwell3D Regs has wrong size");
    static_assert(std::is_trivially_copyable_v<Regs>, "Maxwell3D Regs must be trivially copyable");

    struct State {
        struct ShaderStageInfo {
            std::array<ConstBufferInfo, Regs::MaxConstBuffers> const_buffers;
        };

        std::array<ShaderStageInfo, Regs::MaxShaderStage> shader_stages;
    };

    State state{};

    /// Reads a register value located at the input method address
    u32 GetRegisterValue(u32 method) const;

    /// Write the value to the register identified by method.
    void CallMethod(u32 method, u32 method_argument, bool is_last_call) override;

    /// Write multiple values to the register identified by method.
    void CallMultiMethod(u32 method, const u32* base_start, u32 amount,
                         u32 methods_pending) override;

    bool ShouldExecute() const {
        return execute_on;
    }

    VideoCore::RasterizerInterface& Rasterizer() {
        return *rasterizer;
    }

    const VideoCore::RasterizerInterface& Rasterizer() const {
        return *rasterizer;
    }

    struct DirtyState {
        using Flags = std::bitset<std::numeric_limits<u8>::max()>;
        using Table = std::array<u8, Regs::NUM_REGS>;
        using Tables = std::array<Table, 2>;

        Flags flags;
        Tables tables{};
    } dirty;

    std::unique_ptr<DrawManager> draw_manager;
    friend class DrawManager;

private:
    void InitializeRegisterDefaults();

    void ProcessMacro(u32 method, const u32* base_start, u32 amount, bool is_last_call);

    u32 ProcessShadowRam(u32 method, u32 argument);

    void ProcessDirtyRegisters(u32 method, u32 argument);

    void ProcessMethodCall(u32 method, u32 argument, u32 nonshadow_argument, bool is_last_call);

    /// Retrieves information about a specific TIC entry from the TIC buffer.
    Texture::TICEntry GetTICEntry(u32 tic_index) const;

    /// Retrieves information about a specific TSC entry from the TSC buffer.
    Texture::TSCEntry GetTSCEntry(u32 tsc_index) const;

    /**
     * Call a macro on this engine.
     *
     * @param method Method to call
     * @param parameters Arguments to the method call
     */
    void CallMacroMethod(u32 method, const std::vector<u32>& parameters);

    /// Handles writes to the macro uploading register.
    void ProcessMacroUpload(u32 data);

    /// Handles writes to the macro bind register.
    void ProcessMacroBind(u32 data);

    /// Handles firmware blob 4
    void ProcessFirmwareCall4();

    /// Handles a write to the QUERY_GET register.
    void ProcessQueryGet();

    /// Writes the query result accordingly.
    void StampQueryResult(u64 payload, bool long_query);

    /// Handles conditional rendering.
    void ProcessQueryCondition();

    /// Handles counter resets.
    void ProcessCounterReset();

    /// Handles writes to syncing register.
    void ProcessSyncPoint();

    /// Handles a write to the CB_DATA[i] register.
    void ProcessCBData(u32 value);
    void ProcessCBMultiData(const u32* start_base, u32 amount);

    /// Handles a write to the CB_BIND register.
    void ProcessCBBind(size_t stage_index);

    /// Returns a query's value or an empty object if the value will be deferred through a cache.
    std::optional<u64> GetQueryResult();

    Core::System& system;
    MemoryManager& memory_manager;

    VideoCore::RasterizerInterface* rasterizer = nullptr;

    /// Start offsets of each macro in macro_memory
    std::array<u32, 0x80> macro_positions{};

    /// Macro method that is currently being executed / being fed parameters.
    u32 executing_macro = 0;
    /// Parameters that have been submitted to the macro call so far.
    std::vector<u32> macro_params;

    /// Interpreter for the macro codes uploaded to the GPU.
    std::unique_ptr<MacroEngine> macro_engine;

    Upload::State upload_state;

    bool execute_on{true};
};

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Maxwell3D::Regs, field_name) == position,                               \
                  "Field " #field_name " has invalid position")

ASSERT_REG_POSITION(object_id, 0x0000);
ASSERT_REG_POSITION(nop, 0x0100);
ASSERT_REG_POSITION(notify, 0x0104);
ASSERT_REG_POSITION(wait_for_idle, 0x0110);
ASSERT_REG_POSITION(load_mme, 0x0114);
ASSERT_REG_POSITION(shadow_ram_control, 0x0124);
ASSERT_REG_POSITION(peer, 0x0128);
ASSERT_REG_POSITION(global_render, 0x0130);
ASSERT_REG_POSITION(go_idle, 0x013C);
ASSERT_REG_POSITION(trigger, 0x0140);
ASSERT_REG_POSITION(trigger_wfi, 0x0144);
ASSERT_REG_POSITION(instrumentation_method_header, 0x0150);
ASSERT_REG_POSITION(instrumentation_method_data, 0x0154);
ASSERT_REG_POSITION(upload, 0x0180);
ASSERT_REG_POSITION(launch_dma, 0x01B0);
ASSERT_REG_POSITION(inline_data, 0x01B4);
ASSERT_REG_POSITION(i2m, 0x01DC);
ASSERT_REG_POSITION(run_ds_now, 0x0200);
ASSERT_REG_POSITION(opportunistic_early_z, 0x0204);
ASSERT_REG_POSITION(aliased_line_width_enabled, 0x020C);
ASSERT_REG_POSITION(mandated_early_z, 0x0210);
ASSERT_REG_POSITION(gs_dm_fifo, 0x0214);
ASSERT_REG_POSITION(l2_cache_control, 0x0218);
ASSERT_REG_POSITION(invalidate_shader_cache, 0x021C);
ASSERT_REG_POSITION(sync_info, 0x02C8);
ASSERT_REG_POSITION(prim_circular_buffer_throttle, 0x02D0);
ASSERT_REG_POSITION(flush_invalidate_rop_mini_cache, 0x02D4);
ASSERT_REG_POSITION(surface_clip_block_id, 0x02D8);
ASSERT_REG_POSITION(alpha_circular_buffer_size, 0x02DC);
ASSERT_REG_POSITION(decompress_surface, 0x02E0);
ASSERT_REG_POSITION(zcull_rop_bypass, 0x02E4);
ASSERT_REG_POSITION(zcull_subregion, 0x02E8);
ASSERT_REG_POSITION(raster_bounding_box, 0x02EC);
ASSERT_REG_POSITION(peer_semaphore_release, 0x02F0);
ASSERT_REG_POSITION(iterated_blend_optimization, 0x02F4);
ASSERT_REG_POSITION(zcull_subregion_allocation, 0x02F8);
ASSERT_REG_POSITION(zcull_subregion_algorithm, 0x02FC);
ASSERT_REG_POSITION(ps_output_sample_mask_usage, 0x0300);
ASSERT_REG_POSITION(draw_zero_index, 0x0304);
ASSERT_REG_POSITION(l1_configuration, 0x0308);
ASSERT_REG_POSITION(render_enable_control_load_const_buffer, 0x030C);
ASSERT_REG_POSITION(spa_version, 0x0310);
ASSERT_REG_POSITION(ieee_clean_update, 0x0314);
ASSERT_REG_POSITION(snap_grid, 0x0318);
ASSERT_REG_POSITION(tessellation, 0x0320);
ASSERT_REG_POSITION(sub_tiling_perf, 0x0360);
ASSERT_REG_POSITION(zcull_subregion_report, 0x036C);
ASSERT_REG_POSITION(balanced_primitive_workload, 0x0374);
ASSERT_REG_POSITION(max_patches_per_batch, 0x0378);
ASSERT_REG_POSITION(rasterize_enable, 0x037C);
ASSERT_REG_POSITION(transform_feedback, 0x0380);
ASSERT_REG_POSITION(transform_feedback.controls, 0x0700);
ASSERT_REG_POSITION(raster_input, 0x0740);
ASSERT_REG_POSITION(transform_feedback_enabled, 0x0744);
ASSERT_REG_POSITION(primitive_restart_topology_change_enable, 0x0748);
ASSERT_REG_POSITION(alpha_fraction, 0x074C);
ASSERT_REG_POSITION(hybrid_aa_control, 0x0754);
ASSERT_REG_POSITION(shader_local_memory, 0x077C);
ASSERT_REG_POSITION(color_zero_bandwidth_clear, 0x07A4);
ASSERT_REG_POSITION(z_zero_bandwidth_clear, 0x07A8);
ASSERT_REG_POSITION(isbe_save_restore_program_offset, 0x07AC);
ASSERT_REG_POSITION(zcull_region, 0x07C0);
ASSERT_REG_POSITION(zeta_read_only, 0x07F8);
ASSERT_REG_POSITION(rt, 0x0800);
ASSERT_REG_POSITION(viewport_transform, 0x0A00);
ASSERT_REG_POSITION(viewports, 0x0C00);
ASSERT_REG_POSITION(windows, 0x0D00);
ASSERT_REG_POSITION(clip_id_extent, 0x0D40);
ASSERT_REG_POSITION(max_geometry_instances_per_task, 0x0D60);
ASSERT_REG_POSITION(visible_call_limit, 0x0D64);
ASSERT_REG_POSITION(statistics_count, 0x0D68);
ASSERT_REG_POSITION(clear_rect, 0x0D6C);
ASSERT_REG_POSITION(vertex_buffer, 0x0D74);
ASSERT_REG_POSITION(depth_mode, 0x0D7C);
ASSERT_REG_POSITION(clear_color, 0x0D80);
ASSERT_REG_POSITION(clear_depth, 0x0D90);
ASSERT_REG_POSITION(shader_cache_icache_prefetch, 0x0D94);
ASSERT_REG_POSITION(force_transition_to_beta, 0x0D98);
ASSERT_REG_POSITION(reduce_colour_thresholds, 0x0D9C);
ASSERT_REG_POSITION(clear_stencil, 0x0DA0);
ASSERT_REG_POSITION(invalidate_shader_cache_no_wfi, 0x0DA4);
ASSERT_REG_POSITION(zcull_serialization, 0x0DA8);
ASSERT_REG_POSITION(polygon_mode_front, 0x0DAC);
ASSERT_REG_POSITION(polygon_mode_back, 0x0DB0);
ASSERT_REG_POSITION(polygon_smooth, 0x0DB4);
ASSERT_REG_POSITION(zeta_mark_clean_ieee, 0x0DB8);
ASSERT_REG_POSITION(zcull_dir_format, 0x0DBC);
ASSERT_REG_POSITION(polygon_offset_point_enable, 0x0DC0);
ASSERT_REG_POSITION(polygon_offset_line_enable, 0x0DC4);
ASSERT_REG_POSITION(polygon_offset_fill_enable, 0x0DC8);
ASSERT_REG_POSITION(patch_vertices, 0x0DCC);
ASSERT_REG_POSITION(iterated_blend, 0x0DD0);
ASSERT_REG_POSITION(zcull_criteria, 0x0DD8);
ASSERT_REG_POSITION(fragment_barrier, 0x0DE0);
ASSERT_REG_POSITION(sm_timeout, 0x0DE4);
ASSERT_REG_POSITION(primitive_restart_array, 0x0DE8);
ASSERT_REG_POSITION(load_iterated_blend, 0x0DF0);
ASSERT_REG_POSITION(window_offset_x, 0x0DF8);
ASSERT_REG_POSITION(window_offset_y, 0x0DFC);
ASSERT_REG_POSITION(scissor_test, 0x0E00);
ASSERT_REG_POSITION(select_texture_headers, 0x0F10);
ASSERT_REG_POSITION(vpc_perf, 0x0F14);
ASSERT_REG_POSITION(pm_local_trigger, 0x0F18);
ASSERT_REG_POSITION(post_z_pixel_imask, 0x0F1C);
ASSERT_REG_POSITION(const_color_rendering, 0x0F40);
ASSERT_REG_POSITION(stencil_back_ref, 0x0F54);
ASSERT_REG_POSITION(stencil_back_mask, 0x0F58);
ASSERT_REG_POSITION(stencil_back_func_mask, 0x0F5C);
ASSERT_REG_POSITION(invalidate_texture_data_cache, 0x0F74);
ASSERT_REG_POSITION(tiled_cache_barrier, 0x0F7C);
ASSERT_REG_POSITION(vertex_stream_substitute, 0x0F84);
ASSERT_REG_POSITION(line_mode_clip_generated_edge_do_not_draw, 0x0F8C);
ASSERT_REG_POSITION(color_mask_common, 0x0F90);
ASSERT_REG_POSITION(vtg_warp_watermarks, 0x0F98);
ASSERT_REG_POSITION(depth_bounds, 0x0F9C);
ASSERT_REG_POSITION(sample_mask_target, 0x0FA4);
ASSERT_REG_POSITION(color_target_mrt_enable, 0x0FAC);
ASSERT_REG_POSITION(non_multisampled_z, 0x0FB0);
ASSERT_REG_POSITION(tir_mode, 0x0FB4);
ASSERT_REG_POSITION(anti_alias_raster, 0x0FB8);
ASSERT_REG_POSITION(sample_mask_pos, 0x0FBC);
ASSERT_REG_POSITION(surface_clip_id_memory, 0x0FCC);
ASSERT_REG_POSITION(tir_modulation, 0x0FD4);
ASSERT_REG_POSITION(blend_control_allow_float_pixel_kills, 0x0FDC);
ASSERT_REG_POSITION(zeta, 0x0FE0);
ASSERT_REG_POSITION(surface_clip, 0x0FF4);
ASSERT_REG_POSITION(tiled_cache_treat_heavy_as_light, 0x0FFC);
ASSERT_REG_POSITION(l2_cache_vaf, 0x1000);
ASSERT_REG_POSITION(viewport_multicast, 0x1004);
ASSERT_REG_POSITION(tessellation_cut_height, 0x1008);
ASSERT_REG_POSITION(max_gs_instances_per_task, 0x100C);
ASSERT_REG_POSITION(max_gs_output_vertices_per_task, 0x1010);
ASSERT_REG_POSITION(reserved_sw_method0, 0x1014);
ASSERT_REG_POSITION(gs_output_cb_storage_multiplier, 0x1018);
ASSERT_REG_POSITION(beta_cb_storage_constant, 0x101C);
ASSERT_REG_POSITION(ti_output_cb_storage_multiplier, 0x1020);
ASSERT_REG_POSITION(alpha_cb_storage_constraint, 0x1024);
ASSERT_REG_POSITION(reserved_sw_method1, 0x1028);
ASSERT_REG_POSITION(reserved_sw_method2, 0x102C);
ASSERT_REG_POSITION(tir_modulation_coeff, 0x1030);
ASSERT_REG_POSITION(spare_nop, 0x1044);
ASSERT_REG_POSITION(reserved_sw_method3_to_7, 0x10B0);
ASSERT_REG_POSITION(reduce_color_thresholds_unorm8, 0x10CC);
ASSERT_REG_POSITION(reserved_sw_method10_to_13, 0x10D0);
ASSERT_REG_POSITION(reduce_color_thresholds_unorm10, 0x10E0);
ASSERT_REG_POSITION(reduce_color_thresholds_unorm16, 0x10E4);
ASSERT_REG_POSITION(reduce_color_thresholds_fp11, 0x10E8);
ASSERT_REG_POSITION(reduce_color_thresholds_fp16, 0x10EC);
ASSERT_REG_POSITION(reduce_color_thresholds_srgb8, 0x10F0);
ASSERT_REG_POSITION(unbind_all_constant_buffers, 0x10F4);
ASSERT_REG_POSITION(clear_control, 0x10F8);
ASSERT_REG_POSITION(l2_cache_rop_non_interlocked_reads, 0x10FC);
ASSERT_REG_POSITION(reserved_sw_method14, 0x1100);
ASSERT_REG_POSITION(reserved_sw_method15, 0x1104);
ASSERT_REG_POSITION(no_operation_data_high, 0x110C);
ASSERT_REG_POSITION(depth_bias_control, 0x1110);
ASSERT_REG_POSITION(pm_trigger_end, 0x1114);
ASSERT_REG_POSITION(vertex_id_base, 0x1118);
ASSERT_REG_POSITION(stencil_compression_enabled, 0x111C);
ASSERT_REG_POSITION(vertex_output_attribute_skip_masks, 0x1120);
ASSERT_REG_POSITION(tir_control, 0x1130);
ASSERT_REG_POSITION(mutable_method_treat_mutable_as_heavy, 0x1134);
ASSERT_REG_POSITION(post_ps_use_pre_ps_coverage, 0x1138);
ASSERT_REG_POSITION(fill_via_triangle_mode, 0x113C);
ASSERT_REG_POSITION(blend_per_format_snorm8_unorm16_snorm16_enabled, 0x1140);
ASSERT_REG_POSITION(flush_pending_writes_sm_gloal_store, 0x1144);
ASSERT_REG_POSITION(conservative_raster_enable, 0x1148);
ASSERT_REG_POSITION(vertex_attrib_format, 0x1160);
ASSERT_REG_POSITION(multisample_sample_locations, 0x11E0);
ASSERT_REG_POSITION(offset_render_target_index_by_viewport_index, 0x11F0);
ASSERT_REG_POSITION(force_heavy_method_sync, 0x11F4);
ASSERT_REG_POSITION(multisample_coverage_to_color, 0x11F8);
ASSERT_REG_POSITION(decompress_zeta_surface, 0x11FC);
ASSERT_REG_POSITION(zeta_sparse, 0x1208);
ASSERT_REG_POSITION(invalidate_sampler_cache, 0x120C);
ASSERT_REG_POSITION(invalidate_texture_header_cache, 0x1210);
ASSERT_REG_POSITION(vertex_array_instance_first, 0x1214);
ASSERT_REG_POSITION(vertex_array_instance_subsequent, 0x1218);
ASSERT_REG_POSITION(rt_control, 0x121C);
ASSERT_REG_POSITION(compression_threshold_samples, 0x1220);
ASSERT_REG_POSITION(ps_interlock_control, 0x1224);
ASSERT_REG_POSITION(zeta_size, 0x1228);
ASSERT_REG_POSITION(sampler_binding, 0x1234);
ASSERT_REG_POSITION(draw_auto_byte_count, 0x123C);
ASSERT_REG_POSITION(post_vtg_shader_attrib_skip_mask, 0x1240);
ASSERT_REG_POSITION(ps_ticket_dispenser_value, 0x1260);
ASSERT_REG_POSITION(circular_buffer_size, 0x1280);
ASSERT_REG_POSITION(vtg_register_watermarks, 0x1284);
ASSERT_REG_POSITION(invalidate_texture_cache_no_wfi, 0x1288);
ASSERT_REG_POSITION(l2_cache_rop_interlocked_reads, 0x1290);
ASSERT_REG_POSITION(primitive_restart_topology_change_index, 0x12A4);
ASSERT_REG_POSITION(zcull_region_enable, 0x12C8);
ASSERT_REG_POSITION(depth_test_enable, 0x12CC);
ASSERT_REG_POSITION(fill_mode, 0x12D0);
ASSERT_REG_POSITION(shade_mode, 0x12D4);
ASSERT_REG_POSITION(l2_cache_rop_non_interlocked_writes, 0x12D8);
ASSERT_REG_POSITION(l2_cache_rop_interlocked_writes, 0x12DC);
ASSERT_REG_POSITION(alpha_to_coverage_dither, 0x12E0);
ASSERT_REG_POSITION(blend_per_target_enabled, 0x12E4);
ASSERT_REG_POSITION(depth_write_enabled, 0x12E8);
ASSERT_REG_POSITION(alpha_test_enabled, 0x12EC);
ASSERT_REG_POSITION(inline_index_4x8, 0x1300);
ASSERT_REG_POSITION(d3d_cull_mode, 0x1308);
ASSERT_REG_POSITION(depth_test_func, 0x130C);
ASSERT_REG_POSITION(alpha_test_ref, 0x1310);
ASSERT_REG_POSITION(alpha_test_func, 0x1314);
ASSERT_REG_POSITION(draw_auto_stride, 0x1318);
ASSERT_REG_POSITION(blend_color, 0x131C);
ASSERT_REG_POSITION(invalidate_sampler_cache_lines, 0x1330);
ASSERT_REG_POSITION(invalidate_texture_header_cache_lines, 0x1334);
ASSERT_REG_POSITION(invalidate_texture_data_cache_lines, 0x1338);
ASSERT_REG_POSITION(blend, 0x133C);
ASSERT_REG_POSITION(stencil_enable, 0x1380);
ASSERT_REG_POSITION(stencil_front_op, 0x1384);
ASSERT_REG_POSITION(stencil_front_ref, 0x1394);
ASSERT_REG_POSITION(stencil_front_func_mask, 0x1398);
ASSERT_REG_POSITION(stencil_front_mask, 0x139C);
ASSERT_REG_POSITION(draw_auto_start_byte_count, 0x13A4);
ASSERT_REG_POSITION(frag_color_clamp, 0x13A8);
ASSERT_REG_POSITION(window_origin, 0x13AC);
ASSERT_REG_POSITION(line_width_smooth, 0x13B0);
ASSERT_REG_POSITION(line_width_aliased, 0x13B4);
ASSERT_REG_POSITION(line_override_multisample, 0x1418);
ASSERT_REG_POSITION(alpha_hysteresis_rounds, 0x1420);
ASSERT_REG_POSITION(invalidate_sampler_cache_no_wfi, 0x1424);
ASSERT_REG_POSITION(invalidate_texture_header_cache_no_wfi, 0x1428);
ASSERT_REG_POSITION(global_base_vertex_index, 0x1434);
ASSERT_REG_POSITION(global_base_instance_index, 0x1438);
ASSERT_REG_POSITION(ps_warp_watermarks, 0x1450);
ASSERT_REG_POSITION(ps_regster_watermarks, 0x1454);
ASSERT_REG_POSITION(store_zcull, 0x1464);
ASSERT_REG_POSITION(iterated_blend_constants, 0x1480);
ASSERT_REG_POSITION(load_zcull, 0x1500);
ASSERT_REG_POSITION(surface_clip_id_height, 0x1504);
ASSERT_REG_POSITION(surface_clip_id_clear_rect, 0x1508);
ASSERT_REG_POSITION(user_clip_enable, 0x1510);
ASSERT_REG_POSITION(zpass_pixel_count_enable, 0x1514);
ASSERT_REG_POSITION(point_size, 0x1518);
ASSERT_REG_POSITION(zcull_stats_enable, 0x151C);
ASSERT_REG_POSITION(point_sprite_enable, 0x1520);
ASSERT_REG_POSITION(shader_exceptions_enable, 0x1528);
ASSERT_REG_POSITION(clear_report_value, 0x1530);
ASSERT_REG_POSITION(anti_alias_enable, 0x1534);
ASSERT_REG_POSITION(zeta_enable, 0x1538);
ASSERT_REG_POSITION(anti_alias_alpha_control, 0x153C);
ASSERT_REG_POSITION(render_enable, 0x1550);
ASSERT_REG_POSITION(tex_sampler, 0x155C);
ASSERT_REG_POSITION(slope_scale_depth_bias, 0x156C);
ASSERT_REG_POSITION(line_anti_alias_enable, 0x1570);
ASSERT_REG_POSITION(tex_header, 0x1574);
ASSERT_REG_POSITION(active_zcull_region_id, 0x1590);
ASSERT_REG_POSITION(stencil_two_side_enable, 0x1594);
ASSERT_REG_POSITION(stencil_back_op, 0x1598);
ASSERT_REG_POSITION(framebuffer_srgb, 0x15B8);
ASSERT_REG_POSITION(depth_bias, 0x15BC);
ASSERT_REG_POSITION(zcull_region_format, 0x15C8);
ASSERT_REG_POSITION(rt_layer, 0x15CC);
ASSERT_REG_POSITION(anti_alias_samples_mode, 0x15D0);
ASSERT_REG_POSITION(edge_flag, 0x15E4);
ASSERT_REG_POSITION(draw_inline_index, 0x15E8);
ASSERT_REG_POSITION(inline_index_2x16, 0x15EC);
ASSERT_REG_POSITION(vertex_global_base_offset, 0x15F4);
ASSERT_REG_POSITION(zcull_region_pixel_offset, 0x15FC);
ASSERT_REG_POSITION(point_sprite, 0x1604);
ASSERT_REG_POSITION(program_region, 0x1608);
ASSERT_REG_POSITION(default_attributes, 0x1610);
ASSERT_REG_POSITION(draw, 0x1614);
ASSERT_REG_POSITION(vertex_id_copy, 0x161C);
ASSERT_REG_POSITION(add_to_primitive_id, 0x1620);
ASSERT_REG_POSITION(load_to_primitive_id, 0x1624);
ASSERT_REG_POSITION(shader_based_cull, 0x162C);
ASSERT_REG_POSITION(class_version, 0x1638);
ASSERT_REG_POSITION(primitive_restart, 0x1644);
ASSERT_REG_POSITION(output_vertex_id, 0x164C);
ASSERT_REG_POSITION(anti_alias_point_enable, 0x1658);
ASSERT_REG_POSITION(point_center_mode, 0x165C);
ASSERT_REG_POSITION(line_smooth_params, 0x1668);
ASSERT_REG_POSITION(line_stipple_enable, 0x166C);
ASSERT_REG_POSITION(line_smooth_edge_table, 0x1670);
ASSERT_REG_POSITION(line_stipple_params, 0x1680);
ASSERT_REG_POSITION(provoking_vertex, 0x1684);
ASSERT_REG_POSITION(two_sided_light_enabled, 0x1688);
ASSERT_REG_POSITION(polygon_stipple_enabled, 0x168C);
ASSERT_REG_POSITION(shader_control, 0x1690);
ASSERT_REG_POSITION(class_version_check, 0x16A0);
ASSERT_REG_POSITION(sph_version, 0x16A4);
ASSERT_REG_POSITION(sph_version_check, 0x16A8);
ASSERT_REG_POSITION(alpha_to_coverage_override, 0x16B4);
ASSERT_REG_POSITION(polygon_stipple_pattern, 0x1700);
ASSERT_REG_POSITION(aam_version, 0x1790);
ASSERT_REG_POSITION(aam_version_check, 0x1794);
ASSERT_REG_POSITION(zeta_layer_offset, 0x179C);
ASSERT_REG_POSITION(index_buffer, 0x17C8);
ASSERT_REG_POSITION(index_buffer32_first, 0x17E4);
ASSERT_REG_POSITION(index_buffer16_first, 0x17E8);
ASSERT_REG_POSITION(index_buffer8_first, 0x17EC);
ASSERT_REG_POSITION(index_buffer32_subsequent, 0x17F0);
ASSERT_REG_POSITION(index_buffer16_subsequent, 0x17F4);
ASSERT_REG_POSITION(index_buffer8_subsequent, 0x17F8);
ASSERT_REG_POSITION(depth_bias_clamp, 0x187C);
ASSERT_REG_POSITION(vertex_stream_instances, 0x1880);
ASSERT_REG_POSITION(point_size_attribute, 0x1910);
ASSERT_REG_POSITION(gl_cull_test_enabled, 0x1918);
ASSERT_REG_POSITION(gl_front_face, 0x191C);
ASSERT_REG_POSITION(gl_cull_face, 0x1920);
ASSERT_REG_POSITION(viewport_pixel_center, 0x1924);
ASSERT_REG_POSITION(viewport_scale_offset_enabled, 0x192C);
ASSERT_REG_POSITION(viewport_clip_control, 0x193C);
ASSERT_REG_POSITION(user_clip_op, 0x1940);
ASSERT_REG_POSITION(render_enable_override, 0x1944);
ASSERT_REG_POSITION(primitive_topology_control, 0x1948);
ASSERT_REG_POSITION(window_clip_enable, 0x194C);
ASSERT_REG_POSITION(invalidate_zcull, 0x1958);
ASSERT_REG_POSITION(zcull, 0x1968);
ASSERT_REG_POSITION(topology_override, 0x1970);
ASSERT_REG_POSITION(zcull_sync, 0x1978);
ASSERT_REG_POSITION(clip_id_test_enable, 0x197C);
ASSERT_REG_POSITION(surface_clip_id_width, 0x1980);
ASSERT_REG_POSITION(clip_id, 0x1984);
ASSERT_REG_POSITION(depth_bounds_enable, 0x19BC);
ASSERT_REG_POSITION(blend_float_zero_times_anything_is_zero, 0x19C0);
ASSERT_REG_POSITION(logic_op, 0x19C4);
ASSERT_REG_POSITION(z_compression_enable, 0x19CC);
ASSERT_REG_POSITION(clear_surface, 0x19D0);
ASSERT_REG_POSITION(clear_clip_id_surface, 0x19D4);
ASSERT_REG_POSITION(color_compression_enable, 0x19E0);
ASSERT_REG_POSITION(color_mask, 0x1A00);
ASSERT_REG_POSITION(pipe_nop, 0x1A2C);
ASSERT_REG_POSITION(spare, 0x1A30);
ASSERT_REG_POSITION(report_semaphore, 0x1B00);
ASSERT_REG_POSITION(vertex_streams, 0x1C00);
ASSERT_REG_POSITION(blend_per_target, 0x1E00);
ASSERT_REG_POSITION(vertex_stream_limits, 0x1F00);
ASSERT_REG_POSITION(pipelines, 0x2000);
ASSERT_REG_POSITION(falcon, 0x2300);
ASSERT_REG_POSITION(const_buffer, 0x2380);
ASSERT_REG_POSITION(bind_groups, 0x2400);
ASSERT_REG_POSITION(color_clamp_enable, 0x2600);
ASSERT_REG_POSITION(bindless_texture_const_buffer_slot, 0x2608);
ASSERT_REG_POSITION(trap_handler, 0x260C);
ASSERT_REG_POSITION(stream_out_layout, 0x2800);
ASSERT_REG_POSITION(shader_performance, 0x333C);
ASSERT_REG_POSITION(shadow_scratch, 0x3400);

#undef ASSERT_REG_POSITION

} // namespace Tegra::Engines
