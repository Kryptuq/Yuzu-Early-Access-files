// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include "common/atomic_threadsafe_queue.h"
#include "common/common_types.h"
#include "common/param_package.h"
#include "common/thread.h"
#include "common/vector_math.h"
#include "core/frontend/input.h"
#include "input_common/motion_input.h"

namespace InputCommon::CemuhookUDP {

class Socket;

namespace Response {
struct PadData;
struct PortInfo;
struct TouchPad;
struct Version;
} // namespace Response

enum class PadMotion {
    GyroX,
    GyroY,
    GyroZ,
    AccX,
    AccY,
    AccZ,
    Undefined,
};

enum class PadTouch {
    Click,
    Undefined,
};

struct UDPPadStatus {
    const char* host{"127.0.0.1"};
    u16 port{26760};
    std::size_t pad_index{};
    PadMotion motion{PadMotion::Undefined};
    f32 motion_value{0.0f};
};

struct DeviceStatus {
    std::mutex update_mutex;
    Input::MotionStatus motion_status;
    std::tuple<float, float, bool> touch_status;

    // calibration data for scaling the device's touch area to 3ds
    struct CalibrationData {
        u16 min_x{};
        u16 min_y{};
        u16 max_x{};
        u16 max_y{};
    };
    std::optional<CalibrationData> touch_calibration;
};

class Client {
public:
    // Initialize the UDP client capture and read sequence
    Client();

    // Close and release the client
    ~Client();

    // Used for polling
    void BeginConfiguration();
    void EndConfiguration();

    std::vector<Common::ParamPackage> GetInputDevices() const;

    bool DeviceConnected(std::size_t pad) const;
    void ReloadSockets();

    Common::MPMCQueue<UDPPadStatus>& GetPadQueue();
    const Common::MPMCQueue<UDPPadStatus>& GetPadQueue() const;

    DeviceStatus& GetPadState(const std::string& host, u16 port, std::size_t pad);
    const DeviceStatus& GetPadState(const std::string& host, u16 port, std::size_t pad) const;

    Input::TouchStatus& GetTouchState();
    const Input::TouchStatus& GetTouchState() const;

private:
    struct PadData {
        std::size_t pad_index{};
        bool connected{};
        DeviceStatus status;
        u64 packet_sequence{};

        // Realtime values
        // motion is initalized with PID values for drift correction on joycons
        InputCommon::MotionInput motion{0.3f, 0.005f, 0.0f};
        std::chrono::time_point<std::chrono::steady_clock> last_update;
    };

    struct ClientConnection {
        ClientConnection();
        ~ClientConnection();
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
    void UpdateYuzuSettings(std::size_t client, std::size_t pad_index,
                            const Common::Vec3<float>& acc, const Common::Vec3<float>& gyro);

    // Returns an unused finger id, if there is no fingers available std::nullopt will be
    // returned
    std::optional<std::size_t> GetUnusedFingerID() const;

    // Merges and updates all touch inputs into the touch_status array
    void UpdateTouchInput(Response::TouchPad& touch_pad, std::size_t client, std::size_t id);

    bool configuring = false;

    // Allocate clients for 8 udp servers
    static constexpr std::size_t MAX_UDP_CLIENTS = 8;
    static constexpr std::size_t PADS_PER_CLIENT = 4;
    // Each client can have up 2 touch inputs
    static constexpr std::size_t MAX_TOUCH_FINGERS = MAX_UDP_CLIENTS * 2;
    std::array<PadData, MAX_UDP_CLIENTS * PADS_PER_CLIENT> pads{};
    std::array<ClientConnection, MAX_UDP_CLIENTS> clients{};
    Common::MPMCQueue<UDPPadStatus> pad_queue{1024};
    Input::TouchStatus touch_status{};
    std::array<std::size_t, MAX_TOUCH_FINGERS> finger_id{};
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
