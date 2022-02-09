// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include <functional>
#include <mutex>
#include <unordered_map>

#include "common/common_types.h"
#include "common/input.h"
#include "common/param_package.h"
#include "common/uuid.h"
#include "input_common/main.h"

// Pad Identifier of data source
struct PadIdentifier {
    Common::UUID guid{};
    std::size_t port{};
    std::size_t pad{};

    friend constexpr bool operator==(const PadIdentifier&, const PadIdentifier&) = default;
};

// Basic motion data containing data from the sensors and a timestamp in microseconds
struct BasicMotion {
    float gyro_x{};
    float gyro_y{};
    float gyro_z{};
    float accel_x{};
    float accel_y{};
    float accel_z{};
    u64 delta_timestamp{};
};

// Stages of a battery charge
enum class BatteryLevel {
    Empty,
    Critical,
    Low,
    Medium,
    Full,
    Charging,
};

// Types of input that are stored in the engine
enum class EngineInputType {
    None,
    Button,
    HatButton,
    Analog,
    Motion,
    Battery,
};

namespace std {
// Hash used to create lists from PadIdentifier data
template <>
struct hash<PadIdentifier> {
    size_t operator()(const PadIdentifier& pad_id) const noexcept {
        u64 hash_value = pad_id.guid.Hash();
        hash_value ^= (static_cast<u64>(pad_id.port) << 32);
        hash_value ^= static_cast<u64>(pad_id.pad);
        return static_cast<size_t>(hash_value);
    }
};

} // namespace std

namespace InputCommon {

// Data from the engine and device needed for creating a ParamPackage
struct MappingData {
    std::string engine{};
    PadIdentifier pad{};
    EngineInputType type{};
    int index{};
    bool button_value{};
    std::string hat_name{};
    f32 axis_value{};
    BasicMotion motion_value{};
};

// Triggered if data changed on the controller
struct UpdateCallback {
    std::function<void()> on_change;
};

// Triggered if data changed on the controller and the engine is on configuring mode
struct MappingCallback {
    std::function<void(const MappingData&)> on_data;
};

// Input Identifier of data source
struct InputIdentifier {
    PadIdentifier identifier;
    EngineInputType type;
    int index;
    UpdateCallback callback;
};

class InputEngine {
public:
    explicit InputEngine(std::string input_engine_) : input_engine{std::move(input_engine_)} {}

    virtual ~InputEngine() = default;

    // Enable configuring mode for mapping
    void BeginConfiguration();

    // Disable configuring mode for mapping
    void EndConfiguration();

    // Sets a led pattern for a controller
    virtual void SetLeds([[maybe_unused]] const PadIdentifier& identifier,
                         [[maybe_unused]] const Common::Input::LedStatus& led_status) {}

    // Sets rumble to a controller
    virtual Common::Input::VibrationError SetRumble(
        [[maybe_unused]] const PadIdentifier& identifier,
        [[maybe_unused]] const Common::Input::VibrationStatus& vibration) {
        return Common::Input::VibrationError::NotSupported;
    }

    // Sets polling mode to a controller
    virtual Common::Input::PollingError SetPollingMode(
        [[maybe_unused]] const PadIdentifier& identifier,
        [[maybe_unused]] const Common::Input::PollingMode vibration) {
        return Common::Input::PollingError::NotSupported;
    }

    // Returns the engine name
    [[nodiscard]] const std::string& GetEngineName() const;

    /// Used for automapping features
    virtual std::vector<Common::ParamPackage> GetInputDevices() const {
        return {};
    }

    /// Retrieves the button mappings for the given device
    virtual ButtonMapping GetButtonMappingForDevice(
        [[maybe_unused]] const Common::ParamPackage& params) {
        return {};
    }

    /// Retrieves the analog mappings for the given device
    virtual AnalogMapping GetAnalogMappingForDevice(
        [[maybe_unused]] const Common::ParamPackage& params) {
        return {};
    }

    /// Retrieves the motion mappings for the given device
    virtual MotionMapping GetMotionMappingForDevice(
        [[maybe_unused]] const Common::ParamPackage& params) {
        return {};
    }

    /// Retrieves the name of the given input.
    virtual Common::Input::ButtonNames GetUIName(
        [[maybe_unused]] const Common::ParamPackage& params) const {
        return Common::Input::ButtonNames::Engine;
    }

    /// Retrieves the index number of the given hat button direction
    virtual u8 GetHatButtonId([[maybe_unused]] const std::string& direction_name) const {
        return 0;
    }

    void PreSetController(const PadIdentifier& identifier);
    void PreSetButton(const PadIdentifier& identifier, int button);
    void PreSetHatButton(const PadIdentifier& identifier, int button);
    void PreSetAxis(const PadIdentifier& identifier, int axis);
    void PreSetMotion(const PadIdentifier& identifier, int motion);
    void ResetButtonState();
    void ResetAnalogState();

    bool GetButton(const PadIdentifier& identifier, int button) const;
    bool GetHatButton(const PadIdentifier& identifier, int button, u8 direction) const;
    f32 GetAxis(const PadIdentifier& identifier, int axis) const;
    BatteryLevel GetBattery(const PadIdentifier& identifier) const;
    BasicMotion GetMotion(const PadIdentifier& identifier, int motion) const;

    int SetCallback(InputIdentifier input_identifier);
    void SetMappingCallback(MappingCallback callback);
    void DeleteCallback(int key);

protected:
    void SetButton(const PadIdentifier& identifier, int button, bool value);
    void SetHatButton(const PadIdentifier& identifier, int button, u8 value);
    void SetAxis(const PadIdentifier& identifier, int axis, f32 value);
    void SetBattery(const PadIdentifier& identifier, BatteryLevel value);
    void SetMotion(const PadIdentifier& identifier, int motion, const BasicMotion& value);

    virtual std::string GetHatButtonName([[maybe_unused]] u8 direction_value) const {
        return "Unknown";
    }

private:
    struct ControllerData {
        std::unordered_map<int, bool> buttons;
        std::unordered_map<int, u8> hat_buttons;
        std::unordered_map<int, float> axes;
        std::unordered_map<int, BasicMotion> motions;
        BatteryLevel battery{};
    };

    void TriggerOnButtonChange(const PadIdentifier& identifier, int button, bool value);
    void TriggerOnHatButtonChange(const PadIdentifier& identifier, int button, u8 value);
    void TriggerOnAxisChange(const PadIdentifier& identifier, int axis, f32 value);
    void TriggerOnBatteryChange(const PadIdentifier& identifier, BatteryLevel value);
    void TriggerOnMotionChange(const PadIdentifier& identifier, int motion,
                               const BasicMotion& value);

    bool IsInputIdentifierEqual(const InputIdentifier& input_identifier,
                                const PadIdentifier& identifier, EngineInputType type,
                                int index) const;

    mutable std::mutex mutex;
    mutable std::recursive_mutex mutex_callback;
    bool configuring{false};
    const std::string input_engine;
    int last_callback_key = 0;
    std::unordered_map<PadIdentifier, ControllerData> controller_list;
    std::unordered_map<int, InputIdentifier> callback_list;
    MappingCallback mapping_callback;
};

} // namespace InputCommon
