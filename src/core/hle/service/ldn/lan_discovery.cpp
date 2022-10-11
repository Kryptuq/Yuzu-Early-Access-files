// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/ldn/lan_discovery.h"
#include "core/internal_network/network.h"
#include "core/internal_network/network_interface.h"

namespace Service::LDN {

LanStation::LanStation(s8 node_id_, LANDiscovery* discovery_)
    : node_info(nullptr), status(NodeStatus::Disconnected), node_id(node_id_),
      discovery(discovery_) {}

LanStation::~LanStation() = default;

NodeStatus LanStation::GetStatus() const {
    return status;
}

void LanStation::OnClose() {
    LOG_INFO(Service_LDN, "OnClose {}", node_id);
    Reset();
    discovery->UpdateNodes();
}

void LanStation::Reset() {
    status = NodeStatus::Disconnected;
};

void LanStation::OverrideInfo() {
    bool connected = GetStatus() == NodeStatus::Connected;
    node_info->node_id = node_id;
    node_info->is_connected = connected ? 1 : 0;
}

LANDiscovery::LANDiscovery(Network::RoomNetwork& room_network_)
    : stations({{{1, this}, {2, this}, {3, this}, {4, this}, {5, this}, {6, this}, {7, this}}}),
      room_network{room_network_} {}

LANDiscovery::~LANDiscovery() {
    if (inited) {
        Result rc = Finalize();
        LOG_INFO(Service_LDN, "Finalize: {}", rc.raw);
    }
}

void LANDiscovery::InitNetworkInfo() {
    network_info.common.bssid = GetFakeMac();
    network_info.common.channel = WifiChannel::Wifi24_6;
    network_info.common.link_level = LinkLevel::Good;
    network_info.common.network_type = PackedNetworkType::Ldn;
    network_info.common.ssid = fake_ssid;

    auto& nodes = network_info.ldn.nodes;
    for (std::size_t i = 0; i < NodeCountMax; i++) {
        nodes[i].node_id = static_cast<s8>(i);
        nodes[i].is_connected = 0;
    }
}

void LANDiscovery::InitNodeStateChange() {
    for (auto& node_update : node_changes) {
        node_update.state_change = NodeStateChange::None;
    }
    for (auto& node_state : node_last_states) {
        node_state = 0;
    }
}

State LANDiscovery::GetState() const {
    return state;
}

void LANDiscovery::SetState(State new_state) {
    state = new_state;
}

Result LANDiscovery::GetNetworkInfo(NetworkInfo& out_network) const {
    if (state == State::AccessPointCreated || state == State::StationConnected) {
        std::memcpy(&out_network, &network_info, sizeof(network_info));
        return ResultSuccess;
    }

    return ResultBadState;
}

Result LANDiscovery::GetNetworkInfo(NetworkInfo& out_network,
                                    std::vector<NodeLatestUpdate>& out_updates,
                                    std::size_t buffer_count) {
    if (buffer_count > NodeCountMax) {
        return ResultInvalidBufferCount;
    }

    if (state == State::AccessPointCreated || state == State::StationConnected) {
        std::memcpy(&out_network, &network_info, sizeof(network_info));
        for (std::size_t i = 0; i < buffer_count; i++) {
            out_updates[i].state_change = node_changes[i].state_change;
            node_changes[i].state_change = NodeStateChange::None;
        }
        return ResultSuccess;
    }

    return ResultBadState;
}

DisconnectReason LANDiscovery::GetDisconnectReason() const {
    return disconnect_reason;
}

Result LANDiscovery::Scan(std::vector<NetworkInfo>& networks, u16& count,
                          const ScanFilter& filter) {
    if (!IsFlagSet(filter.flag, ScanFilterFlag::NetworkType) ||
        filter.network_type <= NetworkType::All) {
        if (!IsFlagSet(filter.flag, ScanFilterFlag::Ssid) && filter.ssid.length >= SsidLengthMax) {
            return ResultBadInput;
        }
    }

    {
        std::scoped_lock lock{packet_mutex};
        scan_results.clear();

        SendBroadcast(Network::LDNPacketType::Scan);
    }

    LOG_INFO(Service_LDN, "Waiting for scan replies");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::scoped_lock lock{packet_mutex};
    for (const auto& [key, info] : scan_results) {
        if (count >= networks.size()) {
            break;
        }

        if (IsFlagSet(filter.flag, ScanFilterFlag::LocalCommunicationId)) {
            if (filter.network_id.intent_id.local_communication_id !=
                info.network_id.intent_id.local_communication_id) {
                continue;
            }
        }
        if (IsFlagSet(filter.flag, ScanFilterFlag::SessionId)) {
            if (filter.network_id.session_id != info.network_id.session_id) {
                continue;
            }
        }
        if (IsFlagSet(filter.flag, ScanFilterFlag::NetworkType)) {
            if (filter.network_type != static_cast<NetworkType>(info.common.network_type)) {
                continue;
            }
        }
        if (IsFlagSet(filter.flag, ScanFilterFlag::Ssid)) {
            if (filter.ssid != info.common.ssid) {
                continue;
            }
        }
        if (IsFlagSet(filter.flag, ScanFilterFlag::SceneId)) {
            if (filter.network_id.intent_id.scene_id != info.network_id.intent_id.scene_id) {
                continue;
            }
        }

        networks[count++] = info;
    }

    return ResultSuccess;
}

Result LANDiscovery::SetAdvertiseData(std::span<const u8> data) {
    std::scoped_lock lock{packet_mutex};
    const std::size_t size = data.size();
    if (size > AdvertiseDataSizeMax) {
        return ResultAdvertiseDataTooLarge;
    }

    std::memcpy(network_info.ldn.advertise_data.data(), data.data(), size);
    network_info.ldn.advertise_data_size = static_cast<u16>(size);

    UpdateNodes();

    return ResultSuccess;
}

Result LANDiscovery::OpenAccessPoint() {
    std::scoped_lock lock{packet_mutex};
    disconnect_reason = DisconnectReason::None;
    if (state == State::None) {
        return ResultBadState;
    }

    ResetStations();
    SetState(State::AccessPointOpened);

    return ResultSuccess;
}

Result LANDiscovery::CloseAccessPoint() {
    std::scoped_lock lock{packet_mutex};
    if (state == State::None) {
        return ResultBadState;
    }

    if (state == State::AccessPointCreated) {
        DestroyNetwork();
    }

    ResetStations();
    SetState(State::Initialized);

    return ResultSuccess;
}

Result LANDiscovery::OpenStation() {
    std::scoped_lock lock{packet_mutex};
    disconnect_reason = DisconnectReason::None;
    if (state == State::None) {
        return ResultBadState;
    }

    ResetStations();
    SetState(State::StationOpened);

    return ResultSuccess;
}

Result LANDiscovery::CloseStation() {
    std::scoped_lock lock{packet_mutex};
    if (state == State::None) {
        return ResultBadState;
    }

    if (state == State::StationConnected) {
        Disconnect();
    }

    ResetStations();
    SetState(State::Initialized);

    return ResultSuccess;
}

Result LANDiscovery::CreateNetwork(const SecurityConfig& security_config,
                                   const UserConfig& user_config,
                                   const NetworkConfig& network_config) {
    std::scoped_lock lock{packet_mutex};

    if (state != State::AccessPointOpened) {
        return ResultBadState;
    }

    InitNetworkInfo();
    network_info.ldn.node_count_max = network_config.node_count_max;
    network_info.ldn.security_mode = security_config.security_mode;

    if (network_config.channel == WifiChannel::Default) {
        network_info.common.channel = WifiChannel::Wifi24_6;
    } else {
        network_info.common.channel = network_config.channel;
    }

    std::independent_bits_engine<std::mt19937, 64, u64> bits_engine;
    network_info.network_id.session_id.high = bits_engine();
    network_info.network_id.session_id.low = bits_engine();
    network_info.network_id.intent_id = network_config.intent_id;

    NodeInfo& node0 = network_info.ldn.nodes[0];
    const Result rc2 = GetNodeInfo(node0, user_config, network_config.local_communication_version);
    if (rc2.IsError()) {
        return ResultAccessPointConnectionFailed;
    }

    SetState(State::AccessPointCreated);

    InitNodeStateChange();
    node0.is_connected = 1;
    UpdateNodes();

    return rc2;
}

Result LANDiscovery::DestroyNetwork() {
    for (auto local_ip : connected_clients) {
        SendPacket(Network::LDNPacketType::DestroyNetwork, local_ip);
    }

    ResetStations();

    SetState(State::AccessPointOpened);
    lan_event();

    return ResultSuccess;
}

Result LANDiscovery::Connect(const NetworkInfo& network_info_, const UserConfig& user_config,
                             u16 local_communication_version) {
    std::scoped_lock lock{packet_mutex};
    if (network_info_.ldn.node_count == 0) {
        return ResultInvalidNodeCount;
    }

    Result rc = GetNodeInfo(node_info, user_config, local_communication_version);
    if (rc.IsError()) {
        return ResultConnectionFailed;
    }

    Ipv4Address node_host = network_info_.ldn.nodes[0].ipv4_address;
    std::reverse(std::begin(node_host), std::end(node_host)); // htonl
    host_ip = node_host;
    SendPacket(Network::LDNPacketType::Connect, node_info, *host_ip);

    InitNodeStateChange();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return ResultSuccess;
}

Result LANDiscovery::Disconnect() {
    if (host_ip) {
        SendPacket(Network::LDNPacketType::Disconnect, node_info, *host_ip);
    }

    SetState(State::StationOpened);
    lan_event();

    return ResultSuccess;
}

Result LANDiscovery::Initialize(LanEventFunc lan_event_, bool listening) {
    std::scoped_lock lock{packet_mutex};
    if (inited) {
        return ResultSuccess;
    }

    for (auto& station : stations) {
        station.discovery = this;
        station.node_info = &network_info.ldn.nodes[station.node_id];
        station.Reset();
    }

    connected_clients.clear();
    lan_event = lan_event_;

    SetState(State::Initialized);

    inited = true;
    return ResultSuccess;
}

Result LANDiscovery::Finalize() {
    std::scoped_lock lock{packet_mutex};
    Result rc = ResultSuccess;

    if (inited) {
        if (state == State::AccessPointCreated) {
            DestroyNetwork();
        }
        if (state == State::StationConnected) {
            Disconnect();
        }

        ResetStations();
        inited = false;
    }

    SetState(State::None);

    return rc;
}

void LANDiscovery::ResetStations() {
    for (auto& station : stations) {
        station.Reset();
    }
    connected_clients.clear();
}

void LANDiscovery::UpdateNodes() {
    u8 count = 0;
    for (auto& station : stations) {
        bool connected = station.GetStatus() == NodeStatus::Connected;
        if (connected) {
            count++;
        }
        station.OverrideInfo();
    }
    network_info.ldn.node_count = count + 1;

    for (auto local_ip : connected_clients) {
        SendPacket(Network::LDNPacketType::SyncNetwork, network_info, local_ip);
    }

    OnNetworkInfoChanged();
}

void LANDiscovery::OnSyncNetwork(const NetworkInfo& info) {
    network_info = info;
    if (state == State::StationOpened) {
        SetState(State::StationConnected);
    }
    OnNetworkInfoChanged();
}

void LANDiscovery::OnDisconnectFromHost() {
    LOG_INFO(Service_LDN, "OnDisconnectFromHost state: {}", static_cast<int>(state));
    host_ip = std::nullopt;
    if (state == State::StationConnected) {
        SetState(State::StationOpened);
        lan_event();
    }
}

void LANDiscovery::OnNetworkInfoChanged() {
    if (IsNodeStateChanged()) {
        lan_event();
    }
    return;
}

Network::IPv4Address LANDiscovery::GetLocalIp() const {
    Network::IPv4Address local_ip{0xFF, 0xFF, 0xFF, 0xFF};
    if (auto room_member = room_network.GetRoomMember().lock()) {
        if (room_member->IsConnected()) {
            local_ip = room_member->GetFakeIpAddress();
        }
    }
    return local_ip;
}

template <typename Data>
void LANDiscovery::SendPacket(Network::LDNPacketType type, const Data& data,
                              Ipv4Address remote_ip) {
    Network::LDNPacket packet;
    packet.type = type;

    packet.broadcast = false;
    packet.local_ip = GetLocalIp();
    packet.remote_ip = remote_ip;

    packet.data.resize(sizeof(data));
    std::memcpy(packet.data.data(), &data, sizeof(data));
    SendPacket(packet);
}

void LANDiscovery::SendPacket(Network::LDNPacketType type, Ipv4Address remote_ip) {
    Network::LDNPacket packet;
    packet.type = type;

    packet.broadcast = false;
    packet.local_ip = GetLocalIp();
    packet.remote_ip = remote_ip;

    SendPacket(packet);
}

template <typename Data>
void LANDiscovery::SendBroadcast(Network::LDNPacketType type, const Data& data) {
    Network::LDNPacket packet;
    packet.type = type;

    packet.broadcast = true;
    packet.local_ip = GetLocalIp();

    packet.data.resize(sizeof(data));
    std::memcpy(packet.data.data(), &data, sizeof(data));
    SendPacket(packet);
}

void LANDiscovery::SendBroadcast(Network::LDNPacketType type) {
    Network::LDNPacket packet;
    packet.type = type;

    packet.broadcast = true;
    packet.local_ip = GetLocalIp();

    SendPacket(packet);
}

void LANDiscovery::SendPacket(const Network::LDNPacket& packet) {
    if (auto room_member = room_network.GetRoomMember().lock()) {
        if (room_member->IsConnected()) {
            room_member->SendLdnPacket(packet);
        }
    }
}

void LANDiscovery::ReceivePacket(const Network::LDNPacket& packet) {
    std::scoped_lock lock{packet_mutex};
    switch (packet.type) {
    case Network::LDNPacketType::Scan: {
        LOG_DEBUG(Frontend, "Scan packet received!");
        if (state == State::AccessPointCreated) {
            // Reply to the sender
            SendPacket(Network::LDNPacketType::ScanResp, network_info, packet.local_ip);
        }
        break;
    }
    case Network::LDNPacketType::ScanResp: {
        LOG_DEBUG(Frontend, "ScanResp packet received!");

        NetworkInfo info{};
        std::memcpy(&info, packet.data.data(), sizeof(NetworkInfo));
        scan_results.insert({info.common.bssid, info});

        break;
    }
    case Network::LDNPacketType::Connect: {
        LOG_INFO(Frontend, "Connect packet received!");

        NodeInfo info{};
        std::memcpy(&info, packet.data.data(), sizeof(NodeInfo));

        connected_clients.push_back(packet.local_ip);

        for (LanStation& station : stations) {
            if (station.status != NodeStatus::Connected) {
                *station.node_info = info;
                station.status = NodeStatus::Connected;
                break;
            }
        }

        UpdateNodes();

        break;
    }
    case Network::LDNPacketType::Disconnect: {
        LOG_INFO(Frontend, "Disconnect packet received!");

        connected_clients.erase(
            std::remove(connected_clients.begin(), connected_clients.end(), packet.local_ip),
            connected_clients.end());

        NodeInfo info{};
        std::memcpy(&info, packet.data.data(), sizeof(NodeInfo));

        for (LanStation& station : stations) {
            if (station.status == NodeStatus::Connected &&
                station.node_info->mac_address == info.mac_address) {
                station.OnClose();
                break;
            }
        }

        break;
    }
    case Network::LDNPacketType::DestroyNetwork: {
        ResetStations();
        OnDisconnectFromHost();
        break;
    }
    case Network::LDNPacketType::SyncNetwork: {
        if (state == State::StationOpened || state == State::StationConnected) {
            LOG_INFO(Frontend, "SyncNetwork packet received!");
            NetworkInfo info{};
            std::memcpy(&info, packet.data.data(), sizeof(NetworkInfo));

            OnSyncNetwork(info);
        } else {
            LOG_INFO(Frontend, "SyncNetwork packet received but in wrong State!");
        }

        break;
    }
    default: {
        LOG_INFO(Frontend, "ReceivePacket unhandled type {}", static_cast<int>(packet.type));
        break;
    }
    }
}

bool LANDiscovery::IsNodeStateChanged() {
    bool changed = false;
    const auto& nodes = network_info.ldn.nodes;
    for (int i = 0; i < NodeCountMax; i++) {
        if (nodes[i].is_connected != node_last_states[i]) {
            if (nodes[i].is_connected) {
                node_changes[i].state_change |= NodeStateChange::Connect;
            } else {
                node_changes[i].state_change |= NodeStateChange::Disconnect;
            }
            node_last_states[i] = nodes[i].is_connected;
            changed = true;
        }
    }
    return changed;
}

bool LANDiscovery::IsFlagSet(ScanFilterFlag flag, ScanFilterFlag search_flag) const {
    const auto flag_value = static_cast<u32>(flag);
    const auto search_flag_value = static_cast<u32>(search_flag);
    return (flag_value & search_flag_value) == search_flag_value;
}

int LANDiscovery::GetStationCount() const {
    return static_cast<int>(
        std::count_if(stations.begin(), stations.end(), [](const auto& station) {
            return station.GetStatus() != NodeStatus::Disconnected;
        }));
}

MacAddress LANDiscovery::GetFakeMac() const {
    MacAddress mac{};
    mac.raw[0] = 0x02;
    mac.raw[1] = 0x00;

    const auto ip = GetLocalIp();
    memcpy(mac.raw.data() + 2, &ip, sizeof(ip));

    return mac;
}

Result LANDiscovery::GetNodeInfo(NodeInfo& node, const UserConfig& userConfig,
                                 u16 localCommunicationVersion) {
    node.mac_address = GetFakeMac();
    node.is_connected = 1;
    std::memcpy(node.user_name.data(), userConfig.user_name.data(), UserNameBytesMax + 1);
    node.local_communication_version = localCommunicationVersion;

    Ipv4Address current_address = GetLocalIp();
    std::reverse(std::begin(current_address), std::end(current_address)); // ntohl
    node.ipv4_address = current_address;

    return ResultSuccess;
}

} // namespace Service::LDN
