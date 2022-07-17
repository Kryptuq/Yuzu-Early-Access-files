// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hid/irs_types.h"

namespace Service::IRS {
class ProcessorBase {
public:
    explicit ProcessorBase();
    virtual ~ProcessorBase();

    virtual void StartProcessor() = 0;
    virtual void SuspendProcessor() = 0;
    virtual void StopProcessor() = 0;

    bool IsProcessorActive() const;

protected:
    /// Returns the number of bytes the image uses
    std::size_t GetDataSize(Core::IrSensor::ImageTransferProcessorFormat format) const;

    /// Returns the width of the image
    std::size_t GetDataWidth(Core::IrSensor::ImageTransferProcessorFormat format) const;

    /// Returns the height of the image
    std::size_t GetDataHeight(Core::IrSensor::ImageTransferProcessorFormat format) const;

    bool is_active{false};
};
} // namespace Service::IRS
