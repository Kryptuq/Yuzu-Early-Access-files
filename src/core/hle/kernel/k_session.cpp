// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_client_session.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/k_session.h"

namespace Kernel {

KSession::KSession(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, server{kernel_}, client{kernel_} {}
KSession::~KSession() = default;

void KSession::Initialize(KClientPort* port_, const std::string& name_,
                          std::shared_ptr<SessionRequestManager> manager_) {
    // Increment reference count.
    // Because reference count is one on creation, this will result
    // in a reference count of two. Thus, when both server and client are closed
    // this object will be destroyed.
    Open();

    // Create our sub sessions.
    KAutoObject::Create(std::addressof(server));
    KAutoObject::Create(std::addressof(client));

    // Initialize our sub sessions.
    server.Initialize(this, name_ + ":Server", manager_);
    client.Initialize(this, name_ + ":Client");

    // Set state and name.
    SetState(State::Normal);
    name = name_;

    // Set our owner process.
    process = kernel.CurrentProcess();
    process->Open();

    // Set our port.
    port = port_;
    if (port != nullptr) {
        port->Open();
    }

    // Mark initialized.
    initialized = true;
}

void KSession::Finalize() {
    if (port == nullptr) {
        return;
    }

    port->OnSessionFinalized();
    port->Close();
}

void KSession::OnServerClosed() {
    if (GetState() != State::Normal) {
        return;
    }

    SetState(State::ServerClosed);
    client.OnServerClosed();
}

void KSession::OnClientClosed() {
    if (GetState() != State::Normal) {
        return;
    }

    SetState(State::ClientClosed);
    server.OnClientClosed();
}

void KSession::PostDestroy(uintptr_t arg) {
    // Release the session count resource the owner process holds.
    KProcess* owner = reinterpret_cast<KProcess*>(arg);
    owner->GetResourceLimit()->Release(LimitableResource::Sessions, 1);
    owner->Close();
}

} // namespace Kernel
