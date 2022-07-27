// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <utility>
#include <QFileDialog>

#include <QDirIterator>
#include "common/common_types.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
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
    // This is a list of lexicographically sorted languages, only the available translations are
    // shown to the user.
    static const struct {
        const QString name;
        const char* id;
    } languages[] = {
        // clang-format off
        {QStringLiteral(u"Bahasa Indonesia"), "id"},                                 // Indonesian
        {QStringLiteral(u"Bahasa Melayu"), "ms"},                                    // Malay
        {QStringLiteral(u"Catal\u00E0"), "ca"},                                      // Catalan
        {QStringLiteral(u"\u010Ce\u0161tina"), "cs"},                                // Czech
        {QStringLiteral(u"Dansk"), "da"},                                            // Danish
        {QStringLiteral(u"Deutsch"), "de"},                                          // German
        {QStringLiteral(u"English"), "en"},                                          // English
        {QStringLiteral(u"Espa\u00F1ol"), "es"},                                     // Spanish
        {QStringLiteral(u"Fran\u00E7ais"), "fr"},                                    // French
        {QStringLiteral(u"Hrvatski"), "hr"},                                         // Croatian
        {QStringLiteral(u"Italiano"), "it"},                                         // Italian
        {QStringLiteral(u"Magyar"), "hu"},                                           // Hungarian
        {QStringLiteral(u"Nederlands"), "nl"},                                       // Dutch
        {QStringLiteral(u"Norsk bokm\u00E5l"), "nb"},                                // Norwegian
        {QStringLiteral(u"Polski"), "pl"},                                           // Polish
        {QStringLiteral(u"Portugu\u00EAs"), "pt_PT"},                                // Portuguese
        {QStringLiteral(u"Portugu\u00EAs (Brasil)"), "pt_BR"},                       // Portuguese (Brazil)
        {QStringLiteral(u"Rom\u00E2n\u0103"), "ro"},                                 // Romanian
        {QStringLiteral(u"Srpski"), "sr"},                                           // Serbian
        {QStringLiteral(u"Suomi"), "fi"},                                            // Finnish
        {QStringLiteral(u"Svenska"), "sv"},                                          // Swedish
        {QStringLiteral(u"Ti\u1EBFng Vi\u1EC7t"), "vi"},                             // Vietnamese
        {QStringLiteral(u"Ti\u1EBFng Vi\u1EC7t (Vi\u1EC7t Nam)"), "vi_VN"},          // Vietnamese
        {QStringLiteral(u"T\u00FCrk\u00E7e"), "tr_TR"},                              // Turkish
        {QStringLiteral(u"\u0395\u03BB\u03BB\u03B7\u03BD\u03B9\u03BA\u03AC"), "el"}, // Greek
        {QStringLiteral(u"\u0420\u0443\u0441\u0441\u043A\u0438\u0439"), "ru_RU"},    // Russian
        {QStringLiteral(u"\u0423\u043A\u0440\u0430\u0457\u043D\u0441\u044C\u043A\u0430"),
         "uk"},                                                                      // Ukrainian
        {QStringLiteral(u"\u0627\u0644\u0639\u0631\u0628\u064A\u0629"), "ar"},       // Arabic
        {QStringLiteral(u"\u0641\u0627\u0631\u0633\u06CC"), "fa"},                   // Farsi
        {QStringLiteral(u"\uD55C\uAD6D\uC5B4"), "ko_KR"},                            // Korean
        {QStringLiteral(u"\u65E5\u672C\u8A9E"), "ja_JP"},                            // Japanese
        {QStringLiteral(u"\u7B80\u4F53\u4E2D\u6587"), "zh_CN"},                      // Simplified Chinese
        {QStringLiteral(u"\u7E41\u9AD4\u4E2D\u6587"), "zh_TW"},                      // Traditional Chinese
        // clang-format on
    };
    ui->language_combobox->addItem(tr("<System>"), QString{});
    QDir languages_dir{QStringLiteral(":/languages")};
    QStringList language_files = languages_dir.entryList();
    for (const auto& lang : languages) {
        if (QString::fromLatin1(lang.id) == QStringLiteral("en")) {
            ui->language_combobox->addItem(lang.name, QStringLiteral("en"));
            continue;
        }
        for (int i = 0; i < language_files.size(); ++i) {
            QString locale = language_files[i];
            locale.truncate(locale.lastIndexOf(QLatin1Char{'.'}));
            if (QString::fromLatin1(lang.id) == locale) {
                ui->language_combobox->addItem(lang.name, locale);
                language_files.removeAt(i);
                break;
            }
        }
    }
    // Anything remaining will be at the bottom
    for (const QString& file : language_files) {
        LOG_CRITICAL(Frontend, "Unexpected Language File: {}", file.toStdString());
        QString locale = file;
        locale.truncate(locale.lastIndexOf(QLatin1Char{'.'}));
        const QString language_name = QLocale::languageToString(QLocale(locale).language());
        const QString lang = QStringLiteral("%1 [%2]").arg(language_name, locale);
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
