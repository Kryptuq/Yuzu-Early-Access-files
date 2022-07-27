// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cmath>
#include "common/math_util.h"
#include "common/settings.h"
#include "input_common/helpers/stick_from_buttons.h"

namespace InputCommon {

class Stick final : public Common::Input::InputDevice {
public:
    using Button = std::unique_ptr<Common::Input::InputDevice>;

    Stick(Button up_, Button down_, Button left_, Button right_, Button modifier_,
          float modifier_scale_, float modifier_angle_)
        : up(std::move(up_)), down(std::move(down_)), left(std::move(left_)),
          right(std::move(right_)), modifier(std::move(modifier_)), modifier_scale(modifier_scale_),
          modifier_angle(modifier_angle_) {
        up->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback_) {
                    UpdateUpButtonStatus(callback_);
                },
        });
        down->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback_) {
                    UpdateDownButtonStatus(callback_);
                },
        });
        left->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback_) {
                    UpdateLeftButtonStatus(callback_);
                },
        });
        right->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback_) {
                    UpdateRightButtonStatus(callback_);
                },
        });
        modifier->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback_) {
                    UpdateModButtonStatus(callback_);
                },
        });
        last_x_axis_value = 0.0f;
        last_y_axis_value = 0.0f;
    }

    bool IsAngleGreater(float old_angle, float new_angle) const {
        constexpr float TAU = Common::PI * 2.0f;
        // Use wider angle to ease the transition.
        constexpr float aperture = TAU * 0.15f;
        const float top_limit = new_angle + aperture;
        return (old_angle > new_angle && old_angle <= top_limit) ||
               (old_angle + TAU > new_angle && old_angle + TAU <= top_limit);
    }

    bool IsAngleSmaller(float old_angle, float new_angle) const {
        constexpr float TAU = Common::PI * 2.0f;
        // Use wider angle to ease the transition.
        constexpr float aperture = TAU * 0.15f;
        const float bottom_limit = new_angle - aperture;
        return (old_angle >= bottom_limit && old_angle < new_angle) ||
               (old_angle - TAU >= bottom_limit && old_angle - TAU < new_angle);
    }

    float GetAngle(std::chrono::time_point<std::chrono::steady_clock> now) const {
        constexpr float TAU = Common::PI * 2.0f;
        float new_angle = angle;

        auto time_difference = static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(now - last_update).count());
        time_difference /= 1000.0f * 1000.0f;
        if (time_difference > 0.5f) {
            time_difference = 0.5f;
        }

        if (IsAngleGreater(new_angle, goal_angle)) {
            new_angle -= modifier_angle * time_difference;
            if (new_angle < 0) {
                new_angle += TAU;
            }
            if (!IsAngleGreater(new_angle, goal_angle)) {
                return goal_angle;
            }
        } else if (IsAngleSmaller(new_angle, goal_angle)) {
            new_angle += modifier_angle * time_difference;
            if (new_angle >= TAU) {
                new_angle -= TAU;
            }
            if (!IsAngleSmaller(new_angle, goal_angle)) {
                return goal_angle;
            }
        } else {
            return goal_angle;
        }
        return new_angle;
    }

    void SetGoalAngle(bool r, bool l, bool u, bool d) {
        // Move to the right
        if (r && !u && !d) {
            goal_angle = 0.0f;
        }

        // Move to the upper right
        if (r && u && !d) {
            goal_angle = Common::PI * 0.25f;
        }

        // Move up
        if (u && !l && !r) {
            goal_angle = Common::PI * 0.5f;
        }

        // Move to the upper left
        if (l && u && !d) {
            goal_angle = Common::PI * 0.75f;
        }

        // Move to the left
        if (l && !u && !d) {
            goal_angle = Common::PI;
        }

        // Move to the bottom left
        if (l && !u && d) {
            goal_angle = Common::PI * 1.25f;
        }

        // Move down
        if (d && !l && !r) {
            goal_angle = Common::PI * 1.5f;
        }

        // Move to the bottom right
        if (r && !u && d) {
            goal_angle = Common::PI * 1.75f;
        }
    }

    void UpdateUpButtonStatus(const Common::Input::CallbackStatus& button_callback) {
        up_status = button_callback.button_status.value;
        UpdateStatus();
    }

    void UpdateDownButtonStatus(const Common::Input::CallbackStatus& button_callback) {
        down_status = button_callback.button_status.value;
        UpdateStatus();
    }

    void UpdateLeftButtonStatus(const Common::Input::CallbackStatus& button_callback) {
        left_status = button_callback.button_status.value;
        UpdateStatus();
    }

    void UpdateRightButtonStatus(const Common::Input::CallbackStatus& button_callback) {
        right_status = button_callback.button_status.value;
        UpdateStatus();
    }

    void UpdateModButtonStatus(const Common::Input::CallbackStatus& button_callback) {
        const auto& new_status = button_callback.button_status;
        const bool new_button_value = new_status.inverted ? !new_status.value : new_status.value;
        modifier_status.toggle = new_status.toggle;

        // Update button status with current
        if (!modifier_status.toggle) {
            modifier_status.locked = false;
            if (modifier_status.value != new_button_value) {
                modifier_status.value = new_button_value;
            }
        } else {
            // Toggle button and lock status
            if (new_button_value && !modifier_status.locked) {
                modifier_status.locked = true;
                modifier_status.value = !modifier_status.value;
            }

            // Unlock button ready for next press
            if (!new_button_value && modifier_status.locked) {
                modifier_status.locked = false;
            }
        }

        UpdateStatus();
    }

    void UpdateStatus() {
        const float coef = modifier_status.value ? modifier_scale : 1.0f;

        bool r = right_status;
        bool l = left_status;
        bool u = up_status;
        bool d = down_status;

        // Eliminate contradictory movements
        if (r && l) {
            r = false;
            l = false;
        }
        if (u && d) {
            u = false;
            d = false;
        }

        // Move if a key is pressed
        if (r || l || u || d) {
            amplitude = coef;
        } else {
            amplitude = 0;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto time_difference = static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count());

        if (time_difference < 10) {
            // Disable analog mode if inputs are too fast
            SetGoalAngle(r, l, u, d);
            angle = goal_angle;
        } else {
            angle = GetAngle(now);
            SetGoalAngle(r, l, u, d);
        }

        last_update = now;
        Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Stick,
            .stick_status = GetStatus(),
        };
        last_x_axis_value = status.stick_status.x.raw_value;
        last_y_axis_value = status.stick_status.y.raw_value;
        TriggerOnChange(status);
    }

    void ForceUpdate() override {
        up->ForceUpdate();
        down->ForceUpdate();
        left->ForceUpdate();
        right->ForceUpdate();
        modifier->ForceUpdate();
    }

    void SoftUpdate() override {
        Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Stick,
            .stick_status = GetStatus(),
        };
        if (last_x_axis_value == status.stick_status.x.raw_value &&
            last_y_axis_value == status.stick_status.y.raw_value) {
            return;
        }
        last_x_axis_value = status.stick_status.x.raw_value;
        last_y_axis_value = status.stick_status.y.raw_value;
        TriggerOnChange(status);
    }

    Common::Input::StickStatus GetStatus() const {
        Common::Input::StickStatus status{};
        status.x.properties = properties;
        status.y.properties = properties;
        if (Settings::values.emulate_analog_keyboard) {
            const auto now = std::chrono::steady_clock::now();
            float angle_ = GetAngle(now);
            status.x.raw_value = std::cos(angle_) * amplitude;
            status.y.raw_value = std::sin(angle_) * amplitude;
            return status;
        }
        constexpr float SQRT_HALF = 0.707106781f;
        int x = 0, y = 0;
        if (right_status) {
            ++x;
        }
        if (left_status) {
            --x;
        }
        if (up_status) {
            ++y;
        }
        if (down_status) {
            --y;
        }
        const float coef = modifier_status.value ? modifier_scale : 1.0f;
        status.x.raw_value = static_cast<float>(x) * coef * (y == 0 ? 1.0f : SQRT_HALF);
        status.y.raw_value = static_cast<float>(y) * coef * (x == 0 ? 1.0f : SQRT_HALF);
        return status;
    }

private:
    Button up;
    Button down;
    Button left;
    Button right;
    Button modifier;
    float modifier_scale{};
    float modifier_angle{};
    float angle{};
    float goal_angle{};
    float amplitude{};
    bool up_status{};
    bool down_status{};
    bool left_status{};
    bool right_status{};
    float last_x_axis_value{};
    float last_y_axis_value{};
    Common::Input::ButtonStatus modifier_status{};
    const Common::Input::AnalogProperties properties{0.0f, 1.0f, 0.5f, 0.0f, false};
    std::chrono::time_point<std::chrono::steady_clock> last_update;
};

std::unique_ptr<Common::Input::InputDevice> StickFromButton::Create(
    const Common::ParamPackage& params) {
    const std::string null_engine = Common::ParamPackage{{"engine", "null"}}.Serialize();
    auto up = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        params.Get("up", null_engine));
    auto down = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        params.Get("down", null_engine));
    auto left = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        params.Get("left", null_engine));
    auto right = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        params.Get("right", null_engine));
    auto modifier = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        params.Get("modifier", null_engine));
    auto modifier_scale = params.Get("modifier_scale", 0.5f);
    auto modifier_angle = params.Get("modifier_angle", 5.5f);
    return std::make_unique<Stick>(std::move(up), std::move(down), std::move(left),
                                   std::move(right), std::move(modifier), modifier_scale,
                                   modifier_angle);
}

} // namespace InputCommon
