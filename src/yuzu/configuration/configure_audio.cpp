// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "audio_core/sink/sink.h"
#include "audio_core/sink/sink_details.h"
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_audio.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_audio.h"
#include "yuzu/uisettings.h"

ConfigureAudio::ConfigureAudio(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureAudio>()), system{system_} {
    ui->setupUi(this);

    InitializeAudioSinkComboBox();

    connect(ui->volume_slider, &QSlider::valueChanged, this,
            &ConfigureAudio::SetVolumeIndicatorText);
    connect(ui->sink_combo_box, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureAudio::UpdateAudioDevices);

    ui->volume_label->setVisible(Settings::IsConfiguringGlobal());
    ui->volume_combo_box->setVisible(!Settings::IsConfiguringGlobal());

    SetupPerGameUI();

    SetConfiguration();

    const bool is_powered_on = system_.IsPoweredOn();
    ui->sink_combo_box->setEnabled(!is_powered_on);
    ui->output_combo_box->setEnabled(!is_powered_on);
    ui->input_combo_box->setEnabled(!is_powered_on);
}

ConfigureAudio::~ConfigureAudio() = default;

void ConfigureAudio::SetConfiguration() {
    SetOutputSinkFromSinkID();

    // The device list cannot be pre-populated (nor listed) until the output sink is known.
    UpdateAudioDevices(ui->sink_combo_box->currentIndex());

    SetAudioDevicesFromDeviceID();

    const auto volume_value = static_cast<int>(Settings::values.volume.GetValue());
    ui->volume_slider->setValue(volume_value);
    ui->toggle_background_mute->setChecked(UISettings::values.mute_when_in_background.GetValue());

    if (!Settings::IsConfiguringGlobal()) {
        if (Settings::values.volume.UsingGlobal()) {
            ui->volume_combo_box->setCurrentIndex(0);
            ui->volume_slider->setEnabled(false);
            ui->combo_sound->setCurrentIndex(Settings::values.sound_index.GetValue());
        } else {
            ui->volume_combo_box->setCurrentIndex(1);
            ui->volume_slider->setEnabled(true);
            ConfigurationShared::SetPerGameSetting(ui->combo_sound, &Settings::values.sound_index);
        }
        ConfigurationShared::SetHighlight(ui->volume_layout,
                                          !Settings::values.volume.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->mode_label,
                                          !Settings::values.sound_index.UsingGlobal());
    }
    SetVolumeIndicatorText(ui->volume_slider->sliderPosition());
}

void ConfigureAudio::SetOutputSinkFromSinkID() {
    [[maybe_unused]] const QSignalBlocker blocker(ui->sink_combo_box);

    int new_sink_index = 0;
    const QString sink_id = QString::fromStdString(Settings::values.sink_id.GetValue());
    for (int index = 0; index < ui->sink_combo_box->count(); index++) {
        if (ui->sink_combo_box->itemText(index) == sink_id) {
            new_sink_index = index;
            break;
        }
    }

    ui->sink_combo_box->setCurrentIndex(new_sink_index);
}

void ConfigureAudio::SetAudioDevicesFromDeviceID() {
    int new_device_index = -1;

    const QString output_device_id =
        QString::fromStdString(Settings::values.audio_output_device_id.GetValue());
    for (int index = 0; index < ui->output_combo_box->count(); index++) {
        if (ui->output_combo_box->itemText(index) == output_device_id) {
            new_device_index = index;
            break;
        }
    }

    ui->output_combo_box->setCurrentIndex(new_device_index);

    new_device_index = -1;
    const QString input_device_id =
        QString::fromStdString(Settings::values.audio_input_device_id.GetValue());
    for (int index = 0; index < ui->input_combo_box->count(); index++) {
        if (ui->input_combo_box->itemText(index) == input_device_id) {
            new_device_index = index;
            break;
        }
    }

    ui->input_combo_box->setCurrentIndex(new_device_index);
}

void ConfigureAudio::SetVolumeIndicatorText(int percentage) {
    ui->volume_indicator->setText(tr("%1%", "Volume percentage (e.g. 50%)").arg(percentage));
}

void ConfigureAudio::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.sound_index, ui->combo_sound);

    if (Settings::IsConfiguringGlobal()) {
        Settings::values.sink_id =
            ui->sink_combo_box->itemText(ui->sink_combo_box->currentIndex()).toStdString();
        Settings::values.audio_output_device_id.SetValue(
            ui->output_combo_box->itemText(ui->output_combo_box->currentIndex()).toStdString());
        Settings::values.audio_input_device_id.SetValue(
            ui->input_combo_box->itemText(ui->input_combo_box->currentIndex()).toStdString());
        UISettings::values.mute_when_in_background = ui->toggle_background_mute->isChecked();

        // Guard if during game and set to game-specific value
        if (Settings::values.volume.UsingGlobal()) {
            const auto volume = static_cast<u8>(ui->volume_slider->value());
            Settings::values.volume.SetValue(volume);
        }
    } else {
        if (ui->volume_combo_box->currentIndex() == 0) {
            Settings::values.volume.SetGlobal(true);
        } else {
            Settings::values.volume.SetGlobal(false);
            const auto volume = static_cast<u8>(ui->volume_slider->value());
            Settings::values.volume.SetValue(volume);
        }
    }
}

void ConfigureAudio::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureAudio::UpdateAudioDevices(int sink_index) {
    ui->output_combo_box->clear();
    ui->output_combo_box->addItem(QString::fromUtf8(AudioCore::Sink::auto_device_name));

    const std::string sink_id = ui->sink_combo_box->itemText(sink_index).toStdString();
    for (const auto& device : AudioCore::Sink::GetDeviceListForSink(sink_id, false)) {
        ui->output_combo_box->addItem(QString::fromStdString(device));
    }

    ui->input_combo_box->clear();
    ui->input_combo_box->addItem(QString::fromUtf8(AudioCore::Sink::auto_device_name));
    for (const auto& device : AudioCore::Sink::GetDeviceListForSink(sink_id, true)) {
        ui->input_combo_box->addItem(QString::fromStdString(device));
    }
}

void ConfigureAudio::InitializeAudioSinkComboBox() {
    ui->sink_combo_box->clear();
    ui->sink_combo_box->addItem(QString::fromUtf8(AudioCore::Sink::auto_device_name));

    for (const auto& id : AudioCore::Sink::GetSinkIDs()) {
        ui->sink_combo_box->addItem(QString::fromUtf8(id.data(), static_cast<s32>(id.length())));
    }
}

void ConfigureAudio::RetranslateUI() {
    ui->retranslateUi(this);
    SetVolumeIndicatorText(ui->volume_slider->sliderPosition());
}

void ConfigureAudio::SetupPerGameUI() {
    if (Settings::IsConfiguringGlobal()) {
        ui->volume_slider->setEnabled(Settings::values.volume.UsingGlobal());
        // ui->combo_sound->setEnabled(Settings::values.sound_index.UsingGlobal());

        return;
    }

    // ConfigurationShared::SetColoredComboBox(ui->combo_sound, ui->label_sound,
    //                                        Settings::values.sound_index.GetValue(true));

    connect(ui->volume_combo_box, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        ui->volume_slider->setEnabled(index == 1);
        ConfigurationShared::SetHighlight(ui->volume_layout, index == 1);
    });

    ui->sink_combo_box->setVisible(false);
    ui->sink_label->setVisible(false);
    ui->output_combo_box->setVisible(false);
    ui->output_label->setVisible(false);
    ui->input_combo_box->setVisible(false);
    ui->input_label->setVisible(false);
}
