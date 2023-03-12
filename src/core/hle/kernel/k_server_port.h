// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    explicit KServerPort(KernelCore& kernel);
    ~KServerPort() override;

    void Initialize(KPort* parent);

    void EnqueueSession(KServerSession* session);

    KServerSession* AcceptSession();

    const KPort* GetParent() const {
        return m_parent;
    }

    bool IsLight() const;

    // Overridden virtual functions.
    void Destroy() override;
    bool IsSignaled() const override;

private:
    using SessionList = boost::intrusive::list<KServerSession>;

    void CleanupSessions();

    SessionList m_session_list{};
    KPort* m_parent{};
};

} // namespace Kernel
