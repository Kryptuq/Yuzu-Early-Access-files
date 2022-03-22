// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Tegra {

class GPU;
class Host1x;
class Nvdec;
class SyncptIncrManager;
class Vic;

enum class ChSubmissionMode : u32 {
    SetClass = 0,
    Incrementing = 1,
    NonIncrementing = 2,
    Mask = 3,
    Immediate = 4,
    Restart = 5,
    Gather = 6,
};

enum class ChClassId : u32 {
    NoClass = 0x0,
    Host1x = 0x1,
    VideoEncodeMpeg = 0x20,
    VideoEncodeNvEnc = 0x21,
    VideoStreamingVi = 0x30,
    VideoStreamingIsp = 0x32,
    VideoStreamingIspB = 0x34,
    VideoStreamingViI2c = 0x36,
    GraphicsVic = 0x5d,
    Graphics3D = 0x60,
    GraphicsGpu = 0x61,
    Tsec = 0xe0,
    TsecB = 0xe1,
    NvJpg = 0xc0,
    NvDec = 0xf0
};

union ChCommandHeader {
    u32 raw;
    BitField<0, 16, u32> value;
    BitField<16, 12, u32> method_offset;
    BitField<28, 4, ChSubmissionMode> submission_mode;
};
static_assert(sizeof(ChCommandHeader) == sizeof(u32), "ChCommand header is an invalid size");

struct ChCommand {
    ChClassId class_id{};
    int method_offset{};
    std::vector<u32> arguments;
};

using ChCommandHeaderList = std::vector<ChCommandHeader>;
using ChCommandList = std::vector<ChCommand>;

struct ThiRegisters {
    u32_le increment_syncpt{};
    INSERT_PADDING_WORDS(1);
    u32_le increment_syncpt_error{};
    u32_le ctx_switch_incremement_syncpt{};
    INSERT_PADDING_WORDS(4);
    u32_le ctx_switch{};
    INSERT_PADDING_WORDS(1);
    u32_le ctx_syncpt_eof{};
    INSERT_PADDING_WORDS(5);
    u32_le method_0{};
    u32_le method_1{};
    INSERT_PADDING_WORDS(12);
    u32_le int_status{};
    u32_le int_mask{};
};

enum class ThiMethod : u32 {
    IncSyncpt = offsetof(ThiRegisters, increment_syncpt) / sizeof(u32),
    SetMethod0 = offsetof(ThiRegisters, method_0) / sizeof(u32),
    SetMethod1 = offsetof(ThiRegisters, method_1) / sizeof(u32),
};

class CDmaPusher {
public:
    explicit CDmaPusher(GPU& gpu_);
    ~CDmaPusher();

    /// Process the command entry
    void ProcessEntries(ChCommandHeaderList&& entries);

private:
    /// Invoke command class devices to execute the command based on the current state
    void ExecuteCommand(u32 state_offset, u32 data);

    /// Write arguments value to the ThiRegisters member at the specified offset
    void ThiStateWrite(ThiRegisters& state, u32 offset, u32 argument);

    GPU& gpu;
    std::shared_ptr<Tegra::Nvdec> nvdec_processor;
    std::unique_ptr<Tegra::Vic> vic_processor;
    std::unique_ptr<Tegra::Host1x> host1x_processor;
    std::unique_ptr<SyncptIncrManager> sync_manager;
    ChClassId current_class{};
    ThiRegisters vic_thi_state{};
    ThiRegisters nvdec_thi_state{};

    u32 count{};
    u32 offset{};
    u32 mask{};
    bool incrementing{};
};

} // namespace Tegra
