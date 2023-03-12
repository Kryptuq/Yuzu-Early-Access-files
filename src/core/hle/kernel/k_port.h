// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/common_types.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Kernel {

class KServerSession;

class KPort final : public KAutoObjectWithSlabHeapAndContainer<KPort, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KPort, KAutoObject);

public:
    explicit KPort(KernelCore& kernel);
    ~KPort() override;

    static void PostDestroy(uintptr_t arg) {}

    void Initialize(s32 max_sessions, bool is_light, uintptr_t name);
    void OnClientClosed();
    void OnServerClosed();

    uintptr_t GetName() const {
        return m_name;
    }
    bool IsLight() const {
        return m_is_light;
    }

    bool IsServerClosed() const;

    Result EnqueueSession(KServerSession* session);

    KClientPort& GetClientPort() {
        return m_client;
    }
    KServerPort& GetServerPort() {
        return m_server;
    }
    const KClientPort& GetClientPort() const {
        return m_client;
    }
    const KServerPort& GetServerPort() const {
        return m_server;
    }

private:
    enum class State : u8 {
        Invalid = 0,
        Normal = 1,
        ClientClosed = 2,
        ServerClosed = 3,
    };

    KServerPort m_server;
    KClientPort m_client;
    uintptr_t m_name;
    State m_state{State::Invalid};
    bool m_is_light{};
};

} // namespace Kernel
