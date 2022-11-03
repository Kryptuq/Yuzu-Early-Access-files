// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
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

namespace Kernel {

class HLERequestContext;
class KernelCore;
class KSession;
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

    void Initialize(KSession* parent_session_, std::string&& name_);

    KSession* GetParent() {
        return parent;
    }

    const KSession* GetParent() const {
        return parent;
    }

    bool IsSignaled() const override;
    void OnClientClosed();

    /// TODO: flesh these out to match the real kernel
    Result OnRequest(KSessionRequest* request);
    Result SendReply(bool is_hle = false);
    Result ReceiveRequest(std::shared_ptr<HLERequestContext>* out_context = nullptr,
                          std::weak_ptr<SessionRequestManager> manager = {});

    Result SendReplyHLE() {
        return SendReply(true);
    }

private:
    /// Frees up waiting client sessions when this server session is about to die
    void CleanupRequests();

    /// KSession that owns this KServerSession
    KSession* parent{};

    /// List of threads which are pending a reply.
    boost::intrusive::list<KSessionRequest> m_request_list;
    KSessionRequest* m_current_request{};

    KLightLock m_lock;
};

} // namespace Kernel
