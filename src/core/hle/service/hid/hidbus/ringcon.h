// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>

#include "common/common_types.h"
#include "core/hle/service/hid/hidbus/hidbus_base.h"

namespace Core::HID {
class EmulatedController;
} // namespace Core::HID

namespace Service::HID {

class RingController final : public HidbusBase {
public:
    explicit RingController(Core::System& system_, KernelHelpers::ServiceContext& service_context_);
    ~RingController() override;

    void OnInit() override;

    void OnRelease() override;

    // Updates ringcon transfer memory
    void OnUpdate() override;

    // Returns the device ID of the joycon
    u8 GetDeviceId() const override;

    // Assigns a command from data
    bool SetCommand(std::span<const u8> data) override;

    // Returns a reply from a command
    std::vector<u8> GetReply() const override;

private:
    // These values are obtained from a real ring controller
    static constexpr s16 idle_value = 2280;
    static constexpr s16 idle_deadzone = 120;
    static constexpr s16 range = 2500;

    // Most missing command names are leftovers from other firmware versions
    enum class RingConCommands : u32 {
        GetFirmwareVersion = 0x00020000,
        ReadId = 0x00020100,
        JoyPolling = 0x00020101,
        Unknown1 = 0x00020104,
        c20105 = 0x00020105,
        Unknown2 = 0x00020204,
        Unknown3 = 0x00020304,
        Unknown4 = 0x00020404,
        ReadUnkCal = 0x00020504,
        ReadFactoryCal = 0x00020A04,
        Unknown5 = 0x00021104,
        Unknown6 = 0x00021204,
        Unknown7 = 0x00021304,
        ReadUserCal = 0x00021A04,
        ReadRepCount = 0x00023104,
        ReadTotalPushCount = 0x00023204,
        ResetRepCount = 0x04013104,
        Unknown8 = 0x04011104,
        Unknown9 = 0x04011204,
        Unknown10 = 0x04011304,
        SaveCalData = 0x10011A04,
        Error = 0xFFFFFFFF,
    };

    enum class DataValid : u32 {
        Valid,
        BadCRC,
        Cal,
    };

    struct FirmwareVersion {
        u8 sub;
        u8 main;
    };
    static_assert(sizeof(FirmwareVersion) == 0x2, "FirmwareVersion is an invalid size");

    struct FactoryCalibration {
        s32_le os_max;
        s32_le hk_max;
        s32_le zero_min;
        s32_le zero_max;
    };
    static_assert(sizeof(FactoryCalibration) == 0x10, "FactoryCalibration is an invalid size");

    struct CalibrationValue {
        s16 value;
        u16 crc;
    };
    static_assert(sizeof(CalibrationValue) == 0x4, "CalibrationValue is an invalid size");

    struct UserCalibration {
        CalibrationValue os_max;
        CalibrationValue hk_max;
        CalibrationValue zero;
    };
    static_assert(sizeof(UserCalibration) == 0xC, "UserCalibration is an invalid size");

    struct SaveCalData {
        RingConCommands command;
        UserCalibration calibration;
        INSERT_PADDING_BYTES_NOINIT(4);
    };
    static_assert(sizeof(SaveCalData) == 0x14, "SaveCalData is an invalid size");
    static_assert(std::is_trivially_copyable_v<SaveCalData>,
                  "SaveCalData must be trivially copyable");

    struct FirmwareVersionReply {
        DataValid status;
        FirmwareVersion firmware;
        INSERT_PADDING_BYTES(0x2);
    };
    static_assert(sizeof(FirmwareVersionReply) == 0x8, "FirmwareVersionReply is an invalid size");

    struct Cmd020105Reply {
        DataValid status;
        u8 data;
        INSERT_PADDING_BYTES(0x3);
    };
    static_assert(sizeof(Cmd020105Reply) == 0x8, "Cmd020105Reply is an invalid size");

    struct StatusReply {
        DataValid status;
    };
    static_assert(sizeof(StatusReply) == 0x4, "StatusReply is an invalid size");

    struct GetThreeByteReply {
        DataValid status;
        std::array<u8, 3> data;
        u8 crc;
    };
    static_assert(sizeof(GetThreeByteReply) == 0x8, "GetThreeByteReply is an invalid size");

    struct ReadUnkCalReply {
        DataValid status;
        u16 data;
        INSERT_PADDING_BYTES(0x2);
    };
    static_assert(sizeof(ReadUnkCalReply) == 0x8, "ReadUnkCalReply is an invalid size");

    struct ReadFactoryCalReply {
        DataValid status;
        FactoryCalibration calibration;
    };
    static_assert(sizeof(ReadFactoryCalReply) == 0x14, "ReadFactoryCalReply is an invalid size");

    struct ReadUserCalReply {
        DataValid status;
        UserCalibration calibration;
        INSERT_PADDING_BYTES(0x4);
    };
    static_assert(sizeof(ReadUserCalReply) == 0x14, "ReadUserCalReply is an invalid size");

    struct ReadIdReply {
        DataValid status;
        u16 id_l_x0;
        u16 id_l_x0_2;
        u16 id_l_x4;
        u16 id_h_x0;
        u16 id_h_x0_2;
        u16 id_h_x4;
    };
    static_assert(sizeof(ReadIdReply) == 0x10, "ReadIdReply is an invalid size");

    struct ErrorReply {
        DataValid status;
        INSERT_PADDING_BYTES(0x3);
    };
    static_assert(sizeof(ErrorReply) == 0x8, "ErrorReply is an invalid size");

    struct RingConData {
        DataValid status;
        s16_le data;
        INSERT_PADDING_BYTES(0x2);
    };
    static_assert(sizeof(RingConData) == 0x8, "RingConData is an invalid size");

    // Returns RingConData struct with pressure sensor values
    RingConData GetSensorValue() const;

    // Returns 8 byte reply with firmware version
    std::vector<u8> GetFirmwareVersionReply() const;

    // Returns 16 byte reply with ID values
    std::vector<u8> GetReadIdReply() const;

    // (STUBBED) Returns 8 byte reply
    std::vector<u8> GetC020105Reply() const;

    // (STUBBED) Returns 8 byte empty reply
    std::vector<u8> GetReadUnkCalReply() const;

    // Returns 20 byte reply with factory calibration values
    std::vector<u8> GetReadFactoryCalReply() const;

    // Returns 20 byte reply with user calibration values
    std::vector<u8> GetReadUserCalReply() const;

    // Returns 8 byte reply
    std::vector<u8> GetReadRepCountReply() const;

    // Returns 8 byte reply
    std::vector<u8> GetReadTotalPushCountReply() const;

    // Returns 8 byte reply
    std::vector<u8> GetResetRepCountReply() const;

    // Returns 4 byte save data reply
    std::vector<u8> GetSaveDataReply() const;

    // Returns 8 byte error reply
    std::vector<u8> GetErrorReply() const;

    // Returns 8 bit redundancy check from provided data
    u8 GetCrcValue(const std::vector<u8>& data) const;

    // Converts structs to an u8 vector equivalent
    template <typename T>
    std::vector<u8> GetDataVector(const T& reply) const;

    RingConCommands command{RingConCommands::Error};

    // These counters are used in multitasking mode while the switch is sleeping
    // Total steps taken
    u8 total_rep_count = 0;
    // Total times the ring was pushed
    u8 total_push_count = 0;

    const u8 device_id = 0x20;
    const FirmwareVersion version = {
        .sub = 0x0,
        .main = 0x2c,
    };
    const FactoryCalibration factory_calibration = {
        .os_max = idle_value + range + idle_deadzone,
        .hk_max = idle_value - range - idle_deadzone,
        .zero_min = idle_value - idle_deadzone,
        .zero_max = idle_value + idle_deadzone,
    };
    UserCalibration user_calibration = {
        .os_max = {.value = range, .crc = 228},
        .hk_max = {.value = -range, .crc = 239},
        .zero = {.value = idle_value, .crc = 225},
    };

    Core::HID::EmulatedController* input;
};
} // namespace Service::HID
