// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "core/hle/service/grc/grc.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::GRC {

class GRC final : public ServiceFramework<GRC> {
public:
    explicit GRC(Core::System& system_) : ServiceFramework{system_, "grc:c"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {1, nullptr, "OpenContinuousRecorder"},
            {2, nullptr, "OpenGameMovieTrimmer"},
            {3, nullptr, "OpenOffscreenRecorder"},
            {101, nullptr, "CreateMovieMaker"},
            {9903, nullptr, "SetOffscreenRecordingMarker"}
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<GRC>(system)->InstallAsService(sm);
}

} // namespace Service::GRC
