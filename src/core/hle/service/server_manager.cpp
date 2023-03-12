// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"

#include "core/core.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_client_session.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_object_name.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/svc_results.h"
#include "core/hle/service/hle_ipc.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/sm/sm.h"

namespace Service {

constexpr size_t MaximumWaitObjects = 0x40;

enum HandleType {
    Port,
    Session,
    DeferEvent,
    Event,
};

ServerManager::ServerManager(Core::System& system) : m_system{system}, m_serve_mutex{system} {
    // Initialize event.
    m_event = Kernel::KEvent::Create(system.Kernel());
    m_event->Initialize(nullptr);
}

ServerManager::~ServerManager() {
    // Signal stop.
    m_stop_source.request_stop();
    m_event->Signal();

    // Wait for processing to stop.
    m_stopped.wait(false);
    m_threads.clear();

    // Clean up ports.
    for (const auto& [port, handler] : m_ports) {
        port->Close();
    }

    // Clean up sessions.
    for (const auto& [session, manager] : m_sessions) {
        session->Close();
    }

    for (const auto& request : m_deferrals) {
        request.session->Close();
    }

    // Close event.
    m_event->GetReadableEvent().Close();
    m_event->Close();

    if (m_deferral_event) {
        m_deferral_event->GetReadableEvent().Close();
        // Write event is owned by ServiceManager
    }
}

void ServerManager::RunServer(std::unique_ptr<ServerManager>&& server_manager) {
    server_manager->m_system.RunServer(std::move(server_manager));
}

Result ServerManager::RegisterSession(Kernel::KServerSession* session,
                                      std::shared_ptr<SessionRequestManager> manager) {
    ASSERT(m_sessions.size() + m_ports.size() < MaximumWaitObjects);

    // We are taking ownership of the server session, so don't open it.
    // Begin tracking the server session.
    {
        std::scoped_lock ll{m_list_mutex};
        m_sessions.emplace(session, std::move(manager));
    }

    // Signal the wakeup event.
    m_event->Signal();

    R_SUCCEED();
}

Result ServerManager::RegisterNamedService(const std::string& service_name,
                                           std::shared_ptr<SessionRequestHandler>&& handler,
                                           u32 max_sessions) {
    ASSERT(m_sessions.size() + m_ports.size() < MaximumWaitObjects);

    // Add the new server to sm:.
    ASSERT(R_SUCCEEDED(
        m_system.ServiceManager().RegisterService(service_name, max_sessions, handler)));

    // Get the registered port.
    auto port = m_system.ServiceManager().GetServicePort(service_name);
    ASSERT(port.Succeeded());

    // Open a new reference to the server port.
    (*port)->GetServerPort().Open();

    // Begin tracking the server port.
    {
        std::scoped_lock ll{m_list_mutex};
        m_ports.emplace(std::addressof((*port)->GetServerPort()), std::move(handler));
    }

    // Signal the wakeup event.
    m_event->Signal();

    R_SUCCEED();
}

Result ServerManager::ManageNamedPort(const std::string& service_name,
                                      std::shared_ptr<SessionRequestHandler>&& handler,
                                      u32 max_sessions) {
    ASSERT(m_sessions.size() + m_ports.size() < MaximumWaitObjects);

    // Create a new port.
    auto* port = Kernel::KPort::Create(m_system.Kernel());
    port->Initialize(max_sessions, false, 0);

    // Register the port.
    Kernel::KPort::Register(m_system.Kernel(), port);

    // Ensure that our reference to the port is closed if we fail to register it.
    SCOPE_EXIT({
        port->GetClientPort().Close();
        port->GetServerPort().Close();
    });

    // Register the object name with the kernel.
    R_TRY(Kernel::KObjectName::NewFromName(m_system.Kernel(), std::addressof(port->GetClientPort()),
                                           service_name.c_str()));

    // Open a new reference to the server port.
    port->GetServerPort().Open();

    // Begin tracking the server port.
    {
        std::scoped_lock ll{m_list_mutex};
        m_ports.emplace(std::addressof(port->GetServerPort()), std::move(handler));
    }

    // We succeeded.
    R_SUCCEED();
}

Result ServerManager::ManageDeferral(Kernel::KEvent** out_event) {
    // Create a new event.
    m_deferral_event = Kernel::KEvent::Create(m_system.Kernel());
    ASSERT(m_deferral_event != nullptr);

    // Initialize the event.
    m_deferral_event->Initialize(nullptr);

    // Set the output.
    *out_event = m_deferral_event;

    // We succeeded.
    R_SUCCEED();
}

void ServerManager::StartAdditionalHostThreads(const char* name, size_t num_threads) {
    for (size_t i = 0; i < num_threads; i++) {
        auto thread_name = fmt::format("{}:{}", name, i + 1);
        m_threads.emplace_back(m_system.Kernel().RunOnHostCoreThread(
            std::move(thread_name), [&] { this->LoopProcessImpl(); }));
    }
}

Result ServerManager::LoopProcess() {
    SCOPE_EXIT({
        m_stopped.store(true);
        m_stopped.notify_all();
    });

    R_RETURN(this->LoopProcessImpl());
}

Result ServerManager::LoopProcessImpl() {
    while (!m_stop_source.stop_requested()) {
        R_TRY(this->WaitAndProcessImpl());
    }

    R_SUCCEED();
}

Result ServerManager::WaitAndProcessImpl() {
    Kernel::KScopedAutoObject<Kernel::KSynchronizationObject> wait_obj;
    HandleType wait_type{};

    // Ensure we are the only thread waiting for this server.
    std::unique_lock sl{m_serve_mutex};

    // If we're done, return before we start waiting.
    R_SUCCEED_IF(m_stop_source.stop_requested());

    // Wait for a tracked object to become signaled.
    {
        s32 num_objs{};
        std::array<HandleType, MaximumWaitObjects> wait_types{};
        std::array<Kernel::KSynchronizationObject*, MaximumWaitObjects> wait_objs{};

        const auto AddWaiter{
            [&](Kernel::KSynchronizationObject* synchronization_object, HandleType type) {
                // Open a new reference to the object.
                synchronization_object->Open();

                // Insert into the list.
                wait_types[num_objs] = type;
                wait_objs[num_objs++] = synchronization_object;
            }};

        {
            std::scoped_lock ll{m_list_mutex};

            // Add all of our ports.
            for (const auto& [port, handler] : m_ports) {
                AddWaiter(port, HandleType::Port);
            }

            // Add all of our sessions.
            for (const auto& [session, manager] : m_sessions) {
                AddWaiter(session, HandleType::Session);
            }
        }

        // Add the deferral wakeup event.
        if (m_deferral_event != nullptr) {
            AddWaiter(std::addressof(m_deferral_event->GetReadableEvent()), HandleType::DeferEvent);
        }

        // Add the wakeup event.
        AddWaiter(std::addressof(m_event->GetReadableEvent()), HandleType::Event);

        // Clean up extra references on exit.
        SCOPE_EXIT({
            for (s32 i = 0; i < num_objs; i++) {
                wait_objs[i]->Close();
            }
        });

        // Wait for a signal.
        s32 out_index{-1};
        R_TRY(Kernel::KSynchronizationObject::Wait(m_system.Kernel(), &out_index, wait_objs.data(),
                                                   num_objs, -1));
        ASSERT(out_index >= 0 && out_index < num_objs);

        // Set the output index.
        wait_obj = wait_objs[out_index];
        wait_type = wait_types[out_index];
    }

    // Process what we just received, temporarily removing the object so it is
    // not processed concurrently by another thread.
    {
        switch (wait_type) {
        case HandleType::Port: {
            // Port signaled.
            auto* port = wait_obj->DynamicCast<Kernel::KServerPort*>();
            std::shared_ptr<SessionRequestHandler> handler;

            // Remove from tracking.
            {
                std::scoped_lock ll{m_list_mutex};
                ASSERT(m_ports.contains(port));
                m_ports.at(port).swap(handler);
                m_ports.erase(port);
            }

            // Allow other threads to serve.
            sl.unlock();

            // Finish.
            R_RETURN(this->OnPortEvent(port, std::move(handler)));
        }
        case HandleType::Session: {
            // Session signaled.
            auto* session = wait_obj->DynamicCast<Kernel::KServerSession*>();
            std::shared_ptr<SessionRequestManager> manager;

            // Remove from tracking.
            {
                std::scoped_lock ll{m_list_mutex};
                ASSERT(m_sessions.contains(session));
                m_sessions.at(session).swap(manager);
                m_sessions.erase(session);
            }

            // Allow other threads to serve.
            sl.unlock();

            // Finish.
            R_RETURN(this->OnSessionEvent(session, std::move(manager)));
        }
        case HandleType::DeferEvent: {
            // Clear event.
            ASSERT(R_SUCCEEDED(m_deferral_event->Clear()));

            // Drain the list of deferrals while we process.
            std::list<RequestState> deferrals;
            {
                std::scoped_lock ll{m_list_mutex};
                m_deferrals.swap(deferrals);
            }

            // Allow other threads to serve.
            sl.unlock();

            // Finish.
            R_RETURN(this->OnDeferralEvent(std::move(deferrals)));
        }
        case HandleType::Event: {
            // Clear event and finish.
            R_RETURN(m_event->Clear());
        }
        default: {
            UNREACHABLE();
        }
        }
    }
}

Result ServerManager::OnPortEvent(Kernel::KServerPort* port,
                                  std::shared_ptr<SessionRequestHandler>&& handler) {
    // Accept a new server session.
    Kernel::KServerSession* session = port->AcceptSession();
    ASSERT(session != nullptr);

    // Create the session manager and install the handler.
    auto manager = std::make_shared<SessionRequestManager>(m_system.Kernel(), *this);
    manager->SetSessionHandler(std::shared_ptr(handler));

    // Track the server session.
    {
        std::scoped_lock ll{m_list_mutex};
        m_ports.emplace(port, std::move(handler));
        m_sessions.emplace(session, std::move(manager));
    }

    // Signal the wakeup event.
    m_event->Signal();

    // We succeeded.
    R_SUCCEED();
}

Result ServerManager::OnSessionEvent(Kernel::KServerSession* session,
                                     std::shared_ptr<SessionRequestManager>&& manager) {
    Result rc{ResultSuccess};

    // Try to receive a message.
    std::shared_ptr<HLERequestContext> context;
    rc = session->ReceiveRequest(&context, manager);

    // If the session has been closed, we're done.
    if (rc == Kernel::ResultSessionClosed) {
        // Close the session.
        session->Close();

        // Finish.
        R_SUCCEED();
    }
    ASSERT(R_SUCCEEDED(rc));

    RequestState request{
        .session = session,
        .context = std::move(context),
        .manager = std::move(manager),
    };

    // Complete the sync request with deferral handling.
    R_RETURN(this->CompleteSyncRequest(std::move(request)));
}

Result ServerManager::CompleteSyncRequest(RequestState&& request) {
    Result rc{ResultSuccess};
    Result service_rc{ResultSuccess};

    // Mark the request as not deferred.
    request.context->SetIsDeferred(false);

    // Complete the request. We have exclusive access to this session.
    service_rc = request.manager->CompleteSyncRequest(request.session, *request.context);

    // If we've been deferred, we're done.
    if (request.context->GetIsDeferred()) {
        // Insert into deferral list.
        std::scoped_lock ll{m_list_mutex};
        m_deferrals.emplace_back(std::move(request));

        // Finish.
        R_SUCCEED();
    }

    // Send the reply.
    rc = request.session->SendReplyHLE();

    // If the session has been closed, we're done.
    if (rc == Kernel::ResultSessionClosed || service_rc == IPC::ResultSessionClosed) {
        // Close the session.
        request.session->Close();

        // Finish.
        R_SUCCEED();
    }

    ASSERT(R_SUCCEEDED(rc));
    ASSERT(R_SUCCEEDED(service_rc));

    // Reinsert the session.
    {
        std::scoped_lock ll{m_list_mutex};
        m_sessions.emplace(request.session, std::move(request.manager));
    }

    // Signal the wakeup event.
    m_event->Signal();

    // We succeeded.
    R_SUCCEED();
}

Result ServerManager::OnDeferralEvent(std::list<RequestState>&& deferrals) {
    ON_RESULT_FAILURE {
        std::scoped_lock ll{m_list_mutex};
        m_deferrals.splice(m_deferrals.end(), deferrals);
    };

    while (!deferrals.empty()) {
        RequestState request = deferrals.front();
        deferrals.pop_front();

        // Try again to complete the request.
        R_TRY(this->CompleteSyncRequest(std::move(request)));
    }

    R_SUCCEED();
}

} // namespace Service
