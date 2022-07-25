// Text : Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <QDialog>

class QTimer;
class QCamera;
class QCameraImageCapture;

namespace InputCommon {
class InputSubsystem;
} // namespace InputCommon

namespace Ui {
class ConfigureCamera;
}

class ConfigureCamera : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureCamera(QWidget* parent, InputCommon::InputSubsystem* input_subsystem_);
    ~ConfigureCamera() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    /// Load configuration settings.
    void LoadConfiguration();

    /// Restore all buttons to their default values.
    void RestoreDefaults();

    void DisplayCapturedFrame(int requestId, const QImage& img);

    /// Loads and signals the current selected camera to display a frame
    void PreviewCamera();

    InputCommon::InputSubsystem* input_subsystem;

    bool is_virtual_camera;
    int pending_snapshots;
    std::unique_ptr<QCamera> camera;
    std::unique_ptr<QCameraImageCapture> camera_capture;
    std::unique_ptr<QTimer> camera_timer;
    std::vector<std::string> input_devices;
    std::unique_ptr<Ui::ConfigureCamera> ui;
};
