// Text : Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QStandardItemModel>
#include <QTimer>

#include "input_common/drivers/camera.h"
#include "input_common/main.h"
#include "ui_configure_camera.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/configure_camera.h"

ConfigureCamera::ConfigureCamera(QWidget* parent, InputCommon::InputSubsystem* input_subsystem_)
    : QDialog(parent), input_subsystem{input_subsystem_},
      ui(std::make_unique<Ui::ConfigureCamera>()) {
    ui->setupUi(this);

    connect(ui->restore_defaults_button, &QPushButton::clicked, this,
            &ConfigureCamera::RestoreDefaults);
    connect(ui->preview_button, &QPushButton::clicked, this, &ConfigureCamera::PreviewCamera);

    auto blank_image = QImage(320, 240, QImage::Format::Format_RGB32);
    blank_image.fill(Qt::black);
    DisplayCapturedFrame(0, blank_image);

    LoadConfiguration();
    resize(0, 0);
}

ConfigureCamera::~ConfigureCamera() = default;

void ConfigureCamera::PreviewCamera() {
    const auto index = ui->ir_sensor_combo_box->currentIndex();
    bool camera_found = false;
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo& cameraInfo : cameras) {
        if (input_devices[index] == cameraInfo.deviceName().toStdString() ||
            input_devices[index] == "Auto") {
            LOG_INFO(Frontend, "Selected Camera {} {}", cameraInfo.description().toStdString(),
                     cameraInfo.deviceName().toStdString());
            camera = std::make_unique<QCamera>(cameraInfo);
            camera_found = true;
            break;
        }
    }

    // Clear previous frame
    auto blank_image = QImage(320, 240, QImage::Format::Format_RGB32);
    blank_image.fill(Qt::black);
    DisplayCapturedFrame(0, blank_image);

    if (!camera_found) {
        return;
    }

    camera_capture = std::make_unique<QCameraImageCapture>(camera.get());
    connect(camera_capture.get(), &QCameraImageCapture::imageCaptured, this,
            &ConfigureCamera::DisplayCapturedFrame);
    camera->unload();
    camera->setCaptureMode(QCamera::CaptureViewfinder);
    camera->load();
    camera->start();

    pending_snapshots = 0;
    is_virtual_camera = false;

    camera_timer = std::make_unique<QTimer>();
    connect(camera_timer.get(), &QTimer::timeout, [this] {
        // If the camera doesn't capture, test for virtual cameras
        if (pending_snapshots > 5) {
            is_virtual_camera = true;
        }
        // Virtual cameras like obs need to reset the camera every capture
        if (is_virtual_camera) {
            camera->stop();
            camera->start();
        }
        pending_snapshots++;
        camera_capture->capture();
    });

    camera_timer->start(250);
}

void ConfigureCamera::DisplayCapturedFrame(int requestId, const QImage& img) {
    LOG_INFO(Frontend, "ImageCaptured {} {}", img.width(), img.height());
    const auto converted = img.scaled(320, 240, Qt::AspectRatioMode::IgnoreAspectRatio,
                                      Qt::TransformationMode::SmoothTransformation);
    ui->preview_box->setPixmap(QPixmap::fromImage(converted));
    pending_snapshots = 0;
}

void ConfigureCamera::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureCamera::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureCamera::ApplyConfiguration() {
    const auto index = ui->ir_sensor_combo_box->currentIndex();
    Settings::values.ir_sensor_device.SetValue(input_devices[index]);
}

void ConfigureCamera::LoadConfiguration() {
    input_devices.clear();
    ui->ir_sensor_combo_box->clear();
    input_devices.push_back("Auto");
    ui->ir_sensor_combo_box->addItem(tr("Auto"));
    const auto cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo& cameraInfo : cameras) {
        input_devices.push_back(cameraInfo.deviceName().toStdString());
        ui->ir_sensor_combo_box->addItem(cameraInfo.description());
    }

    const auto current_device = Settings::values.ir_sensor_device.GetValue();

    const auto devices_it = std::find_if(
        input_devices.begin(), input_devices.end(),
        [current_device](const std::string& device) { return device == current_device; });
    const int device_index =
        devices_it != input_devices.end()
            ? static_cast<int>(std::distance(input_devices.begin(), devices_it))
            : 0;
    ui->ir_sensor_combo_box->setCurrentIndex(device_index);
}

void ConfigureCamera::RestoreDefaults() {
    ui->ir_sensor_combo_box->setCurrentIndex(0);
}
