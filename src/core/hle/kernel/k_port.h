// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/common_types.h"
#include "core/hle/kernel/k_client_port.h"
#include "core/hle/kernel/k_server_port.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Kernel {

class KServerSession;

class KPort final : public KAutoObjectWithSlabHeapAndContainer<KPort, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KPort, KAutoObject);

public:
    explicit KPort(KernelCore& kernel_);
    ~KPort() override;

    static void PostDestroy([[maybe_unused]] uintptr_t arg) {}

    void Initialize(s32 max_sessions_, bool is_light_, const std::string& name_);
    void OnClientClosed();
    void OnServerClosed();

    bool IsLight() const {
        return is_light;
    }

    bool IsServerClosed() const;

    ResultCode EnqueueSession(KServerSession* session);

    KClientPort& GetClientPort() {
        return client;
    }
    KServerPort& GetServerPort() {
        return server;
    }
    const KClientPort& GetClientPort() const {
        return client;
    }
    const KServerPort& GetServerPort() const {
        return server;
    }

private:
    enum class State : u8 {
        Invalid = 0,
        Normal = 1,
        ClientClosed = 2,
        ServerClosed = 3,
    };

    KServerPort server;
    KClientPort client;
    State state{State::Invalid};
    bool is_light{};
};

} // namespace Kernel
