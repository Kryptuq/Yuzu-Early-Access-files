// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "core/frontend/emu_window.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2.h"

namespace Core {
class System;
}

namespace InputCommon {
class InputSubsystem;
}

class EmuWindow_SDL2_GL final : public EmuWindow_SDL2 {
public:
    explicit EmuWindow_SDL2_GL(InputCommon::InputSubsystem* input_subsystem_, Core::System& system_,
                               bool fullscreen);
    ~EmuWindow_SDL2_GL();

    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override;

private:
    /// Whether the GPU and driver supports the OpenGL extension required
    bool SupportsRequiredGLExtensions();

    using SDL_GLContext = void*;

    /// The OpenGL context associated with the window
    SDL_GLContext window_context;

    /// The OpenGL context associated with the core
    std::unique_ptr<Core::Frontend::GraphicsContext> core_context;
};
