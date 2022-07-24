// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
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
class ConfigureAudio;
}

class ConfigureAudio : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureAudio(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureAudio() override;

    void ApplyConfiguration();
    void SetConfiguration();

private:
    void changeEvent(QEvent* event) override;

    void InitializeAudioSinkComboBox();

    void RetranslateUI();

    void UpdateAudioDevices(int sink_index);

    void SetOutputSinkFromSinkID();
    void SetAudioDevicesFromDeviceID();
    void SetVolumeIndicatorText(int percentage);

    void SetupPerGameUI();

    std::unique_ptr<Ui::ConfigureAudio> ui;

    const Core::System& system;
};
