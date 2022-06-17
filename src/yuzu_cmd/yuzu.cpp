// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <fmt/ostream.h>

#include "common/detached_tasks.h"
#include "common/logging/backend.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/nvidia_flags.h"
#include "common/scm_rev.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "common/string_util.h"
#include "common/telemetry.h"
#include "core/core.h"
#include "core/cpu_manager.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/vfs_real.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/loader/loader.h"
#include "core/telemetry_session.h"
#include "input_common/main.h"
#include "video_core/renderer_base.h"
#include "yuzu_cmd/config.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2_gl.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2_vk.h"

#ifdef _WIN32
// windows.h needs to be included before shellapi.h
#include <windows.h>

#include <shellapi.h>
#endif

#undef _UNICODE
#include <getopt.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef _WIN32
extern "C" {
// tells Nvidia and AMD drivers to use the dedicated GPU by default on laptops with switchable
// graphics
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void PrintHelp(const char* argv0) {
    std::cout << "Usage: " << argv0
              << " [options] <filename>\n"
                 "-f, --fullscreen      Start in fullscreen mode\n"
                 "-h, --help            Display this help and exit\n"
                 "-v, --version         Output version information and exit\n"
                 "-p, --program         Pass following string as arguments to executable\n"
                 "-c, --config          Load the specified configuration file\n";
}

static void PrintVersion() {
    std::cout << "yuzu " << Common::g_scm_branch << " " << Common::g_scm_desc << std::endl;
}

/// Application entry point
int main(int argc, char** argv) {
    Common::Log::Initialize();
    Common::Log::SetColorConsoleBackendEnabled(true);
    Common::Log::Start();
    Common::DetachedTasks detached_tasks;

    int option_index = 0;
#ifdef _WIN32
    int argc_w;
    auto argv_w = CommandLineToArgvW(GetCommandLineW(), &argc_w);

    if (argv_w == nullptr) {
        LOG_CRITICAL(Frontend, "Failed to get command line arguments");
        return -1;
    }
#endif
    std::string filepath;
    std::optional<std::string> config_path;
    std::string program_args;

    bool fullscreen = false;

    static struct option long_options[] = {
        // clang-format off
        {"fullscreen", no_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"program", optional_argument, 0, 'p'},
        {"config", required_argument, 0, 'c'},
        {0, 0, 0, 0},
        // clang-format on
    };

    while (optind < argc) {
        int arg = getopt_long(argc, argv, "g:fhvp::c:", long_options, &option_index);
        if (arg != -1) {
            switch (static_cast<char>(arg)) {
            case 'f':
                fullscreen = true;
                LOG_INFO(Frontend, "Starting in fullscreen mode...");
                break;
            case 'h':
                PrintHelp(argv[0]);
                return 0;
            case 'v':
                PrintVersion();
                return 0;
            case 'p':
                program_args = argv[optind];
                ++optind;
                break;
            case 'c':
                config_path = optarg;
                break;
            }
        } else {
#ifdef _WIN32
            filepath = Common::UTF16ToUTF8(argv_w[optind]);
#else
            filepath = argv[optind];
#endif
            optind++;
        }
    }

    Config config{config_path};

    // apply the log_filter setting
    // the logger was initialized before and doesn't pick up the filter on its own
    Common::Log::Filter filter;
    filter.ParseFilterString(Settings::values.log_filter.GetValue());
    Common::Log::SetGlobalFilter(filter);

    if (!program_args.empty()) {
        Settings::values.program_args = program_args;
    }

#ifdef _WIN32
    LocalFree(argv_w);
#endif

    MicroProfileOnThreadCreate("EmuThread");
    SCOPE_EXIT({ MicroProfileShutdown(); });

    Common::ConfigureNvidiaEnvironmentFlags();

    if (filepath.empty()) {
        LOG_CRITICAL(Frontend, "Failed to load ROM: No ROM specified");
        return -1;
    }

    Core::System system{};
    InputCommon::InputSubsystem input_subsystem{};

    // Apply the command line arguments
    system.ApplySettings();

    std::unique_ptr<EmuWindow_SDL2> emu_window;
    switch (Settings::values.renderer_backend.GetValue()) {
    case Settings::RendererBackend::OpenGL:
        emu_window = std::make_unique<EmuWindow_SDL2_GL>(&input_subsystem, system, fullscreen);
        break;
    case Settings::RendererBackend::Vulkan:
        emu_window = std::make_unique<EmuWindow_SDL2_VK>(&input_subsystem, system, fullscreen);
        break;
    }

    system.SetContentProvider(std::make_unique<FileSys::ContentProviderUnion>());
    system.SetFilesystem(std::make_shared<FileSys::RealVfsFilesystem>());
    system.GetFileSystemController().CreateFactories(*system.GetFilesystem());

    const Core::SystemResultStatus load_result{system.Load(*emu_window, filepath)};

    switch (load_result) {
    case Core::SystemResultStatus::ErrorGetLoader:
        LOG_CRITICAL(Frontend, "Failed to obtain loader for {}!", filepath);
        return -1;
    case Core::SystemResultStatus::ErrorLoader:
        LOG_CRITICAL(Frontend, "Failed to load ROM!");
        return -1;
    case Core::SystemResultStatus::ErrorNotInitialized:
        LOG_CRITICAL(Frontend, "CPUCore not initialized");
        return -1;
    case Core::SystemResultStatus::ErrorVideoCore:
        LOG_CRITICAL(Frontend, "Failed to initialize VideoCore!");
        return -1;
    case Core::SystemResultStatus::Success:
        break; // Expected case
    default:
        if (static_cast<u32>(load_result) >
            static_cast<u32>(Core::SystemResultStatus::ErrorLoader)) {
            const u16 loader_id = static_cast<u16>(Core::SystemResultStatus::ErrorLoader);
            const u16 error_id = static_cast<u16>(load_result) - loader_id;
            LOG_CRITICAL(Frontend,
                         "While attempting to load the ROM requested, an error occurred. Please "
                         "refer to the yuzu wiki for more information or the yuzu discord for "
                         "additional help.\n\nError Code: {:04X}-{:04X}\nError Description: {}",
                         loader_id, error_id, static_cast<Loader::ResultStatus>(error_id));
        }
    }

    system.TelemetrySession().AddField(Common::Telemetry::FieldType::App, "Frontend", "SDL");

    // Core is loaded, start the GPU (makes the GPU contexts current to this thread)
    system.GPU().Start();
    system.GetCpuManager().OnGpuReady();

    if (Settings::values.use_disk_shader_cache.GetValue()) {
        system.Renderer().ReadRasterizer()->LoadDiskResources(
            system.GetCurrentProcessProgramID(), std::stop_token{},
            [](VideoCore::LoadCallbackStage, size_t value, size_t total) {});
    }

    system.RegisterExitCallback([&] {
        // Just exit right away.
        exit(0);
    });

    void(system.Run());
    if (system.DebuggerEnabled()) {
        system.InitializeDebugger();
    }
    while (emu_window->IsOpen()) {
        emu_window->WaitEvent();
    }
    system.DetachDebugger();
    void(system.Pause());
    system.Shutdown();

    detached_tasks.WaitForAllTasks();
    return 0;
}
