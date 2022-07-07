// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/sink/sink_info_base.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Info for a circular buffer sink.
 */
class CircularBufferSinkInfo : public SinkInfoBase {
public:
    CircularBufferSinkInfo();

    /**
     * Clean up for info, resetting it to a default state.
     */
    void CleanUp() override;

    /**
     * Update the info according to parameters, and write the current state to out_status.
     *
     * @param error_info  - Output error code.
     * @param out_status  - Output status.
     * @param in_params   - Input parameters.
     * @param pool_mapper - Used to map the circular buffer.
     */
    void Update(BehaviorInfo::ErrorInfo& error_info, OutStatus& out_status,
                const InParameter& in_params, const PoolMapper& pool_mapper) override;

    /**
     * Update the circular buffer on command generation, incrementing its current offsets.
     */
    void UpdateForCommandGeneration() override;
};
static_assert(sizeof(CircularBufferSinkInfo) <= sizeof(SinkInfoBase),
              "CircularBufferSinkInfo is too large!");

} // namespace AudioCore::AudioRenderer
