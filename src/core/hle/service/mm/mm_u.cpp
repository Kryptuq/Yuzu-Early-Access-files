// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/mm/mm_u.h"
#include "core/hle/service/sm/sm.h"

namespace Service::MM {

class MM_U final : public ServiceFramework<MM_U> {
public:
    explicit MM_U(Core::System& system_) : ServiceFramework{system_, "mm:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &MM_U::InitializeOld, "InitializeOld"},
            {1, &MM_U::FinalizeOld, "FinalizeOld"},
            {2, &MM_U::SetAndWaitOld, "SetAndWaitOld"},
            {3, &MM_U::GetOld, "GetOld"},
            {4, &MM_U::Initialize, "Initialize"},
            {5, &MM_U::Finalize, "Finalize"},
            {6, &MM_U::SetAndWait, "SetAndWait"},
            {7, &MM_U::Get, "Get"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void InitializeOld(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_MM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void FinalizeOld(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_MM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetAndWaitOld(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        min = rp.Pop<u32>();
        max = rp.Pop<u32>();
        LOG_DEBUG(Service_MM, "(STUBBED) called, min=0x{:X}, max=0x{:X}", min, max);

        current = min;
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetOld(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_MM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(current);
    }

    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_MM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(id); // Any non zero value
    }

    void Finalize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_MM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetAndWait(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u32 input_id = rp.Pop<u32>();
        min = rp.Pop<u32>();
        max = rp.Pop<u32>();
        LOG_DEBUG(Service_MM, "(STUBBED) called, input_id=0x{:X}, min=0x{:X}, max=0x{:X}", input_id,
                  min, max);

        current = min;
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Get(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_MM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(current);
    }

    u32 min{0};
    u32 max{0};
    u32 current{0};
    u32 id{1};
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<MM_U>(system)->InstallAsService(service_manager);
}

} // namespace Service::MM
