// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <span>

#include "common/common_types.h"

#include "video_core/gpu.h"

namespace Tegra::Engines::Blitter {

class Converter {
public:
    virtual void ConvertTo(std::span<const u8> input, std::span<f32> output) = 0;
    virtual void ConvertFrom(std::span<const f32> input, std::span<u8> output) = 0;
    virtual ~Converter() = default;
};

class ConverterFactory {
public:
    ConverterFactory();
    ~ConverterFactory();

    Converter* GetFormatConverter(RenderTargetFormat format);

private:
    Converter* BuildConverter(RenderTargetFormat format);

    struct ConverterFactoryImpl;
    std::unique_ptr<ConverterFactoryImpl> impl;
};

} // namespace Tegra::Engines::Blitter
