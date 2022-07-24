// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "video_core/host1x/codecs/codec.h"

namespace Tegra {

namespace Host1x {

class Host1x;

class Nvdec {
public:
    explicit Nvdec(Host1x& host1x);
    ~Nvdec();

    /// Writes the method into the state, Invoke Execute() if encountered
    void ProcessMethod(u32 method, u32 argument);

    /// Return most recently decoded frame
    [[nodiscard]] AVFramePtr GetFrame();

private:
    /// Invoke codec to decode a frame
    void Execute();

    Host1x& host1x;
    NvdecCommon::NvdecRegisters state;
    std::unique_ptr<Codec> codec;
};

} // namespace Host1x

} // namespace Tegra
