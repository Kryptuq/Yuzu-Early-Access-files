// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/param_package.h"
#include "common/settings.h"
#include "common/thread.h"
#include "common/vector_math.h"
#include "input_common/drivers/sdl_driver.h"

namespace InputCommon {

namespace {
std::string GetGUID(SDL_Joystick* joystick) {
    const SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
    char guid_str[33];
    SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));
    return guid_str;
}
} // Anonymous namespace

static int SDLEventWatcher(void* user_data, SDL_Event* event) {
    auto* const sdl_state = static_cast<SDLDriver*>(user_data);

    sdl_state->HandleGameControllerEvent(*event);

    return 0;
}

class SDLJoystick {
public:
    SDLJoystick(std::string guid_, int port_, SDL_Joystick* joystick,
                SDL_GameController* game_controller)
        : guid{std::move(guid_)}, port{port_}, sdl_joystick{joystick, &SDL_JoystickClose},
          sdl_controller{game_controller, &SDL_GameControllerClose} {
        EnableMotion();
    }

    void EnableMotion() {
        if (sdl_controller) {
            SDL_GameController* controller = sdl_controller.get();
            if (SDL_GameControllerHasSensor(controller, SDL_SENSOR_ACCEL) && !has_accel) {
                SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_ACCEL, SDL_TRUE);
                has_accel = true;
            }
            if (SDL_GameControllerHasSensor(controller, SDL_SENSOR_GYRO) && !has_gyro) {
                SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_GYRO, SDL_TRUE);
                has_gyro = true;
            }
        }
    }

    bool HasGyro() const {
        return has_gyro;
    }

    bool HasAccel() const {
        return has_accel;
    }

    bool UpdateMotion(SDL_ControllerSensorEvent event) {
        constexpr float gravity_constant = 9.80665f;
        std::scoped_lock lock{mutex};
        const u64 time_difference = event.timestamp - last_motion_update;
        last_motion_update = event.timestamp;
        switch (event.sensor) {
        case SDL_SENSOR_ACCEL: {
            motion.accel_x = -event.data[0] / gravity_constant;
            motion.accel_y = event.data[2] / gravity_constant;
            motion.accel_z = -event.data[1] / gravity_constant;
            break;
        }
        case SDL_SENSOR_GYRO: {
            motion.gyro_x = event.data[0] / (Common::PI * 2);
            motion.gyro_y = -event.data[2] / (Common::PI * 2);
            motion.gyro_z = event.data[1] / (Common::PI * 2);
            break;
        }
        }

        // Ignore duplicated timestamps
        if (time_difference == 0) {
            return false;
        }
        motion.delta_timestamp = time_difference * 1000;
        return true;
    }

    const BasicMotion& GetMotion() const {
        return motion;
    }

    bool RumblePlay(const Common::Input::VibrationStatus vibration) {
        constexpr u32 rumble_max_duration_ms = 1000;
        if (sdl_controller) {
            return SDL_GameControllerRumble(
                       sdl_controller.get(), static_cast<u16>(vibration.low_amplitude),
                       static_cast<u16>(vibration.high_amplitude), rumble_max_duration_ms) != -1;
        } else if (sdl_joystick) {
            return SDL_JoystickRumble(sdl_joystick.get(), static_cast<u16>(vibration.low_amplitude),
                                      static_cast<u16>(vibration.high_amplitude),
                                      rumble_max_duration_ms) != -1;
        }

        return false;
    }

    bool HasHDRumble() const {
        if (sdl_controller) {
            const auto type = SDL_GameControllerGetType(sdl_controller.get());
            return (type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO) ||
                   (type == SDL_CONTROLLER_TYPE_PS5);
        }
        return false;
    }
    /**
     * The Pad identifier of the joystick
     */
    const PadIdentifier GetPadIdentifier() const {
        return {
            .guid = Common::UUID{guid},
            .port = static_cast<std::size_t>(port),
            .pad = 0,
        };
    }

    /**
     * The guid of the joystick
     */
    const std::string& GetGUID() const {
        return guid;
    }

    /**
     * The number of joystick from the same type that were connected before this joystick
     */
    int GetPort() const {
        return port;
    }

    SDL_Joystick* GetSDLJoystick() const {
        return sdl_joystick.get();
    }

    SDL_GameController* GetSDLGameController() const {
        return sdl_controller.get();
    }

    void SetSDLJoystick(SDL_Joystick* joystick, SDL_GameController* controller) {
        sdl_joystick.reset(joystick);
        sdl_controller.reset(controller);
    }

    bool IsJoyconLeft() const {
        const std::string controller_name = GetControllerName();
        if (std::strstr(controller_name.c_str(), "Joy-Con Left") != nullptr) {
            return true;
        }
        if (std::strstr(controller_name.c_str(), "Joy-Con (L)") != nullptr) {
            return true;
        }
        return false;
    }

    bool IsJoyconRight() const {
        const std::string controller_name = GetControllerName();
        if (std::strstr(controller_name.c_str(), "Joy-Con Right") != nullptr) {
            return true;
        }
        if (std::strstr(controller_name.c_str(), "Joy-Con (R)") != nullptr) {
            return true;
        }
        return false;
    }

    Common::Input::BatteryLevel GetBatteryLevel() {
        const auto level = SDL_JoystickCurrentPowerLevel(sdl_joystick.get());
        switch (level) {
        case SDL_JOYSTICK_POWER_EMPTY:
            return Common::Input::BatteryLevel::Empty;
        case SDL_JOYSTICK_POWER_LOW:
            return Common::Input::BatteryLevel::Low;
        case SDL_JOYSTICK_POWER_MEDIUM:
            return Common::Input::BatteryLevel::Medium;
        case SDL_JOYSTICK_POWER_FULL:
        case SDL_JOYSTICK_POWER_MAX:
            return Common::Input::BatteryLevel::Full;
        case SDL_JOYSTICK_POWER_WIRED:
            return Common::Input::BatteryLevel::Charging;
        case SDL_JOYSTICK_POWER_UNKNOWN:
        default:
            return Common::Input::BatteryLevel::None;
        }
    }

    std::string GetControllerName() const {
        if (sdl_controller) {
            switch (SDL_GameControllerGetType(sdl_controller.get())) {
            case SDL_CONTROLLER_TYPE_XBOX360:
                return "Xbox 360 Controller";
            case SDL_CONTROLLER_TYPE_XBOXONE:
                return "Xbox One Controller";
            case SDL_CONTROLLER_TYPE_PS3:
                return "DualShock 3 Controller";
            case SDL_CONTROLLER_TYPE_PS4:
                return "DualShock 4 Controller";
            case SDL_CONTROLLER_TYPE_PS5:
                return "DualSense Controller";
            default:
                break;
            }
            const auto name = SDL_GameControllerName(sdl_controller.get());
            if (name) {
                return name;
            }
        }

        if (sdl_joystick) {
            const auto name = SDL_JoystickName(sdl_joystick.get());
            if (name) {
                return name;
            }
        }

        return "Unknown";
    }

private:
    std::string guid;
    int port;
    std::unique_ptr<SDL_Joystick, decltype(&SDL_JoystickClose)> sdl_joystick;
    std::unique_ptr<SDL_GameController, decltype(&SDL_GameControllerClose)> sdl_controller;
    mutable std::mutex mutex;

    u64 last_motion_update{};
    bool has_gyro{false};
    bool has_accel{false};
    BasicMotion motion;
};

std::shared_ptr<SDLJoystick> SDLDriver::GetSDLJoystickByGUID(const std::string& guid, int port) {
    std::scoped_lock lock{joystick_map_mutex};
    const auto it = joystick_map.find(guid);

    if (it != joystick_map.end()) {
        while (it->second.size() <= static_cast<std::size_t>(port)) {
            auto joystick = std::make_shared<SDLJoystick>(guid, static_cast<int>(it->second.size()),
                                                          nullptr, nullptr);
            it->second.emplace_back(std::move(joystick));
        }

        return it->second[static_cast<std::size_t>(port)];
    }

    auto joystick = std::make_shared<SDLJoystick>(guid, 0, nullptr, nullptr);

    return joystick_map[guid].emplace_back(std::move(joystick));
}

std::shared_ptr<SDLJoystick> SDLDriver::GetSDLJoystickBySDLID(SDL_JoystickID sdl_id) {
    auto sdl_joystick = SDL_JoystickFromInstanceID(sdl_id);
    const std::string guid = GetGUID(sdl_joystick);

    std::scoped_lock lock{joystick_map_mutex};
    const auto map_it = joystick_map.find(guid);

    if (map_it == joystick_map.end()) {
        return nullptr;
    }

    const auto vec_it = std::find_if(map_it->second.begin(), map_it->second.end(),
                                     [&sdl_joystick](const auto& joystick) {
                                         return joystick->GetSDLJoystick() == sdl_joystick;
                                     });

    if (vec_it == map_it->second.end()) {
        return nullptr;
    }

    return *vec_it;
}

void SDLDriver::InitJoystick(int joystick_index) {
    SDL_Joystick* sdl_joystick = SDL_JoystickOpen(joystick_index);
    SDL_GameController* sdl_gamecontroller = nullptr;

    if (SDL_IsGameController(joystick_index)) {
        sdl_gamecontroller = SDL_GameControllerOpen(joystick_index);
    }

    if (!sdl_joystick) {
        LOG_ERROR(Input, "Failed to open joystick {}", joystick_index);
        return;
    }

    const std::string guid = GetGUID(sdl_joystick);

    std::scoped_lock lock{joystick_map_mutex};
    if (joystick_map.find(guid) == joystick_map.end()) {
        auto joystick = std::make_shared<SDLJoystick>(guid, 0, sdl_joystick, sdl_gamecontroller);
        PreSetController(joystick->GetPadIdentifier());
        SetBattery(joystick->GetPadIdentifier(), joystick->GetBatteryLevel());
        joystick_map[guid].emplace_back(std::move(joystick));
        return;
    }

    auto& joystick_guid_list = joystick_map[guid];
    const auto joystick_it =
        std::find_if(joystick_guid_list.begin(), joystick_guid_list.end(),
                     [](const auto& joystick) { return !joystick->GetSDLJoystick(); });

    if (joystick_it != joystick_guid_list.end()) {
        (*joystick_it)->SetSDLJoystick(sdl_joystick, sdl_gamecontroller);
        return;
    }

    const int port = static_cast<int>(joystick_guid_list.size());
    auto joystick = std::make_shared<SDLJoystick>(guid, port, sdl_joystick, sdl_gamecontroller);
    PreSetController(joystick->GetPadIdentifier());
    SetBattery(joystick->GetPadIdentifier(), joystick->GetBatteryLevel());
    joystick_guid_list.emplace_back(std::move(joystick));
}

void SDLDriver::CloseJoystick(SDL_Joystick* sdl_joystick) {
    const std::string guid = GetGUID(sdl_joystick);

    std::scoped_lock lock{joystick_map_mutex};
    // This call to guid is safe since the joystick is guaranteed to be in the map
    const auto& joystick_guid_list = joystick_map[guid];
    const auto joystick_it = std::find_if(joystick_guid_list.begin(), joystick_guid_list.end(),
                                          [&sdl_joystick](const auto& joystick) {
                                              return joystick->GetSDLJoystick() == sdl_joystick;
                                          });

    if (joystick_it != joystick_guid_list.end()) {
        (*joystick_it)->SetSDLJoystick(nullptr, nullptr);
    }
}

void SDLDriver::HandleGameControllerEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_JOYBUTTONUP: {
        if (const auto joystick = GetSDLJoystickBySDLID(event.jbutton.which)) {
            const PadIdentifier identifier = joystick->GetPadIdentifier();
            SetButton(identifier, event.jbutton.button, false);
        }
        break;
    }
    case SDL_JOYBUTTONDOWN: {
        if (const auto joystick = GetSDLJoystickBySDLID(event.jbutton.which)) {
            const PadIdentifier identifier = joystick->GetPadIdentifier();
            SetButton(identifier, event.jbutton.button, true);
            // Battery doesn't trigger an event so just update every button press
            SetBattery(identifier, joystick->GetBatteryLevel());
        }
        break;
    }
    case SDL_JOYHATMOTION: {
        if (const auto joystick = GetSDLJoystickBySDLID(event.jhat.which)) {
            const PadIdentifier identifier = joystick->GetPadIdentifier();
            SetHatButton(identifier, event.jhat.hat, event.jhat.value);
        }
        break;
    }
    case SDL_JOYAXISMOTION: {
        if (const auto joystick = GetSDLJoystickBySDLID(event.jaxis.which)) {
            const PadIdentifier identifier = joystick->GetPadIdentifier();
            SetAxis(identifier, event.jaxis.axis, event.jaxis.value / 32767.0f);
        }
        break;
    }
    case SDL_CONTROLLERSENSORUPDATE: {
        if (auto joystick = GetSDLJoystickBySDLID(event.csensor.which)) {
            if (joystick->UpdateMotion(event.csensor)) {
                const PadIdentifier identifier = joystick->GetPadIdentifier();
                SetMotion(identifier, 0, joystick->GetMotion());
            }
        }
        break;
    }
    case SDL_JOYDEVICEREMOVED:
        LOG_DEBUG(Input, "Controller removed with Instance_ID {}", event.jdevice.which);
        CloseJoystick(SDL_JoystickFromInstanceID(event.jdevice.which));
        break;
    case SDL_JOYDEVICEADDED:
        LOG_DEBUG(Input, "Controller connected with device index {}", event.jdevice.which);
        InitJoystick(event.jdevice.which);
        break;
    }
}

void SDLDriver::CloseJoysticks() {
    std::scoped_lock lock{joystick_map_mutex};
    joystick_map.clear();
}

SDLDriver::SDLDriver(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    if (!Settings::values.enable_raw_input) {
        // Disable raw input. When enabled this setting causes SDL to die when a web applet opens
        SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
    }

    // Prevent SDL from adding undesired axis
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

    // Enable HIDAPI rumble. This prevents SDL from disabling motion on PS4 and PS5 controllers
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // Use hidapi driver for joycons. This will allow joycons to be detected as a GameController and
    // not a generic one
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");

    // Disable hidapi driver for xbox. Already default on Windows, this causes conflict with native
    // driver on Linux.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "0");

    // If the frontend is going to manage the event loop, then we don't start one here
    start_thread = SDL_WasInit(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) == 0;
    if (start_thread && SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        LOG_CRITICAL(Input, "SDL_Init failed with: {}", SDL_GetError());
        return;
    }

    SDL_AddEventWatch(&SDLEventWatcher, this);

    initialized = true;
    if (start_thread) {
        poll_thread = std::thread([this] {
            Common::SetCurrentThreadName("yuzu:input:SDL");
            using namespace std::chrono_literals;
            while (initialized) {
                SDL_PumpEvents();
                std::this_thread::sleep_for(1ms);
            }
        });
    }
    // Because the events for joystick connection happens before we have our event watcher added, we
    // can just open all the joysticks right here
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        InitJoystick(i);
    }
}

SDLDriver::~SDLDriver() {
    CloseJoysticks();
    SDL_DelEventWatch(&SDLEventWatcher, this);

    initialized = false;
    if (start_thread) {
        poll_thread.join();
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    }
}

std::vector<Common::ParamPackage> SDLDriver::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices;
    std::unordered_map<int, std::shared_ptr<SDLJoystick>> joycon_pairs;
    for (const auto& [key, value] : joystick_map) {
        for (const auto& joystick : value) {
            if (!joystick->GetSDLJoystick()) {
                continue;
            }
            const std::string name =
                fmt::format("{} {}", joystick->GetControllerName(), joystick->GetPort());
            devices.emplace_back(Common::ParamPackage{
                {"engine", GetEngineName()},
                {"display", std::move(name)},
                {"guid", joystick->GetGUID()},
                {"port", std::to_string(joystick->GetPort())},
            });
            if (joystick->IsJoyconLeft()) {
                joycon_pairs.insert_or_assign(joystick->GetPort(), joystick);
            }
        }
    }

    // Add dual controllers
    for (const auto& [key, value] : joystick_map) {
        for (const auto& joystick : value) {
            if (joystick->IsJoyconRight()) {
                if (!joycon_pairs.contains(joystick->GetPort())) {
                    continue;
                }
                const auto joystick2 = joycon_pairs.at(joystick->GetPort());

                const std::string name =
                    fmt::format("{} {}", "Nintendo Dual Joy-Con", joystick->GetPort());
                devices.emplace_back(Common::ParamPackage{
                    {"engine", GetEngineName()},
                    {"display", std::move(name)},
                    {"guid", joystick->GetGUID()},
                    {"guid2", joystick2->GetGUID()},
                    {"port", std::to_string(joystick->GetPort())},
                });
            }
        }
    }
    return devices;
}

Common::Input::VibrationError SDLDriver::SetRumble(
    const PadIdentifier& identifier, const Common::Input::VibrationStatus& vibration) {
    const auto joystick =
        GetSDLJoystickByGUID(identifier.guid.RawString(), static_cast<int>(identifier.port));
    const auto process_amplitude_exp = [](f32 amplitude, f32 factor) {
        return (amplitude + std::pow(amplitude, factor)) * 0.5f * 0xFFFF;
    };

    // Default exponential curve for rumble
    f32 factor = 0.35f;

    // If vibration is set as a linear output use a flatter value
    if (vibration.type == Common::Input::VibrationAmplificationType::Linear) {
        factor = 0.5f;
    }

    // Amplitude for HD rumble needs no modification
    if (joystick->HasHDRumble()) {
        factor = 1.0f;
    }

    const Common::Input::VibrationStatus new_vibration{
        .low_amplitude = process_amplitude_exp(vibration.low_amplitude, factor),
        .low_frequency = vibration.low_frequency,
        .high_amplitude = process_amplitude_exp(vibration.high_amplitude, factor),
        .high_frequency = vibration.high_frequency,
        .type = Common::Input::VibrationAmplificationType::Exponential,
    };

    if (!joystick->RumblePlay(new_vibration)) {
        return Common::Input::VibrationError::Unknown;
    }

    return Common::Input::VibrationError::None;
}

Common::ParamPackage SDLDriver::BuildAnalogParamPackageForButton(int port, std::string guid,
                                                                 s32 axis, float value) const {
    Common::ParamPackage params{};
    params.Set("engine", GetEngineName());
    params.Set("port", port);
    params.Set("guid", std::move(guid));
    params.Set("axis", axis);
    params.Set("threshold", "0.5");
    params.Set("invert", value < 0 ? "-" : "+");
    return params;
}

Common::ParamPackage SDLDriver::BuildButtonParamPackageForButton(int port, std::string guid,
                                                                 s32 button) const {
    Common::ParamPackage params{};
    params.Set("engine", GetEngineName());
    params.Set("port", port);
    params.Set("guid", std::move(guid));
    params.Set("button", button);
    return params;
}

Common::ParamPackage SDLDriver::BuildHatParamPackageForButton(int port, std::string guid, s32 hat,
                                                              u8 value) const {
    Common::ParamPackage params{};
    params.Set("engine", GetEngineName());
    params.Set("port", port);
    params.Set("guid", std::move(guid));
    params.Set("hat", hat);
    params.Set("direction", GetHatButtonName(value));
    return params;
}

Common::ParamPackage SDLDriver::BuildMotionParam(int port, std::string guid) const {
    Common::ParamPackage params{};
    params.Set("engine", GetEngineName());
    params.Set("motion", 0);
    params.Set("port", port);
    params.Set("guid", std::move(guid));
    return params;
}

Common::ParamPackage SDLDriver::BuildParamPackageForBinding(
    int port, const std::string& guid, const SDL_GameControllerButtonBind& binding) const {
    switch (binding.bindType) {
    case SDL_CONTROLLER_BINDTYPE_NONE:
        break;
    case SDL_CONTROLLER_BINDTYPE_AXIS:
        return BuildAnalogParamPackageForButton(port, guid, binding.value.axis);
    case SDL_CONTROLLER_BINDTYPE_BUTTON:
        return BuildButtonParamPackageForButton(port, guid, binding.value.button);
    case SDL_CONTROLLER_BINDTYPE_HAT:
        return BuildHatParamPackageForButton(port, guid, binding.value.hat.hat,
                                             static_cast<u8>(binding.value.hat.hat_mask));
    }
    return {};
}

Common::ParamPackage SDLDriver::BuildParamPackageForAnalog(PadIdentifier identifier, int axis_x,
                                                           int axis_y, float offset_x,
                                                           float offset_y) const {
    Common::ParamPackage params;
    params.Set("engine", GetEngineName());
    params.Set("port", static_cast<int>(identifier.port));
    params.Set("guid", identifier.guid.RawString());
    params.Set("axis_x", axis_x);
    params.Set("axis_y", axis_y);
    params.Set("offset_x", offset_x);
    params.Set("offset_y", offset_y);
    params.Set("invert_x", "+");
    params.Set("invert_y", "+");
    return params;
}

ButtonMapping SDLDriver::GetButtonMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("guid") || !params.Has("port")) {
        return {};
    }
    const auto joystick = GetSDLJoystickByGUID(params.Get("guid", ""), params.Get("port", 0));

    auto* controller = joystick->GetSDLGameController();
    if (controller == nullptr) {
        return {};
    }

    // This list is missing ZL/ZR since those are not considered buttons in SDL GameController.
    // We will add those afterwards
    // This list also excludes Screenshot since theres not really a mapping for that
    ButtonBindings switch_to_sdl_button;

    if (SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO) {
        switch_to_sdl_button = GetNintendoButtonBinding(joystick);
    } else {
        switch_to_sdl_button = GetDefaultButtonBinding();
    }

    // Add the missing bindings for ZL/ZR
    static constexpr ZButtonBindings switch_to_sdl_axis{{
        {Settings::NativeButton::ZL, SDL_CONTROLLER_AXIS_TRIGGERLEFT},
        {Settings::NativeButton::ZR, SDL_CONTROLLER_AXIS_TRIGGERRIGHT},
    }};

    // Parameters contain two joysticks return dual
    if (params.Has("guid2")) {
        const auto joystick2 = GetSDLJoystickByGUID(params.Get("guid2", ""), params.Get("port", 0));

        if (joystick2->GetSDLGameController() != nullptr) {
            return GetDualControllerMapping(joystick, joystick2, switch_to_sdl_button,
                                            switch_to_sdl_axis);
        }
    }

    return GetSingleControllerMapping(joystick, switch_to_sdl_button, switch_to_sdl_axis);
}

ButtonBindings SDLDriver::GetDefaultButtonBinding() const {
    return {
        std::pair{Settings::NativeButton::A, SDL_CONTROLLER_BUTTON_B},
        {Settings::NativeButton::B, SDL_CONTROLLER_BUTTON_A},
        {Settings::NativeButton::X, SDL_CONTROLLER_BUTTON_Y},
        {Settings::NativeButton::Y, SDL_CONTROLLER_BUTTON_X},
        {Settings::NativeButton::LStick, SDL_CONTROLLER_BUTTON_LEFTSTICK},
        {Settings::NativeButton::RStick, SDL_CONTROLLER_BUTTON_RIGHTSTICK},
        {Settings::NativeButton::L, SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {Settings::NativeButton::R, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {Settings::NativeButton::Plus, SDL_CONTROLLER_BUTTON_START},
        {Settings::NativeButton::Minus, SDL_CONTROLLER_BUTTON_BACK},
        {Settings::NativeButton::DLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
        {Settings::NativeButton::DUp, SDL_CONTROLLER_BUTTON_DPAD_UP},
        {Settings::NativeButton::DRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
        {Settings::NativeButton::DDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN},
        {Settings::NativeButton::SL, SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {Settings::NativeButton::SR, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {Settings::NativeButton::Home, SDL_CONTROLLER_BUTTON_GUIDE},
        {Settings::NativeButton::Screenshot, SDL_CONTROLLER_BUTTON_MISC1},
    };
}

ButtonBindings SDLDriver::GetNintendoButtonBinding(
    const std::shared_ptr<SDLJoystick>& joystick) const {
    // Default SL/SR mapping for pro controllers
    auto sl_button = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    auto sr_button = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;

    if (joystick->IsJoyconLeft()) {
        sl_button = SDL_CONTROLLER_BUTTON_PADDLE2;
        sr_button = SDL_CONTROLLER_BUTTON_PADDLE4;
    }
    if (joystick->IsJoyconRight()) {
        sl_button = SDL_CONTROLLER_BUTTON_PADDLE3;
        sr_button = SDL_CONTROLLER_BUTTON_PADDLE1;
    }

    return {
        std::pair{Settings::NativeButton::A, SDL_CONTROLLER_BUTTON_A},
        {Settings::NativeButton::B, SDL_CONTROLLER_BUTTON_B},
        {Settings::NativeButton::X, SDL_CONTROLLER_BUTTON_X},
        {Settings::NativeButton::Y, SDL_CONTROLLER_BUTTON_Y},
        {Settings::NativeButton::LStick, SDL_CONTROLLER_BUTTON_LEFTSTICK},
        {Settings::NativeButton::RStick, SDL_CONTROLLER_BUTTON_RIGHTSTICK},
        {Settings::NativeButton::L, SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
        {Settings::NativeButton::R, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
        {Settings::NativeButton::Plus, SDL_CONTROLLER_BUTTON_START},
        {Settings::NativeButton::Minus, SDL_CONTROLLER_BUTTON_BACK},
        {Settings::NativeButton::DLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
        {Settings::NativeButton::DUp, SDL_CONTROLLER_BUTTON_DPAD_UP},
        {Settings::NativeButton::DRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
        {Settings::NativeButton::DDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN},
        {Settings::NativeButton::SL, sl_button},
        {Settings::NativeButton::SR, sr_button},
        {Settings::NativeButton::Home, SDL_CONTROLLER_BUTTON_GUIDE},
        {Settings::NativeButton::Screenshot, SDL_CONTROLLER_BUTTON_MISC1},
    };
}

ButtonMapping SDLDriver::GetSingleControllerMapping(
    const std::shared_ptr<SDLJoystick>& joystick, const ButtonBindings& switch_to_sdl_button,
    const ZButtonBindings& switch_to_sdl_axis) const {
    ButtonMapping mapping;
    mapping.reserve(switch_to_sdl_button.size() + switch_to_sdl_axis.size());
    auto* controller = joystick->GetSDLGameController();

    for (const auto& [switch_button, sdl_button] : switch_to_sdl_button) {
        const auto& binding = SDL_GameControllerGetBindForButton(controller, sdl_button);
        mapping.insert_or_assign(
            switch_button,
            BuildParamPackageForBinding(joystick->GetPort(), joystick->GetGUID(), binding));
    }
    for (const auto& [switch_button, sdl_axis] : switch_to_sdl_axis) {
        const auto& binding = SDL_GameControllerGetBindForAxis(controller, sdl_axis);
        mapping.insert_or_assign(
            switch_button,
            BuildParamPackageForBinding(joystick->GetPort(), joystick->GetGUID(), binding));
    }

    return mapping;
}

ButtonMapping SDLDriver::GetDualControllerMapping(const std::shared_ptr<SDLJoystick>& joystick,
                                                  const std::shared_ptr<SDLJoystick>& joystick2,
                                                  const ButtonBindings& switch_to_sdl_button,
                                                  const ZButtonBindings& switch_to_sdl_axis) const {
    ButtonMapping mapping;
    mapping.reserve(switch_to_sdl_button.size() + switch_to_sdl_axis.size());
    auto* controller = joystick->GetSDLGameController();
    auto* controller2 = joystick2->GetSDLGameController();

    for (const auto& [switch_button, sdl_button] : switch_to_sdl_button) {
        if (IsButtonOnLeftSide(switch_button)) {
            const auto& binding = SDL_GameControllerGetBindForButton(controller2, sdl_button);
            mapping.insert_or_assign(
                switch_button,
                BuildParamPackageForBinding(joystick2->GetPort(), joystick2->GetGUID(), binding));
            continue;
        }
        const auto& binding = SDL_GameControllerGetBindForButton(controller, sdl_button);
        mapping.insert_or_assign(
            switch_button,
            BuildParamPackageForBinding(joystick->GetPort(), joystick->GetGUID(), binding));
    }
    for (const auto& [switch_button, sdl_axis] : switch_to_sdl_axis) {
        if (IsButtonOnLeftSide(switch_button)) {
            const auto& binding = SDL_GameControllerGetBindForAxis(controller2, sdl_axis);
            mapping.insert_or_assign(
                switch_button,
                BuildParamPackageForBinding(joystick2->GetPort(), joystick2->GetGUID(), binding));
            continue;
        }
        const auto& binding = SDL_GameControllerGetBindForAxis(controller, sdl_axis);
        mapping.insert_or_assign(
            switch_button,
            BuildParamPackageForBinding(joystick->GetPort(), joystick->GetGUID(), binding));
    }

    return mapping;
}

bool SDLDriver::IsButtonOnLeftSide(Settings::NativeButton::Values button) const {
    switch (button) {
    case Settings::NativeButton::DDown:
    case Settings::NativeButton::DLeft:
    case Settings::NativeButton::DRight:
    case Settings::NativeButton::DUp:
    case Settings::NativeButton::L:
    case Settings::NativeButton::LStick:
    case Settings::NativeButton::Minus:
    case Settings::NativeButton::Screenshot:
    case Settings::NativeButton::ZL:
        return true;
    default:
        return false;
    }
}

AnalogMapping SDLDriver::GetAnalogMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("guid") || !params.Has("port")) {
        return {};
    }
    const auto joystick = GetSDLJoystickByGUID(params.Get("guid", ""), params.Get("port", 0));
    const auto joystick2 = GetSDLJoystickByGUID(params.Get("guid2", ""), params.Get("port", 0));
    auto* controller = joystick->GetSDLGameController();
    if (controller == nullptr) {
        return {};
    }

    AnalogMapping mapping = {};
    const auto& binding_left_x =
        SDL_GameControllerGetBindForAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    const auto& binding_left_y =
        SDL_GameControllerGetBindForAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
    if (params.Has("guid2")) {
        const auto identifier = joystick2->GetPadIdentifier();
        PreSetController(identifier);
        PreSetAxis(identifier, binding_left_x.value.axis);
        PreSetAxis(identifier, binding_left_y.value.axis);
        const auto left_offset_x = -GetAxis(identifier, binding_left_x.value.axis);
        const auto left_offset_y = GetAxis(identifier, binding_left_y.value.axis);
        mapping.insert_or_assign(Settings::NativeAnalog::LStick,
                                 BuildParamPackageForAnalog(identifier, binding_left_x.value.axis,
                                                            binding_left_y.value.axis,
                                                            left_offset_x, left_offset_y));
    } else {
        const auto identifier = joystick->GetPadIdentifier();
        PreSetController(identifier);
        PreSetAxis(identifier, binding_left_x.value.axis);
        PreSetAxis(identifier, binding_left_y.value.axis);
        const auto left_offset_x = -GetAxis(identifier, binding_left_x.value.axis);
        const auto left_offset_y = GetAxis(identifier, binding_left_y.value.axis);
        mapping.insert_or_assign(Settings::NativeAnalog::LStick,
                                 BuildParamPackageForAnalog(identifier, binding_left_x.value.axis,
                                                            binding_left_y.value.axis,
                                                            left_offset_x, left_offset_y));
    }
    const auto& binding_right_x =
        SDL_GameControllerGetBindForAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
    const auto& binding_right_y =
        SDL_GameControllerGetBindForAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);
    const auto identifier = joystick->GetPadIdentifier();
    PreSetController(identifier);
    PreSetAxis(identifier, binding_right_x.value.axis);
    PreSetAxis(identifier, binding_right_y.value.axis);
    const auto right_offset_x = -GetAxis(identifier, binding_right_x.value.axis);
    const auto right_offset_y = GetAxis(identifier, binding_right_y.value.axis);
    mapping.insert_or_assign(Settings::NativeAnalog::RStick,
                             BuildParamPackageForAnalog(identifier, binding_right_x.value.axis,
                                                        binding_right_y.value.axis, right_offset_x,
                                                        right_offset_y));
    return mapping;
}

MotionMapping SDLDriver::GetMotionMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("guid") || !params.Has("port")) {
        return {};
    }
    const auto joystick = GetSDLJoystickByGUID(params.Get("guid", ""), params.Get("port", 0));
    const auto joystick2 = GetSDLJoystickByGUID(params.Get("guid2", ""), params.Get("port", 0));
    auto* controller = joystick->GetSDLGameController();
    if (controller == nullptr) {
        return {};
    }

    MotionMapping mapping = {};
    joystick->EnableMotion();

    if (joystick->HasGyro() || joystick->HasAccel()) {
        mapping.insert_or_assign(Settings::NativeMotion::MotionRight,
                                 BuildMotionParam(joystick->GetPort(), joystick->GetGUID()));
    }
    if (params.Has("guid2")) {
        joystick2->EnableMotion();
        if (joystick2->HasGyro() || joystick2->HasAccel()) {
            mapping.insert_or_assign(Settings::NativeMotion::MotionLeft,
                                     BuildMotionParam(joystick2->GetPort(), joystick2->GetGUID()));
        }
    } else {
        if (joystick->HasGyro() || joystick->HasAccel()) {
            mapping.insert_or_assign(Settings::NativeMotion::MotionLeft,
                                     BuildMotionParam(joystick->GetPort(), joystick->GetGUID()));
        }
    }

    return mapping;
}

Common::Input::ButtonNames SDLDriver::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        // TODO(German77): Find how to substitue the values for real button names
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("hat")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("axis")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("axis_x") && params.Has("axis_y") && params.Has("axis_z")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("motion")) {
        return Common::Input::ButtonNames::Engine;
    }

    return Common::Input::ButtonNames::Invalid;
}

std::string SDLDriver::GetHatButtonName(u8 direction_value) const {
    switch (direction_value) {
    case SDL_HAT_UP:
        return "up";
    case SDL_HAT_DOWN:
        return "down";
    case SDL_HAT_LEFT:
        return "left";
    case SDL_HAT_RIGHT:
        return "right";
    default:
        return {};
    }
}

u8 SDLDriver::GetHatButtonId(const std::string& direction_name) const {
    Uint8 direction;
    if (direction_name == "up") {
        direction = SDL_HAT_UP;
    } else if (direction_name == "down") {
        direction = SDL_HAT_DOWN;
    } else if (direction_name == "left") {
        direction = SDL_HAT_LEFT;
    } else if (direction_name == "right") {
        direction = SDL_HAT_RIGHT;
    } else {
        direction = 0;
    }
    return direction;
}

} // namespace InputCommon
