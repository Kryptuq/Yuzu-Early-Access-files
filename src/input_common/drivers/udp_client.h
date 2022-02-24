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
enum class Battery : u8;
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
class UDPClient final : public InputEngine {
public:
    explicit UDPClient(std::string input_engine_);
    ~UDPClient() override;

    void ReloadSockets();

    /// Used for automapping features
    std::vector<Common::ParamPackage> GetInputDevices() const override;
    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage& params) override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    MotionMapping GetMotionMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;

private:
    enum class PadButton {
        Undefined = 0x0000,
        Share = 0x0001,
        L3 = 0x0002,
        R3 = 0x0004,
        Options = 0x0008,
        Up = 0x0010,
        Right = 0x0020,
        Down = 0x0040,
        Left = 0x0080,
        L2 = 0x0100,
        R2 = 0x0200,
        L1 = 0x0400,
        R1 = 0x0800,
        Triangle = 0x1000,
        Circle = 0x2000,
        Cross = 0x4000,
        Square = 0x8000,
        Touch1 = 0x10000,
        Touch2 = 0x20000,
        Home = 0x40000,
        TouchHardPress = 0x80000,
    };

    enum class PadAxes : u8 {
        LeftStickX,
        LeftStickY,
        RightStickX,
        RightStickY,
        AnalogLeft,
        AnalogDown,
        AnalogRight,
        AnalogUp,
        AnalogSquare,
        AnalogCross,
        AnalogCircle,
        AnalogTriangle,
        AnalogR1,
        AnalogL1,
        AnalogR2,
        AnalogL3,
        AnalogR3,
        Touch1X,
        Touch1Y,
        Touch2X,
        Touch2Y,
        Undefined,
    };

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
        Common::UUID uuid{"00000000-0000-0000-0000-00007F000001"};
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

    // Translates UDP battery level to input engine battery level
    Common::Input::BatteryLevel GetBatteryLevel(Response::Battery battery) const;

    void OnVersion(Response::Version);
    void OnPortInfo(Response::PortInfo);
    void OnPadData(Response::PadData, std::size_t client);
    void StartCommunication(std::size_t client, const std::string& host, u16 port);
    PadIdentifier GetPadIdentifier(std::size_t pad_index) const;
    Common::UUID GetHostUUID(const std::string& host) const;

    Common::Input::ButtonNames GetUIButtonName(const Common::ParamPackage& params) const;

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
