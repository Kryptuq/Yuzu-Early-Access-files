// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

    void InitializeAudioOutputSinkComboBox();

    void RetranslateUI();

    void UpdateAudioDevices(int sink_index);

    void SetOutputSinkFromSinkID();
    void SetAudioDeviceFromDeviceID();
    void SetVolumeIndicatorText(int percentage);

    void SetupPerGameUI();

    std::unique_ptr<Ui::ConfigureAudio> ui;

    const Core::System& system;
};
