// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <future>
#include <vector>
#include "announce_multiplayer_session.h"
#include "common/announce_multiplayer_room.h"
#include "common/assert.h"
#include "common/settings.h"
#include "network/network.h"

#ifdef ENABLE_WEB_SERVICE
#include "web_service/announce_room_json.h"
#endif

namespace Core {

// Time between room is announced to web_service
static constexpr std::chrono::seconds announce_time_interval(15);

AnnounceMultiplayerSession::AnnounceMultiplayerSession(Network::RoomNetwork& room_network_)
    : room_network{room_network_} {
#ifdef ENABLE_WEB_SERVICE
    backend = std::make_unique<WebService::RoomJson>(Settings::values.web_api_url.GetValue(),
                                                     Settings::values.yuzu_username.GetValue(),
                                                     Settings::values.yuzu_token.GetValue());
#else
    backend = std::make_unique<AnnounceMultiplayerRoom::NullBackend>();
#endif
}

WebService::WebResult AnnounceMultiplayerSession::Register() {
    auto room = room_network.GetRoom().lock();
    if (!room) {
        return WebService::WebResult{WebService::WebResult::Code::LibError,
                                     "Network is not initialized", ""};
    }
    if (room->GetState() != Network::Room::State::Open) {
        return WebService::WebResult{WebService::WebResult::Code::LibError, "Room is not open", ""};
    }
    UpdateBackendData(room);
    WebService::WebResult result = backend->Register();
    if (result.result_code != WebService::WebResult::Code::Success) {
        return result;
    }
    LOG_INFO(WebService, "Room has been registered");
    room->SetVerifyUID(result.returned_data);
    registered = true;
    return WebService::WebResult{WebService::WebResult::Code::Success, "", ""};
}

void AnnounceMultiplayerSession::Start() {
    if (announce_multiplayer_thread) {
        Stop();
    }
    shutdown_event.Reset();
    announce_multiplayer_thread =
        std::make_unique<std::thread>(&AnnounceMultiplayerSession::AnnounceMultiplayerLoop, this);
}

void AnnounceMultiplayerSession::Stop() {
    if (announce_multiplayer_thread) {
        shutdown_event.Set();
        announce_multiplayer_thread->join();
        announce_multiplayer_thread.reset();
        backend->Delete();
        registered = false;
    }
}

AnnounceMultiplayerSession::CallbackHandle AnnounceMultiplayerSession::BindErrorCallback(
    std::function<void(const WebService::WebResult&)> function) {
    std::lock_guard lock(callback_mutex);
    auto handle = std::make_shared<std::function<void(const WebService::WebResult&)>>(function);
    error_callbacks.insert(handle);
    return handle;
}

void AnnounceMultiplayerSession::UnbindErrorCallback(CallbackHandle handle) {
    std::lock_guard lock(callback_mutex);
    error_callbacks.erase(handle);
}

AnnounceMultiplayerSession::~AnnounceMultiplayerSession() {
    Stop();
}

void AnnounceMultiplayerSession::UpdateBackendData(std::shared_ptr<Network::Room> room) {
    Network::RoomInformation room_information = room->GetRoomInformation();
    std::vector<AnnounceMultiplayerRoom::Member> memberlist = room->GetRoomMemberList();
    backend->SetRoomInformation(room_information.name, room_information.description,
                                room_information.port, room_information.member_slots,
                                Network::network_version, room->HasPassword(),
                                room_information.preferred_game);
    backend->ClearPlayers();
    for (const auto& member : memberlist) {
        backend->AddPlayer(member);
    }
}

void AnnounceMultiplayerSession::AnnounceMultiplayerLoop() {
    // Invokes all current bound error callbacks.
    const auto ErrorCallback = [this](WebService::WebResult result) {
        std::lock_guard lock(callback_mutex);
        for (auto callback : error_callbacks) {
            (*callback)(result);
        }
    };

    if (!registered) {
        WebService::WebResult result = Register();
        if (result.result_code != WebService::WebResult::Code::Success) {
            ErrorCallback(result);
            return;
        }
    }

    auto update_time = std::chrono::steady_clock::now();
    std::future<WebService::WebResult> future;
    while (!shutdown_event.WaitUntil(update_time)) {
        update_time += announce_time_interval;
        auto room = room_network.GetRoom().lock();
        if (!room) {
            break;
        }
        if (room->GetState() != Network::Room::State::Open) {
            break;
        }
        UpdateBackendData(room);
        WebService::WebResult result = backend->Update();
        if (result.result_code != WebService::WebResult::Code::Success) {
            ErrorCallback(result);
        }
        if (result.result_string == "404") {
            registered = false;
            // Needs to register the room again
            WebService::WebResult register_result = Register();
            if (register_result.result_code != WebService::WebResult::Code::Success) {
                ErrorCallback(register_result);
            }
        }
    }
}

AnnounceMultiplayerRoom::RoomList AnnounceMultiplayerSession::GetRoomList() {
    return backend->GetRoomList();
}

bool AnnounceMultiplayerSession::IsRunning() const {
    return announce_multiplayer_thread != nullptr;
}

void AnnounceMultiplayerSession::UpdateCredentials() {
    ASSERT_MSG(!IsRunning(), "Credentials can only be updated when session is not running");

#ifdef ENABLE_WEB_SERVICE
    backend = std::make_unique<WebService::RoomJson>(Settings::values.web_api_url.GetValue(),
                                                     Settings::values.yuzu_username.GetValue(),
                                                     Settings::values.yuzu_token.GetValue());
#endif
}

} // namespace Core
