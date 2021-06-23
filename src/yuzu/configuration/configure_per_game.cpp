// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <fmt/format.h>

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QString>
#include <QTimer>
#include <QTreeView>

#include "common/fs/fs_util.h"
#include "common/fs/path_util.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/xts_archive.h"
#include "core/loader/loader.h"
#include "ui_configure_per_game.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/configure_input.h"
#include "yuzu/configuration/configure_per_game.h"
#include "yuzu/uisettings.h"
#include "yuzu/util/util.h"

ConfigurePerGame::ConfigurePerGame(QWidget* parent, u64 title_id, const std::string& file_name)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigurePerGame>()), title_id(title_id) {
    const auto file_path = std::filesystem::path(Common::FS::ToU8String(file_name));
    const auto config_file_name = title_id == 0 ? Common::FS::PathToUTF8String(file_path.filename())
                                                : fmt::format("{:016X}", title_id);
    game_config = std::make_unique<Config>(config_file_name, Config::ConfigType::PerGameConfig);

    Settings::SetConfiguringGlobal(false);

    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);
    setWindowTitle(tr("Properties"));
    // remove Help question mark button from the title bar
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->addonsTab->SetTitleId(title_id);

    scene = new QGraphicsScene;
    ui->icon_view->setScene(scene);

    if (Core::System::GetInstance().IsPoweredOn()) {
        QPushButton* apply_button = ui->buttonBox->addButton(QDialogButtonBox::Apply);
        connect(apply_button, &QAbstractButton::clicked, this,
                &ConfigurePerGame::HandleApplyButtonClicked);
    }

    LoadConfiguration();
}

ConfigurePerGame::~ConfigurePerGame() = default;

void ConfigurePerGame::ApplyConfiguration() {
    ui->addonsTab->ApplyConfiguration();
    ui->generalTab->ApplyConfiguration();
    ui->cpuTab->ApplyConfiguration();
    ui->systemTab->ApplyConfiguration();
    ui->graphicsTab->ApplyConfiguration();
    ui->graphicsAdvancedTab->ApplyConfiguration();
    ui->audioTab->ApplyConfiguration();

    Core::System::GetInstance().ApplySettings();
    Settings::LogSettings();

    game_config->Save();
}

void ConfigurePerGame::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigurePerGame::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigurePerGame::HandleApplyButtonClicked() {
    UISettings::values.configuration_applied = true;
    ApplyConfiguration();
}

void ConfigurePerGame::LoadFromFile(FileSys::VirtualFile file) {
    this->file = std::move(file);
    LoadConfiguration();
}

void ConfigurePerGame::LoadConfiguration() {
    if (file == nullptr) {
        return;
    }

    ui->addonsTab->LoadFromFile(file);

    ui->display_title_id->setText(
        QStringLiteral("%1").arg(title_id, 16, 16, QLatin1Char{'0'}).toUpper());

    auto& system = Core::System::GetInstance();
    const FileSys::PatchManager pm{title_id, system.GetFileSystemController(),
                                   system.GetContentProvider()};
    const auto control = pm.GetControlMetadata();
    const auto loader = Loader::GetLoader(system, file);

    if (control.first != nullptr) {
        ui->display_version->setText(QString::fromStdString(control.first->GetVersionString()));
        ui->display_name->setText(QString::fromStdString(control.first->GetApplicationName()));
        ui->display_developer->setText(QString::fromStdString(control.first->GetDeveloperName()));
    } else {
        std::string title;
        if (loader->ReadTitle(title) == Loader::ResultStatus::Success)
            ui->display_name->setText(QString::fromStdString(title));

        FileSys::NACP nacp;
        if (loader->ReadControlData(nacp) == Loader::ResultStatus::Success)
            ui->display_developer->setText(QString::fromStdString(nacp.GetDeveloperName()));

        ui->display_version->setText(QStringLiteral("1.0.0"));
    }

    if (control.second != nullptr) {
        scene->clear();

        QPixmap map;
        const auto bytes = control.second->ReadAllBytes();
        map.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));

        scene->addPixmap(map.scaled(ui->icon_view->width(), ui->icon_view->height(),
                                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
        std::vector<u8> bytes;
        if (loader->ReadIcon(bytes) == Loader::ResultStatus::Success) {
            scene->clear();

            QPixmap map;
            map.loadFromData(bytes.data(), static_cast<u32>(bytes.size()));

            scene->addPixmap(map.scaled(ui->icon_view->width(), ui->icon_view->height(),
                                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
    }

    ui->display_filename->setText(QString::fromStdString(file->GetName()));

    ui->display_format->setText(
        QString::fromStdString(Loader::GetFileTypeString(loader->GetFileType())));

    const auto valueText = ReadableByteSize(file->GetSize());
    ui->display_size->setText(valueText);
}
