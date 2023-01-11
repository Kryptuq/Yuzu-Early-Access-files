// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <glad/glad.h>

#include <QApplication>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) && YUZU_USE_QT_MULTIMEDIA
#include <QCameraImageCapture>
#include <QCameraInfo>
#endif
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>
#include <QString>
#include <QStringList>
#include <QWindow>

#ifdef HAS_OPENGL
#include <QOffscreenSurface>
#include <QOpenGLContext>
#endif

#if !defined(WIN32)
#include <qpa/qplatformnativeinterface.h>
#endif

#include <fmt/format.h>

#include "common/assert.h"
#include "common/microprofile.h"
#include "common/scm_rev.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/cpu_manager.h"
#include "core/frontend/framebuffer_layout.h"
#include "input_common/drivers/camera.h"
#include "input_common/drivers/keyboard.h"
#include "input_common/drivers/mouse.h"
#include "input_common/drivers/tas_input.h"
#include "input_common/drivers/touch_screen.h"
#include "input_common/main.h"
#include "video_core/renderer_base.h"
#include "yuzu/bootmanager.h"
#include "yuzu/main.h"

static Core::Frontend::WindowSystemType GetWindowSystemType();

EmuThread::EmuThread(Core::System& system) : m_system{system} {}

EmuThread::~EmuThread() = default;

void EmuThread::run() {
    const char* name = "EmuControlThread";
    MicroProfileOnThreadCreate(name);
    Common::SetCurrentThreadName(name);

    auto& gpu = m_system.GPU();
    auto stop_token = m_stop_source.get_token();

    m_system.RegisterHostThread();

    // Main process has been loaded. Make the context current to this thread and begin GPU and CPU
    // execution.
    gpu.ObtainContext();

    emit LoadProgress(VideoCore::LoadCallbackStage::Prepare, 0, 0);
    if (Settings::values.use_disk_shader_cache.GetValue()) {
        m_system.Renderer().ReadRasterizer()->LoadDiskResources(
            m_system.GetCurrentProcessProgramID(), stop_token,
            [this](VideoCore::LoadCallbackStage stage, std::size_t value, std::size_t total) {
                emit LoadProgress(stage, value, total);
            });
    }
    emit LoadProgress(VideoCore::LoadCallbackStage::Complete, 0, 0);

    gpu.ReleaseContext();
    gpu.Start();

    m_system.GetCpuManager().OnGpuReady();

    if (m_system.DebuggerEnabled()) {
        m_system.InitializeDebugger();
    }

    while (!stop_token.stop_requested()) {
        std::unique_lock lk{m_should_run_mutex};
        if (m_should_run) {
            m_system.Run();
            m_is_running.store(true);
            m_is_running.notify_all();

            Common::CondvarWait(m_should_run_cv, lk, stop_token, [&] { return !m_should_run; });
        } else {
            m_system.Pause();
            m_is_running.store(false);
            m_is_running.notify_all();

            EmulationPaused(lk);
            Common::CondvarWait(m_should_run_cv, lk, stop_token, [&] { return m_should_run; });
            EmulationResumed(lk);
        }
    }

    // Shutdown the main emulated process
    m_system.DetachDebugger();
    m_system.ShutdownMainProcess();

#if MICROPROFILE_ENABLED
    MicroProfileOnThreadExit();
#endif
}

// Unlock while emitting signals so that the main thread can
// continue pumping events.

void EmuThread::EmulationPaused(std::unique_lock<std::mutex>& lk) {
    lk.unlock();
    emit DebugModeEntered();
    lk.lock();
}

void EmuThread::EmulationResumed(std::unique_lock<std::mutex>& lk) {
    lk.unlock();
    emit DebugModeLeft();
    lk.lock();
}

#ifdef HAS_OPENGL
class OpenGLSharedContext : public Core::Frontend::GraphicsContext {
public:
    /// Create the original context that should be shared from
    explicit OpenGLSharedContext(QSurface* surface_) : surface{surface_} {
        QSurfaceFormat format;
        format.setVersion(4, 6);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setOption(QSurfaceFormat::FormatOption::DeprecatedFunctions);
        if (Settings::values.renderer_debug) {
            format.setOption(QSurfaceFormat::FormatOption::DebugContext);
        }
        // TODO: expose a setting for buffer value (ie default/single/double/triple)
        format.setSwapBehavior(QSurfaceFormat::DefaultSwapBehavior);
        format.setSwapInterval(0);

        context = std::make_unique<QOpenGLContext>();
        context->setFormat(format);
        if (!context->create()) {
            LOG_ERROR(Frontend, "Unable to create main openGL context");
        }
    }

    /// Create the shared contexts for rendering and presentation
    explicit OpenGLSharedContext(QOpenGLContext* share_context, QSurface* main_surface = nullptr) {

        // disable vsync for any shared contexts
        auto format = share_context->format();
        format.setSwapInterval(main_surface ? Settings::values.use_vsync.GetValue() : 0);

        context = std::make_unique<QOpenGLContext>();
        context->setShareContext(share_context);
        context->setFormat(format);
        if (!context->create()) {
            LOG_ERROR(Frontend, "Unable to create shared openGL context");
        }

        if (!main_surface) {
            offscreen_surface = std::make_unique<QOffscreenSurface>(nullptr);
            offscreen_surface->setFormat(format);
            offscreen_surface->create();
            surface = offscreen_surface.get();
        } else {
            surface = main_surface;
        }
    }

    ~OpenGLSharedContext() {
        DoneCurrent();
    }

    void SwapBuffers() override {
        context->swapBuffers(surface);
    }

    void MakeCurrent() override {
        // We can't track the current state of the underlying context in this wrapper class because
        // Qt may make the underlying context not current for one reason or another. In particular,
        // the WebBrowser uses GL, so it seems to conflict if we aren't careful.
        // Instead of always just making the context current (which does not have any caching to
        // check if the underlying context is already current) we can check for the current context
        // in the thread local data by calling `currentContext()` and checking if its ours.
        if (QOpenGLContext::currentContext() != context.get()) {
            context->makeCurrent(surface);
        }
    }

    void DoneCurrent() override {
        context->doneCurrent();
    }

    QOpenGLContext* GetShareContext() {
        return context.get();
    }

    const QOpenGLContext* GetShareContext() const {
        return context.get();
    }

private:
    // Avoid using Qt parent system here since we might move the QObjects to new threads
    // As a note, this means we should avoid using slots/signals with the objects too
    std::unique_ptr<QOpenGLContext> context;
    std::unique_ptr<QOffscreenSurface> offscreen_surface{};
    QSurface* surface;
};
#endif

class DummyContext : public Core::Frontend::GraphicsContext {};

class RenderWidget : public QWidget {
public:
    explicit RenderWidget(GRenderWindow* parent) : QWidget(parent), render_window(parent) {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        if (GetWindowSystemType() == Core::Frontend::WindowSystemType::Wayland) {
            setAttribute(Qt::WA_DontCreateNativeAncestors);
        }
    }

    virtual ~RenderWidget() = default;

    QPaintEngine* paintEngine() const override {
        return nullptr;
    }

private:
    GRenderWindow* render_window;
};

struct OpenGLRenderWidget : public RenderWidget {
    explicit OpenGLRenderWidget(GRenderWindow* parent) : RenderWidget(parent) {
        windowHandle()->setSurfaceType(QWindow::OpenGLSurface);
    }

    void SetContext(std::unique_ptr<Core::Frontend::GraphicsContext>&& context_) {
        context = std::move(context_);
    }

private:
    std::unique_ptr<Core::Frontend::GraphicsContext> context;
};

struct VulkanRenderWidget : public RenderWidget {
    explicit VulkanRenderWidget(GRenderWindow* parent) : RenderWidget(parent) {
        windowHandle()->setSurfaceType(QWindow::VulkanSurface);
    }
};

struct NullRenderWidget : public RenderWidget {
    explicit NullRenderWidget(GRenderWindow* parent) : RenderWidget(parent) {}
};

static Core::Frontend::WindowSystemType GetWindowSystemType() {
    // Determine WSI type based on Qt platform.
    QString platform_name = QGuiApplication::platformName();
    if (platform_name == QStringLiteral("windows"))
        return Core::Frontend::WindowSystemType::Windows;
    else if (platform_name == QStringLiteral("xcb"))
        return Core::Frontend::WindowSystemType::X11;
    else if (platform_name == QStringLiteral("wayland"))
        return Core::Frontend::WindowSystemType::Wayland;
    else if (platform_name == QStringLiteral("wayland-egl"))
        return Core::Frontend::WindowSystemType::Wayland;
    else if (platform_name == QStringLiteral("cocoa"))
        return Core::Frontend::WindowSystemType::Cocoa;
    else if (platform_name == QStringLiteral("android"))
        return Core::Frontend::WindowSystemType::Android;

    LOG_CRITICAL(Frontend, "Unknown Qt platform {}!", platform_name.toStdString());
    return Core::Frontend::WindowSystemType::Windows;
}

static Core::Frontend::EmuWindow::WindowSystemInfo GetWindowSystemInfo(QWindow* window) {
    Core::Frontend::EmuWindow::WindowSystemInfo wsi;
    wsi.type = GetWindowSystemType();

    // Our Win32 Qt external doesn't have the private API.
#if defined(WIN32) || defined(__APPLE__)
    wsi.render_surface = window ? reinterpret_cast<void*>(window->winId()) : nullptr;
#else
    QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
    wsi.display_connection = pni->nativeResourceForWindow("display", window);
    if (wsi.type == Core::Frontend::WindowSystemType::Wayland)
        wsi.render_surface = window ? pni->nativeResourceForWindow("surface", window) : nullptr;
    else
        wsi.render_surface = window ? reinterpret_cast<void*>(window->winId()) : nullptr;
#endif
    wsi.render_surface_scale = window ? static_cast<float>(window->devicePixelRatio()) : 1.0f;

    return wsi;
}

GRenderWindow::GRenderWindow(GMainWindow* parent, EmuThread* emu_thread_,
                             std::shared_ptr<InputCommon::InputSubsystem> input_subsystem_,
                             Core::System& system_)
    : QWidget(parent),
      emu_thread(emu_thread_), input_subsystem{std::move(input_subsystem_)}, system{system_} {
    setWindowTitle(QStringLiteral("yuzu %1 | %2-%3")
                       .arg(QString::fromUtf8(Common::g_build_name),
                            QString::fromUtf8(Common::g_scm_branch),
                            QString::fromUtf8(Common::g_scm_desc)));
    setAttribute(Qt::WA_AcceptTouchEvents);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    input_subsystem->Initialize();
    this->setMouseTracking(true);

    strict_context_required = QGuiApplication::platformName() == QStringLiteral("wayland") ||
                              QGuiApplication::platformName() == QStringLiteral("wayland-egl");

    connect(this, &GRenderWindow::FirstFrameDisplayed, parent, &GMainWindow::OnLoadComplete);
    connect(this, &GRenderWindow::ExecuteProgramSignal, parent, &GMainWindow::OnExecuteProgram,
            Qt::QueuedConnection);
    connect(this, &GRenderWindow::ExitSignal, parent, &GMainWindow::OnExit, Qt::QueuedConnection);
    connect(this, &GRenderWindow::TasPlaybackStateChanged, parent, &GMainWindow::OnTasStateChanged);
}

void GRenderWindow::ExecuteProgram(std::size_t program_index) {
    emit ExecuteProgramSignal(program_index);
}

void GRenderWindow::Exit() {
    emit ExitSignal();
}

GRenderWindow::~GRenderWindow() {
    input_subsystem->Shutdown();
}

void GRenderWindow::OnFrameDisplayed() {
    input_subsystem->GetTas()->UpdateThread();
    const InputCommon::TasInput::TasState new_tas_state =
        std::get<0>(input_subsystem->GetTas()->GetStatus());

    if (!first_frame) {
        last_tas_state = new_tas_state;
        first_frame = true;
        emit FirstFrameDisplayed();
    }

    if (new_tas_state != last_tas_state) {
        last_tas_state = new_tas_state;
        emit TasPlaybackStateChanged();
    }
}

bool GRenderWindow::IsShown() const {
    return !isMinimized();
}

// On Qt 5.0+, this correctly gets the size of the framebuffer (pixels).
//
// Older versions get the window size (density independent pixels),
// and hence, do not support DPI scaling ("retina" displays).
// The result will be a viewport that is smaller than the extent of the window.
void GRenderWindow::OnFramebufferSizeChanged() {
    // Screen changes potentially incur a change in screen DPI, hence we should update the
    // framebuffer size
    const qreal pixel_ratio = windowPixelRatio();
    const u32 width = this->width() * pixel_ratio;
    const u32 height = this->height() * pixel_ratio;
    UpdateCurrentFramebufferLayout(width, height);
}

void GRenderWindow::BackupGeometry() {
    geometry = QWidget::saveGeometry();
}

void GRenderWindow::RestoreGeometry() {
    // We don't want to back up the geometry here (obviously)
    QWidget::restoreGeometry(geometry);
}

void GRenderWindow::restoreGeometry(const QByteArray& geometry_) {
    // Make sure users of this class don't need to deal with backing up the geometry themselves
    QWidget::restoreGeometry(geometry_);
    BackupGeometry();
}

QByteArray GRenderWindow::saveGeometry() {
    // If we are a top-level widget, store the current geometry
    // otherwise, store the last backup
    if (parent() == nullptr) {
        return QWidget::saveGeometry();
    }

    return geometry;
}

qreal GRenderWindow::windowPixelRatio() const {
    return devicePixelRatioF();
}

std::pair<u32, u32> GRenderWindow::ScaleTouch(const QPointF& pos) const {
    const qreal pixel_ratio = windowPixelRatio();
    return {static_cast<u32>(std::max(std::round(pos.x() * pixel_ratio), qreal{0.0})),
            static_cast<u32>(std::max(std::round(pos.y() * pixel_ratio), qreal{0.0}))};
}

void GRenderWindow::closeEvent(QCloseEvent* event) {
    emit Closed();
    QWidget::closeEvent(event);
}

int GRenderWindow::QtKeyToSwitchKey(Qt::Key qt_key) {
    static constexpr std::array<std::pair<Qt::Key, Settings::NativeKeyboard::Keys>, 106> key_map = {
        std::pair<Qt::Key, Settings::NativeKeyboard::Keys>{Qt::Key_A, Settings::NativeKeyboard::A},
        {Qt::Key_A, Settings::NativeKeyboard::A},
        {Qt::Key_B, Settings::NativeKeyboard::B},
        {Qt::Key_C, Settings::NativeKeyboard::C},
        {Qt::Key_D, Settings::NativeKeyboard::D},
        {Qt::Key_E, Settings::NativeKeyboard::E},
        {Qt::Key_F, Settings::NativeKeyboard::F},
        {Qt::Key_G, Settings::NativeKeyboard::G},
        {Qt::Key_H, Settings::NativeKeyboard::H},
        {Qt::Key_I, Settings::NativeKeyboard::I},
        {Qt::Key_J, Settings::NativeKeyboard::J},
        {Qt::Key_K, Settings::NativeKeyboard::K},
        {Qt::Key_L, Settings::NativeKeyboard::L},
        {Qt::Key_M, Settings::NativeKeyboard::M},
        {Qt::Key_N, Settings::NativeKeyboard::N},
        {Qt::Key_O, Settings::NativeKeyboard::O},
        {Qt::Key_P, Settings::NativeKeyboard::P},
        {Qt::Key_Q, Settings::NativeKeyboard::Q},
        {Qt::Key_R, Settings::NativeKeyboard::R},
        {Qt::Key_S, Settings::NativeKeyboard::S},
        {Qt::Key_T, Settings::NativeKeyboard::T},
        {Qt::Key_U, Settings::NativeKeyboard::U},
        {Qt::Key_V, Settings::NativeKeyboard::V},
        {Qt::Key_W, Settings::NativeKeyboard::W},
        {Qt::Key_X, Settings::NativeKeyboard::X},
        {Qt::Key_Y, Settings::NativeKeyboard::Y},
        {Qt::Key_Z, Settings::NativeKeyboard::Z},
        {Qt::Key_1, Settings::NativeKeyboard::N1},
        {Qt::Key_2, Settings::NativeKeyboard::N2},
        {Qt::Key_3, Settings::NativeKeyboard::N3},
        {Qt::Key_4, Settings::NativeKeyboard::N4},
        {Qt::Key_5, Settings::NativeKeyboard::N5},
        {Qt::Key_6, Settings::NativeKeyboard::N6},
        {Qt::Key_7, Settings::NativeKeyboard::N7},
        {Qt::Key_8, Settings::NativeKeyboard::N8},
        {Qt::Key_9, Settings::NativeKeyboard::N9},
        {Qt::Key_0, Settings::NativeKeyboard::N0},
        {Qt::Key_Return, Settings::NativeKeyboard::Return},
        {Qt::Key_Escape, Settings::NativeKeyboard::Escape},
        {Qt::Key_Backspace, Settings::NativeKeyboard::Backspace},
        {Qt::Key_Tab, Settings::NativeKeyboard::Tab},
        {Qt::Key_Space, Settings::NativeKeyboard::Space},
        {Qt::Key_Minus, Settings::NativeKeyboard::Minus},
        {Qt::Key_Plus, Settings::NativeKeyboard::Plus},
        {Qt::Key_questiondown, Settings::NativeKeyboard::Plus},
        {Qt::Key_BracketLeft, Settings::NativeKeyboard::OpenBracket},
        {Qt::Key_BraceLeft, Settings::NativeKeyboard::OpenBracket},
        {Qt::Key_BracketRight, Settings::NativeKeyboard::CloseBracket},
        {Qt::Key_BraceRight, Settings::NativeKeyboard::CloseBracket},
        {Qt::Key_Bar, Settings::NativeKeyboard::Pipe},
        {Qt::Key_Dead_Tilde, Settings::NativeKeyboard::Tilde},
        {Qt::Key_Ntilde, Settings::NativeKeyboard::Semicolon},
        {Qt::Key_Semicolon, Settings::NativeKeyboard::Semicolon},
        {Qt::Key_Apostrophe, Settings::NativeKeyboard::Quote},
        {Qt::Key_Dead_Grave, Settings::NativeKeyboard::Backquote},
        {Qt::Key_Comma, Settings::NativeKeyboard::Comma},
        {Qt::Key_Period, Settings::NativeKeyboard::Period},
        {Qt::Key_Slash, Settings::NativeKeyboard::Slash},
        {Qt::Key_CapsLock, Settings::NativeKeyboard::CapsLockKey},
        {Qt::Key_F1, Settings::NativeKeyboard::F1},
        {Qt::Key_F2, Settings::NativeKeyboard::F2},
        {Qt::Key_F3, Settings::NativeKeyboard::F3},
        {Qt::Key_F4, Settings::NativeKeyboard::F4},
        {Qt::Key_F5, Settings::NativeKeyboard::F5},
        {Qt::Key_F6, Settings::NativeKeyboard::F6},
        {Qt::Key_F7, Settings::NativeKeyboard::F7},
        {Qt::Key_F8, Settings::NativeKeyboard::F8},
        {Qt::Key_F9, Settings::NativeKeyboard::F9},
        {Qt::Key_F10, Settings::NativeKeyboard::F10},
        {Qt::Key_F11, Settings::NativeKeyboard::F11},
        {Qt::Key_F12, Settings::NativeKeyboard::F12},
        {Qt::Key_Print, Settings::NativeKeyboard::PrintScreen},
        {Qt::Key_ScrollLock, Settings::NativeKeyboard::ScrollLockKey},
        {Qt::Key_Pause, Settings::NativeKeyboard::Pause},
        {Qt::Key_Insert, Settings::NativeKeyboard::Insert},
        {Qt::Key_Home, Settings::NativeKeyboard::Home},
        {Qt::Key_PageUp, Settings::NativeKeyboard::PageUp},
        {Qt::Key_Delete, Settings::NativeKeyboard::Delete},
        {Qt::Key_End, Settings::NativeKeyboard::End},
        {Qt::Key_PageDown, Settings::NativeKeyboard::PageDown},
        {Qt::Key_Right, Settings::NativeKeyboard::Right},
        {Qt::Key_Left, Settings::NativeKeyboard::Left},
        {Qt::Key_Down, Settings::NativeKeyboard::Down},
        {Qt::Key_Up, Settings::NativeKeyboard::Up},
        {Qt::Key_NumLock, Settings::NativeKeyboard::NumLockKey},
        // Numpad keys are missing here
        {Qt::Key_F13, Settings::NativeKeyboard::F13},
        {Qt::Key_F14, Settings::NativeKeyboard::F14},
        {Qt::Key_F15, Settings::NativeKeyboard::F15},
        {Qt::Key_F16, Settings::NativeKeyboard::F16},
        {Qt::Key_F17, Settings::NativeKeyboard::F17},
        {Qt::Key_F18, Settings::NativeKeyboard::F18},
        {Qt::Key_F19, Settings::NativeKeyboard::F19},
        {Qt::Key_F20, Settings::NativeKeyboard::F20},
        {Qt::Key_F21, Settings::NativeKeyboard::F21},
        {Qt::Key_F22, Settings::NativeKeyboard::F22},
        {Qt::Key_F23, Settings::NativeKeyboard::F23},
        {Qt::Key_F24, Settings::NativeKeyboard::F24},
        // {Qt::..., Settings::NativeKeyboard::KPComma},
        // {Qt::..., Settings::NativeKeyboard::Ro},
        {Qt::Key_Hiragana_Katakana, Settings::NativeKeyboard::KatakanaHiragana},
        {Qt::Key_yen, Settings::NativeKeyboard::Yen},
        {Qt::Key_Henkan, Settings::NativeKeyboard::Henkan},
        {Qt::Key_Muhenkan, Settings::NativeKeyboard::Muhenkan},
        // {Qt::..., Settings::NativeKeyboard::NumPadCommaPc98},
        {Qt::Key_Hangul, Settings::NativeKeyboard::HangulEnglish},
        {Qt::Key_Hangul_Hanja, Settings::NativeKeyboard::Hanja},
        {Qt::Key_Katakana, Settings::NativeKeyboard::KatakanaKey},
        {Qt::Key_Hiragana, Settings::NativeKeyboard::HiraganaKey},
        {Qt::Key_Zenkaku_Hankaku, Settings::NativeKeyboard::ZenkakuHankaku},
        // Modifier keys are handled by the modifier property
    };

    for (const auto& [qkey, nkey] : key_map) {
        if (qt_key == qkey) {
            return nkey;
        }
    }

    return Settings::NativeKeyboard::None;
}

int GRenderWindow::QtModifierToSwitchModifier(Qt::KeyboardModifiers qt_modifiers) {
    int modifier = 0;

    if ((qt_modifiers & Qt::KeyboardModifier::ShiftModifier) != 0) {
        modifier |= 1 << Settings::NativeKeyboard::LeftShift;
    }
    if ((qt_modifiers & Qt::KeyboardModifier::ControlModifier) != 0) {
        modifier |= 1 << Settings::NativeKeyboard::LeftControl;
    }
    if ((qt_modifiers & Qt::KeyboardModifier::AltModifier) != 0) {
        modifier |= 1 << Settings::NativeKeyboard::LeftAlt;
    }
    if ((qt_modifiers & Qt::KeyboardModifier::MetaModifier) != 0) {
        modifier |= 1 << Settings::NativeKeyboard::LeftMeta;
    }

    // TODO: These keys can't be obtained with Qt::KeyboardModifier

    // if ((qt_modifiers & 0x10) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::RightShift;
    // }
    // if ((qt_modifiers & 0x20) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::RightControl;
    // }
    // if ((qt_modifiers & 0x40) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::RightAlt;
    // }
    // if ((qt_modifiers & 0x80) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::RightMeta;
    // }
    // if ((qt_modifiers & 0x100) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::CapsLock;
    // }
    // if ((qt_modifiers & 0x200) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::NumLock;
    // }
    // if ((qt_modifiers & ???) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::ScrollLock;
    // }
    // if ((qt_modifiers & ???) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::Katakana;
    // }
    // if ((qt_modifiers & ???) != 0) {
    //    modifier |= 1 << Settings::NativeKeyboard::Hiragana;
    // }
    return modifier;
}

void GRenderWindow::keyPressEvent(QKeyEvent* event) {
    /**
     * This feature can be enhanced with the following functions, but they do not provide
     * cross-platform behavior.
     *
     * event->nativeVirtualKey() can distinguish between keys on the numpad.
     * event->nativeModifiers() can distinguish between left and right keys and numlock,
     * capslock, scroll lock.
     */
    if (!event->isAutoRepeat()) {
        const auto modifier = QtModifierToSwitchModifier(event->modifiers());
        const auto key = QtKeyToSwitchKey(Qt::Key(event->key()));
        input_subsystem->GetKeyboard()->SetKeyboardModifiers(modifier);
        input_subsystem->GetKeyboard()->PressKeyboardKey(key);
        // This is used for gamepads that can have any key mapped
        input_subsystem->GetKeyboard()->PressKey(event->key());
    }
}

void GRenderWindow::keyReleaseEvent(QKeyEvent* event) {
    /**
     * This feature can be enhanced with the following functions, but they do not provide
     * cross-platform behavior.
     *
     * event->nativeVirtualKey() can distinguish between keys on the numpad.
     * event->nativeModifiers() can distinguish between left and right buttons and numlock,
     * capslock, scroll lock.
     */
    if (!event->isAutoRepeat()) {
        const auto modifier = QtModifierToSwitchModifier(event->modifiers());
        const auto key = QtKeyToSwitchKey(Qt::Key(event->key()));
        input_subsystem->GetKeyboard()->SetKeyboardModifiers(modifier);
        input_subsystem->GetKeyboard()->ReleaseKeyboardKey(key);
        // This is used for gamepads that can have any key mapped
        input_subsystem->GetKeyboard()->ReleaseKey(event->key());
    }
}

InputCommon::MouseButton GRenderWindow::QtButtonToMouseButton(Qt::MouseButton button) {
    switch (button) {
    case Qt::LeftButton:
        return InputCommon::MouseButton::Left;
    case Qt::RightButton:
        return InputCommon::MouseButton::Right;
    case Qt::MiddleButton:
        return InputCommon::MouseButton::Wheel;
    case Qt::BackButton:
        return InputCommon::MouseButton::Backward;
    case Qt::ForwardButton:
        return InputCommon::MouseButton::Forward;
    case Qt::TaskButton:
        return InputCommon::MouseButton::Task;
    default:
        return InputCommon::MouseButton::Extra;
    }
}

void GRenderWindow::mousePressEvent(QMouseEvent* event) {
    // Touch input is handled in TouchBeginEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    // Qt sometimes returns the parent coordinates. To avoid this we read the global mouse
    // coordinates and map them to the current render area
    const auto pos = mapFromGlobal(QCursor::pos());
    const auto [x, y] = ScaleTouch(pos);
    const auto [touch_x, touch_y] = MapToTouchScreen(x, y);
    const auto button = QtButtonToMouseButton(event->button());
    input_subsystem->GetMouse()->PressButton(x, y, touch_x, touch_y, button);

    emit MouseActivity();
}

void GRenderWindow::mouseMoveEvent(QMouseEvent* event) {
    // Touch input is handled in TouchUpdateEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    // Qt sometimes returns the parent coordinates. To avoid this we read the global mouse
    // coordinates and map them to the current render area
    const auto pos = mapFromGlobal(QCursor::pos());
    const auto [x, y] = ScaleTouch(pos);
    const auto [touch_x, touch_y] = MapToTouchScreen(x, y);
    const int center_x = width() / 2;
    const int center_y = height() / 2;
    input_subsystem->GetMouse()->MouseMove(x, y, touch_x, touch_y, center_x, center_y);

    if (Settings::values.mouse_panning && !Settings::values.mouse_enabled) {
        QCursor::setPos(mapToGlobal(QPoint{center_x, center_y}));
    }

    emit MouseActivity();
}

void GRenderWindow::mouseReleaseEvent(QMouseEvent* event) {
    // Touch input is handled in TouchEndEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }

    const auto button = QtButtonToMouseButton(event->button());
    input_subsystem->GetMouse()->ReleaseButton(button);
}

void GRenderWindow::wheelEvent(QWheelEvent* event) {
    const int x = event->angleDelta().x();
    const int y = event->angleDelta().y();
    input_subsystem->GetMouse()->MouseWheelChange(x, y);
}

void GRenderWindow::TouchBeginEvent(const QTouchEvent* event) {
    QList<QTouchEvent::TouchPoint> touch_points = event->touchPoints();
    for (const auto& touch_point : touch_points) {
        const auto [x, y] = ScaleTouch(touch_point.pos());
        const auto [touch_x, touch_y] = MapToTouchScreen(x, y);
        input_subsystem->GetTouchScreen()->TouchPressed(touch_x, touch_y, touch_point.id());
    }
}

void GRenderWindow::TouchUpdateEvent(const QTouchEvent* event) {
    QList<QTouchEvent::TouchPoint> touch_points = event->touchPoints();
    input_subsystem->GetTouchScreen()->ClearActiveFlag();
    for (const auto& touch_point : touch_points) {
        const auto [x, y] = ScaleTouch(touch_point.pos());
        const auto [touch_x, touch_y] = MapToTouchScreen(x, y);
        input_subsystem->GetTouchScreen()->TouchMoved(touch_x, touch_y, touch_point.id());
    }
    input_subsystem->GetTouchScreen()->ReleaseInactiveTouch();
}

void GRenderWindow::TouchEndEvent() {
    input_subsystem->GetTouchScreen()->ReleaseAllTouch();
}

void GRenderWindow::InitializeCamera() {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) && YUZU_USE_QT_MULTIMEDIA
    constexpr auto camera_update_ms = std::chrono::milliseconds{50}; // (50ms, 20Hz)
    if (!Settings::values.enable_ir_sensor) {
        return;
    }

    bool camera_found = false;
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo& cameraInfo : cameras) {
        if (Settings::values.ir_sensor_device.GetValue() == cameraInfo.deviceName().toStdString() ||
            Settings::values.ir_sensor_device.GetValue() == "Auto") {
            camera = std::make_unique<QCamera>(cameraInfo);
            if (!camera->isCaptureModeSupported(QCamera::CaptureMode::CaptureViewfinder) &&
                !camera->isCaptureModeSupported(QCamera::CaptureMode::CaptureStillImage)) {
                LOG_ERROR(Frontend,
                          "Camera doesn't support CaptureViewfinder or CaptureStillImage");
                continue;
            }
            camera_found = true;
            break;
        }
    }

    if (!camera_found) {
        return;
    }

    camera_capture = std::make_unique<QCameraImageCapture>(camera.get());

    if (!camera_capture->isCaptureDestinationSupported(
            QCameraImageCapture::CaptureDestination::CaptureToBuffer)) {
        LOG_ERROR(Frontend, "Camera doesn't support saving to buffer");
        return;
    }

    const auto camera_width = input_subsystem->GetCamera()->getImageWidth();
    const auto camera_height = input_subsystem->GetCamera()->getImageHeight();
    camera_data.resize(camera_width * camera_height);
    camera_capture->setCaptureDestination(QCameraImageCapture::CaptureDestination::CaptureToBuffer);
    connect(camera_capture.get(), &QCameraImageCapture::imageCaptured, this,
            &GRenderWindow::OnCameraCapture);
    camera->unload();
    if (camera->isCaptureModeSupported(QCamera::CaptureMode::CaptureViewfinder)) {
        camera->setCaptureMode(QCamera::CaptureViewfinder);
    } else if (camera->isCaptureModeSupported(QCamera::CaptureMode::CaptureStillImage)) {
        camera->setCaptureMode(QCamera::CaptureStillImage);
    }
    camera->load();
    camera->start();

    pending_camera_snapshots = 0;
    is_virtual_camera = false;

    camera_timer = std::make_unique<QTimer>();
    connect(camera_timer.get(), &QTimer::timeout, [this] { RequestCameraCapture(); });
    // This timer should be dependent of camera resolution 5ms for every 100 pixels
    camera_timer->start(camera_update_ms);
#endif
}

void GRenderWindow::FinalizeCamera() {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) && YUZU_USE_QT_MULTIMEDIA
    if (camera_timer) {
        camera_timer->stop();
    }
    if (camera) {
        camera->unload();
    }
#endif
}

void GRenderWindow::RequestCameraCapture() {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) && YUZU_USE_QT_MULTIMEDIA
    if (!Settings::values.enable_ir_sensor) {
        return;
    }

    // If the camera doesn't capture, test for virtual cameras
    if (pending_camera_snapshots > 5) {
        is_virtual_camera = true;
    }
    // Virtual cameras like obs need to reset the camera every capture
    if (is_virtual_camera) {
        camera->stop();
        camera->start();
    }

    pending_camera_snapshots++;
    camera_capture->capture();
#endif
}

void GRenderWindow::OnCameraCapture(int requestId, const QImage& img) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0)) && YUZU_USE_QT_MULTIMEDIA
    // TODO: Capture directly in the format and resolution needed
    const auto camera_width = input_subsystem->GetCamera()->getImageWidth();
    const auto camera_height = input_subsystem->GetCamera()->getImageHeight();
    const auto converted =
        img.scaled(static_cast<int>(camera_width), static_cast<int>(camera_height),
                   Qt::AspectRatioMode::IgnoreAspectRatio,
                   Qt::TransformationMode::SmoothTransformation)
            .mirrored(false, true);
    if (camera_data.size() != camera_width * camera_height) {
        camera_data.resize(camera_width * camera_height);
    }
    std::memcpy(camera_data.data(), converted.bits(), camera_width * camera_height * sizeof(u32));
    input_subsystem->GetCamera()->SetCameraData(camera_width, camera_height, camera_data);
    pending_camera_snapshots = 0;
#endif
}

bool GRenderWindow::event(QEvent* event) {
    if (event->type() == QEvent::TouchBegin) {
        TouchBeginEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchUpdate) {
        TouchUpdateEvent(static_cast<QTouchEvent*>(event));
        return true;
    } else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        TouchEndEvent();
        return true;
    }

    return QWidget::event(event);
}

void GRenderWindow::focusOutEvent(QFocusEvent* event) {
    QWidget::focusOutEvent(event);
    input_subsystem->GetKeyboard()->ReleaseAllKeys();
    input_subsystem->GetMouse()->ReleaseAllButtons();
    input_subsystem->GetTouchScreen()->ReleaseAllTouch();
}

void GRenderWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    OnFramebufferSizeChanged();
}

std::unique_ptr<Core::Frontend::GraphicsContext> GRenderWindow::CreateSharedContext() const {
#ifdef HAS_OPENGL
    if (Settings::values.renderer_backend.GetValue() == Settings::RendererBackend::OpenGL) {
        auto c = static_cast<OpenGLSharedContext*>(main_context.get());
        // Bind the shared contexts to the main surface in case the backend wants to take over
        // presentation
        return std::make_unique<OpenGLSharedContext>(c->GetShareContext(),
                                                     child_widget->windowHandle());
    }
#endif
    return std::make_unique<DummyContext>();
}

bool GRenderWindow::InitRenderTarget() {
    ReleaseRenderTarget();

    {
        // Create a dummy render widget so that Qt
        // places the render window at the correct position.
        const RenderWidget dummy_widget{this};
    }

    first_frame = false;

    switch (Settings::values.renderer_backend.GetValue()) {
    case Settings::RendererBackend::OpenGL:
        if (!InitializeOpenGL()) {
            return false;
        }
        break;
    case Settings::RendererBackend::Vulkan:
        if (!InitializeVulkan()) {
            return false;
        }
        break;
    case Settings::RendererBackend::Null:
        InitializeNull();
        break;
    }

    // Update the Window System information with the new render target
    window_info = GetWindowSystemInfo(child_widget->windowHandle());

    child_widget->resize(Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height);
    layout()->addWidget(child_widget);
    // Reset minimum required size to avoid resizing issues on the main window after restarting.
    setMinimumSize(1, 1);

    resize(Layout::ScreenUndocked::Width, Layout::ScreenUndocked::Height);

    OnMinimalClientAreaChangeRequest(GetActiveConfig().min_client_area_size);
    OnFramebufferSizeChanged();
    BackupGeometry();

    if (Settings::values.renderer_backend.GetValue() == Settings::RendererBackend::OpenGL) {
        if (!LoadOpenGL()) {
            return false;
        }
    }

    return true;
}

void GRenderWindow::ReleaseRenderTarget() {
    if (child_widget) {
        layout()->removeWidget(child_widget);
        child_widget->deleteLater();
        child_widget = nullptr;
    }
    main_context.reset();
}

void GRenderWindow::CaptureScreenshot(const QString& screenshot_path) {
    auto& renderer = system.Renderer();
    const f32 res_scale = Settings::values.resolution_info.up_factor;

    if (renderer.IsScreenshotPending()) {
        LOG_WARNING(Render,
                    "A screenshot is already requested or in progress, ignoring the request");
        return;
    }

    const Layout::FramebufferLayout layout{Layout::FrameLayoutFromResolutionScale(res_scale)};
    screenshot_image = QImage(QSize(layout.width, layout.height), QImage::Format_RGB32);
    renderer.RequestScreenshot(
        screenshot_image.bits(),
        [=, this](bool invert_y) {
            const std::string std_screenshot_path = screenshot_path.toStdString();
            if (screenshot_image.mirrored(false, invert_y).save(screenshot_path)) {
                LOG_INFO(Frontend, "Screenshot saved to \"{}\"", std_screenshot_path);
            } else {
                LOG_ERROR(Frontend, "Failed to save screenshot to \"{}\"", std_screenshot_path);
            }
        },
        layout);
}

bool GRenderWindow::IsLoadingComplete() const {
    return first_frame;
}

void GRenderWindow::OnMinimalClientAreaChangeRequest(std::pair<u32, u32> minimal_size) {
    setMinimumSize(minimal_size.first, minimal_size.second);
}

bool GRenderWindow::InitializeOpenGL() {
#ifdef HAS_OPENGL
    if (!QOpenGLContext::supportsThreadedOpenGL()) {
        QMessageBox::warning(this, tr("OpenGL not available!"),
                             tr("OpenGL shared contexts are not supported."));
        return false;
    }

    // TODO: One of these flags might be interesting: WA_OpaquePaintEvent, WA_NoBackground,
    // WA_DontShowOnScreen, WA_DeleteOnClose
    auto child = new OpenGLRenderWidget(this);
    child_widget = child;
    child_widget->windowHandle()->create();
    auto context = std::make_shared<OpenGLSharedContext>(child->windowHandle());
    main_context = context;
    child->SetContext(
        std::make_unique<OpenGLSharedContext>(context->GetShareContext(), child->windowHandle()));

    return true;
#else
    QMessageBox::warning(this, tr("OpenGL not available!"),
                         tr("yuzu has not been compiled with OpenGL support."));
    return false;
#endif
}

bool GRenderWindow::InitializeVulkan() {
    auto child = new VulkanRenderWidget(this);
    child_widget = child;
    child_widget->windowHandle()->create();
    main_context = std::make_unique<DummyContext>();

    return true;
}

void GRenderWindow::InitializeNull() {
    child_widget = new NullRenderWidget(this);
    main_context = std::make_unique<DummyContext>();
}

bool GRenderWindow::LoadOpenGL() {
    auto context = CreateSharedContext();
    auto scope = context->Acquire();
    if (!gladLoadGL()) {
        QMessageBox::warning(
            this, tr("Error while initializing OpenGL!"),
            tr("Your GPU may not support OpenGL, or you do not have the latest graphics driver."));
        return false;
    }

    const QString renderer =
        QString::fromUtf8(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    if (!GLAD_GL_VERSION_4_6) {
        LOG_ERROR(Frontend, "GPU does not support OpenGL 4.6: {}", renderer.toStdString());
        QMessageBox::warning(this, tr("Error while initializing OpenGL 4.6!"),
                             tr("Your GPU may not support OpenGL 4.6, or you do not have the "
                                "latest graphics driver.<br><br>GL Renderer:<br>%1")
                                 .arg(renderer));
        return false;
    }

    QStringList unsupported_gl_extensions = GetUnsupportedGLExtensions();
    if (!unsupported_gl_extensions.empty()) {
        QMessageBox::warning(
            this, tr("Error while initializing OpenGL!"),
            tr("Your GPU may not support one or more required OpenGL extensions. Please ensure you "
               "have the latest graphics driver.<br><br>GL Renderer:<br>%1<br><br>Unsupported "
               "extensions:<br>%2")
                .arg(renderer)
                .arg(unsupported_gl_extensions.join(QStringLiteral("<br>"))));
        return false;
    }
    return true;
}

QStringList GRenderWindow::GetUnsupportedGLExtensions() const {
    QStringList unsupported_ext;

    // Extensions required to support some texture formats.
    if (!GLAD_GL_EXT_texture_compression_s3tc) {
        unsupported_ext.append(QStringLiteral("EXT_texture_compression_s3tc"));
    }
    if (!GLAD_GL_ARB_texture_compression_rgtc) {
        unsupported_ext.append(QStringLiteral("ARB_texture_compression_rgtc"));
    }

    if (!unsupported_ext.empty()) {
        const std::string gl_renderer{reinterpret_cast<const char*>(glGetString(GL_RENDERER))};
        LOG_ERROR(Frontend, "GPU does not support all required extensions: {}", gl_renderer);
    }
    for (const QString& ext : unsupported_ext) {
        LOG_ERROR(Frontend, "Unsupported GL extension: {}", ext.toStdString());
    }

    return unsupported_ext;
}

void GRenderWindow::OnEmulationStarting(EmuThread* emu_thread_) {
    emu_thread = emu_thread_;
}

void GRenderWindow::OnEmulationStopping() {
    emu_thread = nullptr;
}

void GRenderWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &GRenderWindow::OnFramebufferSizeChanged,
            Qt::UniqueConnection);
}

bool GRenderWindow::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::HoverMove) {
        if (Settings::values.mouse_panning || Settings::values.mouse_enabled) {
            auto* hover_event = static_cast<QMouseEvent*>(event);
            mouseMoveEvent(hover_event);
            return false;
        }
        emit MouseActivity();
    }
    return false;
}
