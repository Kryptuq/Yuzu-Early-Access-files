// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_types.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/result.h"

namespace Kernel {

class KClientSession;
class KernelCore;
class KPort;

class KClientPort final : public KSynchronizationObject {
    KERNEL_AUTOOBJECT_TRAITS(KClientPort, KSynchronizationObject);

public:
    explicit KClientPort(KernelCore& kernel);
    ~KClientPort() override;

    void Initialize(KPort* parent, s32 max_sessions);
    void OnSessionFinalized();
    void OnServerClosed();

    const KPort* GetParent() const {
        return m_parent;
    }
    KPort* GetParent() {
        return m_parent;
    }

    s32 GetNumSessions() const {
        return m_num_sessions;
    }
    s32 GetPeakSessions() const {
        return m_peak_sessions;
    }
    s32 GetMaxSessions() const {
        return m_max_sessions;
    }

    bool IsLight() const;
    bool IsServerClosed() const;

    // Overridden virtual functions.
    void Destroy() override;
    bool IsSignaled() const override;

    Result CreateSession(KClientSession** out);

private:
    std::atomic<s32> m_num_sessions{};
    std::atomic<s32> m_peak_sessions{};
    s32 m_max_sessions{};
    KPort* m_parent{};
};

} // namespace Kernel
