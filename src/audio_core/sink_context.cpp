// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/sink_context.h"

namespace AudioCore {
SinkContext::SinkContext(std::size_t sink_count_) : sink_count{sink_count_} {}
SinkContext::~SinkContext() = default;

std::size_t SinkContext::GetCount() const {
    return sink_count;
}

void SinkContext::UpdateMainSink(const SinkInfo::InParams& in) {
    ASSERT(in.type == SinkTypes::Device);

    if (in.device.down_matrix_enabled) {
        downmix_coefficients = in.device.down_matrix_coef;
    } else {
        downmix_coefficients = {
            1.0f,   // front
            0.707f, // center
            0.0f,   // lfe
            0.707f, // back
        };
    }

    in_use = in.in_use;
    use_count = in.device.input_count;
    buffers = in.device.input;
}

bool SinkContext::InUse() const {
    return in_use;
}

std::vector<u8> SinkContext::OutputBuffers() const {
    std::vector<u8> buffer_ret(use_count);
    std::memcpy(buffer_ret.data(), buffers.data(), use_count);
    return buffer_ret;
}

const DownmixCoefficients& SinkContext::GetDownmixCoefficients() const {
    return downmix_coefficients;
}

} // namespace AudioCore
