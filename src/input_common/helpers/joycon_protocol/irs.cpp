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
    ScopedSetBlocking sb(this);
    DriverResult result{DriverResult::Success};

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

    return result;
}

DriverResult IrsProtocol::DisableIrs() {
    LOG_DEBUG(Input, "Disable IRS");
    ScopedSetBlocking sb(this);
    DriverResult result{DriverResult::Success};

    if (result == DriverResult::Success) {
        result = EnableMCU(false);
    }

    is_enabled = false;

    return result;
}

DriverResult IrsProtocol::SetIrsConfig(IrsMode mode, IrsResolution format) {
    irs_mode = mode;
    switch (format) {
    case IrsResolution::Size320x240:
        resolution_code = IrsResolutionCode::Size320x240;
        fragments = IrsFragments::Size320x240;
        resolution = IrsResolution::Size320x240;
        break;
    case IrsResolution::Size160x120:
        resolution_code = IrsResolutionCode::Size160x120;
        fragments = IrsFragments::Size160x120;
        resolution = IrsResolution::Size160x120;
        break;
    case IrsResolution::Size80x60:
        resolution_code = IrsResolutionCode::Size80x60;
        fragments = IrsFragments::Size80x60;
        resolution = IrsResolution::Size80x60;
        break;
    case IrsResolution::Size20x15:
        resolution_code = IrsResolutionCode::Size20x15;
        fragments = IrsFragments::Size20x15;
        resolution = IrsResolution::Size20x15;
        break;
    case IrsResolution::Size40x30:
    default:
        resolution_code = IrsResolutionCode::Size40x30;
        fragments = IrsFragments::Size40x30;
        resolution = IrsResolution::Size40x30;
        break;
    }

    // Restart feature
    if (is_enabled) {
        DisableIrs();
        return EnableIrs();
    }

    return DriverResult::Success;
}

DriverResult IrsProtocol::RequestImage(std::span<u8> buffer) {
    const u8 next_packet_fragment =
        static_cast<u8>((packet_fragment + 1) % (static_cast<u8>(fragments) + 1));

    if (buffer[0] == 0x31 && buffer[49] == 0x03) {
        u8 new_packet_fragment = buffer[52];
        if (new_packet_fragment == next_packet_fragment) {
            packet_fragment = next_packet_fragment;
            memcpy(buf_image.data() + (300 * packet_fragment), buffer.data() + 59, 300);

            return RequestFrame(packet_fragment);
        }

        if (new_packet_fragment == packet_fragment) {
            return RequestFrame(packet_fragment);
        }

        return ResendFrame(next_packet_fragment);
    }

    return RequestFrame(packet_fragment);
}

DriverResult IrsProtocol::ConfigureIrs() {
    LOG_DEBUG(Input, "Configure IRS");
    constexpr std::size_t max_tries = 28;
    SubCommandResponse output{};
    std::size_t tries = 0;

    const IrsConfigure irs_configuration{
        .command = MCUCommand::ConfigureIR,
        .sub_command = MCUSubCommand::SetDeviceMode,
        .irs_mode = IrsMode::ImageTransfer,
        .number_of_fragments = fragments,
        .mcu_major_version = 0x0500,
        .mcu_minor_version = 0x1800,
        .crc = {},
    };
    buf_image.resize((static_cast<u8>(fragments) + 1) * 300);

    std::array<u8, sizeof(IrsConfigure)> request_data{};
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
    } while (output.command_data[0] != 0x0b);

    return DriverResult::Success;
}

DriverResult IrsProtocol::WriteRegistersStep1() {
    LOG_DEBUG(Input, "WriteRegistersStep1");
    DriverResult result{DriverResult::Success};
    constexpr std::size_t max_tries = 28;
    SubCommandResponse output{};
    std::size_t tries = 0;

    const IrsWriteRegisters irs_registers{
        .command = MCUCommand::ConfigureIR,
        .sub_command = MCUSubCommand::WriteDeviceRegisters,
        .number_of_registers = 0x9,
        .registers =
            {
                IrsRegister{IrRegistersAddress::Resolution, static_cast<u8>(resolution_code)},
                {IrRegistersAddress::ExposureLSB, static_cast<u8>(exposure & 0xff)},
                {IrRegistersAddress::ExposureMSB, static_cast<u8>(exposure >> 8)},
                {IrRegistersAddress::ExposureTime, 0x00},
                {IrRegistersAddress::Leds, static_cast<u8>(leds)},
                {IrRegistersAddress::DigitalGainLSB, static_cast<u8>((digital_gain & 0x0f) << 4)},
                {IrRegistersAddress::DigitalGainMSB, static_cast<u8>((digital_gain & 0xf0) >> 4)},
                {IrRegistersAddress::LedFilter, static_cast<u8>(led_filter)},
                {IrRegistersAddress::WhitePixelThreshold, 0xc8},
            },
        .crc = {},
    };

    std::array<u8, sizeof(IrsWriteRegisters)> request_data{};
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
            result = SendMCUCommand(SubCommand::SET_REPORT_MODE, mcu_request);
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
    } while (!(output.command_data[0] == 0x13 && output.command_data[2] == 0x07) &&
             output.command_data[0] != 0x23);

    return DriverResult::Success;
}

DriverResult IrsProtocol::WriteRegistersStep2() {
    LOG_DEBUG(Input, "WriteRegistersStep2");
    constexpr std::size_t max_tries = 28;
    SubCommandResponse output{};
    std::size_t tries = 0;

    const IrsWriteRegisters irs_registers{
        .command = MCUCommand::ConfigureIR,
        .sub_command = MCUSubCommand::WriteDeviceRegisters,
        .number_of_registers = 0x8,
        .registers =
            {
                IrsRegister{IrRegistersAddress::LedIntensitiyMSB,
                            static_cast<u8>(led_intensity >> 8)},
                {IrRegistersAddress::LedIntensitiyLSB, static_cast<u8>(led_intensity & 0xff)},
                {IrRegistersAddress::ImageFlip, static_cast<u8>(image_flip)},
                {IrRegistersAddress::DenoiseSmoothing, static_cast<u8>((denoise >> 16) & 0xff)},
                {IrRegistersAddress::DenoiseEdge, static_cast<u8>((denoise >> 8) & 0xff)},
                {IrRegistersAddress::DenoiseColor, static_cast<u8>(denoise & 0xff)},
                {IrRegistersAddress::UpdateTime, 0x2d},
                {IrRegistersAddress::FinalizeConfig, 0x01},
            },
        .crc = {},
    };

    std::array<u8, sizeof(IrsWriteRegisters)> request_data{};
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
    } while (output.command_data[0] != 0x13 && output.command_data[0] != 0x23);

    return DriverResult::Success;
}

DriverResult IrsProtocol::RequestFrame(u8 frame) {
    std::array<u8, 38> mcu_request{};
    mcu_request[3] = frame;
    mcu_request[36] = CalculateMCU_CRC8(mcu_request.data(), 36);
    mcu_request[37] = 0xFF;
    return SendMCUCommand(SubCommand::SET_REPORT_MODE, mcu_request);
}

DriverResult IrsProtocol::ResendFrame(u8 frame) {
    std::array<u8, 38> mcu_request{};
    mcu_request[1] = 0x1;
    mcu_request[2] = frame;
    mcu_request[3] = 0x0;
    mcu_request[36] = CalculateMCU_CRC8(mcu_request.data(), 36);
    mcu_request[37] = 0xFF;
    return SendMCUCommand(SubCommand::SET_REPORT_MODE, mcu_request);
}

std::vector<u8> IrsProtocol::GetImage() const {
    return buf_image;
}

IrsResolution IrsProtocol::GetIrsFormat() const {
    return resolution;
}

bool IrsProtocol::IsEnabled() const {
    return is_enabled;
}

} // namespace InputCommon::Joycon
