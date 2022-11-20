// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include <QWidget>

#include "ui_configure_input_per_game.h"
#include "yuzu/configuration/input_profiles.h"

class QComboBox;

namespace Core {
class System;
} // namespace Core

class Config;

class ConfigureInputPerGame : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureInputPerGame(Core::System& system_, Config* config_,
                                   QWidget* parent = nullptr);

    /// Load and Save configurations to settings file.
    void ApplyConfiguration();

private:
    /// Load configuration from settings file.
    void LoadConfiguration();

    /// Save configuration to settings file.
    void SaveConfiguration();

    std::unique_ptr<Ui::ConfigureInputPerGame> ui;
    std::unique_ptr<InputProfiles> profiles;

    std::array<QComboBox*, 8> profile_comboboxes;

    Core::System& system;
    Config* config;
};
