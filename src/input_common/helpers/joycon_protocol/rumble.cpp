// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>

#include "common/logging/log.h"
#include "input_common/helpers/joycon_protocol/rumble.h"

namespace InputCommon::Joycon {

RumbleProtocol::RumbleProtocol(std::shared_ptr<JoyconHandle> handle)
    : JoyconCommonProtocol(std::move(handle)) {}

DriverResult RumbleProtocol::EnableRumble(bool is_enabled) {
    LOG_DEBUG(Input, "Enable Rumble");
    ScopedSetBlocking sb(this);
    const std::array<u8, 1> buffer{static_cast<u8>(is_enabled ? 1 : 0)};
    return SendSubCommand(SubCommand::ENABLE_VIBRATION, buffer);
}

DriverResult RumbleProtocol::SendVibration(const VibrationValue& vibration) {
    std::array<u8, sizeof(DefaultVibrationBuffer)> buffer{};

    if (vibration.high_amplitude <= 0.0f && vibration.low_amplitude <= 0.0f) {
        return SendVibrationReport(DefaultVibrationBuffer);
    }

    // Protect joycons from damage from strong vibrations
    const f32 clamp_amplitude =
        1.0f / std::max(1.0f, vibration.high_amplitude + vibration.low_amplitude);

    const u16 encoded_high_frequency = EncodeHighFrequency(vibration.high_frequency);
    const u8 encoded_high_amplitude =
        EncodeHighAmplitude(vibration.high_amplitude * clamp_amplitude);
    const u8 encoded_low_frequency = EncodeLowFrequency(vibration.low_frequency);
    const u16 encoded_low_amplitude = EncodeLowAmplitude(vibration.low_amplitude * clamp_amplitude);

    buffer[0] = static_cast<u8>(encoded_high_frequency & 0xFF);
    buffer[1] = static_cast<u8>(encoded_high_amplitude | ((encoded_high_frequency >> 8) & 0x01));
    buffer[2] = static_cast<u8>(encoded_low_frequency | ((encoded_low_amplitude >> 8) & 0x80));
    buffer[3] = static_cast<u8>(encoded_low_amplitude & 0xFF);

    // Duplicate rumble for now
    buffer[4] = buffer[0];
    buffer[5] = buffer[1];
    buffer[6] = buffer[2];
    buffer[7] = buffer[3];

    return SendVibrationReport(buffer);
}

u16 RumbleProtocol::EncodeHighFrequency(f32 frequency) const {
    const u8 new_frequency =
        static_cast<u8>(std::clamp(std::log2(frequency / 10.0f) * 32.0f, 0.0f, 255.0f));
    return static_cast<u16>((new_frequency - 0x60) * 4);
}

u8 RumbleProtocol::EncodeLowFrequency(f32 frequency) const {
    const u8 new_frequency =
        static_cast<u8>(std::clamp(std::log2(frequency / 10.0f) * 32.0f, 0.0f, 255.0f));
    return static_cast<u8>(new_frequency - 0x40);
}

u8 RumbleProtocol::EncodeHighAmplitude(f32 amplitude) const {
    // More information about these values can be found here:
    // https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md

    static constexpr std::array<std::pair<f32, int>, 101> high_fequency_amplitude{
        std::pair<f32, int>{0.0f, 0x0},
        {0.01f, 0x2},
        {0.012f, 0x4},
        {0.014f, 0x6},
        {0.017f, 0x8},
        {0.02f, 0x0a},
        {0.024f, 0x0c},
        {0.028f, 0x0e},
        {0.033f, 0x10},
        {0.04f, 0x12},
        {0.047f, 0x14},
        {0.056f, 0x16},
        {0.067f, 0x18},
        {0.08f, 0x1a},
        {0.095f, 0x1c},
        {0.112f, 0x1e},
        {0.117f, 0x20},
        {0.123f, 0x22},
        {0.128f, 0x24},
        {0.134f, 0x26},
        {0.14f, 0x28},
        {0.146f, 0x2a},
        {0.152f, 0x2c},
        {0.159f, 0x2e},
        {0.166f, 0x30},
        {0.173f, 0x32},
        {0.181f, 0x34},
        {0.189f, 0x36},
        {0.198f, 0x38},
        {0.206f, 0x3a},
        {0.215f, 0x3c},
        {0.225f, 0x3e},
        {0.23f, 0x40},
        {0.235f, 0x42},
        {0.24f, 0x44},
        {0.245f, 0x46},
        {0.251f, 0x48},
        {0.256f, 0x4a},
        {0.262f, 0x4c},
        {0.268f, 0x4e},
        {0.273f, 0x50},
        {0.279f, 0x52},
        {0.286f, 0x54},
        {0.292f, 0x56},
        {0.298f, 0x58},
        {0.305f, 0x5a},
        {0.311f, 0x5c},
        {0.318f, 0x5e},
        {0.325f, 0x60},
        {0.332f, 0x62},
        {0.34f, 0x64},
        {0.347f, 0x66},
        {0.355f, 0x68},
        {0.362f, 0x6a},
        {0.37f, 0x6c},
        {0.378f, 0x6e},
        {0.387f, 0x70},
        {0.395f, 0x72},
        {0.404f, 0x74},
        {0.413f, 0x76},
        {0.422f, 0x78},
        {0.431f, 0x7a},
        {0.44f, 0x7c},
        {0.45f, 0x7e},
        {0.46f, 0x80},
        {0.47f, 0x82},
        {0.48f, 0x84},
        {0.491f, 0x86},
        {0.501f, 0x88},
        {0.512f, 0x8a},
        {0.524f, 0x8c},
        {0.535f, 0x8e},
        {0.547f, 0x90},
        {0.559f, 0x92},
        {0.571f, 0x94},
        {0.584f, 0x96},
        {0.596f, 0x98},
        {0.609f, 0x9a},
        {0.623f, 0x9c},
        {0.636f, 0x9e},
        {0.65f, 0xa0},
        {0.665f, 0xa2},
        {0.679f, 0xa4},
        {0.694f, 0xa6},
        {0.709f, 0xa8},
        {0.725f, 0xaa},
        {0.741f, 0xac},
        {0.757f, 0xae},
        {0.773f, 0xb0},
        {0.79f, 0xb2},
        {0.808f, 0xb4},
        {0.825f, 0xb6},
        {0.843f, 0xb8},
        {0.862f, 0xba},
        {0.881f, 0xbc},
        {0.9f, 0xbe},
        {0.92f, 0xc0},
        {0.94f, 0xc2},
        {0.96f, 0xc4},
        {0.981f, 0xc6},
        {1.003f, 0xc8},
    };

    for (const auto& [amplitude_value, code] : high_fequency_amplitude) {
        if (amplitude <= amplitude_value) {
            return static_cast<u8>(code);
        }
    }

    return static_cast<u8>(high_fequency_amplitude[high_fequency_amplitude.size() - 1].second);
}

u16 RumbleProtocol::EncodeLowAmplitude(f32 amplitude) const {
    // More information about these values can be found here:
    // https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/rumble_data_table.md

    static constexpr std::array<std::pair<f32, int>, 101> high_fequency_amplitude{
        std::pair<f32, int>{0.0f, 0x0040},
        {0.01f, 0x8040},
        {0.012f, 0x0041},
        {0.014f, 0x8041},
        {0.017f, 0x0042},
        {0.02f, 0x8042},
        {0.024f, 0x0043},
        {0.028f, 0x8043},
        {0.033f, 0x0044},
        {0.04f, 0x8044},
        {0.047f, 0x0045},
        {0.056f, 0x8045},
        {0.067f, 0x0046},
        {0.08f, 0x8046},
        {0.095f, 0x0047},
        {0.112f, 0x8047},
        {0.117f, 0x0048},
        {0.123f, 0x8048},
        {0.128f, 0x0049},
        {0.134f, 0x8049},
        {0.14f, 0x004a},
        {0.146f, 0x804a},
        {0.152f, 0x004b},
        {0.159f, 0x804b},
        {0.166f, 0x004c},
        {0.173f, 0x804c},
        {0.181f, 0x004d},
        {0.189f, 0x804d},
        {0.198f, 0x004e},
        {0.206f, 0x804e},
        {0.215f, 0x004f},
        {0.225f, 0x804f},
        {0.23f, 0x0050},
        {0.235f, 0x8050},
        {0.24f, 0x0051},
        {0.245f, 0x8051},
        {0.251f, 0x0052},
        {0.256f, 0x8052},
        {0.262f, 0x0053},
        {0.268f, 0x8053},
        {0.273f, 0x0054},
        {0.279f, 0x8054},
        {0.286f, 0x0055},
        {0.292f, 0x8055},
        {0.298f, 0x0056},
        {0.305f, 0x8056},
        {0.311f, 0x0057},
        {0.318f, 0x8057},
        {0.325f, 0x0058},
        {0.332f, 0x8058},
        {0.34f, 0x0059},
        {0.347f, 0x8059},
        {0.355f, 0x005a},
        {0.362f, 0x805a},
        {0.37f, 0x005b},
        {0.378f, 0x805b},
        {0.387f, 0x005c},
        {0.395f, 0x805c},
        {0.404f, 0x005d},
        {0.413f, 0x805d},
        {0.422f, 0x005e},
        {0.431f, 0x805e},
        {0.44f, 0x005f},
        {0.45f, 0x805f},
        {0.46f, 0x0060},
        {0.47f, 0x8060},
        {0.48f, 0x0061},
        {0.491f, 0x8061},
        {0.501f, 0x0062},
        {0.512f, 0x8062},
        {0.524f, 0x0063},
        {0.535f, 0x8063},
        {0.547f, 0x0064},
        {0.559f, 0x8064},
        {0.571f, 0x0065},
        {0.584f, 0x8065},
        {0.596f, 0x0066},
        {0.609f, 0x8066},
        {0.623f, 0x0067},
        {0.636f, 0x8067},
        {0.65f, 0x0068},
        {0.665f, 0x8068},
        {0.679f, 0x0069},
        {0.694f, 0x8069},
        {0.709f, 0x006a},
        {0.725f, 0x806a},
        {0.741f, 0x006b},
        {0.757f, 0x806b},
        {0.773f, 0x006c},
        {0.79f, 0x806c},
        {0.808f, 0x006d},
        {0.825f, 0x806d},
        {0.843f, 0x006e},
        {0.862f, 0x806e},
        {0.881f, 0x006f},
        {0.9f, 0x806f},
        {0.92f, 0x0070},
        {0.94f, 0x8070},
        {0.96f, 0x0071},
        {0.981f, 0x8071},
        {1.003f, 0x0072},
    };

    for (const auto& [amplitude_value, code] : high_fequency_amplitude) {
        if (amplitude <= amplitude_value) {
            return static_cast<u16>(code);
        }
    }

    return static_cast<u16>(high_fequency_amplitude[high_fequency_amplitude.size() - 1].second);
}

} // namespace InputCommon::Joycon
