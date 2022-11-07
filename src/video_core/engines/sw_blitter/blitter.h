// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "video_core/engines/fermi_2d.h"

namespace Tegra {
class MemoryManager;
}

namespace Tegra::Engines::Blitter {

class SoftwareBlitEngine {
public:
    explicit SoftwareBlitEngine(MemoryManager& memory_manager_);
    ~SoftwareBlitEngine();

    bool Blit(Fermi2D::Surface& src, Fermi2D::Surface& dst, Fermi2D::Config& copy_config);

private:
    MemoryManager& memory_manager;
    struct BlitEngineImpl;
    std::unique_ptr<BlitEngineImpl> impl;
};

} // namespace Tegra::Engines::Blitter
