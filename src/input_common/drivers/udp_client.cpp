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
            Request::PadData::Flags::AllPorts,
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

UDPClient::UDPClient(const std::string& input_engine_) : InputEngine(input_engine_) {
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
        const int touch_id = static_cast<int>(client * 2 + id);

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
            SetAxis(identifier, touch_id * 2, x);
            SetAxis(identifier, touch_id * 2 + 1, y);
            SetButton(identifier, touch_id, true);
            continue;
        }
        SetAxis(identifier, touch_id * 2, 0);
        SetAxis(identifier, touch_id * 2 + 1, 0);
        SetButton(identifier, touch_id, false);
    }
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

const PadIdentifier UDPClient::GetPadIdentifier(std::size_t pad_index) const {
    const std::size_t client = pad_index / PADS_PER_CLIENT;
    return {
        .guid = clients[client].uuid,
        .port = static_cast<std::size_t>(clients[client].port),
        .pad = pad_index,
    };
}

const Common::UUID UDPClient::GetHostUUID(const std::string host) const {
    const auto ip = boost::asio::ip::address_v4::from_string(host);
    const auto hex_host = fmt::format("{:06x}", ip.to_ulong());
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
        Status current_status{Status::Initialized};
        SocketCallback callback{
            [](Response::Version) {}, [](Response::PortInfo) {},
            [&](Response::PadData data) {
                static constexpr u16 CALIBRATION_THRESHOLD = 100;
                static constexpr u16 MAX_VALUE = UINT16_MAX;

                if (current_status == Status::Initialized) {
                    // Receiving data means the communication is ready now
                    current_status = Status::Ready;
                    status_callback(current_status);
                }
                const auto& touchpad_0 = data.touch[0];
                if (touchpad_0.is_active == 0) {
                    return;
                }
                LOG_DEBUG(Input, "Current touch: {} {}", touchpad_0.x, touchpad_0.y);
                const u16 min_x = std::min(MAX_VALUE, static_cast<u16>(touchpad_0.x));
                const u16 min_y = std::min(MAX_VALUE, static_cast<u16>(touchpad_0.y));
                if (current_status == Status::Ready) {
                    // First touch - min data (min_x/min_y)
                    current_status = Status::Stage1Completed;
                    status_callback(current_status);
                }
                if (touchpad_0.x - min_x > CALIBRATION_THRESHOLD &&
                    touchpad_0.y - min_y > CALIBRATION_THRESHOLD) {
                    // Set the current position as max value and finishes configuration
                    const u16 max_x = touchpad_0.x;
                    const u16 max_y = touchpad_0.y;
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
