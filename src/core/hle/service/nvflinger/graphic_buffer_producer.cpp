// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2010 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/libs/gui/IGraphicBufferProducer.cpp

#pragma once

#include "core/hle/service/nvflinger/graphic_buffer_producer.h"
#include "core/hle/service/nvflinger/parcel.h"

namespace Service::android {

QueueBufferInput::QueueBufferInput(Parcel& parcel) {
    parcel.ReadFlattened(*this);
}

QueueBufferOutput::QueueBufferOutput() = default;

} // namespace Service::android
