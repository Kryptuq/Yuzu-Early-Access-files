// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <vector>
#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>
#include "common/common_types.h"

namespace UISettings {

using ContextualShortcut = std::pair<QString, int>;

struct Shortcut {
    QString name;
    QString group;
    ContextualShortcut shortcut;
};

using Themes = std::array<std::pair<const char*, const char*>, 6>;
extern const Themes themes;

struct GameDir {
    QString path;
    bool deep_scan = false;
    bool expanded = false;
    bool operator==(const GameDir& rhs) const {
        return path == rhs.path;
    }
    bool operator!=(const GameDir& rhs) const {
        return !operator==(rhs);
    }
};

struct Values {
    QByteArray geometry;
    QByteArray state;

    QByteArray renderwindow_geometry;

    QByteArray gamelist_header_state;

    QByteArray microprofile_geometry;
    bool microprofile_visible;

    bool single_window_mode;
    bool fullscreen;
    bool display_titlebar;
    bool show_filter_bar;
    bool show_status_bar;

    bool confirm_before_closing;
    bool first_start;
    bool pause_when_in_background;
    bool hide_mouse;

    bool select_user_on_boot;

    // Discord RPC
    bool enable_discord_presence;

    bool enable_screenshot_save_as;
    u16 screenshot_resolution_factor;

    QString roms_path;
    QString symbols_path;
    QString game_dir_deprecated;
    bool game_dir_deprecated_deepscan;
    QVector<UISettings::GameDir> game_dirs;
    QVector<u64> favorited_ids;
    QStringList recent_files;
    QString language;

    QString theme;

    // Shortcut name <Shortcut, context>
    std::vector<Shortcut> shortcuts;

    uint32_t callout_flags;

    // logging
    bool show_console;

    // Game List
    bool show_add_ons;
    uint32_t icon_size;
    uint8_t row_1_text_id;
    uint8_t row_2_text_id;
    std::atomic_bool is_game_list_reload_pending{false};
    bool cache_game_list;

    bool configuration_applied;
    bool reset_to_defaults;
};

extern Values values;

} // namespace UISettings

Q_DECLARE_METATYPE(UISettings::GameDir*);
