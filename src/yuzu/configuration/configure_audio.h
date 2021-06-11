// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace ConfigurationShared {
enum class CheckState;
}

namespace Ui {
class ConfigureAudio;
}

class ConfigureAudio : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureAudio(QWidget* parent = nullptr);
    ~ConfigureAudio() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;

    void InitializeAudioOutputSinkComboBox();

    void RetranslateUI();

    void UpdateAudioDevices(int sink_index);

    void SetConfiguration();
    void SetOutputSinkFromSinkID();
    void SetAudioDeviceFromDeviceID();
    void SetVolumeIndicatorText(int percentage);

    void SetupPerGameUI();

    std::unique_ptr<Ui::ConfigureAudio> ui;

    ConfigurationShared::CheckState enable_audio_stretching;
};
