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
#include "core/memory.h"

namespace Kernel {

using ThreadQueueImplForKServerSessionRequest = KThreadQueue;

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
    return !m_request_list.empty() && m_current_request == nullptr;
}

Result KServerSession::QueueSyncRequest(KThread* thread, Core::Memory::Memory& memory) {
    u32* cmd_buf{reinterpret_cast<u32*>(memory.GetPointer(thread->GetTLSAddress()))};
    auto context = std::make_shared<HLERequestContext>(kernel, memory, this, thread);

    context->PopulateFromIncomingCommandBuffer(kernel.CurrentProcess()->GetHandleTable(), cmd_buf);

    return manager->QueueSyncRequest(parent, std::move(context));
}

Result KServerSession::CompleteSyncRequest(HLERequestContext& context) {
    Result result = manager->CompleteSyncRequest(this, context);

    // The calling thread is waiting for this request to complete, so wake it up.
    context.GetThread().EndWait(result);

    return result;
}

Result KServerSession::OnRequest(KSessionRequest* request) {
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

            // Get whether we're empty.
            const bool was_empty = m_request_list.empty();

            // Add the request to the list.
            request->Open();
            m_request_list.push_back(*request);

            // If we were empty, signal.
            if (was_empty) {
                this->NotifyAvailable();
            }
        }

        // If we have a request event, this is asynchronous, and we don't need to wait.
        R_SUCCEED_IF(request->GetEvent() != nullptr);

        // This is a synchronous request, so we should wait for our request to complete.
        GetCurrentThread(kernel).SetWaitReasonForDebugging(ThreadWaitReasonForDebugging::IPC);
        GetCurrentThread(kernel).BeginWait(&wait_queue);
    }

    return GetCurrentThread(kernel).GetWaitResult();
}

Result KServerSession::SendReply() {
    // Lock the session.
    KScopedLightLock lk{m_lock};

    // Get the request.
    KSessionRequest* request;
    {
        KScopedSchedulerLock sl{kernel};

        // Get the current request.
        request = m_current_request;
        R_UNLESS(request != nullptr, ResultInvalidState);

        // Clear the current request, since we're processing it.
        m_current_request = nullptr;
        if (!m_request_list.empty()) {
            this->NotifyAvailable();
        }
    }

    // Close reference to the request once we're done processing it.
    SCOPE_EXIT({ request->Close(); });

    // Extract relevant information from the request.
    const uintptr_t client_message = request->GetAddress();
    const size_t client_buffer_size = request->GetSize();
    KThread* client_thread = request->GetThread();
    KEvent* event = request->GetEvent();

    // Check whether we're closed.
    const bool closed = (client_thread == nullptr || parent->IsClientClosed());

    Result result = ResultSuccess;
    if (!closed) {
        // If we're not closed, send the reply.
        Core::Memory::Memory& memory{kernel.System().Memory()};
        KThread* server_thread{GetCurrentThreadPointer(kernel)};
        UNIMPLEMENTED_IF(server_thread->GetOwnerProcess() != client_thread->GetOwnerProcess());

        auto* src_msg_buffer = memory.GetPointer(server_thread->GetTLSAddress());
        auto* dst_msg_buffer = memory.GetPointer(client_message);
        std::memcpy(dst_msg_buffer, src_msg_buffer, client_buffer_size);
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
        if (event != nullptr) {
            // // Get the client process/page table.
            // KProcess *client_process             = client_thread->GetOwnerProcess();
            // KPageTable *client_page_table        = &client_process->PageTable();

            // // If we need to, reply with an async error.
            // if (R_FAILED(client_result)) {
            //     ReplyAsyncError(client_process, client_message, client_buffer_size,
            //     client_result);
            // }

            // // Unlock the client buffer.
            // // NOTE: Nintendo does not check the result of this.
            // client_page_table->UnlockForIpcUserBuffer(client_message, client_buffer_size);

            // Signal the event.
            event->Signal();
        } else {
            // End the client thread's wait.
            KScopedSchedulerLock sl{kernel};

            if (!client_thread->IsTerminationRequested()) {
                client_thread->EndWait(client_result);
            }
        }
    }

    return result;
}

Result KServerSession::ReceiveRequest() {
    // Lock the session.
    KScopedLightLock lk{m_lock};

    // Get the request and client thread.
    KSessionRequest* request;
    KThread* client_thread;

    {
        KScopedSchedulerLock sl{kernel};

        // Ensure that we can service the request.
        R_UNLESS(!parent->IsClientClosed(), ResultSessionClosed);

        // Ensure we aren't already servicing a request.
        R_UNLESS(m_current_request == nullptr, ResultNotFound);

        // Ensure we have a request to service.
        R_UNLESS(!m_request_list.empty(), ResultNotFound);

        // Pop the first request from the list.
        request = &m_request_list.front();
        m_request_list.pop_front();

        // Get the thread for the request.
        client_thread = request->GetThread();
        R_UNLESS(client_thread != nullptr, ResultSessionClosed);

        // Open the client thread.
        client_thread->Open();
    }

    SCOPE_EXIT({ client_thread->Close(); });

    // Set the request as our current.
    m_current_request = request;

    // Get the client address.
    uintptr_t client_message = request->GetAddress();
    size_t client_buffer_size = request->GetSize();
    // bool recv_list_broken = false;

    // Receive the message.
    Core::Memory::Memory& memory{kernel.System().Memory()};
    KThread* server_thread{GetCurrentThreadPointer(kernel)};
    UNIMPLEMENTED_IF(server_thread->GetOwnerProcess() != client_thread->GetOwnerProcess());

    auto* src_msg_buffer = memory.GetPointer(client_message);
    auto* dst_msg_buffer = memory.GetPointer(server_thread->GetTLSAddress());
    std::memcpy(dst_msg_buffer, src_msg_buffer, client_buffer_size);

    // We succeeded.
    return ResultSuccess;
}

void KServerSession::CleanupRequests() {
    KScopedLightLock lk(m_lock);

    // Clean up any pending requests.
    while (true) {
        // Get the next request.
        KSessionRequest* request = nullptr;
        {
            KScopedSchedulerLock sl{kernel};

            if (m_current_request) {
                // Choose the current request if we have one.
                request = m_current_request;
                m_current_request = nullptr;
            } else if (!m_request_list.empty()) {
                // Pop the request from the front of the list.
                request = &m_request_list.front();
                m_request_list.pop_front();
            }
        }

        // If there's no request, we're done.
        if (request == nullptr) {
            break;
        }

        // Close a reference to the request once it's cleaned up.
        SCOPE_EXIT({ request->Close(); });

        // Extract relevant information from the request.
        // const uintptr_t client_message  = request->GetAddress();
        // const size_t client_buffer_size = request->GetSize();
        KThread* client_thread = request->GetThread();
        KEvent* event = request->GetEvent();

        // KProcess *server_process             = request->GetServerProcess();
        // KProcess *client_process             = (client_thread != nullptr) ?
        //                                         client_thread->GetOwnerProcess() : nullptr;
        // KProcessPageTable *client_page_table = (client_process != nullptr) ?
        //                                         &client_process->GetPageTable() : nullptr;

        // Cleanup the mappings.
        // Result result = CleanupMap(request, server_process, client_page_table);

        // If there's a client thread, update it.
        if (client_thread != nullptr) {
            if (event != nullptr) {
                // // We need to reply async.
                // ReplyAsyncError(client_process, client_message, client_buffer_size,
                //                 (R_SUCCEEDED(result) ? ResultSessionClosed : result));

                // // Unlock the client buffer.
                // NOTE: Nintendo does not check the result of this.
                // client_page_table->UnlockForIpcUserBuffer(client_message, client_buffer_size);

                // Signal the event.
                event->Signal();
            } else {
                // End the client thread's wait.
                KScopedSchedulerLock sl{kernel};

                if (!client_thread->IsTerminationRequested()) {
                    client_thread->EndWait(ResultSessionClosed);
                }
            }
        }
    }
}

} // namespace Kernel
