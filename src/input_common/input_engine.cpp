// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/logging/log.h"
#include "common/param_package.h"
#include "input_common/input_engine.h"

namespace InputCommon {

void InputEngine::PreSetController(const PadIdentifier& identifier) {
    std::lock_guard lock{mutex};
    if (!controller_list.contains(identifier)) {
        controller_list.insert_or_assign(identifier, ControllerData{});
    }
}

void InputEngine::PreSetButton(const PadIdentifier& identifier, int button) {
    std::lock_guard lock{mutex};
    ControllerData& controller = controller_list.at(identifier);
    if (!controller.buttons.contains(button)) {
        controller.buttons.insert_or_assign(button, false);
    }
}

void InputEngine::PreSetHatButton(const PadIdentifier& identifier, int button) {
    std::lock_guard lock{mutex};
    ControllerData& controller = controller_list.at(identifier);
    if (!controller.hat_buttons.contains(button)) {
        controller.hat_buttons.insert_or_assign(button, u8{0});
    }
}

void InputEngine::PreSetAxis(const PadIdentifier& identifier, int axis) {
    std::lock_guard lock{mutex};
    ControllerData& controller = controller_list.at(identifier);
    if (!controller.axes.contains(axis)) {
        controller.axes.insert_or_assign(axis, 0.0f);
    }
}

void InputEngine::PreSetMotion(const PadIdentifier& identifier, int motion) {
    std::lock_guard lock{mutex};
    ControllerData& controller = controller_list.at(identifier);
    if (!controller.motions.contains(motion)) {
        controller.motions.insert_or_assign(motion, BasicMotion{});
    }
}

void InputEngine::SetButton(const PadIdentifier& identifier, int button, bool value) {
    {
        std::lock_guard lock{mutex};
        ControllerData& controller = controller_list.at(identifier);
        if (!configuring) {
            controller.buttons.insert_or_assign(button, value);
        }
    }
    TriggerOnButtonChange(identifier, button, value);
}

void InputEngine::SetHatButton(const PadIdentifier& identifier, int button, u8 value) {
    {
        std::lock_guard lock{mutex};
        ControllerData& controller = controller_list.at(identifier);
        if (!configuring) {
            controller.hat_buttons.insert_or_assign(button, value);
        }
    }
    TriggerOnHatButtonChange(identifier, button, value);
}

void InputEngine::SetAxis(const PadIdentifier& identifier, int axis, f32 value) {
    {
        std::lock_guard lock{mutex};
        ControllerData& controller = controller_list.at(identifier);
        if (!configuring) {
            controller.axes.insert_or_assign(axis, value);
        }
    }
    TriggerOnAxisChange(identifier, axis, value);
}

void InputEngine::SetBattery(const PadIdentifier& identifier, BatteryLevel value) {
    {
        std::lock_guard lock{mutex};
        ControllerData& controller = controller_list.at(identifier);
        if (!configuring) {
            controller.battery = value;
        }
    }
    TriggerOnBatteryChange(identifier, value);
}

void InputEngine::SetMotion(const PadIdentifier& identifier, int motion, BasicMotion value) {
    {
        std::lock_guard lock{mutex};
        ControllerData& controller = controller_list.at(identifier);
        if (!configuring) {
            controller.motions.insert_or_assign(motion, value);
        }
    }
    TriggerOnMotionChange(identifier, motion, value);
}

bool InputEngine::GetButton(const PadIdentifier& identifier, int button) const {
    std::lock_guard lock{mutex};
    if (!controller_list.contains(identifier)) {
        LOG_ERROR(Input, "Invalid identifier guid={}, pad={}, port={}", identifier.guid.Format(),
                  identifier.pad, identifier.port);
        return false;
    }
    ControllerData controller = controller_list.at(identifier);
    if (!controller.buttons.contains(button)) {
        LOG_ERROR(Input, "Invalid button {}", button);
        return false;
    }
    return controller.buttons.at(button);
}

bool InputEngine::GetHatButton(const PadIdentifier& identifier, int button, u8 direction) const {
    std::lock_guard lock{mutex};
    if (!controller_list.contains(identifier)) {
        LOG_ERROR(Input, "Invalid identifier guid={}, pad={}, port={}", identifier.guid.Format(),
                  identifier.pad, identifier.port);
        return false;
    }
    ControllerData controller = controller_list.at(identifier);
    if (!controller.hat_buttons.contains(button)) {
        LOG_ERROR(Input, "Invalid hat button {}", button);
        return false;
    }
    return (controller.hat_buttons.at(button) & direction) != 0;
}

f32 InputEngine::GetAxis(const PadIdentifier& identifier, int axis) const {
    std::lock_guard lock{mutex};
    if (!controller_list.contains(identifier)) {
        LOG_ERROR(Input, "Invalid identifier guid={}, pad={}, port={}", identifier.guid.Format(),
                  identifier.pad, identifier.port);
        return 0.0f;
    }
    ControllerData controller = controller_list.at(identifier);
    if (!controller.axes.contains(axis)) {
        LOG_ERROR(Input, "Invalid axis {}", axis);
        return 0.0f;
    }
    return controller.axes.at(axis);
}

BatteryLevel InputEngine::GetBattery(const PadIdentifier& identifier) const {
    std::lock_guard lock{mutex};
    if (!controller_list.contains(identifier)) {
        LOG_ERROR(Input, "Invalid identifier guid={}, pad={}, port={}", identifier.guid.Format(),
                  identifier.pad, identifier.port);
        return BatteryLevel::Charging;
    }
    ControllerData controller = controller_list.at(identifier);
    return controller.battery;
}

BasicMotion InputEngine::GetMotion(const PadIdentifier& identifier, int motion) const {
    std::lock_guard lock{mutex};
    if (!controller_list.contains(identifier)) {
        LOG_ERROR(Input, "Invalid identifier guid={}, pad={}, port={}", identifier.guid.Format(),
                  identifier.pad, identifier.port);
        return {};
    }
    ControllerData controller = controller_list.at(identifier);
    return controller.motions.at(motion);
}

void InputEngine::ResetButtonState() {
    for (std::pair<PadIdentifier, ControllerData> controller : controller_list) {
        for (std::pair<int, bool> button : controller.second.buttons) {
            SetButton(controller.first, button.first, false);
        }
        for (std::pair<int, bool> button : controller.second.hat_buttons) {
            SetHatButton(controller.first, button.first, false);
        }
    }
}

void InputEngine::ResetAnalogState() {
    for (std::pair<PadIdentifier, ControllerData> controller : controller_list) {
        for (std::pair<int, float> axis : controller.second.axes) {
            SetAxis(controller.first, axis.first, 0.0);
        }
    }
}

void InputEngine::TriggerOnButtonChange(const PadIdentifier& identifier, int button, bool value) {
    std::lock_guard lock{mutex_callback};
    for (const std::pair<int, InputIdentifier> poller_pair : callback_list) {
        const InputIdentifier& poller = poller_pair.second;
        if (!IsInputIdentifierEqual(poller, identifier, EngineInputType::Button, button)) {
            continue;
        }
        if (poller.callback.on_change) {
            poller.callback.on_change();
        }
    }
    if (!configuring || !mapping_callback.on_data) {
        return;
    }

    PreSetButton(identifier, button);
    if (value == GetButton(identifier, button)) {
        return;
    }
    mapping_callback.on_data(MappingData{
        .engine = GetEngineName(),
        .pad = identifier,
        .type = EngineInputType::Button,
        .index = button,
        .button_value = value,
    });
}

void InputEngine::TriggerOnHatButtonChange(const PadIdentifier& identifier, int button, u8 value) {
    std::lock_guard lock{mutex_callback};
    for (const std::pair<int, InputIdentifier> poller_pair : callback_list) {
        const InputIdentifier& poller = poller_pair.second;
        if (!IsInputIdentifierEqual(poller, identifier, EngineInputType::HatButton, button)) {
            continue;
        }
        if (poller.callback.on_change) {
            poller.callback.on_change();
        }
    }
    if (!configuring || !mapping_callback.on_data) {
        return;
    }
    for (std::size_t index = 1; index < 0xff; index <<= 1) {
        bool button_value = (value & index) != 0;
        if (button_value == GetHatButton(identifier, button, static_cast<u8>(index))) {
            continue;
        }
        mapping_callback.on_data(MappingData{
            .engine = GetEngineName(),
            .pad = identifier,
            .type = EngineInputType::HatButton,
            .index = button,
            .hat_name = GetHatButtonName(static_cast<u8>(index)),
        });
    }
}

void InputEngine::TriggerOnAxisChange(const PadIdentifier& identifier, int axis, f32 value) {
    std::lock_guard lock{mutex_callback};
    for (const std::pair<int, InputIdentifier> poller_pair : callback_list) {
        const InputIdentifier& poller = poller_pair.second;
        if (!IsInputIdentifierEqual(poller, identifier, EngineInputType::Analog, axis)) {
            continue;
        }
        if (poller.callback.on_change) {
            poller.callback.on_change();
        }
    }
    if (!configuring || !mapping_callback.on_data) {
        return;
    }
    if (std::abs(value - GetAxis(identifier, axis)) < 0.5f) {
        return;
    }
    mapping_callback.on_data(MappingData{
        .engine = GetEngineName(),
        .pad = identifier,
        .type = EngineInputType::Analog,
        .index = axis,
        .axis_value = value,
    });
}

void InputEngine::TriggerOnBatteryChange(const PadIdentifier& identifier,
                                         [[maybe_unused]] BatteryLevel value) {
    std::lock_guard lock{mutex_callback};
    for (const std::pair<int, InputIdentifier> poller_pair : callback_list) {
        const InputIdentifier& poller = poller_pair.second;
        if (!IsInputIdentifierEqual(poller, identifier, EngineInputType::Battery, 0)) {
            continue;
        }
        if (poller.callback.on_change) {
            poller.callback.on_change();
        }
    }
}

void InputEngine::TriggerOnMotionChange(const PadIdentifier& identifier, int motion,
                                        BasicMotion value) {
    std::lock_guard lock{mutex_callback};
    for (const std::pair<int, InputIdentifier> poller_pair : callback_list) {
        const InputIdentifier& poller = poller_pair.second;
        if (!IsInputIdentifierEqual(poller, identifier, EngineInputType::Motion, motion)) {
            continue;
        }
        if (poller.callback.on_change) {
            poller.callback.on_change();
        }
    }
    if (!configuring || !mapping_callback.on_data) {
        return;
    }
    if (std::abs(value.gyro_x) < 0.6f && std::abs(value.gyro_y) < 0.6f &&
        std::abs(value.gyro_z) < 0.6f) {
        return;
    }
    mapping_callback.on_data(MappingData{
        .engine = GetEngineName(),
        .pad = identifier,
        .type = EngineInputType::Motion,
        .index = motion,
        .motion_value = value,
    });
}

bool InputEngine::IsInputIdentifierEqual(const InputIdentifier& input_identifier,
                                         const PadIdentifier& identifier, EngineInputType type,
                                         int index) const {
    if (input_identifier.type != type) {
        return false;
    }
    if (input_identifier.index != index) {
        return false;
    }
    if (input_identifier.identifier != identifier) {
        return false;
    }
    return true;
}

void InputEngine::BeginConfiguration() {
    configuring = true;
}

void InputEngine::EndConfiguration() {
    configuring = false;
}

const std::string& InputEngine::GetEngineName() const {
    return input_engine;
}

int InputEngine::SetCallback(InputIdentifier input_identifier) {
    std::lock_guard lock{mutex_callback};
    callback_list.insert_or_assign(last_callback_key, input_identifier);
    return last_callback_key++;
}

void InputEngine::SetMappingCallback(MappingCallback callback) {
    std::lock_guard lock{mutex_callback};
    mapping_callback = std::move(callback);
}

void InputEngine::DeleteCallback(int key) {
    std::lock_guard lock{mutex_callback};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}

} // namespace InputCommon
