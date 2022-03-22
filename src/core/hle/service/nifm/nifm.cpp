// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/nifm/nifm.h"
#include "core/hle/service/service.h"

namespace {

// Avoids name conflict with Windows' CreateEvent macro.
[[nodiscard]] Kernel::KEvent* CreateKEvent(Service::KernelHelpers::ServiceContext& service_context,
                                           std::string&& name) {
    return service_context.CreateEvent(std::move(name));
}

} // Anonymous namespace

#include "core/network/network.h"
#include "core/network/network_interface.h"

namespace Service::NIFM {

enum class RequestState : u32 {
    NotSubmitted = 1,
    Error = 1, ///< The duplicate 1 is intentional; it means both not submitted and error on HW.
    Pending = 2,
    Connected = 3,
};

struct IpAddressSetting {
    bool is_automatic{};
    Network::IPv4Address current_address{};
    Network::IPv4Address subnet_mask{};
    Network::IPv4Address gateway{};
};
static_assert(sizeof(IpAddressSetting) == 0xD, "IpAddressSetting has incorrect size.");

struct DnsSetting {
    bool is_automatic{};
    Network::IPv4Address primary_dns{};
    Network::IPv4Address secondary_dns{};
};
static_assert(sizeof(DnsSetting) == 0x9, "DnsSetting has incorrect size.");

struct ProxySetting {
    bool enabled{};
    INSERT_PADDING_BYTES(1);
    u16 port{};
    std::array<char, 0x64> proxy_server{};
    bool automatic_auth_enabled{};
    std::array<char, 0x20> user{};
    std::array<char, 0x20> password{};
    INSERT_PADDING_BYTES(1);
};
static_assert(sizeof(ProxySetting) == 0xAA, "ProxySetting has incorrect size.");

struct IpSettingData {
    IpAddressSetting ip_address_setting{};
    DnsSetting dns_setting{};
    ProxySetting proxy_setting{};
    u16 mtu{};
};
static_assert(sizeof(IpSettingData) == 0xC2, "IpSettingData has incorrect size.");

struct SfWirelessSettingData {
    u8 ssid_length{};
    std::array<char, 0x20> ssid{};
    u8 unknown_1{};
    u8 unknown_2{};
    u8 unknown_3{};
    std::array<char, 0x41> passphrase{};
};
static_assert(sizeof(SfWirelessSettingData) == 0x65, "SfWirelessSettingData has incorrect size.");

struct NifmWirelessSettingData {
    u8 ssid_length{};
    std::array<char, 0x21> ssid{};
    u8 unknown_1{};
    INSERT_PADDING_BYTES(1);
    u32 unknown_2{};
    u32 unknown_3{};
    std::array<char, 0x41> passphrase{};
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(NifmWirelessSettingData) == 0x70,
              "NifmWirelessSettingData has incorrect size.");

#pragma pack(push, 1)
struct SfNetworkProfileData {
    IpSettingData ip_setting_data{};
    u128 uuid{};
    std::array<char, 0x40> network_name{};
    u8 unknown_1{};
    u8 unknown_2{};
    u8 unknown_3{};
    u8 unknown_4{};
    SfWirelessSettingData wireless_setting_data{};
    INSERT_PADDING_BYTES(1);
};
static_assert(sizeof(SfNetworkProfileData) == 0x17C, "SfNetworkProfileData has incorrect size.");

struct NifmNetworkProfileData {
    u128 uuid{};
    std::array<char, 0x40> network_name{};
    u32 unknown_1{};
    u32 unknown_2{};
    u8 unknown_3{};
    u8 unknown_4{};
    INSERT_PADDING_BYTES(2);
    NifmWirelessSettingData wireless_setting_data{};
    IpSettingData ip_setting_data{};
};
static_assert(sizeof(NifmNetworkProfileData) == 0x18E,
              "NifmNetworkProfileData has incorrect size.");
#pragma pack(pop)

class IScanRequest final : public ServiceFramework<IScanRequest> {
public:
    explicit IScanRequest(Core::System& system_) : ServiceFramework{system_, "IScanRequest"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Submit"},
            {1, nullptr, "IsProcessing"},
            {2, nullptr, "GetResult"},
            {3, nullptr, "GetSystemEventReadableHandle"},
            {4, nullptr, "SetChannels"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IRequest final : public ServiceFramework<IRequest> {
public:
    explicit IRequest(Core::System& system_)
        : ServiceFramework{system_, "IRequest"}, service_context{system_, "IRequest"} {
        static const FunctionInfo functions[] = {
            {0, &IRequest::GetRequestState, "GetRequestState"},
            {1, &IRequest::GetResult, "GetResult"},
            {2, &IRequest::GetSystemEventReadableHandles, "GetSystemEventReadableHandles"},
            {3, &IRequest::Cancel, "Cancel"},
            {4, &IRequest::Submit, "Submit"},
            {5, nullptr, "SetRequirement"},
            {6, nullptr, "SetRequirementPreset"},
            {8, nullptr, "SetPriority"},
            {9, nullptr, "SetNetworkProfileId"},
            {10, nullptr, "SetRejectable"},
            {11, &IRequest::SetConnectionConfirmationOption, "SetConnectionConfirmationOption"},
            {12, nullptr, "SetPersistent"},
            {13, nullptr, "SetInstant"},
            {14, nullptr, "SetSustainable"},
            {15, nullptr, "SetRawPriority"},
            {16, nullptr, "SetGreedy"},
            {17, nullptr, "SetSharable"},
            {18, nullptr, "SetRequirementByRevision"},
            {19, nullptr, "GetRequirement"},
            {20, nullptr, "GetRevision"},
            {21, &IRequest::GetAppletInfo, "GetAppletInfo"},
            {22, nullptr, "GetAdditionalInfo"},
            {23, nullptr, "SetKeptInSleep"},
            {24, nullptr, "RegisterSocketDescriptor"},
            {25, nullptr, "UnregisterSocketDescriptor"},
        };
        RegisterHandlers(functions);

        event1 = CreateKEvent(service_context, "IRequest:Event1");
        event2 = CreateKEvent(service_context, "IRequest:Event2");
    }

    ~IRequest() override {
        service_context.CloseEvent(event1);
        service_context.CloseEvent(event2);
    }

private:
    void Submit(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetRequestState(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);

        if (Network::GetHostIPv4Address().has_value()) {
            rb.PushEnum(RequestState::Connected);
        } else {
            rb.PushEnum(RequestState::NotSubmitted);
        }
    }

    void GetResult(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetSystemEventReadableHandles(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 2};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(event1->GetReadableEvent(), event2->GetReadableEvent());
    }

    void Cancel(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetConnectionConfirmationOption(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetAppletInfo(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        std::vector<u8> out_buffer(ctx.GetWriteBufferSize());

        ctx.WriteBuffer(out_buffer);

        IPC::ResponseBuilder rb{ctx, 5};
        rb.Push(ResultSuccess);
        rb.Push<u32>(0);
        rb.Push<u32>(0);
        rb.Push<u32>(0);
    }

    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* event1;
    Kernel::KEvent* event2;
};

class INetworkProfile final : public ServiceFramework<INetworkProfile> {
public:
    explicit INetworkProfile(Core::System& system_) : ServiceFramework{system_, "INetworkProfile"} {
        static const FunctionInfo functions[] = {
            {0, nullptr, "Update"},
            {1, nullptr, "PersistOld"},
            {2, nullptr, "Persist"},
        };
        RegisterHandlers(functions);
    }
};

class IGeneralService final : public ServiceFramework<IGeneralService> {
public:
    explicit IGeneralService(Core::System& system_);

private:
    void GetClientId(Kernel::HLERequestContext& ctx) {
        static constexpr u32 client_id = 1;
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u64>(client_id); // Client ID needs to be non zero otherwise it's considered invalid
    }
    void CreateScanRequest(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIFM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IScanRequest>(system);
    }
    void CreateRequest(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIFM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IRequest>(system);
    }
    void GetCurrentNetworkProfile(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        const auto net_iface = Network::GetSelectedNetworkInterface();

        const SfNetworkProfileData network_profile_data = [&net_iface] {
            if (!net_iface) {
                return SfNetworkProfileData{};
            }

            return SfNetworkProfileData{
                .ip_setting_data{
                    .ip_address_setting{
                        .is_automatic{true},
                        .current_address{Network::TranslateIPv4(net_iface->ip_address)},
                        .subnet_mask{Network::TranslateIPv4(net_iface->subnet_mask)},
                        .gateway{Network::TranslateIPv4(net_iface->gateway)},
                    },
                    .dns_setting{
                        .is_automatic{true},
                        .primary_dns{1, 1, 1, 1},
                        .secondary_dns{1, 0, 0, 1},
                    },
                    .proxy_setting{
                        .enabled{false},
                        .port{},
                        .proxy_server{},
                        .automatic_auth_enabled{},
                        .user{},
                        .password{},
                    },
                    .mtu{1500},
                },
                .uuid{0xdeadbeef, 0xdeadbeef},
                .network_name{"yuzu Network"},
                .wireless_setting_data{
                    .ssid_length{12},
                    .ssid{"yuzu Network"},
                    .passphrase{"yuzupassword"},
                },
            };
        }();

        ctx.WriteBuffer(network_profile_data);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
    void RemoveNetworkProfile(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
    void GetCurrentIpAddress(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        auto ipv4 = Network::GetHostIPv4Address();
        if (!ipv4) {
            LOG_ERROR(Service_NIFM, "Couldn't get host IPv4 address, defaulting to 0.0.0.0");
            ipv4.emplace(Network::IPv4Address{0, 0, 0, 0});
        }

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushRaw(*ipv4);
    }
    void CreateTemporaryNetworkProfile(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIFM, "called");

        ASSERT_MSG(ctx.GetReadBufferSize() == 0x17c,
                   "SfNetworkProfileData is not the correct size");
        u128 uuid{};
        auto buffer = ctx.ReadBuffer();
        std::memcpy(&uuid, buffer.data() + 8, sizeof(u128));

        IPC::ResponseBuilder rb{ctx, 6, 0, 1};

        rb.Push(ResultSuccess);
        rb.PushIpcInterface<INetworkProfile>(system);
        rb.PushRaw<u128>(uuid);
    }
    void GetCurrentIpConfigInfo(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        struct IpConfigInfo {
            IpAddressSetting ip_address_setting{};
            DnsSetting dns_setting{};
        };
        static_assert(sizeof(IpConfigInfo) == sizeof(IpAddressSetting) + sizeof(DnsSetting),
                      "IpConfigInfo has incorrect size.");

        const auto net_iface = Network::GetSelectedNetworkInterface();

        const IpConfigInfo ip_config_info = [&net_iface] {
            if (!net_iface) {
                return IpConfigInfo{};
            }

            return IpConfigInfo{
                .ip_address_setting{
                    .is_automatic{true},
                    .current_address{Network::TranslateIPv4(net_iface->ip_address)},
                    .subnet_mask{Network::TranslateIPv4(net_iface->subnet_mask)},
                    .gateway{Network::TranslateIPv4(net_iface->gateway)},
                },
                .dns_setting{
                    .is_automatic{true},
                    .primary_dns{1, 1, 1, 1},
                    .secondary_dns{1, 0, 0, 1},
                },
            };
        }();

        IPC::ResponseBuilder rb{ctx, 2 + (sizeof(IpConfigInfo) + 3) / sizeof(u32)};
        rb.Push(ResultSuccess);
        rb.PushRaw<IpConfigInfo>(ip_config_info);
    }
    void IsWirelessCommunicationEnabled(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u8>(0);
    }
    void IsEthernetCommunicationEnabled(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        if (Network::GetHostIPv4Address().has_value()) {
            rb.Push<u8>(1);
        } else {
            rb.Push<u8>(0);
        }
    }
    void IsAnyInternetRequestAccepted(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIFM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        if (Network::GetHostIPv4Address().has_value()) {
            rb.Push<u8>(1);
        } else {
            rb.Push<u8>(0);
        }
    }
};

IGeneralService::IGeneralService(Core::System& system_)
    : ServiceFramework{system_, "IGeneralService"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {1, &IGeneralService::GetClientId, "GetClientId"},
        {2, &IGeneralService::CreateScanRequest, "CreateScanRequest"},
        {4, &IGeneralService::CreateRequest, "CreateRequest"},
        {5, &IGeneralService::GetCurrentNetworkProfile, "GetCurrentNetworkProfile"},
        {6, nullptr, "EnumerateNetworkInterfaces"},
        {7, nullptr, "EnumerateNetworkProfiles"},
        {8, nullptr, "GetNetworkProfile"},
        {9, nullptr, "SetNetworkProfile"},
        {10, &IGeneralService::RemoveNetworkProfile, "RemoveNetworkProfile"},
        {11, nullptr, "GetScanDataOld"},
        {12, &IGeneralService::GetCurrentIpAddress, "GetCurrentIpAddress"},
        {13, nullptr, "GetCurrentAccessPointOld"},
        {14, &IGeneralService::CreateTemporaryNetworkProfile, "CreateTemporaryNetworkProfile"},
        {15, &IGeneralService::GetCurrentIpConfigInfo, "GetCurrentIpConfigInfo"},
        {16, nullptr, "SetWirelessCommunicationEnabled"},
        {17, &IGeneralService::IsWirelessCommunicationEnabled, "IsWirelessCommunicationEnabled"},
        {18, nullptr, "GetInternetConnectionStatus"},
        {19, nullptr, "SetEthernetCommunicationEnabled"},
        {20, &IGeneralService::IsEthernetCommunicationEnabled, "IsEthernetCommunicationEnabled"},
        {21, &IGeneralService::IsAnyInternetRequestAccepted, "IsAnyInternetRequestAccepted"},
        {22, nullptr, "IsAnyForegroundRequestAccepted"},
        {23, nullptr, "PutToSleep"},
        {24, nullptr, "WakeUp"},
        {25, nullptr, "GetSsidListVersion"},
        {26, nullptr, "SetExclusiveClient"},
        {27, nullptr, "GetDefaultIpSetting"},
        {28, nullptr, "SetDefaultIpSetting"},
        {29, nullptr, "SetWirelessCommunicationEnabledForTest"},
        {30, nullptr, "SetEthernetCommunicationEnabledForTest"},
        {31, nullptr, "GetTelemetorySystemEventReadableHandle"},
        {32, nullptr, "GetTelemetryInfo"},
        {33, nullptr, "ConfirmSystemAvailability"},
        {34, nullptr, "SetBackgroundRequestEnabled"},
        {35, nullptr, "GetScanData"},
        {36, nullptr, "GetCurrentAccessPoint"},
        {37, nullptr, "Shutdown"},
        {38, nullptr, "GetAllowedChannels"},
        {39, nullptr, "NotifyApplicationSuspended"},
        {40, nullptr, "SetAcceptableNetworkTypeFlag"},
        {41, nullptr, "GetAcceptableNetworkTypeFlag"},
        {42, nullptr, "NotifyConnectionStateChanged"},
        {43, nullptr, "SetWowlDelayedWakeTime"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

class NetworkInterface final : public ServiceFramework<NetworkInterface> {
public:
    explicit NetworkInterface(const char* name, Core::System& system_)
        : ServiceFramework{system_, name} {
        static const FunctionInfo functions[] = {
            {4, &NetworkInterface::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
            {5, &NetworkInterface::CreateGeneralService, "CreateGeneralService"},
        };
        RegisterHandlers(functions);
    }

private:
    void CreateGeneralServiceOld(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIFM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IGeneralService>(system);
    }

    void CreateGeneralService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIFM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IGeneralService>(system);
    }
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<NetworkInterface>("nifm:a", system)->InstallAsService(service_manager);
    std::make_shared<NetworkInterface>("nifm:s", system)->InstallAsService(service_manager);
    std::make_shared<NetworkInterface>("nifm:u", system)->InstallAsService(service_manager);
}

} // namespace Service::NIFM
