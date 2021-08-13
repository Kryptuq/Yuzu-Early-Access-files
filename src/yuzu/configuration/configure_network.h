// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QFutureWatcher>
#include <QWidget>

namespace Ui {
class ConfigureNetwork;
}

class ConfigureNetwork : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureNetwork(QWidget* parent = nullptr);
    ~ConfigureNetwork() override;

    void ApplyConfiguration();
    void RetranslateUi();

private:
    void SetConfiguration();

    std::pair<QString, QString> BCATDownloadEvents();
    void OnBCATImplChanged();
    void OnUpdateBCATEmptyLabel(std::pair<QString, QString> string);

    std::unique_ptr<Ui::ConfigureNetwork> ui;
    QFutureWatcher<std::pair<QString, QString>> watcher{this};
};
