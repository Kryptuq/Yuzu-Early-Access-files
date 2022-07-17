// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/hid/irsensor/processor_base.h"

namespace Service::IRS {

ProcessorBase::ProcessorBase() {}
ProcessorBase::~ProcessorBase() = default;

bool ProcessorBase::IsProcessorActive() const {
    return is_active;
}

std::size_t ProcessorBase::GetDataSize(Core::IrSensor::ImageTransferProcessorFormat format) const {
    switch (format) {
    case Core::IrSensor::ImageTransferProcessorFormat::Size320x240:
        return 320 * 240;
    case Core::IrSensor::ImageTransferProcessorFormat::Size160x120:
        return 160 * 120;
    case Core::IrSensor::ImageTransferProcessorFormat::Size80x60:
        return 80 * 60;
    case Core::IrSensor::ImageTransferProcessorFormat::Size40x30:
        return 40 * 30;
    case Core::IrSensor::ImageTransferProcessorFormat::Size20x15:
        return 20 * 15;
    default:
        return 0;
    }
}

std::size_t ProcessorBase::GetDataWidth(Core::IrSensor::ImageTransferProcessorFormat format) const {
    switch (format) {
    case Core::IrSensor::ImageTransferProcessorFormat::Size320x240:
        return 320;
    case Core::IrSensor::ImageTransferProcessorFormat::Size160x120:
        return 160;
    case Core::IrSensor::ImageTransferProcessorFormat::Size80x60:
        return 80;
    case Core::IrSensor::ImageTransferProcessorFormat::Size40x30:
        return 40;
    case Core::IrSensor::ImageTransferProcessorFormat::Size20x15:
        return 20;
    default:
        return 0;
    }
}

std::size_t ProcessorBase::GetDataHeight(
    Core::IrSensor::ImageTransferProcessorFormat format) const {
    switch (format) {
    case Core::IrSensor::ImageTransferProcessorFormat::Size320x240:
        return 240;
    case Core::IrSensor::ImageTransferProcessorFormat::Size160x120:
        return 120;
    case Core::IrSensor::ImageTransferProcessorFormat::Size80x60:
        return 60;
    case Core::IrSensor::ImageTransferProcessorFormat::Size40x30:
        return 30;
    case Core::IrSensor::ImageTransferProcessorFormat::Size20x15:
        return 15;
    default:
        return 0;
    }
}

} // namespace Service::IRS
