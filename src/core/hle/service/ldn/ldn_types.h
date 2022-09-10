// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fmt/format.h>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "network/network.h"

namespace Service::LDN {

constexpr size_t SsidLengthMax = 32;
constexpr size_t AdvertiseDataSizeMax = 384;
constexpr size_t UserNameBytesMax = 32;
constexpr int NodeCountMax = 8;
constexpr int StationCountMax = NodeCountMax - 1;
constexpr size_t PassphraseLengthMax = 64;

enum class SecurityMode : u16 {
    All,
    Retail,
    Debug,
};

enum class NodeStateChange : u8 {
    None,
    Connect,
    Disconnect,
    DisconnectAndConnect,
};

inline NodeStateChange operator|(NodeStateChange a, NodeStateChange b) {
    return static_cast<NodeStateChange>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline NodeStateChange operator|=(NodeStateChange& a, NodeStateChange b) {
    return a = a | b;
}

enum class ScanFilterFlag : u32 {
    None = 0,
    LocalCommunicationId = 1 << 0,
    SessionId = 1 << 1,
    NetworkType = 1 << 2,
    Ssid = 1 << 4,
    SceneId = 1 << 5,
    IntentId = LocalCommunicationId | SceneId,
    NetworkId = IntentId | SessionId,
};

enum class NetworkType : u32 {
    None,
    General,
    Ldn,
    All,
};

enum class PackedNetworkType : u8 {
    None,
    General,
    Ldn,
    All,
};

enum class State : u32 {
    None,
    Initialized,
    AccessPointOpened,
    AccessPointCreated,
    StationOpened,
    StationConnected,
    Error,
};

enum class DisconnectReason : s16 {
    Unknown = -1,
    None,
    DisconnectedByUser,
    DisconnectedBySystem,
    DestroyedByUser,
    DestroyedBySystem,
    Rejected,
    SignalLost,
};

enum class NetworkError {
    Unknown = -1,
    None = 0,
    PortUnreachable,
    TooManyPlayers,
    VersionTooLow,
    VersionTooHigh,
    ConnectFailure,
    ConnectNotFound,
    ConnectTimeout,
    ConnectRejected,
    RejectFailed,
};

enum class AcceptPolicy : u8 {
    AcceptAll,
    RejectAll,
    BlackList,
    WhiteList,
};

enum class WifiChannel : s16 {
    Default = 0,
    Wifi24_1 = 1,
    Wifi24_6 = 6,
    Wifi24_11 = 11,
    Wifi50_36 = 36,
    Wifi50_40 = 40,
    Wifi50_44 = 44,
    Wifi50_48 = 48,
};

enum class LinkLevel : s8 {
    Bad,
    Low,
    Good,
    Excellent,
};

enum class NodeStatus : u8 {
    Disconnected,
    Connected,
};

struct NodeLatestUpdate {
    NodeStateChange state_change;
    INSERT_PADDING_BYTES(0x7); // Unknown
};
static_assert(sizeof(NodeLatestUpdate) == 0x8, "NodeLatestUpdate is an invalid size");

struct SessionId {
    u64 high;
    u64 low;

    bool operator==(const SessionId&) const = default;
};
static_assert(sizeof(SessionId) == 0x10, "SessionId is an invalid size");

struct IntentId {
    u64 local_communication_id;
    INSERT_PADDING_BYTES(0x2); // Reserved
    u16 scene_id;
    INSERT_PADDING_BYTES(0x4); // Reserved
};
static_assert(sizeof(IntentId) == 0x10, "IntentId is an invalid size");

struct NetworkId {
    IntentId intent_id;
    SessionId session_id;
};
static_assert(sizeof(NetworkId) == 0x20, "NetworkId is an invalid size");

struct Ssid {
    u8 length{};
    std::array<char, SsidLengthMax + 1> raw{};

    Ssid() = default;

    explicit Ssid(std::string_view data) {
        length = static_cast<u8>(std::min(data.size(), SsidLengthMax));
        data.copy(raw.data(), length);
        raw[length] = 0;
    }

    std::string GetStringValue() const {
        return std::string(raw.data());
    }

    bool operator==(const Ssid& b) const {
        return (length == b.length) && (std::memcmp(raw.data(), b.raw.data(), length) == 0);
    }
};
static_assert(sizeof(Ssid) == 0x22, "Ssid is an invalid size");

using Ipv4Address = std::array<u8, 4>;
static_assert(sizeof(Ipv4Address) == 0x4, "Ipv4Address is an invalid size");

struct MacAddress {
    std::array<u8, 6> raw{};

    friend bool operator==(const MacAddress& lhs, const MacAddress& rhs) = default;
};
static_assert(sizeof(MacAddress) == 0x6, "MacAddress is an invalid size");

struct MACAddressHash {
    size_t operator()(const MacAddress& address) const {
        u64 value{};
        std::memcpy(&value, address.raw.data(), sizeof(address.raw));
        return value;
    }
};

struct ScanFilter {
    NetworkId network_id;
    NetworkType network_type;
    MacAddress mac_address;
    Ssid ssid;
    INSERT_PADDING_BYTES(0x10);
    ScanFilterFlag flag;
};
static_assert(sizeof(ScanFilter) == 0x60, "ScanFilter is an invalid size");

struct CommonNetworkInfo {
    MacAddress bssid;
    Ssid ssid;
    WifiChannel channel;
    LinkLevel link_level;
    PackedNetworkType network_type;
    INSERT_PADDING_BYTES(0x4);
};
static_assert(sizeof(CommonNetworkInfo) == 0x30, "CommonNetworkInfo is an invalid size");

struct NodeInfo {
    Ipv4Address ipv4_address;
    MacAddress mac_address;
    s8 node_id;
    u8 is_connected;
    std::array<u8, UserNameBytesMax + 1> user_name;
    INSERT_PADDING_BYTES(0x1); // Reserved
    s16 local_communication_version;
    INSERT_PADDING_BYTES(0x10); // Reserved
};
static_assert(sizeof(NodeInfo) == 0x40, "NodeInfo is an invalid size");

struct LdnNetworkInfo {
    std::array<u8, 0x10> security_parameter;
    SecurityMode security_mode;
    AcceptPolicy station_accept_policy;
    u8 has_action_frame;
    INSERT_PADDING_BYTES(0x2); // Padding
    u8 node_count_max;
    u8 node_count;
    std::array<NodeInfo, NodeCountMax> nodes;
    INSERT_PADDING_BYTES(0x2); // Reserved
    u16 advertise_data_size;
    std::array<u8, AdvertiseDataSizeMax> advertise_data;
    INSERT_PADDING_BYTES(0x8C); // Reserved
    u64 random_authentication_id;
};
static_assert(sizeof(LdnNetworkInfo) == 0x430, "LdnNetworkInfo is an invalid size");

struct NetworkInfo {
    NetworkId network_id;
    CommonNetworkInfo common;
    LdnNetworkInfo ldn;
};
static_assert(sizeof(NetworkInfo) == 0x480, "NetworkInfo is an invalid size");

struct SecurityConfig {
    SecurityMode security_mode;
    u16 passphrase_size;
    std::array<u8, PassphraseLengthMax> passphrase;
};
static_assert(sizeof(SecurityConfig) == 0x44, "SecurityConfig is an invalid size");

struct UserConfig {
    std::array<u8, UserNameBytesMax + 1> user_name;
    INSERT_PADDING_BYTES(0xF); // Reserved
};
static_assert(sizeof(UserConfig) == 0x30, "UserConfig is an invalid size");

#pragma pack(push, 4)
struct ConnectRequest {
    SecurityConfig security_config;
    UserConfig user_config;
    u32 local_communication_version;
    u32 option_unknown;
    NetworkInfo network_info;
};
static_assert(sizeof(ConnectRequest) == 0x4FC, "ConnectRequest is an invalid size");
#pragma pack(pop)

struct SecurityParameter {
    std::array<u8, 0x10> data; // Data, used with the same key derivation as SecurityConfig
    SessionId session_id;
};
static_assert(sizeof(SecurityParameter) == 0x20, "SecurityParameter is an invalid size");

struct NetworkConfig {
    IntentId intent_id;
    WifiChannel channel;
    u8 node_count_max;
    INSERT_PADDING_BYTES(0x1); // Reserved
    u16 local_communication_version;
    INSERT_PADDING_BYTES(0xA); // Reserved
};
static_assert(sizeof(NetworkConfig) == 0x20, "NetworkConfig is an invalid size");

struct AddressEntry {
    Ipv4Address ipv4_address;
    MacAddress mac_address;
    INSERT_PADDING_BYTES(0x2); // Reserved
};
static_assert(sizeof(AddressEntry) == 0xC, "AddressEntry is an invalid size");

struct AddressList {
    std::array<AddressEntry, 0x8> addresses;
};
static_assert(sizeof(AddressList) == 0x60, "AddressList is an invalid size");

} // namespace Service::LDN
