// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2014 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/IProducerListener.h

#pragma once

namespace Service::android {

class IProducerListener {
public:
    virtual void OnBufferReleased() = 0;
};

} // namespace Service::android
