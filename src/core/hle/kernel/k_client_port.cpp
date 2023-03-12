// SPDX-FileCopyrightText: 2021 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KClientPort::KClientPort(KernelCore& kernel) : KSynchronizationObject{kernel} {}
KClientPort::~KClientPort() = default;

void KClientPort::Initialize(KPort* parent, s32 max_sessions) {
    // Set member variables.
    m_num_sessions = 0;
    m_peak_sessions = 0;
    m_parent = parent;
    m_max_sessions = max_sessions;
}

void KClientPort::OnSessionFinalized() {
    KScopedSchedulerLock sl{m_kernel};

    if (const auto prev = m_num_sessions--; prev == m_max_sessions) {
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
    m_parent->OnClientClosed();

    // Close our reference to our parent.
    m_parent->Close();
}

bool KClientPort::IsSignaled() const {
    return m_num_sessions.load() < m_max_sessions;
}

Result KClientPort::CreateSession(KClientSession** out) {
    // Declare the session we're going to allocate.
    KSession* session{};

    // Reserve a new session from the resource limit.
    //! FIXME: we are reserving this from the wrong resource limit!
    KScopedResourceReservation session_reservation(
        m_kernel.ApplicationProcess()->GetResourceLimit(), LimitableResource::SessionCountMax);
    R_UNLESS(session_reservation.Succeeded(), ResultLimitReached);

    // Allocate a session normally.
    session = KSession::Create(m_kernel);

    // Check that we successfully created a session.
    R_UNLESS(session != nullptr, ResultOutOfResource);

    // Update the session counts.
    {
        ON_RESULT_FAILURE {
            session->Close();
        };

        // Atomically increment the number of sessions.
        s32 new_sessions{};
        {
            const auto max = m_max_sessions;
            auto cur_sessions = m_num_sessions.load(std::memory_order_acquire);
            do {
                R_UNLESS(cur_sessions < max, ResultOutOfSessions);
                new_sessions = cur_sessions + 1;
            } while (!m_num_sessions.compare_exchange_weak(cur_sessions, new_sessions,
                                                           std::memory_order_relaxed));
        }

        // Atomically update the peak session tracking.
        {
            auto peak = m_peak_sessions.load(std::memory_order_acquire);
            do {
                if (peak >= new_sessions) {
                    break;
                }
            } while (!m_peak_sessions.compare_exchange_weak(peak, new_sessions,
                                                            std::memory_order_relaxed));
        }
    }

    // Initialize the session.
    session->Initialize(this, m_parent->GetName());

    // Commit the session reservation.
    session_reservation.Commit();

    // Register the session.
    KSession::Register(m_kernel, session);
    ON_RESULT_FAILURE {
        session->GetClientSession().Close();
        session->GetServerSession().Close();
    };

    // Enqueue the session with our parent.
    R_TRY(m_parent->EnqueueSession(std::addressof(session->GetServerSession())));

    // We succeeded, so set the output.
    *out = std::addressof(session->GetClientSession());
    R_SUCCEED();
}

} // namespace Kernel
