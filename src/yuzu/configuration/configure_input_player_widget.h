// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <QFrame>
#include <QPointer>

#include "common/input.h"
#include "common/settings_input.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_types.h"

class QLabel;

using AnalogParam = std::array<Common::ParamPackage, Settings::NativeAnalog::NumAnalogs>;
using ButtonParam = std::array<Common::ParamPackage, Settings::NativeButton::NumButtons>;

// Widget for representing controller animations
class PlayerControlPreview : public QFrame {
    Q_OBJECT

public:
    explicit PlayerControlPreview(QWidget* parent);
    ~PlayerControlPreview() override;

    // Sets the emulated controller to be displayed
    void SetController(Core::HID::EmulatedController* controller);

    // Disables events from the emulated controller
    void UnloadController();

    // Starts blinking animation at the button specified
    void BeginMappingButton(std::size_t button_id);

    // Starts moving animation at the stick specified
    void BeginMappingAnalog(std::size_t stick_id);

    // Stops any ongoing animation
    void EndMapping();

    // Handles emulated controller events
    void ControllerUpdate(Core::HID::ControllerTriggerType type);

    // Updates input on sheduled interval
    void UpdateInput();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    enum class Direction : std::size_t {
        None,
        Up,
        Right,
        Down,
        Left,
    };

    enum class Symbol {
        House,
        A,
        B,
        X,
        Y,
        L,
        R,
        C,
        SL,
        ZL,
        ZR,
        SR,
        Charging,
    };

    struct ColorMapping {
        QColor outline{};
        QColor primary{};
        QColor left{};
        QColor right{};
        QColor button{};
        QColor button2{};
        QColor font{};
        QColor font2{};
        QColor highlight{};
        QColor highlight2{};
        QColor transparent{};
        QColor indicator{};
        QColor indicator2{};
        QColor led_on{};
        QColor led_off{};
        QColor slider{};
        QColor slider_button{};
        QColor slider_arrow{};
        QColor deadzone{};
        QColor charging{};
    };

    void UpdateColors();
    void ResetInputs();

    // Draw controller functions
    void DrawHandheldController(QPainter& p, QPointF center);
    void DrawDualController(QPainter& p, QPointF center);
    void DrawLeftController(QPainter& p, QPointF center);
    void DrawRightController(QPainter& p, QPointF center);
    void DrawProController(QPainter& p, QPointF center);
    void DrawGCController(QPainter& p, QPointF center);

    // Draw body functions
    void DrawHandheldBody(QPainter& p, QPointF center);
    void DrawDualBody(QPainter& p, QPointF center);
    void DrawLeftBody(QPainter& p, QPointF center);
    void DrawRightBody(QPainter& p, QPointF center);
    void DrawProBody(QPainter& p, QPointF center);
    void DrawGCBody(QPainter& p, QPointF center);

    // Draw triggers functions
    void DrawProTriggers(QPainter& p, QPointF center,
                         const Common::Input::ButtonStatus& left_pressed,
                         const Common::Input::ButtonStatus& right_pressed);
    void DrawGCTriggers(QPainter& p, QPointF center, Common::Input::TriggerStatus left_trigger,
                        Common::Input::TriggerStatus right_trigger);
    void DrawHandheldTriggers(QPainter& p, QPointF center,
                              const Common::Input::ButtonStatus& left_pressed,
                              const Common::Input::ButtonStatus& right_pressed);
    void DrawDualTriggers(QPainter& p, QPointF center,
                          const Common::Input::ButtonStatus& left_pressed,
                          const Common::Input::ButtonStatus& right_pressed);
    void DrawDualTriggersTopView(QPainter& p, QPointF center,
                                 const Common::Input::ButtonStatus& left_pressed,
                                 const Common::Input::ButtonStatus& right_pressed);
    void DrawDualZTriggersTopView(QPainter& p, QPointF center,
                                  const Common::Input::ButtonStatus& left_pressed,
                                  const Common::Input::ButtonStatus& right_pressed);
    void DrawLeftTriggers(QPainter& p, QPointF center,
                          const Common::Input::ButtonStatus& left_pressed);
    void DrawLeftZTriggers(QPainter& p, QPointF center,
                           const Common::Input::ButtonStatus& left_pressed);
    void DrawLeftTriggersTopView(QPainter& p, QPointF center,
                                 const Common::Input::ButtonStatus& left_pressed);
    void DrawLeftZTriggersTopView(QPainter& p, QPointF center,
                                  const Common::Input::ButtonStatus& left_pressed);
    void DrawRightTriggers(QPainter& p, QPointF center,
                           const Common::Input::ButtonStatus& right_pressed);
    void DrawRightZTriggers(QPainter& p, QPointF center,
                            const Common::Input::ButtonStatus& right_pressed);
    void DrawRightTriggersTopView(QPainter& p, QPointF center,
                                  const Common::Input::ButtonStatus& right_pressed);
    void DrawRightZTriggersTopView(QPainter& p, QPointF center,
                                   const Common::Input::ButtonStatus& right_pressed);

    // Draw joystick functions
    void DrawJoystick(QPainter& p, QPointF center, float size,
                      const Common::Input::ButtonStatus& pressed);
    void DrawJoystickSideview(QPainter& p, QPointF center, float angle, float size,
                              const Common::Input::ButtonStatus& pressed);
    void DrawRawJoystick(QPainter& p, QPointF center_left, QPointF center_right);
    void DrawJoystickProperties(QPainter& p, QPointF center,
                                const Common::Input::AnalogProperties& properties);
    void DrawJoystickDot(QPainter& p, QPointF center, const Common::Input::StickStatus& stick,
                         bool raw);
    void DrawProJoystick(QPainter& p, QPointF center, QPointF offset, float scalar,
                         const Common::Input::ButtonStatus& pressed);
    void DrawGCJoystick(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed);

    // Draw button functions
    void DrawCircleButton(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed,
                          float button_size);
    void DrawRoundButton(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed,
                         float width, float height, Direction direction = Direction::None,
                         float radius = 2);
    void DrawMinusButton(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed,
                         int button_size);
    void DrawPlusButton(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed,
                        int button_size);
    void DrawGCButtonX(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed);
    void DrawGCButtonY(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed);
    void DrawGCButtonZ(QPainter& p, QPointF center, const Common::Input::ButtonStatus& pressed);
    void DrawArrowButtonOutline(QPainter& p, const QPointF center, float size = 1.0f);
    void DrawArrowButton(QPainter& p, QPointF center, Direction direction,
                         const Common::Input::ButtonStatus& pressed, float size = 1.0f);
    void DrawTriggerButton(QPainter& p, QPointF center, Direction direction,
                           const Common::Input::ButtonStatus& pressed);

    // Draw battery functions
    void DrawBattery(QPainter& p, QPointF center, Common::Input::BatteryLevel battery);

    // Draw icon functions
    void DrawSymbol(QPainter& p, QPointF center, Symbol symbol, float icon_size);
    void DrawArrow(QPainter& p, QPointF center, Direction direction, float size);

    // Draw primitive types
    template <size_t N>
    void DrawPolygon(QPainter& p, const std::array<QPointF, N>& polygon);
    void DrawCircle(QPainter& p, QPointF center, float size);
    void DrawRectangle(QPainter& p, QPointF center, float width, float height);
    void DrawRoundRectangle(QPainter& p, QPointF center, float width, float height, float round);
    void DrawText(QPainter& p, QPointF center, float text_size, const QString& text);
    void SetTextFont(QPainter& p, float text_size,
                     const QString& font_family = QStringLiteral("sans-serif"));

    bool is_controller_set{};
    bool is_connected{};
    bool needs_redraw{};
    Core::HID::NpadStyleIndex controller_type;

    bool mapping_active{};
    int blink_counter{};
    int callback_key;
    QColor button_color{};
    ColorMapping colors{};
    Core::HID::LedPattern led_pattern{0, 0, 0, 0};
    std::size_t player_index{};
    Core::HID::EmulatedController* controller;
    std::size_t button_mapping_index{Settings::NativeButton::NumButtons};
    std::size_t analog_mapping_index{Settings::NativeAnalog::NumAnalogs};
    Core::HID::ButtonValues button_values{};
    Core::HID::SticksValues stick_values{};
    Core::HID::TriggerValues trigger_values{};
    Core::HID::BatteryValues battery_values{};
};
