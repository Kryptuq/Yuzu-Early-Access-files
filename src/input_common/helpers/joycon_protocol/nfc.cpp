// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include "common/logging/log.h"
#include "input_common/helpers/joycon_protocol/nfc.h"

namespace InputCommon::Joycon {

NfcProtocol::NfcProtocol(std::shared_ptr<JoyconHandle> handle)
    : JoyconCommonProtocol(std::move(handle)) {}

DriverResult NfcProtocol::EnableNfc() {
    LOG_INFO(Input, "Enable NFC");
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
            .mode = MCUMode::NFC,
            .crc = {},
        };

        result = ConfigureMCU(config);
    }

    return result;
}

DriverResult NfcProtocol::DisableNfc() {
    LOG_DEBUG(Input, "Disable NFC");
    ScopedSetBlocking sb(this);
    DriverResult result{DriverResult::Success};

    if (result == DriverResult::Success) {
        result = EnableMCU(false);
    }

    is_enabled = false;

    return result;
}

DriverResult NfcProtocol::StartNFCPollingMode() {
    LOG_DEBUG(Input, "Start NFC pooling Mode");
    ScopedSetBlocking sb(this);
    DriverResult result{DriverResult::Success};
    TagFoundData tag_data{};

    if (result == DriverResult::Success) {
        result = WaitSetMCUMode(ReportMode::NFC_IR_MODE_60HZ, MCUMode::NFC);
    }
    if (result == DriverResult::Success) {
        result = WaitUntilNfcIsReady();
    }
    if (result == DriverResult::Success) {
        is_enabled = true;
    }

    return result;
}

DriverResult NfcProtocol::ScanAmiibo(std::vector<u8>& data) {
    LOG_DEBUG(Input, "Start NFC pooling Mode");
    ScopedSetBlocking sb(this);
    DriverResult result{DriverResult::Success};
    TagFoundData tag_data{};

    if (result == DriverResult::Success) {
        result = StartPolling(tag_data);
    }
    if (result == DriverResult::Success) {
        result = ReadTag(tag_data);
    }
    if (result == DriverResult::Success) {
        result = WaitUntilNfcIsReady();
    }
    if (result == DriverResult::Success) {
        result = StartPolling(tag_data);
    }
    if (result == DriverResult::Success) {
        result = GetAmiiboData(data);
    }

    return result;
}

bool NfcProtocol::HasAmiibo() {
    ScopedSetBlocking sb(this);
    DriverResult result{DriverResult::Success};
    TagFoundData tag_data{};

    if (result == DriverResult::Success) {
        result = StartPolling(tag_data);
    }

    return result == DriverResult::Success;
}

DriverResult NfcProtocol::WaitUntilNfcIsReady() {
    constexpr std::size_t timeout_limit = 10;
    std::vector<u8> output;
    std::size_t tries = 0;

    do {
        auto result = SendStartWaitingRecieveRequest(output);

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ > timeout_limit) {
            return DriverResult::Timeout;
        }
    } while (output[49] != 0x2a || (output[51] << 8) + output[50] != 0x0500 || output[55] != 0x31 ||
             output[56] != 0x00);

    return DriverResult::Success;
}

DriverResult NfcProtocol::StartPolling(TagFoundData& data) {
    LOG_DEBUG(Input, "Start Polling for tag");
    constexpr std::size_t timeout_limit = 7;
    std::vector<u8> output;
    std::size_t tries = 0;

    do {
        const auto result = SendStartPollingRequest(output);
        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ > timeout_limit) {
            return DriverResult::Timeout;
        }
    } while (output[49] != 0x2a || (output[51] << 8) + output[50] != 0x0500 || output[56] != 0x09);

    data.type = output[62];
    data.uuid.resize(output[64]);
    memcpy(data.uuid.data(), output.data() + 65, data.uuid.size());

    return DriverResult::Success;
}

DriverResult NfcProtocol::ReadTag(const TagFoundData& data) {
    constexpr std::size_t timeout_limit = 10;
    std::vector<u8> output;
    std::size_t tries = 0;

    std::string uuid_string;
    for (auto& content : data.uuid) {
        uuid_string += fmt::format(" {:02x}", content);
    }

    LOG_INFO(Input, "Tag detected, type={}, uuid={}", data.type, uuid_string);

    tries = 0;
    std::size_t ntag_pages = 0;
    // Read Tag data
loop1:
    while (true) {
        auto result = SendReadAmiiboRequest(output, ntag_pages);

        int attempt = 0;
        while (true) {
            if (attempt != 0) {
                result = GetMCUDataResponse(ReportMode::NFC_IR_MODE_60HZ, output);
            }
            if ((output[49] == 0x3a || output[49] == 0x2a) && output[56] == 0x07) {
                return DriverResult::ErrorReadingData;
            }
            if (output[49] == 0x3a && output[51] == 0x07 && output[52] == 0x01) {
                if (data.type != 2) {
                    goto loop1;
                }
                switch (output[74]) {
                case 0:
                    ntag_pages = 135;
                    break;
                case 3:
                    ntag_pages = 45;
                    break;
                case 4:
                    ntag_pages = 231;
                    break;
                default:
                    return DriverResult::ErrorReadingData;
                }
                goto loop1;
            }
            if (output[49] == 0x2a && output[56] == 0x04) {
                // finished
                SendStopPollingRequest(output);
                return DriverResult::Success;
            }
            if (output[49] == 0x2a) {
                goto loop1;
            }
            if (attempt++ > 6) {
                goto loop1;
            }
        }

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ > timeout_limit) {
            return DriverResult::Timeout;
        }
    }

    return DriverResult::Success;
}

DriverResult NfcProtocol::GetAmiiboData(std::vector<u8>& ntag_data) {
    constexpr std::size_t timeout_limit = 10;
    std::vector<u8> output;
    std::size_t tries = 0;

    std::size_t ntag_pages = 135;
    std::size_t ntag_buffer_pos = 0;
    // Read Tag data
loop1:
    while (true) {
        auto result = SendReadAmiiboRequest(output, ntag_pages);

        int attempt = 0;
        while (1) {
            if (attempt != 0) {
                result = GetMCUDataResponse(ReportMode::NFC_IR_MODE_60HZ, output);
            }
            if ((output[49] == 0x3a || output[49] == 0x2a) && output[56] == 0x07) {
                return DriverResult::ErrorReadingData;
            }
            if (output[49] == 0x3a && output[51] == 0x07) {
                std::size_t payload_size = (output[54] << 8 | output[55]) & 0x7FF;
                if (output[52] == 0x01) {
                    memcpy(ntag_data.data() + ntag_buffer_pos, output.data() + 116,
                           payload_size - 60);
                    ntag_buffer_pos += payload_size - 60;
                } else {
                    memcpy(ntag_data.data() + ntag_buffer_pos, output.data() + 56, payload_size);
                }
                goto loop1;
            }
            if (output[49] == 0x2a && output[56] == 0x04) {
                LOG_INFO(Input, "Finished reading amiibo");
                return DriverResult::Success;
            }
            if (output[49] == 0x2a) {
                goto loop1;
            }
            if (attempt++ > 4) {
                goto loop1;
            }
        }

        if (result != DriverResult::Success) {
            return result;
        }
        if (tries++ > timeout_limit) {
            return DriverResult::Timeout;
        }
    }

    return DriverResult::Success;
}

DriverResult NfcProtocol::SendStartPollingRequest(std::vector<u8>& output) {
    NFCRequestState request{
        .sub_command = MCUSubCommand::ReadDeviceMode,
        .command_argument = NFCReadCommand::StartPolling,
        .packet_id = 0x0,
        .packet_flag = MCUPacketFlag::LastCommandPacket,
        .data_length = sizeof(NFCPollingCommandData),
        .nfc_polling =
            {
                .enable_mifare = 0x01,
                .unknown_1 = 0x00,
                .unknown_2 = 0x00,
                .unknown_3 = 0x2c,
                .unknown_4 = 0x01,
            },
        .crc = {},
    };

    std::array<u8, sizeof(NFCRequestState)> request_data{};
    memcpy(request_data.data(), &request, sizeof(NFCRequestState));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);
    return SendMCUData(ReportMode::NFC_IR_MODE_60HZ, SubCommand::STATE, request_data, output);
}

DriverResult NfcProtocol::SendStopPollingRequest(std::vector<u8>& output) {
    NFCRequestState request{
        .sub_command = MCUSubCommand::ReadDeviceMode,
        .command_argument = NFCReadCommand::StopPolling,
        .packet_id = 0x0,
        .packet_flag = MCUPacketFlag::LastCommandPacket,
        .data_length = 0,
        .raw_data = {},
        .crc = {},
    };

    std::array<u8, sizeof(NFCRequestState)> request_data{};
    memcpy(request_data.data(), &request, sizeof(NFCRequestState));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);
    return SendMCUData(ReportMode::NFC_IR_MODE_60HZ, SubCommand::STATE, request_data, output);
}

DriverResult NfcProtocol::SendStartWaitingRecieveRequest(std::vector<u8>& output) {
    NFCRequestState request{
        .sub_command = MCUSubCommand::ReadDeviceMode,
        .command_argument = NFCReadCommand::StartWaitingRecieve,
        .packet_id = 0x0,
        .packet_flag = MCUPacketFlag::LastCommandPacket,
        .data_length = 0,
        .raw_data = {},
        .crc = {},
    };

    std::vector<u8> request_data(sizeof(NFCRequestState));
    memcpy(request_data.data(), &request, sizeof(NFCRequestState));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);
    return SendMCUData(ReportMode::NFC_IR_MODE_60HZ, SubCommand::STATE, request_data, output);
}

DriverResult NfcProtocol::SendReadAmiiboRequest(std::vector<u8>& output, std::size_t ntag_pages) {
    NFCRequestState request{
        .sub_command = MCUSubCommand::ReadDeviceMode,
        .command_argument = NFCReadCommand::Ntag,
        .packet_id = 0x0,
        .packet_flag = MCUPacketFlag::LastCommandPacket,
        .data_length = sizeof(NFCReadCommandData),
        .nfc_read =
            {
                .unknown = 0xd0,
                .uuid_length = 0x07,
                .unknown_2 = 0x00,
                .uid = {},
                .tag_type = NFCTagType::AllTags,
                .read_block = GetReadBlockCommand(ntag_pages),
            },
        .crc = {},
    };

    std::array<u8, sizeof(NFCRequestState)> request_data{};
    memcpy(request_data.data(), &request, sizeof(NFCRequestState));
    request_data[37] = CalculateMCU_CRC8(request_data.data() + 1, 36);
    return SendMCUData(ReportMode::NFC_IR_MODE_60HZ, SubCommand::STATE, request_data, output);
}

NFCReadBlockCommand NfcProtocol::GetReadBlockCommand(std::size_t pages) const {
    switch (static_cast<NFCBlock>(pages)) {
    case NFCBlock::Block0:
        return {
            .block_count = 1,
        };
    case NFCBlock::Block45:
        return {
            .block_count = 1,
            .blocks =
                {
                    NFCReadBlock{0x00, 0x2C},
                },
        };
    case NFCBlock::Block135:
        return {
            .block_count = 3,
            .blocks =
                {
                    NFCReadBlock{0x00, 0x3b},
                    {0x3c, 0x77},
                    {0x78, 0x86},
                },
        };
    case NFCBlock::Block231:
        return {
            .block_count = 4,
            .blocks =
                {
                    NFCReadBlock{0x00, 0x3b},
                    {0x3c, 0x77},
                    {0x78, 0x83},
                    {0xb4, 0xe6},
                },
        };
    default:
        return {};
    };
}

bool NfcProtocol::IsEnabled() const {
    return is_enabled;
}

} // namespace InputCommon::Joycon
