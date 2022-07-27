// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <QWidget>

namespace Core {
class System;
}

class ConfigureDialog;

namespace ConfigurationShared {
enum class CheckState;
}

class HotkeyRegistry;

namespace Ui {
class ConfigureGeneral;
}

class ConfigureGeneral : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGeneral(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureGeneral() override;

    void SetResetCallback(std::function<void()> callback);
    void ResetDefaults();
    void ApplyConfiguration();
    void SetConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void SetupPerGameUI();

    std::function<void()> reset_callback;

    std::unique_ptr<Ui::ConfigureGeneral> ui;

    ConfigurationShared::CheckState use_speed_limit;
    ConfigurationShared::CheckState use_multi_core;
    ConfigurationShared::CheckState use_extended_memory_layout;

    const Core::System& system;
};
