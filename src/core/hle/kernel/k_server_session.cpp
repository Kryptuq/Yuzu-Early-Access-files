// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <tuple>
#include <utility>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_handle_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/k_session.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/k_thread_queue.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/service_thread.h"
#include "core/memory.h"

namespace Kernel {

using ThreadQueueImplForKServerSessionRequest = KThreadQueue;

static constexpr u32 MessageBufferSize = 0x100;

KServerSession::KServerSession(KernelCore& kernel_)
    : KSynchronizationObject{kernel_}, m_lock{kernel_} {}

KServerSession::~KServerSession() = default;

void KServerSession::Initialize(KSession* parent_session_, std::string&& name_,
                                std::shared_ptr<SessionRequestManager> manager_) {
    // Set member variables.
    parent = parent_session_;
    name = std::move(name_);
    manager = manager_;
}

void KServerSession::Destroy() {
    parent->OnServerClosed();

    this->CleanupRequests();

    parent->Close();

    // Release host emulation members.
    manager.reset();

    // Ensure that the global list tracking server objects does not hold on to a reference.
    kernel.UnregisterServerObject(this);
}

void KServerSession::OnClientClosed() {
    if (manager && manager->HasSessionHandler()) {
        manager->SessionHandler().ClientDisconnected(this);
    }
}

bool KServerSession::IsSignaled() const {
    ASSERT(KScheduler::IsSchedulerLockedByCurrentThread(kernel));

    // If the client is closed, we're always signaled.
    if (parent->IsClientClosed()) {
        return true;
    }

    // Otherwise, we're signaled if we have a request and aren't handling one.
    return !m_thread_request_list.empty() && m_current_thread_request == nullptr;
}

void KServerSession::AppendDomainHandler(SessionRequestHandlerPtr handler) {
    manager->AppendDomainHandler(std::move(handler));
}

std::size_t KServerSession::NumDomainRequestHandlers() const {
    return manager->DomainHandlerCount();
}

Result KServerSession::HandleDomainSyncRequest(Kernel::HLERequestContext& context) {
    if (!context.HasDomainMessageHeader()) {
        return ResultSuccess;
    }

    // Set domain handlers in HLE context, used for domain objects (IPC interfaces) as inputs
    context.SetSessionRequestManager(manager);

    // If there is a DomainMessageHeader, then this is CommandType "Request"
    const auto& domain_message_header = context.GetDomainMessageHeader();
    const u32 object_id{domain_message_header.object_id};
    switch (domain_message_header.command) {
    case IPC::DomainMessageHeader::CommandType::SendMessage:
        if (object_id > manager->DomainHandlerCount()) {
            LOG_CRITICAL(IPC,
                         "object_id {} is too big! This probably means a recent service call "
                         "to {} needed to return a new interface!",
                         object_id, name);
            ASSERT(false);
            return ResultSuccess; // Ignore error if asserts are off
        }
        if (auto strong_ptr = manager->DomainHandler(object_id - 1).lock()) {
            return strong_ptr->HandleSyncRequest(*this, context);
        } else {
            ASSERT(false);
            return ResultSuccess;
        }

    case IPC::DomainMessageHeader::CommandType::CloseVirtualHandle: {
        LOG_DEBUG(IPC, "CloseVirtualHandle, object_id=0x{:08X}", object_id);

        manager->CloseDomainHandler(object_id - 1);

        IPC::ResponseBuilder rb{context, 2};
        rb.Push(ResultSuccess);
        return ResultSuccess;
    }
    }

    LOG_CRITICAL(IPC, "Unknown domain command={}", domain_message_header.command.Value());
    ASSERT(false);
    return ResultSuccess;
}

Result KServerSession::QueueSyncRequest(KThread* thread, Core::Memory::Memory& memory) {
    u32* cmd_buf{reinterpret_cast<u32*>(memory.GetPointer(thread->GetTLSAddress()))};
    auto context = std::make_shared<HLERequestContext>(kernel, memory, this, thread);

    context->PopulateFromIncomingCommandBuffer(kernel.CurrentProcess()->GetHandleTable(), cmd_buf);

    // Ensure we have a session request handler
    if (manager->HasSessionRequestHandler(*context)) {
        if (auto strong_ptr = manager->GetServiceThread().lock()) {
            strong_ptr->QueueSyncRequest(*parent, std::move(context));
        } else {
            ASSERT_MSG(false, "strong_ptr is nullptr!");
        }
    } else {
        ASSERT_MSG(false, "handler is invalid!");
    }

    return ResultSuccess;
}

Result KServerSession::CompleteSyncRequest(HLERequestContext& context) {
    Result result = ResultSuccess;

    // If the session has been converted to a domain, handle the domain request
    if (manager->HasSessionRequestHandler(context)) {
        if (IsDomain() && context.HasDomainMessageHeader()) {
            result = HandleDomainSyncRequest(context);
            // If there is no domain header, the regular session handler is used
        } else if (manager->HasSessionHandler()) {
            // If this ServerSession has an associated HLE handler, forward the request to it.
            result = manager->SessionHandler().HandleSyncRequest(*this, context);
        }
    } else {
        ASSERT_MSG(false, "Session handler is invalid, stubbing response!");
        IPC::ResponseBuilder rb(context, 2);
        rb.Push(ResultSuccess);
    }

    if (convert_to_domain) {
        ASSERT_MSG(!IsDomain(), "ServerSession is already a domain instance.");
        manager->ConvertToDomain();
        convert_to_domain = false;
    }

    // The calling thread is waiting for this request to complete, so wake it up.
    context.GetThread().EndWait(result);

    return result;
}

Result KServerSession::OnRequest() {
    // Create the wait queue.
    ThreadQueueImplForKServerSessionRequest wait_queue{kernel};

    {
        // Lock the scheduler.
        KScopedSchedulerLock sl{kernel};

        // Ensure that we can handle new requests.
        R_UNLESS(!parent->IsServerClosed(), ResultSessionClosed);

        // Check that we're not terminating.
        R_UNLESS(!GetCurrentThread(kernel).IsTerminationRequested(), ResultTerminationRequested);

        if (manager) {
            // HLE request.
            auto& memory{kernel.System().Memory()};
            this->QueueSyncRequest(GetCurrentThreadPointer(kernel), memory);
        } else {
            // Non-HLE request.
            auto* thread{GetCurrentThreadPointer(kernel)};

            // Get whether we're empty.
            const bool was_empty = m_thread_request_list.empty();

            // Add the thread to the list.
            thread->Open();
            m_thread_request_list.push_back(thread);

            // If we were empty, signal.
            if (was_empty) {
                this->NotifyAvailable();
            }
        }

        // This is a synchronous request, so we should wait for our request to complete.
        GetCurrentThread(kernel).SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::IPC);
        GetCurrentThread(kernel).BeginWait(&wait_queue);
    }

    return GetCurrentThread(kernel).GetWaitResult();
}

Result KServerSession::SendReply() {
    // Lock the session.
    KScopedLightLock lk(m_lock);

    // Get the request.
    KThread* client_thread;
    {
        KScopedSchedulerLock sl{kernel};

        // Get the current request.
        client_thread = m_current_thread_request;
        R_UNLESS(client_thread != nullptr, ResultInvalidState);

        // Clear the current request, since we're processing it.
        m_current_thread_request = nullptr;
        if (!m_thread_request_list.empty()) {
            this->NotifyAvailable();
        }
    }

    // Close reference to the request once we're done processing it.
    SCOPE_EXIT({ client_thread->Close(); });

    // Extract relevant information from the request.
    // const uintptr_t client_message  = request->GetAddress();
    // const size_t client_buffer_size = request->GetSize();
    // KThread *client_thread          = request->GetThread();
    // KEvent *event                   = request->GetEvent();

    // Check whether we're closed.
    const bool closed = (client_thread == nullptr || parent->IsClientClosed());

    Result result = ResultSuccess;
    if (!closed) {
        // If we're not closed, send the reply.
        Core::Memory::Memory& memory{kernel.System().Memory()};
        KThread* server_thread{GetCurrentThreadPointer(kernel)};
        UNIMPLEMENTED_IF(server_thread->GetOwnerProcess() != client_thread->GetOwnerProcess());

        auto* src_msg_buffer = memory.GetPointer(server_thread->GetTLSAddress());
        auto* dst_msg_buffer = memory.GetPointer(client_thread->GetTLSAddress());
        std::memcpy(dst_msg_buffer, src_msg_buffer, MessageBufferSize);
    } else {
        result = ResultSessionClosed;
    }

    // Select a result for the client.
    Result client_result = result;
    if (closed && R_SUCCEEDED(result)) {
        result = ResultSessionClosed;
        client_result = ResultSessionClosed;
    } else {
        result = ResultSuccess;
    }

    // If there's a client thread, update it.
    if (client_thread != nullptr) {
        // End the client thread's wait.
        KScopedSchedulerLock sl{kernel};

        if (!client_thread->IsTerminationRequested()) {
            client_thread->EndWait(client_result);
        }
    }

    return result;
}

Result KServerSession::ReceiveRequest() {
    // Lock the session.
    KScopedLightLock lk(m_lock);

    // Get the request and client thread.
    // KSessionRequest *request;
    KThread* client_thread;

    {
        KScopedSchedulerLock sl{kernel};

        // Ensure that we can service the request.
        R_UNLESS(!parent->IsClientClosed(), ResultSessionClosed);

        // Ensure we aren't already servicing a request.
        R_UNLESS(m_current_thread_request == nullptr, ResultNotFound);

        // Ensure we have a request to service.
        R_UNLESS(!m_thread_request_list.empty(), ResultNotFound);

        // Pop the first request from the list.
        client_thread = m_thread_request_list.front();
        m_thread_request_list.pop_front();

        // Get the thread for the request.
        R_UNLESS(client_thread != nullptr, ResultSessionClosed);

        // Open the client thread.
        client_thread->Open();
    }

    // SCOPE_EXIT({ client_thread->Close(); });

    // Set the request as our current.
    m_current_thread_request = client_thread;

    // Receive the message.
    Core::Memory::Memory& memory{kernel.System().Memory()};
    KThread* server_thread{GetCurrentThreadPointer(kernel)};
    UNIMPLEMENTED_IF(server_thread->GetOwnerProcess() != client_thread->GetOwnerProcess());

    auto* src_msg_buffer = memory.GetPointer(client_thread->GetTLSAddress());
    auto* dst_msg_buffer = memory.GetPointer(server_thread->GetTLSAddress());
    std::memcpy(dst_msg_buffer, src_msg_buffer, MessageBufferSize);

    // We succeeded.
    return ResultSuccess;
}

void KServerSession::CleanupRequests() {
    KScopedLightLock lk(m_lock);

    // Clean up any pending requests.
    while (true) {
        // Get the next request.
        // KSessionRequest *request = nullptr;
        KThread* client_thread = nullptr;
        {
            KScopedSchedulerLock sl{kernel};

            if (m_current_thread_request) {
                // Choose the current request if we have one.
                client_thread = m_current_thread_request;
                m_current_thread_request = nullptr;
            } else if (!m_thread_request_list.empty()) {
                // Pop the request from the front of the list.
                client_thread = m_thread_request_list.front();
                m_thread_request_list.pop_front();
            }
        }

        // If there's no request, we're done.
        if (client_thread == nullptr) {
            break;
        }

        // Close a reference to the request once it's cleaned up.
        SCOPE_EXIT({ client_thread->Close(); });

        // Extract relevant information from the request.
        // const uintptr_t client_message  = request->GetAddress();
        // const size_t client_buffer_size = request->GetSize();
        // KThread *client_thread          = request->GetThread();
        // KEvent *event                   = request->GetEvent();

        // KProcess *server_process             = request->GetServerProcess();
        // KProcess *client_process             = (client_thread != nullptr) ?
        //                                         client_thread->GetOwnerProcess() : nullptr;
        // KProcessPageTable *client_page_table = (client_process != nullptr) ?
        //                                         &client_process->GetPageTable() : nullptr;

        // Cleanup the mappings.
        // Result result = CleanupMap(request, server_process, client_page_table);

        // If there's a client thread, update it.
        if (client_thread != nullptr) {
            // End the client thread's wait.
            KScopedSchedulerLock sl{kernel};

            if (!client_thread->IsTerminationRequested()) {
                client_thread->EndWait(ResultSessionClosed);
            }
        }
    }
}

} // namespace Kernel
