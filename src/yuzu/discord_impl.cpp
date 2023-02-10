// SPDX-FileCopyrightText: 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <string>
#include <discord_rpc.h>
#include <fmt/format.h>
#include <httplib.h>
#include "common/common_types.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/loader/loader.h"
#include "yuzu/discord_impl.h"
#include "yuzu/uisettings.h"

namespace DiscordRPC {

DiscordImpl::DiscordImpl(Core::System& system_) : system{system_} {
    DiscordEventHandlers handlers{};
    // The number is the client ID for yuzu, it's used for images and the
    // application name
    Discord_Initialize("712465656758665259", &handlers, 1, nullptr);
}

DiscordImpl::~DiscordImpl() {
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordImpl::Pause() {
    Discord_ClearPresence();
}

static std::string GetGameString(const std::string& title) {
    // Convert to lowercase
    std::string icon_name = Common::ToLower(title);

    // Replace spaces with dashes
    std::replace(icon_name.begin(), icon_name.end(), ' ', '-');

    // Remove non-alphanumeric characters but keep dashes
    std::erase_if(icon_name, [](char c) { return !std::isalnum(c) && c != '-'; });

    // Remove dashes from the start and end of the string
    icon_name.erase(icon_name.begin(), std::find_if(icon_name.begin(), icon_name.end(),
                                                    [](int ch) { return ch != '-'; }));
    icon_name.erase(
        std::find_if(icon_name.rbegin(), icon_name.rend(), [](int ch) { return ch != '-'; }).base(),
        icon_name.end());

    // Remove double dashes
    icon_name.erase(std::unique(icon_name.begin(), icon_name.end(),
                                [](char a, char b) { return a == '-' && b == '-'; }),
                    icon_name.end());

    return icon_name;
}

void DiscordImpl::Update() {
    s64 start_time = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    const std::string default_text = "yuzu is an emulator for the Nintendo Switch";
    const std::string default_image = "yuzu_logo_ea";
    std::string game_cover_url = "https://yuzu-emu.org";
    std::string title;

    DiscordRichPresence presence{};

    if (system.IsPoweredOn()) {
        system.GetAppLoader().ReadTitle(title);

        // Used to format Icon URL for yuzu website game compatibility page
        std::string icon_name = GetGameString(title);

        // New Check for game cover
        httplib::Client cli(game_cover_url);

        if (auto res = cli.Head(fmt::format("/images/game/boxart/{}.png", icon_name).c_str())) {
            if (res->status == 200) {
                game_cover_url += fmt::format("/images/game/boxart/{}.png", icon_name);
            } else {
                game_cover_url = "yuzu_logo_ea";
            }
        } else {
            game_cover_url = "yuzu_logo_ea";
        }

        presence.largeImageKey = game_cover_url.c_str();
        presence.largeImageText = title.c_str();

        presence.smallImageKey = default_image.c_str();
        presence.smallImageText = default_text.c_str();
        presence.state = title.c_str();
        presence.details = "Currently in game";
    } else {
        presence.largeImageKey = default_image.c_str();
        presence.largeImageText = default_text.c_str();
        presence.details = "Currently not in game";
    }

    presence.startTimestamp = start_time;
    Discord_UpdatePresence(&presence);
}
} // namespace DiscordRPC
