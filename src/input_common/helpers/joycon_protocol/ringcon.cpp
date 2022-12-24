// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "input_common/helpers/joycon_protocol/ringcon.h"

namespace InputCommon::Joycon {

RingConProtocol::RingConProtocol(std::shared_ptr<JoyconHandle> handle)
    : JoyconCommonProtocol(std::move(handle)) {}

DriverResult RingConProtocol::EnableRingCon() {
    LOG_DEBUG(Input, "Enable Ringcon");
    DriverResult result{DriverResult::Success};
    SetBlocking();

    if (result == DriverResult::Success) {
        result = SetReportMode(ReportMode::STANDARD_FULL_60HZ);
    }
    if (result == DriverResult::Success) {
        result = EnableMCU(true);
    }
    if (result == DriverResult::Success) {
        const MCUConfig config{
            .command = MCUCommand::ConfigureMCU,
            .sub_command = MCUSubCommand::SetDeviceMode,
            .mode = MCUMode::Standby,
            .crc = {},
        };
        result = ConfigureMCU(config);
    }

    SetNonBlocking();
    return result;
}

DriverResult RingConProtocol::DisableRingCon() {
    LOG_DEBUG(Input, "Disable RingCon");
    DriverResult result{DriverResult::Success};
    SetBlocking();

    if (result == DriverResult::Success) {
        result = EnableMCU(false);
    }

    is_enabled = false;

    SetNonBlocking();
    return result;
}

DriverResult RingConProtocol::StartRingconPolling() {
    LOG_DEBUG(Input, "Enable Ringcon");
    bool is_connected = false;
    DriverResult result{DriverResult::Success};
    SetBlocking();

    if (result == DriverResult::Success) {
        result = IsRingConnected(is_connected);
    }
    if (result == DriverResult::Success && is_connected) {
        LOG_INFO(Input, "Ringcon detected");
        result = ConfigureRing();
    }
    if (result == DriverResult::Success) {
        is_enabled = true;
    }

    SetNonBlocking();
    return result;
}

DriverResult RingConProtocol::IsRingConnected(bool& is_connected) {
    LOG_DEBUG(Input, "IsRingConnected");
    constexpr std::size_t max_tries = 28;
    std::vector<u8> output;
    std::size_t tries = 0;
    is_connected = false;

    do {
        std::array<u8, 1> empty_data{};
        const auto result = SendSubCommand(SubCommand::UNKNOWN_RINGCON, empty_data, output);

        if (result != DriverResult::Success) {
            return result;
        }

        if (tries++ >= max_tries) {
            return DriverResult::NoDeviceDetected;
        }
    } while (output[14] != 0x59 || output[16] != 0x20);

    is_connected = true;
    return DriverResult::Success;
}

DriverResult RingConProtocol::ConfigureRing() {
    LOG_DEBUG(Input, "ConfigureRing");
    constexpr std::size_t max_tries = 28;
    DriverResult result{DriverResult::Success};
    std::vector<u8> output;
    std::size_t tries = 0;

    static constexpr std::array<u8, 37> ring_config{
        0x06, 0x03, 0x25, 0x06, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x16, 0xED, 0x34, 0x36,
        0x00, 0x00, 0x00, 0x0A, 0x64, 0x0B, 0xE6, 0xA9, 0x22, 0x00, 0x00, 0x04, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0xA8, 0xE1, 0x34, 0x36};
    do {
        result = SendSubCommand(SubCommand::UNKNOWN_RINGCON3, ring_config, output);

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ >= max_tries) {
            return DriverResult::NoDeviceDetected;
        }
    } while (output[14] != 0x5C);

    static constexpr std::array<u8, 4> ringcon_data{0x04, 0x01, 0x01, 0x02};
    result = SendSubCommand(SubCommand::UNKNOWN_RINGCON2, ringcon_data, output);

    return result;
}

bool RingConProtocol::IsEnabled() const {
    return is_enabled;
}

} // namespace InputCommon::Joycon
