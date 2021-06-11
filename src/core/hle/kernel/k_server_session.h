// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/intrusive/list.hpp>

#include "common/threadsafe_queue.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/service_thread.h"
#include "core/hle/result.h"

namespace Core::Memory {
class Memory;
}

namespace Core::Timing {
class CoreTiming;
struct EventType;
} // namespace Core::Timing

namespace Kernel {

class HLERequestContext;
class KernelCore;
class KSession;
class SessionRequestHandler;
class SessionRequestManager;
class KThread;

class KServerSession final : public KSynchronizationObject,
                             public boost::intrusive::list_base_hook<> {
    KERNEL_AUTOOBJECT_TRAITS(KServerSession, KSynchronizationObject);

    friend class ServiceThread;

public:
    explicit KServerSession(KernelCore& kernel_);
    ~KServerSession() override;

    void Destroy() override;

    void Initialize(KSession* parent_session_, std::string&& name_,
                    std::shared_ptr<SessionRequestManager> manager_);

    KSession* GetParent() {
        return parent;
    }

    const KSession* GetParent() const {
        return parent;
    }

    bool IsSignaled() const override;

    void OnClientClosed();

    void ClientConnected(SessionRequestHandlerPtr handler) {
        manager->SetSessionHandler(std::move(handler));
    }

    void ClientDisconnected() {
        manager = nullptr;
    }

    /**
     * Handle a sync request from the emulated application.
     *
     * @param thread      Thread that initiated the request.
     * @param memory      Memory context to handle the sync request under.
     * @param core_timing Core timing context to schedule the request event under.
     *
     * @returns ResultCode from the operation.
     */
    ResultCode HandleSyncRequest(KThread* thread, Core::Memory::Memory& memory,
                                 Core::Timing::CoreTiming& core_timing);

    /// Adds a new domain request handler to the collection of request handlers within
    /// this ServerSession instance.
    void AppendDomainHandler(SessionRequestHandlerPtr handler);

    /// Retrieves the total number of domain request handlers that have been
    /// appended to this ServerSession instance.
    std::size_t NumDomainRequestHandlers() const;

    /// Returns true if the session has been converted to a domain, otherwise False
    bool IsDomain() const {
        return manager->IsDomain();
    }

    /// Converts the session to a domain at the end of the current command
    void ConvertToDomain() {
        convert_to_domain = true;
    }

    /// Gets the session request manager, which forwards requests to the underlying service
    std::shared_ptr<SessionRequestManager>& GetSessionRequestManager() {
        return manager;
    }

private:
    /// Queues a sync request from the emulated application.
    ResultCode QueueSyncRequest(KThread* thread, Core::Memory::Memory& memory);

    /// Completes a sync request from the emulated application.
    ResultCode CompleteSyncRequest(HLERequestContext& context);

    /// Handles a SyncRequest to a domain, forwarding the request to the proper object or closing an
    /// object handle.
    ResultCode HandleDomainSyncRequest(Kernel::HLERequestContext& context);

    /// This session's HLE request handlers
    std::shared_ptr<SessionRequestManager> manager;

    /// When set to True, converts the session to a domain at the end of the command
    bool convert_to_domain{};

    /// KSession that owns this KServerSession
    KSession* parent{};
};

} // namespace Kernel
