// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/olsc/olsc.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::OLSC {

class OLSC final : public ServiceFramework<OLSC> {
public:
    explicit OLSC(Core::System& system_) : ServiceFramework{system_, "olsc:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &OLSC::Initialize, "Initialize"},
            {10, nullptr, "VerifySaveDataBackupLicenseAsync"},
            {13, &OLSC::GetSaveDataBackupSetting, "GetSaveDataBackupSetting"},
            {14, &OLSC::SetSaveDataBackupSettingEnabled, "SetSaveDataBackupSettingEnabled"},
            {15, nullptr, "SetCustomData"},
            {16, nullptr, "DeleteSaveDataBackupSetting"},
            {18, nullptr, "GetSaveDataBackupInfoCache"},
            {19, nullptr, "UpdateSaveDataBackupInfoCacheAsync"},
            {22, nullptr, "DeleteSaveDataBackupAsync"},
            {25, nullptr, "ListDownloadableSaveDataBackupInfoAsync"},
            {26, nullptr, "DownloadSaveDataBackupAsync"},
            {27, nullptr, "UploadSaveDataBackupAsync"},
            {9010, nullptr, "VerifySaveDataBackupLicenseAsyncForDebug"},
            {9013, nullptr, "GetSaveDataBackupSettingForDebug"},
            {9014, nullptr, "SetSaveDataBackupSettingEnabledForDebug"},
            {9015, nullptr, "SetCustomDataForDebug"},
            {9016, nullptr, "DeleteSaveDataBackupSettingForDebug"},
            {9018, nullptr, "GetSaveDataBackupInfoCacheForDebug"},
            {9019, nullptr, "UpdateSaveDataBackupInfoCacheAsyncForDebug"},
            {9022, nullptr, "DeleteSaveDataBackupAsyncForDebug"},
            {9025, nullptr, "ListDownloadableSaveDataBackupInfoAsyncForDebug"},
            {9026, nullptr, "DownloadSaveDataBackupAsyncForDebug"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_OLSC, "(STUBBED) called");

        initialized = true;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetSaveDataBackupSetting(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_OLSC, "(STUBBED) called");

        // backup_setting is set to 0 since real value is unknown
        constexpr u64 backup_setting = 0;

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(backup_setting);
    }

    void SetSaveDataBackupSettingEnabled(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_OLSC, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    bool initialized{};
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<OLSC>(system)->InstallAsService(service_manager);
}

} // namespace Service::OLSC
