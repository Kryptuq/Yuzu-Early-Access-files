// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <QBuffer>
#include <QByteArray>
#include <QGraphicsOpacityEffect>
#include <QIODevice>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QStyleOption>
#include "core/frontend/framebuffer_layout.h"
#include "core/loader/loader.h"
#include "ui_loading_screen.h"
#include "video_core/rasterizer_interface.h"
#include "yuzu/loading_screen.h"

// Mingw seems to not have QMovie at all. If QMovie is missing then use a single frame instead of an
// showing the full animation
#if !YUZU_QT_MOVIE_MISSING
#include <QMovie>
#endif

constexpr char PROGRESSBAR_STYLE_PREPARE[] = R"(
QProgressBar {}
QProgressBar::chunk {})";

constexpr char PROGRESSBAR_STYLE_BUILD[] = R"(
QProgressBar {
  background-color: black;
  border: 2px solid white;
  border-radius: 4px;
  padding: 2px;
}
QProgressBar::chunk {
  background-color: #ff3c28;
  width: 1px;
})";

constexpr char PROGRESSBAR_STYLE_COMPLETE[] = R"(
QProgressBar {
  background-color: #0ab9e6;
  border: 2px solid white;
  border-radius: 4px;
  padding: 2px;
}
QProgressBar::chunk {
  background-color: #ff3c28;
})";

LoadingScreen::LoadingScreen(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::LoadingScreen>()),
      previous_stage(VideoCore::LoadCallbackStage::Complete) {
    ui->setupUi(this);
    setMinimumSize(Layout::MinimumSize::Width, Layout::MinimumSize::Height);

    // Create a fade out effect to hide this loading screen widget.
    // When fading opacity, it will fade to the parent widgets background color, which is why we
    // create an internal widget named fade_widget that we use the effect on, while keeping the
    // loading screen widget's background color black. This way we can create a fade to black effect
    opacity_effect = new QGraphicsOpacityEffect(this);
    opacity_effect->setOpacity(1);
    ui->fade_parent->setGraphicsEffect(opacity_effect);
    fadeout_animation = std::make_unique<QPropertyAnimation>(opacity_effect, "opacity");
    fadeout_animation->setDuration(500);
    fadeout_animation->setStartValue(1);
    fadeout_animation->setEndValue(0);
    fadeout_animation->setEasingCurve(QEasingCurve::OutBack);

    // After the fade completes, hide the widget and reset the opacity
    connect(fadeout_animation.get(), &QPropertyAnimation::finished, [this] {
        hide();
        opacity_effect->setOpacity(1);
        emit Hidden();
    });
    connect(this, &LoadingScreen::LoadProgress, this, &LoadingScreen::OnLoadProgress,
            Qt::QueuedConnection);
    qRegisterMetaType<VideoCore::LoadCallbackStage>();

    stage_translations = {
        {VideoCore::LoadCallbackStage::Prepare, tr("Loading...")},
        {VideoCore::LoadCallbackStage::Build, tr("Loading Shaders %1 / %2")},
        {VideoCore::LoadCallbackStage::Complete, tr("Launching...")},
    };
    progressbar_style = {
        {VideoCore::LoadCallbackStage::Prepare, PROGRESSBAR_STYLE_PREPARE},
        {VideoCore::LoadCallbackStage::Build, PROGRESSBAR_STYLE_BUILD},
        {VideoCore::LoadCallbackStage::Complete, PROGRESSBAR_STYLE_COMPLETE},
    };
}

LoadingScreen::~LoadingScreen() = default;

void LoadingScreen::Prepare(Loader::AppLoader& loader) {
    std::vector<u8> buffer;
    if (loader.ReadBanner(buffer) == Loader::ResultStatus::Success) {
#ifdef YUZU_QT_MOVIE_MISSING
        QPixmap map;
        map.loadFromData(buffer.data(), buffer.size());
        ui->banner->setPixmap(map);
#else
        backing_mem = std::make_unique<QByteArray>(reinterpret_cast<char*>(buffer.data()),
                                                   static_cast<int>(buffer.size()));
        backing_buf = std::make_unique<QBuffer>(backing_mem.get());
        backing_buf->open(QIODevice::ReadOnly);
        animation = std::make_unique<QMovie>(backing_buf.get(), QByteArray());
        animation->start();
        ui->banner->setMovie(animation.get());
#endif
        buffer.clear();
    }
    if (loader.ReadLogo(buffer) == Loader::ResultStatus::Success) {
        QPixmap map;
        map.loadFromData(buffer.data(), static_cast<uint>(buffer.size()));
        ui->logo->setPixmap(map);
    }

    slow_shader_compile_start = false;
    OnLoadProgress(VideoCore::LoadCallbackStage::Prepare, 0, 0);
}

void LoadingScreen::OnLoadComplete() {
    fadeout_animation->start(QPropertyAnimation::KeepWhenStopped);
}

void LoadingScreen::OnLoadProgress(VideoCore::LoadCallbackStage stage, std::size_t value,
                                   std::size_t total) {
    using namespace std::chrono;
    const auto now = steady_clock::now();
    // reset the timer if the stage changes
    if (stage != previous_stage) {
        ui->progress_bar->setStyleSheet(QString::fromUtf8(progressbar_style[stage]));
        // Hide the progress bar during the prepare stage
        if (stage == VideoCore::LoadCallbackStage::Prepare) {
            ui->progress_bar->hide();
        } else {
            ui->progress_bar->show();
        }
        previous_stage = stage;
        // reset back to fast shader compiling since the stage changed
        slow_shader_compile_start = false;
    }
    // update the max of the progress bar if the number of shaders change
    if (total != previous_total) {
        ui->progress_bar->setMaximum(static_cast<int>(total));
        previous_total = total;
    }
    // Reset the progress bar ranges if compilation is done
    if (stage == VideoCore::LoadCallbackStage::Complete) {
        ui->progress_bar->setRange(0, 0);
    }

    QString estimate;
    // If there's a drastic slowdown in the rate, then display an estimate
    if (now - previous_time > milliseconds{50} || slow_shader_compile_start) {
        if (!slow_shader_compile_start) {
            slow_shader_start = steady_clock::now();
            slow_shader_compile_start = true;
            slow_shader_first_value = value;
        }
        // only calculate an estimate time after a second has passed since stage change
        const auto diff = duration_cast<milliseconds>(now - slow_shader_start);
        if (diff > seconds{1}) {
            const auto eta_mseconds =
                static_cast<long>(static_cast<double>(total - slow_shader_first_value) /
                                  (value - slow_shader_first_value) * diff.count());
            estimate =
                tr("Estimated Time %1")
                    .arg(QTime(0, 0, 0, 0)
                             .addMSecs(std::max<long>(eta_mseconds - diff.count() + 1000, 1000))
                             .toString(QStringLiteral("mm:ss")));
        }
    }

    // update labels and progress bar
    if (stage == VideoCore::LoadCallbackStage::Build) {
        ui->stage->setText(stage_translations[stage].arg(value).arg(total));
    } else {
        ui->stage->setText(stage_translations[stage]);
    }
    ui->value->setText(estimate);
    ui->progress_bar->setValue(static_cast<int>(value));
    previous_time = now;
}

void LoadingScreen::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

void LoadingScreen::Clear() {
#ifndef YUZU_QT_MOVIE_MISSING
    animation.reset();
    backing_buf.reset();
    backing_mem.reset();
#endif
}
