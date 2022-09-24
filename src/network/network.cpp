// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "enet/enet.h"
#include "network/network.h"

namespace Network {

RoomNetwork::RoomNetwork() {
    m_room = std::make_shared<Room>();
    m_room_member = std::make_shared<RoomMember>();
}

bool RoomNetwork::Init() {
    if (enet_initialize() != 0) {
        LOG_ERROR(Network, "Error initializing ENet");
        return false;
    }
    m_room = std::make_shared<Room>();
    m_room_member = std::make_shared<RoomMember>();
    LOG_DEBUG(Network, "initialized OK");
    return true;
}

std::weak_ptr<Room> RoomNetwork::GetRoom() {
    return m_room;
}

std::weak_ptr<RoomMember> RoomNetwork::GetRoomMember() {
    return m_room_member;
}

void RoomNetwork::Shutdown() {
    if (m_room_member) {
        if (m_room_member->IsConnected())
            m_room_member->Leave();
        m_room_member.reset();
    }
    if (m_room) {
        if (m_room->GetState() == Room::State::Open)
            m_room->Destroy();
        m_room.reset();
    }
    enet_deinitialize();
    LOG_DEBUG(Network, "shutdown OK");
}

} // namespace Network
