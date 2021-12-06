// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "audio_core/delay_line.h"

namespace AudioCore {
DelayLineBase::DelayLineBase() = default;
DelayLineBase::~DelayLineBase() = default;

void DelayLineBase::Initialize(s32 max_delay_, float* src_buffer) {
    buffer = src_buffer;
    buffer_end = buffer + max_delay_;
    max_delay = max_delay_;
    output = buffer;
    SetDelay(max_delay_);
    Clear();
}

void DelayLineBase::SetDelay(s32 new_delay) {
    if (max_delay < new_delay) {
        return;
    }
    delay = new_delay;
    input = (buffer + ((output - buffer) + new_delay) % (max_delay + 1));
}

s32 DelayLineBase::GetDelay() const {
    return delay;
}

s32 DelayLineBase::GetMaxDelay() const {
    return max_delay;
}

f32 DelayLineBase::TapOut(s32 last_sample) {
    const float* ptr = input - (last_sample + 1);
    if (ptr < buffer) {
        ptr += (max_delay + 1);
    }

    return *ptr;
}

f32 DelayLineBase::Tick(f32 sample) {
    *(input++) = sample;
    const auto out_sample = *(output++);

    if (buffer_end < input) {
        input = buffer;
    }

    if (buffer_end < output) {
        output = buffer;
    }

    return out_sample;
}

float* DelayLineBase::GetInput() {
    return input;
}

const float* DelayLineBase::GetInput() const {
    return input;
}

f32 DelayLineBase::GetOutputSample() const {
    return *output;
}

void DelayLineBase::Clear() {
    std::memset(buffer, 0, sizeof(float) * max_delay);
}

void DelayLineBase::Reset() {
    buffer = nullptr;
    buffer_end = nullptr;
    max_delay = 0;
    input = nullptr;
    output = nullptr;
    delay = 0;
}

DelayLineAllPass::DelayLineAllPass() = default;
DelayLineAllPass::~DelayLineAllPass() = default;

void DelayLineAllPass::Initialize(u32 delay_, float coeffcient_, f32* src_buffer) {
    DelayLineBase::Initialize(delay_, src_buffer);
    SetCoefficient(coeffcient_);
}

void DelayLineAllPass::SetCoefficient(float coeffcient_) {
    coefficient = coeffcient_;
}

f32 DelayLineAllPass::Tick(f32 sample) {
    const auto temp = sample - coefficient * *output;
    return coefficient * temp + DelayLineBase::Tick(temp);
}

void DelayLineAllPass::Reset() {
    coefficient = 0.0f;
    DelayLineBase::Reset();
}

} // namespace AudioCore
