// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"
#include "core/hle/service/usb/usb.h"

namespace Service::USB {

class IDsInterface final : public ServiceFramework<IDsInterface> {
public:
    explicit IDsInterface(Core::System& system_) : ServiceFramework{system_, "IDsInterface"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "AddEndpoint"},
            {1, nullptr, "GetSetupEvent"},
            {2, nullptr, "GetSetupPacket"},
            {3, nullptr, "Enable"},
            {4, nullptr, "Disable"},
            {5, nullptr, "CtrlIn"},
            {6, nullptr, "CtrlOut"},
            {7, nullptr, "GetCtrlInCompletionEvent"},
            {8, nullptr, "GetCtrlInUrbReport"},
            {9, nullptr, "GetCtrlOutCompletionEvent"},
            {10, nullptr, "GetCtrlOutUrbReport"},
            {11, nullptr, "CtrlStall"},
            {12, nullptr, "AppendConfigurationData"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IDsRootSession final : public ServiceFramework<IDsRootSession> {
public:
    explicit IDsRootSession(Core::System& system_) : ServiceFramework{system_, "usb:ds"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "OpenDsService"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IClientEpSession final : public ServiceFramework<IClientEpSession> {
public:
    explicit IClientEpSession(Core::System& system_)
        : ServiceFramework{system_, "IClientEpSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ReOpen"},
            {1, nullptr, "Close"},
            {2, nullptr, "GetCompletionEvent"},
            {3, nullptr, "PopulateRing"},
            {4, nullptr, "PostBufferAsync"},
            {5, nullptr, "GetXferReport"},
            {6, nullptr, "PostBufferMultiAsync"},
            {7, nullptr, "CreateSmmuSpace"},
            {8, nullptr, "ShareReportRing"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IClientIfSession final : public ServiceFramework<IClientIfSession> {
public:
    explicit IClientIfSession(Core::System& system_)
        : ServiceFramework{system_, "IClientIfSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetStateChangeEvent"},
            {1, nullptr, "SetInterface"},
            {2, nullptr, "GetInterface"},
            {3, nullptr, "GetAlternateInterface"},
            {4, nullptr, "GetCurrentFrame"},
            {5, nullptr, "CtrlXferAsync"},
            {6, nullptr, "GetCtrlXferCompletionEvent"},
            {7, nullptr, "GetCtrlXferReport"},
            {8, nullptr, "ResetDevice"},
            {9, nullptr, "OpenUsbEp"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IClientRootSession final : public ServiceFramework<IClientRootSession> {
public:
    explicit IClientRootSession(Core::System& system_) : ServiceFramework{system_, "usb:hs"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "BindClientProcess"},
            {1, nullptr, "QueryAllInterfaces"},
            {2, nullptr, "QueryAvailableInterfaces"},
            {3, nullptr, "QueryAcquiredInterfaces"},
            {4, nullptr, "CreateInterfaceAvailableEvent"},
            {5, nullptr, "DestroyInterfaceAvailableEvent"},
            {6, nullptr, "GetInterfaceStateChangeEvent"},
            {7, nullptr, "AcquireUsbIf"},
            {8, nullptr, "SetTestMode"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IPdSession final : public ServiceFramework<IPdSession> {
public:
    explicit IPdSession(Core::System& system_) : ServiceFramework{system_, "IPdSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "BindNoticeEvent"},
            {1, nullptr, "UnbindNoticeEvent"},
            {2, nullptr, "GetStatus"},
            {3, nullptr, "GetNotice"},
            {4, nullptr, "EnablePowerRequestNotice"},
            {5, nullptr, "DisablePowerRequestNotice"},
            {6, nullptr, "ReplyPowerRequest"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IPdManager final : public ServiceFramework<IPdManager> {
public:
    explicit IPdManager(Core::System& system_) : ServiceFramework{system_, "usb:pd"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IPdManager::OpenSession, "OpenSession"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void OpenSession(HLERequestContext& ctx) {
        LOG_DEBUG(Service_USB, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IPdSession>(system);
    }
};

class IPdCradleSession final : public ServiceFramework<IPdCradleSession> {
public:
    explicit IPdCradleSession(Core::System& system_)
        : ServiceFramework{system_, "IPdCradleSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "SetCradleVdo"},
            {1, nullptr, "GetCradleVdo"},
            {2, nullptr, "ResetCradleUsbHub"},
            {3, nullptr, "GetHostPdcFirmwareType"},
            {4, nullptr, "GetHostPdcFirmwareRevision"},
            {5, nullptr, "GetHostPdcManufactureId"},
            {6, nullptr, "GetHostPdcDeviceId"},
            {7, nullptr, "EnableCradleRecovery"},
            {8, nullptr, "DisableCradleRecovery"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IPdCradleManager final : public ServiceFramework<IPdCradleManager> {
public:
    explicit IPdCradleManager(Core::System& system_) : ServiceFramework{system_, "usb:pd:c"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IPdCradleManager::OpenCradleSession, "OpenCradleSession"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void OpenCradleSession(HLERequestContext& ctx) {
        LOG_DEBUG(Service_USB, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IPdCradleSession>(system);
    }
};

class IPmMainService final : public ServiceFramework<IPmMainService> {
public:
    explicit IPmMainService(Core::System& system_) : ServiceFramework{system_, "usb:pm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetPowerEvent"},
            {1, nullptr, "GetPowerState"},
            {2, nullptr, "GetDataEvent"},
            {3, nullptr, "GetDataRole"},
            {4, nullptr, "SetDiagData"},
            {5, nullptr, "GetDiagData"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("usb:ds", std::make_shared<IDsRootSession>(system));
    server_manager->RegisterNamedService("usb:hs", std::make_shared<IClientRootSession>(system));
    server_manager->RegisterNamedService("usb:pd", std::make_shared<IPdManager>(system));
    server_manager->RegisterNamedService("usb:pd:c", std::make_shared<IPdCradleManager>(system));
    server_manager->RegisterNamedService("usb:pm", std::make_shared<IPmMainService>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::USB
