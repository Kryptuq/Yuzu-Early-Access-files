// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <span>
#include <thread>
#include <unordered_map>

#include "common/logging/log.h"
#include "common/socket_types.h"
#include "core/hle/result.h"
#include "core/hle/service/ldn/ldn_results.h"
#include "core/hle/service/ldn/ldn_types.h"
#include "network/network.h"

namespace Service::LDN {

class LANDiscovery;

class LanStation {
public:
    LanStation(s8 node_id_, LANDiscovery* discovery_);
    ~LanStation();

    void OnClose();
    NodeStatus GetStatus() const;
    void Reset();
    void OverrideInfo();

protected:
    friend class LANDiscovery;
    NodeInfo* node_info;
    NodeStatus status;
    s8 node_id;
    LANDiscovery* discovery;
};

class LANDiscovery {
public:
    using LanEventFunc = std::function<void()>;

    LANDiscovery(Network::RoomNetwork& room_network_);
    ~LANDiscovery();

    State GetState() const;
    void SetState(State new_state);

    Result GetNetworkInfo(NetworkInfo& out_network) const;
    Result GetNetworkInfo(NetworkInfo& out_network, std::vector<NodeLatestUpdate>& out_updates,
                          std::size_t buffer_count);

    DisconnectReason GetDisconnectReason() const;
    Result Scan(std::vector<NetworkInfo>& networks, u16& count, const ScanFilter& filter);
    Result SetAdvertiseData(std::span<const u8> data);

    Result OpenAccessPoint();
    Result CloseAccessPoint();

    Result OpenStation();
    Result CloseStation();

    Result CreateNetwork(const SecurityConfig& security_config, const UserConfig& user_config,
                         const NetworkConfig& network_config);
    Result DestroyNetwork();

    Result Connect(const NetworkInfo& network_info_, const UserConfig& user_config,
                   u16 local_communication_version);
    Result Disconnect();

    Result Initialize(LanEventFunc lan_event_ = empty_func, bool listening = true);
    Result Finalize();

    void ReceivePacket(const Network::LDNPacket& packet);

protected:
    friend class LanStation;

    void InitNetworkInfo();
    void InitNodeStateChange();

    void ResetStations();
    void UpdateNodes();

    void OnSyncNetwork(const NetworkInfo& info);
    void OnDisconnectFromHost();
    void OnNetworkInfoChanged();

    bool IsNodeStateChanged();
    bool IsFlagSet(ScanFilterFlag flag, ScanFilterFlag search_flag) const;
    int GetStationCount() const;
    MacAddress GetFakeMac() const;
    Result GetNodeInfo(NodeInfo& node, const UserConfig& user_config,
                       u16 local_communication_version);

    Network::IPv4Address GetLocalIp() const;
    template <typename Data>
    void SendPacket(Network::LDNPacketType type, const Data& data, Ipv4Address remote_ip);
    void SendPacket(Network::LDNPacketType type, Ipv4Address remote_ip);
    template <typename Data>
    void SendBroadcast(Network::LDNPacketType type, const Data& data);
    void SendBroadcast(Network::LDNPacketType type);
    void SendPacket(const Network::LDNPacket& packet);

    static const LanEventFunc empty_func;
    const Ssid fake_ssid{"YuzuFakeSsidForLdn"};

    bool inited{};
    std::mutex packet_mutex;
    std::array<LanStation, StationCountMax> stations;
    std::array<NodeLatestUpdate, NodeCountMax> node_changes{};
    std::array<u8, NodeCountMax> node_last_states{};
    std::unordered_map<MacAddress, NetworkInfo, MACAddressHash> scan_results{};
    NodeInfo node_info{};
    NetworkInfo network_info{};
    State state{State::None};
    DisconnectReason disconnect_reason{DisconnectReason::None};

    // TODO (flTobi): Should this be an std::set?
    std::vector<Ipv4Address> connected_clients;
    std::optional<Ipv4Address> host_ip;

    LanEventFunc LanEvent;

    Network::RoomNetwork& room_network;
};
} // namespace Service::LDN
