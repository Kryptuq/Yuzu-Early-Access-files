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
    explicit KServerPort(KernelCore& kernel_);
    ~KServerPort() override;

    void Initialize(KPort* parent_port_, std::string&& name_);

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
    KPort* parent{};
};

} // namespace Kernel
