// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "core/hle/service/bpc/bpc.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::BPC {

class BPC final : public ServiceFramework<BPC> {
public:
    explicit BPC(Core::System& system_) : ServiceFramework{system_, "bpc"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ShutdownSystem"},
            {1, nullptr, "RebootSystem"},
            {2, nullptr, "GetWakeupReason"},
            {3, nullptr, "GetShutdownReason"},
            {4, nullptr, "GetAcOk"},
            {5, nullptr, "GetBoardPowerControlEvent"},
            {6, nullptr, "GetSleepButtonState"},
            {7, nullptr, "GetPowerEvent"},
            {8, nullptr, "CreateWakeupTimer"},
            {9, nullptr, "CancelWakeupTimer"},
            {10, nullptr, "EnableWakeupTimerOnDevice"},
            {11, nullptr, "CreateWakeupTimerEx"},
            {12, nullptr, "GetLastEnabledWakeupTimerType"},
            {13, nullptr, "CleanAllWakeupTimers"},
            {14, nullptr, "GetPowerButton"},
            {15, nullptr, "SetEnableWakeupTimer"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class BPC_R final : public ServiceFramework<BPC_R> {
public:
    explicit BPC_R(Core::System& system_) : ServiceFramework{system_, "bpc:r"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetRtcTime"},
            {1, nullptr, "SetRtcTime"},
            {2, nullptr, "GetRtcResetDetected"},
            {3, nullptr, "ClearRtcResetDetected"},
            {4, nullptr, "SetUpRtcResetOnShutdown"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<BPC>(system)->InstallAsService(sm);
    std::make_shared<BPC_R>(system)->InstallAsService(sm);
}

} // namespace Service::BPC
