// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"

namespace Tegra {
class MemoryManager;
}

namespace VideoCore {
class RasterizerInterface;
}

namespace Tegra::Engines::Upload {

struct Registers {
    u32 line_length_in;
    u32 line_count;

    struct {
        u32 address_high;
        u32 address_low;
        u32 pitch;
        union {
            BitField<0, 4, u32> block_width;
            BitField<4, 4, u32> block_height;
            BitField<8, 4, u32> block_depth;
        };
        u32 width;
        u32 height;
        u32 depth;
        u32 z;
        u32 x;
        u32 y;

        GPUVAddr Address() const {
            return static_cast<GPUVAddr>((static_cast<GPUVAddr>(address_high) << 32) | address_low);
        }

        u32 BlockWidth() const {
            return block_width.Value();
        }

        u32 BlockHeight() const {
            return block_height.Value();
        }

        u32 BlockDepth() const {
            return block_depth.Value();
        }
    } dest;
};

class State {
public:
    explicit State(MemoryManager& memory_manager_, Registers& regs_);
    ~State();

    void ProcessExec(bool is_linear_);
    void ProcessData(u32 data, bool is_last_call);

    /// Binds a rasterizer to this engine.
    void BindRasterizer(VideoCore::RasterizerInterface* rasterizer);

private:
    u32 write_offset = 0;
    u32 copy_size = 0;
    std::vector<u8> inner_buffer;
    std::vector<u8> tmp_buffer;
    bool is_linear = false;
    Registers& regs;
    MemoryManager& memory_manager;
    VideoCore::RasterizerInterface* rasterizer = nullptr;
};

} // namespace Tegra::Engines::Upload
