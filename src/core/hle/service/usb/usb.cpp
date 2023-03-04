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
            {0, nullptr, "BindDevice"},
            {1, nullptr, "BindClientProcess"},
            {2, nullptr, "AddInterface"},
            {3, nullptr, "GetStateChangeEvent"},
            {4, nullptr, "GetState"},
            {5, nullptr, "ClearDeviceData"},
            {6, nullptr, "AddUsbStringDescriptor"},
            {7, nullptr, "DeleteUsbStringDescriptor"},
            {8, nullptr, "SetUsbDeviceDescriptor"},
            {9, nullptr, "SetBinaryObjectStore"},
            {10, nullptr, "Enable"},
            {11, nullptr, "Disable"},
            {12, nullptr, "Unknown12"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class USB_DS final : public ServiceFramework<USB_DS> {
public:
    explicit USB_DS(Core::System& system_) : ServiceFramework{system_, "usb:ds"} {
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

class USB_HS final : public ServiceFramework<USB_HS> {
public:
    explicit USB_HS(Core::System& system_) : ServiceFramework{system_, "usb:hs"} {
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
            {8, nullptr, "ResetDevice"},
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

class USB_PD final : public ServiceFramework<USB_PD> {
public:
    explicit USB_PD(Core::System& system_) : ServiceFramework{system_, "usb:pd"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &USB_PD::GetPdSession, "GetPdSession"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetPdSession(HLERequestContext& ctx) {
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

class USB_PD_C final : public ServiceFramework<USB_PD_C> {
public:
    explicit USB_PD_C(Core::System& system_) : ServiceFramework{system_, "usb:pd:c"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &USB_PD_C::GetPdCradleSession, "GetPdCradleSession"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetPdCradleSession(HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IPdCradleSession>(system);

        LOG_DEBUG(Service_USB, "called");
    }
};

class USB_PM final : public ServiceFramework<USB_PM> {
public:
    explicit USB_PM(Core::System& system_) : ServiceFramework{system_, "usb:pm"} {
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

    server_manager->RegisterNamedService("usb:ds", std::make_shared<USB_DS>(system));
    server_manager->RegisterNamedService("usb:hs", std::make_shared<USB_HS>(system));
    server_manager->RegisterNamedService("usb:pd", std::make_shared<USB_PD>(system));
    server_manager->RegisterNamedService("usb:pd:c", std::make_shared<USB_PD_C>(system));
    server_manager->RegisterNamedService("usb:pm", std::make_shared<USB_PM>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::USB
