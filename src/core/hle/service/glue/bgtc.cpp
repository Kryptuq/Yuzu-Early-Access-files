// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/glue/bgtc.h"

namespace Service::Glue {

BGTC_T::BGTC_T(Core::System& system_) : ServiceFramework{system_, "bgtc:t"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {100, &BGTC_T::OpenTaskService, "OpenTaskService"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

BGTC_T::~BGTC_T() = default;

void BGTC_T::OpenTaskService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_BGTC, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ITaskService>(system);
}

ITaskService::ITaskService(Core::System& system_) : ServiceFramework{system_, "ITaskService"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {1, nullptr, "NotifyTaskStarting"},
        {2, nullptr, "NotifyTaskFinished"},
        {3, nullptr, "GetTriggerEvent"},
        {4, nullptr, "IsInHalfAwake"},
        {5, nullptr, "NotifyClientName"},
        {6, nullptr, "IsInFullAwake"},
        {11, nullptr, "ScheduleTask"},
        {12, nullptr, "GetScheduledTaskInterval"},
        {13, nullptr, "UnscheduleTask"},
        {14, nullptr, "GetScheduleEvent"},
        {15, nullptr, "SchedulePeriodicTask"},
        {16, nullptr, "Unknown16"},
        {101, nullptr, "GetOperationMode"},
        {102, nullptr, "WillDisconnectNetworkWhenEnteringSleep"},
        {103, nullptr, "WillStayHalfAwakeInsteadSleep"},
        {200, nullptr, "Unknown200"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ITaskService::~ITaskService() = default;

BGTC_SC::BGTC_SC(Core::System& system_) : ServiceFramework{system_, "bgtc:sc"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {1, nullptr, "GetState"},
        {2, nullptr, "GetStateChangedEvent"},
        {3, nullptr, "NotifyEnteringHalfAwake"},
        {4, nullptr, "NotifyLeavingHalfAwake"},
        {5, nullptr, "SetIsUsingSleepUnsupportedDevices"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

BGTC_SC::~BGTC_SC() = default;

} // namespace Service::Glue
