// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2014 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/IConsumerListener.h

#pragma once

namespace Service::android {

class BufferItem;

/// ConsumerListener is the interface through which the BufferQueue notifies the consumer of events
/// that the consumer may wish to react to.
class IConsumerListener {
public:
    IConsumerListener() = default;
    virtual ~IConsumerListener() = default;

    virtual void OnFrameAvailable(const BufferItem& item) = 0;
    virtual void OnFrameReplaced(const BufferItem& item) = 0;
    virtual void OnBuffersReleased() = 0;
    virtual void OnSidebandStreamChanged() = 0;
};

}; // namespace Service::android
