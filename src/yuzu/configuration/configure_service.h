// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QFutureWatcher>
#include <QWidget>

namespace Ui {
class ConfigureService;
}

class ConfigureService : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureService(QWidget* parent = nullptr);
    ~ConfigureService() override;

    void ApplyConfiguration();
    void RetranslateUi();

private:
    void SetConfiguration();

    std::pair<QString, QString> BCATDownloadEvents();
    void OnBCATImplChanged();
    void OnUpdateBCATEmptyLabel(std::pair<QString, QString> string);

    std::unique_ptr<Ui::ConfigureService> ui;
    QFutureWatcher<std::pair<QString, QString>> watcher{this};
};
