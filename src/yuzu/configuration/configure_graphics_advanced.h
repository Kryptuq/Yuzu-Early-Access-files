// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QWidget>

namespace Core {
class System;
}

namespace ConfigurationShared {
enum class CheckState;
}

namespace Ui {
class ConfigureGraphicsAdvanced;
}

class ConfigureGraphicsAdvanced : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGraphicsAdvanced(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureGraphicsAdvanced() override;

    void ApplyConfiguration();
    void SetConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void SetupPerGameUI();

    std::unique_ptr<Ui::ConfigureGraphicsAdvanced> ui;

    ConfigurationShared::CheckState use_vsync;
    ConfigurationShared::CheckState use_asynchronous_shaders;
    ConfigurationShared::CheckState use_fast_gpu_time;
    ConfigurationShared::CheckState use_pessimistic_flushes;

    const Core::System& system;
};
