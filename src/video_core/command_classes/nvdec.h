// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "video_core/command_classes/codecs/codec.h"

namespace Tegra {
class GPU;

class Nvdec {
public:
    explicit Nvdec(GPU& gpu);
    ~Nvdec();

    /// Writes the method into the state, Invoke Execute() if encountered
    void ProcessMethod(u32 method, u32 argument);

    /// Return most recently decoded frame
    [[nodiscard]] AVFramePtr GetFrame();

private:
    /// Invoke codec to decode a frame
    void Execute();

    GPU& gpu;
    NvdecCommon::NvdecRegisters state;
    std::unique_ptr<Codec> codec;
};
} // namespace Tegra
