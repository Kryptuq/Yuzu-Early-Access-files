// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>
#include "common/param_package.h"

class QLabel;
class QPushButton;
class QStringListModel;
class QVBoxLayout;

namespace InputCommon {
class InputSubsystem;
}

namespace InputCommon::CemuhookUDP {
class CalibrationConfigurationJob;
}

namespace Ui {
class ConfigureMotionTouch;
}

/// A dialog for touchpad calibration configuration.
class CalibrationConfigurationDialog : public QDialog {
    Q_OBJECT
public:
    explicit CalibrationConfigurationDialog(QWidget* parent, const std::string& host, u16 port);
    ~CalibrationConfigurationDialog() override;

private:
    Q_INVOKABLE void UpdateLabelText(const QString& text);
    Q_INVOKABLE void UpdateButtonText(const QString& text);

    QVBoxLayout* layout;
    QLabel* status_label;
    QPushButton* cancel_button;
    std::unique_ptr<InputCommon::CemuhookUDP::CalibrationConfigurationJob> job;

    // Configuration results
    bool completed{};
    u16 min_x{};
    u16 min_y{};
    u16 max_x{};
    u16 max_y{};

    friend class ConfigureMotionTouch;
};

class ConfigureMotionTouch : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureMotionTouch(QWidget* parent, InputCommon::InputSubsystem* input_subsystem_);
    ~ConfigureMotionTouch() override;

public slots:
    void ApplyConfiguration();

private slots:
    void OnUDPAddServer();
    void OnUDPDeleteServer();
    void OnCemuhookUDPTest();
    void OnConfigureTouchCalibration();
    void OnConfigureTouchFromButton();

private:
    void closeEvent(QCloseEvent* event) override;
    Q_INVOKABLE void ShowUDPTestResult(bool result);
    void SetConfiguration();
    void UpdateUiDisplay();
    void ConnectEvents();
    bool CanCloseDialog();
    std::string GetUDPServerString() const;

    InputCommon::InputSubsystem* input_subsystem;

    std::unique_ptr<Ui::ConfigureMotionTouch> ui;
    QStringListModel* udp_server_list_model;

    // Coordinate system of the CemuhookUDP touch provider
    int min_x{};
    int min_y{};
    int max_x{};
    int max_y{};

    bool udp_test_in_progress{};

    std::vector<Settings::TouchFromButtonMap> touch_from_button_maps;
};
