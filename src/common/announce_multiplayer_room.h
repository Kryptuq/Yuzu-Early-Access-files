// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>
#include "common/common_types.h"
#include "common/socket_types.h"
#include "web_service/web_result.h"

namespace AnnounceMultiplayerRoom {

struct GameInfo {
    std::string name{""};
    u64 id{0};
    std::string version{""};
};

struct Member {
    std::string username;
    std::string nickname;
    std::string display_name;
    std::string avatar_url;
    Network::IPv4Address fake_ip;
    GameInfo game;
};

struct RoomInformation {
    std::string name;          ///< Name of the server
    std::string description;   ///< Server description
    u32 member_slots;          ///< Maximum number of members in this room
    u16 port;                  ///< The port of this room
    GameInfo preferred_game;   ///< Game to advertise that you want to play
    std::string host_username; ///< Forum username of the host
    bool enable_yuzu_mods;     ///< Allow yuzu Moderators to moderate on this room
};

struct Room {
    RoomInformation information;

    std::string id;
    std::string verify_uid; ///< UID used for verification
    std::string ip;
    u32 net_version;
    bool has_password;

    std::vector<Member> members;
};
using RoomList = std::vector<Room>;

/**
 * A AnnounceMultiplayerRoom interface class. A backend to submit/get to/from a web service should
 * implement this interface.
 */
class Backend {
public:
    virtual ~Backend() = default;

    /**
     * Sets the Information that gets used for the announce
     * @param uid The Id of the room
     * @param name The name of the room
     * @param description The room description
     * @param port The port of the room
     * @param net_version The version of the libNetwork that gets used
     * @param has_password True if the room is password protected
     * @param preferred_game The preferred game of the room
     * @param preferred_game_id The title id of the preferred game
     */
    virtual void SetRoomInformation(const std::string& name, const std::string& description,
                                    const u16 port, const u32 max_player, const u32 net_version,
                                    const bool has_password, const GameInfo& preferred_game) = 0;
    /**
     * Adds a player information to the data that gets announced
     * @param member The player to add
     */
    virtual void AddPlayer(const Member& member) = 0;

    /**
     * Updates the data in the announce service. Re-register the room when required.
     * @result The result of the update attempt
     */
    virtual WebService::WebResult Update() = 0;

    /**
     * Registers the data in the announce service
     * @result The result of the register attempt. When the result code is Success, A global Guid of
     * the room which may be used for verification will be in the result's returned_data.
     */
    virtual WebService::WebResult Register() = 0;

    /**
     * Empties the stored players
     */
    virtual void ClearPlayers() = 0;

    /**
     * Get the room information from the announce service
     * @result A list of all rooms the announce service has
     */
    virtual RoomList GetRoomList() = 0;

    /**
     * Sends a delete message to the announce service
     */
    virtual void Delete() = 0;
};

/**
 * Empty implementation of AnnounceMultiplayerRoom interface that drops all data. Used when a
 * functional backend implementation is not available.
 */
class NullBackend : public Backend {
public:
    ~NullBackend() = default;
    void SetRoomInformation(const std::string& /*name*/, const std::string& /*description*/,
                            const u16 /*port*/, const u32 /*max_player*/, const u32 /*net_version*/,
                            const bool /*has_password*/,
                            const GameInfo& /*preferred_game*/) override {}
    void AddPlayer(const Member& /*member*/) override {}
    WebService::WebResult Update() override {
        return WebService::WebResult{WebService::WebResult::Code::NoWebservice,
                                     "WebService is missing", ""};
    }
    WebService::WebResult Register() override {
        return WebService::WebResult{WebService::WebResult::Code::NoWebservice,
                                     "WebService is missing", ""};
    }
    void ClearPlayers() override {}
    RoomList GetRoomList() override {
        return RoomList{};
    }

    void Delete() override {}
};

} // namespace AnnounceMultiplayerRoom
