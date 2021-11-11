// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <memory>
#include <utility>
#include <QGridLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include "common/param_package.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "input_common/drivers/keyboard.h"
#include "input_common/drivers/mouse.h"
#include "input_common/main.h"
#include "ui_configure_input_player.h"
#include "yuzu/bootmanager.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/configure_input_player.h"
#include "yuzu/configuration/configure_input_player_widget.h"
#include "yuzu/configuration/configure_vibration.h"
#include "yuzu/configuration/input_profiles.h"
#include "yuzu/util/limitable_input_dialog.h"

const std::array<std::string, ConfigureInputPlayer::ANALOG_SUB_BUTTONS_NUM>
    ConfigureInputPlayer::analog_sub_buttons{{
        "up",
        "down",
        "left",
        "right",
    }};

namespace {

QString GetKeyName(int key_code) {
    switch (key_code) {
    case Qt::Key_Shift:
        return QObject::tr("Shift");
    case Qt::Key_Control:
        return QObject::tr("Ctrl");
    case Qt::Key_Alt:
        return QObject::tr("Alt");
    case Qt::Key_Meta:
        return {};
    default:
        return QKeySequence(key_code).toString();
    }
}

void SetAnalogParam(const Common::ParamPackage& input_param, Common::ParamPackage& analog_param,
                    const std::string& button_name) {
    // The poller returned a complete axis, so set all the buttons
    if (input_param.Has("axis_x") && input_param.Has("axis_y")) {
        analog_param = input_param;
        return;
    }
    // Check if the current configuration has either no engine or an axis binding.
    // Clears out the old binding and adds one with analog_from_button.
    if (!analog_param.Has("engine") || analog_param.Has("axis_x") || analog_param.Has("axis_y")) {
        analog_param = {
            {"engine", "analog_from_button"},
        };
    }
    analog_param.Set(button_name, input_param.Serialize());
}
} // namespace

QString ConfigureInputPlayer::ButtonToText(const Common::ParamPackage& param) {
    if (!param.Has("engine")) {
        return QObject::tr("[not set]");
    }

    // Retrieve the names from Qt
    if (param.Get("engine", "") == "keyboard") {
        const QString button_str = GetKeyName(param.Get("code", 0));
        const QString toggle = QString::fromStdString(param.Get("toggle", false) ? "~" : "");
        return QObject::tr("%1%2").arg(toggle, button_str);
    }

    std::string button_name = input_subsystem->GetButtonName(param);
    return QString::fromStdString(button_name);
}

QString ConfigureInputPlayer::AnalogToText(const Common::ParamPackage& param,
                                           const std::string& dir) {
    if (!param.Has("engine")) {
        return QObject::tr("[not set]");
    }

    if (param.Get("engine", "") == "analog_from_button") {
        return ButtonToText(Common::ParamPackage{param.Get(dir, "")});
    }

    if (!param.Has("axis_x") || !param.Has("axis_y")) {
        return QObject::tr("[unknown]");
    }

    const auto engine_str = param.Get("engine", "");
    const QString axis_x_str = QString::fromStdString(param.Get("axis_x", ""));
    const QString axis_y_str = QString::fromStdString(param.Get("axis_y", ""));
    const bool invert_x = param.Get("invert_x", "+") == "-";
    const bool invert_y = param.Get("invert_y", "+") == "-";

    if (dir == "modifier") {
        return QObject::tr("[unused]");
    }

    if (dir == "left") {
        const QString invert_x_str = QString::fromStdString(invert_x ? "+" : "-");
        return QObject::tr("Axis %1%2").arg(axis_x_str, invert_x_str);
    }
    if (dir == "right") {
        const QString invert_x_str = QString::fromStdString(invert_x ? "-" : "+");
        return QObject::tr("Axis %1%2").arg(axis_x_str, invert_x_str);
    }
    if (dir == "up") {
        const QString invert_y_str = QString::fromStdString(invert_y ? "-" : "+");
        return QObject::tr("Axis %1%2").arg(axis_y_str, invert_y_str);
    }
    if (dir == "down") {
        const QString invert_y_str = QString::fromStdString(invert_y ? "+" : "-");
        return QObject::tr("Axis %1%2").arg(axis_y_str, invert_y_str);
    }

    return QObject::tr("[unknown]");
}

ConfigureInputPlayer::ConfigureInputPlayer(QWidget* parent, std::size_t player_index,
                                           QWidget* bottom_row,
                                           InputCommon::InputSubsystem* input_subsystem_,
                                           InputProfiles* profiles_, Core::HID::HIDCore& hid_core_,
                                           bool is_powered_on_, bool debug)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureInputPlayer>()), player_index(player_index),
      debug(debug), is_powered_on{is_powered_on_}, input_subsystem{input_subsystem_},
      profiles(profiles_), timeout_timer(std::make_unique<QTimer>()),
      poll_timer(std::make_unique<QTimer>()), bottom_row(bottom_row), hid_core{hid_core_} {
    if (player_index == 0) {
        auto* emulated_controller_p1 =
            hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
        auto* emulated_controller_hanheld =
            hid_core.GetEmulatedController(Core::HID::NpadIdType::Handheld);
        emulated_controller_p1->SaveCurrentConfig();
        emulated_controller_p1->EnableConfiguration();
        emulated_controller_hanheld->SaveCurrentConfig();
        emulated_controller_hanheld->EnableConfiguration();
        if (emulated_controller_hanheld->IsConnected(true)) {
            emulated_controller_p1->Disconnect();
            emulated_controller = emulated_controller_hanheld;
        } else {
            emulated_controller = emulated_controller_p1;
        }
    } else {
        emulated_controller = hid_core.GetEmulatedControllerByIndex(player_index);
        emulated_controller->SaveCurrentConfig();
        emulated_controller->EnableConfiguration();
    }
    ui->setupUi(this);

    setFocusPolicy(Qt::ClickFocus);

    button_map = {
        ui->buttonA,        ui->buttonB,      ui->buttonX,         ui->buttonY,
        ui->buttonLStick,   ui->buttonRStick, ui->buttonL,         ui->buttonR,
        ui->buttonZL,       ui->buttonZR,     ui->buttonPlus,      ui->buttonMinus,
        ui->buttonDpadLeft, ui->buttonDpadUp, ui->buttonDpadRight, ui->buttonDpadDown,
        ui->buttonSL,       ui->buttonSR,     ui->buttonHome,      ui->buttonScreenshot,
    };

    analog_map_buttons = {{
        {
            ui->buttonLStickUp,
            ui->buttonLStickDown,
            ui->buttonLStickLeft,
            ui->buttonLStickRight,
        },
        {
            ui->buttonRStickUp,
            ui->buttonRStickDown,
            ui->buttonRStickLeft,
            ui->buttonRStickRight,
        },
    }};

    motion_map = {
        ui->buttonMotionLeft,
        ui->buttonMotionRight,
    };

    analog_map_deadzone_label = {ui->labelLStickDeadzone, ui->labelRStickDeadzone};
    analog_map_deadzone_slider = {ui->sliderLStickDeadzone, ui->sliderRStickDeadzone};
    analog_map_modifier_groupbox = {ui->buttonLStickModGroup, ui->buttonRStickModGroup};
    analog_map_modifier_button = {ui->buttonLStickMod, ui->buttonRStickMod};
    analog_map_modifier_label = {ui->labelLStickModifierRange, ui->labelRStickModifierRange};
    analog_map_modifier_slider = {ui->sliderLStickModifierRange, ui->sliderRStickModifierRange};
    analog_map_range_groupbox = {ui->buttonLStickRangeGroup, ui->buttonRStickRangeGroup};
    analog_map_range_spinbox = {ui->spinboxLStickRange, ui->spinboxRStickRange};

    ui->controllerFrame->SetController(emulated_controller);

    for (int button_id = 0; button_id < Settings::NativeButton::NumButtons; ++button_id) {
        auto* const button = button_map[button_id];

        if (button == nullptr) {
            continue;
        }

        connect(button, &QPushButton::clicked, [=, this] {
            HandleClick(
                button, button_id,
                [=, this](Common::ParamPackage params) {
                    emulated_controller->SetButtonParam(button_id, params);
                },
                InputCommon::Polling::InputType::Button);
        });

        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QPushButton::customContextMenuRequested,
                [=, this](const QPoint& menu_location) {
                    QMenu context_menu;
                    Common::ParamPackage param = emulated_controller->GetButtonParam(button_id);
                    context_menu.addAction(tr("Clear"), [&] {
                        emulated_controller->SetButtonParam(button_id, {});
                        button_map[button_id]->setText(tr("[not set]"));
                    });
                    if (param.Has("button") || param.Has("hat")) {
                        context_menu.addAction(tr("Toggle button"), [&] {
                            const bool toggle_value = !param.Get("toggle", false);
                            param.Set("toggle", toggle_value);
                            button_map[button_id]->setText(ButtonToText(param));
                            emulated_controller->SetButtonParam(button_id, param);
                        });
                        context_menu.addAction(tr("Invert button"), [&] {
                            const bool toggle_value = !param.Get("inverted", false);
                            param.Set("inverted", toggle_value);
                            button_map[button_id]->setText(ButtonToText(param));
                            emulated_controller->SetButtonParam(button_id, param);
                        });
                    }
                    if (param.Has("axis")) {
                        context_menu.addAction(tr("Invert axis"), [&] {
                            const bool toggle_value = !(param.Get("invert", "+") == "-");
                            param.Set("invert", toggle_value ? "-" : "+");
                            button_map[button_id]->setText(ButtonToText(param));
                            emulated_controller->SetButtonParam(button_id, param);
                        });
                        context_menu.addAction(tr("Set threshold"), [&] {
                            const int button_threshold =
                                static_cast<int>(param.Get("threshold", 0.5f) * 100.0f);
                            const int new_threshold = QInputDialog::getInt(
                                this, tr("Set threshold"), tr("Choose a value between 0% and 100%"),
                                button_threshold, 0, 100);
                            param.Set("threshold", new_threshold / 100.0f);

                            if (button_id == Settings::NativeButton::ZL) {
                                ui->sliderZLThreshold->setValue(new_threshold);
                            }
                            if (button_id == Settings::NativeButton::ZR) {
                                ui->sliderZRThreshold->setValue(new_threshold);
                            }
                            emulated_controller->SetButtonParam(button_id, param);
                        });
                    }
                    context_menu.exec(button_map[button_id]->mapToGlobal(menu_location));
                });
    }

    for (int motion_id = 0; motion_id < Settings::NativeMotion::NumMotions; ++motion_id) {
        auto* const button = motion_map[motion_id];
        if (button == nullptr) {
            continue;
        }

        connect(button, &QPushButton::clicked, [=, this] {
            HandleClick(
                button, motion_id,
                [=, this](Common::ParamPackage params) {
                    emulated_controller->SetMotionParam(motion_id, params);
                },
                InputCommon::Polling::InputType::Motion);
        });

        button->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(button, &QPushButton::customContextMenuRequested,
                [=, this](const QPoint& menu_location) {
                    QMenu context_menu;
                    context_menu.addAction(tr("Clear"), [&] {
                        emulated_controller->SetMotionParam(motion_id, {});
                        motion_map[motion_id]->setText(tr("[not set]"));
                    });
                    context_menu.exec(motion_map[motion_id]->mapToGlobal(menu_location));
                });
    }

    connect(ui->sliderZLThreshold, &QSlider::valueChanged, [=, this] {
        Common::ParamPackage param =
            emulated_controller->GetButtonParam(Settings::NativeButton::ZL);
        if (param.Has("threshold")) {
            const auto slider_value = ui->sliderZLThreshold->value();
            param.Set("threshold", slider_value / 100.0f);
            emulated_controller->SetButtonParam(Settings::NativeButton::ZL, param);
        }
    });

    connect(ui->sliderZRThreshold, &QSlider::valueChanged, [=, this] {
        Common::ParamPackage param =
            emulated_controller->GetButtonParam(Settings::NativeButton::ZR);
        if (param.Has("threshold")) {
            const auto slider_value = ui->sliderZRThreshold->value();
            param.Set("threshold", slider_value / 100.0f);
            emulated_controller->SetButtonParam(Settings::NativeButton::ZR, param);
        }
    });

    for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; ++analog_id) {
        for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; ++sub_button_id) {
            auto* const analog_button = analog_map_buttons[analog_id][sub_button_id];

            if (analog_button == nullptr) {
                continue;
            }

            connect(analog_button, &QPushButton::clicked, [=, this] {
                if (!map_analog_stick_accepted) {
                    map_analog_stick_accepted =
                        QMessageBox::information(
                            this, tr("Map Analog Stick"),
                            tr("After pressing OK, first move your joystick horizontally, and then "
                               "vertically.\nTo invert the axes, first move your joystick "
                               "vertically, and then horizontally."),
                            QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok;
                    if (!map_analog_stick_accepted) {
                        return;
                    }
                }
                HandleClick(
                    analog_map_buttons[analog_id][sub_button_id], analog_id,
                    [=, this](const Common::ParamPackage& params) {
                        Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
                        SetAnalogParam(params, param, analog_sub_buttons[sub_button_id]);
                        emulated_controller->SetStickParam(analog_id, param);
                    },
                    InputCommon::Polling::InputType::Stick);
            });

            analog_button->setContextMenuPolicy(Qt::CustomContextMenu);

            connect(analog_button, &QPushButton::customContextMenuRequested,
                    [=, this](const QPoint& menu_location) {
                        QMenu context_menu;
                        Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
                        context_menu.addAction(tr("Clear"), [&] {
                            emulated_controller->SetStickParam(analog_id, {});
                            analog_map_buttons[analog_id][sub_button_id]->setText(tr("[not set]"));
                        });
                        context_menu.addAction(tr("Invert axis"), [&] {
                            if (sub_button_id == 2 || sub_button_id == 3) {
                                const bool invert_value = param.Get("invert_x", "+") == "-";
                                const std::string invert_str = invert_value ? "+" : "-";
                                param.Set("invert_x", invert_str);
                                emulated_controller->SetStickParam(analog_id, param);
                            }
                            if (sub_button_id == 0 || sub_button_id == 1) {
                                const bool invert_value = param.Get("invert_y", "+") == "-";
                                const std::string invert_str = invert_value ? "+" : "-";
                                param.Set("invert_y", invert_str);
                                emulated_controller->SetStickParam(analog_id, param);
                            }
                            for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM;
                                 ++sub_button_id) {
                                analog_map_buttons[analog_id][sub_button_id]->setText(
                                    AnalogToText(param, analog_sub_buttons[sub_button_id]));
                            }
                        });
                        context_menu.exec(analog_map_buttons[analog_id][sub_button_id]->mapToGlobal(
                            menu_location));
                    });
        }

        // Handle clicks for the modifier buttons as well.
        connect(analog_map_modifier_button[analog_id], &QPushButton::clicked, [=, this] {
            HandleClick(
                analog_map_modifier_button[analog_id], analog_id,
                [=, this](const Common::ParamPackage& params) {
                    Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
                    param.Set("modifier", params.Serialize());
                    emulated_controller->SetStickParam(analog_id, param);
                },
                InputCommon::Polling::InputType::Button);
        });

        analog_map_modifier_button[analog_id]->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(analog_map_modifier_button[analog_id], &QPushButton::customContextMenuRequested,
                [=, this](const QPoint& menu_location) {
                    QMenu context_menu;
                    Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
                    context_menu.addAction(tr("Clear"), [&] {
                        param.Set("modifier", "");
                        analog_map_modifier_button[analog_id]->setText(tr("[not set]"));
                        emulated_controller->SetStickParam(analog_id, param);
                    });
                    context_menu.addAction(tr("Toggle button"), [&] {
                        Common::ParamPackage modifier_param =
                            Common::ParamPackage{param.Get("modifier", "")};
                        const bool toggle_value = !modifier_param.Get("toggle", false);
                        modifier_param.Set("toggle", toggle_value);
                        param.Set("modifier", modifier_param.Serialize());
                        analog_map_modifier_button[analog_id]->setText(
                            ButtonToText(modifier_param));
                        emulated_controller->SetStickParam(analog_id, param);
                    });
                    context_menu.exec(
                        analog_map_modifier_button[analog_id]->mapToGlobal(menu_location));
                });

        connect(analog_map_range_spinbox[analog_id], qOverload<int>(&QSpinBox::valueChanged),
                [=, this] {
                    Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
                    const auto spinbox_value = analog_map_range_spinbox[analog_id]->value();
                    param.Set("range", spinbox_value / 100.0f);
                    emulated_controller->SetStickParam(analog_id, param);
                });

        connect(analog_map_deadzone_slider[analog_id], &QSlider::valueChanged, [=, this] {
            Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
            const auto slider_value = analog_map_deadzone_slider[analog_id]->value();
            analog_map_deadzone_label[analog_id]->setText(tr("Deadzone: %1%").arg(slider_value));
            param.Set("deadzone", slider_value / 100.0f);
            emulated_controller->SetStickParam(analog_id, param);
        });

        connect(analog_map_modifier_slider[analog_id], &QSlider::valueChanged, [=, this] {
            Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
            const auto slider_value = analog_map_modifier_slider[analog_id]->value();
            analog_map_modifier_label[analog_id]->setText(
                tr("Modifier Range: %1%").arg(slider_value));
            param.Set("modifier_scale", slider_value / 100.0f);
            emulated_controller->SetStickParam(analog_id, param);
        });
    }

    // Player Connected checkbox
    connect(ui->groupConnectedController, &QGroupBox::toggled,
            [this](bool checked) { emit Connected(checked); });

    if (player_index == 0) {
        connect(ui->comboControllerType, qOverload<int>(&QComboBox::currentIndexChanged),
                [this](int index) {
                    emit HandheldStateChanged(GetControllerTypeFromIndex(index) ==
                                              Core::HID::NpadStyleIndex::Handheld);
                });
    }

    if (debug || player_index == 9) {
        ui->groupConnectedController->setCheckable(false);
    }

    // The Debug Controller can only choose the Pro Controller.
    if (debug) {
        ui->buttonScreenshot->setEnabled(false);
        ui->buttonHome->setEnabled(false);
        ui->comboControllerType->addItem(tr("Pro Controller"));
    } else {
        SetConnectableControllers();
    }

    UpdateControllerAvailableButtons();
    UpdateControllerEnabledButtons();
    UpdateControllerButtonNames();
    UpdateMotionButtons();
    connect(ui->comboControllerType, qOverload<int>(&QComboBox::currentIndexChanged),
            [this, player_index](int) {
                UpdateControllerAvailableButtons();
                UpdateControllerEnabledButtons();
                UpdateControllerButtonNames();
                UpdateMotionButtons();
                const Core::HID::NpadStyleIndex type =
                    GetControllerTypeFromIndex(ui->comboControllerType->currentIndex());

                if (player_index == 0) {
                    auto* emulated_controller_p1 =
                        hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
                    auto* emulated_controller_hanheld =
                        hid_core.GetEmulatedController(Core::HID::NpadIdType::Handheld);
                    bool is_connected = emulated_controller->IsConnected(true);

                    emulated_controller_p1->SetNpadStyleIndex(type);
                    emulated_controller_hanheld->SetNpadStyleIndex(type);
                    if (is_connected) {
                        if (type == Core::HID::NpadStyleIndex::Handheld) {
                            emulated_controller_p1->Disconnect();
                            emulated_controller_hanheld->Connect();
                            emulated_controller = emulated_controller_hanheld;
                        } else {
                            emulated_controller_hanheld->Disconnect();
                            emulated_controller_p1->Connect();
                            emulated_controller = emulated_controller_p1;
                        }
                    }
                    ui->controllerFrame->SetController(emulated_controller);
                }
                emulated_controller->SetNpadStyleIndex(type);
            });

    connect(ui->comboDevices, qOverload<int>(&QComboBox::activated), this,
            &ConfigureInputPlayer::UpdateMappingWithDefaults);

    ui->comboDevices->setCurrentIndex(-1);

    ui->buttonRefreshDevices->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    connect(ui->buttonRefreshDevices, &QPushButton::clicked,
            [this] { emit RefreshInputDevices(); });

    timeout_timer->setSingleShot(true);
    connect(timeout_timer.get(), &QTimer::timeout, [this] { SetPollingResult({}, true); });

    connect(poll_timer.get(), &QTimer::timeout, [this] {
        const auto& params = input_subsystem->GetNextInput();
        if (params.Has("engine") && IsInputAcceptable(params)) {
            SetPollingResult(params, false);
            return;
        }
    });

    UpdateInputProfiles();

    connect(ui->buttonProfilesNew, &QPushButton::clicked, this,
            &ConfigureInputPlayer::CreateProfile);
    connect(ui->buttonProfilesDelete, &QPushButton::clicked, this,
            &ConfigureInputPlayer::DeleteProfile);
    connect(ui->comboProfiles, qOverload<int>(&QComboBox::activated), this,
            &ConfigureInputPlayer::LoadProfile);
    connect(ui->buttonProfilesSave, &QPushButton::clicked, this,
            &ConfigureInputPlayer::SaveProfile);

    LoadConfiguration();
}

ConfigureInputPlayer::~ConfigureInputPlayer() {
    if (player_index == 0) {
        auto* emulated_controller_p1 =
            hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
        auto* emulated_controller_hanheld =
            hid_core.GetEmulatedController(Core::HID::NpadIdType::Handheld);
        emulated_controller_p1->DisableConfiguration();
        emulated_controller_hanheld->DisableConfiguration();
    } else {
        emulated_controller->DisableConfiguration();
    }
}

void ConfigureInputPlayer::ApplyConfiguration() {
    if (player_index == 0) {
        auto* emulated_controller_p1 =
            hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
        auto* emulated_controller_hanheld =
            hid_core.GetEmulatedController(Core::HID::NpadIdType::Handheld);
        emulated_controller_p1->DisableConfiguration();
        emulated_controller_p1->SaveCurrentConfig();
        emulated_controller_p1->EnableConfiguration();
        emulated_controller_hanheld->DisableConfiguration();
        emulated_controller_hanheld->SaveCurrentConfig();
        emulated_controller_hanheld->EnableConfiguration();
        return;
    }
    emulated_controller->DisableConfiguration();
    emulated_controller->SaveCurrentConfig();
    emulated_controller->EnableConfiguration();
}

void ConfigureInputPlayer::showEvent(QShowEvent* event) {
    if (bottom_row == nullptr) {
        return;
    }
    QWidget::showEvent(event);
    ui->main->addWidget(bottom_row);
}

void ConfigureInputPlayer::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureInputPlayer::RetranslateUI() {
    ui->retranslateUi(this);
    UpdateUI();
}

void ConfigureInputPlayer::LoadConfiguration() {
    emulated_controller->ReloadFromSettings();

    UpdateUI();
    UpdateInputDeviceCombobox();

    if (debug) {
        return;
    }

    const int comboBoxIndex =
        GetIndexFromControllerType(emulated_controller->GetNpadStyleIndex(true));
    ui->comboControllerType->setCurrentIndex(comboBoxIndex);
    ui->groupConnectedController->setChecked(emulated_controller->IsConnected(true));
}

void ConfigureInputPlayer::ConnectPlayer(bool connected) {
    ui->groupConnectedController->setChecked(connected);
    if (connected) {
        emulated_controller->Connect();
    } else {
        emulated_controller->Disconnect();
    }
}

void ConfigureInputPlayer::UpdateInputDeviceCombobox() {
    // Skip input device persistence if "Input Devices" is set to "Any".
    if (ui->comboDevices->currentIndex() == 0) {
        UpdateInputDevices();
        return;
    }

    const auto devices =
        emulated_controller->GetMappedDevices(Core::HID::EmulatedDeviceIndex::AllDevices);
    UpdateInputDevices();

    if (devices.empty()) {
        return;
    }

    if (devices.size() > 2) {
        ui->comboDevices->setCurrentIndex(0);
        return;
    }

    const auto first_engine = devices[0].Get("engine", "");
    const auto first_guid = devices[0].Get("guid", "");
    const auto first_port = devices[0].Get("port", 0);

    if (devices.size() == 1) {
        const auto devices_it =
            std::find_if(input_devices.begin(), input_devices.end(),
                         [first_engine, first_guid, first_port](const Common::ParamPackage param) {
                             return param.Get("engine", "") == first_engine &&
                                    param.Get("guid", "") == first_guid &&
                                    param.Get("port", 0) == first_port;
                         });
        const int device_index =
            devices_it != input_devices.end()
                ? static_cast<int>(std::distance(input_devices.begin(), devices_it))
                : 0;
        ui->comboDevices->setCurrentIndex(device_index);
        return;
    }

    const auto second_engine = devices[1].Get("engine", "");
    const auto second_guid = devices[1].Get("guid", "");
    const auto second_port = devices[1].Get("port", 0);

    const bool is_keyboard_mouse = (first_engine == "keyboard" || first_engine == "mouse") &&
                                   (second_engine == "keyboard" || second_engine == "mouse");

    if (is_keyboard_mouse) {
        ui->comboDevices->setCurrentIndex(2);
        return;
    }

    const bool is_engine_equal = first_engine == second_engine;
    const bool is_port_equal = first_port == second_port;

    if (is_engine_equal && is_port_equal) {
        const auto devices_it = std::find_if(
            input_devices.begin(), input_devices.end(),
            [first_engine, first_guid, second_guid, first_port](const Common::ParamPackage param) {
                const bool is_guid_valid =
                    (param.Get("guid", "") == first_guid &&
                     param.Get("guid2", "") == second_guid) ||
                    (param.Get("guid", "") == second_guid && param.Get("guid2", "") == first_guid);
                return param.Get("engine", "") == first_engine && is_guid_valid &&
                       param.Get("port", 0) == first_port;
            });
        const int device_index =
            devices_it != input_devices.end()
                ? static_cast<int>(std::distance(input_devices.begin(), devices_it))
                : 0;
        ui->comboDevices->setCurrentIndex(device_index);
    } else {
        ui->comboDevices->setCurrentIndex(0);
    }
}

void ConfigureInputPlayer::RestoreDefaults() {
    UpdateMappingWithDefaults();
}

void ConfigureInputPlayer::ClearAll() {
    for (int button_id = 0; button_id < Settings::NativeButton::NumButtons; ++button_id) {
        const auto* const button = button_map[button_id];
        if (button == nullptr) {
            continue;
        }
        emulated_controller->SetButtonParam(button_id, {});
    }

    for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; ++analog_id) {
        for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; ++sub_button_id) {
            const auto* const analog_button = analog_map_buttons[analog_id][sub_button_id];
            if (analog_button == nullptr) {
                continue;
            }
            emulated_controller->SetStickParam(analog_id, {});
        }
    }

    for (int motion_id = 0; motion_id < Settings::NativeMotion::NumMotions; ++motion_id) {
        const auto* const motion_button = motion_map[motion_id];
        if (motion_button == nullptr) {
            continue;
        }
        emulated_controller->SetMotionParam(motion_id, {});
    }

    UpdateUI();
    UpdateInputDevices();
}

void ConfigureInputPlayer::UpdateUI() {
    for (int button = 0; button < Settings::NativeButton::NumButtons; ++button) {
        const Common::ParamPackage param = emulated_controller->GetButtonParam(button);
        button_map[button]->setText(ButtonToText(param));
    }

    const Common::ParamPackage ZL_param =
        emulated_controller->GetButtonParam(Settings::NativeButton::ZL);
    if (ZL_param.Has("threshold")) {
        const int button_threshold = static_cast<int>(ZL_param.Get("threshold", 0.5f) * 100.0f);
        ui->sliderZLThreshold->setValue(button_threshold);
    }

    const Common::ParamPackage ZR_param =
        emulated_controller->GetButtonParam(Settings::NativeButton::ZR);
    if (ZR_param.Has("threshold")) {
        const int button_threshold = static_cast<int>(ZR_param.Get("threshold", 0.5f) * 100.0f);
        ui->sliderZRThreshold->setValue(button_threshold);
    }

    for (int motion_id = 0; motion_id < Settings::NativeMotion::NumMotions; ++motion_id) {
        const Common::ParamPackage param = emulated_controller->GetMotionParam(motion_id);
        motion_map[motion_id]->setText(ButtonToText(param));
    }

    for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; ++analog_id) {
        const Common::ParamPackage param = emulated_controller->GetStickParam(analog_id);
        for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; ++sub_button_id) {
            auto* const analog_button = analog_map_buttons[analog_id][sub_button_id];

            if (analog_button == nullptr) {
                continue;
            }

            analog_button->setText(AnalogToText(param, analog_sub_buttons[sub_button_id]));
        }

        analog_map_modifier_button[analog_id]->setText(
            ButtonToText(Common::ParamPackage{param.Get("modifier", "")}));

        const auto deadzone_label = analog_map_deadzone_label[analog_id];
        const auto deadzone_slider = analog_map_deadzone_slider[analog_id];
        const auto modifier_groupbox = analog_map_modifier_groupbox[analog_id];
        const auto modifier_label = analog_map_modifier_label[analog_id];
        const auto modifier_slider = analog_map_modifier_slider[analog_id];
        const auto range_groupbox = analog_map_range_groupbox[analog_id];
        const auto range_spinbox = analog_map_range_spinbox[analog_id];

        int slider_value;
        const bool is_controller = input_subsystem->IsController(param);

        if (is_controller) {
            slider_value = static_cast<int>(param.Get("deadzone", 0.15f) * 100);
            deadzone_label->setText(tr("Deadzone: %1%").arg(slider_value));
            deadzone_slider->setValue(slider_value);
            range_spinbox->setValue(static_cast<int>(param.Get("range", 1.0f) * 100));
        } else {
            slider_value = static_cast<int>(param.Get("modifier_scale", 0.5f) * 100);
            modifier_label->setText(tr("Modifier Range: %1%").arg(slider_value));
            modifier_slider->setValue(slider_value);
        }

        deadzone_label->setVisible(is_controller);
        deadzone_slider->setVisible(is_controller);
        modifier_groupbox->setVisible(!is_controller);
        modifier_label->setVisible(!is_controller);
        modifier_slider->setVisible(!is_controller);
        range_groupbox->setVisible(is_controller);
    }
}

void ConfigureInputPlayer::SetConnectableControllers() {
    const auto add_controllers = [this](bool enable_all,
                                        Core::HID::NpadStyleTag npad_style_set = {}) {
        index_controller_type_pairs.clear();
        ui->comboControllerType->clear();

        if (enable_all || npad_style_set.fullkey == 1) {
            index_controller_type_pairs.emplace_back(ui->comboControllerType->count(),
                                                     Core::HID::NpadStyleIndex::ProController);
            ui->comboControllerType->addItem(tr("Pro Controller"));
        }

        if (enable_all || npad_style_set.joycon_dual == 1) {
            index_controller_type_pairs.emplace_back(ui->comboControllerType->count(),
                                                     Core::HID::NpadStyleIndex::JoyconDual);
            ui->comboControllerType->addItem(tr("Dual Joycons"));
        }

        if (enable_all || npad_style_set.joycon_left == 1) {
            index_controller_type_pairs.emplace_back(ui->comboControllerType->count(),
                                                     Core::HID::NpadStyleIndex::JoyconLeft);
            ui->comboControllerType->addItem(tr("Left Joycon"));
        }

        if (enable_all || npad_style_set.joycon_right == 1) {
            index_controller_type_pairs.emplace_back(ui->comboControllerType->count(),
                                                     Core::HID::NpadStyleIndex::JoyconRight);
            ui->comboControllerType->addItem(tr("Right Joycon"));
        }

        if (player_index == 0 && (enable_all || npad_style_set.handheld == 1)) {
            index_controller_type_pairs.emplace_back(ui->comboControllerType->count(),
                                                     Core::HID::NpadStyleIndex::Handheld);
            ui->comboControllerType->addItem(tr("Handheld"));
        }

        if (enable_all || npad_style_set.gamecube == 1) {
            index_controller_type_pairs.emplace_back(ui->comboControllerType->count(),
                                                     Core::HID::NpadStyleIndex::GameCube);
            ui->comboControllerType->addItem(tr("GameCube Controller"));
        }
    };

    if (!is_powered_on) {
        add_controllers(true);
        return;
    }

    add_controllers(false, hid_core.GetSupportedStyleTag());
}

Core::HID::NpadStyleIndex ConfigureInputPlayer::GetControllerTypeFromIndex(int index) const {
    const auto it =
        std::find_if(index_controller_type_pairs.begin(), index_controller_type_pairs.end(),
                     [index](const auto& pair) { return pair.first == index; });

    if (it == index_controller_type_pairs.end()) {
        return Core::HID::NpadStyleIndex::ProController;
    }

    return it->second;
}

int ConfigureInputPlayer::GetIndexFromControllerType(Core::HID::NpadStyleIndex type) const {
    const auto it =
        std::find_if(index_controller_type_pairs.begin(), index_controller_type_pairs.end(),
                     [type](const auto& pair) { return pair.second == type; });

    if (it == index_controller_type_pairs.end()) {
        return -1;
    }

    return it->first;
}

void ConfigureInputPlayer::UpdateInputDevices() {
    input_devices = input_subsystem->GetInputDevices();
    ui->comboDevices->clear();
    for (auto device : input_devices) {
        ui->comboDevices->addItem(QString::fromStdString(device.Get("display", "Unknown")), {});
    }
}

void ConfigureInputPlayer::UpdateControllerAvailableButtons() {
    auto layout = GetControllerTypeFromIndex(ui->comboControllerType->currentIndex());
    if (debug) {
        layout = Core::HID::NpadStyleIndex::ProController;
    }

    // List of all the widgets that will be hidden by any of the following layouts that need
    // "unhidden" after the controller type changes
    const std::array<QWidget*, 11> layout_show = {
        ui->buttonShoulderButtonsSLSR,
        ui->horizontalSpacerShoulderButtonsWidget,
        ui->horizontalSpacerShoulderButtonsWidget2,
        ui->buttonShoulderButtonsLeft,
        ui->buttonMiscButtonsMinusScreenshot,
        ui->bottomLeft,
        ui->buttonShoulderButtonsRight,
        ui->buttonMiscButtonsPlusHome,
        ui->bottomRight,
        ui->buttonMiscButtonsMinusGroup,
        ui->buttonMiscButtonsScreenshotGroup,
    };

    for (auto* widget : layout_show) {
        widget->show();
    }

    std::vector<QWidget*> layout_hidden;
    switch (layout) {
    case Core::HID::NpadStyleIndex::ProController:
    case Core::HID::NpadStyleIndex::JoyconDual:
    case Core::HID::NpadStyleIndex::Handheld:
        layout_hidden = {
            ui->buttonShoulderButtonsSLSR,
            ui->horizontalSpacerShoulderButtonsWidget2,
        };
        break;
    case Core::HID::NpadStyleIndex::JoyconLeft:
        layout_hidden = {
            ui->horizontalSpacerShoulderButtonsWidget2,
            ui->buttonShoulderButtonsRight,
            ui->buttonMiscButtonsPlusHome,
            ui->bottomRight,
        };
        break;
    case Core::HID::NpadStyleIndex::JoyconRight:
        layout_hidden = {
            ui->horizontalSpacerShoulderButtonsWidget,
            ui->buttonShoulderButtonsLeft,
            ui->buttonMiscButtonsMinusScreenshot,
            ui->bottomLeft,
        };
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        layout_hidden = {
            ui->buttonShoulderButtonsSLSR,
            ui->horizontalSpacerShoulderButtonsWidget2,
            ui->buttonMiscButtonsMinusGroup,
            ui->buttonMiscButtonsScreenshotGroup,
        };
        break;
    default:
        break;
    }

    for (auto* widget : layout_hidden) {
        widget->hide();
    }
}

void ConfigureInputPlayer::UpdateControllerEnabledButtons() {
    auto layout = GetControllerTypeFromIndex(ui->comboControllerType->currentIndex());
    if (debug) {
        layout = Core::HID::NpadStyleIndex::ProController;
    }

    // List of all the widgets that will be disabled by any of the following layouts that need
    // "enabled" after the controller type changes
    const std::array<QWidget*, 3> layout_enable = {
        ui->buttonLStickPressedGroup,
        ui->groupRStickPressed,
        ui->buttonShoulderButtonsButtonLGroup,
    };

    for (auto* widget : layout_enable) {
        widget->setEnabled(true);
    }

    std::vector<QWidget*> layout_disable;
    switch (layout) {
    case Core::HID::NpadStyleIndex::ProController:
    case Core::HID::NpadStyleIndex::JoyconDual:
    case Core::HID::NpadStyleIndex::Handheld:
    case Core::HID::NpadStyleIndex::JoyconLeft:
    case Core::HID::NpadStyleIndex::JoyconRight:
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        layout_disable = {
            ui->buttonHome,
            ui->buttonLStickPressedGroup,
            ui->groupRStickPressed,
            ui->buttonShoulderButtonsButtonLGroup,
        };
        break;
    default:
        break;
    }

    for (auto* widget : layout_disable) {
        widget->setEnabled(false);
    }
}

void ConfigureInputPlayer::UpdateMotionButtons() {
    if (debug) {
        // Motion isn't used with the debug controller, hide both groupboxes.
        ui->buttonMotionLeftGroup->hide();
        ui->buttonMotionRightGroup->hide();
        return;
    }

    // Show/hide the "Motion 1/2" groupboxes depending on the currently selected controller.
    switch (GetControllerTypeFromIndex(ui->comboControllerType->currentIndex())) {
    case Core::HID::NpadStyleIndex::ProController:
    case Core::HID::NpadStyleIndex::JoyconLeft:
    case Core::HID::NpadStyleIndex::Handheld:
        // Show "Motion 1" and hide "Motion 2".
        ui->buttonMotionLeftGroup->show();
        ui->buttonMotionRightGroup->hide();
        break;
    case Core::HID::NpadStyleIndex::JoyconRight:
        // Show "Motion 2" and hide "Motion 1".
        ui->buttonMotionLeftGroup->hide();
        ui->buttonMotionRightGroup->show();
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        // Hide both "Motion 1/2".
        ui->buttonMotionLeftGroup->hide();
        ui->buttonMotionRightGroup->hide();
        break;
    case Core::HID::NpadStyleIndex::JoyconDual:
    default:
        // Show both "Motion 1/2".
        ui->buttonMotionLeftGroup->show();
        ui->buttonMotionRightGroup->show();
        break;
    }
}

void ConfigureInputPlayer::UpdateControllerButtonNames() {
    auto layout = GetControllerTypeFromIndex(ui->comboControllerType->currentIndex());
    if (debug) {
        layout = Core::HID::NpadStyleIndex::ProController;
    }

    switch (layout) {
    case Core::HID::NpadStyleIndex::ProController:
    case Core::HID::NpadStyleIndex::JoyconDual:
    case Core::HID::NpadStyleIndex::Handheld:
    case Core::HID::NpadStyleIndex::JoyconLeft:
    case Core::HID::NpadStyleIndex::JoyconRight:
        ui->buttonMiscButtonsPlusGroup->setTitle(tr("Plus"));
        ui->buttonShoulderButtonsButtonZLGroup->setTitle(tr("ZL"));
        ui->buttonShoulderButtonsZRGroup->setTitle(tr("ZR"));
        ui->buttonShoulderButtonsRGroup->setTitle(tr("R"));
        ui->LStick->setTitle(tr("Left Stick"));
        ui->RStick->setTitle(tr("Right Stick"));
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        ui->buttonMiscButtonsPlusGroup->setTitle(tr("Start / Pause"));
        ui->buttonShoulderButtonsButtonZLGroup->setTitle(tr("L"));
        ui->buttonShoulderButtonsZRGroup->setTitle(tr("R"));
        ui->buttonShoulderButtonsRGroup->setTitle(tr("Z"));
        ui->LStick->setTitle(tr("Control Stick"));
        ui->RStick->setTitle(tr("C-Stick"));
        break;
    default:
        break;
    }
}

void ConfigureInputPlayer::UpdateMappingWithDefaults() {
    if (ui->comboDevices->currentIndex() == 0) {
        return;
    }

    for (int button_id = 0; button_id < Settings::NativeButton::NumButtons; ++button_id) {
        const auto* const button = button_map[button_id];
        if (button == nullptr) {
            continue;
        }
        emulated_controller->SetButtonParam(button_id, {});
    }

    for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; ++analog_id) {
        for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; ++sub_button_id) {
            const auto* const analog_button = analog_map_buttons[analog_id][sub_button_id];
            if (analog_button == nullptr) {
                continue;
            }
            emulated_controller->SetStickParam(analog_id, {});
        }
    }

    for (int motion_id = 0; motion_id < Settings::NativeMotion::NumMotions; ++motion_id) {
        const auto* const motion_button = motion_map[motion_id];
        if (motion_button == nullptr) {
            continue;
        }
        emulated_controller->SetMotionParam(motion_id, {});
    }

    // Reset keyboard or mouse bindings
    if (ui->comboDevices->currentIndex() == 1 || ui->comboDevices->currentIndex() == 2) {
        for (int button_id = 0; button_id < Settings::NativeButton::NumButtons; ++button_id) {
            emulated_controller->SetButtonParam(
                button_id, Common::ParamPackage{InputCommon::GenerateKeyboardParam(
                               Config::default_buttons[button_id])});
        }
        for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; ++analog_id) {
            Common::ParamPackage analog_param{};
            for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; ++sub_button_id) {
                Common::ParamPackage params{InputCommon::GenerateKeyboardParam(
                    Config::default_analogs[analog_id][sub_button_id])};
                SetAnalogParam(params, analog_param, analog_sub_buttons[sub_button_id]);
            }

            analog_param.Set("modifier", InputCommon::GenerateKeyboardParam(
                                             Config::default_stick_mod[analog_id]));
            emulated_controller->SetStickParam(analog_id, analog_param);
        }

        for (int motion_id = 0; motion_id < Settings::NativeMotion::NumMotions; ++motion_id) {
            emulated_controller->SetMotionParam(
                motion_id, Common::ParamPackage{InputCommon::GenerateKeyboardParam(
                               Config::default_motions[motion_id])});
        }

        // If mouse is selected we want to override with mappings from the driver
        if (ui->comboDevices->currentIndex() == 1) {
            UpdateUI();
            return;
        }
    }

    // Reset controller bindings
    const auto& device = input_devices[ui->comboDevices->currentIndex()];
    auto button_mappings = input_subsystem->GetButtonMappingForDevice(device);
    auto analog_mappings = input_subsystem->GetAnalogMappingForDevice(device);
    auto motion_mappings = input_subsystem->GetMotionMappingForDevice(device);

    for (const auto& button_mapping : button_mappings) {
        const std::size_t index = button_mapping.first;
        emulated_controller->SetButtonParam(index, button_mapping.second);
    }
    for (const auto& analog_mapping : analog_mappings) {
        const std::size_t index = analog_mapping.first;
        emulated_controller->SetStickParam(index, analog_mapping.second);
    }
    for (const auto& motion_mapping : motion_mappings) {
        const std::size_t index = motion_mapping.first;
        emulated_controller->SetMotionParam(index, motion_mapping.second);
    }

    UpdateUI();
}

void ConfigureInputPlayer::HandleClick(
    QPushButton* button, std::size_t button_id,
    std::function<void(const Common::ParamPackage&)> new_input_setter,
    InputCommon::Polling::InputType type) {
    if (button == ui->buttonMotionLeft || button == ui->buttonMotionRight) {
        button->setText(tr("Shake!"));
    } else {
        button->setText(tr("[waiting]"));
    }
    button->setFocus();

    input_setter = new_input_setter;

    input_subsystem->BeginMapping(type);

    QWidget::grabMouse();
    QWidget::grabKeyboard();

    if (type == InputCommon::Polling::InputType::Button) {
        ui->controllerFrame->BeginMappingButton(button_id);
    } else if (type == InputCommon::Polling::InputType::Stick) {
        ui->controllerFrame->BeginMappingAnalog(button_id);
    }

    timeout_timer->start(2500); // Cancel after 2.5 seconds
    poll_timer->start(25);      // Check for new inputs every 25ms
}

void ConfigureInputPlayer::SetPollingResult(const Common::ParamPackage& params, bool abort) {
    timeout_timer->stop();
    poll_timer->stop();
    input_subsystem->StopMapping();

    QWidget::releaseMouse();
    QWidget::releaseKeyboard();

    if (!abort) {
        (*input_setter)(params);
    }

    UpdateUI();
    UpdateInputDeviceCombobox();
    ui->controllerFrame->EndMapping();

    input_setter = std::nullopt;
}

bool ConfigureInputPlayer::IsInputAcceptable(const Common::ParamPackage& params) const {
    if (ui->comboDevices->currentIndex() == 0) {
        return true;
    }

    if (params.Has("motion")) {
        return true;
    }

    // Keyboard/Mouse
    if (ui->comboDevices->currentIndex() == 1 || ui->comboDevices->currentIndex() == 2) {
        return params.Get("engine", "") == "keyboard" || params.Get("engine", "") == "mouse";
    }

    const auto current_input_device = input_devices[ui->comboDevices->currentIndex()];
    return params.Get("engine", "") == current_input_device.Get("engine", "") &&
           (params.Get("guid", "") == current_input_device.Get("guid", "") ||
            params.Get("guid", "") == current_input_device.Get("guid2", "")) &&
           params.Get("port", 0) == current_input_device.Get("port", 0);
}

void ConfigureInputPlayer::mousePressEvent(QMouseEvent* event) {
    if (!input_setter || !event) {
        return;
    }

    const auto button = GRenderWindow::QtButtonToMouseButton(event->button());
    input_subsystem->GetMouse()->PressButton(0, 0, 0, 0, button);
}

void ConfigureInputPlayer::keyPressEvent(QKeyEvent* event) {
    event->ignore();
    if (!input_setter || !event) {
        return;
    }
    if (event->key() != Qt::Key_Escape) {
        input_subsystem->GetKeyboard()->PressKey(event->key());
    }
}

void ConfigureInputPlayer::CreateProfile() {
    const auto profile_name =
        LimitableInputDialog::GetText(this, tr("New Profile"), tr("Enter a profile name:"), 1, 20,
                                      LimitableInputDialog::InputLimiter::Filesystem);

    if (profile_name.isEmpty()) {
        return;
    }

    if (!InputProfiles::IsProfileNameValid(profile_name.toStdString())) {
        QMessageBox::critical(this, tr("Create Input Profile"),
                              tr("The given profile name is not valid!"));
        return;
    }

    ApplyConfiguration();

    if (!profiles->CreateProfile(profile_name.toStdString(), player_index)) {
        QMessageBox::critical(this, tr("Create Input Profile"),
                              tr("Failed to create the input profile \"%1\"").arg(profile_name));
        UpdateInputProfiles();
        emit RefreshInputProfiles(player_index);
        return;
    }

    emit RefreshInputProfiles(player_index);

    ui->comboProfiles->addItem(profile_name);
    ui->comboProfiles->setCurrentIndex(ui->comboProfiles->count() - 1);
}

void ConfigureInputPlayer::DeleteProfile() {
    const QString profile_name = ui->comboProfiles->itemText(ui->comboProfiles->currentIndex());

    if (profile_name.isEmpty()) {
        return;
    }

    if (!profiles->DeleteProfile(profile_name.toStdString())) {
        QMessageBox::critical(this, tr("Delete Input Profile"),
                              tr("Failed to delete the input profile \"%1\"").arg(profile_name));
        UpdateInputProfiles();
        emit RefreshInputProfiles(player_index);
        return;
    }

    emit RefreshInputProfiles(player_index);

    ui->comboProfiles->removeItem(ui->comboProfiles->currentIndex());
    ui->comboProfiles->setCurrentIndex(-1);
}

void ConfigureInputPlayer::LoadProfile() {
    const QString profile_name = ui->comboProfiles->itemText(ui->comboProfiles->currentIndex());

    if (profile_name.isEmpty()) {
        return;
    }

    ApplyConfiguration();

    if (!profiles->LoadProfile(profile_name.toStdString(), player_index)) {
        QMessageBox::critical(this, tr("Load Input Profile"),
                              tr("Failed to load the input profile \"%1\"").arg(profile_name));
        UpdateInputProfiles();
        emit RefreshInputProfiles(player_index);
        return;
    }

    LoadConfiguration();
}

void ConfigureInputPlayer::SaveProfile() {
    const QString profile_name = ui->comboProfiles->itemText(ui->comboProfiles->currentIndex());

    if (profile_name.isEmpty()) {
        return;
    }

    ApplyConfiguration();

    if (!profiles->SaveProfile(profile_name.toStdString(), player_index)) {
        QMessageBox::critical(this, tr("Save Input Profile"),
                              tr("Failed to save the input profile \"%1\"").arg(profile_name));
        UpdateInputProfiles();
        emit RefreshInputProfiles(player_index);
        return;
    }
}

void ConfigureInputPlayer::UpdateInputProfiles() {
    ui->comboProfiles->clear();

    for (const auto& profile_name : profiles->GetInputProfileNames()) {
        ui->comboProfiles->addItem(QString::fromStdString(profile_name));
    }

    ui->comboProfiles->setCurrentIndex(-1);
}
