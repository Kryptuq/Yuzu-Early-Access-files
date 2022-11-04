// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include <boost/intrusive/list.hpp>

#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/kernel/svc_types.h"

namespace Kernel {

class KEventInfo : public KSlabAllocated<KEventInfo>, public boost::intrusive::list_base_hook<> {
public:
    struct InfoCreateThread {
        u32 thread_id{};
        uintptr_t tls_address{};
    };

    struct InfoExitProcess {
        Svc::ProcessExitReason reason{};
    };

    struct InfoExitThread {
        Svc::ThreadExitReason reason{};
    };

    struct InfoException {
        Svc::DebugException exception_type{};
        s32 exception_data_count{};
        uintptr_t exception_address{};
        std::array<uintptr_t, 4> exception_data{};
    };

    struct InfoSystemCall {
        s64 tick{};
        s32 id{};
    };

public:
    KEventInfo() = default;
    ~KEventInfo() = default;

public:
    Svc::DebugEvent event{};
    u32 thread_id{};
    u32 flags{};
    bool is_attached{};
    bool continue_flag{};
    bool ignore_continue{};
    bool close_once{};
    union {
        InfoCreateThread create_thread;
        InfoExitProcess exit_process;
        InfoExitThread exit_thread;
        InfoException exception;
        InfoSystemCall system_call;
    } info{};
    KThread* debug_thread{};
};

} // namespace Kernel
