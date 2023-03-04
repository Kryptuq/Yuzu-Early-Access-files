// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "common/concepts.h"
#include "core/hle/kernel/k_port.h"
#include "core/hle/result.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class KClientPort;
class KClientSession;
class KernelCore;
class KPort;
class SessionRequestHandler;
} // namespace Kernel

namespace Service::SM {

class Controller;

/// Interface to "sm:" service
class SM final : public ServiceFramework<SM> {
public:
    explicit SM(ServiceManager& service_manager_, Core::System& system_);
    ~SM() override;

private:
    void Initialize(HLERequestContext& ctx);
    void GetService(HLERequestContext& ctx);
    void GetServiceTipc(HLERequestContext& ctx);
    void RegisterService(HLERequestContext& ctx);
    void UnregisterService(HLERequestContext& ctx);

    ResultVal<Kernel::KClientSession*> GetServiceImpl(HLERequestContext& ctx);

    ServiceManager& service_manager;
    Kernel::KernelCore& kernel;
};

class ServiceManager {
public:
    explicit ServiceManager(Kernel::KernelCore& kernel_);
    ~ServiceManager();

    Result RegisterService(std::string name, u32 max_sessions, SessionRequestHandlerPtr handler);
    Result UnregisterService(const std::string& name);
    ResultVal<Kernel::KPort*> GetServicePort(const std::string& name);

    template <Common::DerivedFrom<SessionRequestHandler> T>
    std::shared_ptr<T> GetService(const std::string& service_name) const {
        auto service = registered_services.find(service_name);
        if (service == registered_services.end()) {
            LOG_DEBUG(Service, "Can't find service: {}", service_name);
            return nullptr;
        }
        return std::static_pointer_cast<T>(service->second);
    }

    void InvokeControlRequest(HLERequestContext& context);

    void SetDeferralEvent(Kernel::KEvent* deferral_event_) {
        deferral_event = deferral_event_;
    }

private:
    std::shared_ptr<SM> sm_interface;
    std::unique_ptr<Controller> controller_interface;

    /// Map of registered services, retrieved using GetServicePort.
    std::mutex lock;
    std::unordered_map<std::string, SessionRequestHandlerPtr> registered_services;
    std::unordered_map<std::string, Kernel::KPort*> service_ports;

    /// Kernel context
    Kernel::KernelCore& kernel;
    Kernel::KEvent* deferral_event{};
};

/// Runs SM services.
void LoopProcess(Core::System& system);

} // namespace Service::SM
