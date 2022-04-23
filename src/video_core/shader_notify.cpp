// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>

#include "video_core/shader_notify.h"

using namespace std::chrono_literals;

namespace VideoCore {

const auto TIME_TO_STOP_REPORTING = 2s;

int ShaderNotify::ShadersBuilding() noexcept {
    const int now_complete = num_complete.load(std::memory_order::relaxed);
    const int now_building = num_building.load(std::memory_order::relaxed);
    if (now_complete == now_building) {
        const auto now = std::chrono::steady_clock::now();
        if (completed && num_complete == num_when_completed) {
            if (now - complete_time > TIME_TO_STOP_REPORTING) {
                report_base = now_complete;
                completed = false;
            }
        } else {
            completed = true;
            num_when_completed = num_complete;
            complete_time = now;
        }
    }
    return now_building - report_base;
}

} // namespace VideoCore
