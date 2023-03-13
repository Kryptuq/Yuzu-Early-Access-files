// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QButtonGroup>
#include <QMessageBox>
#include <QPushButton>
#include <QtConcurrent/qtconcurrentrun.h>
#include "common/logging/log.h"
#include "common/telemetry.h"
#include "core/telemetry_session.h"
#include "ui_compatdb.h"
#include "yuzu/compatdb.h"

CompatDB::CompatDB(Core::TelemetrySession& telemetry_session_, QWidget* parent)
    : QWizard(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::CompatDB>()}, telemetry_session{telemetry_session_} {
    ui->setupUi(this);

    connect(ui->radioButton_GameBoot_Yes, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_GameBoot_No, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Gameplay_Yes, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Gameplay_No, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_NoFreeze_Yes, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_NoFreeze_No, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Complete_Yes, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Complete_No, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Graphical_Major, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Graphical_Minor, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Graphical_No, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Audio_Major, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Audio_Minor, &QRadioButton::clicked, this, &CompatDB::EnableNext);
    connect(ui->radioButton_Audio_No, &QRadioButton::clicked, this, &CompatDB::EnableNext);

    connect(button(NextButton), &QPushButton::clicked, this, &CompatDB::Submit);
    connect(&testcase_watcher, &QFutureWatcher<bool>::finished, this,
            &CompatDB::OnTestcaseSubmitted);
}

CompatDB::~CompatDB() = default;

enum class CompatDBPage {
    Intro = 0,
    GameBoot = 1,
    GamePlay = 2,
    Freeze = 3,
    Completion = 4,
    Graphical = 5,
    Audio = 6,
    Final = 7,
};

void CompatDB::Submit() {
    QButtonGroup* compatibility_GameBoot = new QButtonGroup(this);
    compatibility_GameBoot->addButton(ui->radioButton_GameBoot_Yes, 0);
    compatibility_GameBoot->addButton(ui->radioButton_GameBoot_No, 1);

    QButtonGroup* compatibility_Gameplay = new QButtonGroup(this);
    compatibility_Gameplay->addButton(ui->radioButton_Gameplay_Yes, 0);
    compatibility_Gameplay->addButton(ui->radioButton_Gameplay_No, 1);

    QButtonGroup* compatibility_NoFreeze = new QButtonGroup(this);
    compatibility_NoFreeze->addButton(ui->radioButton_NoFreeze_Yes, 0);
    compatibility_NoFreeze->addButton(ui->radioButton_NoFreeze_No, 1);

    QButtonGroup* compatibility_Complete = new QButtonGroup(this);
    compatibility_Complete->addButton(ui->radioButton_Complete_Yes, 0);
    compatibility_Complete->addButton(ui->radioButton_Complete_No, 1);

    QButtonGroup* compatibility_Graphical = new QButtonGroup(this);
    compatibility_Graphical->addButton(ui->radioButton_Graphical_Major, 0);
    compatibility_Graphical->addButton(ui->radioButton_Graphical_Minor, 1);
    compatibility_Graphical->addButton(ui->radioButton_Graphical_No, 2);

    QButtonGroup* compatibility_Audio = new QButtonGroup(this);
    compatibility_Audio->addButton(ui->radioButton_Audio_Major, 0);
    compatibility_Graphical->addButton(ui->radioButton_Audio_Minor, 1);
    compatibility_Audio->addButton(ui->radioButton_Audio_No, 2);

    const int compatibility = static_cast<int>(CalculateCompatibility());

    switch ((static_cast<CompatDBPage>(currentId()))) {
    case CompatDBPage::Intro:
        break;
    case CompatDBPage::GameBoot:
        if (compatibility_GameBoot->checkedId() == -1) {
            button(NextButton)->setEnabled(false);
        }
        break;
    case CompatDBPage::GamePlay:
        if (compatibility_Gameplay->checkedId() == -1) {
            button(NextButton)->setEnabled(false);
        }
        break;
    case CompatDBPage::Freeze:
        if (compatibility_NoFreeze->checkedId() == -1) {
            button(NextButton)->setEnabled(false);
        }
        break;
    case CompatDBPage::Completion:
        if (compatibility_Complete->checkedId() == -1) {
            button(NextButton)->setEnabled(false);
        }
        break;
    case CompatDBPage::Graphical:
        if (compatibility_Graphical->checkedId() == -1) {
            button(NextButton)->setEnabled(false);
        }
        break;
    case CompatDBPage::Audio:
        if (compatibility_Audio->checkedId() == -1) {
            button(NextButton)->setEnabled(false);
        }
        break;
    case CompatDBPage::Final:
        back();
        LOG_INFO(Frontend, "Compatibility Rating: {}", compatibility);
        telemetry_session.AddField(Common::Telemetry::FieldType::UserFeedback, "Compatibility",
                                   compatibility);

        button(NextButton)->setEnabled(false);
        button(NextButton)->setText(tr("Submitting"));
        button(CancelButton)->setVisible(false);

        testcase_watcher.setFuture(
            QtConcurrent::run([this] { return telemetry_session.SubmitTestcase(); }));
        break;
    default:
        LOG_ERROR(Frontend, "Unexpected page: {}", currentId());
        break;
    }
}

int CompatDB::nextId() const {
    switch ((static_cast<CompatDBPage>(currentId()))) {
    case CompatDBPage::Intro:
        return static_cast<int>(CompatDBPage::GameBoot);
    case CompatDBPage::GameBoot:
        if (ui->radioButton_GameBoot_No->isChecked()) {
            return static_cast<int>(CompatDBPage::Final);
        }
        return static_cast<int>(CompatDBPage::GamePlay);
    case CompatDBPage::GamePlay:
        if (ui->radioButton_Gameplay_No->isChecked()) {
            return static_cast<int>(CompatDBPage::Final);
        }
        return static_cast<int>(CompatDBPage::Freeze);
    case CompatDBPage::Freeze:
        if (ui->radioButton_NoFreeze_No->isChecked()) {
            return static_cast<int>(CompatDBPage::Final);
        }
        return static_cast<int>(CompatDBPage::Completion);
    case CompatDBPage::Completion:
        if (ui->radioButton_Complete_No->isChecked()) {
            return static_cast<int>(CompatDBPage::Final);
        }
        return static_cast<int>(CompatDBPage::Graphical);
    case CompatDBPage::Graphical:
        return static_cast<int>(CompatDBPage::Audio);
    case CompatDBPage::Audio:
        return static_cast<int>(CompatDBPage::Final);
    case CompatDBPage::Final:
        return -1;
    default:
        LOG_ERROR(Frontend, "Unexpected page: {}", currentId());
        return static_cast<int>(CompatDBPage::Intro);
    }
}

CompatibilityStatus CompatDB::CalculateCompatibility() const {
    if (ui->radioButton_GameBoot_No->isChecked()) {
        return CompatibilityStatus::WontBoot;
    }

    if (ui->radioButton_Gameplay_No->isChecked()) {
        return CompatibilityStatus::IntroMenu;
    }

    if (ui->radioButton_NoFreeze_No->isChecked() || ui->radioButton_Complete_No->isChecked()) {
        return CompatibilityStatus::Ingame;
    }

    if (ui->radioButton_Graphical_Major->isChecked() || ui->radioButton_Audio_Major->isChecked()) {
        return CompatibilityStatus::Ingame;
    }

    if (ui->radioButton_Graphical_Minor->isChecked() || ui->radioButton_Audio_Minor->isChecked()) {
        return CompatibilityStatus::Playable;
    }

    return CompatibilityStatus::Perfect;
}

void CompatDB::OnTestcaseSubmitted() {
    if (!testcase_watcher.result()) {
        QMessageBox::critical(this, tr("Communication error"),
                              tr("An error occurred while sending the Testcase"));
        button(NextButton)->setEnabled(true);
        button(NextButton)->setText(tr("Next"));
        button(CancelButton)->setVisible(true);
    } else {
        next();
        // older versions of QT don't support the "NoCancelButtonOnLastPage" option, this is a
        // workaround
        button(CancelButton)->setVisible(false);
    }
}

void CompatDB::EnableNext() {
    button(NextButton)->setEnabled(true);
}
