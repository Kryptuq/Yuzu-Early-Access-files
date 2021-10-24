// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

namespace Core {
class System;
}

namespace Core::Frontend {
class EmuWindow;
}

namespace Tegra {
class GPU;
}

namespace VideoCore {

class RendererBase;

/// Creates an emulated GPU instance using the given system context.
std::unique_ptr<Tegra::GPU> CreateGPU(Core::Frontend::EmuWindow& emu_window, Core::System& system);

float GetResolutionScaleFactor(const RendererBase& renderer);

} // namespace VideoCore
