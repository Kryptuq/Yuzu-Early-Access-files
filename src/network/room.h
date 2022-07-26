// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include "common/announce_multiplayer_room.h"
#include "common/common_types.h"
#include "network/verify_user.h"

namespace Network {

using AnnounceMultiplayerRoom::GameInfo;
using AnnounceMultiplayerRoom::MacAddress;
using AnnounceMultiplayerRoom::Member;
using AnnounceMultiplayerRoom::RoomInformation;

constexpr u32 network_version = 1; ///< The version of this Room and RoomMember

constexpr u16 DefaultRoomPort = 24872;

constexpr u32 MaxMessageSize = 500;

/// Maximum number of concurrent connections allowed to this room.
static constexpr u32 MaxConcurrentConnections = 254;

constexpr std::size_t NumChannels = 1; // Number of channels used for the connection

/// A special MAC address that tells the room we're joining to assign us a MAC address
/// automatically.
constexpr MacAddress NoPreferredMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// 802.11 broadcast MAC address
constexpr MacAddress BroadcastMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// The different types of messages that can be sent. The first byte of each packet defines the type
enum RoomMessageTypes : u8 {
    IdJoinRequest = 1,
    IdJoinSuccess,
    IdRoomInformation,
    IdSetGameInfo,
    IdWifiPacket,
    IdChatMessage,
    IdNameCollision,
    IdMacCollision,
    IdVersionMismatch,
    IdWrongPassword,
    IdCloseRoom,
    IdRoomIsFull,
    IdConsoleIdCollision,
    IdStatusMessage,
    IdHostKicked,
    IdHostBanned,
    /// Moderation requests
    IdModKick,
    IdModBan,
    IdModUnban,
    IdModGetBanList,
    // Moderation responses
    IdModBanListResponse,
    IdModPermissionDenied,
    IdModNoSuchUser,
    IdJoinSuccessAsMod,
};

/// Types of system status messages
enum StatusMessageTypes : u8 {
    IdMemberJoin = 1,  ///< Member joining
    IdMemberLeave,     ///< Member leaving
    IdMemberKicked,    ///< A member is kicked from the room
    IdMemberBanned,    ///< A member is banned from the room
    IdAddressUnbanned, ///< A username / ip address is unbanned from the room
};

/// This is what a server [person creating a server] would use.
class Room final {
public:
    enum class State : u8 {
        Open,   ///< The room is open and ready to accept connections.
        Closed, ///< The room is not opened and can not accept connections.
    };

    Room();
    ~Room();

    /**
     * Gets the current state of the room.
     */
    State GetState() const;

    /**
     * Gets the room information of the room.
     */
    const RoomInformation& GetRoomInformation() const;

    /**
     * Gets the verify UID of this room.
     */
    std::string GetVerifyUID() const;

    /**
     * Gets a list of the mbmers connected to the room.
     */
    std::vector<Member> GetRoomMemberList() const;

    /**
     * Checks if the room is password protected
     */
    bool HasPassword() const;

    using UsernameBanList = std::vector<std::string>;
    using IPBanList = std::vector<std::string>;

    using BanList = std::pair<UsernameBanList, IPBanList>;

    /**
     * Creates the socket for this room. Will bind to default address if
     * server is empty string.
     */
    bool Create(const std::string& name, const std::string& description = "",
                const std::string& server = "", u16 server_port = DefaultRoomPort,
                const std::string& password = "",
                const u32 max_connections = MaxConcurrentConnections,
                const std::string& host_username = "", const GameInfo = {},
                std::unique_ptr<VerifyUser::Backend> verify_backend = nullptr,
                const BanList& ban_list = {}, bool enable_yuzu_mods = false);

    /**
     * Sets the verification GUID of the room.
     */
    void SetVerifyUID(const std::string& uid);

    /**
     * Gets the ban list (including banned forum usernames and IPs) of the room.
     */
    BanList GetBanList() const;

    /**
     * Destroys the socket
     */
    void Destroy();

private:
    class RoomImpl;
    std::unique_ptr<RoomImpl> room_impl;
};

} // namespace Network
