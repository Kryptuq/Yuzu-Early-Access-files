// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QDesktopServices>
#include <QUrl>
#include "common/fs/path_util.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_debug.h"
#include "yuzu/configuration/configure_debug.h"
#include "yuzu/debugger/console.h"
#include "yuzu/uisettings.h"

ConfigureDebug::ConfigureDebug(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureDebug>()}, system{system_} {
    ui->setupUi(this);
    SetConfiguration();

    connect(ui->open_log_button, &QPushButton::clicked, []() {
        const auto path =
            QString::fromStdString(Common::FS::GetYuzuPathString(Common::FS::YuzuPath::LogDir));
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
}

ConfigureDebug::~ConfigureDebug() = default;

void ConfigureDebug::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();

    ui->toggle_console->setEnabled(runtime_lock);
    ui->toggle_console->setChecked(UISettings::values.show_console.GetValue());
    ui->log_filter_edit->setText(QString::fromStdString(Settings::values.log_filter.GetValue()));
    ui->homebrew_args_edit->setText(
        QString::fromStdString(Settings::values.program_args.GetValue()));
    ui->fs_access_log->setEnabled(runtime_lock);
    ui->fs_access_log->setChecked(Settings::values.enable_fs_access_log.GetValue());
    ui->reporting_services->setChecked(Settings::values.reporting_services.GetValue());
    ui->quest_flag->setChecked(Settings::values.quest_flag.GetValue());
    ui->use_debug_asserts->setChecked(Settings::values.use_debug_asserts.GetValue());
    ui->use_auto_stub->setChecked(Settings::values.use_auto_stub.GetValue());
    ui->enable_all_controllers->setChecked(Settings::values.enable_all_controllers.GetValue());
    ui->enable_graphics_debugging->setEnabled(runtime_lock);
    ui->enable_graphics_debugging->setChecked(Settings::values.renderer_debug.GetValue());
    ui->enable_shader_feedback->setEnabled(runtime_lock);
    ui->enable_shader_feedback->setChecked(Settings::values.renderer_shader_feedback.GetValue());
    ui->enable_cpu_debugging->setEnabled(runtime_lock);
    ui->enable_cpu_debugging->setChecked(Settings::values.cpu_debug_mode.GetValue());
    ui->enable_nsight_aftermath->setEnabled(runtime_lock);
    ui->enable_nsight_aftermath->setChecked(Settings::values.enable_nsight_aftermath.GetValue());
    ui->dump_shaders->setEnabled(runtime_lock);
    ui->dump_shaders->setChecked(Settings::values.dump_shaders.GetValue());
    ui->dump_macros->setEnabled(runtime_lock);
    ui->dump_macros->setChecked(Settings::values.dump_macros.GetValue());
    ui->disable_macro_jit->setEnabled(runtime_lock);
    ui->disable_macro_jit->setChecked(Settings::values.disable_macro_jit.GetValue());
    ui->disable_loop_safety_checks->setEnabled(runtime_lock);
    ui->disable_loop_safety_checks->setChecked(
        Settings::values.disable_shader_loop_safety_checks.GetValue());
    ui->extended_logging->setChecked(Settings::values.extended_logging.GetValue());

#ifdef YUZU_USE_QT_WEB_ENGINE
    ui->disable_web_applet->setChecked(UISettings::values.disable_web_applet.GetValue());
#else
    ui->disable_web_applet->setEnabled(false);
    ui->disable_web_applet->setText(QString::fromUtf8("Web applet not compiled"));
#endif
}

void ConfigureDebug::ApplyConfiguration() {
    UISettings::values.show_console = ui->toggle_console->isChecked();
    Settings::values.log_filter = ui->log_filter_edit->text().toStdString();
    Settings::values.program_args = ui->homebrew_args_edit->text().toStdString();
    Settings::values.enable_fs_access_log = ui->fs_access_log->isChecked();
    Settings::values.reporting_services = ui->reporting_services->isChecked();
    Settings::values.quest_flag = ui->quest_flag->isChecked();
    Settings::values.use_debug_asserts = ui->use_debug_asserts->isChecked();
    Settings::values.use_auto_stub = ui->use_auto_stub->isChecked();
    Settings::values.enable_all_controllers = ui->enable_all_controllers->isChecked();
    Settings::values.renderer_debug = ui->enable_graphics_debugging->isChecked();
    Settings::values.renderer_shader_feedback = ui->enable_shader_feedback->isChecked();
    Settings::values.cpu_debug_mode = ui->enable_cpu_debugging->isChecked();
    Settings::values.enable_nsight_aftermath = ui->enable_nsight_aftermath->isChecked();
    Settings::values.dump_shaders = ui->dump_shaders->isChecked();
    Settings::values.dump_macros = ui->dump_macros->isChecked();
    Settings::values.disable_shader_loop_safety_checks =
        ui->disable_loop_safety_checks->isChecked();
    Settings::values.disable_macro_jit = ui->disable_macro_jit->isChecked();
    Settings::values.extended_logging = ui->extended_logging->isChecked();
    UISettings::values.disable_web_applet = ui->disable_web_applet->isChecked();
    Debugger::ToggleConsole();
    Common::Log::Filter filter;
    filter.ParseFilterString(Settings::values.log_filter.GetValue());
    Common::Log::SetGlobalFilter(filter);
}

void ConfigureDebug::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureDebug::RetranslateUI() {
    ui->retranslateUi(this);
}
