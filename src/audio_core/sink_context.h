// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>
#include "audio_core/common.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace AudioCore {

using DownmixCoefficients = std::array<float_le, 4>;

enum class SinkTypes : u8 {
    Invalid = 0,
    Device = 1,
    Circular = 2,
};

enum class SinkSampleFormat : u32_le {
    None = 0,
    Pcm8 = 1,
    Pcm16 = 2,
    Pcm24 = 3,
    Pcm32 = 4,
    PcmFloat = 5,
    Adpcm = 6,
};

class SinkInfo {
public:
    struct CircularBufferIn {
        u64_le address;
        u32_le size;
        u32_le input_count;
        u32_le sample_count;
        u32_le previous_position;
        SinkSampleFormat sample_format;
        std::array<u8, AudioCommon::MAX_CHANNEL_COUNT> input;
        bool in_use;
        INSERT_PADDING_BYTES_NOINIT(5);
    };
    static_assert(sizeof(CircularBufferIn) == 0x28,
                  "SinkInfo::CircularBufferIn is in invalid size");

    struct DeviceIn {
        std::array<u8, 255> device_name;
        INSERT_PADDING_BYTES_NOINIT(1);
        s32_le input_count;
        std::array<u8, AudioCommon::MAX_CHANNEL_COUNT> input;
        INSERT_PADDING_BYTES_NOINIT(1);
        bool down_matrix_enabled;
        DownmixCoefficients down_matrix_coef;
    };
    static_assert(sizeof(DeviceIn) == 0x11c, "SinkInfo::DeviceIn is an invalid size");

    struct InParams {
        SinkTypes type{};
        bool in_use{};
        INSERT_PADDING_BYTES(2);
        u32_le node_id{};
        INSERT_PADDING_WORDS(6);
        union {
            // std::array<u8, 0x120> raw{};
            DeviceIn device;
            CircularBufferIn circular_buffer;
        };
    };
    static_assert(sizeof(InParams) == 0x140, "SinkInfo::InParams are an invalid size!");
};

class SinkContext {
public:
    explicit SinkContext(std::size_t sink_count_);
    ~SinkContext();

    [[nodiscard]] std::size_t GetCount() const;

    void UpdateMainSink(const SinkInfo::InParams& in);
    [[nodiscard]] bool InUse() const;
    [[nodiscard]] std::vector<u8> OutputBuffers() const;

    [[nodiscard]] const DownmixCoefficients& GetDownmixCoefficients() const;

private:
    bool in_use{false};
    s32 use_count{};
    std::array<u8, AudioCommon::MAX_CHANNEL_COUNT> buffers{};
    std::size_t sink_count{};
    DownmixCoefficients downmix_coefficients{};
};
} // namespace AudioCore
