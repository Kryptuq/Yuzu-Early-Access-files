// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QKeySequence>
#include <QShortcut>
#include <QTreeWidgetItem>
#include <QtGlobal>
#include "yuzu/hotkeys.h"
#include "yuzu/uisettings.h"

HotkeyRegistry::HotkeyRegistry() = default;
HotkeyRegistry::~HotkeyRegistry() = default;

void HotkeyRegistry::SaveHotkeys() {
    UISettings::values.shortcuts.clear();
    for (const auto& group : hotkey_groups) {
        for (const auto& hotkey : group.second) {
            UISettings::values.shortcuts.push_back(
                {hotkey.first, group.first,
                 UISettings::ContextualShortcut(hotkey.second.keyseq.toString(),
                                                hotkey.second.context)});
        }
    }
}

void HotkeyRegistry::LoadHotkeys() {
    // Make sure NOT to use a reference here because it would become invalid once we call
    // beginGroup()
    for (auto shortcut : UISettings::values.shortcuts) {
        Hotkey& hk = hotkey_groups[shortcut.group][shortcut.name];
        if (!shortcut.shortcut.first.isEmpty()) {
            hk.keyseq = QKeySequence::fromString(shortcut.shortcut.first, QKeySequence::NativeText);
            hk.context = static_cast<Qt::ShortcutContext>(shortcut.shortcut.second);
        }
        if (hk.shortcut) {
            hk.shortcut->disconnect();
            hk.shortcut->setKey(hk.keyseq);
        }
    }
}

QShortcut* HotkeyRegistry::GetHotkey(const QString& group, const QString& action, QWidget* widget) {
    Hotkey& hk = hotkey_groups[group][action];

    if (!hk.shortcut)
        hk.shortcut = new QShortcut(hk.keyseq, widget, nullptr, nullptr, hk.context);

    hk.shortcut->setAutoRepeat(false);

    return hk.shortcut;
}

QKeySequence HotkeyRegistry::GetKeySequence(const QString& group, const QString& action) {
    return hotkey_groups[group][action].keyseq;
}

Qt::ShortcutContext HotkeyRegistry::GetShortcutContext(const QString& group,
                                                       const QString& action) {
    return hotkey_groups[group][action].context;
}
