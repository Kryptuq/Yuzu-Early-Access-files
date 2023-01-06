// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/engines/engine_interface.h"
#include "video_core/gpu.h"

namespace Tegra {
class MemoryManager;
}

namespace VideoCore {
class RasterizerInterface;
}

namespace Tegra::Engines {

namespace Blitter {
class SoftwareBlitEngine;
}

/**
 * This Engine is known as G80_2D. Documentation can be found in:
 * https://github.com/envytools/envytools/blob/master/rnndb/graph/g80_2d.xml
 * https://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/nouveau/nv50/nv50_2d.xml.h
 */

#define FERMI2D_REG_INDEX(field_name)                                                              \
    (offsetof(Tegra::Engines::Fermi2D::Regs, field_name) / sizeof(u32))

class Fermi2D final : public EngineInterface {
public:
    explicit Fermi2D(MemoryManager& memory_manager_);
    ~Fermi2D() override;

    /// Binds a rasterizer to this engine.
    void BindRasterizer(VideoCore::RasterizerInterface* rasterizer);

    /// Write the value to the register identified by method.
    void CallMethod(u32 method, u32 method_argument, bool is_last_call) override;

    /// Write multiple values to the register identified by method.
    void CallMultiMethod(u32 method, const u32* base_start, u32 amount,
                         u32 methods_pending) override;

    enum class Origin : u32 {
        Center = 0,
        Corner = 1,
    };

    enum class Filter : u32 {
        Point = 0,
        Bilinear = 1,
    };

    enum class Operation : u32 {
        SrcCopyAnd = 0,
        ROPAnd = 1,
        Blend = 2,
        SrcCopy = 3,
        ROP = 4,
        SrcCopyPremult = 5,
        BlendPremult = 6,
    };

    enum class MemoryLayout : u32 {
        BlockLinear = 0,
        Pitch = 1,
    };

    enum class CpuIndexWrap : u32 {
        Wrap = 0,
        NoWrap = 1,
    };

    struct Surface {
        RenderTargetFormat format;
        MemoryLayout linear;
        union {
            BitField<0, 4, u32> block_width;
            BitField<4, 4, u32> block_height;
            BitField<8, 4, u32> block_depth;
        };
        u32 depth;
        u32 layer;
        u32 pitch;
        u32 width;
        u32 height;
        u32 addr_upper;
        u32 addr_lower;

        [[nodiscard]] constexpr GPUVAddr Address() const noexcept {
            return (GPUVAddr{addr_upper} << 32) | GPUVAddr{addr_lower};
        }
    };
    static_assert(sizeof(Surface) == 0x28, "Surface has incorrect size");

    enum class SectorPromotion : u32 {
        NoPromotion = 0,
        PromoteTo2V = 1,
        PromoteTo2H = 2,
        PromoteTo4 = 3,
    };

    enum class NumTpcs : u32 {
        All = 0,
        One = 1,
    };

    enum class RenderEnableMode : u32 {
        False = 0,
        True = 1,
        Conditional = 2,
        RenderIfEqual = 3,
        RenderIfNotEqual = 4,
    };

    enum class ColorKeyFormat : u32 {
        A16R56G6B5 = 0,
        A1R5G55B5 = 1,
        A8R8G8B8 = 2,
        A2R10G10B10 = 3,
        Y8 = 4,
        Y16 = 5,
        Y32 = 6,
    };

    union Beta4 {
        BitField<0, 8, u32> b;
        BitField<8, 8, u32> g;
        BitField<16, 8, u32> r;
        BitField<24, 8, u32> a;
    };

    struct Point {
        u32 x;
        u32 y;
    };

    enum class PatternSelect : u32 {
        MonoChrome8x8 = 0,
        MonoChrome64x1 = 1,
        MonoChrome1x64 = 2,
        Color = 3,
    };

    enum class NotifyType : u32 {
        WriteOnly = 0,
        WriteThenAwaken = 1,
    };

    enum class MonochromePatternColorFormat : u32 {
        A8X8R8G6B5 = 0,
        A1R5G5B5 = 1,
        A8R8G8B8 = 2,
        A8Y8 = 3,
        A8X8Y16 = 4,
        Y32 = 5,
    };

    enum class MonochromePatternFormat : u32 {
        CGA6_M1 = 0,
        LE_M1 = 1,
    };

    union Regs {
        static constexpr std::size_t NUM_REGS = 0x258;
        struct {
            u32 object;
            INSERT_PADDING_WORDS_NOINIT(0x3F);
            u32 no_operation;
            NotifyType notify;
            INSERT_PADDING_WORDS_NOINIT(0x2);
            u32 wait_for_idle;
            INSERT_PADDING_WORDS_NOINIT(0xB);
            u32 pm_trigger;
            INSERT_PADDING_WORDS_NOINIT(0xF);
            u32 context_dma_notify;
            u32 dst_context_dma;
            u32 src_context_dma;
            u32 semaphore_context_dma;
            INSERT_PADDING_WORDS_NOINIT(0x1C);
            Surface dst;
            CpuIndexWrap pixels_from_cpu_index_wrap;
            u32 kind2d_check_enable;
            Surface src;
            SectorPromotion pixels_from_memory_sector_promotion;
            INSERT_PADDING_WORDS_NOINIT(0x1);
            NumTpcs num_tpcs;
            u32 render_enable_addr_upper;
            u32 render_enable_addr_lower;
            RenderEnableMode render_enable_mode;
            INSERT_PADDING_WORDS_NOINIT(0x4);
            u32 clip_x0;
            u32 clip_y0;
            u32 clip_width;
            u32 clip_height;
            BitField<0, 1, u32> clip_enable;
            BitField<0, 3, ColorKeyFormat> color_key_format;
            u32 color_key;
            BitField<0, 1, u32> color_key_enable;
            BitField<0, 8, u32> rop;
            u32 beta1;
            Beta4 beta4;
            Operation operation;
            union {
                BitField<0, 6, u32> x;
                BitField<8, 6, u32> y;
            } pattern_offset;
            BitField<0, 2, PatternSelect> pattern_select;
            INSERT_PADDING_WORDS_NOINIT(0xC);
            struct {
                BitField<0, 3, MonochromePatternColorFormat> color_format;
                BitField<0, 1, MonochromePatternFormat> format;
                u32 color0;
                u32 color1;
                u32 pattern0;
                u32 pattern1;
            } monochrome_pattern;
            struct {
                std::array<u32, 0x40> X8R8G8B8;
                std::array<u32, 0x20> R5G6B5;
                std::array<u32, 0x20> X1R5G5B5;
                std::array<u32, 0x10> Y8;
            } color_pattern;
            INSERT_PADDING_WORDS_NOINIT(0x10);
            struct {
                u32 prim_mode;
                u32 prim_color_format;
                u32 prim_color;
                u32 line_tie_break_bits;
                INSERT_PADDING_WORDS_NOINIT(0x14);
                u32 prim_point_xy;
                INSERT_PADDING_WORDS_NOINIT(0x7);
                std::array<Point, 0x40> prim_point;
            } render_solid;
            struct {
                u32 data_type;
                u32 color_format;
                u32 index_format;
                u32 mono_format;
                u32 wrap;
                u32 color0;
                u32 color1;
                u32 mono_opacity;
                INSERT_PADDING_WORDS_NOINIT(0x6);
                u32 src_width;
                u32 src_height;
                u32 dx_du_frac;
                u32 dx_du_int;
                u32 dx_dv_frac;
                u32 dy_dv_int;
                u32 dst_x0_frac;
                u32 dst_x0_int;
                u32 dst_y0_frac;
                u32 dst_y0_int;
                u32 data;
            } pixels_from_cpu;
            INSERT_PADDING_WORDS_NOINIT(0x3);
            u32 big_endian_control;
            INSERT_PADDING_WORDS_NOINIT(0x3);
            struct {
                BitField<0, 3, u32> block_shape;
                BitField<0, 5, u32> corral_size;
                BitField<0, 1, u32> safe_overlap;
                union {
                    BitField<0, 1, Origin> origin;
                    BitField<4, 1, Filter> filter;
                } sample_mode;
                INSERT_PADDING_WORDS_NOINIT(0x8);
                s32 dst_x0;
                s32 dst_y0;
                s32 dst_width;
                s32 dst_height;
                s64 du_dx;
                s64 dv_dy;
                s64 src_x0;
                s64 src_y0;
            } pixels_from_memory;
        };
        std::array<u32, NUM_REGS> reg_array;
    } regs{};

    struct Config {
        Operation operation;
        Filter filter;
        bool must_accelerate;
        s32 dst_x0;
        s32 dst_y0;
        s32 dst_x1;
        s32 dst_y1;
        s32 src_x0;
        s32 src_y0;
        s32 src_x1;
        s32 src_y1;
    };

private:
    VideoCore::RasterizerInterface* rasterizer = nullptr;
    std::unique_ptr<Blitter::SoftwareBlitEngine> sw_blitter;
    MemoryManager& memory_manager;

    /// Performs the copy from the source surface to the destination surface as configured in the
    /// registers.
    void Blit();

    void ConsumeSinkImpl() override;
};

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Fermi2D::Regs, field_name) == position,                                 \
                  "Field " #field_name " has invalid position")

ASSERT_REG_POSITION(object, 0x0);
ASSERT_REG_POSITION(no_operation, 0x100);
ASSERT_REG_POSITION(notify, 0x104);
ASSERT_REG_POSITION(wait_for_idle, 0x110);
ASSERT_REG_POSITION(pm_trigger, 0x140);
ASSERT_REG_POSITION(context_dma_notify, 0x180);
ASSERT_REG_POSITION(dst_context_dma, 0x184);
ASSERT_REG_POSITION(src_context_dma, 0x188);
ASSERT_REG_POSITION(semaphore_context_dma, 0x18C);
ASSERT_REG_POSITION(dst, 0x200);
ASSERT_REG_POSITION(pixels_from_cpu_index_wrap, 0x228);
ASSERT_REG_POSITION(kind2d_check_enable, 0x22C);
ASSERT_REG_POSITION(src, 0x230);
ASSERT_REG_POSITION(pixels_from_memory_sector_promotion, 0x258);
ASSERT_REG_POSITION(num_tpcs, 0x260);
ASSERT_REG_POSITION(render_enable_addr_upper, 0x264);
ASSERT_REG_POSITION(render_enable_addr_lower, 0x268);
ASSERT_REG_POSITION(clip_x0, 0x280);
ASSERT_REG_POSITION(clip_y0, 0x284);
ASSERT_REG_POSITION(clip_width, 0x288);
ASSERT_REG_POSITION(clip_height, 0x28c);
ASSERT_REG_POSITION(clip_enable, 0x290);
ASSERT_REG_POSITION(color_key_format, 0x294);
ASSERT_REG_POSITION(color_key, 0x298);
ASSERT_REG_POSITION(rop, 0x2A0);
ASSERT_REG_POSITION(beta1, 0x2A4);
ASSERT_REG_POSITION(beta4, 0x2A8);
ASSERT_REG_POSITION(operation, 0x2AC);
ASSERT_REG_POSITION(pattern_offset, 0x2B0);
ASSERT_REG_POSITION(pattern_select, 0x2B4);
ASSERT_REG_POSITION(monochrome_pattern, 0x2E8);
ASSERT_REG_POSITION(color_pattern, 0x300);
ASSERT_REG_POSITION(render_solid, 0x580);
ASSERT_REG_POSITION(pixels_from_cpu, 0x800);
ASSERT_REG_POSITION(big_endian_control, 0x870);
ASSERT_REG_POSITION(pixels_from_memory, 0x880);

#undef ASSERT_REG_POSITION

} // namespace Tegra::Engines
