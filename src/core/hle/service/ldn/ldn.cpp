// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/core.h"
#include "core/hle/service/ldn/ldn.h"
#include "core/hle/service/ldn/ldn_results.h"
#include "core/hle/service/ldn/ldn_types.h"
#include "core/internal_network/network.h"
#include "core/internal_network/network_interface.h"

// This is defined by synchapi.h and conflicts with ServiceContext::CreateEvent
#undef CreateEvent

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
        : ServiceFramework{system_, "IUserLocalCommunicationService", ServiceThreadType::CreateNew},
          service_context{system, "IUserLocalCommunicationService"}, room_network{
                                                                         system_.GetRoomNetwork()} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IUserLocalCommunicationService::GetState, "GetState"},
            {1, &IUserLocalCommunicationService::GetNetworkInfo, "GetNetworkInfo"},
            {2, nullptr, "GetIpv4Address"},
            {3, &IUserLocalCommunicationService::GetDisconnectReason, "GetDisconnectReason"},
            {4, &IUserLocalCommunicationService::GetSecurityParameter, "GetSecurityParameter"},
            {5, &IUserLocalCommunicationService::GetNetworkConfig, "GetNetworkConfig"},
            {100, &IUserLocalCommunicationService::AttachStateChangeEvent, "AttachStateChangeEvent"},
            {101, &IUserLocalCommunicationService::GetNetworkInfoLatestUpdate, "GetNetworkInfoLatestUpdate"},
            {102, &IUserLocalCommunicationService::Scan, "Scan"},
            {103, &IUserLocalCommunicationService::ScanPrivate, "ScanPrivate"},
            {104, nullptr, "SetWirelessControllerRestriction"},
            {200, &IUserLocalCommunicationService::OpenAccessPoint, "OpenAccessPoint"},
            {201, &IUserLocalCommunicationService::CloseAccessPoint, "CloseAccessPoint"},
            {202, &IUserLocalCommunicationService::CreateNetwork, "CreateNetwork"},
            {203, &IUserLocalCommunicationService::CreateNetworkPrivate, "CreateNetworkPrivate"},
            {204, &IUserLocalCommunicationService::DestroyNetwork, "DestroyNetwork"},
            {205, nullptr, "Reject"},
            {206, &IUserLocalCommunicationService::SetAdvertiseData, "SetAdvertiseData"},
            {207, &IUserLocalCommunicationService::SetStationAcceptPolicy, "SetStationAcceptPolicy"},
            {208, &IUserLocalCommunicationService::AddAcceptFilterEntry, "AddAcceptFilterEntry"},
            {209, nullptr, "ClearAcceptFilter"},
            {300, &IUserLocalCommunicationService::OpenStation, "OpenStation"},
            {301, &IUserLocalCommunicationService::CloseStation, "CloseStation"},
            {302, &IUserLocalCommunicationService::Connect, "Connect"},
            {303, nullptr, "ConnectPrivate"},
            {304, &IUserLocalCommunicationService::Disconnect, "Disconnect"},
            {400, &IUserLocalCommunicationService::Initialize, "Initialize"},
            {401, &IUserLocalCommunicationService::Finalize, "Finalize"},
            {402, &IUserLocalCommunicationService::Initialize2, "Initialize2"},
        };
        // clang-format on

        RegisterHandlers(functions);

        state_change_event =
            service_context.CreateEvent("IUserLocalCommunicationService:StateChangeEvent");
    }

    ~IUserLocalCommunicationService() {
        service_context.CloseEvent(state_change_event);
    }

    void OnEventFired() {
        state_change_event->GetWritableEvent().Signal();
    }

    void GetState(Kernel::HLERequestContext& ctx) {
        State state = State::Error;
        LOG_WARNING(Service_LDN, "(STUBBED) called, state = {}", state);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(state);
    }

    void GetNetworkInfo(Kernel::HLERequestContext& ctx) {
        const auto write_buffer_size = ctx.GetWriteBufferSize();

        if (write_buffer_size != sizeof(NetworkInfo)) {
            LOG_ERROR(Service_LDN, "Invalid buffer size {}", write_buffer_size);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultBadInput);
            return;
        }

        NetworkInfo network_info{};
        const auto rc = ResultSuccess;
        if (rc.IsError()) {
            LOG_ERROR(Service_LDN, "NetworkInfo is not valid {}", rc.raw);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(rc);
            return;
        }

        LOG_WARNING(Service_LDN, "(STUBBED) called, ssid='{}', nodes={}",
                    network_info.common.ssid.GetStringValue(), network_info.ldn.node_count);

        ctx.WriteBuffer<NetworkInfo>(network_info);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(rc);
    }

    void GetDisconnectReason(Kernel::HLERequestContext& ctx) {
        const auto disconnect_reason = DisconnectReason::None;

        LOG_WARNING(Service_LDN, "(STUBBED) called, disconnect_reason={}", disconnect_reason);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(disconnect_reason);
    }

    void GetSecurityParameter(Kernel::HLERequestContext& ctx) {
        SecurityParameter security_parameter{};
        NetworkInfo info{};
        const Result rc = ResultSuccess;

        if (rc.IsError()) {
            LOG_ERROR(Service_LDN, "NetworkInfo is not valid {}", rc.raw);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(rc);
            return;
        }

        security_parameter.session_id = info.network_id.session_id;
        std::memcpy(security_parameter.data.data(), info.ldn.security_parameter.data(),
                    sizeof(SecurityParameter::data));

        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 10};
        rb.Push(rc);
        rb.PushRaw<SecurityParameter>(security_parameter);
    }

    void GetNetworkConfig(Kernel::HLERequestContext& ctx) {
        NetworkConfig config{};
        NetworkInfo info{};
        const Result rc = ResultSuccess;

        if (rc.IsError()) {
            LOG_ERROR(Service_LDN, "NetworkConfig is not valid {}", rc.raw);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(rc);
            return;
        }

        config.intent_id = info.network_id.intent_id;
        config.channel = info.common.channel;
        config.node_count_max = info.ldn.node_count_max;
        config.local_communication_version = info.ldn.nodes[0].local_communication_version;

        LOG_WARNING(Service_LDN,
                    "(STUBBED) called, intent_id={}/{}, channel={}, node_count_max={}, "
                    "local_communication_version={}",
                    config.intent_id.local_communication_id, config.intent_id.scene_id,
                    config.channel, config.node_count_max, config.local_communication_version);

        IPC::ResponseBuilder rb{ctx, 10};
        rb.Push(rc);
        rb.PushRaw<NetworkConfig>(config);
    }

    void AttachStateChangeEvent(Kernel::HLERequestContext& ctx) {
        LOG_INFO(Service_LDN, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(state_change_event->GetReadableEvent());
    }

    void GetNetworkInfoLatestUpdate(Kernel::HLERequestContext& ctx) {
        const std::size_t network_buffer_size = ctx.GetWriteBufferSize(0);
        const std::size_t node_buffer_count = ctx.GetWriteBufferSize(1) / sizeof(NodeLatestUpdate);

        if (node_buffer_count == 0 || network_buffer_size != sizeof(NetworkInfo)) {
            LOG_ERROR(Service_LDN, "Invalid buffer size {}, {}", network_buffer_size,
                      node_buffer_count);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultBadInput);
            return;
        }

        NetworkInfo info;
        std::vector<NodeLatestUpdate> latest_update(node_buffer_count);

        const auto rc = ResultSuccess;
        if (rc.IsError()) {
            LOG_ERROR(Service_LDN, "NetworkInfo is not valid {}", rc.raw);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(rc);
            return;
        }

        LOG_WARNING(Service_LDN, "(STUBBED) called, ssid='{}', nodes={}",
                    info.common.ssid.GetStringValue(), info.ldn.node_count);

        ctx.WriteBuffer(info, 0);
        ctx.WriteBuffer(latest_update, 1);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Scan(Kernel::HLERequestContext& ctx) {
        ScanImpl(ctx);
    }

    void ScanPrivate(Kernel::HLERequestContext& ctx) {
        ScanImpl(ctx, true);
    }

    void ScanImpl(Kernel::HLERequestContext& ctx, bool is_private = false) {
        IPC::RequestParser rp{ctx};
        const auto channel{rp.PopEnum<WifiChannel>()};
        const auto scan_filter{rp.PopRaw<ScanFilter>()};

        const std::size_t network_info_size = ctx.GetWriteBufferSize() / sizeof(NetworkInfo);

        if (network_info_size == 0) {
            LOG_ERROR(Service_LDN, "Invalid buffer size {}", network_info_size);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultBadInput);
            return;
        }

        u16 count = 0;
        std::vector<NetworkInfo> network_infos(network_info_size);

        LOG_WARNING(Service_LDN,
                    "(STUBBED) called, channel={}, filter_scan_flag={}, filter_network_type={}",
                    channel, scan_filter.flag, scan_filter.network_type);

        ctx.WriteBuffer(network_infos);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(count);
    }

    void OpenAccessPoint(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CloseAccessPoint(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CreateNetwork(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        struct Parameters {
            SecurityConfig security_config;
            UserConfig user_config;
            INSERT_PADDING_WORDS_NOINIT(1);
            NetworkConfig network_config;
        };
        static_assert(sizeof(Parameters) == 0x98, "Parameters has incorrect size.");

        const auto parameters{rp.PopRaw<Parameters>()};

        LOG_WARNING(Service_LDN,
                    "(STUBBED) called, passphrase_size={}, security_mode={}, "
                    "local_communication_version={}",
                    parameters.security_config.passphrase_size,
                    parameters.security_config.security_mode,
                    parameters.network_config.local_communication_version);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CreateNetworkPrivate(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        struct Parameters {
            SecurityConfig security_config;
            SecurityParameter security_parameter;
            UserConfig user_config;
            NetworkConfig network_config;
        };
        static_assert(sizeof(Parameters) == 0xB8, "Parameters has incorrect size.");

        const auto parameters{rp.PopRaw<Parameters>()};

        LOG_WARNING(Service_LDN,
                    "(STUBBED) called, passphrase_size={}, security_mode={}, "
                    "local_communication_version={}",
                    parameters.security_config.passphrase_size,
                    parameters.security_config.security_mode,
                    parameters.network_config.local_communication_version);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void DestroyNetwork(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetAdvertiseData(Kernel::HLERequestContext& ctx) {
        std::vector<u8> read_buffer = ctx.ReadBuffer();

        LOG_WARNING(Service_LDN, "(STUBBED) called, size {}", read_buffer.size());

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetStationAcceptPolicy(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void AddAcceptFilterEntry(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void OpenStation(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CloseStation(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Connect(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        struct Parameters {
            SecurityConfig security_config;
            UserConfig user_config;
            u32 local_communication_version;
            u32 option;
        };
        static_assert(sizeof(Parameters) == 0x7C, "Parameters has incorrect size.");

        const auto parameters{rp.PopRaw<Parameters>()};

        LOG_WARNING(Service_LDN,
                    "(STUBBED) called, passphrase_size={}, security_mode={}, "
                    "local_communication_version={}",
                    parameters.security_config.passphrase_size,
                    parameters.security_config.security_mode,
                    parameters.local_communication_version);

        const std::vector<u8> read_buffer = ctx.ReadBuffer();
        NetworkInfo network_info{};

        if (read_buffer.size() != sizeof(NetworkInfo)) {
            LOG_ERROR(Frontend, "NetworkInfo doesn't match read_buffer size!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ResultBadInput);
            return;
        }

        std::memcpy(&network_info, read_buffer.data(), read_buffer.size());

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Disconnect(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        const auto rc = InitializeImpl(ctx);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(rc);
    }

    void Finalize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        is_initialized = false;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Initialize2(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_LDN, "(STUBBED) called");

        const auto rc = InitializeImpl(ctx);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(rc);
    }

    Result InitializeImpl(Kernel::HLERequestContext& ctx) {
        const auto network_interface = Network::GetSelectedNetworkInterface();
        if (!network_interface) {
            LOG_ERROR(Service_LDN, "No network interface is set");
            return ResultAirplaneModeEnabled;
        }

        is_initialized = true;
        // TODO (flTobi): Change this to ResultSuccess when LDN is fully implemented
        return ResultAirplaneModeEnabled;
    }

    KernelHelpers::ServiceContext service_context;
    Kernel::KEvent* state_change_event;
    Network::RoomNetwork& room_network;

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
        rb.Push(ResultDisabled);
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
