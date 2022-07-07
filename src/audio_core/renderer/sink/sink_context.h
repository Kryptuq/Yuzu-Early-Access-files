// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/sink/sink_info_base.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Manages output sinks.
 */
class SinkContext {
public:
    /**
     * Initialize the sink context.
     *
     * @param sink_infos - Workbuffer for the sinks.
     * @param sink_count - Number of sinks in the buffer.
     */
    void Initialize(std::span<SinkInfoBase> sink_infos, u32 sink_count);

    /**
     * Get a given index's info.
     *
     * @param index - Sink index to get.
     * @return The sink info base for the given index.
     */
    SinkInfoBase* GetInfo(u32 index);

    /**
     * Get the current number of sinks.
     *
     * @return The number of sinks.
     */
    u32 GetCount() const;

private:
    /// Buffer of sink infos
    std::span<SinkInfoBase> sink_infos{};
    /// Number of sinks in the buffer
    u32 sink_count{};
};

} // namespace AudioCore::AudioRenderer
