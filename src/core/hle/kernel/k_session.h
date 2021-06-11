// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
    explicit KSession(KernelCore& kernel_);
    ~KSession() override;

    void Initialize(KClientPort* port_, const std::string& name_,
                    std::shared_ptr<SessionRequestManager> manager_ = nullptr);

    void Finalize() override;

    bool IsInitialized() const override {
        return initialized;
    }

    uintptr_t GetPostDestroyArgument() const override {
        return reinterpret_cast<uintptr_t>(process);
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
        return client;
    }

    KServerSession& GetServerSession() {
        return server;
    }

    const KClientSession& GetClientSession() const {
        return client;
    }

    const KServerSession& GetServerSession() const {
        return server;
    }

    const KClientPort* GetParent() const {
        return port;
    }

    KClientPort* GetParent() {
        return port;
    }

private:
    enum class State : u8 {
        Invalid = 0,
        Normal = 1,
        ClientClosed = 2,
        ServerClosed = 3,
    };

    void SetState(State state) {
        atomic_state = static_cast<u8>(state);
    }

    State GetState() const {
        return static_cast<State>(atomic_state.load(std::memory_order_relaxed));
    }

    KServerSession server;
    KClientSession client;
    std::atomic<std::underlying_type_t<State>> atomic_state{
        static_cast<std::underlying_type_t<State>>(State::Invalid)};
    KClientPort* port{};
    KProcess* process{};
    bool initialized{};
};

} // namespace Kernel
