// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/intrusive_red_black_tree.h"

namespace Kernel {

class KTimerTask : public Common::IntrusiveRedBlackTreeBaseNode<KTimerTask> {
public:
    static constexpr int Compare(const KTimerTask& lhs, const KTimerTask& rhs) {
        if (lhs.GetTime() < rhs.GetTime()) {
            return -1;
        } else {
            return 1;
        }
    }

    constexpr explicit KTimerTask() = default;

    constexpr void SetTime(s64 t) {
        m_time = t;
    }

    constexpr s64 GetTime() const {
        return m_time;
    }

    // NOTE: This is virtual in Nintendo's kernel. Prior to 13.0.0, KWaitObject was also a
    // TimerTask; this is no longer the case. Since this is now KThread exclusive, we have
    // devirtualized (see inline declaration for this inside k_thread.h).
    void OnTimer();

private:
    // Absolute time in nanoseconds
    s64 m_time{};
};

} // namespace Kernel
