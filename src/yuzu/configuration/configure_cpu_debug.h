// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QWidget>

namespace Core {
class System;
}

namespace Ui {
class ConfigureCpuDebug;
}

class ConfigureCpuDebug : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureCpuDebug(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureCpuDebug() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void SetConfiguration();

    std::unique_ptr<Ui::ConfigureCpuDebug> ui;

    const Core::System& system;
};
