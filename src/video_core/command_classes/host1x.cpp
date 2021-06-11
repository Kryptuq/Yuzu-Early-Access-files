// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/command_classes/host1x.h"
#include "video_core/gpu.h"

Tegra::Host1x::Host1x(GPU& gpu_) : gpu(gpu_) {}

Tegra::Host1x::~Host1x() = default;

void Tegra::Host1x::ProcessMethod(Method method, u32 argument) {
    switch (method) {
    case Method::LoadSyncptPayload32:
        syncpoint_value = argument;
        break;
    case Method::WaitSyncpt:
    case Method::WaitSyncpt32:
        Execute(argument);
        break;
    default:
        UNIMPLEMENTED_MSG("Host1x method 0x{:X}", static_cast<u32>(method));
        break;
    }
}

void Tegra::Host1x::Execute(u32 data) {
    gpu.WaitFence(data, syncpoint_value);
}
