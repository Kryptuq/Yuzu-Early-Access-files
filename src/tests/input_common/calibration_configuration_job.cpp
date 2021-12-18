// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <boost/crc.hpp>
#include <catch2/catch.hpp>

#include "input_common/drivers/udp_client.h"
#include "input_common/helpers/udp_protocol.h"

class FakeCemuhookServer {
public:
    FakeCemuhookServer()
        : socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)) {}

    ~FakeCemuhookServer() {
        is_running = false;
        boost::system::error_code error_code;
        socket.shutdown(boost::asio::socket_base::shutdown_both, error_code);
        socket.close();
        if (handler.joinable()) {
            handler.join();
        }
    }

    u16 GetPort() {
        return socket.local_endpoint().port();
    }

    std::string GetHost() {
        return socket.local_endpoint().address().to_string();
    }

    void Run(const std::vector<InputCommon::CemuhookUDP::Response::TouchPad> touch_movement_path) {
        constexpr size_t HeaderSize = sizeof(InputCommon::CemuhookUDP::Header);
        constexpr size_t PadDataSize =
            sizeof(InputCommon::CemuhookUDP::Message<InputCommon::CemuhookUDP::Response::PadData>);

        REQUIRE(touch_movement_path.size() > 0);
        is_running = true;
        handler = std::thread([touch_movement_path, this]() {
            auto current_touch_position = touch_movement_path.begin();
            while (is_running) {
                boost::asio::ip::udp::endpoint sender_endpoint;
                boost::system::error_code error_code;
                auto received_size = socket.receive_from(boost::asio::buffer(receive_buffer),
                                                         sender_endpoint, 0, error_code);

                if (received_size < HeaderSize) {
                    continue;
                }

                InputCommon::CemuhookUDP::Header header{};
                std::memcpy(&header, receive_buffer.data(), HeaderSize);
                switch (header.type) {
                case InputCommon::CemuhookUDP::Type::PadData: {
                    InputCommon::CemuhookUDP::Response::PadData pad_data{};
                    pad_data.touch[0] = *current_touch_position;
                    const auto pad_message = InputCommon::CemuhookUDP::CreateMessage(
                        InputCommon::CemuhookUDP::SERVER_MAGIC, pad_data, 0);
                    std::memcpy(send_buffer.data(), &pad_message, PadDataSize);
                    socket.send_to(boost::asio::buffer(send_buffer, PadDataSize), sender_endpoint,
                                   0, error_code);

                    bool can_advance =
                        std::next(current_touch_position) != touch_movement_path.end();
                    if (can_advance) {
                        std::advance(current_touch_position, 1);
                    }
                    break;
                }
                case InputCommon::CemuhookUDP::Type::PortInfo:
                case InputCommon::CemuhookUDP::Type::Version:
                default:
                    break;
                }
            }
        });
    }

private:
    boost::asio::io_service io_service;
    boost::asio::ip::udp::socket socket;
    std::array<u8, InputCommon::CemuhookUDP::MAX_PACKET_SIZE> send_buffer;
    std::array<u8, InputCommon::CemuhookUDP::MAX_PACKET_SIZE> receive_buffer;
    bool is_running = false;
    std::thread handler;
};

TEST_CASE("CalibrationConfigurationJob completed", "[input_common]") {
    Common::Event complete_event;
    FakeCemuhookServer server;
    server.Run({{
                    .is_active = 1,
                    .x = 0,
                    .y = 0,
                },
                {
                    .is_active = 1,
                    .x = 200,
                    .y = 200,
                }});

    InputCommon::CemuhookUDP::CalibrationConfigurationJob::Status status{};
    u16 min_x{};
    u16 min_y{};
    u16 max_x{};
    u16 max_y{};
    InputCommon::CemuhookUDP::CalibrationConfigurationJob job(
        server.GetHost(), server.GetPort(),
        [&status,
         &complete_event](InputCommon::CemuhookUDP::CalibrationConfigurationJob::Status status_) {
            status = status_;
            if (status ==
                InputCommon::CemuhookUDP::CalibrationConfigurationJob::Status::Completed) {
                complete_event.Set();
            }
        },
        [&](u16 min_x_, u16 min_y_, u16 max_x_, u16 max_y_) {
            min_x = min_x_;
            min_y = min_y_;
            max_x = max_x_;
            max_y = max_y_;
        });

    complete_event.WaitUntil(std::chrono::system_clock::now() + std::chrono::seconds(10));
    REQUIRE(status == InputCommon::CemuhookUDP::CalibrationConfigurationJob::Status::Completed);
    REQUIRE(min_x == 0);
    REQUIRE(min_y == 0);
    REQUIRE(max_x == 200);
    REQUIRE(max_y == 200);
}
