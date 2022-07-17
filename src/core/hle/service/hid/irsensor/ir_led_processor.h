// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hid/irs_types.h"
#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Service::IRS {
class IrLedProcessor final : public ProcessorBase {
public:
    explicit IrLedProcessor(Core::IrSensor::DeviceFormat& device_format);
    ~IrLedProcessor() override;

    // Called when the processor is initialized
    void StartProcessor() override;

    // Called when the processor is suspended
    void SuspendProcessor() override;

    // Called when the processor is stopped
    void StopProcessor() override;

    // Sets config parameters of the camera
    void SetConfig(Core::IrSensor::PackedIrLedProcessorConfig config);

private:
    // This is nn::irsensor::IrLedProcessorConfig
    struct IrLedProcessorConfig {
        Core::IrSensor::CameraLightTarget light_target;
    };
    static_assert(sizeof(IrLedProcessorConfig) == 0x4, "IrLedProcessorConfig is an invalid size");

    struct IrLedProcessorState {
        s64 sampling_number;
        u64 timestamp;
        std::array<u8, 0x8> data;
    };
    static_assert(sizeof(IrLedProcessorState) == 0x18, "IrLedProcessorState is an invalid size");

    IrLedProcessorConfig current_config{};
    Core::IrSensor::DeviceFormat& device;
};

} // namespace Service::IRS
