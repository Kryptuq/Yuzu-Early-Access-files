// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include <QImage>
#include <QThread>
#include <QTouchEvent>
#include <QWidget>

#include "common/thread.h"
#include "core/frontend/emu_window.h"

class GRenderWindow;
class GMainWindow;
class QKeyEvent;
class QStringList;

namespace Core {
enum class SystemResultStatus : u32;
class System;
} // namespace Core

namespace InputCommon {
class InputSubsystem;
enum class MouseButton;
} // namespace InputCommon

namespace InputCommon::TasInput {
enum class TasState;
} // namespace InputCommon::TasInput

namespace VideoCore {
enum class LoadCallbackStage;
class RendererBase;
} // namespace VideoCore

class EmuThread final : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(Core::System& system_);
    ~EmuThread() override;

    /**
     * Start emulation (on new thread)
     * @warning Only call when not running!
     */
    void run() override;

    /**
     * Steps the emulation thread by a single CPU instruction (if the CPU is not already running)
     * @note This function is thread-safe
     */
    void ExecStep() {
        exec_step = true;
        running_cv.notify_all();
    }

    /**
     * Sets whether the emulation thread is running or not
     * @param running Boolean value, set the emulation thread to running if true
     * @note This function is thread-safe
     */
    void SetRunning(bool running) {
        std::unique_lock lock{running_mutex};
        this->running = running;
        lock.unlock();
        running_cv.notify_all();
        if (!running) {
            running_wait.Set();
            /// Wait until effectively paused
            while (running_guard)
                ;
        }
    }

    /**
     * Check if the emulation thread is running or not
     * @return True if the emulation thread is running, otherwise false
     * @note This function is thread-safe
     */
    bool IsRunning() const {
        return running;
    }

    /**
     * Requests for the emulation thread to stop running
     */
    void RequestStop() {
        stop_source.request_stop();
        SetRunning(false);
    }

private:
    bool exec_step = false;
    bool running = false;
    std::stop_source stop_source;
    std::mutex running_mutex;
    std::condition_variable_any running_cv;
    Common::Event running_wait{};
    std::atomic_bool running_guard{false};
    Core::System& system;

signals:
    /**
     * Emitted when the CPU has halted execution
     *
     * @warning When connecting to this signal from other threads, make sure to specify either
     * Qt::QueuedConnection (invoke slot within the destination object's message thread) or even
     * Qt::BlockingQueuedConnection (additionally block source thread until slot returns)
     */
    void DebugModeEntered();

    /**
     * Emitted right before the CPU continues execution
     *
     * @warning When connecting to this signal from other threads, make sure to specify either
     * Qt::QueuedConnection (invoke slot within the destination object's message thread) or even
     * Qt::BlockingQueuedConnection (additionally block source thread until slot returns)
     */
    void DebugModeLeft();

    void ErrorThrown(Core::SystemResultStatus, std::string);

    void LoadProgress(VideoCore::LoadCallbackStage stage, std::size_t value, std::size_t total);
};

class GRenderWindow : public QWidget, public Core::Frontend::EmuWindow {
    Q_OBJECT

public:
    explicit GRenderWindow(GMainWindow* parent, EmuThread* emu_thread_,
                           std::shared_ptr<InputCommon::InputSubsystem> input_subsystem_,
                           Core::System& system_);
    ~GRenderWindow() override;

    // EmuWindow implementation.
    void OnFrameDisplayed() override;
    bool IsShown() const override;
    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override;

    void BackupGeometry();
    void RestoreGeometry();
    void restoreGeometry(const QByteArray& geometry); // overridden
    QByteArray saveGeometry();                        // overridden

    qreal windowPixelRatio() const;

    void closeEvent(QCloseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    /// Converts a Qt keybard key into NativeKeyboard key
    static int QtKeyToSwitchKey(Qt::Key qt_keys);

    /// Converts a Qt modifier keys into NativeKeyboard modifier keys
    static int QtModifierToSwitchModifier(Qt::KeyboardModifiers qt_modifiers);

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    /// Converts a Qt mouse button into MouseInput mouse button
    static InputCommon::MouseButton QtButtonToMouseButton(Qt::MouseButton button);

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    bool event(QEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    bool InitRenderTarget();

    /// Destroy the previous run's child_widget which should also destroy the child_window
    void ReleaseRenderTarget();

    bool IsLoadingComplete() const;

    void CaptureScreenshot(const QString& screenshot_path);

    std::pair<u32, u32> ScaleTouch(const QPointF& pos) const;

    /**
     * Instructs the window to re-launch the application using the specified program_index.
     * @param program_index Specifies the index within the application of the program to launch.
     */
    void ExecuteProgram(std::size_t program_index);

    /// Instructs the window to exit the application.
    void Exit();

public slots:
    void OnEmulationStarting(EmuThread* emu_thread);
    void OnEmulationStopping();
    void OnFramebufferSizeChanged();

signals:
    /// Emitted when the window is closed
    void Closed();
    void FirstFrameDisplayed();
    void ExecuteProgramSignal(std::size_t program_index);
    void ExitSignal();
    void MouseActivity();
    void TasPlaybackStateChanged();

private:
    void TouchBeginEvent(const QTouchEvent* event);
    void TouchUpdateEvent(const QTouchEvent* event);
    void TouchEndEvent();

    void OnMinimalClientAreaChangeRequest(std::pair<u32, u32> minimal_size) override;

    bool InitializeOpenGL();
    bool InitializeVulkan();
    bool LoadOpenGL();
    QStringList GetUnsupportedGLExtensions() const;

    EmuThread* emu_thread;
    std::shared_ptr<InputCommon::InputSubsystem> input_subsystem;

    // Main context that will be shared with all other contexts that are requested.
    // If this is used in a shared context setting, then this should not be used directly, but
    // should instead be shared from
    std::shared_ptr<Core::Frontend::GraphicsContext> main_context;

    /// Temporary storage of the screenshot taken
    QImage screenshot_image;

    QByteArray geometry;

    QWidget* child_widget = nullptr;

    bool first_frame = false;
    InputCommon::TasInput::TasState last_tas_state;

    Core::System& system;

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;
};
