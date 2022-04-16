// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <QWidget>

class QColor;
class QPushButton;

namespace Ui {
class ConfigureInputAdvanced;
}

class ConfigureInputAdvanced : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureInputAdvanced(QWidget* parent = nullptr);
    ~ConfigureInputAdvanced() override;

    void ApplyConfiguration();

signals:
    void CallDebugControllerDialog();
    void CallMouseConfigDialog();
    void CallTouchscreenConfigDialog();
    void CallMotionTouchConfigDialog();
    void CallRingControllerDialog();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();
    void UpdateUIEnabled();

    void OnControllerButtonClick(std::size_t player_idx, std::size_t button_idx);

    void LoadConfiguration();

    std::unique_ptr<Ui::ConfigureInputAdvanced> ui;

    std::array<std::array<QColor, 4>, 8> controllers_colors;
    std::array<std::array<QPushButton*, 4>, 8> controllers_color_buttons;
};
