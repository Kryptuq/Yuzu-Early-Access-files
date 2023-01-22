// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include "common/settings.h"

namespace ConfigurationShared {

constexpr int USE_GLOBAL_INDEX = 0;
constexpr int USE_GLOBAL_SEPARATOR_INDEX = 1;
constexpr int USE_GLOBAL_OFFSET = 2;

// CheckBoxes require a tracker for their state since we emulate a tristate CheckBox
enum class CheckState {
    Off,    // Checkbox overrides to off/false
    On,     // Checkbox overrides to on/true
    Global, // Checkbox defers to the global state
    Count,  // Simply the number of states, not a valid checkbox state
};

// Global-aware apply and set functions

// ApplyPerGameSetting, given a Settings::Setting and a Qt UI element, properly applies a Setting
void ApplyPerGameSetting(Settings::SwitchableSetting<bool>* setting, const QCheckBox* checkbox,
                         const CheckState& tracker);
template <typename Type, bool ranged>
void ApplyPerGameSetting(Settings::SwitchableSetting<Type, ranged>* setting,
                         const QComboBox* combobox) {
    if (Settings::IsConfiguringGlobal() && setting->UsingGlobal()) {
        setting->SetValue(static_cast<Type>(combobox->currentIndex()));
    } else if (!Settings::IsConfiguringGlobal()) {
        if (combobox->currentIndex() == ConfigurationShared::USE_GLOBAL_INDEX) {
            setting->SetGlobal(true);
        } else {
            setting->SetGlobal(false);
            setting->SetValue(static_cast<Type>(combobox->currentIndex() -
                                                ConfigurationShared::USE_GLOBAL_OFFSET));
        }
    }
}

// Sets a Qt UI element given a Settings::Setting
void SetPerGameSetting(QCheckBox* checkbox, const Settings::SwitchableSetting<bool>* setting);

template <typename Type, bool ranged>
void SetPerGameSetting(QComboBox* combobox,
                       const Settings::SwitchableSetting<Type, ranged>* setting) {
    combobox->setCurrentIndex(setting->UsingGlobal() ? ConfigurationShared::USE_GLOBAL_INDEX
                                                     : static_cast<int>(setting->GetValue()) +
                                                           ConfigurationShared::USE_GLOBAL_OFFSET);
}

// (Un)highlights a Qt UI element
void SetHighlight(QWidget* widget, bool highlighted);

// Sets up a QCheckBox like a tristate one, given a Setting
void SetColoredTristate(QCheckBox* checkbox, const Settings::SwitchableSetting<bool>& setting,
                        CheckState& tracker);
void SetColoredTristate(QCheckBox* checkbox, bool global, bool state, bool global_state,
                        CheckState& tracker);

// Sets up coloring of a QWidget `target` based on the state of a QComboBox, and calls
// InsertGlobalItem
void SetColoredComboBox(QComboBox* combobox, QWidget* target, int global);

// Adds the "Use Global Configuration" selection and separator to the beginning of a QComboBox
void InsertGlobalItem(QComboBox* combobox, int global_index);

// Returns the correct index of a QComboBox taking into account global configuration
int GetComboboxIndex(int global_setting_index, const QComboBox* combobox);

} // namespace ConfigurationShared
