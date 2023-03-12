// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <exception>
#include <memory>
#include <utility>

#include "audio_core/audio_core.h"
#include "common/fs/fs.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/settings.h"
#include "common/string_util.h"
#include "core/arm/exclusive_monitor.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu_manager.h"
#include "core/debugger/debugger.h"
#include "core/device_memory.h"
#include "core/file_sys/bis_factory.h"
#include "core/file_sys/mode.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/savedata_factory.h"
#include "core/file_sys/vfs_concat.h"
#include "core/file_sys/vfs_real.h"
#include "core/hid/hid_core.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"
#include "core/hle/service/am/applets/applets.h"
#include "core/hle/service/apm/apm_controller.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/glue/glue_manager.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"
#include "core/hle/service/time/time_manager.h"
#include "core/internal_network/network.h"
#include "core/loader/loader.h"
#include "core/memory.h"
#include "core/memory/cheat_engine.h"
#include "core/perf_stats.h"
#include "core/reporter.h"
#include "core/telemetry_session.h"
#include "core/tools/freezer.h"
#include "network/network.h"
#include "video_core/host1x/host1x.h"
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"

MICROPROFILE_DEFINE(ARM_Jit_Dynarmic_CPU0, "ARM JIT", "Dynarmic CPU 0", MP_RGB(255, 64, 64));
MICROPROFILE_DEFINE(ARM_Jit_Dynarmic_CPU1, "ARM JIT", "Dynarmic CPU 1", MP_RGB(255, 64, 64));
MICROPROFILE_DEFINE(ARM_Jit_Dynarmic_CPU2, "ARM JIT", "Dynarmic CPU 2", MP_RGB(255, 64, 64));
MICROPROFILE_DEFINE(ARM_Jit_Dynarmic_CPU3, "ARM JIT", "Dynarmic CPU 3", MP_RGB(255, 64, 64));

namespace Core {

namespace {

FileSys::StorageId GetStorageIdForFrontendSlot(
    std::optional<FileSys::ContentProviderUnionSlot> slot) {
    if (!slot.has_value()) {
        return FileSys::StorageId::None;
    }

    switch (*slot) {
    case FileSys::ContentProviderUnionSlot::UserNAND:
        return FileSys::StorageId::NandUser;
    case FileSys::ContentProviderUnionSlot::SysNAND:
        return FileSys::StorageId::NandSystem;
    case FileSys::ContentProviderUnionSlot::SDMC:
        return FileSys::StorageId::SdCard;
    case FileSys::ContentProviderUnionSlot::FrontendManual:
        return FileSys::StorageId::Host;
    default:
        return FileSys::StorageId::None;
    }
}

} // Anonymous namespace

FileSys::VirtualFile GetGameFileFromPath(const FileSys::VirtualFilesystem& vfs,
                                         const std::string& path) {
    // To account for split 00+01+etc files.
    std::string dir_name;
    std::string filename;
    Common::SplitPath(path, &dir_name, &filename, nullptr);

    if (filename == "00") {
        const auto dir = vfs->OpenDirectory(dir_name, FileSys::Mode::Read);
        std::vector<FileSys::VirtualFile> concat;

        for (u32 i = 0; i < 0x10; ++i) {
            const auto file_name = fmt::format("{:02X}", i);
            auto next = dir->GetFile(file_name);

            if (next != nullptr) {
                concat.push_back(std::move(next));
            } else {
                next = dir->GetFile(file_name);

                if (next == nullptr) {
                    break;
                }

                concat.push_back(std::move(next));
            }
        }

        if (concat.empty()) {
            return nullptr;
        }

        return FileSys::ConcatenatedVfsFile::MakeConcatenatedFile(std::move(concat),
                                                                  dir->GetName());
    }

    if (Common::FS::IsDir(path)) {
        return vfs->OpenFile(path + "/main", FileSys::Mode::Read);
    }

    return vfs->OpenFile(path, FileSys::Mode::Read);
}

struct System::Impl {
    explicit Impl(System& system)
        : kernel{system}, fs_controller{system}, memory{system}, hid_core{}, room_network{},
          cpu_manager{system}, reporter{system}, applet_manager{system}, time_manager{system} {}

    void Initialize(System& system) {
        device_memory = std::make_unique<Core::DeviceMemory>();

        is_multicore = Settings::values.use_multi_core.GetValue();
        extended_memory_layout = Settings::values.use_extended_memory_layout.GetValue();

        core_timing.SetMulticore(is_multicore);
        core_timing.Initialize([&system]() { system.RegisterHostThread(); });

        const auto posix_time = std::chrono::system_clock::now().time_since_epoch();
        const auto current_time =
            std::chrono::duration_cast<std::chrono::seconds>(posix_time).count();
        Settings::values.custom_rtc_differential =
            Settings::values.custom_rtc.value_or(current_time) - current_time;

        // Create a default fs if one doesn't already exist.
        if (virtual_filesystem == nullptr) {
            virtual_filesystem = std::make_shared<FileSys::RealVfsFilesystem>();
        }
        if (content_provider == nullptr) {
            content_provider = std::make_unique<FileSys::ContentProviderUnion>();
        }

        // Create default implementations of applets if one is not provided.
        applet_manager.SetDefaultAppletsIfMissing();

        is_async_gpu = Settings::values.use_asynchronous_gpu_emulation.GetValue();

        kernel.SetMulticore(is_multicore);
        cpu_manager.SetMulticore(is_multicore);
        cpu_manager.SetAsyncGpu(is_async_gpu);
    }

    void ReinitializeIfNecessary(System& system) {
        const bool must_reinitialize =
            is_multicore != Settings::values.use_multi_core.GetValue() ||
            extended_memory_layout != Settings::values.use_extended_memory_layout.GetValue();

        if (!must_reinitialize) {
            return;
        }

        LOG_DEBUG(Kernel, "Re-initializing");

        is_multicore = Settings::values.use_multi_core.GetValue();
        extended_memory_layout = Settings::values.use_extended_memory_layout.GetValue();

        Initialize(system);
    }

    void Run() {
        std::unique_lock<std::mutex> lk(suspend_guard);

        kernel.SuspendApplication(false);
        core_timing.SyncPause(false);
        is_paused.store(false, std::memory_order_relaxed);
    }

    void Pause() {
        std::unique_lock<std::mutex> lk(suspend_guard);

        core_timing.SyncPause(true);
        kernel.SuspendApplication(true);
        is_paused.store(true, std::memory_order_relaxed);
    }

    bool IsPaused() const {
        return is_paused.load(std::memory_order_relaxed);
    }

    std::unique_lock<std::mutex> StallApplication() {
        std::unique_lock<std::mutex> lk(suspend_guard);
        kernel.SuspendApplication(true);
        core_timing.SyncPause(true);
        return lk;
    }

    void UnstallApplication() {
        if (!IsPaused()) {
            core_timing.SyncPause(false);
            kernel.SuspendApplication(false);
        }
    }

    void InitializeDebugger(System& system, u16 port) {
        debugger = std::make_unique<Debugger>(system, port);
    }

    SystemResultStatus SetupForApplicationProcess(System& system, Frontend::EmuWindow& emu_window) {
        LOG_DEBUG(Core, "initialized OK");

        // Setting changes may require a full system reinitialization (e.g., disabling multicore).
        ReinitializeIfNecessary(system);

        kernel.Initialize();
        cpu_manager.Initialize();

        /// Reset all glue registrations
        arp_manager.ResetAll();

        telemetry_session = std::make_unique<Core::TelemetrySession>();

        host1x_core = std::make_unique<Tegra::Host1x::Host1x>(system);
        gpu_core = VideoCore::CreateGPU(emu_window, system);
        if (!gpu_core) {
            return SystemResultStatus::ErrorVideoCore;
        }

        audio_core = std::make_unique<AudioCore::AudioCore>(system);

        service_manager = std::make_shared<Service::SM::ServiceManager>(kernel);
        services = std::make_unique<Service::Services>(service_manager, system);

        // Initialize time manager, which must happen after kernel is created
        time_manager.Initialize();

        is_powered_on = true;
        exit_lock = false;

        microprofile_dynarmic[0] = MICROPROFILE_TOKEN(ARM_Jit_Dynarmic_CPU0);
        microprofile_dynarmic[1] = MICROPROFILE_TOKEN(ARM_Jit_Dynarmic_CPU1);
        microprofile_dynarmic[2] = MICROPROFILE_TOKEN(ARM_Jit_Dynarmic_CPU2);
        microprofile_dynarmic[3] = MICROPROFILE_TOKEN(ARM_Jit_Dynarmic_CPU3);

        LOG_DEBUG(Core, "Initialized OK");

        return SystemResultStatus::Success;
    }

    SystemResultStatus Load(System& system, Frontend::EmuWindow& emu_window,
                            const std::string& filepath, u64 program_id,
                            std::size_t program_index) {
        app_loader = Loader::GetLoader(system, GetGameFileFromPath(virtual_filesystem, filepath),
                                       program_id, program_index);

        if (!app_loader) {
            LOG_CRITICAL(Core, "Failed to obtain loader for {}!", filepath);
            return SystemResultStatus::ErrorGetLoader;
        }

        SystemResultStatus init_result{SetupForApplicationProcess(system, emu_window)};
        if (init_result != SystemResultStatus::Success) {
            LOG_CRITICAL(Core, "Failed to initialize system (Error {})!",
                         static_cast<int>(init_result));
            ShutdownMainProcess();
            return init_result;
        }

        telemetry_session->AddInitialInfo(*app_loader, fs_controller, *content_provider);

        // Create a resource limit for the process.
        const auto physical_memory_size =
            kernel.MemoryManager().GetSize(Kernel::KMemoryManager::Pool::Application);
        auto* resource_limit = Kernel::CreateResourceLimitForProcess(system, physical_memory_size);

        // Create the process.
        auto main_process = Kernel::KProcess::Create(system.Kernel());
        ASSERT(Kernel::KProcess::Initialize(main_process, system, "main",
                                            Kernel::KProcess::ProcessType::Userland, resource_limit)
                   .IsSuccess());
        const auto [load_result, load_parameters] = app_loader->Load(*main_process, system);
        if (load_result != Loader::ResultStatus::Success) {
            LOG_CRITICAL(Core, "Failed to load ROM (Error {})!", load_result);
            ShutdownMainProcess();

            return static_cast<SystemResultStatus>(
                static_cast<u32>(SystemResultStatus::ErrorLoader) + static_cast<u32>(load_result));
        }
        AddGlueRegistrationForProcess(*app_loader, *main_process);
        kernel.MakeApplicationProcess(main_process);
        kernel.InitializeCores();

        // Initialize cheat engine
        if (cheat_engine) {
            cheat_engine->Initialize();
        }

        // All threads are started, begin main process execution, now that we're in the clear.
        main_process->Run(load_parameters->main_thread_priority,
                          load_parameters->main_thread_stack_size);

        if (Settings::values.gamecard_inserted) {
            if (Settings::values.gamecard_current_game) {
                fs_controller.SetGameCard(GetGameFileFromPath(virtual_filesystem, filepath));
            } else if (!Settings::values.gamecard_path.GetValue().empty()) {
                const auto& gamecard_path = Settings::values.gamecard_path.GetValue();
                fs_controller.SetGameCard(GetGameFileFromPath(virtual_filesystem, gamecard_path));
            }
        }

        if (app_loader->ReadProgramId(program_id) != Loader::ResultStatus::Success) {
            LOG_ERROR(Core, "Failed to find title id for ROM (Error {})", load_result);
        }
        perf_stats = std::make_unique<PerfStats>(program_id);
        // Reset counters and set time origin to current frame
        GetAndResetPerfStats();
        perf_stats->BeginSystemFrame();

        std::string name = "Unknown Game";
        if (app_loader->ReadTitle(name) != Loader::ResultStatus::Success) {
            LOG_ERROR(Core, "Failed to read title for ROM (Error {})", load_result);
        }

        std::string title_version;
        const FileSys::PatchManager pm(program_id, system.GetFileSystemController(),
                                       system.GetContentProvider());
        const auto metadata = pm.GetControlMetadata();
        if (metadata.first != nullptr) {
            title_version = metadata.first->GetVersionString();
        }
        if (auto room_member = room_network.GetRoomMember().lock()) {
            Network::GameInfo game_info;
            game_info.name = name;
            game_info.id = program_id;
            game_info.version = title_version;
            room_member->SendGameInfo(game_info);
        }

        status = SystemResultStatus::Success;
        return status;
    }

    void ShutdownMainProcess() {
        SetShuttingDown(true);

        // Log last frame performance stats if game was loded
        if (perf_stats) {
            const auto perf_results = GetAndResetPerfStats();
            constexpr auto performance = Common::Telemetry::FieldType::Performance;

            telemetry_session->AddField(performance, "Shutdown_EmulationSpeed",
                                        perf_results.emulation_speed * 100.0);
            telemetry_session->AddField(performance, "Shutdown_Framerate",
                                        perf_results.average_game_fps);
            telemetry_session->AddField(performance, "Shutdown_Frametime",
                                        perf_results.frametime * 1000.0);
            telemetry_session->AddField(performance, "Mean_Frametime_MS",
                                        perf_stats->GetMeanFrametime());
        }

        is_powered_on = false;
        exit_lock = false;

        if (gpu_core != nullptr) {
            gpu_core->NotifyShutdown();
        }

        kernel.SuspendApplication(true);
        if (services) {
            services->KillNVNFlinger();
        }
        kernel.CloseServices();
        services.reset();
        service_manager.reset();
        cheat_engine.reset();
        telemetry_session.reset();
        time_manager.Shutdown();
        core_timing.ClearPendingEvents();
        app_loader.reset();
        audio_core.reset();
        gpu_core.reset();
        host1x_core.reset();
        perf_stats.reset();
        kernel.ShutdownCores();
        cpu_manager.Shutdown();
        debugger.reset();
        kernel.Shutdown();
        memory.Reset();

        if (auto room_member = room_network.GetRoomMember().lock()) {
            Network::GameInfo game_info{};
            room_member->SendGameInfo(game_info);
        }

        LOG_DEBUG(Core, "Shutdown OK");
    }

    bool IsShuttingDown() const {
        return is_shutting_down;
    }

    void SetShuttingDown(bool shutting_down) {
        is_shutting_down = shutting_down;
    }

    Loader::ResultStatus GetGameName(std::string& out) const {
        if (app_loader == nullptr)
            return Loader::ResultStatus::ErrorNotInitialized;
        return app_loader->ReadTitle(out);
    }

    void AddGlueRegistrationForProcess(Loader::AppLoader& loader, Kernel::KProcess& process) {
        std::vector<u8> nacp_data;
        FileSys::NACP nacp;
        if (loader.ReadControlData(nacp) == Loader::ResultStatus::Success) {
            nacp_data = nacp.GetRawBytes();
        } else {
            nacp_data.resize(sizeof(FileSys::RawNACP));
        }

        Service::Glue::ApplicationLaunchProperty launch{};
        launch.title_id = process.GetProgramId();

        FileSys::PatchManager pm{launch.title_id, fs_controller, *content_provider};
        launch.version = pm.GetGameVersion().value_or(0);

        // TODO(DarkLordZach): When FSController/Game Card Support is added, if
        // current_process_game_card use correct StorageId
        launch.base_game_storage_id = GetStorageIdForFrontendSlot(content_provider->GetSlotForEntry(
            launch.title_id, FileSys::ContentRecordType::Program));
        launch.update_storage_id = GetStorageIdForFrontendSlot(content_provider->GetSlotForEntry(
            FileSys::GetUpdateTitleID(launch.title_id), FileSys::ContentRecordType::Program));

        arp_manager.Register(launch.title_id, launch, std::move(nacp_data));
    }

    void SetStatus(SystemResultStatus new_status, const char* details = nullptr) {
        status = new_status;
        if (details) {
            status_details = details;
        }
    }

    PerfStatsResults GetAndResetPerfStats() {
        return perf_stats->GetAndResetStats(core_timing.GetGlobalTimeUs());
    }

    mutable std::mutex suspend_guard;
    std::atomic_bool is_paused{};
    std::atomic<bool> is_shutting_down{};

    Timing::CoreTiming core_timing;
    Kernel::KernelCore kernel;
    /// RealVfsFilesystem instance
    FileSys::VirtualFilesystem virtual_filesystem;
    /// ContentProviderUnion instance
    std::unique_ptr<FileSys::ContentProviderUnion> content_provider;
    Service::FileSystem::FileSystemController fs_controller;
    /// AppLoader used to load the current executing application
    std::unique_ptr<Loader::AppLoader> app_loader;
    std::unique_ptr<Tegra::GPU> gpu_core;
    std::unique_ptr<Tegra::Host1x::Host1x> host1x_core;
    std::unique_ptr<Core::DeviceMemory> device_memory;
    std::unique_ptr<AudioCore::AudioCore> audio_core;
    Core::Memory::Memory memory;
    Core::HID::HIDCore hid_core;
    Network::RoomNetwork room_network;

    CpuManager cpu_manager;
    std::atomic_bool is_powered_on{};
    bool exit_lock = false;

    Reporter reporter;
    std::unique_ptr<Memory::CheatEngine> cheat_engine;
    std::unique_ptr<Tools::Freezer> memory_freezer;
    std::array<u8, 0x20> build_id{};

    /// Frontend applets
    Service::AM::Applets::AppletManager applet_manager;

    /// APM (Performance) services
    Service::APM::Controller apm_controller{core_timing};

    /// Service State
    Service::Glue::ARPManager arp_manager;
    Service::Time::TimeManager time_manager;

    /// Service manager
    std::shared_ptr<Service::SM::ServiceManager> service_manager;

    /// Services
    std::unique_ptr<Service::Services> services;

    /// Telemetry session for this emulation session
    std::unique_ptr<Core::TelemetrySession> telemetry_session;

    /// Network instance
    Network::NetworkInstance network_instance;

    /// Debugger
    std::unique_ptr<Core::Debugger> debugger;

    SystemResultStatus status = SystemResultStatus::Success;
    std::string status_details = "";

    std::unique_ptr<Core::PerfStats> perf_stats;
    Core::SpeedLimiter speed_limiter;

    bool is_multicore{};
    bool is_async_gpu{};
    bool extended_memory_layout{};

    ExecuteProgramCallback execute_program_callback;
    ExitCallback exit_callback;

    std::array<u64, Core::Hardware::NUM_CPU_CORES> dynarmic_ticks{};
    std::array<MicroProfileToken, Core::Hardware::NUM_CPU_CORES> microprofile_dynarmic{};
};

System::System() : impl{std::make_unique<Impl>(*this)} {}

System::~System() = default;

CpuManager& System::GetCpuManager() {
    return impl->cpu_manager;
}

const CpuManager& System::GetCpuManager() const {
    return impl->cpu_manager;
}

void System::Initialize() {
    impl->Initialize(*this);
}

void System::Run() {
    impl->Run();
}

void System::Pause() {
    impl->Pause();
}

bool System::IsPaused() const {
    return impl->IsPaused();
}

void System::InvalidateCpuInstructionCaches() {
    impl->kernel.InvalidateAllInstructionCaches();
}

void System::InvalidateCpuInstructionCacheRange(VAddr addr, std::size_t size) {
    impl->kernel.InvalidateCpuInstructionCacheRange(addr, size);
}

void System::ShutdownMainProcess() {
    impl->ShutdownMainProcess();
}

bool System::IsShuttingDown() const {
    return impl->IsShuttingDown();
}

void System::SetShuttingDown(bool shutting_down) {
    impl->SetShuttingDown(shutting_down);
}

void System::DetachDebugger() {
    if (impl->debugger) {
        impl->debugger->NotifyShutdown();
    }
}

std::unique_lock<std::mutex> System::StallApplication() {
    return impl->StallApplication();
}

void System::UnstallApplication() {
    impl->UnstallApplication();
}

void System::InitializeDebugger() {
    impl->InitializeDebugger(*this, Settings::values.gdbstub_port.GetValue());
}

SystemResultStatus System::Load(Frontend::EmuWindow& emu_window, const std::string& filepath,
                                u64 program_id, std::size_t program_index) {
    return impl->Load(*this, emu_window, filepath, program_id, program_index);
}

bool System::IsPoweredOn() const {
    return impl->is_powered_on.load(std::memory_order::relaxed);
}

void System::PrepareReschedule(const u32 core_index) {
    impl->kernel.PrepareReschedule(core_index);
}

PerfStatsResults System::GetAndResetPerfStats() {
    return impl->GetAndResetPerfStats();
}

TelemetrySession& System::TelemetrySession() {
    return *impl->telemetry_session;
}

const TelemetrySession& System::TelemetrySession() const {
    return *impl->telemetry_session;
}

ARM_Interface& System::CurrentArmInterface() {
    return impl->kernel.CurrentPhysicalCore().ArmInterface();
}

const ARM_Interface& System::CurrentArmInterface() const {
    return impl->kernel.CurrentPhysicalCore().ArmInterface();
}

Kernel::PhysicalCore& System::CurrentPhysicalCore() {
    return impl->kernel.CurrentPhysicalCore();
}

const Kernel::PhysicalCore& System::CurrentPhysicalCore() const {
    return impl->kernel.CurrentPhysicalCore();
}

/// Gets the global scheduler
Kernel::GlobalSchedulerContext& System::GlobalSchedulerContext() {
    return impl->kernel.GlobalSchedulerContext();
}

/// Gets the global scheduler
const Kernel::GlobalSchedulerContext& System::GlobalSchedulerContext() const {
    return impl->kernel.GlobalSchedulerContext();
}

Kernel::KProcess* System::ApplicationProcess() {
    return impl->kernel.ApplicationProcess();
}

Core::DeviceMemory& System::DeviceMemory() {
    return *impl->device_memory;
}

const Core::DeviceMemory& System::DeviceMemory() const {
    return *impl->device_memory;
}

const Kernel::KProcess* System::ApplicationProcess() const {
    return impl->kernel.ApplicationProcess();
}

ARM_Interface& System::ArmInterface(std::size_t core_index) {
    return impl->kernel.PhysicalCore(core_index).ArmInterface();
}

const ARM_Interface& System::ArmInterface(std::size_t core_index) const {
    return impl->kernel.PhysicalCore(core_index).ArmInterface();
}

ExclusiveMonitor& System::Monitor() {
    return impl->kernel.GetExclusiveMonitor();
}

const ExclusiveMonitor& System::Monitor() const {
    return impl->kernel.GetExclusiveMonitor();
}

Memory::Memory& System::Memory() {
    return impl->memory;
}

const Core::Memory::Memory& System::Memory() const {
    return impl->memory;
}

Tegra::GPU& System::GPU() {
    return *impl->gpu_core;
}

const Tegra::GPU& System::GPU() const {
    return *impl->gpu_core;
}

Tegra::Host1x::Host1x& System::Host1x() {
    return *impl->host1x_core;
}

const Tegra::Host1x::Host1x& System::Host1x() const {
    return *impl->host1x_core;
}

VideoCore::RendererBase& System::Renderer() {
    return impl->gpu_core->Renderer();
}

const VideoCore::RendererBase& System::Renderer() const {
    return impl->gpu_core->Renderer();
}

Kernel::KernelCore& System::Kernel() {
    return impl->kernel;
}

const Kernel::KernelCore& System::Kernel() const {
    return impl->kernel;
}

HID::HIDCore& System::HIDCore() {
    return impl->hid_core;
}

const HID::HIDCore& System::HIDCore() const {
    return impl->hid_core;
}

AudioCore::AudioCore& System::AudioCore() {
    return *impl->audio_core;
}

const AudioCore::AudioCore& System::AudioCore() const {
    return *impl->audio_core;
}

Timing::CoreTiming& System::CoreTiming() {
    return impl->core_timing;
}

const Timing::CoreTiming& System::CoreTiming() const {
    return impl->core_timing;
}

Core::PerfStats& System::GetPerfStats() {
    return *impl->perf_stats;
}

const Core::PerfStats& System::GetPerfStats() const {
    return *impl->perf_stats;
}

Core::SpeedLimiter& System::SpeedLimiter() {
    return impl->speed_limiter;
}

const Core::SpeedLimiter& System::SpeedLimiter() const {
    return impl->speed_limiter;
}

u64 System::GetApplicationProcessProgramID() const {
    return impl->kernel.ApplicationProcess()->GetProgramId();
}

Loader::ResultStatus System::GetGameName(std::string& out) const {
    return impl->GetGameName(out);
}

void System::SetStatus(SystemResultStatus new_status, const char* details) {
    impl->SetStatus(new_status, details);
}

const std::string& System::GetStatusDetails() const {
    return impl->status_details;
}

Loader::AppLoader& System::GetAppLoader() {
    return *impl->app_loader;
}

const Loader::AppLoader& System::GetAppLoader() const {
    return *impl->app_loader;
}

void System::SetFilesystem(FileSys::VirtualFilesystem vfs) {
    impl->virtual_filesystem = std::move(vfs);
}

FileSys::VirtualFilesystem System::GetFilesystem() const {
    return impl->virtual_filesystem;
}

void System::RegisterCheatList(const std::vector<Memory::CheatEntry>& list,
                               const std::array<u8, 32>& build_id, VAddr main_region_begin,
                               u64 main_region_size) {
    impl->cheat_engine = std::make_unique<Memory::CheatEngine>(*this, list, build_id);
    impl->cheat_engine->SetMainMemoryParameters(main_region_begin, main_region_size);
}

void System::SetAppletFrontendSet(Service::AM::Applets::AppletFrontendSet&& set) {
    impl->applet_manager.SetAppletFrontendSet(std::move(set));
}

void System::SetDefaultAppletFrontendSet() {
    impl->applet_manager.SetDefaultAppletFrontendSet();
}

Service::AM::Applets::AppletManager& System::GetAppletManager() {
    return impl->applet_manager;
}

const Service::AM::Applets::AppletManager& System::GetAppletManager() const {
    return impl->applet_manager;
}

void System::SetContentProvider(std::unique_ptr<FileSys::ContentProviderUnion> provider) {
    impl->content_provider = std::move(provider);
}

FileSys::ContentProvider& System::GetContentProvider() {
    return *impl->content_provider;
}

const FileSys::ContentProvider& System::GetContentProvider() const {
    return *impl->content_provider;
}

Service::FileSystem::FileSystemController& System::GetFileSystemController() {
    return impl->fs_controller;
}

const Service::FileSystem::FileSystemController& System::GetFileSystemController() const {
    return impl->fs_controller;
}

void System::RegisterContentProvider(FileSys::ContentProviderUnionSlot slot,
                                     FileSys::ContentProvider* provider) {
    impl->content_provider->SetSlot(slot, provider);
}

void System::ClearContentProvider(FileSys::ContentProviderUnionSlot slot) {
    impl->content_provider->ClearSlot(slot);
}

const Reporter& System::GetReporter() const {
    return impl->reporter;
}

Service::Glue::ARPManager& System::GetARPManager() {
    return impl->arp_manager;
}

const Service::Glue::ARPManager& System::GetARPManager() const {
    return impl->arp_manager;
}

Service::APM::Controller& System::GetAPMController() {
    return impl->apm_controller;
}

const Service::APM::Controller& System::GetAPMController() const {
    return impl->apm_controller;
}

Service::Time::TimeManager& System::GetTimeManager() {
    return impl->time_manager;
}

const Service::Time::TimeManager& System::GetTimeManager() const {
    return impl->time_manager;
}

void System::SetExitLock(bool locked) {
    impl->exit_lock = locked;
}

bool System::GetExitLock() const {
    return impl->exit_lock;
}

void System::SetApplicationProcessBuildID(const CurrentBuildProcessID& id) {
    impl->build_id = id;
}

const System::CurrentBuildProcessID& System::GetApplicationProcessBuildID() const {
    return impl->build_id;
}

Service::SM::ServiceManager& System::ServiceManager() {
    return *impl->service_manager;
}

const Service::SM::ServiceManager& System::ServiceManager() const {
    return *impl->service_manager;
}

void System::RegisterCoreThread(std::size_t id) {
    impl->kernel.RegisterCoreThread(id);
}

void System::RegisterHostThread() {
    impl->kernel.RegisterHostThread();
}

void System::EnterDynarmicProfile() {
    std::size_t core = impl->kernel.GetCurrentHostThreadID();
    impl->dynarmic_ticks[core] = MicroProfileEnter(impl->microprofile_dynarmic[core]);
}

void System::ExitDynarmicProfile() {
    std::size_t core = impl->kernel.GetCurrentHostThreadID();
    MicroProfileLeave(impl->microprofile_dynarmic[core], impl->dynarmic_ticks[core]);
}

bool System::IsMulticore() const {
    return impl->is_multicore;
}

bool System::DebuggerEnabled() const {
    return Settings::values.use_gdbstub.GetValue();
}

Core::Debugger& System::GetDebugger() {
    return *impl->debugger;
}

const Core::Debugger& System::GetDebugger() const {
    return *impl->debugger;
}

Network::RoomNetwork& System::GetRoomNetwork() {
    return impl->room_network;
}

const Network::RoomNetwork& System::GetRoomNetwork() const {
    return impl->room_network;
}

void System::RunServer(std::unique_ptr<Service::ServerManager>&& server_manager) {
    return impl->kernel.RunServer(std::move(server_manager));
}

void System::RegisterExecuteProgramCallback(ExecuteProgramCallback&& callback) {
    impl->execute_program_callback = std::move(callback);
}

void System::ExecuteProgram(std::size_t program_index) {
    if (impl->execute_program_callback) {
        impl->execute_program_callback(program_index);
    } else {
        LOG_CRITICAL(Core, "execute_program_callback must be initialized by the frontend");
    }
}

void System::RegisterExitCallback(ExitCallback&& callback) {
    impl->exit_callback = std::move(callback);
}

void System::Exit() {
    if (impl->exit_callback) {
        impl->exit_callback();
    } else {
        LOG_CRITICAL(Core, "exit_callback must be initialized by the frontend");
    }
}

void System::ApplySettings() {
    if (IsPoweredOn()) {
        Renderer().RefreshBaseSettings();
    }
}

} // namespace Core
