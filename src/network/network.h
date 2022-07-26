// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "network/room.h"
#include "network/room_member.h"

namespace Network {

class RoomNetwork {
public:
    RoomNetwork();

    /// Initializes and registers the network device, the room, and the room member.
    bool Init();

    /// Returns a pointer to the room handle
    std::weak_ptr<Room> GetRoom();

    /// Returns a pointer to the room member handle
    std::weak_ptr<RoomMember> GetRoomMember();

    /// Unregisters the network device, the room, and the room member and shut them down.
    void Shutdown();

private:
    std::shared_ptr<RoomMember> m_room_member; ///< RoomMember (Client) for network games
    std::shared_ptr<Room> m_room;              ///< Room (Server) for network games
};

} // namespace Network
