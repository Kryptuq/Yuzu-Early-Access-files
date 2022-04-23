// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nfc/nfc.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

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
    void CreateAmInterface(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IAm>(system);
    }
};

class MFIUser final : public ServiceFramework<MFIUser> {
public:
    explicit MFIUser(Core::System& system_) : ServiceFramework{system_, "NFC::MFIUser"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "Finalize"},
            {2, nullptr, "ListDevices"},
            {3, nullptr, "StartDetection"},
            {4, nullptr, "StopDetection"},
            {5, nullptr, "Read"},
            {6, nullptr, "Write"},
            {7, nullptr, "GetTagInfo"},
            {8, nullptr, "GetActivateEventHandle"},
            {9, nullptr, "GetDeactivateEventHandle"},
            {10, nullptr, "GetState"},
            {11, nullptr, "GetDeviceState"},
            {12, nullptr, "GetNpadId"},
            {13, nullptr, "GetAvailabilityChangeEventHandle"},
        };
        // clang-format on

        RegisterHandlers(functions);
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
    void CreateUserInterface(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<MFIUser>(system);
    }
};

class IUser final : public ServiceFramework<IUser> {
public:
    explicit IUser(Core::System& system_) : ServiceFramework{system_, "NFC::IUser"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IUser::InitializeOld, "InitializeOld"},
            {1, &IUser::FinalizeOld, "FinalizeOld"},
            {2, &IUser::GetStateOld, "GetStateOld"},
            {3, &IUser::IsNfcEnabledOld, "IsNfcEnabledOld"},
            {400, nullptr, "Initialize"},
            {401, nullptr, "Finalize"},
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
            {1000, nullptr, "ReadMifare"},
            {1001, nullptr, "WriteMifare"},
            {1300, nullptr, "SendCommandByPassThrough"},
            {1301, nullptr, "KeepPassThroughSession"},
            {1302, nullptr, "ReleasePassThroughSession"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    enum class NfcStates : u32 {
        Finalized = 6,
    };

    void InitializeOld(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0};
        rb.Push(ResultSuccess);
        // We don't deal with hardware initialization so we can just stub this.
    }

    void IsNfcEnabledOld(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "IsNfcEnabledOld");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushRaw<u8>(true);
    }

    void GetStateOld(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFC, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(NfcStates::Finalized); // TODO(ogniK): Figure out if this matches nfp
    }

    void FinalizeOld(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFC, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
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
    void CreateUserInterface(Kernel::HLERequestContext& ctx) {
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
    void CreateSystemInterface(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISystem>(system);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<NFC_AM>(system)->InstallAsService(sm);
    std::make_shared<NFC_MF_U>(system)->InstallAsService(sm);
    std::make_shared<NFC_U>(system)->InstallAsService(sm);
    std::make_shared<NFC_SYS>(system)->InstallAsService(sm);
}

} // namespace Service::NFC
