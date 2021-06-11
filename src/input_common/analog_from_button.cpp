// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include "common/math_util.h"
#include "common/settings.h"
#include "input_common/analog_from_button.h"

namespace InputCommon {

class Analog final : public Input::AnalogDevice {
public:
    using Button = std::unique_ptr<Input::ButtonDevice>;

    Analog(Button up_, Button down_, Button left_, Button right_, Button modifier_,
           float modifier_scale_, float modifier_angle_)
        : up(std::move(up_)), down(std::move(down_)), left(std::move(left_)),
          right(std::move(right_)), modifier(std::move(modifier_)), modifier_scale(modifier_scale_),
          modifier_angle(modifier_angle_) {
        Input::InputCallback<bool> callbacks{
            [this]([[maybe_unused]] bool status) { UpdateStatus(); }};
        up->SetCallback(callbacks);
        down->SetCallback(callbacks);
        left->SetCallback(callbacks);
        right->SetCallback(callbacks);
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

    void UpdateStatus() {
        const float coef = modifier->GetStatus() ? modifier_scale : 1.0f;

        bool r = right->GetStatus();
        bool l = left->GetStatus();
        bool u = up->GetStatus();
        bool d = down->GetStatus();

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
    }

    std::tuple<float, float> GetStatus() const override {
        if (Settings::values.emulate_analog_keyboard) {
            const auto now = std::chrono::steady_clock::now();
            float angle_ = GetAngle(now);
            return std::make_tuple(std::cos(angle_) * amplitude, std::sin(angle_) * amplitude);
        }
        constexpr float SQRT_HALF = 0.707106781f;
        int x = 0, y = 0;
        if (right->GetStatus()) {
            ++x;
        }
        if (left->GetStatus()) {
            --x;
        }
        if (up->GetStatus()) {
            ++y;
        }
        if (down->GetStatus()) {
            --y;
        }
        const float coef = modifier->GetStatus() ? modifier_scale : 1.0f;
        return std::make_tuple(static_cast<float>(x) * coef * (y == 0 ? 1.0f : SQRT_HALF),
                               static_cast<float>(y) * coef * (x == 0 ? 1.0f : SQRT_HALF));
    }

    Input::AnalogProperties GetAnalogProperties() const override {
        return {modifier_scale, 1.0f, 0.5f};
    }

    bool GetAnalogDirectionStatus(Input::AnalogDirection direction) const override {
        switch (direction) {
        case Input::AnalogDirection::RIGHT:
            return right->GetStatus();
        case Input::AnalogDirection::LEFT:
            return left->GetStatus();
        case Input::AnalogDirection::UP:
            return up->GetStatus();
        case Input::AnalogDirection::DOWN:
            return down->GetStatus();
        }
        return false;
    }

private:
    Button up;
    Button down;
    Button left;
    Button right;
    Button modifier;
    float modifier_scale;
    float modifier_angle;
    float angle{};
    float goal_angle{};
    float amplitude{};
    std::chrono::time_point<std::chrono::steady_clock> last_update;
};

std::unique_ptr<Input::AnalogDevice> AnalogFromButton::Create(const Common::ParamPackage& params) {
    const std::string null_engine = Common::ParamPackage{{"engine", "null"}}.Serialize();
    auto up = Input::CreateDevice<Input::ButtonDevice>(params.Get("up", null_engine));
    auto down = Input::CreateDevice<Input::ButtonDevice>(params.Get("down", null_engine));
    auto left = Input::CreateDevice<Input::ButtonDevice>(params.Get("left", null_engine));
    auto right = Input::CreateDevice<Input::ButtonDevice>(params.Get("right", null_engine));
    auto modifier = Input::CreateDevice<Input::ButtonDevice>(params.Get("modifier", null_engine));
    auto modifier_scale = params.Get("modifier_scale", 0.5f);
    auto modifier_angle = params.Get("modifier_angle", 5.5f);
    return std::make_unique<Analog>(std::move(up), std::move(down), std::move(left),
                                    std::move(right), std::move(modifier), modifier_scale,
                                    modifier_angle);
}

} // namespace InputCommon
