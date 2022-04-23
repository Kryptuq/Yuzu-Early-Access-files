// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/service/npns/npns.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::NPNS {

class NPNS_S final : public ServiceFramework<NPNS_S> {
public:
    explicit NPNS_S(Core::System& system_) : ServiceFramework{system_, "npns:s"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {1, nullptr, "ListenAll"},
            {2, nullptr, "ListenTo"},
            {3, nullptr, "Receive"},
            {4, nullptr, "ReceiveRaw"},
            {5, nullptr, "GetReceiveEvent"},
            {6, nullptr, "ListenUndelivered"},
            {7, nullptr, "GetStateChangeEVent"},
            {11, nullptr, "SubscribeTopic"},
            {12, nullptr, "UnsubscribeTopic"},
            {13, nullptr, "QueryIsTopicExist"},
            {21, nullptr, "CreateToken"},
            {22, nullptr, "CreateTokenWithApplicationId"},
            {23, nullptr, "DestroyToken"},
            {24, nullptr, "DestroyTokenWithApplicationId"},
            {25, nullptr, "QueryIsTokenValid"},
            {26, nullptr, "ListenToMyApplicationId"},
            {27, nullptr, "DestroyTokenAll"},
            {31, nullptr, "UploadTokenToBaaS"},
            {32, nullptr, "DestroyTokenForBaaS"},
            {33, nullptr, "CreateTokenForBaaS"},
            {34, nullptr, "SetBaaSDeviceAccountIdList"},
            {101, nullptr, "Suspend"},
            {102, nullptr, "Resume"},
            {103, nullptr, "GetState"},
            {104, nullptr, "GetStatistics"},
            {105, nullptr, "GetPlayReportRequestEvent"},
            {111, nullptr, "GetJid"},
            {112, nullptr, "CreateJid"},
            {113, nullptr, "DestroyJid"},
            {114, nullptr, "AttachJid"},
            {115, nullptr, "DetachJid"},
            {120, nullptr, "CreateNotificationReceiver"},
            {151, nullptr, "GetStateWithHandover"},
            {152, nullptr, "GetStateChangeEventWithHandover"},
            {153, nullptr, "GetDropEventWithHandover"},
            {154, nullptr, "CreateTokenAsync"},
            {155, nullptr, "CreateTokenAsyncWithApplicationId"},
            {161, nullptr, "GetRequestChangeStateCancelEvent"},
            {162, nullptr, "RequestChangeStateForceTimedWithCancelEvent"},
            {201, nullptr, "RequestChangeStateForceTimed"},
            {202, nullptr, "RequestChangeStateForceAsync"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class NPNS_U final : public ServiceFramework<NPNS_U> {
public:
    explicit NPNS_U(Core::System& system_) : ServiceFramework{system_, "npns:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {1, nullptr, "ListenAll"},
            {2, nullptr, "ListenTo"},
            {3, nullptr, "Receive"},
            {4, nullptr, "ReceiveRaw"},
            {5, nullptr, "GetReceiveEvent"},
            {7, nullptr, "GetStateChangeEVent"},
            {21, nullptr, "CreateToken"},
            {23, nullptr, "DestroyToken"},
            {25, nullptr, "QueryIsTokenValid"},
            {26, nullptr, "ListenToMyApplicationId"},
            {101, nullptr, "Suspend"},
            {102, nullptr, "Resume"},
            {103, nullptr, "GetState"},
            {104, nullptr, "GetStatistics"},
            {111, nullptr, "GetJid"},
            {120, nullptr, "CreateNotificationReceiver"},
            {151, nullptr, "GetStateWithHandover"},
            {152, nullptr, "GetStateChangeEventWithHandover"},
            {153, nullptr, "GetDropEventWithHandover"},
            {154, nullptr, "CreateTokenAsync"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<NPNS_S>(system)->InstallAsService(sm);
    std::make_shared<NPNS_U>(system)->InstallAsService(sm);
}

} // namespace Service::NPNS
