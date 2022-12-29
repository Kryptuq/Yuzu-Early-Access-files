// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include "common/logging/log.h"
#include "input_common/helpers/joycon_protocol/irs.h"

namespace InputCommon::Joycon {

IrsProtocol::IrsProtocol(std::shared_ptr<JoyconHandle> handle)
    : JoyconCommonProtocol(std::move(handle)) {}

DriverResult IrsProtocol::EnableIrs() {
    LOG_INFO(Input, "Enable IRS");
    DriverResult result{DriverResult::Success};
    SetBlocking();

    if (result == DriverResult::Success) {
        result = SetReportMode(ReportMode::NFC_IR_MODE_60HZ);
    }
    if (result == DriverResult::Success) {
        result = EnableMCU(true);
    }
    if (result == DriverResult::Success) {
        result = WaitSetMCUMode(ReportMode::NFC_IR_MODE_60HZ, MCUMode::Standby);
    }
    if (result == DriverResult::Success) {
        const MCUConfig config{
            .command = MCUCommand::ConfigureMCU,
            .sub_command = MCUSubCommand::SetMCUMode,
            .mode = MCUMode::IR,
            .crc = {},
        };

        result = ConfigureMCU(config);
    }
    if (result == DriverResult::Success) {
        result = WaitSetMCUMode(ReportMode::NFC_IR_MODE_60HZ, MCUMode::IR);
    }
    if (result == DriverResult::Success) {
        result = ConfigureIrs();
    }
    if (result == DriverResult::Success) {
        result = WriteRegistersStep1();
    }
    if (result == DriverResult::Success) {
        result = WriteRegistersStep2();
    }

    is_enabled = true;

    SetNonBlocking();
    return result;
}

DriverResult IrsProtocol::DisableIrs() {
    LOG_DEBUG(Input, "Disable IRS");
    DriverResult result{DriverResult::Success};
    SetBlocking();

    if (result == DriverResult::Success) {
        result = EnableMCU(false);
    }

    is_enabled = false;

    SetNonBlocking();
    return result;
}

DriverResult IrsProtocol::ConfigureIrs() {
    LOG_DEBUG(Input, "Configure IRS");
    constexpr std::size_t max_tries = 28;
    std::vector<u8> output;
    std::size_t tries = 0;

    const IrsConfigure irs_configuration{
        .command = MCUCommand::ConfigureIR,
        .sub_command = MCUSubCommand::SetDeviceMode,
        .irs_mode = IrsMode::ImageTransfer,
        .number_of_fragments = 0x3,
        .mcu_major_version = 0x0500,
        .mcu_minor_version = 0x1800,
        .crc = {},
    };

    std::vector<u8> request_data(sizeof(IrsConfigure));
    memcpy(request_data.data(), &irs_configuration, sizeof(IrsConfigure));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);
    do {
        const auto result = SendSubCommand(SubCommand::SET_MCU_CONFIG, request_data, output);

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ >= max_tries) {
            return DriverResult::WrongReply;
        }
    } while (output[15] != 0x0b);

    return DriverResult::Success;
}

DriverResult IrsProtocol::WriteRegistersStep1() {
    LOG_DEBUG(Input, "Configure IRS");
    DriverResult result{DriverResult::Success};
    constexpr std::size_t max_tries = 28;
    std::vector<u8> output;
    std::size_t tries = 0;

    const IrsWriteRegisters irs_registers{
        .command = MCUCommand::ConfigureIR,
        .sub_command = MCUSubCommand::WriteDeviceRegisters,
        .number_of_registers = 0x9,
        .registers =
            {
                IrsRegister{0x2e00, resolution},
                {0x3001, static_cast<u8>(exposure & 0xff)},
                {0x3101, static_cast<u8>(exposure >> 8)},
                {0x3201, 0x00},
                {0x1000, leds},
                {0x2e01, static_cast<u8>((digital_gain & 0x0f) << 4)},
                {0x2f01, static_cast<u8>((digital_gain & 0xf0) >> 4)},
                {0x0e00, ex_light_filter},
                {0x4301, 0xc8},
            },
        .crc = {},
    };

    std::vector<u8> request_data(sizeof(IrsWriteRegisters));
    memcpy(request_data.data(), &irs_registers, sizeof(IrsWriteRegisters));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);

    std::array<u8, 38> mcu_request{0x02};
    mcu_request[36] = CalculateMCU_CRC8(mcu_request.data(), 36);
    mcu_request[37] = 0xFF;

    if (result != DriverResult::Success) {
        return result;
    }

    do {
        result = SendSubCommand(SubCommand::SET_MCU_CONFIG, request_data, output);

        // First time we need to set the report mode
        if (result == DriverResult::Success && tries == 0) {
            result = SendMcuCommand(SubCommand::SET_REPORT_MODE, mcu_request);
        }
        if (result == DriverResult::Success && tries == 0) {
            GetSubCommandResponse(SubCommand::SET_MCU_CONFIG, output);
        }

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ >= max_tries) {
            return DriverResult::WrongReply;
        }
    } while (!(output[15] == 0x13 && output[17] == 0x07) && output[15] != 0x23);

    return DriverResult::Success;
}

DriverResult IrsProtocol::WriteRegistersStep2() {
    LOG_DEBUG(Input, "Configure IRS");
    constexpr std::size_t max_tries = 28;
    std::vector<u8> output;
    std::size_t tries = 0;

    const IrsWriteRegisters irs_registers{
        .command = MCUCommand::ConfigureIR,
        .sub_command = MCUSubCommand::WriteDeviceRegisters,
        .number_of_registers = 0x8,
        .registers =
            {
                IrsRegister{0x1100, static_cast<u8>(led_intensity >> 8)},
                {0x1200, static_cast<u8>(led_intensity & 0xff)},
                {0x2d00, image_flip},
                {0x6701, static_cast<u8>((denoise >> 16) & 0xff)},
                {0x6801, static_cast<u8>((denoise >> 8) & 0xff)},
                {0x6901, static_cast<u8>(denoise & 0xff)},
                {0x0400, 0x2d},
                {0x0700, 0x01},
            },
        .crc = {},
    };

    std::vector<u8> request_data(sizeof(IrsWriteRegisters));
    memcpy(request_data.data(), &irs_registers, sizeof(IrsWriteRegisters));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);
    do {
        const auto result = SendSubCommand(SubCommand::SET_MCU_CONFIG, request_data, output);

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ >= max_tries) {
            return DriverResult::WrongReply;
        }
    } while (output[15] != 0x13 && output[15] != 0x23);

    return DriverResult::Success;
}

bool IrsProtocol::IsEnabled() const {
    return is_enabled;
}

} // namespace InputCommon::Joycon
