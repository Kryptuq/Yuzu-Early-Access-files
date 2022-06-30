// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/pctl/pctl.h"
#include "core/hle/service/pctl/pctl_module.h"

namespace Service::PCTL {

namespace Error {

constexpr Result ResultNoFreeCommunication{ErrorModule::PCTL, 101};
constexpr Result ResultStereoVisionRestricted{ErrorModule::PCTL, 104};
constexpr Result ResultNoCapability{ErrorModule::PCTL, 131};
constexpr Result ResultNoRestrictionEnabled{ErrorModule::PCTL, 181};

} // namespace Error

class IParentalControlService final : public ServiceFramework<IParentalControlService> {
public:
    explicit IParentalControlService(Core::System& system_, Capability capability_)
        : ServiceFramework{system_, "IParentalControlService"}, capability{capability_} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {1, &IParentalControlService::Initialize, "Initialize"},
            {1001, &IParentalControlService::CheckFreeCommunicationPermission, "CheckFreeCommunicationPermission"},
            {1002, nullptr, "ConfirmLaunchApplicationPermission"},
            {1003, nullptr, "ConfirmResumeApplicationPermission"},
            {1004, nullptr, "ConfirmSnsPostPermission"},
            {1005, nullptr, "ConfirmSystemSettingsPermission"},
            {1006, nullptr, "IsRestrictionTemporaryUnlocked"},
            {1007, nullptr, "RevertRestrictionTemporaryUnlocked"},
            {1008, nullptr, "EnterRestrictedSystemSettings"},
            {1009, nullptr, "LeaveRestrictedSystemSettings"},
            {1010, nullptr, "IsRestrictedSystemSettingsEntered"},
            {1011, nullptr, "RevertRestrictedSystemSettingsEntered"},
            {1012, nullptr, "GetRestrictedFeatures"},
            {1013, &IParentalControlService::ConfirmStereoVisionPermission, "ConfirmStereoVisionPermission"},
            {1014, nullptr, "ConfirmPlayableApplicationVideoOld"},
            {1015, nullptr, "ConfirmPlayableApplicationVideo"},
            {1016, nullptr, "ConfirmShowNewsPermission"},
            {1017, &IParentalControlService::EndFreeCommunication, "EndFreeCommunication"},
            {1018, &IParentalControlService::IsFreeCommunicationAvailable, "IsFreeCommunicationAvailable"},
            {1031, &IParentalControlService::IsRestrictionEnabled, "IsRestrictionEnabled"},
            {1032, nullptr, "GetSafetyLevel"},
            {1033, nullptr, "SetSafetyLevel"},
            {1034, nullptr, "GetSafetyLevelSettings"},
            {1035, nullptr, "GetCurrentSettings"},
            {1036, nullptr, "SetCustomSafetyLevelSettings"},
            {1037, nullptr, "GetDefaultRatingOrganization"},
            {1038, nullptr, "SetDefaultRatingOrganization"},
            {1039, nullptr, "GetFreeCommunicationApplicationListCount"},
            {1042, nullptr, "AddToFreeCommunicationApplicationList"},
            {1043, nullptr, "DeleteSettings"},
            {1044, nullptr, "GetFreeCommunicationApplicationList"},
            {1045, nullptr, "UpdateFreeCommunicationApplicationList"},
            {1046, nullptr, "DisableFeaturesForReset"},
            {1047, nullptr, "NotifyApplicationDownloadStarted"},
            {1048, nullptr, "NotifyNetworkProfileCreated"},
            {1049, nullptr, "ResetFreeCommunicationApplicationList"},
            {1061, &IParentalControlService::ConfirmStereoVisionRestrictionConfigurable, "ConfirmStereoVisionRestrictionConfigurable"},
            {1062, &IParentalControlService::GetStereoVisionRestriction, "GetStereoVisionRestriction"},
            {1063, &IParentalControlService::SetStereoVisionRestriction, "SetStereoVisionRestriction"},
            {1064, &IParentalControlService::ResetConfirmedStereoVisionPermission, "ResetConfirmedStereoVisionPermission"},
            {1065, &IParentalControlService::IsStereoVisionPermitted, "IsStereoVisionPermitted"},
            {1201, nullptr, "UnlockRestrictionTemporarily"},
            {1202, nullptr, "UnlockSystemSettingsRestriction"},
            {1203, nullptr, "SetPinCode"},
            {1204, nullptr, "GenerateInquiryCode"},
            {1205, nullptr, "CheckMasterKey"},
            {1206, nullptr, "GetPinCodeLength"},
            {1207, nullptr, "GetPinCodeChangedEvent"},
            {1208, nullptr, "GetPinCode"},
            {1403, nullptr, "IsPairingActive"},
            {1406, nullptr, "GetSettingsLastUpdated"},
            {1411, nullptr, "GetPairingAccountInfo"},
            {1421, nullptr, "GetAccountNickname"},
            {1424, nullptr, "GetAccountState"},
            {1425, nullptr, "RequestPostEvents"},
            {1426, nullptr, "GetPostEventInterval"},
            {1427, nullptr, "SetPostEventInterval"},
            {1432, nullptr, "GetSynchronizationEvent"},
            {1451, nullptr, "StartPlayTimer"},
            {1452, nullptr, "StopPlayTimer"},
            {1453, nullptr, "IsPlayTimerEnabled"},
            {1454, nullptr, "GetPlayTimerRemainingTime"},
            {1455, nullptr, "IsRestrictedByPlayTimer"},
            {1456, nullptr, "GetPlayTimerSettings"},
            {1457, nullptr, "GetPlayTimerEventToRequestSuspension"},
            {1458, nullptr, "IsPlayTimerAlarmDisabled"},
            {1471, nullptr, "NotifyWrongPinCodeInputManyTimes"},
            {1472, nullptr, "CancelNetworkRequest"},
            {1473, nullptr, "GetUnlinkedEvent"},
            {1474, nullptr, "ClearUnlinkedEvent"},
            {1601, nullptr, "DisableAllFeatures"},
            {1602, nullptr, "PostEnableAllFeatures"},
            {1603, nullptr, "IsAllFeaturesDisabled"},
            {1901, nullptr, "DeleteFromFreeCommunicationApplicationListForDebug"},
            {1902, nullptr, "ClearFreeCommunicationApplicationListForDebug"},
            {1903, nullptr, "GetExemptApplicationListCountForDebug"},
            {1904, nullptr, "GetExemptApplicationListForDebug"},
            {1905, nullptr, "UpdateExemptApplicationListForDebug"},
            {1906, nullptr, "AddToExemptApplicationListForDebug"},
            {1907, nullptr, "DeleteFromExemptApplicationListForDebug"},
            {1908, nullptr, "ClearExemptApplicationListForDebug"},
            {1941, nullptr, "DeletePairing"},
            {1951, nullptr, "SetPlayTimerSettingsForDebug"},
            {1952, nullptr, "GetPlayTimerSpentTimeForTest"},
            {1953, nullptr, "SetPlayTimerAlarmDisabledForDebug"},
            {2001, nullptr, "RequestPairingAsync"},
            {2002, nullptr, "FinishRequestPairing"},
            {2003, nullptr, "AuthorizePairingAsync"},
            {2004, nullptr, "FinishAuthorizePairing"},
            {2005, nullptr, "RetrievePairingInfoAsync"},
            {2006, nullptr, "FinishRetrievePairingInfo"},
            {2007, nullptr, "UnlinkPairingAsync"},
            {2008, nullptr, "FinishUnlinkPairing"},
            {2009, nullptr, "GetAccountMiiImageAsync"},
            {2010, nullptr, "FinishGetAccountMiiImage"},
            {2011, nullptr, "GetAccountMiiImageContentTypeAsync"},
            {2012, nullptr, "FinishGetAccountMiiImageContentType"},
            {2013, nullptr, "SynchronizeParentalControlSettingsAsync"},
            {2014, nullptr, "FinishSynchronizeParentalControlSettings"},
            {2015, nullptr, "FinishSynchronizeParentalControlSettingsWithLastUpdated"},
            {2016, nullptr, "RequestUpdateExemptionListAsync"},
        };
        // clang-format on
        RegisterHandlers(functions);
    }

private:
    bool CheckFreeCommunicationPermissionImpl() const {
        if (states.temporary_unlocked) {
            return true;
        }
        if ((states.application_info.parental_control_flag & 1) == 0) {
            return true;
        }
        if (pin_code[0] == '\0') {
            return true;
        }
        if (!settings.is_free_communication_default_on) {
            return true;
        }
        // TODO(ogniK): Check for blacklisted/exempted applications. Return false can happen here
        // but as we don't have multiproceses support yet, we can just assume our application is
        // valid for the time being
        return true;
    }

    bool ConfirmStereoVisionPermissionImpl() const {
        if (states.temporary_unlocked) {
            return true;
        }
        if (pin_code[0] == '\0') {
            return true;
        }
        if (!settings.is_stero_vision_restricted) {
            return false;
        }
        return true;
    }

    void SetStereoVisionRestrictionImpl(bool is_restricted) {
        if (settings.disabled) {
            return;
        }

        if (pin_code[0] == '\0') {
            return;
        }
        settings.is_stero_vision_restricted = is_restricted;
    }

    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");
        IPC::ResponseBuilder rb{ctx, 2};

        if (False(capability & (Capability::Application | Capability::System))) {
            LOG_ERROR(Service_PCTL, "Invalid capability! capability={:X}", capability);
            return;
        }

        // TODO(ogniK): Recovery flag initialization for pctl:r

        const auto tid = system.GetCurrentProcessProgramID();
        if (tid != 0) {
            const FileSys::PatchManager pm{tid, system.GetFileSystemController(),
                                           system.GetContentProvider()};
            const auto control = pm.GetControlMetadata();
            if (control.first) {
                states.tid_from_event = 0;
                states.launch_time_valid = false;
                states.is_suspended = false;
                states.free_communication = false;
                states.stereo_vision = false;
                states.application_info = ApplicationInfo{
                    .tid = tid,
                    .age_rating = control.first->GetRatingAge(),
                    .parental_control_flag = control.first->GetParentalControlFlag(),
                    .capability = capability,
                };

                if (False(capability & (Capability::System | Capability::Recovery))) {
                    // TODO(ogniK): Signal application launch event
                }
            }
        }

        rb.Push(ResultSuccess);
    }

    void CheckFreeCommunicationPermission(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");

        IPC::ResponseBuilder rb{ctx, 2};
        if (!CheckFreeCommunicationPermissionImpl()) {
            rb.Push(Error::ResultNoFreeCommunication);
        } else {
            rb.Push(ResultSuccess);
        }

        states.free_communication = true;
    }

    void ConfirmStereoVisionPermission(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");
        states.stereo_vision = true;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void EndFreeCommunication(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_PCTL, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void IsFreeCommunicationAvailable(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_PCTL, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        if (!CheckFreeCommunicationPermissionImpl()) {
            rb.Push(Error::ResultNoFreeCommunication);
        } else {
            rb.Push(ResultSuccess);
        }
    }

    void IsRestrictionEnabled(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        if (False(capability & (Capability::Status | Capability::Recovery))) {
            LOG_ERROR(Service_PCTL, "Application does not have Status or Recovery capabilities!");
            rb.Push(Error::ResultNoCapability);
            rb.Push(false);
            return;
        }

        rb.Push(pin_code[0] != '\0');
    }

    void ConfirmStereoVisionRestrictionConfigurable(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");

        IPC::ResponseBuilder rb{ctx, 2};

        if (False(capability & Capability::StereoVision)) {
            LOG_ERROR(Service_PCTL, "Application does not have StereoVision capability!");
            rb.Push(Error::ResultNoCapability);
            return;
        }

        if (pin_code[0] == '\0') {
            rb.Push(Error::ResultNoRestrictionEnabled);
            return;
        }

        rb.Push(ResultSuccess);
    }

    void IsStereoVisionPermitted(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        if (!ConfirmStereoVisionPermissionImpl()) {
            rb.Push(Error::ResultStereoVisionRestricted);
            rb.Push(false);
        } else {
            rb.Push(ResultSuccess);
            rb.Push(true);
        }
    }

    void SetStereoVisionRestriction(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto can_use = rp.Pop<bool>();
        LOG_DEBUG(Service_PCTL, "called, can_use={}", can_use);

        IPC::ResponseBuilder rb{ctx, 2};
        if (False(capability & Capability::StereoVision)) {
            LOG_ERROR(Service_PCTL, "Application does not have StereoVision capability!");
            rb.Push(Error::ResultNoCapability);
            return;
        }

        SetStereoVisionRestrictionImpl(can_use);
        rb.Push(ResultSuccess);
    }

    void GetStereoVisionRestriction(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        if (False(capability & Capability::StereoVision)) {
            LOG_ERROR(Service_PCTL, "Application does not have StereoVision capability!");
            rb.Push(Error::ResultNoCapability);
            rb.Push(false);
            return;
        }

        rb.Push(ResultSuccess);
        rb.Push(settings.is_stero_vision_restricted);
    }

    void ResetConfirmedStereoVisionPermission(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PCTL, "called");

        states.stereo_vision = false;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    struct ApplicationInfo {
        u64 tid{};
        std::array<u8, 32> age_rating{};
        u32 parental_control_flag{};
        Capability capability{};
    };

    struct States {
        u64 current_tid{};
        ApplicationInfo application_info{};
        u64 tid_from_event{};
        bool launch_time_valid{};
        bool is_suspended{};
        bool temporary_unlocked{};
        bool free_communication{};
        bool stereo_vision{};
    };

    struct ParentalControlSettings {
        bool is_stero_vision_restricted{};
        bool is_free_communication_default_on{};
        bool disabled{};
    };

    States states{};
    ParentalControlSettings settings{};
    std::array<char, 8> pin_code{};
    Capability capability{};
};

void Module::Interface::CreateService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_PCTL, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    // TODO(ogniK): Get TID from process

    rb.PushIpcInterface<IParentalControlService>(system, capability);
}

void Module::Interface::CreateServiceWithoutInitialize(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_PCTL, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IParentalControlService>(system, capability);
}

Module::Interface::Interface(Core::System& system_, std::shared_ptr<Module> module_,
                             const char* name_, Capability capability_)
    : ServiceFramework{system_, name_}, module{std::move(module_)}, capability{capability_} {}

Module::Interface::~Interface() = default;

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    auto module = std::make_shared<Module>();
    std::make_shared<PCTL>(system, module, "pctl",
                           Capability::Application | Capability::SnsPost | Capability::Status |
                               Capability::StereoVision)
        ->InstallAsService(service_manager);
    // TODO(ogniK): Implement remaining capabilities
    std::make_shared<PCTL>(system, module, "pctl:a", Capability::None)
        ->InstallAsService(service_manager);
    std::make_shared<PCTL>(system, module, "pctl:r", Capability::None)
        ->InstallAsService(service_manager);
    std::make_shared<PCTL>(system, module, "pctl:s", Capability::None)
        ->InstallAsService(service_manager);
}

} // namespace Service::PCTL
