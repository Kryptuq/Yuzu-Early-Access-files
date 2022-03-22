// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <glad/glad.h>

#include <QApplication>
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
#include "core/frontend/framebuffer_layout.h"
#include "input_common/drivers/keyboard.h"
#include "input_common/drivers/mouse.h"
#include "input_common/drivers/tas_input.h"
#include "input_common/drivers/touch_screen.h"
#include "input_common/main.h"
#include "video_core/renderer_base.h"
#include "yuzu/bootmanager.h"
#include "yuzu/main.h"

EmuThread::EmuThread(Core::System& system_) : system{system_} {}

EmuThread::~EmuThread() = default;

void EmuThread::run() {
    std::string name = "yuzu:EmuControlThread";
    MicroProfileOnThreadCreate(name.c_str());
    Common::SetCurrentThreadName(name.c_str());

    auto& gpu = system.GPU();
    auto stop_token = stop_source.get_token();

    system.RegisterHostThread();

    // Main process has been loaded. Make the context current to this thread and begin GPU and CPU
    // execution.
    gpu.Start();

    gpu.ObtainContext();

    emit LoadProgress(VideoCore::LoadCallbackStage::Prepare, 0, 0);

    if (Settings::values.use_disk_shader_cache.GetValue()) {
        system.Renderer().ReadRasterizer()->LoadDiskResources(
            system.GetCurrentProcessProgramID(), stop_token,
            [this](VideoCore::LoadCallbackStage stage, std::size_t value, std::size_t total) {
                emit LoadProgress(stage, value, total);
            });
    }
    emit LoadProgress(VideoCore::LoadCallbackStage::Complete, 0, 0);

    gpu.ReleaseContext();

    // Holds whether the cpu was running during the last iteration,
    // so that the DebugModeLeft signal can be emitted before the
    // next execution step
    bool was_active = false;
    while (!stop_token.stop_requested()) {
        if (running) {
            if (was_active) {
                emit DebugModeLeft();
            }

            running_guard = true;
            Core::SystemResultStatus result = system.Run();
            if (result != Core::SystemResultStatus::Success) {
                running_guard = false;
                this->SetRunning(false);
                emit ErrorThrown(result, system.GetStatusDetails());
            }
            running_wait.Wait();
            result = system.Pause();
            if (result != Core::SystemResultStatus::Success) {
                running_guard = false;
                this->SetRunning(false);
                emit ErrorThrown(result, system.GetStatusDetails());
            }
            running_guard = false;

            if (!stop_token.stop_requested()) {
                was_active = true;
                emit DebugModeEntered();
            }
        } else if (exec_step) {
            UNIMPLEMENTED();
        } else {
            std::unique_lock lock{running_mutex};
            running_cv.wait(lock, stop_token, [this] { return IsRunning() || exec_step; });
        }
    }

    // Shutdown the core emulation
    system.Shutdown();

#if MICROPROFILE_ENABLED
    MicroProfileOnThreadExit();
#endif
}

#ifdef HAS_OPENGL
class OpenGLSharedContext : public Core::Frontend::GraphicsContext {
public:
    /// Create the original context that should be shared from
    explicit OpenGLSharedContext(QSurface* surface) : surface(surface) {
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
    }

    virtual ~RenderWidget() = default;

    QPaintEngine* paintEngine() const override {
        return nullptr;
    }

private:
    GRenderWindow* render_window;
};

class OpenGLRenderWidget : public RenderWidget {
public:
    explicit OpenGLRenderWidget(GRenderWindow* parent) : RenderWidget(parent) {
        windowHandle()->setSurfaceType(QWindow::OpenGLSurface);
    }

    void SetContext(std::unique_ptr<Core::Frontend::GraphicsContext>&& context_) {
        context = std::move(context_);
    }

private:
    std::unique_ptr<Core::Frontend::GraphicsContext> context;
};

class VulkanRenderWidget : public RenderWidget {
public:
    explicit VulkanRenderWidget(GRenderWindow* parent) : RenderWidget(parent) {
        windowHandle()->setSurfaceType(QWindow::VulkanSurface);
    }
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

    LOG_CRITICAL(Frontend, "Unknown Qt platform!");
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

void GRenderWindow::restoreGeometry(const QByteArray& geometry) {
    // Make sure users of this class don't need to deal with backing up the geometry themselves
    QWidget::restoreGeometry(geometry);
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
    switch (qt_key) {
    case Qt::Key_A:
        return Settings::NativeKeyboard::A;
    case Qt::Key_B:
        return Settings::NativeKeyboard::B;
    case Qt::Key_C:
        return Settings::NativeKeyboard::C;
    case Qt::Key_D:
        return Settings::NativeKeyboard::D;
    case Qt::Key_E:
        return Settings::NativeKeyboard::E;
    case Qt::Key_F:
        return Settings::NativeKeyboard::F;
    case Qt::Key_G:
        return Settings::NativeKeyboard::G;
    case Qt::Key_H:
        return Settings::NativeKeyboard::H;
    case Qt::Key_I:
        return Settings::NativeKeyboard::I;
    case Qt::Key_J:
        return Settings::NativeKeyboard::J;
    case Qt::Key_K:
        return Settings::NativeKeyboard::K;
    case Qt::Key_L:
        return Settings::NativeKeyboard::L;
    case Qt::Key_M:
        return Settings::NativeKeyboard::M;
    case Qt::Key_N:
        return Settings::NativeKeyboard::N;
    case Qt::Key_O:
        return Settings::NativeKeyboard::O;
    case Qt::Key_P:
        return Settings::NativeKeyboard::P;
    case Qt::Key_Q:
        return Settings::NativeKeyboard::Q;
    case Qt::Key_R:
        return Settings::NativeKeyboard::R;
    case Qt::Key_S:
        return Settings::NativeKeyboard::S;
    case Qt::Key_T:
        return Settings::NativeKeyboard::T;
    case Qt::Key_U:
        return Settings::NativeKeyboard::U;
    case Qt::Key_V:
        return Settings::NativeKeyboard::V;
    case Qt::Key_W:
        return Settings::NativeKeyboard::W;
    case Qt::Key_X:
        return Settings::NativeKeyboard::X;
    case Qt::Key_Y:
        return Settings::NativeKeyboard::Y;
    case Qt::Key_Z:
        return Settings::NativeKeyboard::Z;
    case Qt::Key_1:
        return Settings::NativeKeyboard::N1;
    case Qt::Key_2:
        return Settings::NativeKeyboard::N2;
    case Qt::Key_3:
        return Settings::NativeKeyboard::N3;
    case Qt::Key_4:
        return Settings::NativeKeyboard::N4;
    case Qt::Key_5:
        return Settings::NativeKeyboard::N5;
    case Qt::Key_6:
        return Settings::NativeKeyboard::N6;
    case Qt::Key_7:
        return Settings::NativeKeyboard::N7;
    case Qt::Key_8:
        return Settings::NativeKeyboard::N8;
    case Qt::Key_9:
        return Settings::NativeKeyboard::N9;
    case Qt::Key_0:
        return Settings::NativeKeyboard::N0;
    case Qt::Key_Return:
        return Settings::NativeKeyboard::Return;
    case Qt::Key_Escape:
        return Settings::NativeKeyboard::Escape;
    case Qt::Key_Backspace:
        return Settings::NativeKeyboard::Backspace;
    case Qt::Key_Tab:
        return Settings::NativeKeyboard::Tab;
    case Qt::Key_Space:
        return Settings::NativeKeyboard::Space;
    case Qt::Key_Minus:
        return Settings::NativeKeyboard::Minus;
    case Qt::Key_Plus:
    case Qt::Key_questiondown:
        return Settings::NativeKeyboard::Plus;
    case Qt::Key_BracketLeft:
    case Qt::Key_BraceLeft:
        return Settings::NativeKeyboard::OpenBracket;
    case Qt::Key_BracketRight:
    case Qt::Key_BraceRight:
        return Settings::NativeKeyboard::CloseBracket;
    case Qt::Key_Bar:
        return Settings::NativeKeyboard::Pipe;
    case Qt::Key_Dead_Tilde:
        return Settings::NativeKeyboard::Tilde;
    case Qt::Key_Ntilde:
    case Qt::Key_Semicolon:
        return Settings::NativeKeyboard::Semicolon;
    case Qt::Key_Apostrophe:
        return Settings::NativeKeyboard::Quote;
    case Qt::Key_Dead_Grave:
        return Settings::NativeKeyboard::Backquote;
    case Qt::Key_Comma:
        return Settings::NativeKeyboard::Comma;
    case Qt::Key_Period:
        return Settings::NativeKeyboard::Period;
    case Qt::Key_Slash:
        return Settings::NativeKeyboard::Slash;
    case Qt::Key_CapsLock:
        return Settings::NativeKeyboard::CapsLock;
    case Qt::Key_F1:
        return Settings::NativeKeyboard::F1;
    case Qt::Key_F2:
        return Settings::NativeKeyboard::F2;
    case Qt::Key_F3:
        return Settings::NativeKeyboard::F3;
    case Qt::Key_F4:
        return Settings::NativeKeyboard::F4;
    case Qt::Key_F5:
        return Settings::NativeKeyboard::F5;
    case Qt::Key_F6:
        return Settings::NativeKeyboard::F6;
    case Qt::Key_F7:
        return Settings::NativeKeyboard::F7;
    case Qt::Key_F8:
        return Settings::NativeKeyboard::F8;
    case Qt::Key_F9:
        return Settings::NativeKeyboard::F9;
    case Qt::Key_F10:
        return Settings::NativeKeyboard::F10;
    case Qt::Key_F11:
        return Settings::NativeKeyboard::F11;
    case Qt::Key_F12:
        return Settings::NativeKeyboard::F12;
    case Qt::Key_Print:
        return Settings::NativeKeyboard::PrintScreen;
    case Qt::Key_ScrollLock:
        return Settings::NativeKeyboard::ScrollLock;
    case Qt::Key_Pause:
        return Settings::NativeKeyboard::Pause;
    case Qt::Key_Insert:
        return Settings::NativeKeyboard::Insert;
    case Qt::Key_Home:
        return Settings::NativeKeyboard::Home;
    case Qt::Key_PageUp:
        return Settings::NativeKeyboard::PageUp;
    case Qt::Key_Delete:
        return Settings::NativeKeyboard::Delete;
    case Qt::Key_End:
        return Settings::NativeKeyboard::End;
    case Qt::Key_PageDown:
        return Settings::NativeKeyboard::PageDown;
    case Qt::Key_Right:
        return Settings::NativeKeyboard::Right;
    case Qt::Key_Left:
        return Settings::NativeKeyboard::Left;
    case Qt::Key_Down:
        return Settings::NativeKeyboard::Down;
    case Qt::Key_Up:
        return Settings::NativeKeyboard::Up;
    case Qt::Key_NumLock:
        return Settings::NativeKeyboard::NumLock;
    // Numpad keys are missing here
    case Qt::Key_F13:
        return Settings::NativeKeyboard::F13;
    case Qt::Key_F14:
        return Settings::NativeKeyboard::F14;
    case Qt::Key_F15:
        return Settings::NativeKeyboard::F15;
    case Qt::Key_F16:
        return Settings::NativeKeyboard::F16;
    case Qt::Key_F17:
        return Settings::NativeKeyboard::F17;
    case Qt::Key_F18:
        return Settings::NativeKeyboard::F18;
    case Qt::Key_F19:
        return Settings::NativeKeyboard::F19;
    case Qt::Key_F20:
        return Settings::NativeKeyboard::F20;
    case Qt::Key_F21:
        return Settings::NativeKeyboard::F21;
    case Qt::Key_F22:
        return Settings::NativeKeyboard::F22;
    case Qt::Key_F23:
        return Settings::NativeKeyboard::F23;
    case Qt::Key_F24:
        return Settings::NativeKeyboard::F24;
    // case Qt:::
    //    return Settings::NativeKeyboard::KPComma;
    // case Qt:::
    //    return Settings::NativeKeyboard::Ro;
    case Qt::Key_Hiragana_Katakana:
        return Settings::NativeKeyboard::KatakanaHiragana;
    case Qt::Key_yen:
        return Settings::NativeKeyboard::Yen;
    case Qt::Key_Henkan:
        return Settings::NativeKeyboard::Henkan;
    case Qt::Key_Muhenkan:
        return Settings::NativeKeyboard::Muhenkan;
    // case Qt:::
    //    return Settings::NativeKeyboard::NumPadCommaPc98;
    case Qt::Key_Hangul:
        return Settings::NativeKeyboard::HangulEnglish;
    case Qt::Key_Hangul_Hanja:
        return Settings::NativeKeyboard::Hanja;
    case Qt::Key_Katakana:
        return Settings::NativeKeyboard::KatakanaKey;
    case Qt::Key_Hiragana:
        return Settings::NativeKeyboard::HiraganaKey;
    case Qt::Key_Zenkaku_Hankaku:
        return Settings::NativeKeyboard::ZenkakuHankaku;
    // Modifier keys are handled by the modifier property
    default:
        return Settings::NativeKeyboard::None;
    }
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
        QCursor::setPos(mapToGlobal({center_x, center_y}));
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
        if (!TouchUpdate(touch_point)) {
            TouchStart(touch_point);
        }
    }
}

void GRenderWindow::TouchUpdateEvent(const QTouchEvent* event) {
    QList<QTouchEvent::TouchPoint> touch_points = event->touchPoints();
    for (const auto& touch_point : touch_points) {
        if (!TouchUpdate(touch_point)) {
            TouchStart(touch_point);
        }
    }
    // Release all inactive points
    for (std::size_t id = 0; id < touch_ids.size(); ++id) {
        if (!TouchExist(touch_ids[id], touch_points)) {
            touch_ids[id] = 0;
            input_subsystem->GetTouchScreen()->TouchReleased(id);
        }
    }
}

void GRenderWindow::TouchEndEvent() {
    for (std::size_t id = 0; id < touch_ids.size(); ++id) {
        if (touch_ids[id] != 0) {
            touch_ids[id] = 0;
            input_subsystem->GetTouchScreen()->TouchReleased(id);
        }
    }
}

void GRenderWindow::TouchStart(const QTouchEvent::TouchPoint& touch_point) {
    for (std::size_t id = 0; id < touch_ids.size(); ++id) {
        if (touch_ids[id] == 0) {
            touch_ids[id] = touch_point.id() + 1;
            const auto [x, y] = ScaleTouch(touch_point.pos());
            const auto [touch_x, touch_y] = MapToTouchScreen(x, y);
            input_subsystem->GetTouchScreen()->TouchPressed(touch_x, touch_y, id);
        }
    }
}

bool GRenderWindow::TouchUpdate(const QTouchEvent::TouchPoint& touch_point) {
    for (std::size_t id = 0; id < touch_ids.size(); ++id) {
        if (touch_ids[id] == static_cast<std::size_t>(touch_point.id() + 1)) {
            const auto [x, y] = ScaleTouch(touch_point.pos());
            const auto [touch_x, touch_y] = MapToTouchScreen(x, y);
            input_subsystem->GetTouchScreen()->TouchMoved(touch_x, touch_y, id);
            return true;
        }
    }
    return false;
}

bool GRenderWindow::TouchExist(std::size_t id,
                               const QList<QTouchEvent::TouchPoint>& touch_points) const {
    return std::any_of(touch_points.begin(), touch_points.end(), [id](const auto& point) {
        return id == static_cast<std::size_t>(point.id() + 1);
    });
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
        LOG_ERROR(Frontend, "GPU does not support all required extensions: {}",
                  glGetString(GL_RENDERER));
    }
    for (const QString& ext : unsupported_ext) {
        LOG_ERROR(Frontend, "Unsupported GL extension: {}", ext.toStdString());
    }

    return unsupported_ext;
}

void GRenderWindow::OnEmulationStarting(EmuThread* emu_thread) {
    this->emu_thread = emu_thread;
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
