// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2010 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/libs/gui/IGraphicBufferProducer.cpp

#include "core/hle/service/nvflinger/graphic_buffer_producer.h"
#include "core/hle/service/nvflinger/parcel.h"

namespace Service::android {

QueueBufferInput::QueueBufferInput(InputParcel& parcel) {
    parcel.ReadFlattened(*this);
}

QueueBufferOutput::QueueBufferOutput() = default;

} // namespace Service::android
