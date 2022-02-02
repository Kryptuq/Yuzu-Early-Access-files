// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <random>
#include <boost/asio.hpp>
#include <fmt/format.h>

#include "common/logging/log.h"
#include "common/param_package.h"
#include "common/settings.h"
#include "input_common/drivers/udp_client.h"
#include "input_common/helpers/udp_protocol.h"

using boost::asio::ip::udp;

namespace InputCommon::CemuhookUDP {

struct SocketCallback {
    std::function<void(Response::Version)> version;
    std::function<void(Response::PortInfo)> port_info;
    std::function<void(Response::PadData)> pad_data;
};

class Socket {
public:
    using clock = std::chrono::system_clock;

    explicit Socket(const std::string& host, u16 port, SocketCallback callback_)
        : callback(std::move(callback_)), timer(io_service),
          socket(io_service, udp::endpoint(udp::v4(), 0)), client_id(GenerateRandomClientId()) {
        boost::system::error_code ec{};
        auto ipv4 = boost::asio::ip::make_address_v4(host, ec);
        if (ec.value() != boost::system::errc::success) {
            LOG_ERROR(Input, "Invalid IPv4 address \"{}\" provided to socket", host);
            ipv4 = boost::asio::ip::address_v4{};
        }

        send_endpoint = {udp::endpoint(ipv4, port)};
    }

    void Stop() {
        io_service.stop();
    }

    void Loop() {
        io_service.run();
    }

    void StartSend(const clock::time_point& from) {
        timer.expires_at(from + std::chrono::seconds(3));
        timer.async_wait([this](const boost::system::error_code& error) { HandleSend(error); });
    }

    void StartReceive() {
        socket.async_receive_from(
            boost::asio::buffer(receive_buffer), receive_endpoint,
            [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
                HandleReceive(error, bytes_transferred);
            });
    }

private:
    u32 GenerateRandomClientId() const {
        std::random_device device;
        return device();
    }

    void HandleReceive(const boost::system::error_code&, std::size_t bytes_transferred) {
        if (auto type = Response::Validate(receive_buffer.data(), bytes_transferred)) {
            switch (*type) {
            case Type::Version: {
                Response::Version version;
                std::memcpy(&version, &receive_buffer[sizeof(Header)], sizeof(Response::Version));
                callback.version(std::move(version));
                break;
            }
            case Type::PortInfo: {
                Response::PortInfo port_info;
                std::memcpy(&port_info, &receive_buffer[sizeof(Header)],
                            sizeof(Response::PortInfo));
                callback.port_info(std::move(port_info));
                break;
            }
            case Type::PadData: {
                Response::PadData pad_data;
                std::memcpy(&pad_data, &receive_buffer[sizeof(Header)], sizeof(Response::PadData));
                callback.pad_data(std::move(pad_data));
                break;
            }
            }
        }
        StartReceive();
    }

    void HandleSend(const boost::system::error_code&) {
        boost::system::error_code _ignored{};
        // Send a request for getting port info for the pad
        const Request::PortInfo port_info{4, {0, 1, 2, 3}};
        const auto port_message = Request::Create(port_info, client_id);
        std::memcpy(&send_buffer1, &port_message, PORT_INFO_SIZE);
        socket.send_to(boost::asio::buffer(send_buffer1), send_endpoint, {}, _ignored);

        // Send a request for getting pad data for the pad
        const Request::PadData pad_data{
            Request::RegisterFlags::AllPads,
            0,
            EMPTY_MAC_ADDRESS,
        };
        const auto pad_message = Request::Create(pad_data, client_id);
        std::memcpy(send_buffer2.data(), &pad_message, PAD_DATA_SIZE);
        socket.send_to(boost::asio::buffer(send_buffer2), send_endpoint, {}, _ignored);
        StartSend(timer.expiry());
    }

    SocketCallback callback;
    boost::asio::io_service io_service;
    boost::asio::basic_waitable_timer<clock> timer;
    udp::socket socket;

    const u32 client_id;

    static constexpr std::size_t PORT_INFO_SIZE = sizeof(Message<Request::PortInfo>);
    static constexpr std::size_t PAD_DATA_SIZE = sizeof(Message<Request::PadData>);
    std::array<u8, PORT_INFO_SIZE> send_buffer1;
    std::array<u8, PAD_DATA_SIZE> send_buffer2;
    udp::endpoint send_endpoint;

    std::array<u8, MAX_PACKET_SIZE> receive_buffer;
    udp::endpoint receive_endpoint;
};

static void SocketLoop(Socket* socket) {
    socket->StartReceive();
    socket->StartSend(Socket::clock::now());
    socket->Loop();
}

UDPClient::UDPClient(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    LOG_INFO(Input, "Udp Initialization started");
    ReloadSockets();
}

UDPClient::~UDPClient() {
    Reset();
}

UDPClient::ClientConnection::ClientConnection() = default;

UDPClient::ClientConnection::~ClientConnection() = default;

void UDPClient::ReloadSockets() {
    Reset();

    std::stringstream servers_ss(Settings::values.udp_input_servers.GetValue());
    std::string server_token;
    std::size_t client = 0;
    while (std::getline(servers_ss, server_token, ',')) {
        if (client == MAX_UDP_CLIENTS) {
            break;
        }
        std::stringstream server_ss(server_token);
        std::string token;
        std::getline(server_ss, token, ':');
        std::string udp_input_address = token;
        std::getline(server_ss, token, ':');
        char* temp;
        const u16 udp_input_port = static_cast<u16>(std::strtol(token.c_str(), &temp, 0));
        if (*temp != '\0') {
            LOG_ERROR(Input, "Port number is not valid {}", token);
            continue;
        }

        const std::size_t client_number = GetClientNumber(udp_input_address, udp_input_port);
        if (client_number != MAX_UDP_CLIENTS) {
            LOG_ERROR(Input, "Duplicated UDP servers found");
            continue;
        }
        StartCommunication(client++, udp_input_address, udp_input_port);
    }
}

std::size_t UDPClient::GetClientNumber(std::string_view host, u16 port) const {
    for (std::size_t client = 0; client < clients.size(); client++) {
        if (clients[client].active == -1) {
            continue;
        }
        if (clients[client].host == host && clients[client].port == port) {
            return client;
        }
    }
    return MAX_UDP_CLIENTS;
}

BatteryLevel UDPClient::GetBatteryLevel(Response::Battery battery) const {
    switch (battery) {
    case Response::Battery::Dying:
        return BatteryLevel::Empty;
    case Response::Battery::Low:
        return BatteryLevel::Critical;
    case Response::Battery::Medium:
        return BatteryLevel::Low;
    case Response::Battery::High:
        return BatteryLevel::Medium;
    case Response::Battery::Full:
    case Response::Battery::Charged:
        return BatteryLevel::Full;
    case Response::Battery::Charging:
    default:
        return BatteryLevel::Charging;
    }
}

void UDPClient::OnVersion([[maybe_unused]] Response::Version data) {
    LOG_TRACE(Input, "Version packet received: {}", data.version);
}

void UDPClient::OnPortInfo([[maybe_unused]] Response::PortInfo data) {
    LOG_TRACE(Input, "PortInfo packet received: {}", data.model);
}

void UDPClient::OnPadData(Response::PadData data, std::size_t client) {
    const std::size_t pad_index = (client * PADS_PER_CLIENT) + data.info.id;

    if (pad_index >= pads.size()) {
        LOG_ERROR(Input, "Invalid pad id {}", data.info.id);
        return;
    }

    LOG_TRACE(Input, "PadData packet received");
    if (data.packet_counter == pads[pad_index].packet_sequence) {
        LOG_WARNING(
            Input,
            "PadData packet dropped because its stale info. Current count: {} Packet count: {}",
            pads[pad_index].packet_sequence, data.packet_counter);
        pads[pad_index].connected = false;
        return;
    }

    clients[client].active = 1;
    pads[pad_index].connected = true;
    pads[pad_index].packet_sequence = data.packet_counter;

    const auto now = std::chrono::steady_clock::now();
    const auto time_difference = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - pads[pad_index].last_update)
            .count());
    pads[pad_index].last_update = now;

    // Gyroscope values are not it the correct scale from better joy.
    // Dividing by 312 allows us to make one full turn = 1 turn
    // This must be a configurable valued called sensitivity
    const float gyro_scale = 1.0f / 312.0f;

    const BasicMotion motion{
        .gyro_x = data.gyro.pitch * gyro_scale,
        .gyro_y = data.gyro.roll * gyro_scale,
        .gyro_z = -data.gyro.yaw * gyro_scale,
        .accel_x = data.accel.x,
        .accel_y = -data.accel.z,
        .accel_z = data.accel.y,
        .delta_timestamp = time_difference,
    };
    const PadIdentifier identifier = GetPadIdentifier(pad_index);
    SetMotion(identifier, 0, motion);

    for (std::size_t id = 0; id < data.touch.size(); ++id) {
        const auto touch_pad = data.touch[id];
        const auto touch_axis_x_id =
            static_cast<int>(id == 0 ? PadAxes::Touch1X : PadAxes::Touch2X);
        const auto touch_axis_y_id =
            static_cast<int>(id == 0 ? PadAxes::Touch1Y : PadAxes::Touch2Y);
        const auto touch_button_id =
            static_cast<int>(id == 0 ? PadButton::Touch1 : PadButton::Touch2);

        // TODO: Use custom calibration per device
        const Common::ParamPackage touch_param(Settings::values.touch_device.GetValue());
        const u16 min_x = static_cast<u16>(touch_param.Get("min_x", 100));
        const u16 min_y = static_cast<u16>(touch_param.Get("min_y", 50));
        const u16 max_x = static_cast<u16>(touch_param.Get("max_x", 1800));
        const u16 max_y = static_cast<u16>(touch_param.Get("max_y", 850));

        const f32 x =
            static_cast<f32>(std::clamp(static_cast<u16>(touch_pad.x), min_x, max_x) - min_x) /
            static_cast<f32>(max_x - min_x);
        const f32 y =
            static_cast<f32>(std::clamp(static_cast<u16>(touch_pad.y), min_y, max_y) - min_y) /
            static_cast<f32>(max_y - min_y);

        if (touch_pad.is_active) {
            SetAxis(identifier, touch_axis_x_id, x);
            SetAxis(identifier, touch_axis_y_id, y);
            SetButton(identifier, touch_button_id, true);
            continue;
        }
        SetAxis(identifier, touch_axis_x_id, 0);
        SetAxis(identifier, touch_axis_y_id, 0);
        SetButton(identifier, touch_button_id, false);
    }

    SetAxis(identifier, static_cast<int>(PadAxes::LeftStickX),
            (data.left_stick_x - 127.0f) / 127.0f);
    SetAxis(identifier, static_cast<int>(PadAxes::LeftStickY),
            (data.left_stick_y - 127.0f) / 127.0f);
    SetAxis(identifier, static_cast<int>(PadAxes::RightStickX),
            (data.right_stick_x - 127.0f) / 127.0f);
    SetAxis(identifier, static_cast<int>(PadAxes::RightStickY),
            (data.right_stick_y - 127.0f) / 127.0f);

    static constexpr std::array<PadButton, 16> buttons{
        PadButton::Share,    PadButton::L3,     PadButton::R3,    PadButton::Options,
        PadButton::Up,       PadButton::Right,  PadButton::Down,  PadButton::Left,
        PadButton::L2,       PadButton::R2,     PadButton::L1,    PadButton::R1,
        PadButton::Triangle, PadButton::Circle, PadButton::Cross, PadButton::Square};

    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const bool button_status = (data.digital_button & (1U << i)) != 0;
        const int button = static_cast<int>(buttons[i]);
        SetButton(identifier, button, button_status);
    }

    SetButton(identifier, static_cast<int>(PadButton::Home), data.home != 0);
    SetButton(identifier, static_cast<int>(PadButton::TouchHardPress), data.touch_hard_press != 0);

    SetBattery(identifier, GetBatteryLevel(data.info.battery));
}

void UDPClient::StartCommunication(std::size_t client, const std::string& host, u16 port) {
    SocketCallback callback{[this](Response::Version version) { OnVersion(version); },
                            [this](Response::PortInfo info) { OnPortInfo(info); },
                            [this, client](Response::PadData data) { OnPadData(data, client); }};
    LOG_INFO(Input, "Starting communication with UDP input server on {}:{}", host, port);
    clients[client].uuid = GetHostUUID(host);
    clients[client].host = host;
    clients[client].port = port;
    clients[client].active = 0;
    clients[client].socket = std::make_unique<Socket>(host, port, callback);
    clients[client].thread = std::thread{SocketLoop, clients[client].socket.get()};
    for (std::size_t index = 0; index < PADS_PER_CLIENT; ++index) {
        const PadIdentifier identifier = GetPadIdentifier(client * PADS_PER_CLIENT + index);
        PreSetController(identifier);
    }
}

PadIdentifier UDPClient::GetPadIdentifier(std::size_t pad_index) const {
    const std::size_t client = pad_index / PADS_PER_CLIENT;
    return {
        .guid = clients[client].uuid,
        .port = static_cast<std::size_t>(clients[client].port),
        .pad = pad_index,
    };
}

Common::UUID UDPClient::GetHostUUID(const std::string& host) const {
    const auto ip = boost::asio::ip::make_address_v4(host);
    const auto hex_host = fmt::format("{:06x}", ip.to_uint());
    return Common::UUID{hex_host};
}

void UDPClient::Reset() {
    for (auto& client : clients) {
        if (client.thread.joinable()) {
            client.active = -1;
            client.socket->Stop();
            client.thread.join();
        }
    }
}

std::vector<Common::ParamPackage> UDPClient::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices;
    if (!Settings::values.enable_udp_controller) {
        return devices;
    }
    for (std::size_t client = 0; client < clients.size(); client++) {
        if (clients[client].active != 1) {
            continue;
        }
        for (std::size_t index = 0; index < PADS_PER_CLIENT; ++index) {
            const std::size_t pad_index = client * PADS_PER_CLIENT + index;
            if (!pads[pad_index].connected) {
                continue;
            }
            const auto pad_identifier = GetPadIdentifier(pad_index);
            Common::ParamPackage identifier{};
            identifier.Set("engine", GetEngineName());
            identifier.Set("display", fmt::format("UDP Controller {}", pad_identifier.pad));
            identifier.Set("guid", pad_identifier.guid.Format());
            identifier.Set("port", static_cast<int>(pad_identifier.port));
            identifier.Set("pad", static_cast<int>(pad_identifier.pad));
            devices.emplace_back(identifier);
        }
    }
    return devices;
}

ButtonMapping UDPClient::GetButtonMappingForDevice(const Common::ParamPackage& params) {
    // This list excludes any button that can't be really mapped
    static constexpr std::array<std::pair<Settings::NativeButton::Values, PadButton>, 20>
        switch_to_dsu_button = {
            std::pair{Settings::NativeButton::A, PadButton::Circle},
            {Settings::NativeButton::B, PadButton::Cross},
            {Settings::NativeButton::X, PadButton::Triangle},
            {Settings::NativeButton::Y, PadButton::Square},
            {Settings::NativeButton::Plus, PadButton::Options},
            {Settings::NativeButton::Minus, PadButton::Share},
            {Settings::NativeButton::DLeft, PadButton::Left},
            {Settings::NativeButton::DUp, PadButton::Up},
            {Settings::NativeButton::DRight, PadButton::Right},
            {Settings::NativeButton::DDown, PadButton::Down},
            {Settings::NativeButton::L, PadButton::L1},
            {Settings::NativeButton::R, PadButton::R1},
            {Settings::NativeButton::ZL, PadButton::L2},
            {Settings::NativeButton::ZR, PadButton::R2},
            {Settings::NativeButton::SL, PadButton::L2},
            {Settings::NativeButton::SR, PadButton::R2},
            {Settings::NativeButton::LStick, PadButton::L3},
            {Settings::NativeButton::RStick, PadButton::R3},
            {Settings::NativeButton::Home, PadButton::Home},
            {Settings::NativeButton::Screenshot, PadButton::TouchHardPress},
        };
    if (!params.Has("guid") || !params.Has("port") || !params.Has("pad")) {
        return {};
    }

    ButtonMapping mapping{};
    for (const auto& [switch_button, dsu_button] : switch_to_dsu_button) {
        Common::ParamPackage button_params{};
        button_params.Set("engine", GetEngineName());
        button_params.Set("guid", params.Get("guid", ""));
        button_params.Set("port", params.Get("port", 0));
        button_params.Set("pad", params.Get("pad", 0));
        button_params.Set("button", static_cast<int>(dsu_button));
        mapping.insert_or_assign(switch_button, std::move(button_params));
    }

    return mapping;
}

AnalogMapping UDPClient::GetAnalogMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("guid") || !params.Has("port") || !params.Has("pad")) {
        return {};
    }

    AnalogMapping mapping = {};
    Common::ParamPackage left_analog_params;
    left_analog_params.Set("engine", GetEngineName());
    left_analog_params.Set("guid", params.Get("guid", ""));
    left_analog_params.Set("port", params.Get("port", 0));
    left_analog_params.Set("pad", params.Get("pad", 0));
    left_analog_params.Set("axis_x", static_cast<int>(PadAxes::LeftStickX));
    left_analog_params.Set("axis_y", static_cast<int>(PadAxes::LeftStickY));
    mapping.insert_or_assign(Settings::NativeAnalog::LStick, std::move(left_analog_params));
    Common::ParamPackage right_analog_params;
    right_analog_params.Set("engine", GetEngineName());
    right_analog_params.Set("guid", params.Get("guid", ""));
    right_analog_params.Set("port", params.Get("port", 0));
    right_analog_params.Set("pad", params.Get("pad", 0));
    right_analog_params.Set("axis_x", static_cast<int>(PadAxes::RightStickX));
    right_analog_params.Set("axis_y", static_cast<int>(PadAxes::RightStickY));
    mapping.insert_or_assign(Settings::NativeAnalog::RStick, std::move(right_analog_params));
    return mapping;
}

MotionMapping UDPClient::GetMotionMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("guid") || !params.Has("port") || !params.Has("pad")) {
        return {};
    }

    MotionMapping mapping = {};
    Common::ParamPackage left_motion_params;
    left_motion_params.Set("engine", GetEngineName());
    left_motion_params.Set("guid", params.Get("guid", ""));
    left_motion_params.Set("port", params.Get("port", 0));
    left_motion_params.Set("pad", params.Get("pad", 0));
    left_motion_params.Set("motion", 0);

    Common::ParamPackage right_motion_params;
    right_motion_params.Set("engine", GetEngineName());
    right_motion_params.Set("guid", params.Get("guid", ""));
    right_motion_params.Set("port", params.Get("port", 0));
    right_motion_params.Set("pad", params.Get("pad", 0));
    right_motion_params.Set("motion", 0);

    mapping.insert_or_assign(Settings::NativeMotion::MotionLeft, std::move(left_motion_params));
    mapping.insert_or_assign(Settings::NativeMotion::MotionRight, std::move(right_motion_params));
    return mapping;
}

Common::Input::ButtonNames UDPClient::GetUIButtonName(const Common::ParamPackage& params) const {
    PadButton button = static_cast<PadButton>(params.Get("button", 0));
    switch (button) {
    case PadButton::Left:
        return Common::Input::ButtonNames::ButtonLeft;
    case PadButton::Right:
        return Common::Input::ButtonNames::ButtonRight;
    case PadButton::Down:
        return Common::Input::ButtonNames::ButtonDown;
    case PadButton::Up:
        return Common::Input::ButtonNames::ButtonUp;
    case PadButton::L1:
        return Common::Input::ButtonNames::L1;
    case PadButton::L2:
        return Common::Input::ButtonNames::L2;
    case PadButton::L3:
        return Common::Input::ButtonNames::L3;
    case PadButton::R1:
        return Common::Input::ButtonNames::R1;
    case PadButton::R2:
        return Common::Input::ButtonNames::R2;
    case PadButton::R3:
        return Common::Input::ButtonNames::R3;
    case PadButton::Circle:
        return Common::Input::ButtonNames::Circle;
    case PadButton::Cross:
        return Common::Input::ButtonNames::Cross;
    case PadButton::Square:
        return Common::Input::ButtonNames::Square;
    case PadButton::Triangle:
        return Common::Input::ButtonNames::Triangle;
    case PadButton::Share:
        return Common::Input::ButtonNames::Share;
    case PadButton::Options:
        return Common::Input::ButtonNames::Options;
    case PadButton::Home:
        return Common::Input::ButtonNames::Home;
    case PadButton::Touch1:
    case PadButton::Touch2:
    case PadButton::TouchHardPress:
        return Common::Input::ButtonNames::Touch;
    default:
        return Common::Input::ButtonNames::Undefined;
    }
}

Common::Input::ButtonNames UDPClient::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        return GetUIButtonName(params);
    }
    if (params.Has("axis")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("motion")) {
        return Common::Input::ButtonNames::Engine;
    }

    return Common::Input::ButtonNames::Invalid;
}

void TestCommunication(const std::string& host, u16 port,
                       const std::function<void()>& success_callback,
                       const std::function<void()>& failure_callback) {
    std::thread([=] {
        Common::Event success_event;
        SocketCallback callback{
            .version = [](Response::Version) {},
            .port_info = [](Response::PortInfo) {},
            .pad_data = [&](Response::PadData) { success_event.Set(); },
        };
        Socket socket{host, port, std::move(callback)};
        std::thread worker_thread{SocketLoop, &socket};
        const bool result =
            success_event.WaitUntil(std::chrono::steady_clock::now() + std::chrono::seconds(10));
        socket.Stop();
        worker_thread.join();
        if (result) {
            success_callback();
        } else {
            failure_callback();
        }
    }).detach();
}

CalibrationConfigurationJob::CalibrationConfigurationJob(
    const std::string& host, u16 port, std::function<void(Status)> status_callback,
    std::function<void(u16, u16, u16, u16)> data_callback) {

    std::thread([=, this] {
        u16 min_x{UINT16_MAX};
        u16 min_y{UINT16_MAX};
        u16 max_x{};
        u16 max_y{};

        Status current_status{Status::Initialized};
        SocketCallback callback{[](Response::Version) {}, [](Response::PortInfo) {},
                                [&](Response::PadData data) {
                                    constexpr u16 CALIBRATION_THRESHOLD = 100;

                                    if (current_status == Status::Initialized) {
                                        // Receiving data means the communication is ready now
                                        current_status = Status::Ready;
                                        status_callback(current_status);
                                    }
                                    if (data.touch[0].is_active == 0) {
                                        return;
                                    }
                                    LOG_DEBUG(Input, "Current touch: {} {}", data.touch[0].x,
                                              data.touch[0].y);
                                    min_x = std::min(min_x, static_cast<u16>(data.touch[0].x));
                                    min_y = std::min(min_y, static_cast<u16>(data.touch[0].y));
                                    if (current_status == Status::Ready) {
                                        // First touch - min data (min_x/min_y)
                                        current_status = Status::Stage1Completed;
                                        status_callback(current_status);
                                    }
                                    if (data.touch[0].x - min_x > CALIBRATION_THRESHOLD &&
                                        data.touch[0].y - min_y > CALIBRATION_THRESHOLD) {
                                        // Set the current position as max value and finishes
                                        // configuration
                                        max_x = data.touch[0].x;
                                        max_y = data.touch[0].y;
                                        current_status = Status::Completed;
                                        data_callback(min_x, min_y, max_x, max_y);
                                        status_callback(current_status);

                                        complete_event.Set();
                                    }
                                }};
        Socket socket{host, port, std::move(callback)};
        std::thread worker_thread{SocketLoop, &socket};
        complete_event.Wait();
        socket.Stop();
        worker_thread.join();
    }).detach();
}

CalibrationConfigurationJob::~CalibrationConfigurationJob() {
    Stop();
}

void CalibrationConfigurationJob::Stop() {
    complete_event.Set();
}

} // namespace InputCommon::CemuhookUDP
