// Copyright 2021 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/scope_exit.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KClientPort::KClientPort(KernelCore& kernel_) : KSynchronizationObject{kernel_} {}
KClientPort::~KClientPort() = default;

void KClientPort::Initialize(KPort* parent_port_, s32 max_sessions_, std::string&& name_) {
    // Set member variables.
    num_sessions = 0;
    peak_sessions = 0;
    parent = parent_port_;
    max_sessions = max_sessions_;
    name = std::move(name_);
}

void KClientPort::OnSessionFinalized() {
    KScopedSchedulerLock sl{kernel};

    // This might happen if a session was improperly used with this port.
    ASSERT_MSG(num_sessions > 0, "num_sessions is invalid");

    const auto prev = num_sessions--;
    if (prev == max_sessions) {
        this->NotifyAvailable();
    }
}

void KClientPort::OnServerClosed() {}

bool KClientPort::IsLight() const {
    return this->GetParent()->IsLight();
}

bool KClientPort::IsServerClosed() const {
    return this->GetParent()->IsServerClosed();
}

void KClientPort::Destroy() {
    // Note with our parent that we're closed.
    parent->OnClientClosed();

    // Close our reference to our parent.
    parent->Close();
}

bool KClientPort::IsSignaled() const {
    return num_sessions < max_sessions;
}

Result KClientPort::CreateSession(KClientSession** out,
                                  std::shared_ptr<SessionRequestManager> session_manager) {
    // Reserve a new session from the resource limit.
    KScopedResourceReservation session_reservation(kernel.CurrentProcess()->GetResourceLimit(),
                                                   LimitableResource::Sessions);
    R_UNLESS(session_reservation.Succeeded(), ResultLimitReached);

    // Update the session counts.
    {
        // Atomically increment the number of sessions.
        s32 new_sessions{};
        {
            const auto max = max_sessions;
            auto cur_sessions = num_sessions.load(std::memory_order_acquire);
            do {
                R_UNLESS(cur_sessions < max, ResultOutOfSessions);
                new_sessions = cur_sessions + 1;
            } while (!num_sessions.compare_exchange_weak(cur_sessions, new_sessions,
                                                         std::memory_order_relaxed));
        }

        // Atomically update the peak session tracking.
        {
            auto peak = peak_sessions.load(std::memory_order_acquire);
            do {
                if (peak >= new_sessions) {
                    break;
                }
            } while (!peak_sessions.compare_exchange_weak(peak, new_sessions,
                                                          std::memory_order_relaxed));
        }
    }

    // Create a new session.
    KSession* session = KSession::Create(kernel);
    if (session == nullptr) {
        // Decrement the session count.
        const auto prev = num_sessions--;
        if (prev == max_sessions) {
            this->NotifyAvailable();
        }

        return ResultOutOfResource;
    }

    // Initialize the session.
    session->Initialize(this, parent->GetName(), session_manager);

    // Commit the session reservation.
    session_reservation.Commit();

    // Register the session.
    KSession::Register(kernel, session);
    auto session_guard = SCOPE_GUARD({
        session->GetClientSession().Close();
        session->GetServerSession().Close();
    });

    // Enqueue the session with our parent.
    R_TRY(parent->EnqueueSession(std::addressof(session->GetServerSession())));

    // We succeeded, so set the output.
    session_guard.Cancel();
    *out = std::addressof(session->GetClientSession());
    return ResultSuccess;
}

} // namespace Kernel
