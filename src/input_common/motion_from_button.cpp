// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "input_common/motion_from_button.h"
#include "input_common/motion_input.h"

namespace InputCommon {

class MotionKey final : public Input::MotionDevice {
public:
    using Button = std::unique_ptr<Input::ButtonDevice>;

    explicit MotionKey(Button key_) : key(std::move(key_)) {}

    Input::MotionStatus GetStatus() const override {

        if (key->GetStatus()) {
            return motion.GetRandomMotion(2, 6);
        }
        return motion.GetRandomMotion(0, 0);
    }

private:
    Button key;
    InputCommon::MotionInput motion{0.0f, 0.0f, 0.0f};
};

std::unique_ptr<Input::MotionDevice> MotionFromButton::Create(const Common::ParamPackage& params) {
    auto key = Input::CreateDevice<Input::ButtonDevice>(params.Serialize());
    return std::make_unique<MotionKey>(std::move(key));
}

} // namespace InputCommon
