// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <string>

#include "core/hle/kernel/k_client_session.h"
#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class SessionRequestManager;

class KSession final : public KAutoObjectWithSlabHeapAndContainer<KSession, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KSession, KAutoObject);

public:
    explicit KSession(KernelCore& kernel);
    ~KSession() override;

    void Initialize(KClientPort* port, uintptr_t name);
    void Finalize() override;

    bool IsInitialized() const override {
        return m_initialized;
    }

    uintptr_t GetPostDestroyArgument() const override {
        return reinterpret_cast<uintptr_t>(m_process);
    }

    static void PostDestroy(uintptr_t arg);

    void OnServerClosed();

    void OnClientClosed();

    bool IsServerClosed() const {
        return this->GetState() != State::Normal;
    }

    bool IsClientClosed() const {
        return this->GetState() != State::Normal;
    }

    KClientSession& GetClientSession() {
        return m_client;
    }

    KServerSession& GetServerSession() {
        return m_server;
    }

    const KClientSession& GetClientSession() const {
        return m_client;
    }

    const KServerSession& GetServerSession() const {
        return m_server;
    }

    const KClientPort* GetParent() const {
        return m_port;
    }

private:
    enum class State : u8 {
        Invalid = 0,
        Normal = 1,
        ClientClosed = 2,
        ServerClosed = 3,
    };

    void SetState(State state) {
        m_atomic_state = static_cast<u8>(state);
    }

    State GetState() const {
        return static_cast<State>(m_atomic_state.load());
    }

    KServerSession m_server;
    KClientSession m_client;
    KClientPort* m_port{};
    uintptr_t m_name{};
    KProcess* m_process{};
    std::atomic<u8> m_atomic_state{static_cast<u8>(State::Invalid)};
    bool m_initialized{};
};

} // namespace Kernel
