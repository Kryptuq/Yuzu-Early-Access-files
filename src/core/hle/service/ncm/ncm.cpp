// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/file_sys/romfs_factory.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/ncm/ncm.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::NCM {

class ILocationResolver final : public ServiceFramework<ILocationResolver> {
public:
    explicit ILocationResolver(Core::System& system_, FileSys::StorageId id)
        : ServiceFramework{system_, "ILocationResolver"}, storage{id} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ResolveProgramPath"},
            {1, nullptr, "RedirectProgramPath"},
            {2, nullptr, "ResolveApplicationControlPath"},
            {3, nullptr, "ResolveApplicationHtmlDocumentPath"},
            {4, nullptr, "ResolveDataPath"},
            {5, nullptr, "RedirectApplicationControlPath"},
            {6, nullptr, "RedirectApplicationHtmlDocumentPath"},
            {7, nullptr, "ResolveApplicationLegalInformationPath"},
            {8, nullptr, "RedirectApplicationLegalInformationPath"},
            {9, nullptr, "Refresh"},
            {10, nullptr, "RedirectApplicationProgramPath"},
            {11, nullptr, "ClearApplicationRedirection"},
            {12, nullptr, "EraseProgramRedirection"},
            {13, nullptr, "EraseApplicationControlRedirection"},
            {14, nullptr, "EraseApplicationHtmlDocumentRedirection"},
            {15, nullptr, "EraseApplicationLegalInformationRedirection"},
            {16, nullptr, "ResolveProgramPathForDebug"},
            {17, nullptr, "RedirectProgramPathForDebug"},
            {18, nullptr, "RedirectApplicationProgramPathForDebug"},
            {19, nullptr, "EraseProgramRedirectionForDebug"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    [[maybe_unused]] FileSys::StorageId storage;
};

class IRegisteredLocationResolver final : public ServiceFramework<IRegisteredLocationResolver> {
public:
    explicit IRegisteredLocationResolver(Core::System& system_)
        : ServiceFramework{system_, "IRegisteredLocationResolver"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ResolveProgramPath"},
            {1, nullptr, "RegisterProgramPath"},
            {2, nullptr, "UnregisterProgramPath"},
            {3, nullptr, "RedirectProgramPath"},
            {4, nullptr, "ResolveHtmlDocumentPath"},
            {5, nullptr, "RegisterHtmlDocumentPath"},
            {6, nullptr, "UnregisterHtmlDocumentPath"},
            {7, nullptr, "RedirectHtmlDocumentPath"},
            {8, nullptr, "Refresh"},
            {9, nullptr, "RefreshExcluding"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IAddOnContentLocationResolver final : public ServiceFramework<IAddOnContentLocationResolver> {
public:
    explicit IAddOnContentLocationResolver(Core::System& system_)
        : ServiceFramework{system_, "IAddOnContentLocationResolver"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ResolveAddOnContentPath"},
            {1, nullptr, "RegisterAddOnContentStorage"},
            {2, nullptr, "UnregisterAllAddOnContentPath"},
            {3, nullptr, "RefreshApplicationAddOnContent"},
            {4, nullptr, "UnregisterApplicationAddOnContent"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class LR final : public ServiceFramework<LR> {
public:
    explicit LR(Core::System& system_) : ServiceFramework{system_, "lr"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "OpenLocationResolver"},
            {1, nullptr, "OpenRegisteredLocationResolver"},
            {2, nullptr, "RefreshLocationResolver"},
            {3, nullptr, "OpenAddOnContentLocationResolver"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class NCM final : public ServiceFramework<NCM> {
public:
    explicit NCM(Core::System& system_) : ServiceFramework{system_, "ncm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "CreateContentStorage"},
            {1, nullptr, "CreateContentMetaDatabase"},
            {2, nullptr, "VerifyContentStorage"},
            {3, nullptr, "VerifyContentMetaDatabase"},
            {4, nullptr, "OpenContentStorage"},
            {5, nullptr, "OpenContentMetaDatabase"},
            {6, nullptr, "CloseContentStorageForcibly"},
            {7, nullptr, "CloseContentMetaDatabaseForcibly"},
            {8, nullptr, "CleanupContentMetaDatabase"},
            {9, nullptr, "ActivateContentStorage"},
            {10, nullptr, "InactivateContentStorage"},
            {11, nullptr, "ActivateContentMetaDatabase"},
            {12, nullptr, "InactivateContentMetaDatabase"},
            {13, nullptr, "InvalidateRightsIdCache"},
            {14, nullptr, "GetMemoryReport"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("lr", std::make_shared<LR>(system));
    server_manager->RegisterNamedService("ncm", std::make_shared<NCM>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::NCM
