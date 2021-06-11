// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstdlib>
#include <memory>
#include <string>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2_vk.h"

// Include these late to avoid polluting everything with Xlib macros
// Ignore -Wimplicit-fallthrough due to https://github.com/libsdl-org/SDL/issues/4307
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif
#include <SDL.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <SDL_syswm.h>

EmuWindow_SDL2_VK::EmuWindow_SDL2_VK(InputCommon::InputSubsystem* input_subsystem)
    : EmuWindow_SDL2{input_subsystem} {
    const std::string window_title = fmt::format("yuzu {} | {}-{} (Vulkan)", Common::g_build_name,
                                                 Common::g_scm_branch, Common::g_scm_desc);
    render_window =
        SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_SysWMinfo wm;
    SDL_VERSION(&wm.version);
    if (SDL_GetWindowWMInfo(render_window, &wm) == SDL_FALSE) {
        LOG_CRITICAL(Frontend, "Failed to get information from the window manager");
        std::exit(EXIT_FAILURE);
    }

    SetWindowIcon();

    switch (wm.subsystem) {
#ifdef SDL_VIDEO_DRIVER_WINDOWS
    case SDL_SYSWM_TYPE::SDL_SYSWM_WINDOWS:
        window_info.type = Core::Frontend::WindowSystemType::Windows;
        window_info.render_surface = reinterpret_cast<void*>(wm.info.win.window);
        break;
#endif
#ifdef SDL_VIDEO_DRIVER_X11
    case SDL_SYSWM_TYPE::SDL_SYSWM_X11:
        window_info.type = Core::Frontend::WindowSystemType::X11;
        window_info.display_connection = wm.info.x11.display;
        window_info.render_surface = reinterpret_cast<void*>(wm.info.x11.window);
        break;
#endif
#ifdef SDL_VIDEO_DRIVER_WAYLAND
    case SDL_SYSWM_TYPE::SDL_SYSWM_WAYLAND:
        window_info.type = Core::Frontend::WindowSystemType::Wayland;
        window_info.display_connection = wm.info.wl.display;
        window_info.render_surface = wm.info.wl.surface;
        break;
#endif
    default:
        LOG_CRITICAL(Frontend, "Window manager subsystem not implemented");
        std::exit(EXIT_FAILURE);
    }

    OnResize();
    OnMinimalClientAreaChangeRequest(GetActiveConfig().min_client_area_size);
    SDL_PumpEvents();
    LOG_INFO(Frontend, "yuzu Version: {} | {}-{} (Vulkan)", Common::g_build_name,
             Common::g_scm_branch, Common::g_scm_desc);
}

EmuWindow_SDL2_VK::~EmuWindow_SDL2_VK() = default;

std::unique_ptr<Core::Frontend::GraphicsContext> EmuWindow_SDL2_VK::CreateSharedContext() const {
    return std::make_unique<DummyContext>();
}
