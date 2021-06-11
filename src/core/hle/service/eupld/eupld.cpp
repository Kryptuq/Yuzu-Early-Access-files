// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "core/hle/service/eupld/eupld.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::EUPLD {

class ErrorUploadContext final : public ServiceFramework<ErrorUploadContext> {
public:
    explicit ErrorUploadContext(Core::System& system_) : ServiceFramework{system_, "eupld:c"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "SetUrl"},
            {1, nullptr, "ImportCrt"},
            {2, nullptr, "ImportPki"},
            {3, nullptr, "SetAutoUpload"},
            {4, nullptr, "GetAutoUpload"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class ErrorUploadRequest final : public ServiceFramework<ErrorUploadRequest> {
public:
    explicit ErrorUploadRequest(Core::System& system_) : ServiceFramework{system_, "eupld:r"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "UploadAll"},
            {2, nullptr, "UploadSelected"},
            {3, nullptr, "GetUploadStatus"},
            {4, nullptr, "CancelUpload"},
            {5, nullptr, "GetResult"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<ErrorUploadContext>(system)->InstallAsService(sm);
    std::make_shared<ErrorUploadRequest>(system)->InstallAsService(sm);
}

} // namespace Service::EUPLD
