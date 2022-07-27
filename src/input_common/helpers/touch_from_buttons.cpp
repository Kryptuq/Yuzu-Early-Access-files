// SPDX-FileCopyrightText: 2020 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/settings.h"
#include "input_common/helpers/touch_from_buttons.h"

namespace InputCommon {

class TouchFromButtonDevice final : public Common::Input::InputDevice {
public:
    using Button = std::unique_ptr<Common::Input::InputDevice>;
    TouchFromButtonDevice(Button button_, int touch_id_, float x_, float y_)
        : button(std::move(button_)), touch_id(touch_id_), x(x_), y(y_) {
        last_button_value = false;
        button->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback_) {
                    UpdateButtonStatus(callback_);
                },
        });
        button->ForceUpdate();
    }

    void ForceUpdate() override {
        button->ForceUpdate();
    }

    Common::Input::TouchStatus GetStatus(bool pressed) const {
        const Common::Input::ButtonStatus button_status{
            .value = pressed,
        };
        Common::Input::TouchStatus status{
            .pressed = button_status,
            .x = {},
            .y = {},
            .id = touch_id,
        };
        status.x.properties = properties;
        status.y.properties = properties;

        if (!pressed) {
            return status;
        }

        status.x.raw_value = x;
        status.y.raw_value = y;
        return status;
    }

    void UpdateButtonStatus(const Common::Input::CallbackStatus& button_callback) {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Touch,
            .touch_status = GetStatus(button_callback.button_status.value),
        };
        if (last_button_value != button_callback.button_status.value) {
            last_button_value = button_callback.button_status.value;
            TriggerOnChange(status);
        }
    }

private:
    Button button;
    bool last_button_value;
    const int touch_id;
    const float x;
    const float y;
    const Common::Input::AnalogProperties properties{0.0f, 1.0f, 0.5f, 0.0f, false};
};

std::unique_ptr<Common::Input::InputDevice> TouchFromButton::Create(
    const Common::ParamPackage& params) {
    const std::string null_engine = Common::ParamPackage{{"engine", "null"}}.Serialize();
    auto button = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        params.Get("button", null_engine));
    const auto touch_id = params.Get("touch_id", 0);
    const float x = params.Get("x", 0.0f) / 1280.0f;
    const float y = params.Get("y", 0.0f) / 720.0f;
    return std::make_unique<TouchFromButtonDevice>(std::move(button), touch_id, x, y);
}

} // namespace InputCommon
