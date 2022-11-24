// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>
#include <QString>
#include <QWidget>
#include "common/settings.h"

namespace Core {
class System;
}

namespace ConfigurationShared {
enum class CheckState;
}

namespace Ui {
class ConfigureGraphics;
}

class ConfigureGraphics : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGraphics(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureGraphics() override;

    void ApplyConfiguration();
    void SetConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void UpdateBackgroundColorButton(QColor color);
    void UpdateAPILayout();
    void UpdateDeviceSelection(int device);
    void UpdateShaderBackendSelection(int backend);

    void RetrieveVulkanDevices();

    void SetFSRIndicatorText(int percentage);

    void SetupPerGameUI();

    Settings::RendererBackend GetCurrentGraphicsBackend() const;
    Settings::NvdecEmulation GetCurrentNvdecEmulation() const;

    std::unique_ptr<Ui::ConfigureGraphics> ui;
    QColor bg_color;

    ConfigurationShared::CheckState use_nvdec_emulation;
    ConfigurationShared::CheckState accelerate_astc;
    ConfigurationShared::CheckState use_disk_shader_cache;
    ConfigurationShared::CheckState use_asynchronous_gpu_emulation;

    std::vector<QString> vulkan_devices;
    u32 vulkan_device{};
    Settings::ShaderBackend shader_backend{};

    const Core::System& system;
};
