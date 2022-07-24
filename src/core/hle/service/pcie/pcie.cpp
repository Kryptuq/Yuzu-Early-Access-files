// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/service/pcie/pcie.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::PCIe {

class ISession final : public ServiceFramework<ISession> {
public:
    explicit ISession(Core::System& system_) : ServiceFramework{system_, "ISession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "QueryFunctions"},
            {1, nullptr, "AcquireFunction"},
            {2, nullptr, "ReleaseFunction"},
            {3, nullptr, "GetFunctionState"},
            {4, nullptr, "GetBarProfile"},
            {5, nullptr, "ReadConfig"},
            {6, nullptr, "WriteConfig"},
            {7, nullptr, "ReadBarRegion"},
            {8, nullptr, "WriteBarRegion"},
            {9, nullptr, "FindCapability"},
            {10, nullptr, "FindExtendedCapability"},
            {11, nullptr, "MapDma"},
            {12, nullptr, "UnmapDma"},
            {13, nullptr, "UnmapDmaBusAddress"},
            {14, nullptr, "GetDmaBusAddress"},
            {15, nullptr, "GetDmaBusAddressRange"},
            {16, nullptr, "SetDmaEnable"},
            {17, nullptr, "AcquireIrq"},
            {18, nullptr, "ReleaseIrq"},
            {19, nullptr, "SetIrqEnable"},
            {20, nullptr, "SetAspmEnable"},
            {21, nullptr, "SetResetUponResumeEnable"},
            {22, nullptr, "ResetFunction"},
            {23, nullptr, "Unknown23"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class PCIe final : public ServiceFramework<PCIe> {
public:
    explicit PCIe(Core::System& system_) : ServiceFramework{system_, "pcie"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "RegisterClassDriver"},
            {1, nullptr, "QueryFunctionsUnregistered"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<PCIe>(system)->InstallAsService(sm);
}

} // namespace Service::PCIe
