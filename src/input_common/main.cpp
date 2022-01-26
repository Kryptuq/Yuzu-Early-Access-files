// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <thread>
#include "common/input.h"
#include "common/param_package.h"
#include "input_common/drivers/gc_adapter.h"
#include "input_common/drivers/keyboard.h"
#include "input_common/drivers/mouse.h"
#include "input_common/drivers/tas_input.h"
#include "input_common/drivers/touch_screen.h"
#include "input_common/drivers/udp_client.h"
#include "input_common/helpers/stick_from_buttons.h"
#include "input_common/helpers/touch_from_buttons.h"
#include "input_common/input_engine.h"
#include "input_common/input_mapping.h"
#include "input_common/input_poller.h"
#include "input_common/main.h"
#ifdef HAVE_SDL2
#include "input_common/drivers/sdl_driver.h"
#endif

namespace InputCommon {

struct InputSubsystem::Impl {
    void Initialize() {
        mapping_factory = std::make_shared<MappingFactory>();
        MappingCallback mapping_callback{[this](const MappingData& data) { RegisterInput(data); }};

        keyboard = std::make_shared<Keyboard>("keyboard");
        keyboard->SetMappingCallback(mapping_callback);
        keyboard_factory = std::make_shared<InputFactory>(keyboard);
        keyboard_output_factory = std::make_shared<OutputFactory>(keyboard);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(keyboard->GetEngineName(),
                                                                   keyboard_factory);
        Common::Input::RegisterFactory<Common::Input::OutputDevice>(keyboard->GetEngineName(),
                                                                    keyboard_output_factory);

        mouse = std::make_shared<Mouse>("mouse");
        mouse->SetMappingCallback(mapping_callback);
        mouse_factory = std::make_shared<InputFactory>(mouse);
        mouse_output_factory = std::make_shared<OutputFactory>(mouse);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(mouse->GetEngineName(),
                                                                   mouse_factory);
        Common::Input::RegisterFactory<Common::Input::OutputDevice>(mouse->GetEngineName(),
                                                                    mouse_output_factory);

        touch_screen = std::make_shared<TouchScreen>("touch");
        touch_screen_factory = std::make_shared<InputFactory>(touch_screen);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(touch_screen->GetEngineName(),
                                                                   touch_screen_factory);

        gcadapter = std::make_shared<GCAdapter>("gcpad");
        gcadapter->SetMappingCallback(mapping_callback);
        gcadapter_input_factory = std::make_shared<InputFactory>(gcadapter);
        gcadapter_output_factory = std::make_shared<OutputFactory>(gcadapter);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(gcadapter->GetEngineName(),
                                                                   gcadapter_input_factory);
        Common::Input::RegisterFactory<Common::Input::OutputDevice>(gcadapter->GetEngineName(),
                                                                    gcadapter_output_factory);

        udp_client = std::make_shared<CemuhookUDP::UDPClient>("cemuhookudp");
        udp_client->SetMappingCallback(mapping_callback);
        udp_client_input_factory = std::make_shared<InputFactory>(udp_client);
        udp_client_output_factory = std::make_shared<OutputFactory>(udp_client);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(udp_client->GetEngineName(),
                                                                   udp_client_input_factory);
        Common::Input::RegisterFactory<Common::Input::OutputDevice>(udp_client->GetEngineName(),
                                                                    udp_client_output_factory);

        tas_input = std::make_shared<TasInput::Tas>("tas");
        tas_input->SetMappingCallback(mapping_callback);
        tas_input_factory = std::make_shared<InputFactory>(tas_input);
        tas_output_factory = std::make_shared<OutputFactory>(tas_input);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(tas_input->GetEngineName(),
                                                                   tas_input_factory);
        Common::Input::RegisterFactory<Common::Input::OutputDevice>(tas_input->GetEngineName(),
                                                                    tas_output_factory);

#ifdef HAVE_SDL2
        sdl = std::make_shared<SDLDriver>("sdl");
        sdl->SetMappingCallback(mapping_callback);
        sdl_input_factory = std::make_shared<InputFactory>(sdl);
        sdl_output_factory = std::make_shared<OutputFactory>(sdl);
        Common::Input::RegisterFactory<Common::Input::InputDevice>(sdl->GetEngineName(),
                                                                   sdl_input_factory);
        Common::Input::RegisterFactory<Common::Input::OutputDevice>(sdl->GetEngineName(),
                                                                    sdl_output_factory);
#endif

        Common::Input::RegisterFactory<Common::Input::InputDevice>(
            "touch_from_button", std::make_shared<TouchFromButton>());
        Common::Input::RegisterFactory<Common::Input::InputDevice>(
            "analog_from_button", std::make_shared<StickFromButton>());
    }

    void Shutdown() {
        Common::Input::UnregisterFactory<Common::Input::InputDevice>(keyboard->GetEngineName());
        Common::Input::UnregisterFactory<Common::Input::OutputDevice>(keyboard->GetEngineName());
        keyboard.reset();

        Common::Input::UnregisterFactory<Common::Input::InputDevice>(mouse->GetEngineName());
        Common::Input::UnregisterFactory<Common::Input::OutputDevice>(mouse->GetEngineName());
        mouse.reset();

        Common::Input::UnregisterFactory<Common::Input::InputDevice>(touch_screen->GetEngineName());
        touch_screen.reset();

        Common::Input::UnregisterFactory<Common::Input::InputDevice>(gcadapter->GetEngineName());
        Common::Input::UnregisterFactory<Common::Input::OutputDevice>(gcadapter->GetEngineName());
        gcadapter.reset();

        Common::Input::UnregisterFactory<Common::Input::InputDevice>(udp_client->GetEngineName());
        Common::Input::UnregisterFactory<Common::Input::OutputDevice>(udp_client->GetEngineName());
        udp_client.reset();

        Common::Input::UnregisterFactory<Common::Input::InputDevice>(tas_input->GetEngineName());
        Common::Input::UnregisterFactory<Common::Input::OutputDevice>(tas_input->GetEngineName());
        tas_input.reset();

#ifdef HAVE_SDL2
        Common::Input::UnregisterFactory<Common::Input::InputDevice>(sdl->GetEngineName());
        Common::Input::UnregisterFactory<Common::Input::OutputDevice>(sdl->GetEngineName());
        sdl.reset();
#endif

        Common::Input::UnregisterFactory<Common::Input::InputDevice>("touch_from_button");
        Common::Input::UnregisterFactory<Common::Input::InputDevice>("analog_from_button");
    }

    [[nodiscard]] std::vector<Common::ParamPackage> GetInputDevices() const {
        std::vector<Common::ParamPackage> devices = {
            Common::ParamPackage{{"display", "Any"}, {"engine", "any"}},
        };

        auto keyboard_devices = keyboard->GetInputDevices();
        devices.insert(devices.end(), keyboard_devices.begin(), keyboard_devices.end());
        auto mouse_devices = mouse->GetInputDevices();
        devices.insert(devices.end(), mouse_devices.begin(), mouse_devices.end());
        auto gcadapter_devices = gcadapter->GetInputDevices();
        devices.insert(devices.end(), gcadapter_devices.begin(), gcadapter_devices.end());
        auto udp_devices = udp_client->GetInputDevices();
        devices.insert(devices.end(), udp_devices.begin(), udp_devices.end());
#ifdef HAVE_SDL2
        auto sdl_devices = sdl->GetInputDevices();
        devices.insert(devices.end(), sdl_devices.begin(), sdl_devices.end());
#endif

        return devices;
    }

    [[nodiscard]] AnalogMapping GetAnalogMappingForDevice(
        const Common::ParamPackage& params) const {
        if (!params.Has("engine") || params.Get("engine", "") == "any") {
            return {};
        }
        const std::string engine = params.Get("engine", "");
        if (engine == mouse->GetEngineName()) {
            return mouse->GetAnalogMappingForDevice(params);
        }
        if (engine == gcadapter->GetEngineName()) {
            return gcadapter->GetAnalogMappingForDevice(params);
        }
        if (engine == udp_client->GetEngineName()) {
            return udp_client->GetAnalogMappingForDevice(params);
        }
        if (engine == tas_input->GetEngineName()) {
            return tas_input->GetAnalogMappingForDevice(params);
        }
#ifdef HAVE_SDL2
        if (engine == sdl->GetEngineName()) {
            return sdl->GetAnalogMappingForDevice(params);
        }
#endif
        return {};
    }

    [[nodiscard]] ButtonMapping GetButtonMappingForDevice(
        const Common::ParamPackage& params) const {
        if (!params.Has("engine") || params.Get("engine", "") == "any") {
            return {};
        }
        const std::string engine = params.Get("engine", "");
        if (engine == gcadapter->GetEngineName()) {
            return gcadapter->GetButtonMappingForDevice(params);
        }
        if (engine == udp_client->GetEngineName()) {
            return udp_client->GetButtonMappingForDevice(params);
        }
        if (engine == tas_input->GetEngineName()) {
            return tas_input->GetButtonMappingForDevice(params);
        }
#ifdef HAVE_SDL2
        if (engine == sdl->GetEngineName()) {
            return sdl->GetButtonMappingForDevice(params);
        }
#endif
        return {};
    }

    [[nodiscard]] MotionMapping GetMotionMappingForDevice(
        const Common::ParamPackage& params) const {
        if (!params.Has("engine") || params.Get("engine", "") == "any") {
            return {};
        }
        const std::string engine = params.Get("engine", "");
        if (engine == udp_client->GetEngineName()) {
            return udp_client->GetMotionMappingForDevice(params);
        }
#ifdef HAVE_SDL2
        if (engine == sdl->GetEngineName()) {
            return sdl->GetMotionMappingForDevice(params);
        }
#endif
        return {};
    }

    Common::Input::ButtonNames GetButtonName(const Common::ParamPackage& params) const {
        if (!params.Has("engine") || params.Get("engine", "") == "any") {
            return Common::Input::ButtonNames::Undefined;
        }
        const std::string engine = params.Get("engine", "");
        if (engine == mouse->GetEngineName()) {
            return mouse->GetUIName(params);
        }
        if (engine == gcadapter->GetEngineName()) {
            return gcadapter->GetUIName(params);
        }
        if (engine == udp_client->GetEngineName()) {
            return udp_client->GetUIName(params);
        }
        if (engine == tas_input->GetEngineName()) {
            return tas_input->GetUIName(params);
        }
#ifdef HAVE_SDL2
        if (engine == sdl->GetEngineName()) {
            return sdl->GetUIName(params);
        }
#endif
        return Common::Input::ButtonNames::Invalid;
    }

    bool IsController(const Common::ParamPackage& params) {
        const std::string engine = params.Get("engine", "");
        if (engine == mouse->GetEngineName()) {
            return true;
        }
        if (engine == gcadapter->GetEngineName()) {
            return true;
        }
        if (engine == udp_client->GetEngineName()) {
            return true;
        }
        if (engine == tas_input->GetEngineName()) {
            return true;
        }
#ifdef HAVE_SDL2
        if (engine == sdl->GetEngineName()) {
            return true;
        }
#endif
        return false;
    }

    void BeginConfiguration() {
        keyboard->BeginConfiguration();
        mouse->BeginConfiguration();
        gcadapter->BeginConfiguration();
        udp_client->BeginConfiguration();
#ifdef HAVE_SDL2
        sdl->BeginConfiguration();
#endif
    }

    void EndConfiguration() {
        keyboard->EndConfiguration();
        mouse->EndConfiguration();
        gcadapter->EndConfiguration();
        udp_client->EndConfiguration();
#ifdef HAVE_SDL2
        sdl->EndConfiguration();
#endif
    }

    void RegisterInput(const MappingData& data) {
        mapping_factory->RegisterInput(data);
    }

    std::shared_ptr<MappingFactory> mapping_factory;

    std::shared_ptr<Keyboard> keyboard;
    std::shared_ptr<Mouse> mouse;
    std::shared_ptr<GCAdapter> gcadapter;
    std::shared_ptr<TouchScreen> touch_screen;
    std::shared_ptr<TasInput::Tas> tas_input;
    std::shared_ptr<CemuhookUDP::UDPClient> udp_client;

    std::shared_ptr<InputFactory> keyboard_factory;
    std::shared_ptr<InputFactory> mouse_factory;
    std::shared_ptr<InputFactory> gcadapter_input_factory;
    std::shared_ptr<InputFactory> touch_screen_factory;
    std::shared_ptr<InputFactory> udp_client_input_factory;
    std::shared_ptr<InputFactory> tas_input_factory;

    std::shared_ptr<OutputFactory> keyboard_output_factory;
    std::shared_ptr<OutputFactory> mouse_output_factory;
    std::shared_ptr<OutputFactory> gcadapter_output_factory;
    std::shared_ptr<OutputFactory> udp_client_output_factory;
    std::shared_ptr<OutputFactory> tas_output_factory;

#ifdef HAVE_SDL2
    std::shared_ptr<SDLDriver> sdl;
    std::shared_ptr<InputFactory> sdl_input_factory;
    std::shared_ptr<OutputFactory> sdl_output_factory;
#endif
};

InputSubsystem::InputSubsystem() : impl{std::make_unique<Impl>()} {}

InputSubsystem::~InputSubsystem() = default;

void InputSubsystem::Initialize() {
    impl->Initialize();
}

void InputSubsystem::Shutdown() {
    impl->Shutdown();
}

Keyboard* InputSubsystem::GetKeyboard() {
    return impl->keyboard.get();
}

const Keyboard* InputSubsystem::GetKeyboard() const {
    return impl->keyboard.get();
}

Mouse* InputSubsystem::GetMouse() {
    return impl->mouse.get();
}

const Mouse* InputSubsystem::GetMouse() const {
    return impl->mouse.get();
}

TouchScreen* InputSubsystem::GetTouchScreen() {
    return impl->touch_screen.get();
}

const TouchScreen* InputSubsystem::GetTouchScreen() const {
    return impl->touch_screen.get();
}

TasInput::Tas* InputSubsystem::GetTas() {
    return impl->tas_input.get();
}

const TasInput::Tas* InputSubsystem::GetTas() const {
    return impl->tas_input.get();
}

std::vector<Common::ParamPackage> InputSubsystem::GetInputDevices() const {
    return impl->GetInputDevices();
}

AnalogMapping InputSubsystem::GetAnalogMappingForDevice(const Common::ParamPackage& device) const {
    return impl->GetAnalogMappingForDevice(device);
}

ButtonMapping InputSubsystem::GetButtonMappingForDevice(const Common::ParamPackage& device) const {
    return impl->GetButtonMappingForDevice(device);
}

MotionMapping InputSubsystem::GetMotionMappingForDevice(const Common::ParamPackage& device) const {
    return impl->GetMotionMappingForDevice(device);
}

Common::Input::ButtonNames InputSubsystem::GetButtonName(const Common::ParamPackage& params) const {
    return impl->GetButtonName(params);
}

bool InputSubsystem::IsController(const Common::ParamPackage& params) const {
    return impl->IsController(params);
}

void InputSubsystem::ReloadInputDevices() {
    impl->udp_client.get()->ReloadSockets();
}

void InputSubsystem::BeginMapping(Polling::InputType type) {
    impl->BeginConfiguration();
    impl->mapping_factory->BeginMapping(type);
}

Common::ParamPackage InputSubsystem::GetNextInput() const {
    return impl->mapping_factory->GetNextInput();
}

void InputSubsystem::StopMapping() const {
    impl->EndConfiguration();
    impl->mapping_factory->StopMapping();
}

std::string GenerateKeyboardParam(int key_code) {
    Common::ParamPackage param;
    param.Set("engine", "keyboard");
    param.Set("code", key_code);
    param.Set("toggle", false);
    return param.Serialize();
}

std::string GenerateAnalogParamFromKeys(int key_up, int key_down, int key_left, int key_right,
                                        int key_modifier, float modifier_scale) {
    Common::ParamPackage circle_pad_param{
        {"engine", "analog_from_button"},
        {"up", GenerateKeyboardParam(key_up)},
        {"down", GenerateKeyboardParam(key_down)},
        {"left", GenerateKeyboardParam(key_left)},
        {"right", GenerateKeyboardParam(key_right)},
        {"modifier", GenerateKeyboardParam(key_modifier)},
        {"modifier_scale", std::to_string(modifier_scale)},
    };
    return circle_pad_param.Serialize();
}
} // namespace InputCommon
