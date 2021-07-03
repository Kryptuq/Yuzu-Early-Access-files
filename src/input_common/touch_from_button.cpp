// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include "common/settings.h"
#include "core/frontend/framebuffer_layout.h"
#include "input_common/touch_from_button.h"

namespace InputCommon {

class TouchFromButtonDevice final : public Input::TouchDevice {
public:
    TouchFromButtonDevice() {
        const auto button_index =
            static_cast<u64>(Settings::values.touch_from_button_map_index.GetValue());
        const auto& buttons = Settings::values.touch_from_button_maps[button_index].buttons;

        for (const auto& config_entry : buttons) {
            const Common::ParamPackage package{config_entry};
            map.emplace_back(
                Input::CreateDevice<Input::ButtonDevice>(config_entry),
                std::clamp(package.Get("x", 0), 0, static_cast<int>(Layout::ScreenUndocked::Width)),
                std::clamp(package.Get("y", 0), 0,
                           static_cast<int>(Layout::ScreenUndocked::Height)));
        }
    }

    Input::TouchStatus GetStatus() const override {
        Input::TouchStatus touch_status{};
        for (std::size_t id = 0; id < map.size() && id < touch_status.size(); ++id) {
            const bool state = std::get<0>(map[id])->GetStatus();
            if (state) {
                const float x = static_cast<float>(std::get<1>(map[id])) /
                                static_cast<int>(Layout::ScreenUndocked::Width);
                const float y = static_cast<float>(std::get<2>(map[id])) /
                                static_cast<int>(Layout::ScreenUndocked::Height);
                touch_status[id] = {x, y, true};
            }
        }
        return touch_status;
    }

private:
    // A vector of the mapped button, its x and its y-coordinate
    std::vector<std::tuple<std::unique_ptr<Input::ButtonDevice>, int, int>> map;
};

std::unique_ptr<Input::TouchDevice> TouchFromButtonFactory::Create(const Common::ParamPackage&) {
    return std::make_unique<TouchFromButtonDevice>();
}

} // namespace InputCommon
