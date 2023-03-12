// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <tuple>
#include "common/assert.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/k_thread.h"

namespace Kernel {

KServerPort::KServerPort(KernelCore& kernel) : KSynchronizationObject{kernel} {}
KServerPort::~KServerPort() = default;

void KServerPort::Initialize(KPort* parent) {
    // Set member variables.
    m_parent = parent;
}

bool KServerPort::IsLight() const {
    return this->GetParent()->IsLight();
}

void KServerPort::CleanupSessions() {
    // Ensure our preconditions are met.
    if (this->IsLight()) {
        UNIMPLEMENTED();
    }

    // Cleanup the session list.
    while (true) {
        // Get the last session in the list
        KServerSession* session = nullptr;
        {
            KScopedSchedulerLock sl{m_kernel};
            if (!m_session_list.empty()) {
                session = std::addressof(m_session_list.front());
                m_session_list.pop_front();
            }
        }

        // Close the session.
        if (session != nullptr) {
            session->Close();
        } else {
            break;
        }
    }
}

void KServerPort::Destroy() {
    // Note with our parent that we're closed.
    m_parent->OnServerClosed();

    // Perform necessary cleanup of our session lists.
    this->CleanupSessions();

    // Close our reference to our parent.
    m_parent->Close();
}

bool KServerPort::IsSignaled() const {
    if (this->IsLight()) {
        UNIMPLEMENTED();
        return false;
    } else {
        return !m_session_list.empty();
    }
}

void KServerPort::EnqueueSession(KServerSession* session) {
    ASSERT(!this->IsLight());

    KScopedSchedulerLock sl{m_kernel};

    // Add the session to our queue.
    m_session_list.push_back(*session);
    if (m_session_list.size() == 1) {
        this->NotifyAvailable();
    }
}

KServerSession* KServerPort::AcceptSession() {
    ASSERT(!this->IsLight());

    KScopedSchedulerLock sl{m_kernel};

    // Return the first session in the list.
    if (m_session_list.empty()) {
        return nullptr;
    }

    KServerSession* session = std::addressof(m_session_list.front());
    m_session_list.pop_front();
    return session;
}

} // namespace Kernel
