// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string_view>
#include <vector>

#include "common/polyfill_thread.h"
#include "core/hle/result.h"
#include "core/hle/service/mutex.h"

namespace Core {
class System;
}

namespace Kernel {
class KEvent;
class KServerPort;
class KServerSession;
class KSynchronizationObject;
class SessionRequestHandler;
class SessionRequestManager;
} // namespace Kernel

namespace Service {

class ServerManager {
public:
    explicit ServerManager(Core::System& system);
    ~ServerManager();

    Result RegisterSession(Kernel::KServerSession* session,
                           std::shared_ptr<Kernel::SessionRequestManager> manager);
    Result RegisterNamedService(const std::string& service_name,
                                std::shared_ptr<Kernel::SessionRequestHandler>&& handler,
                                u32 max_sessions = 64);
    Result ManageNamedPort(const std::string& service_name,
                           std::shared_ptr<Kernel::SessionRequestHandler>&& handler,
                           u32 max_sessions = 64);

    Result LoopProcess();
    void StartAdditionalHostThreads(const char* name, size_t num_threads);

    static void RunServer(std::unique_ptr<ServerManager>&& server);

private:
    Result LoopProcessImpl();
    Result WaitAndProcessImpl();
    Result OnPortEvent(Kernel::KServerPort* port,
                       std::shared_ptr<Kernel::SessionRequestHandler>&& handler);
    Result OnSessionEvent(Kernel::KServerSession* session,
                          std::shared_ptr<Kernel::SessionRequestManager>&& manager);

private:
    Core::System& m_system;
    Mutex m_serve_mutex;
    std::mutex m_list_mutex;

    // Guest state tracking
    std::map<Kernel::KServerPort*, std::shared_ptr<Kernel::SessionRequestHandler>> m_ports{};
    std::map<Kernel::KServerSession*, std::shared_ptr<Kernel::SessionRequestManager>> m_sessions{};
    Kernel::KEvent* m_event{};

    // Host state tracking
    std::atomic<bool> m_stopped{};
    std::vector<std::jthread> m_threads{};
    std::stop_source m_stop_source{};
};

} // namespace Service
