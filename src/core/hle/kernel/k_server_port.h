// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <utility>

#include <boost/intrusive/list.hpp>

#include "core/hle/kernel/k_server_session.h"
#include "core/hle/kernel/k_synchronization_object.h"

namespace Kernel {

class KernelCore;
class KPort;
class SessionRequestHandler;

class KServerPort final : public KSynchronizationObject {
    KERNEL_AUTOOBJECT_TRAITS(KServerPort, KSynchronizationObject);

public:
    explicit KServerPort(KernelCore& kernel_);
    ~KServerPort() override;

    void Initialize(KPort* parent_port_, std::string&& name_);

    /// Whether or not this server port has an HLE handler available.
    bool HasSessionRequestHandler() const {
        return !session_handler.expired();
    }

    /// Gets the HLE handler for this port.
    SessionRequestHandlerWeakPtr GetSessionRequestHandler() const {
        return session_handler;
    }

    /**
     * Sets the HLE handler template for the port. ServerSessions crated by connecting to this port
     * will inherit a reference to this handler.
     */
    void SetSessionHandler(SessionRequestHandlerWeakPtr&& handler) {
        session_handler = std::move(handler);
    }

    void EnqueueSession(KServerSession* pending_session);

    KServerSession* AcceptSession();

    const KPort* GetParent() const {
        return parent;
    }

    bool IsLight() const;

    // Overridden virtual functions.
    void Destroy() override;
    bool IsSignaled() const override;

private:
    using SessionList = boost::intrusive::list<KServerSession>;

    void CleanupSessions();

    SessionList session_list;
    SessionRequestHandlerWeakPtr session_handler;
    KPort* parent{};
};

} // namespace Kernel
