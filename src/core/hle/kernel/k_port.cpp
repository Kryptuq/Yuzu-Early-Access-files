// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KPort::KPort(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, server{kernel_}, client{kernel_} {}

KPort::~KPort() = default;

void KPort::Initialize(s32 max_sessions_, bool is_light_, const std::string& name_) {
    // Open a new reference count to the initialized port.
    Open();

    // Create and initialize our server/client pair.
    KAutoObject::Create(std::addressof(server));
    KAutoObject::Create(std::addressof(client));
    server.Initialize(this, name_ + ":Server");
    client.Initialize(this, max_sessions_, name_ + ":Client");

    // Set our member variables.
    is_light = is_light_;
    name = name_;
    state = State::Normal;
}

void KPort::OnClientClosed() {
    KScopedSchedulerLock sl{kernel};

    if (state == State::Normal) {
        state = State::ClientClosed;
    }
}

void KPort::OnServerClosed() {
    KScopedSchedulerLock sl{kernel};

    if (state == State::Normal) {
        state = State::ServerClosed;
    }
}

bool KPort::IsServerClosed() const {
    KScopedSchedulerLock sl{kernel};
    return state == State::ServerClosed;
}

ResultCode KPort::EnqueueSession(KServerSession* session) {
    KScopedSchedulerLock sl{kernel};

    R_UNLESS(state == State::Normal, ResultPortClosed);

    server.EnqueueSession(session);

    if (auto session_ptr = server.GetSessionRequestHandler().lock(); session_ptr) {
        session_ptr->ClientConnected(server.AcceptSession());
    } else {
        UNREACHABLE();
    }

    return ResultSuccess;
}

} // namespace Kernel
