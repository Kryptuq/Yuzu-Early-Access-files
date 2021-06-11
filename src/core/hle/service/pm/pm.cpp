// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/pm/pm.h"
#include "core/hle/service/service.h"

namespace Service::PM {

namespace {

constexpr ResultCode ERROR_PROCESS_NOT_FOUND{ErrorModule::PM, 1};

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
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetProcessId(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto title_id = rp.PopRaw<u64>();

        LOG_DEBUG(Service_PM, "called, title_id={:016X}", title_id);

        const auto process =
            SearchProcessList(kernel.GetProcessList(), [title_id](const auto& proc) {
                return proc->GetTitleID() == title_id;
            });

        if (!process.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_PROCESS_NOT_FOUND);
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

    const Kernel::KernelCore& kernel;
};

class Info final : public ServiceFramework<Info> {
public:
    explicit Info(Core::System& system_, const std::vector<Kernel::KProcess*>& process_list_)
        : ServiceFramework{system_, "pm:info"}, process_list{process_list_} {
        static const FunctionInfo functions[] = {
            {0, &Info::GetTitleId, "GetTitleId"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetTitleId(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto process_id = rp.PopRaw<u64>();

        LOG_DEBUG(Service_PM, "called, process_id={:016X}", process_id);

        const auto process = SearchProcessList(process_list, [process_id](const auto& proc) {
            return proc->GetProcessID() == process_id;
        });

        if (!process.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_PROCESS_NOT_FOUND);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push((*process)->GetTitleID());
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
