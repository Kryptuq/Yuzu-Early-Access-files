// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "common/scope_exit.h"
#include "common/thread.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/service_thread.h"

namespace Kernel {

class ServiceThread::Impl final {
public:
    explicit Impl(KernelCore& kernel, const std::string& service_name);
    ~Impl();

    void WaitAndProcessImpl();
    void SessionClosed(KServerSession* server_session,
                       std::shared_ptr<SessionRequestManager> manager);
    void LoopProcess();

    void RegisterServerSession(KServerSession* session,
                               std::shared_ptr<SessionRequestManager> manager);

private:
    KernelCore& kernel;

    std::jthread m_host_thread;
    std::mutex m_session_mutex;
    std::map<KServerSession*, std::shared_ptr<SessionRequestManager>> m_sessions;
    KEvent* m_wakeup_event;
    KProcess* m_process;
    KThread* m_thread;
    std::atomic<bool> m_shutdown_requested;
    const std::string m_service_name;
};

void ServiceThread::Impl::WaitAndProcessImpl() {
    // Create local list of waitable sessions.
    std::vector<KSynchronizationObject*> objs;
    std::vector<std::shared_ptr<SessionRequestManager>> managers;

    {
        // Lock to get the set.
        std::scoped_lock lk{m_session_mutex};

        // Reserve the needed quantity.
        objs.reserve(m_sessions.size() + 1);
        managers.reserve(m_sessions.size());

        // Copy to our local list.
        for (const auto& [session, manager] : m_sessions) {
            objs.push_back(session);
            managers.push_back(manager);
        }

        // Insert the wakeup event at the end.
        objs.push_back(&m_wakeup_event->GetReadableEvent());
    }

    // Wait on the list of sessions.
    s32 index{-1};
    Result rc = KSynchronizationObject::Wait(kernel, &index, objs.data(),
                                             static_cast<s32>(objs.size()), -1);
    ASSERT(!rc.IsFailure());

    // If this was the wakeup event, clear it and finish.
    if (index >= static_cast<s64>(objs.size() - 1)) {
        m_wakeup_event->Clear();
        return;
    }

    // This event is from a server session.
    auto* server_session = static_cast<KServerSession*>(objs[index]);
    auto& manager = managers[index];

    // Fetch the HLE request context.
    std::shared_ptr<HLERequestContext> context;
    rc = server_session->ReceiveRequest(&context, manager);

    // If the session was closed, handle that.
    if (rc == ResultSessionClosed) {
        SessionClosed(server_session, manager);

        // Finish.
        return;
    }

    // TODO: handle other cases
    ASSERT(rc == ResultSuccess);

    // Perform the request.
    Result service_rc = manager->CompleteSyncRequest(server_session, *context);

    // Reply to the client.
    rc = server_session->SendReplyHLE();

    if (rc == ResultSessionClosed || service_rc == IPC::ERR_REMOTE_PROCESS_DEAD) {
        SessionClosed(server_session, manager);
        return;
    }

    // TODO: handle other cases
    ASSERT(rc == ResultSuccess);
    ASSERT(service_rc == ResultSuccess);
}

void ServiceThread::Impl::SessionClosed(KServerSession* server_session,
                                        std::shared_ptr<SessionRequestManager> manager) {
    {
        // Lock to get the set.
        std::scoped_lock lk{m_session_mutex};

        // Erase the session.
        ASSERT(m_sessions.erase(server_session) == 1);
    }

    // Close our reference to the server session.
    server_session->Close();
}

void ServiceThread::Impl::LoopProcess() {
    Common::SetCurrentThreadName(m_service_name.c_str());

    kernel.RegisterHostThread(m_thread);

    while (!m_shutdown_requested.load()) {
        WaitAndProcessImpl();
    }
}

void ServiceThread::Impl::RegisterServerSession(KServerSession* server_session,
                                                std::shared_ptr<SessionRequestManager> manager) {
    // Open the server session.
    server_session->Open();

    {
        // Lock to get the set.
        std::scoped_lock lk{m_session_mutex};

        // Insert the session and manager.
        m_sessions[server_session] = manager;
    }

    // Signal the wakeup event.
    m_wakeup_event->Signal();
}

ServiceThread::Impl::~Impl() {
    // Shut down the processing thread.
    m_shutdown_requested.store(true);
    m_wakeup_event->Signal();
    m_host_thread.join();

    // Lock mutex.
    m_session_mutex.lock();

    // Close all remaining sessions.
    for (const auto& [server_session, manager] : m_sessions) {
        server_session->Close();
    }

    // Destroy remaining managers.
    m_sessions.clear();

    // Close event.
    m_wakeup_event->GetReadableEvent().Close();
    m_wakeup_event->Close();

    // Close thread.
    m_thread->Close();

    // Close process.
    m_process->Close();
}

ServiceThread::Impl::Impl(KernelCore& kernel_, const std::string& service_name)
    : kernel{kernel_}, m_service_name{service_name} {
    // Initialize process.
    m_process = KProcess::Create(kernel);
    KProcess::Initialize(m_process, kernel.System(), service_name,
                         KProcess::ProcessType::KernelInternal, kernel.GetSystemResourceLimit());

    // Reserve a new event from the process resource limit
    KScopedResourceReservation event_reservation(m_process, LimitableResource::Events);
    ASSERT(event_reservation.Succeeded());

    // Initialize event.
    m_wakeup_event = KEvent::Create(kernel);
    m_wakeup_event->Initialize(m_process);

    // Commit the event reservation.
    event_reservation.Commit();

    // Reserve a new thread from the process resource limit
    KScopedResourceReservation thread_reservation(m_process, LimitableResource::Threads);
    ASSERT(thread_reservation.Succeeded());

    // Initialize thread.
    m_thread = KThread::Create(kernel);
    ASSERT(KThread::InitializeDummyThread(m_thread, m_process).IsSuccess());

    // Commit the thread reservation.
    thread_reservation.Commit();

    // Start thread.
    m_host_thread = std::jthread([this] { LoopProcess(); });
}

ServiceThread::ServiceThread(KernelCore& kernel, const std::string& name)
    : impl{std::make_unique<Impl>(kernel, name)} {}

ServiceThread::~ServiceThread() = default;

void ServiceThread::RegisterServerSession(KServerSession* session,
                                          std::shared_ptr<SessionRequestManager> manager) {
    impl->RegisterServerSession(session, manager);
}

} // namespace Kernel
