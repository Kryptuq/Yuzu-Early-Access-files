// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/ipc_helpers.h"
#include "core/hle/result.h"
#include "core/hle/service/ldn/errors.h"
#include "core/hle/service/ldn/ldn.h"
#include "core/hle/service/sm/sm.h"

namespace Service::LDN {

class IMonitorService final : public ServiceFramework<IMonitorService> {
public:
    explicit IMonitorService(Core::System& system_) : ServiceFramework{system_, "IMonitorService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetStateForMonitor"},
            {1, nullptr, "GetNetworkInfoForMonitor"},
            {2, nullptr, "GetIpv4AddressForMonitor"},
            {3, nullptr, "GetDisconnectReasonForMonitor"},
            {4, nullptr, "GetSecurityParameterForMonitor"},
            {5, nullptr, "GetNetworkConfigForMonitor"},
            {100, nullptr, "InitializeMonitor"},
            {101, nullptr, "FinalizeMonitor"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class LDNM final : public ServiceFramework<LDNM> {
public:
    explicit LDNM(Core::System& system_) : ServiceFramework{system_, "ldn:m"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &LDNM::CreateMonitorService, "CreateMonitorService"}
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateMonitorService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_LDN, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IMonitorService>(system);
    }
};

class ISystemLocalCommunicationService final
    : public ServiceFramework<ISystemLocalCommunicationService> {
public:
    explicit ISystemLocalCommunicationService(Core::System& system_)
        : ServiceFramework{system_, "ISystemLocalCommunicationService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetState"},
            {1, nullptr, "GetNetworkInfo"},
            {2, nullptr, "GetIpv4Address"},
            {3, nullptr, "GetDisconnectReason"},
            {4, nullptr, "GetSecurityParameter"},
            {5, nullptr, "GetNetworkConfig"},
            {100, nullptr, "AttachStateChangeEvent"},
            {101, nullptr, "GetNetworkInfoLatestUpdate"},
            {102, nullptr, "Scan"},
            {103, nullptr, "ScanPrivate"},
            {104, nullptr, "SetWirelessControllerRestriction"},
            {200, nullptr, "OpenAccessPoint"},
            {201, nullptr, "CloseAccessPoint"},
            {202, nullptr, "CreateNetwork"},
            {203, nullptr, "CreateNetworkPrivate"},
            {204, nullptr, "DestroyNetwork"},
            {205, nullptr, "Reject"},
            {206, nullptr, "SetAdvertiseData"},
            {207, nullptr, "SetStationAcceptPolicy"},
            {208, nullptr, "AddAcceptFilterEntry"},
            {209, nullptr, "ClearAcceptFilter"},
            {300, nullptr, "OpenStation"},
            {301, nullptr, "CloseStation"},
            {302, nullptr, "Connect"},
            {303, nullptr, "ConnectPrivate"},
            {304, nullptr, "Disconnect"},
            {400, nullptr, "InitializeSystem"},
            {401, nullptr, "FinalizeSystem"},
            {402, nullptr, "SetOperationMode"},
            {403, nullptr, "InitializeSystem2"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IUserLocalCommunicationService final
    : public ServiceFramework<IUserLocalCommunicationService> {
public:
    explicit IUserLocalCommunicationService(Core::System& system_)
        : ServiceFramework{system_, "IUserLocalCommunicationService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IUserLocalCommunicationService::GetState, "GetState"},
            {1, nullptr, "GetNetworkInfo"},
            {2, nullptr, "GetIpv4Address"},
            {3, nullptr, "GetDisconnectReason"},
            {4, nullptr, "GetSecurityParameter"},
            {5, nullptr, "GetNetworkConfig"},
            {100, nullptr, "AttachStateChangeEvent"},
            {101, nullptr, "GetNetworkInfoLatestUpdate"},
            {102, nullptr, "Scan"},
            {103, nullptr, "ScanPrivate"},
            {104, nullptr, "SetWirelessControllerRestriction"},
            {200, nullptr, "OpenAccessPoint"},
            {201, nullptr, "CloseAccessPoint"},
            {202, nullptr, "CreateNetwork"},
            {203, nullptr, "CreateNetworkPrivate"},
            {204, nullptr, "DestroyNetwork"},
            {205, nullptr, "Reject"},
            {206, nullptr, "SetAdvertiseData"},
            {207, nullptr, "SetStationAcceptPolicy"},
            {208, nullptr, "AddAcceptFilterEntry"},
            {209, nullptr, "ClearAcceptFilter"},
            {300, nullptr, "OpenStation"},
            {301, nullptr, "CloseStation"},
            {302, nullptr, "Connect"},
            {303, nullptr, "ConnectPrivate"},
            {304, nullptr, "Disconnect"},
            {400, nullptr, "Initialize"},
            {401, nullptr, "Finalize"},
            {402, &IUserLocalCommunicationService::Initialize2, "Initialize2"}, // 7.0.0+
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void GetState(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};

        // Indicate a network error, as we do not actually emulate LDN
        rb.Push(static_cast<u32>(State::Error));

        rb.Push(ResultSuccess);
    }

    void Initialize2(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_LDN, "called");

        is_initialized = true;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERROR_DISABLED);
    }

private:
    enum class State {
        None,
        Initialized,
        AccessPointOpened,
        AccessPointCreated,
        StationOpened,
        StationConnected,
        Error,
    };

    bool is_initialized{};
};

class LDNS final : public ServiceFramework<LDNS> {
public:
    explicit LDNS(Core::System& system_) : ServiceFramework{system_, "ldn:s"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &LDNS::CreateSystemLocalCommunicationService, "CreateSystemLocalCommunicationService"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateSystemLocalCommunicationService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_LDN, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISystemLocalCommunicationService>(system);
    }
};

class LDNU final : public ServiceFramework<LDNU> {
public:
    explicit LDNU(Core::System& system_) : ServiceFramework{system_, "ldn:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &LDNU::CreateUserLocalCommunicationService, "CreateUserLocalCommunicationService"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateUserLocalCommunicationService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_LDN, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IUserLocalCommunicationService>(system);
    }
};

class INetworkService final : public ServiceFramework<INetworkService> {
public:
    explicit INetworkService(Core::System& system_) : ServiceFramework{system_, "INetworkService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {256, nullptr, "AttachNetworkInterfaceStateChangeEvent"},
            {264, nullptr, "GetNetworkInterfaceLastError"},
            {272, nullptr, "GetRole"},
            {280, nullptr, "GetAdvertiseData"},
            {288, nullptr, "GetGroupInfo"},
            {296, nullptr, "GetGroupInfo2"},
            {304, nullptr, "GetGroupOwner"},
            {312, nullptr, "GetIpConfig"},
            {320, nullptr, "GetLinkLevel"},
            {512, nullptr, "Scan"},
            {768, nullptr, "CreateGroup"},
            {776, nullptr, "DestroyGroup"},
            {784, nullptr, "SetAdvertiseData"},
            {1536, nullptr, "SendToOtherGroup"},
            {1544, nullptr, "RecvFromOtherGroup"},
            {1552, nullptr, "AddAcceptableGroupId"},
            {1560, nullptr, "ClearAcceptableGroupId"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class INetworkServiceMonitor final : public ServiceFramework<INetworkServiceMonitor> {
public:
    explicit INetworkServiceMonitor(Core::System& system_)
        : ServiceFramework{system_, "INetworkServiceMonitor"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &INetworkServiceMonitor::Initialize, "Initialize"},
            {256, nullptr, "AttachNetworkInterfaceStateChangeEvent"},
            {264, nullptr, "GetNetworkInterfaceLastError"},
            {272, nullptr, "GetRole"},
            {280, nullptr, "GetAdvertiseData"},
            {281, nullptr, "GetAdvertiseData2"},
            {288, nullptr, "GetGroupInfo"},
            {296, nullptr, "GetGroupInfo2"},
            {304, nullptr, "GetGroupOwner"},
            {312, nullptr, "GetIpConfig"},
            {320, nullptr, "GetLinkLevel"},
            {328, nullptr, "AttachJoinEvent"},
            {336, nullptr, "GetMembers"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERROR_DISABLED);
    }
};

class LP2PAPP final : public ServiceFramework<LP2PAPP> {
public:
    explicit LP2PAPP(Core::System& system_) : ServiceFramework{system_, "lp2p:app"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &LP2PAPP::CreateMonitorService, "CreateNetworkService"},
            {8, &LP2PAPP::CreateMonitorService, "CreateNetworkServiceMonitor"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateNetworkervice(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 reserved_input = rp.Pop<u64>();
        const u32 input = rp.Pop<u32>();

        LOG_WARNING(Service_LDN, "(STUBBED) called reserved_input={} input={}", reserved_input,
                    input);

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<INetworkService>(system);
    }

    void CreateMonitorService(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 reserved_input = rp.Pop<u64>();

        LOG_WARNING(Service_LDN, "(STUBBED) called reserved_input={}", reserved_input);

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<INetworkServiceMonitor>(system);
    }
};

class LP2PSYS final : public ServiceFramework<LP2PSYS> {
public:
    explicit LP2PSYS(Core::System& system_) : ServiceFramework{system_, "lp2p:sys"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &LP2PSYS::CreateMonitorService, "CreateNetworkService"},
            {8, &LP2PSYS::CreateMonitorService, "CreateNetworkServiceMonitor"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateNetworkervice(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 reserved_input = rp.Pop<u64>();
        const u32 input = rp.Pop<u32>();

        LOG_WARNING(Service_LDN, "(STUBBED) called reserved_input={} input={}", reserved_input,
                    input);

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<INetworkService>(system);
    }

    void CreateMonitorService(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 reserved_input = rp.Pop<u64>();

        LOG_WARNING(Service_LDN, "(STUBBED) called reserved_input={}", reserved_input);

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<INetworkServiceMonitor>(system);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<LDNM>(system)->InstallAsService(sm);
    std::make_shared<LDNS>(system)->InstallAsService(sm);
    std::make_shared<LDNU>(system)->InstallAsService(sm);
    std::make_shared<LP2PAPP>(system)->InstallAsService(sm);
    std::make_shared<LP2PSYS>(system)->InstallAsService(sm);
}

} // namespace Service::LDN
