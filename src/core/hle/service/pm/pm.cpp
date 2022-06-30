// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/pm/pm.h"
#include "core/hle/service/service.h"

namespace Service::PM {

namespace {

constexpr Result ResultProcessNotFound{ErrorModule::PM, 1};
[[maybe_unused]] constexpr Result ResultAlreadyStarted{ErrorModule::PM, 2};
[[maybe_unused]] constexpr Result ResultNotTerminated{ErrorModule::PM, 3};
[[maybe_unused]] constexpr Result ResultDebugHookInUse{ErrorModule::PM, 4};
[[maybe_unused]] constexpr Result ResultApplicationRunning{ErrorModule::PM, 5};
[[maybe_unused]] constexpr Result ResultInvalidSize{ErrorModule::PM, 6};

constexpr u64 NO_PROCESS_FOUND_PID{0};

std::optional<Kernel::KProcess*> SearchProcessList(
    const std::vector<Kernel::KProcess*>& process_list,
    std::function<bool(Kernel::KProcess*)> predicate) {
    const auto iter = std::find_if(process_list.begin(), process_list.end(), predicate);

    if (iter == process_list.end()) {
        return std::nullopt;
    }

    return *iter;
}

void GetApplicationPidGeneric(Kernel::HLERequestContext& ctx,
                              const std::vector<Kernel::KProcess*>& process_list) {
    const auto process = SearchProcessList(process_list, [](const auto& proc) {
        return proc->GetProcessID() == Kernel::KProcess::ProcessIDMin;
    });

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(process.has_value() ? (*process)->GetProcessID() : NO_PROCESS_FOUND_PID);
}

} // Anonymous namespace

class BootMode final : public ServiceFramework<BootMode> {
public:
    explicit BootMode(Core::System& system_) : ServiceFramework{system_, "pm:bm"} {
        static const FunctionInfo functions[] = {
            {0, &BootMode::GetBootMode, "GetBootMode"},
            {1, &BootMode::SetMaintenanceBoot, "SetMaintenanceBoot"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetBootMode(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PM, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(boot_mode);
    }

    void SetMaintenanceBoot(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PM, "called");

        boot_mode = SystemBootMode::Maintenance;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    SystemBootMode boot_mode = SystemBootMode::Normal;
};

class DebugMonitor final : public ServiceFramework<DebugMonitor> {
public:
    explicit DebugMonitor(Core::System& system_)
        : ServiceFramework{system_, "pm:dmnt"}, kernel{system_.Kernel()} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetJitDebugProcessIdList"},
            {1, nullptr, "StartProcess"},
            {2, &DebugMonitor::GetProcessId, "GetProcessId"},
            {3, nullptr, "HookToCreateProcess"},
            {4, &DebugMonitor::GetApplicationProcessId, "GetApplicationProcessId"},
            {5, nullptr, "HookToCreateApplicationProgress"},
            {6, nullptr, "ClearHook"},
            {65000, &DebugMonitor::AtmosphereGetProcessInfo, "AtmosphereGetProcessInfo"},
            {65001, nullptr, "AtmosphereGetCurrentLimitInfo"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetProcessId(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto program_id = rp.PopRaw<u64>();

        LOG_DEBUG(Service_PM, "called, program_id={:016X}", program_id);

        const auto process =
            SearchProcessList(kernel.GetProcessList(), [program_id](const auto& proc) {
                return proc->GetProgramID() == program_id;
            });

        if (!process.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultProcessNotFound);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push((*process)->GetProcessID());
    }

    void GetApplicationProcessId(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PM, "called");
        GetApplicationPidGeneric(ctx, kernel.GetProcessList());
    }

    void AtmosphereGetProcessInfo(Kernel::HLERequestContext& ctx) {
        // https://github.com/Atmosphere-NX/Atmosphere/blob/master/stratosphere/pm/source/impl/pm_process_manager.cpp#L614
        // This implementation is incomplete; only a handle to the process is returned.
        IPC::RequestParser rp{ctx};
        const auto pid = rp.PopRaw<u64>();

        LOG_WARNING(Service_PM, "(Partial Implementation) called, pid={:016X}", pid);

        const auto process = SearchProcessList(kernel.GetProcessList(), [pid](const auto& proc) {
            return proc->GetProcessID() == pid;
        });

        if (!process.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultProcessNotFound);
            return;
        }

        struct ProgramLocation {
            u64 program_id;
            u8 storage_id;
        };
        static_assert(sizeof(ProgramLocation) == 0x10, "ProgramLocation has an invalid size");

        struct OverrideStatus {
            u64 keys_held;
            u64 flags;
        };
        static_assert(sizeof(OverrideStatus) == 0x10, "OverrideStatus has an invalid size");

        OverrideStatus override_status{};
        ProgramLocation program_location{
            .program_id = (*process)->GetProgramID(),
            .storage_id = 0,
        };

        IPC::ResponseBuilder rb{ctx, 10, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(*process);
        rb.PushRaw(program_location);
        rb.PushRaw(override_status);
    }

    const Kernel::KernelCore& kernel;
};

class Info final : public ServiceFramework<Info> {
public:
    explicit Info(Core::System& system_, const std::vector<Kernel::KProcess*>& process_list_)
        : ServiceFramework{system_, "pm:info"}, process_list{process_list_} {
        static const FunctionInfo functions[] = {
            {0, &Info::GetProgramId, "GetProgramId"},
            {65000, &Info::AtmosphereGetProcessId, "AtmosphereGetProcessId"},
            {65001, nullptr, "AtmosphereHasLaunchedProgram"},
            {65002, nullptr, "AtmosphereGetProcessInfo"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetProgramId(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto process_id = rp.PopRaw<u64>();

        LOG_DEBUG(Service_PM, "called, process_id={:016X}", process_id);

        const auto process = SearchProcessList(process_list, [process_id](const auto& proc) {
            return proc->GetProcessID() == process_id;
        });

        if (!process.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultProcessNotFound);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push((*process)->GetProgramID());
    }

    void AtmosphereGetProcessId(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto program_id = rp.PopRaw<u64>();

        LOG_DEBUG(Service_PM, "called, program_id={:016X}", program_id);

        const auto process = SearchProcessList(process_list, [program_id](const auto& proc) {
            return proc->GetProgramID() == program_id;
        });

        if (!process.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultProcessNotFound);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push((*process)->GetProcessID());
    }

    const std::vector<Kernel::KProcess*>& process_list;
};

class Shell final : public ServiceFramework<Shell> {
public:
    explicit Shell(Core::System& system_)
        : ServiceFramework{system_, "pm:shell"}, kernel{system_.Kernel()} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "LaunchProgram"},
            {1, nullptr, "TerminateProcess"},
            {2, nullptr, "TerminateProgram"},
            {3, nullptr, "GetProcessEventHandle"},
            {4, nullptr, "GetProcessEventInfo"},
            {5, nullptr, "NotifyBootFinished"},
            {6, &Shell::GetApplicationProcessIdForShell, "GetApplicationProcessIdForShell"},
            {7, nullptr, "BoostSystemMemoryResourceLimit"},
            {8, nullptr, "BoostApplicationThreadResourceLimit"},
            {9, nullptr, "GetBootFinishedEventHandle"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetApplicationProcessIdForShell(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PM, "called");
        GetApplicationPidGeneric(ctx, kernel.GetProcessList());
    }

    const Kernel::KernelCore& kernel;
};

void InstallInterfaces(Core::System& system) {
    std::make_shared<BootMode>(system)->InstallAsService(system.ServiceManager());
    std::make_shared<DebugMonitor>(system)->InstallAsService(system.ServiceManager());
    std::make_shared<Info>(system, system.Kernel().GetProcessList())
        ->InstallAsService(system.ServiceManager());
    std::make_shared<Shell>(system)->InstallAsService(system.ServiceManager());
}

} // namespace Service::PM
