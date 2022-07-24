// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Service::IRS {
class PointingProcessor final : public ProcessorBase {
public:
    explicit PointingProcessor(Core::IrSensor::DeviceFormat& device_format);
    ~PointingProcessor() override;

    // Called when the processor is initialized
    void StartProcessor() override;

    // Called when the processor is suspended
    void SuspendProcessor() override;

    // Called when the processor is stopped
    void StopProcessor() override;

    // Sets config parameters of the camera
    void SetConfig(Core::IrSensor::PackedPointingProcessorConfig config);

private:
    // This is nn::irsensor::PointingProcessorConfig
    struct PointingProcessorConfig {
        Core::IrSensor::IrsRect window_of_interest;
    };
    static_assert(sizeof(PointingProcessorConfig) == 0x8,
                  "PointingProcessorConfig is an invalid size");

    struct PointingProcessorMarkerData {
        u8 pointing_status;
        INSERT_PADDING_BYTES(3);
        u32 unknown;
        float unkown_float1;
        float position_x;
        float position_y;
        float unkown_float2;
        Core::IrSensor::IrsRect window_of_interest;
    };
    static_assert(sizeof(PointingProcessorMarkerData) == 0x20,
                  "PointingProcessorMarkerData is an invalid size");

    struct PointingProcessorMarkerState {
        s64 sampling_number;
        u64 timestamp;
        std::array<PointingProcessorMarkerData, 0x3> data;
    };
    static_assert(sizeof(PointingProcessorMarkerState) == 0x70,
                  "PointingProcessorMarkerState is an invalid size");

    PointingProcessorConfig current_config{};
    Core::IrSensor::DeviceFormat& device;
};

} // namespace Service::IRS
