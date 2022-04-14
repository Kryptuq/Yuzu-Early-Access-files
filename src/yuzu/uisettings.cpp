// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "yuzu/uisettings.h"

namespace UISettings {

const Themes themes{{
    {"Default", "default"},
    {"Default Colorful", "colorful"},
    {"Dark", "qdarkstyle"},
    {"Dark Colorful", "colorful_dark"},
    {"Midnight Blue", "qdarkstyle_midnight_blue"},
    {"Midnight Blue Colorful", "colorful_midnight_blue"},
}};

bool IsDarkTheme() {
    const auto& theme = UISettings::values.theme;
    return theme == QStringLiteral("qdarkstyle") ||
           theme == QStringLiteral("qdarkstyle_midnight_blue") ||
           theme == QStringLiteral("colorful_dark") ||
           theme == QStringLiteral("colorful_midnight_blue");
}

Values values = {};

} // namespace UISettings
