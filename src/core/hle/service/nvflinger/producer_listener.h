// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2014 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/IProducerListener.h

#pragma once

namespace Service::android {

class IProducerListener {
public:
    virtual ~IProducerListener() = default;
    virtual void OnBufferReleased() = 0;
};

} // namespace Service::android
