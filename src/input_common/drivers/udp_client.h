// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <optional>

#include "common/common_types.h"
#include "common/thread.h"
#include "input_common/input_engine.h"

namespace InputCommon::CemuhookUDP {

class Socket;

namespace Response {
struct PadData;
struct PortInfo;
struct TouchPad;
struct Version;
} // namespace Response

enum class PadTouch {
    Click,
    Undefined,
};

struct UDPPadStatus {
    std::string host{"127.0.0.1"};
    u16 port{26760};
    std::size_t pad_index{};
};

struct DeviceStatus {
    std::mutex update_mutex;

    // calibration data for scaling the device's touch area to 3ds
    struct CalibrationData {
        u16 min_x{};
        u16 min_y{};
        u16 max_x{};
        u16 max_y{};
    };
    std::optional<CalibrationData> touch_calibration;
};

/**
 * A button device factory representing a keyboard. It receives keyboard events and forward them
 * to all button devices it created.
 */
class UDPClient final : public InputCommon::InputEngine {
public:
    explicit UDPClient(const std::string& input_engine_);
    ~UDPClient();

    void ReloadSockets();

private:
    struct PadData {
        std::size_t pad_index{};
        bool connected{};
        DeviceStatus status;
        u64 packet_sequence{};

        std::chrono::time_point<std::chrono::steady_clock> last_update;
    };

    struct ClientConnection {
        ClientConnection();
        ~ClientConnection();
        Common::UUID uuid{"7F000001"};
        std::string host{"127.0.0.1"};
        u16 port{26760};
        s8 active{-1};
        std::unique_ptr<Socket> socket;
        std::thread thread;
    };

    // For shutting down, clear all data, join all threads, release usb
    void Reset();

    // Translates configuration to client number
    std::size_t GetClientNumber(std::string_view host, u16 port) const;

    void OnVersion(Response::Version);
    void OnPortInfo(Response::PortInfo);
    void OnPadData(Response::PadData, std::size_t client);
    void StartCommunication(std::size_t client, const std::string& host, u16 port);
    const PadIdentifier GetPadIdentifier(std::size_t pad_index) const;
    const Common::UUID GetHostUUID(const std::string host) const;

    // Allocate clients for 8 udp servers
    static constexpr std::size_t MAX_UDP_CLIENTS = 8;
    static constexpr std::size_t PADS_PER_CLIENT = 4;
    std::array<PadData, MAX_UDP_CLIENTS * PADS_PER_CLIENT> pads{};
    std::array<ClientConnection, MAX_UDP_CLIENTS> clients{};
};

/// An async job allowing configuration of the touchpad calibration.
class CalibrationConfigurationJob {
public:
    enum class Status {
        Initialized,
        Ready,
        Stage1Completed,
        Completed,
    };
    /**
     * Constructs and starts the job with the specified parameter.
     *
     * @param status_callback Callback for job status updates
     * @param data_callback Called when calibration data is ready
     */
    explicit CalibrationConfigurationJob(const std::string& host, u16 port,
                                         std::function<void(Status)> status_callback,
                                         std::function<void(u16, u16, u16, u16)> data_callback);
    ~CalibrationConfigurationJob();
    void Stop();

private:
    Common::Event complete_event;
};

void TestCommunication(const std::string& host, u16 port,
                       const std::function<void()>& success_callback,
                       const std::function<void()>& failure_callback);

} // namespace InputCommon::CemuhookUDP
