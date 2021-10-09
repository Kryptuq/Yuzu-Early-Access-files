// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Core {
class System;
}

namespace Ui {
class ConfigureDebug;
}

class ConfigureDebug : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureDebug(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureDebug() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;

    void RetranslateUI();
    void SetConfiguration();

    std::unique_ptr<Ui::ConfigureDebug> ui;

    const Core::System& system;
};
