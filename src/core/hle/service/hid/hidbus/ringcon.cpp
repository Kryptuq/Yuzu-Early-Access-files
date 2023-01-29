// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/service/hid/hidbus/ringcon.h"

namespace Service::HID {

RingController::RingController(Core::HID::HIDCore& hid_core_,
                               KernelHelpers::ServiceContext& service_context_)
    : HidbusBase(service_context_) {
    input = hid_core_.GetEmulatedController(Core::HID::NpadIdType::Player1);
}

RingController::~RingController() = default;

void RingController::OnInit() {
    input->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                          Common::Input::PollingMode::Ring);
    return;
}

void RingController::OnRelease() {
    input->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                          Common::Input::PollingMode::Active);
    return;
};

void RingController::OnUpdate() {
    if (!is_activated) {
        return;
    }

    if (!device_enabled) {
        return;
    }

    if (!polling_mode_enabled || !is_transfer_memory_set) {
        return;
    }

    // TODO: Increment multitasking counters from motion and sensor data

    switch (polling_mode) {
    case JoyPollingMode::SixAxisSensorEnable: {
        enable_sixaxis_data.header.total_entries = 10;
        enable_sixaxis_data.header.result = ResultSuccess;
        const auto& last_entry =
            enable_sixaxis_data.entries[enable_sixaxis_data.header.latest_entry];

        enable_sixaxis_data.header.latest_entry =
            (enable_sixaxis_data.header.latest_entry + 1) % 10;
        auto& curr_entry = enable_sixaxis_data.entries[enable_sixaxis_data.header.latest_entry];

        curr_entry.sampling_number = last_entry.sampling_number + 1;
        curr_entry.polling_data.sampling_number = curr_entry.sampling_number;

        const RingConData ringcon_value = GetSensorValue();
        curr_entry.polling_data.out_size = sizeof(ringcon_value);
        std::memcpy(curr_entry.polling_data.data.data(), &ringcon_value, sizeof(ringcon_value));

        std::memcpy(transfer_memory, &enable_sixaxis_data, sizeof(enable_sixaxis_data));
        break;
    }
    default:
        LOG_ERROR(Service_HID, "Polling mode not supported {}", polling_mode);
        break;
    }
}

RingController::RingConData RingController::GetSensorValue() const {
    RingConData ringcon_sensor_value{
        .status = DataValid::Valid,
        .data = 0,
    };

    const f32 force_value = input->GetRingSensorForce().force * range;
    ringcon_sensor_value.data = static_cast<s16>(force_value) + idle_value;

    return ringcon_sensor_value;
}

u8 RingController::GetDeviceId() const {
    return device_id;
}

std::vector<u8> RingController::GetReply() const {
    const RingConCommands current_command = command;

    switch (current_command) {
    case RingConCommands::GetFirmwareVersion:
        return GetFirmwareVersionReply();
    case RingConCommands::ReadId:
        return GetReadIdReply();
    case RingConCommands::c20105:
        return GetC020105Reply();
    case RingConCommands::ReadUnkCal:
        return GetReadUnkCalReply();
    case RingConCommands::ReadFactoryCal:
        return GetReadFactoryCalReply();
    case RingConCommands::ReadUserCal:
        return GetReadUserCalReply();
    case RingConCommands::ReadRepCount:
        return GetReadRepCountReply();
    case RingConCommands::ReadTotalPushCount:
        return GetReadTotalPushCountReply();
    case RingConCommands::ResetRepCount:
        return GetResetRepCountReply();
    case RingConCommands::SaveCalData:
        return GetSaveDataReply();
    default:
        return GetErrorReply();
    }
}

bool RingController::SetCommand(const std::vector<u8>& data) {
    if (data.size() < 4) {
        LOG_ERROR(Service_HID, "Command size not supported {}", data.size());
        command = RingConCommands::Error;
        return false;
    }

    std::memcpy(&command, data.data(), sizeof(RingConCommands));

    switch (command) {
    case RingConCommands::GetFirmwareVersion:
    case RingConCommands::ReadId:
    case RingConCommands::c20105:
    case RingConCommands::ReadUnkCal:
    case RingConCommands::ReadFactoryCal:
    case RingConCommands::ReadUserCal:
    case RingConCommands::ReadRepCount:
    case RingConCommands::ReadTotalPushCount:
        ASSERT_MSG(data.size() == 0x4, "data.size is not 0x4 bytes");
        send_command_async_event->Signal();
        return true;
    case RingConCommands::ResetRepCount:
        ASSERT_MSG(data.size() == 0x4, "data.size is not 0x4 bytes");
        total_rep_count = 0;
        send_command_async_event->Signal();
        return true;
    case RingConCommands::SaveCalData: {
        ASSERT_MSG(data.size() == 0x14, "data.size is not 0x14 bytes");

        SaveCalData save_info{};
        std::memcpy(&save_info, data.data(), sizeof(SaveCalData));
        user_calibration = save_info.calibration;
        send_command_async_event->Signal();
        return true;
    }
    default:
        LOG_ERROR(Service_HID, "Command not implemented {}", command);
        command = RingConCommands::Error;
        // Signal a reply to avoid softlocking the game
        send_command_async_event->Signal();
        return false;
    }
}

std::vector<u8> RingController::GetFirmwareVersionReply() const {
    const FirmwareVersionReply reply{
        .status = DataValid::Valid,
        .firmware = version,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetReadIdReply() const {
    // The values are hardcoded from a real joycon
    const ReadIdReply reply{
        .status = DataValid::Valid,
        .id_l_x0 = 8,
        .id_l_x0_2 = 41,
        .id_l_x4 = 22294,
        .id_h_x0 = 19777,
        .id_h_x0_2 = 13621,
        .id_h_x4 = 8245,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetC020105Reply() const {
    const Cmd020105Reply reply{
        .status = DataValid::Valid,
        .data = 1,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetReadUnkCalReply() const {
    const ReadUnkCalReply reply{
        .status = DataValid::Valid,
        .data = 0,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetReadFactoryCalReply() const {
    const ReadFactoryCalReply reply{
        .status = DataValid::Valid,
        .calibration = factory_calibration,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetReadUserCalReply() const {
    const ReadUserCalReply reply{
        .status = DataValid::Valid,
        .calibration = user_calibration,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetReadRepCountReply() const {
    const GetThreeByteReply reply{
        .status = DataValid::Valid,
        .data = {total_rep_count, 0, 0},
        .crc = GetCrcValue({total_rep_count, 0, 0, 0}),
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetReadTotalPushCountReply() const {
    const GetThreeByteReply reply{
        .status = DataValid::Valid,
        .data = {total_push_count, 0, 0},
        .crc = GetCrcValue({total_push_count, 0, 0, 0}),
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetResetRepCountReply() const {
    return GetReadRepCountReply();
}

std::vector<u8> RingController::GetSaveDataReply() const {
    const StatusReply reply{
        .status = DataValid::Valid,
    };

    return GetDataVector(reply);
}

std::vector<u8> RingController::GetErrorReply() const {
    const ErrorReply reply{
        .status = DataValid::BadCRC,
    };

    return GetDataVector(reply);
}

u8 RingController::GetCrcValue(const std::vector<u8>& data) const {
    u8 crc = 0;
    for (std::size_t index = 0; index < data.size(); index++) {
        for (u8 i = 0x80; i > 0; i >>= 1) {
            bool bit = (crc & 0x80) != 0;
            if ((data[index] & i) != 0) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x8d;
            }
        }
    }
    return crc;
}

template <typename T>
std::vector<u8> RingController::GetDataVector(const T& reply) const {
    static_assert(std::is_trivially_copyable_v<T>);
    std::vector<u8> data;
    data.resize(sizeof(reply));
    std::memcpy(data.data(), &reply, sizeof(reply));
    return data;
}

} // namespace Service::HID
