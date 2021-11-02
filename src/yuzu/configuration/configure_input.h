// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>

#include <QKeyEvent>
#include <QList>
#include <QWidget>

namespace Core {
class System;
}

class QCheckBox;
class QString;
class QTimer;

class ConfigureInputAdvanced;
class ConfigureInputPlayer;

class InputProfiles;

namespace InputCommon {
class InputSubsystem;
}

namespace Ui {
class ConfigureInput;
}

void OnDockedModeChanged(bool last_state, bool new_state, Core::System& system);

class ConfigureInput : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureInput(Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureInput() override;

    /// Initializes the input dialog with the given input subsystem.
    void Initialize(InputCommon::InputSubsystem* input_subsystem_, Core::System& system,
                    std::size_t max_players = 8);

    /// Save all button configurations to settings file.
    void ApplyConfiguration();

    QList<QWidget*> GetSubTabs() const;

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();
    void ClearAll();

    void UpdateDockedState(bool is_handheld);
    void UpdateAllInputDevices();
    void UpdateAllInputProfiles(std::size_t player_index);

    /// Load configuration settings.
    void LoadConfiguration();
    void LoadPlayerControllerIndices();

    /// Restore all buttons to their default values.
    void RestoreDefaults();

    std::unique_ptr<Ui::ConfigureInput> ui;

    std::unique_ptr<InputProfiles> profiles;

    std::array<ConfigureInputPlayer*, 8> player_controllers;
    std::array<QWidget*, 8> player_tabs;
    std::array<QCheckBox*, 8> player_connected;
    ConfigureInputAdvanced* advanced;

    Core::System& system;
};
