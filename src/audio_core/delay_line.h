#pragma once

#include "common/common_types.h"

namespace AudioCore {

class DelayLineBase {
public:
    DelayLineBase();
    ~DelayLineBase();

    void Initialize(s32 max_delay_, float* src_buffer);
    void SetDelay(s32 new_delay);
    s32 GetDelay() const;
    s32 GetMaxDelay() const;
    f32 TapOut(s32 last_sample);
    f32 Tick(f32 sample);
    float* GetInput();
    const float* GetInput() const;
    f32 GetOutputSample() const;
    void Clear();
    void Reset();

protected:
    float* buffer{nullptr};
    float* buffer_end{nullptr};
    s32 max_delay{};
    float* input{nullptr};
    float* output{nullptr};
    s32 delay{};
};

class DelayLineAllPass final : public DelayLineBase {
public:
    DelayLineAllPass();
    ~DelayLineAllPass();

    void Initialize(u32 delay, float coeffcient_, f32* src_buffer);
    void SetCoefficient(float coeffcient_);
    f32 Tick(f32 sample);
    void Reset();

private:
    float coefficient{};
};
} // namespace AudioCore
