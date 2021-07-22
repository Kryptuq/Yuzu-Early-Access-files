// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

class EmuWindow_SDL2_VK final : public EmuWindow_SDL2 {
public:
    explicit EmuWindow_SDL2_VK(InputCommon::InputSubsystem* input_subsystem, bool fullscreen);
    ~EmuWindow_SDL2_VK() override;

    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override;
};

class DummyContext : public Core::Frontend::GraphicsContext {};
