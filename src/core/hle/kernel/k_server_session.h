// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>
#include <memory>
#include <string>
#include <utility>

#include <boost/intrusive/list.hpp>

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_session_request.h"
#include "core/hle/kernel/k_synchronization_object.h"
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

    /// Gets the session request manager, which forwards requests to the underlying service
    std::shared_ptr<SessionRequestManager>& GetSessionRequestManager() {
        return manager;
    }

    /// TODO: flesh these out to match the real kernel
    Result OnRequest(KSessionRequest* request);
    Result SendReply();
    Result ReceiveRequest();

private:
    /// Frees up waiting client sessions when this server session is about to die
    void CleanupRequests();

    /// Queues a sync request from the emulated application.
    Result QueueSyncRequest(KThread* thread, Core::Memory::Memory& memory);

    /// Completes a sync request from the emulated application.
    Result CompleteSyncRequest(HLERequestContext& context);

    /// This session's HLE request handlers; if nullptr, this is not an HLE server
    std::shared_ptr<SessionRequestManager> manager;

    /// When set to True, converts the session to a domain at the end of the command
    bool convert_to_domain{};

    /// KSession that owns this KServerSession
    KSession* parent{};

    /// List of threads which are pending a reply.
    boost::intrusive::list<KSessionRequest> m_request_list;
    KSessionRequest* m_current_request{};

    KLightLock m_lock;
};

} // namespace Kernel
