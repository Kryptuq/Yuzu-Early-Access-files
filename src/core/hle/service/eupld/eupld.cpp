// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/service/eupld/eupld.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

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

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("eupld:c", std::make_shared<ErrorUploadContext>(system));
    server_manager->RegisterNamedService("eupld:r", std::make_shared<ErrorUploadRequest>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::EUPLD
