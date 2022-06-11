// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#if !QT_CONFIG(movie)
#define YUZU_QT_MOVIE_MISSING 1
#endif

namespace Loader {
class AppLoader;
}

namespace Ui {
class LoadingScreen;
}

namespace VideoCore {
enum class LoadCallbackStage;
}

class QBuffer;
class QByteArray;
class QGraphicsOpacityEffect;
class QMovie;
class QPropertyAnimation;

class LoadingScreen : public QWidget {
    Q_OBJECT

public:
    explicit LoadingScreen(QWidget* parent = nullptr);

    ~LoadingScreen();

    /// Call before showing the loading screen to load the widgets with the logo and banner for the
    /// currently loaded application.
    void Prepare(Loader::AppLoader& loader);

    /// After the loading screen is hidden, the owner of this class can call this to clean up any
    /// used resources such as the logo and banner.
    void Clear();

    /// Slot used to update the status of the progress bar
    void OnLoadProgress(VideoCore::LoadCallbackStage stage, std::size_t value, std::size_t total);

    /// Hides the LoadingScreen with a fade out effect
    void OnLoadComplete();

    // In order to use a custom widget with a stylesheet, you need to override the paintEvent
    // See https://wiki.qt.io/How_to_Change_the_Background_Color_of_QWidget
    void paintEvent(QPaintEvent* event) override;

signals:
    void LoadProgress(VideoCore::LoadCallbackStage stage, std::size_t value, std::size_t total);
    /// Signals that this widget is completely hidden now and should be replaced with the other
    /// widget
    void Hidden();

private:
#ifndef YUZU_QT_MOVIE_MISSING
    std::unique_ptr<QMovie> animation;
    std::unique_ptr<QBuffer> backing_buf;
    std::unique_ptr<QByteArray> backing_mem;
#endif
    std::unique_ptr<Ui::LoadingScreen> ui;
    std::size_t previous_total = 0;
    VideoCore::LoadCallbackStage previous_stage;

    QGraphicsOpacityEffect* opacity_effect = nullptr;
    std::unique_ptr<QPropertyAnimation> fadeout_animation;

    // Definitions for the differences in text and styling for each stage
    std::unordered_map<VideoCore::LoadCallbackStage, const char*> progressbar_style;
    std::unordered_map<VideoCore::LoadCallbackStage, QString> stage_translations;

    // newly generated shaders are added to the end of the file, so when loading and compiling
    // shaders, it will start quickly but end slow if new shaders were added since previous launch.
    // These variables are used to detect the change in speed so we can generate an ETA
    bool slow_shader_compile_start = false;
    std::chrono::steady_clock::time_point slow_shader_start;
    std::chrono::steady_clock::time_point previous_time;
    std::size_t slow_shader_first_value = 0;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_DECLARE_METATYPE(VideoCore::LoadCallbackStage);
#endif
