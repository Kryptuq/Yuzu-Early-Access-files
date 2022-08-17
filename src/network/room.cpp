// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <mutex>
#include <random>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include "common/logging/log.h"
#include "enet/enet.h"
#include "network/packet.h"
#include "network/room.h"
#include "network/verify_user.h"

namespace Network {

class Room::RoomImpl {
public:
    std::mt19937 random_gen; ///< Random number generator. Used for GenerateFakeIPAddress

    ENetHost* server = nullptr; ///< Network interface.

    std::atomic<State> state{State::Closed}; ///< Current state of the room.
    RoomInformation room_information;        ///< Information about this room.

    std::string verify_uid;              ///< A GUID which may be used for verfication.
    mutable std::mutex verify_uid_mutex; ///< Mutex for verify_uid

    std::string password; ///< The password required to connect to this room.

    struct Member {
        std::string nickname; ///< The nickname of the member.
        GameInfo game_info;   ///< The current game of the member
        IPv4Address fake_ip;  ///< The assigned fake ip address of the member.
        /// Data of the user, often including authenticated forum username.
        VerifyUser::UserData user_data;
        ENetPeer* peer; ///< The remote peer.
    };
    using MemberList = std::vector<Member>;
    MemberList members;                     ///< Information about the members of this room
    mutable std::shared_mutex member_mutex; ///< Mutex for locking the members list

    UsernameBanList username_ban_list; ///< List of banned usernames
    IPBanList ip_ban_list;             ///< List of banned IP addresses
    mutable std::mutex ban_list_mutex; ///< Mutex for the ban lists

    RoomImpl() : random_gen(std::random_device()()) {}

    /// Thread that receives and dispatches network packets
    std::unique_ptr<std::thread> room_thread;

    /// Verification backend of the room
    std::unique_ptr<VerifyUser::Backend> verify_backend;

    /// Thread function that will receive and dispatch messages until the room is destroyed.
    void ServerLoop();
    void StartLoop();

    /**
     * Parses and answers a room join request from a client.
     * Validates the uniqueness of the username and assigns the MAC address
     * that the client will use for the remainder of the connection.
     */
    void HandleJoinRequest(const ENetEvent* event);

    /**
     * Parses and answers a kick request from a client.
     * Validates the permissions and that the given user exists and then kicks the member.
     */
    void HandleModKickPacket(const ENetEvent* event);

    /**
     * Parses and answers a ban request from a client.
     * Validates the permissions and bans the user (by forum username or IP).
     */
    void HandleModBanPacket(const ENetEvent* event);

    /**
     * Parses and answers a unban request from a client.
     * Validates the permissions and unbans the address.
     */
    void HandleModUnbanPacket(const ENetEvent* event);

    /**
     * Parses and answers a get ban list request from a client.
     * Validates the permissions and returns the ban list.
     */
    void HandleModGetBanListPacket(const ENetEvent* event);

    /**
     * Returns whether the nickname is valid, ie. isn't already taken by someone else in the room.
     */
    bool IsValidNickname(const std::string& nickname) const;

    /**
     * Returns whether the fake ip address is valid, ie. isn't already taken by someone else in the
     * room.
     */
    bool IsValidFakeIPAddress(const IPv4Address& address) const;

    /**
     * Returns whether a user has mod permissions.
     */
    bool HasModPermission(const ENetPeer* client) const;

    /**
     * Sends a ID_ROOM_IS_FULL message telling the client that the room is full.
     */
    void SendRoomIsFull(ENetPeer* client);

    /**
     * Sends a ID_ROOM_NAME_COLLISION message telling the client that the name is invalid.
     */
    void SendNameCollision(ENetPeer* client);

    /**
     * Sends a ID_ROOM_IP_COLLISION message telling the client that the IP is invalid.
     */
    void SendIPCollision(ENetPeer* client);

    /**
     * Sends a ID_ROOM_VERSION_MISMATCH message telling the client that the version is invalid.
     */
    void SendVersionMismatch(ENetPeer* client);

    /**
     * Sends a ID_ROOM_WRONG_PASSWORD message telling the client that the password is wrong.
     */
    void SendWrongPassword(ENetPeer* client);

    /**
     * Notifies the member that its connection attempt was successful,
     * and it is now part of the room.
     */
    void SendJoinSuccess(ENetPeer* client, IPv4Address fake_ip);

    /**
     * Notifies the member that its connection attempt was successful,
     * and it is now part of the room, and it has been granted mod permissions.
     */
    void SendJoinSuccessAsMod(ENetPeer* client, IPv4Address fake_ip);

    /**
     * Sends a IdHostKicked message telling the client that they have been kicked.
     */
    void SendUserKicked(ENetPeer* client);

    /**
     * Sends a IdHostBanned message telling the client that they have been banned.
     */
    void SendUserBanned(ENetPeer* client);

    /**
     * Sends a IdModPermissionDenied message telling the client that they do not have mod
     * permission.
     */
    void SendModPermissionDenied(ENetPeer* client);

    /**
     * Sends a IdModNoSuchUser message telling the client that the given user could not be found.
     */
    void SendModNoSuchUser(ENetPeer* client);

    /**
     * Sends the ban list in response to a client's request for getting ban list.
     */
    void SendModBanListResponse(ENetPeer* client);

    /**
     * Notifies the members that the room is closed,
     */
    void SendCloseMessage();

    /**
     * Sends a system message to all the connected clients.
     */
    void SendStatusMessage(StatusMessageTypes type, const std::string& nickname,
                           const std::string& username, const std::string& ip);

    /**
     * Sends the information about the room, along with the list of members
     * to every connected client in the room.
     * The packet has the structure:
     * <MessageID>ID_ROOM_INFORMATION
     * <String> room_name
     * <String> room_description
     * <u32> member_slots: The max number of clients allowed in this room
     * <String> uid
     * <u16> port
     * <u32> num_members: the number of currently joined clients
     * This is followed by the following three values for each member:
     * <String> nickname of that member
     * <IPv4Address> fake_ip of that member
     * <String> game_name of that member
     */
    void BroadcastRoomInformation();

    /**
     * Generates a free MAC address to assign to a new client.
     * The first 3 bytes are the NintendoOUI 0x00, 0x1F, 0x32
     */
    IPv4Address GenerateFakeIPAddress();

    /**
     * Broadcasts this packet to all members except the sender.
     * @param event The ENet event containing the data
     */
    void HandleProxyPacket(const ENetEvent* event);

    /**
     * Extracts a chat entry from a received ENet packet and adds it to the chat queue.
     * @param event The ENet event that was received.
     */
    void HandleChatPacket(const ENetEvent* event);

    /**
     * Extracts the game name from a received ENet packet and broadcasts it.
     * @param event The ENet event that was received.
     */
    void HandleGameNamePacket(const ENetEvent* event);

    /**
     * Removes the client from the members list if it was in it and announces the change
     * to all other clients.
     */
    void HandleClientDisconnection(ENetPeer* client);
};

// RoomImpl
void Room::RoomImpl::ServerLoop() {
    while (state != State::Closed) {
        ENetEvent event;
        if (enet_host_service(server, &event, 50) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                switch (event.packet->data[0]) {
                case IdJoinRequest:
                    HandleJoinRequest(&event);
                    break;
                case IdSetGameInfo:
                    HandleGameNamePacket(&event);
                    break;
                case IdProxyPacket:
                    HandleProxyPacket(&event);
                    break;
                case IdChatMessage:
                    HandleChatPacket(&event);
                    break;
                // Moderation
                case IdModKick:
                    HandleModKickPacket(&event);
                    break;
                case IdModBan:
                    HandleModBanPacket(&event);
                    break;
                case IdModUnban:
                    HandleModUnbanPacket(&event);
                    break;
                case IdModGetBanList:
                    HandleModGetBanListPacket(&event);
                    break;
                }
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                HandleClientDisconnection(event.peer);
                break;
            case ENET_EVENT_TYPE_NONE:
            case ENET_EVENT_TYPE_CONNECT:
                break;
            }
        }
    }
    // Close the connection to all members:
    SendCloseMessage();
}

void Room::RoomImpl::StartLoop() {
    room_thread = std::make_unique<std::thread>(&Room::RoomImpl::ServerLoop, this);
}

void Room::RoomImpl::HandleJoinRequest(const ENetEvent* event) {
    {
        std::lock_guard lock(member_mutex);
        if (members.size() >= room_information.member_slots) {
            SendRoomIsFull(event->peer);
            return;
        }
    }
    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);
    packet.IgnoreBytes(sizeof(u8)); // Ignore the message type
    std::string nickname;
    packet.Read(nickname);

    IPv4Address preferred_fake_ip;
    packet.Read(preferred_fake_ip);

    u32 client_version;
    packet.Read(client_version);

    std::string pass;
    packet.Read(pass);

    std::string token;
    packet.Read(token);

    if (pass != password) {
        SendWrongPassword(event->peer);
        return;
    }

    if (!IsValidNickname(nickname)) {
        SendNameCollision(event->peer);
        return;
    }

    if (preferred_fake_ip != NoPreferredIP) {
        // Verify if the preferred fake ip is available
        if (!IsValidFakeIPAddress(preferred_fake_ip)) {
            SendIPCollision(event->peer);
            return;
        }
    } else {
        // Assign a fake ip address of this client automatically
        preferred_fake_ip = GenerateFakeIPAddress();
    }

    if (client_version != network_version) {
        SendVersionMismatch(event->peer);
        return;
    }

    // At this point the client is ready to be added to the room.
    Member member{};
    member.fake_ip = preferred_fake_ip;
    member.nickname = nickname;
    member.peer = event->peer;

    std::string uid;
    {
        std::lock_guard lock(verify_uid_mutex);
        uid = verify_uid;
    }
    member.user_data = verify_backend->LoadUserData(uid, token);

    std::string ip;
    {
        std::lock_guard lock(ban_list_mutex);

        // Check username ban
        if (!member.user_data.username.empty() &&
            std::find(username_ban_list.begin(), username_ban_list.end(),
                      member.user_data.username) != username_ban_list.end()) {

            SendUserBanned(event->peer);
            return;
        }

        // Check IP ban
        std::array<char, 256> ip_raw{};
        enet_address_get_host_ip(&event->peer->address, ip_raw.data(), sizeof(ip_raw) - 1);
        ip = ip_raw.data();

        if (std::find(ip_ban_list.begin(), ip_ban_list.end(), ip) != ip_ban_list.end()) {
            SendUserBanned(event->peer);
            return;
        }
    }

    // Notify everyone that the user has joined.
    SendStatusMessage(IdMemberJoin, member.nickname, member.user_data.username, ip);

    {
        std::lock_guard lock(member_mutex);
        members.push_back(std::move(member));
    }

    // Notify everyone that the room information has changed.
    BroadcastRoomInformation();
    if (HasModPermission(event->peer)) {
        SendJoinSuccessAsMod(event->peer, preferred_fake_ip);
    } else {
        SendJoinSuccess(event->peer, preferred_fake_ip);
    }
}

void Room::RoomImpl::HandleModKickPacket(const ENetEvent* event) {
    if (!HasModPermission(event->peer)) {
        SendModPermissionDenied(event->peer);
        return;
    }

    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);
    packet.IgnoreBytes(sizeof(u8)); // Ignore the message type

    std::string nickname;
    packet.Read(nickname);

    std::string username, ip;
    {
        std::lock_guard lock(member_mutex);
        const auto target_member =
            std::find_if(members.begin(), members.end(),
                         [&nickname](const auto& member) { return member.nickname == nickname; });
        if (target_member == members.end()) {
            SendModNoSuchUser(event->peer);
            return;
        }

        // Notify the kicked member
        SendUserKicked(target_member->peer);

        username = target_member->user_data.username;

        std::array<char, 256> ip_raw{};
        enet_address_get_host_ip(&target_member->peer->address, ip_raw.data(), sizeof(ip_raw) - 1);
        ip = ip_raw.data();

        enet_peer_disconnect(target_member->peer, 0);
        members.erase(target_member);
    }

    // Announce the change to all clients.
    SendStatusMessage(IdMemberKicked, nickname, username, ip);
    BroadcastRoomInformation();
}

void Room::RoomImpl::HandleModBanPacket(const ENetEvent* event) {
    if (!HasModPermission(event->peer)) {
        SendModPermissionDenied(event->peer);
        return;
    }

    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);
    packet.IgnoreBytes(sizeof(u8)); // Ignore the message type

    std::string nickname;
    packet.Read(nickname);

    std::string username, ip;
    {
        std::lock_guard lock(member_mutex);
        const auto target_member =
            std::find_if(members.begin(), members.end(),
                         [&nickname](const auto& member) { return member.nickname == nickname; });
        if (target_member == members.end()) {
            SendModNoSuchUser(event->peer);
            return;
        }

        // Notify the banned member
        SendUserBanned(target_member->peer);

        nickname = target_member->nickname;
        username = target_member->user_data.username;

        std::array<char, 256> ip_raw{};
        enet_address_get_host_ip(&target_member->peer->address, ip_raw.data(), sizeof(ip_raw) - 1);
        ip = ip_raw.data();

        enet_peer_disconnect(target_member->peer, 0);
        members.erase(target_member);
    }

    {
        std::lock_guard lock(ban_list_mutex);

        if (!username.empty()) {
            // Ban the forum username
            if (std::find(username_ban_list.begin(), username_ban_list.end(), username) ==
                username_ban_list.end()) {

                username_ban_list.emplace_back(username);
            }
        }

        // Ban the member's IP as well
        if (std::find(ip_ban_list.begin(), ip_ban_list.end(), ip) == ip_ban_list.end()) {
            ip_ban_list.emplace_back(ip);
        }
    }

    // Announce the change to all clients.
    SendStatusMessage(IdMemberBanned, nickname, username, ip);
    BroadcastRoomInformation();
}

void Room::RoomImpl::HandleModUnbanPacket(const ENetEvent* event) {
    if (!HasModPermission(event->peer)) {
        SendModPermissionDenied(event->peer);
        return;
    }

    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);
    packet.IgnoreBytes(sizeof(u8)); // Ignore the message type

    std::string address;
    packet.Read(address);

    bool unbanned = false;
    {
        std::lock_guard lock(ban_list_mutex);

        auto it = std::find(username_ban_list.begin(), username_ban_list.end(), address);
        if (it != username_ban_list.end()) {
            unbanned = true;
            username_ban_list.erase(it);
        }

        it = std::find(ip_ban_list.begin(), ip_ban_list.end(), address);
        if (it != ip_ban_list.end()) {
            unbanned = true;
            ip_ban_list.erase(it);
        }
    }

    if (unbanned) {
        SendStatusMessage(IdAddressUnbanned, address, "", "");
    } else {
        SendModNoSuchUser(event->peer);
    }
}

void Room::RoomImpl::HandleModGetBanListPacket(const ENetEvent* event) {
    if (!HasModPermission(event->peer)) {
        SendModPermissionDenied(event->peer);
        return;
    }

    SendModBanListResponse(event->peer);
}

bool Room::RoomImpl::IsValidNickname(const std::string& nickname) const {
    // A nickname is valid if it matches the regex and is not already taken by anybody else in the
    // room.
    const std::regex nickname_regex("^[ a-zA-Z0-9._-]{4,20}$");
    if (!std::regex_match(nickname, nickname_regex))
        return false;

    std::lock_guard lock(member_mutex);
    return std::all_of(members.begin(), members.end(),
                       [&nickname](const auto& member) { return member.nickname != nickname; });
}

bool Room::RoomImpl::IsValidFakeIPAddress(const IPv4Address& address) const {
    // An IP address is valid if it is not already taken by anybody else in the room.
    std::lock_guard lock(member_mutex);
    return std::all_of(members.begin(), members.end(),
                       [&address](const auto& member) { return member.fake_ip != address; });
}

bool Room::RoomImpl::HasModPermission(const ENetPeer* client) const {
    std::lock_guard lock(member_mutex);
    const auto sending_member =
        std::find_if(members.begin(), members.end(),
                     [client](const auto& member) { return member.peer == client; });
    if (sending_member == members.end()) {
        return false;
    }
    if (room_information.enable_yuzu_mods &&
        sending_member->user_data.moderator) { // Community moderator

        return true;
    }
    if (!room_information.host_username.empty() &&
        sending_member->user_data.username == room_information.host_username) { // Room host

        return true;
    }
    return false;
}

void Room::RoomImpl::SendNameCollision(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdNameCollision));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendIPCollision(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdIpCollision));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendWrongPassword(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdWrongPassword));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendRoomIsFull(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdRoomIsFull));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendVersionMismatch(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdVersionMismatch));
    packet.Write(network_version);

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendJoinSuccess(ENetPeer* client, IPv4Address fake_ip) {
    Packet packet;
    packet.Write(static_cast<u8>(IdJoinSuccess));
    packet.Write(fake_ip);
    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendJoinSuccessAsMod(ENetPeer* client, IPv4Address fake_ip) {
    Packet packet;
    packet.Write(static_cast<u8>(IdJoinSuccessAsMod));
    packet.Write(fake_ip);
    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendUserKicked(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdHostKicked));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendUserBanned(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdHostBanned));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendModPermissionDenied(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdModPermissionDenied));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendModNoSuchUser(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdModNoSuchUser));

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendModBanListResponse(ENetPeer* client) {
    Packet packet;
    packet.Write(static_cast<u8>(IdModBanListResponse));
    {
        std::lock_guard lock(ban_list_mutex);
        packet.Write(username_ban_list);
        packet.Write(ip_ban_list);
    }

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::RoomImpl::SendCloseMessage() {
    Packet packet;
    packet.Write(static_cast<u8>(IdCloseRoom));
    std::lock_guard lock(member_mutex);
    if (!members.empty()) {
        ENetPacket* enet_packet =
            enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
        for (auto& member : members) {
            enet_peer_send(member.peer, 0, enet_packet);
        }
    }
    enet_host_flush(server);
    for (auto& member : members) {
        enet_peer_disconnect(member.peer, 0);
    }
}

void Room::RoomImpl::SendStatusMessage(StatusMessageTypes type, const std::string& nickname,
                                       const std::string& username, const std::string& ip) {
    Packet packet;
    packet.Write(static_cast<u8>(IdStatusMessage));
    packet.Write(static_cast<u8>(type));
    packet.Write(nickname);
    packet.Write(username);
    std::lock_guard lock(member_mutex);
    if (!members.empty()) {
        ENetPacket* enet_packet =
            enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
        for (auto& member : members) {
            enet_peer_send(member.peer, 0, enet_packet);
        }
    }
    enet_host_flush(server);

    const std::string display_name =
        username.empty() ? nickname : fmt::format("{} ({})", nickname, username);

    switch (type) {
    case IdMemberJoin:
        LOG_INFO(Network, "[{}] {} has joined.", ip, display_name);
        break;
    case IdMemberLeave:
        LOG_INFO(Network, "[{}] {} has left.", ip, display_name);
        break;
    case IdMemberKicked:
        LOG_INFO(Network, "[{}] {} has been kicked.", ip, display_name);
        break;
    case IdMemberBanned:
        LOG_INFO(Network, "[{}] {} has been banned.", ip, display_name);
        break;
    case IdAddressUnbanned:
        LOG_INFO(Network, "{} has been unbanned.", display_name);
        break;
    }
}

void Room::RoomImpl::BroadcastRoomInformation() {
    Packet packet;
    packet.Write(static_cast<u8>(IdRoomInformation));
    packet.Write(room_information.name);
    packet.Write(room_information.description);
    packet.Write(room_information.member_slots);
    packet.Write(room_information.port);
    packet.Write(room_information.preferred_game.name);
    packet.Write(room_information.host_username);

    packet.Write(static_cast<u32>(members.size()));
    {
        std::lock_guard lock(member_mutex);
        for (const auto& member : members) {
            packet.Write(member.nickname);
            packet.Write(member.fake_ip);
            packet.Write(member.game_info.name);
            packet.Write(member.game_info.id);
            packet.Write(member.user_data.username);
            packet.Write(member.user_data.display_name);
            packet.Write(member.user_data.avatar_url);
        }
    }

    ENetPacket* enet_packet =
        enet_packet_create(packet.GetData(), packet.GetDataSize(), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, enet_packet);
    enet_host_flush(server);
}

IPv4Address Room::RoomImpl::GenerateFakeIPAddress() {
    IPv4Address result_ip{192, 168, 0, 0};
    std::uniform_int_distribution<> dis(0x01, 0xFE); // Random byte between 1 and 0xFE
    do {
        for (std::size_t i = 2; i < result_ip.size(); ++i) {
            result_ip[i] = dis(random_gen);
        }
    } while (!IsValidFakeIPAddress(result_ip));

    return result_ip;
}

void Room::RoomImpl::HandleProxyPacket(const ENetEvent* event) {
    Packet in_packet;
    in_packet.Append(event->packet->data, event->packet->dataLength);
    in_packet.IgnoreBytes(sizeof(u8)); // Message type

    in_packet.IgnoreBytes(sizeof(u8));          // Domain
    in_packet.IgnoreBytes(sizeof(IPv4Address)); // IP
    in_packet.IgnoreBytes(sizeof(u16));         // Port

    in_packet.IgnoreBytes(sizeof(u8)); // Domain
    IPv4Address remote_ip;
    in_packet.Read(remote_ip);          // IP
    in_packet.IgnoreBytes(sizeof(u16)); // Port

    in_packet.IgnoreBytes(sizeof(u8)); // Protocol
    bool broadcast;
    in_packet.Read(broadcast); // Broadcast

    Packet out_packet;
    out_packet.Append(event->packet->data, event->packet->dataLength);
    ENetPacket* enet_packet = enet_packet_create(out_packet.GetData(), out_packet.GetDataSize(),
                                                 ENET_PACKET_FLAG_RELIABLE);

    const auto& destination_address = remote_ip;
    if (broadcast) { // Send the data to everyone except the sender
        std::lock_guard lock(member_mutex);
        bool sent_packet = false;
        for (const auto& member : members) {
            if (member.peer != event->peer) {
                sent_packet = true;
                enet_peer_send(member.peer, 0, enet_packet);
            }
        }

        if (!sent_packet) {
            enet_packet_destroy(enet_packet);
        }
    } else { // Send the data only to the destination client
        std::lock_guard lock(member_mutex);
        auto member = std::find_if(members.begin(), members.end(),
                                   [destination_address](const Member& member_entry) -> bool {
                                       return member_entry.fake_ip == destination_address;
                                   });
        if (member != members.end()) {
            enet_peer_send(member->peer, 0, enet_packet);
        } else {
            LOG_ERROR(Network,
                      "Attempting to send to unknown IP address: "
                      "{}.{}.{}.{}",
                      destination_address[0], destination_address[1], destination_address[2],
                      destination_address[3]);
            enet_packet_destroy(enet_packet);
        }
    }
    enet_host_flush(server);
}

void Room::RoomImpl::HandleChatPacket(const ENetEvent* event) {
    Packet in_packet;
    in_packet.Append(event->packet->data, event->packet->dataLength);

    in_packet.IgnoreBytes(sizeof(u8)); // Ignore the message type
    std::string message;
    in_packet.Read(message);
    auto CompareNetworkAddress = [event](const Member member) -> bool {
        return member.peer == event->peer;
    };

    std::lock_guard lock(member_mutex);
    const auto sending_member = std::find_if(members.begin(), members.end(), CompareNetworkAddress);
    if (sending_member == members.end()) {
        return; // Received a chat message from a unknown sender
    }

    // Limit the size of chat messages to MaxMessageSize
    message.resize(std::min(static_cast<u32>(message.size()), MaxMessageSize));

    Packet out_packet;
    out_packet.Write(static_cast<u8>(IdChatMessage));
    out_packet.Write(sending_member->nickname);
    out_packet.Write(sending_member->user_data.username);
    out_packet.Write(message);

    ENetPacket* enet_packet = enet_packet_create(out_packet.GetData(), out_packet.GetDataSize(),
                                                 ENET_PACKET_FLAG_RELIABLE);
    bool sent_packet = false;
    for (const auto& member : members) {
        if (member.peer != event->peer) {
            sent_packet = true;
            enet_peer_send(member.peer, 0, enet_packet);
        }
    }

    if (!sent_packet) {
        enet_packet_destroy(enet_packet);
    }

    enet_host_flush(server);

    if (sending_member->user_data.username.empty()) {
        LOG_INFO(Network, "{}: {}", sending_member->nickname, message);
    } else {
        LOG_INFO(Network, "{} ({}): {}", sending_member->nickname,
                 sending_member->user_data.username, message);
    }
}

void Room::RoomImpl::HandleGameNamePacket(const ENetEvent* event) {
    Packet in_packet;
    in_packet.Append(event->packet->data, event->packet->dataLength);

    in_packet.IgnoreBytes(sizeof(u8)); // Ignore the message type
    GameInfo game_info;
    in_packet.Read(game_info.name);
    in_packet.Read(game_info.id);

    {
        std::lock_guard lock(member_mutex);
        auto member = std::find_if(members.begin(), members.end(),
                                   [event](const Member& member_entry) -> bool {
                                       return member_entry.peer == event->peer;
                                   });
        if (member != members.end()) {
            member->game_info = game_info;

            const std::string display_name =
                member->user_data.username.empty()
                    ? member->nickname
                    : fmt::format("{} ({})", member->nickname, member->user_data.username);

            if (game_info.name.empty()) {
                LOG_INFO(Network, "{} is not playing", display_name);
            } else {
                LOG_INFO(Network, "{} is playing {}", display_name, game_info.name);
            }
        }
    }
    BroadcastRoomInformation();
}

void Room::RoomImpl::HandleClientDisconnection(ENetPeer* client) {
    // Remove the client from the members list.
    std::string nickname, username, ip;
    {
        std::lock_guard lock(member_mutex);
        auto member =
            std::find_if(members.begin(), members.end(), [client](const Member& member_entry) {
                return member_entry.peer == client;
            });
        if (member != members.end()) {
            nickname = member->nickname;
            username = member->user_data.username;

            std::array<char, 256> ip_raw{};
            enet_address_get_host_ip(&member->peer->address, ip_raw.data(), sizeof(ip_raw) - 1);
            ip = ip_raw.data();

            members.erase(member);
        }
    }

    // Announce the change to all clients.
    enet_peer_disconnect(client, 0);
    if (!nickname.empty())
        SendStatusMessage(IdMemberLeave, nickname, username, ip);
    BroadcastRoomInformation();
}

// Room
Room::Room() : room_impl{std::make_unique<RoomImpl>()} {}

Room::~Room() = default;

bool Room::Create(const std::string& name, const std::string& description,
                  const std::string& server_address, u16 server_port, const std::string& password,
                  const u32 max_connections, const std::string& host_username,
                  const GameInfo preferred_game,
                  std::unique_ptr<VerifyUser::Backend> verify_backend,
                  const Room::BanList& ban_list, bool enable_yuzu_mods) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    if (!server_address.empty()) {
        enet_address_set_host(&address, server_address.c_str());
    }
    address.port = server_port;

    // In order to send the room is full message to the connecting client, we need to leave one
    // slot open so enet won't reject the incoming connection without telling us
    room_impl->server = enet_host_create(&address, max_connections + 1, NumChannels, 0, 0);
    if (!room_impl->server) {
        return false;
    }
    room_impl->state = State::Open;

    room_impl->room_information.name = name;
    room_impl->room_information.description = description;
    room_impl->room_information.member_slots = max_connections;
    room_impl->room_information.port = server_port;
    room_impl->room_information.preferred_game = preferred_game;
    room_impl->room_information.host_username = host_username;
    room_impl->room_information.enable_yuzu_mods = enable_yuzu_mods;
    room_impl->password = password;
    room_impl->verify_backend = std::move(verify_backend);
    room_impl->username_ban_list = ban_list.first;
    room_impl->ip_ban_list = ban_list.second;

    room_impl->StartLoop();
    return true;
}

Room::State Room::GetState() const {
    return room_impl->state;
}

const RoomInformation& Room::GetRoomInformation() const {
    return room_impl->room_information;
}

std::string Room::GetVerifyUID() const {
    std::lock_guard lock(room_impl->verify_uid_mutex);
    return room_impl->verify_uid;
}

Room::BanList Room::GetBanList() const {
    std::lock_guard lock(room_impl->ban_list_mutex);
    return {room_impl->username_ban_list, room_impl->ip_ban_list};
}

std::vector<Member> Room::GetRoomMemberList() const {
    std::vector<Member> member_list;
    std::lock_guard lock(room_impl->member_mutex);
    for (const auto& member_impl : room_impl->members) {
        Member member;
        member.nickname = member_impl.nickname;
        member.username = member_impl.user_data.username;
        member.display_name = member_impl.user_data.display_name;
        member.avatar_url = member_impl.user_data.avatar_url;
        member.fake_ip = member_impl.fake_ip;
        member.game = member_impl.game_info;
        member_list.push_back(member);
    }
    return member_list;
}

bool Room::HasPassword() const {
    return !room_impl->password.empty();
}

void Room::SetVerifyUID(const std::string& uid) {
    std::lock_guard lock(room_impl->verify_uid_mutex);
    room_impl->verify_uid = uid;
}

void Room::Destroy() {
    room_impl->state = State::Closed;
    room_impl->room_thread->join();
    room_impl->room_thread.reset();

    if (room_impl->server) {
        enet_host_destroy(room_impl->server);
    }
    room_impl->room_information = {};
    room_impl->server = nullptr;
    {
        std::lock_guard lock(room_impl->member_mutex);
        room_impl->members.clear();
    }
    room_impl->room_information.member_slots = 0;
    room_impl->room_information.name.clear();
}

} // namespace Network
