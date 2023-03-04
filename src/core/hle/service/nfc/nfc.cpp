// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/nfc/mifare_user.h"
#include "core/hle/service/nfc/nfc.h"
#include "core/hle/service/nfc/nfc_user.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::NFC {

class IAm final : public ServiceFramework<IAm> {
public:
    explicit IAm(Core::System& system_) : ServiceFramework{system_, "NFC::IAm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "Finalize"},
            {2, nullptr, "NotifyForegroundApplet"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class NFC_AM final : public ServiceFramework<NFC_AM> {
public:
    explicit NFC_AM(Core::System& system_) : ServiceFramework{system_, "nfc:am"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &NFC_AM::CreateAmInterface, "CreateAmInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateAmInterface(HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IAm>(system);
    }
};

class NFC_MF_U final : public ServiceFramework<NFC_MF_U> {
public:
    explicit NFC_MF_U(Core::System& system_) : ServiceFramework{system_, "nfc:mf:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &NFC_MF_U::CreateUserInterface, "CreateUserInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateUserInterface(HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<MFIUser>(system);
    }
};

class NFC_U final : public ServiceFramework<NFC_U> {
public:
    explicit NFC_U(Core::System& system_) : ServiceFramework{system_, "nfc:user"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &NFC_U::CreateUserInterface, "CreateUserInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateUserInterface(HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IUser>(system);
    }
};

class ISystem final : public ServiceFramework<ISystem> {
public:
    explicit ISystem(Core::System& system_) : ServiceFramework{system_, "ISystem"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "Finalize"},
            {2, nullptr, "GetStateOld"},
            {3, nullptr, "IsNfcEnabledOld"},
            {100, nullptr, "SetNfcEnabledOld"},
            {400, nullptr, "InitializeSystem"},
            {401, nullptr, "FinalizeSystem"},
            {402, nullptr, "GetState"},
            {403, nullptr, "IsNfcEnabled"},
            {404, nullptr, "ListDevices"},
            {405, nullptr, "GetDeviceState"},
            {406, nullptr, "GetNpadId"},
            {407, nullptr, "AttachAvailabilityChangeEvent"},
            {408, nullptr, "StartDetection"},
            {409, nullptr, "StopDetection"},
            {410, nullptr, "GetTagInfo"},
            {411, nullptr, "AttachActivateEvent"},
            {412, nullptr, "AttachDeactivateEvent"},
            {500, nullptr, "SetNfcEnabled"},
            {510, nullptr, "OutputTestWave"},
            {1000, nullptr, "ReadMifare"},
            {1001, nullptr, "WriteMifare"},
            {1300, nullptr, "SendCommandByPassThrough"},
            {1301, nullptr, "KeepPassThroughSession"},
            {1302, nullptr, "ReleasePassThroughSession"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class NFC_SYS final : public ServiceFramework<NFC_SYS> {
public:
    explicit NFC_SYS(Core::System& system_) : ServiceFramework{system_, "nfc:sys"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &NFC_SYS::CreateSystemInterface, "CreateSystemInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateSystemInterface(HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISystem>(system);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("nfc:am", std::make_shared<NFC_AM>(system));
    server_manager->RegisterNamedService("nfc:mf:u", std::make_shared<NFC_MF_U>(system));
    server_manager->RegisterNamedService("nfc:user", std::make_shared<NFC_U>(system));
    server_manager->RegisterNamedService("nfc:sys", std::make_shared<NFC_SYS>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::NFC
