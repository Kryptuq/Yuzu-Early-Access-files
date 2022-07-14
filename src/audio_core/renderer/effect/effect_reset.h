// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/effect/aux_.h"
#include "audio_core/renderer/effect/biquad_filter.h"
#include "audio_core/renderer/effect/buffer_mixer.h"
#include "audio_core/renderer/effect/capture.h"
#include "audio_core/renderer/effect/compressor.h"
#include "audio_core/renderer/effect/delay.h"
#include "audio_core/renderer/effect/i3dl2.h"
#include "audio_core/renderer/effect/light_limiter.h"
#include "audio_core/renderer/effect/reverb.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Reset an effect, and create a new one of the given type.
 *
 * @param effect - Effect to reset and re-construct.
 * @param type   - Type of the new effect to create.
 */
static void ResetEffect(EffectInfoBase* effect, const EffectInfoBase::Type type) {
    *effect = {};

    switch (type) {
    case EffectInfoBase::Type::Invalid:
        std::construct_at<EffectInfoBase>(effect);
        effect->SetType(EffectInfoBase::Type::Invalid);
        break;
    case EffectInfoBase::Type::Mix:
        std::construct_at<BufferMixerInfo>(reinterpret_cast<BufferMixerInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::Mix);
        break;
    case EffectInfoBase::Type::Aux:
        std::construct_at<AuxInfo>(reinterpret_cast<AuxInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::Aux);
        break;
    case EffectInfoBase::Type::Delay:
        std::construct_at<DelayInfo>(reinterpret_cast<DelayInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::Delay);
        break;
    case EffectInfoBase::Type::Reverb:
        std::construct_at<ReverbInfo>(reinterpret_cast<ReverbInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::Reverb);
        break;
    case EffectInfoBase::Type::I3dl2Reverb:
        std::construct_at<I3dl2ReverbInfo>(reinterpret_cast<I3dl2ReverbInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::I3dl2Reverb);
        break;
    case EffectInfoBase::Type::BiquadFilter:
        std::construct_at<BiquadFilterInfo>(reinterpret_cast<BiquadFilterInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::BiquadFilter);
        break;
    case EffectInfoBase::Type::LightLimiter:
        std::construct_at<LightLimiterInfo>(reinterpret_cast<LightLimiterInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::LightLimiter);
        break;
    case EffectInfoBase::Type::Capture:
        std::construct_at<CaptureInfo>(reinterpret_cast<CaptureInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::Capture);
        break;
    case EffectInfoBase::Type::Compressor:
        std::construct_at<CompressorInfo>(reinterpret_cast<CompressorInfo*>(effect));
        effect->SetType(EffectInfoBase::Type::Compressor);
        break;
    }
}

} // namespace AudioCore::AudioRenderer
