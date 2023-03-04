// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/glue/arp.h"
#include "core/hle/service/glue/errors.h"
#include "core/hle/service/glue/glue_manager.h"
#include "core/hle/service/ipc_helpers.h"

namespace Service::Glue {

namespace {
std::optional<u64> GetTitleIDForProcessID(const Core::System& system, u64 process_id) {
    const auto& list = system.Kernel().GetProcessList();
    const auto iter = std::find_if(list.begin(), list.end(), [&process_id](const auto& process) {
        return process->GetProcessID() == process_id;
    });

    if (iter == list.end()) {
        return std::nullopt;
    }

    return (*iter)->GetProgramID();
}
} // Anonymous namespace

ARP_R::ARP_R(Core::System& system_, const ARPManager& manager_)
    : ServiceFramework{system_, "arp:r"}, manager{manager_} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, &ARP_R::GetApplicationLaunchProperty, "GetApplicationLaunchProperty"},
            {1, &ARP_R::GetApplicationLaunchPropertyWithApplicationId, "GetApplicationLaunchPropertyWithApplicationId"},
            {2, &ARP_R::GetApplicationControlProperty, "GetApplicationControlProperty"},
            {3, &ARP_R::GetApplicationControlPropertyWithApplicationId, "GetApplicationControlPropertyWithApplicationId"},
            {4, nullptr, "GetApplicationInstanceUnregistrationNotifier"},
            {5, nullptr, "ListApplicationInstanceId"},
            {6, nullptr, "GetMicroApplicationInstanceId"},
            {7, nullptr, "GetApplicationCertificate"},
            {9998, nullptr, "GetPreomiaApplicationLaunchProperty"},
            {9999, nullptr, "GetPreomiaApplicationControlProperty"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

ARP_R::~ARP_R() = default;

void ARP_R::GetApplicationLaunchProperty(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto process_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_ARP, "called, process_id={:016X}", process_id);

    const auto title_id = GetTitleIDForProcessID(system, process_id);
    if (!title_id.has_value()) {
        LOG_ERROR(Service_ARP, "Failed to get title ID for process ID!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERR_NOT_REGISTERED);
        return;
    }

    const auto res = manager.GetLaunchProperty(*title_id);

    if (res.Failed()) {
        LOG_ERROR(Service_ARP, "Failed to get launch property!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res.Code());
        return;
    }

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.PushRaw(*res);
}

void ARP_R::GetApplicationLaunchPropertyWithApplicationId(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto title_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_ARP, "called, title_id={:016X}", title_id);

    const auto res = manager.GetLaunchProperty(title_id);

    if (res.Failed()) {
        LOG_ERROR(Service_ARP, "Failed to get launch property!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res.Code());
        return;
    }

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.PushRaw(*res);
}

void ARP_R::GetApplicationControlProperty(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto process_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_ARP, "called, process_id={:016X}", process_id);

    const auto title_id = GetTitleIDForProcessID(system, process_id);
    if (!title_id.has_value()) {
        LOG_ERROR(Service_ARP, "Failed to get title ID for process ID!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERR_NOT_REGISTERED);
        return;
    }

    const auto res = manager.GetControlProperty(*title_id);

    if (res.Failed()) {
        LOG_ERROR(Service_ARP, "Failed to get control property!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res.Code());
        return;
    }

    ctx.WriteBuffer(*res);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ARP_R::GetApplicationControlPropertyWithApplicationId(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto title_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_ARP, "called, title_id={:016X}", title_id);

    const auto res = manager.GetControlProperty(title_id);

    if (res.Failed()) {
        LOG_ERROR(Service_ARP, "Failed to get control property!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res.Code());
        return;
    }

    ctx.WriteBuffer(*res);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

class IRegistrar final : public ServiceFramework<IRegistrar> {
    friend class ARP_W;

public:
    using IssuerFn = std::function<Result(u64, ApplicationLaunchProperty, std::vector<u8>)>;

    explicit IRegistrar(Core::System& system_, IssuerFn&& issuer)
        : ServiceFramework{system_, "IRegistrar"}, issue_process_id{std::move(issuer)} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IRegistrar::Issue, "Issue"},
            {1, &IRegistrar::SetApplicationLaunchProperty, "SetApplicationLaunchProperty"},
            {2, &IRegistrar::SetApplicationControlProperty, "SetApplicationControlProperty"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void Issue(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto process_id = rp.PopRaw<u64>();

        LOG_DEBUG(Service_ARP, "called, process_id={:016X}", process_id);

        if (process_id == 0) {
            LOG_ERROR(Service_ARP, "Must have non-zero process ID!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_INVALID_PROCESS_ID);
            return;
        }

        if (issued) {
            LOG_ERROR(Service_ARP,
                      "Attempted to issue registrar, but registrar is already issued!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_INVALID_ACCESS);
            return;
        }

        issue_process_id(process_id, launch, std::move(control));
        issued = true;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetApplicationLaunchProperty(HLERequestContext& ctx) {
        LOG_DEBUG(Service_ARP, "called");

        if (issued) {
            LOG_ERROR(
                Service_ARP,
                "Attempted to set application launch property, but registrar is already issued!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_INVALID_ACCESS);
            return;
        }

        IPC::RequestParser rp{ctx};
        launch = rp.PopRaw<ApplicationLaunchProperty>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetApplicationControlProperty(HLERequestContext& ctx) {
        LOG_DEBUG(Service_ARP, "called");

        if (issued) {
            LOG_ERROR(
                Service_ARP,
                "Attempted to set application control property, but registrar is already issued!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_INVALID_ACCESS);
            return;
        }

        // TODO: Can this be a span?
        control = ctx.ReadBufferCopy();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    IssuerFn issue_process_id;
    bool issued = false;
    ApplicationLaunchProperty launch{};
    std::vector<u8> control;
};

ARP_W::ARP_W(Core::System& system_, ARPManager& manager_)
    : ServiceFramework{system_, "arp:w"}, manager{manager_} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, &ARP_W::AcquireRegistrar, "AcquireRegistrar"},
            {1, &ARP_W::UnregisterApplicationInstance , "UnregisterApplicationInstance "},
            {2, nullptr, "AcquireUpdater"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

ARP_W::~ARP_W() = default;

void ARP_W::AcquireRegistrar(HLERequestContext& ctx) {
    LOG_DEBUG(Service_ARP, "called");

    registrar = std::make_shared<IRegistrar>(
        system, [this](u64 process_id, ApplicationLaunchProperty launch, std::vector<u8> control) {
            const auto res = GetTitleIDForProcessID(system, process_id);
            if (!res.has_value()) {
                return ERR_NOT_REGISTERED;
            }

            return manager.Register(*res, launch, std::move(control));
        });

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface(registrar);
}

void ARP_W::UnregisterApplicationInstance(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto process_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_ARP, "called, process_id={:016X}", process_id);

    if (process_id == 0) {
        LOG_ERROR(Service_ARP, "Must have non-zero process ID!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERR_INVALID_PROCESS_ID);
        return;
    }

    const auto title_id = GetTitleIDForProcessID(system, process_id);

    if (!title_id.has_value()) {
        LOG_ERROR(Service_ARP, "No title ID for process ID!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERR_NOT_REGISTERED);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(manager.Unregister(*title_id));
}

} // namespace Service::Glue
