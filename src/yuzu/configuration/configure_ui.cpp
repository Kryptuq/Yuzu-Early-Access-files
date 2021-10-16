// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <utility>
#include <QFileDialog>

#include <QDirIterator>
#include "common/common_types.h"
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_ui.h"
#include "yuzu/configuration/configure_ui.h"
#include "yuzu/uisettings.h"

namespace {
constexpr std::array default_game_icon_sizes{
    std::make_pair(0, QT_TRANSLATE_NOOP("ConfigureUI", "None")),
    std::make_pair(32, QT_TRANSLATE_NOOP("ConfigureUI", "Small (32x32)")),
    std::make_pair(64, QT_TRANSLATE_NOOP("ConfigureUI", "Standard (64x64)")),
    std::make_pair(128, QT_TRANSLATE_NOOP("ConfigureUI", "Large (128x128)")),
    std::make_pair(256, QT_TRANSLATE_NOOP("ConfigureUI", "Full Size (256x256)")),
};

constexpr std::array default_folder_icon_sizes{
    std::make_pair(0, QT_TRANSLATE_NOOP("ConfigureUI", "None")),
    std::make_pair(24, QT_TRANSLATE_NOOP("ConfigureUI", "Small (24x24)")),
    std::make_pair(48, QT_TRANSLATE_NOOP("ConfigureUI", "Standard (48x48)")),
    std::make_pair(72, QT_TRANSLATE_NOOP("ConfigureUI", "Large (72x72)")),
};

// clang-format off
constexpr std::array row_text_names{
    QT_TRANSLATE_NOOP("ConfigureUI", "Filename"),
    QT_TRANSLATE_NOOP("ConfigureUI", "Filetype"),
    QT_TRANSLATE_NOOP("ConfigureUI", "Title ID"),
    QT_TRANSLATE_NOOP("ConfigureUI", "Title Name"),
    QT_TRANSLATE_NOOP("ConfigureUI", "None"),
};
// clang-format on

QString GetTranslatedGameIconSize(size_t index) {
    return QCoreApplication::translate("ConfigureUI", default_game_icon_sizes[index].second);
}

QString GetTranslatedFolderIconSize(size_t index) {
    return QCoreApplication::translate("ConfigureUI", default_folder_icon_sizes[index].second);
}

QString GetTranslatedRowTextName(size_t index) {
    return QCoreApplication::translate("ConfigureUI", row_text_names[index]);
}
} // Anonymous namespace

ConfigureUi::ConfigureUi(Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureUi>()}, system{system_} {
    ui->setupUi(this);

    InitializeLanguageComboBox();

    for (const auto& theme : UISettings::themes) {
        ui->theme_combobox->addItem(QString::fromUtf8(theme.first),
                                    QString::fromUtf8(theme.second));
    }

    InitializeIconSizeComboBox();
    InitializeRowComboBoxes();

    SetConfiguration();

    // Force game list reload if any of the relevant settings are changed.
    connect(ui->show_add_ons, &QCheckBox::stateChanged, this, &ConfigureUi::RequestGameListUpdate);
    connect(ui->game_icon_size_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
    connect(ui->folder_icon_size_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigureUi::RequestGameListUpdate);
    connect(ui->row_1_text_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
    connect(ui->row_2_text_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);

    // Update text ComboBoxes after user interaction.
    connect(ui->row_1_text_combobox, QOverload<int>::of(&QComboBox::activated),
            [this] { ConfigureUi::UpdateSecondRowComboBox(); });
    connect(ui->row_2_text_combobox, QOverload<int>::of(&QComboBox::activated),
            [this] { ConfigureUi::UpdateFirstRowComboBox(); });

    // Set screenshot path to user specification.
    connect(ui->screenshot_path_button, &QToolButton::pressed, this, [this] {
        auto dir =
            QFileDialog::getExistingDirectory(this, tr("Select Screenshots Path..."),
                                              QString::fromStdString(Common::FS::GetYuzuPathString(
                                                  Common::FS::YuzuPath::ScreenshotsDir)));
        if (!dir.isEmpty()) {
            if (dir.back() != QChar::fromLatin1('/')) {
                dir.append(QChar::fromLatin1('/'));
            }

            ui->screenshot_path_edit->setText(dir);
        }
    });
}

ConfigureUi::~ConfigureUi() = default;

void ConfigureUi::ApplyConfiguration() {
    UISettings::values.theme =
        ui->theme_combobox->itemData(ui->theme_combobox->currentIndex()).toString();
    UISettings::values.show_add_ons = ui->show_add_ons->isChecked();
    UISettings::values.game_icon_size = ui->game_icon_size_combobox->currentData().toUInt();
    UISettings::values.folder_icon_size = ui->folder_icon_size_combobox->currentData().toUInt();
    UISettings::values.row_1_text_id = ui->row_1_text_combobox->currentData().toUInt();
    UISettings::values.row_2_text_id = ui->row_2_text_combobox->currentData().toUInt();

    UISettings::values.enable_screenshot_save_as = ui->enable_screenshot_save_as->isChecked();
    Common::FS::SetYuzuPath(Common::FS::YuzuPath::ScreenshotsDir,
                            ui->screenshot_path_edit->text().toStdString());
    system.ApplySettings();
}

void ConfigureUi::RequestGameListUpdate() {
    UISettings::values.is_game_list_reload_pending.exchange(true);
}

void ConfigureUi::SetConfiguration() {
    ui->theme_combobox->setCurrentIndex(ui->theme_combobox->findData(UISettings::values.theme));
    ui->language_combobox->setCurrentIndex(
        ui->language_combobox->findData(UISettings::values.language));
    ui->show_add_ons->setChecked(UISettings::values.show_add_ons.GetValue());
    ui->game_icon_size_combobox->setCurrentIndex(
        ui->game_icon_size_combobox->findData(UISettings::values.game_icon_size.GetValue()));
    ui->folder_icon_size_combobox->setCurrentIndex(
        ui->folder_icon_size_combobox->findData(UISettings::values.folder_icon_size.GetValue()));

    ui->enable_screenshot_save_as->setChecked(
        UISettings::values.enable_screenshot_save_as.GetValue());
    ui->screenshot_path_edit->setText(QString::fromStdString(
        Common::FS::GetYuzuPathString(Common::FS::YuzuPath::ScreenshotsDir)));
}

void ConfigureUi::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureUi::RetranslateUI() {
    ui->retranslateUi(this);

    for (int i = 0; i < ui->game_icon_size_combobox->count(); i++) {
        ui->game_icon_size_combobox->setItemText(i,
                                                 GetTranslatedGameIconSize(static_cast<size_t>(i)));
    }

    for (int i = 0; i < ui->folder_icon_size_combobox->count(); i++) {
        ui->folder_icon_size_combobox->setItemText(
            i, GetTranslatedFolderIconSize(static_cast<size_t>(i)));
    }

    for (int i = 0; i < ui->row_1_text_combobox->count(); i++) {
        const QString name = GetTranslatedRowTextName(static_cast<size_t>(i));

        ui->row_1_text_combobox->setItemText(i, name);
        ui->row_2_text_combobox->setItemText(i, name);
    }
}

void ConfigureUi::InitializeLanguageComboBox() {
    ui->language_combobox->addItem(tr("<System>"), QString{});
    ui->language_combobox->addItem(tr("English"), QStringLiteral("en"));
    QDirIterator it(QStringLiteral(":/languages"), QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString locale = it.next();
        locale.truncate(locale.lastIndexOf(QLatin1Char{'.'}));
        locale.remove(0, locale.lastIndexOf(QLatin1Char{'/'}) + 1);
        const QString lang = QLocale::languageToString(QLocale(locale).language());
        ui->language_combobox->addItem(lang, locale);
    }

    // Unlike other configuration changes, interface language changes need to be reflected on the
    // interface immediately. This is done by passing a signal to the main window, and then
    // retranslating when passing back.
    connect(ui->language_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::OnLanguageChanged);
}

void ConfigureUi::InitializeIconSizeComboBox() {
    for (size_t i = 0; i < default_game_icon_sizes.size(); i++) {
        const auto size = default_game_icon_sizes[i].first;
        ui->game_icon_size_combobox->addItem(GetTranslatedGameIconSize(i), size);
    }
    for (size_t i = 0; i < default_folder_icon_sizes.size(); i++) {
        const auto size = default_folder_icon_sizes[i].first;
        ui->folder_icon_size_combobox->addItem(GetTranslatedFolderIconSize(i), size);
    }
}

void ConfigureUi::InitializeRowComboBoxes() {
    UpdateFirstRowComboBox(true);
    UpdateSecondRowComboBox(true);
}

void ConfigureUi::UpdateFirstRowComboBox(bool init) {
    const int currentIndex =
        init ? UISettings::values.row_1_text_id.GetValue()
             : ui->row_1_text_combobox->findData(ui->row_1_text_combobox->currentData());

    ui->row_1_text_combobox->clear();

    for (std::size_t i = 0; i < row_text_names.size(); i++) {
        const QString row_text_name = GetTranslatedRowTextName(i);
        ui->row_1_text_combobox->addItem(row_text_name, QVariant::fromValue(i));
    }

    ui->row_1_text_combobox->setCurrentIndex(ui->row_1_text_combobox->findData(currentIndex));

    ui->row_1_text_combobox->removeItem(4); // None
    ui->row_1_text_combobox->removeItem(
        ui->row_1_text_combobox->findData(ui->row_2_text_combobox->currentData()));
}

void ConfigureUi::UpdateSecondRowComboBox(bool init) {
    const int currentIndex =
        init ? UISettings::values.row_2_text_id.GetValue()
             : ui->row_2_text_combobox->findData(ui->row_2_text_combobox->currentData());

    ui->row_2_text_combobox->clear();

    for (std::size_t i = 0; i < row_text_names.size(); ++i) {
        const QString row_text_name = GetTranslatedRowTextName(i);
        ui->row_2_text_combobox->addItem(row_text_name, QVariant::fromValue(i));
    }

    ui->row_2_text_combobox->setCurrentIndex(ui->row_2_text_combobox->findData(currentIndex));

    ui->row_2_text_combobox->removeItem(
        ui->row_2_text_combobox->findData(ui->row_1_text_combobox->currentData()));
}

void ConfigureUi::OnLanguageChanged(int index) {
    if (index == -1)
        return;

    emit LanguageChanged(ui->language_combobox->itemData(index).toString());
}
