// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <thread>

#ifdef _WIN32
// windows.h needs to be included before shellapi.h
#include <windows.h>

#include <shellapi.h>
#endif

#include <mbedtls/base64.h>
#include "common/common_types.h"
#include "common/detached_tasks.h"
#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging/backend.h"
#include "common/logging/log.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "common/string_util.h"
#include "core/core.h"
#include "network/announce_multiplayer_session.h"
#include "network/network.h"
#include "network/room.h"
#include "network/verify_user.h"

#ifdef ENABLE_WEB_SERVICE
#include "web_service/verify_user_jwt.h"
#endif

#undef _UNICODE
#include <getopt.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

static void PrintHelp(const char* argv0) {
    LOG_INFO(Network,
             "Usage: {}"
             " [options] <filename>\n"
             "--room-name         The name of the room\n"
             "--room-description  The room description\n"
             "--port              The port used for the room\n"
             "--max_members       The maximum number of players for this room\n"
             "--password          The password for the room\n"
             "--preferred-game    The preferred game for this room\n"
             "--preferred-game-id The preferred game-id for this room\n"
             "--username          The username used for announce\n"
             "--token             The token used for announce\n"
             "--web-api-url       yuzu Web API url\n"
             "--ban-list-file     The file for storing the room ban list\n"
             "--log-file          The file for storing the room log\n"
             "--enable-yuzu-mods Allow yuzu Community Moderators to moderate on your room\n"
             "-h, --help          Display this help and exit\n"
             "-v, --version       Output version information and exit\n",
             argv0);
}

static void PrintVersion() {
    LOG_INFO(Network, "yuzu dedicated room {} {} Libnetwork: {}", Common::g_scm_branch,
             Common::g_scm_desc, Network::network_version);
}

/// The magic text at the beginning of a yuzu-room ban list file.
static constexpr char BanListMagic[] = "YuzuRoom-BanList-1";

static constexpr char token_delimiter{':'};

static void PadToken(std::string& token) {
    std::size_t outlen = 0;

    std::array<unsigned char, 512> output{};
    std::array<unsigned char, 2048> roundtrip{};
    for (size_t i = 0; i < 3; i++) {
        mbedtls_base64_decode(output.data(), output.size(), &outlen,
                              reinterpret_cast<const unsigned char*>(token.c_str()),
                              token.length());
        mbedtls_base64_encode(roundtrip.data(), roundtrip.size(), &outlen, output.data(), outlen);
        if (memcmp(roundtrip.data(), token.data(), token.size()) == 0) {
            break;
        }
        token.push_back('=');
    }
}

static std::string UsernameFromDisplayToken(const std::string& display_token) {
    std::size_t outlen;

    std::array<unsigned char, 512> output{};
    mbedtls_base64_decode(output.data(), output.size(), &outlen,
                          reinterpret_cast<const unsigned char*>(display_token.c_str()),
                          display_token.length());
    std::string decoded_display_token(reinterpret_cast<char*>(&output), outlen);
    return decoded_display_token.substr(0, decoded_display_token.find(token_delimiter));
}

static std::string TokenFromDisplayToken(const std::string& display_token) {
    std::size_t outlen;

    std::array<unsigned char, 512> output{};
    mbedtls_base64_decode(output.data(), output.size(), &outlen,
                          reinterpret_cast<const unsigned char*>(display_token.c_str()),
                          display_token.length());
    std::string decoded_display_token(reinterpret_cast<char*>(&output), outlen);
    return decoded_display_token.substr(decoded_display_token.find(token_delimiter) + 1);
}

static Network::Room::BanList LoadBanList(const std::string& path) {
    std::ifstream file;
    Common::FS::OpenFileStream(file, path, std::ios_base::in);
    if (!file || file.eof()) {
        LOG_ERROR(Network, "Could not open ban list!");
        return {};
    }
    std::string magic;
    std::getline(file, magic);
    if (magic != BanListMagic) {
        LOG_ERROR(Network, "Ban list is not valid!");
        return {};
    }

    // false = username ban list, true = ip ban list
    bool ban_list_type = false;
    Network::Room::UsernameBanList username_ban_list;
    Network::Room::IPBanList ip_ban_list;
    while (!file.eof()) {
        std::string line;
        std::getline(file, line);
        line.erase(std::remove(line.begin(), line.end(), '\0'), line.end());
        line = Common::StripSpaces(line);
        if (line.empty()) {
            // An empty line marks start of the IP ban list
            ban_list_type = true;
            continue;
        }
        if (ban_list_type) {
            ip_ban_list.emplace_back(line);
        } else {
            username_ban_list.emplace_back(line);
        }
    }

    return {username_ban_list, ip_ban_list};
}

static void SaveBanList(const Network::Room::BanList& ban_list, const std::string& path) {
    std::ofstream file;
    Common::FS::OpenFileStream(file, path, std::ios_base::out);
    if (!file) {
        LOG_ERROR(Network, "Could not save ban list!");
        return;
    }

    file << BanListMagic << "\n";

    // Username ban list
    for (const auto& username : ban_list.first) {
        file << username << "\n";
    }
    file << "\n";

    // IP ban list
    for (const auto& ip : ban_list.second) {
        file << ip << "\n";
    }
}

static void InitializeLogging(const std::string& log_file) {
    Common::Log::Initialize();
    Common::Log::SetColorConsoleBackendEnabled(true);
    Common::Log::Start();
}

/// Application entry point
int main(int argc, char** argv) {
    Common::DetachedTasks detached_tasks;
    int option_index = 0;
    char* endarg;

    std::string room_name;
    std::string room_description;
    std::string password;
    std::string preferred_game;
    std::string username;
    std::string token;
    std::string web_api_url;
    std::string ban_list_file;
    std::string log_file = "yuzu-room.log";
    u64 preferred_game_id = 0;
    u32 port = Network::DefaultRoomPort;
    u32 max_members = 16;
    bool enable_yuzu_mods = false;

    static struct option long_options[] = {
        {"room-name", required_argument, 0, 'n'},
        {"room-description", required_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {"max_members", required_argument, 0, 'm'},
        {"password", required_argument, 0, 'w'},
        {"preferred-game", required_argument, 0, 'g'},
        {"preferred-game-id", required_argument, 0, 'i'},
        {"username", optional_argument, 0, 'u'},
        {"token", required_argument, 0, 't'},
        {"web-api-url", required_argument, 0, 'a'},
        {"ban-list-file", required_argument, 0, 'b'},
        {"log-file", required_argument, 0, 'l'},
        {"enable-yuzu-mods", no_argument, 0, 'e'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0},
    };

    InitializeLogging(log_file);

    while (optind < argc) {
        int arg = getopt_long(argc, argv, "n:d:p:m:w:g:u:t:a:i:l:hv", long_options, &option_index);
        if (arg != -1) {
            switch (static_cast<char>(arg)) {
            case 'n':
                room_name.assign(optarg);
                break;
            case 'd':
                room_description.assign(optarg);
                break;
            case 'p':
                port = strtoul(optarg, &endarg, 0);
                break;
            case 'm':
                max_members = strtoul(optarg, &endarg, 0);
                break;
            case 'w':
                password.assign(optarg);
                break;
            case 'g':
                preferred_game.assign(optarg);
                break;
            case 'i':
                preferred_game_id = strtoull(optarg, &endarg, 16);
                break;
            case 'u':
                username.assign(optarg);
                break;
            case 't':
                token.assign(optarg);
                break;
            case 'a':
                web_api_url.assign(optarg);
                break;
            case 'b':
                ban_list_file.assign(optarg);
                break;
            case 'l':
                log_file.assign(optarg);
                break;
            case 'e':
                enable_yuzu_mods = true;
                break;
            case 'h':
                PrintHelp(argv[0]);
                return 0;
            case 'v':
                PrintVersion();
                return 0;
            }
        }
    }

    if (room_name.empty()) {
        LOG_ERROR(Network, "Room name is empty!");
        PrintHelp(argv[0]);
        return -1;
    }
    if (preferred_game.empty()) {
        LOG_ERROR(Network, "Preferred game is empty!");
        PrintHelp(argv[0]);
        return -1;
    }
    if (preferred_game_id == 0) {
        LOG_ERROR(Network,
                  "preferred-game-id not set!\nThis should get set to allow users to find your "
                  "room.\nSet with --preferred-game-id id");
    }
    if (max_members > Network::MaxConcurrentConnections || max_members < 2) {
        LOG_ERROR(Network, "max_members needs to be in the range 2 - {}!",
                  Network::MaxConcurrentConnections);
        PrintHelp(argv[0]);
        return -1;
    }
    if (port > UINT16_MAX) {
        LOG_ERROR(Network, "Port needs to be in the range 0 - 65535!");
        PrintHelp(argv[0]);
        return -1;
    }
    if (ban_list_file.empty()) {
        LOG_ERROR(Network, "Ban list file not set!\nThis should get set to load and save room ban "
                           "list.\nSet with --ban-list-file <file>");
    }
    bool announce = true;
    if (token.empty() && announce) {
        announce = false;
        LOG_INFO(Network, "Token is empty: Hosting a private room");
    }
    if (web_api_url.empty() && announce) {
        announce = false;
        LOG_INFO(Network, "Endpoint url is empty: Hosting a private room");
    }
    if (announce) {
        if (username.empty()) {
            LOG_INFO(Network, "Hosting a public room");
            Settings::values.web_api_url = web_api_url;
            PadToken(token);
            Settings::values.yuzu_username = UsernameFromDisplayToken(token);
            username = Settings::values.yuzu_username.GetValue();
            Settings::values.yuzu_token = TokenFromDisplayToken(token);
        } else {
            LOG_INFO(Network, "Hosting a public room");
            Settings::values.web_api_url = web_api_url;
            Settings::values.yuzu_username = username;
            Settings::values.yuzu_token = token;
        }
    }
    if (!announce && enable_yuzu_mods) {
        enable_yuzu_mods = false;
        LOG_INFO(Network, "Can not enable yuzu Moderators for private rooms");
    }

    // Load the ban list
    Network::Room::BanList ban_list;
    if (!ban_list_file.empty()) {
        ban_list = LoadBanList(ban_list_file);
    }

    std::unique_ptr<Network::VerifyUser::Backend> verify_backend;
    if (announce) {
#ifdef ENABLE_WEB_SERVICE
        verify_backend =
            std::make_unique<WebService::VerifyUserJWT>(Settings::values.web_api_url.GetValue());
#else
        LOG_INFO(Network,
                 "yuzu Web Services is not available with this build: validation is disabled.");
        verify_backend = std::make_unique<Network::VerifyUser::NullBackend>();
#endif
    } else {
        verify_backend = std::make_unique<Network::VerifyUser::NullBackend>();
    }

    Network::RoomNetwork network{};
    network.Init();
    if (auto room = network.GetRoom().lock()) {
        AnnounceMultiplayerRoom::GameInfo preferred_game_info{.name = preferred_game,
                                                              .id = preferred_game_id};
        if (!room->Create(room_name, room_description, "", port, password, max_members, username,
                          preferred_game_info, std::move(verify_backend), ban_list,
                          enable_yuzu_mods)) {
            LOG_INFO(Network, "Failed to create room: ");
            return -1;
        }
        LOG_INFO(Network, "Room is open. Close with Q+Enter...");
        auto announce_session = std::make_unique<Core::AnnounceMultiplayerSession>(network);
        if (announce) {
            announce_session->Start();
        }
        while (room->GetState() == Network::Room::State::Open) {
            std::string in;
            std::cin >> in;
            if (in.size() > 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (announce) {
            announce_session->Stop();
        }
        announce_session.reset();
        // Save the ban list
        if (!ban_list_file.empty()) {
            SaveBanList(room->GetBanList(), ban_list_file);
        }
        room->Destroy();
    }
    network.Shutdown();
    detached_tasks.WaitForAllTasks();
    return 0;
}
