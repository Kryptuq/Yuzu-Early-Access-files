// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "audio_core/sink_context.h"

namespace AudioCore {
SinkContext::SinkContext(std::size_t sink_count_) : sink_count{sink_count_} {}
SinkContext::~SinkContext() = default;

std::size_t SinkContext::GetCount() const {
    return sink_count;
}

void SinkContext::UpdateMainSink(const SinkInfo::InParams& in) {
    ASSERT(in.type == SinkTypes::Device);

    has_downmix_coefs = in.device.down_matrix_enabled;
    if (has_downmix_coefs) {
        downmix_coefficients = in.device.down_matrix_coef;
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

bool SinkContext::HasDownMixingCoefficients() const {
    return has_downmix_coefs;
}

const DownmixCoefficients& SinkContext::GetDownmixCoefficients() const {
    return downmix_coefficients;
}

} // namespace AudioCore
