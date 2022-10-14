// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/vfs.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/glue/glue_manager.h"
#include "core/hle/service/ns/errors.h"
#include "core/hle/service/ns/iplatform_service_manager.h"
#include "core/hle/service/ns/language.h"
#include "core/hle/service/ns/ns.h"
#include "core/hle/service/ns/pdm_qry.h"
#include "core/hle/service/set/set.h"

namespace Service::NS {

IAccountProxyInterface::IAccountProxyInterface(Core::System& system_)
    : ServiceFramework{system_, "IAccountProxyInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CreateUserAccount"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IAccountProxyInterface::~IAccountProxyInterface() = default;

IApplicationManagerInterface::IApplicationManagerInterface(Core::System& system_)
    : ServiceFramework{system_, "IApplicationManagerInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "ListApplicationRecord"},
        {1, nullptr, "GenerateApplicationRecordCount"},
        {2, nullptr, "GetApplicationRecordUpdateSystemEvent"},
        {3, nullptr, "GetApplicationViewDeprecated"},
        {4, nullptr, "DeleteApplicationEntity"},
        {5, nullptr, "DeleteApplicationCompletely"},
        {6, nullptr, "IsAnyApplicationEntityRedundant"},
        {7, nullptr, "DeleteRedundantApplicationEntity"},
        {8, nullptr, "IsApplicationEntityMovable"},
        {9, nullptr, "MoveApplicationEntity"},
        {11, nullptr, "CalculateApplicationOccupiedSize"},
        {16, nullptr, "PushApplicationRecord"},
        {17, nullptr, "ListApplicationRecordContentMeta"},
        {19, nullptr, "LaunchApplicationOld"},
        {21, nullptr, "GetApplicationContentPath"},
        {22, nullptr, "TerminateApplication"},
        {23, nullptr, "ResolveApplicationContentPath"},
        {26, nullptr, "BeginInstallApplication"},
        {27, nullptr, "DeleteApplicationRecord"},
        {30, nullptr, "RequestApplicationUpdateInfo"},
        {31, nullptr, "Unknown31"},
        {32, nullptr, "CancelApplicationDownload"},
        {33, nullptr, "ResumeApplicationDownload"},
        {35, nullptr, "UpdateVersionList"},
        {36, nullptr, "PushLaunchVersion"},
        {37, nullptr, "ListRequiredVersion"},
        {38, nullptr, "CheckApplicationLaunchVersion"},
        {39, nullptr, "CheckApplicationLaunchRights"},
        {40, nullptr, "GetApplicationLogoData"},
        {41, nullptr, "CalculateApplicationDownloadRequiredSize"},
        {42, nullptr, "CleanupSdCard"},
        {43, nullptr, "CheckSdCardMountStatus"},
        {44, nullptr, "GetSdCardMountStatusChangedEvent"},
        {45, nullptr, "GetGameCardAttachmentEvent"},
        {46, nullptr, "GetGameCardAttachmentInfo"},
        {47, nullptr, "GetTotalSpaceSize"},
        {48, nullptr, "GetFreeSpaceSize"},
        {49, nullptr, "GetSdCardRemovedEvent"},
        {52, nullptr, "GetGameCardUpdateDetectionEvent"},
        {53, nullptr, "DisableApplicationAutoDelete"},
        {54, nullptr, "EnableApplicationAutoDelete"},
        {55, &IApplicationManagerInterface::GetApplicationDesiredLanguage, "GetApplicationDesiredLanguage"},
        {56, nullptr, "SetApplicationTerminateResult"},
        {57, nullptr, "ClearApplicationTerminateResult"},
        {58, nullptr, "GetLastSdCardMountUnexpectedResult"},
        {59, &IApplicationManagerInterface::ConvertApplicationLanguageToLanguageCode, "ConvertApplicationLanguageToLanguageCode"},
        {60, nullptr, "ConvertLanguageCodeToApplicationLanguage"},
        {61, nullptr, "GetBackgroundDownloadStressTaskInfo"},
        {62, nullptr, "GetGameCardStopper"},
        {63, nullptr, "IsSystemProgramInstalled"},
        {64, nullptr, "StartApplyDeltaTask"},
        {65, nullptr, "GetRequestServerStopper"},
        {66, nullptr, "GetBackgroundApplyDeltaStressTaskInfo"},
        {67, nullptr, "CancelApplicationApplyDelta"},
        {68, nullptr, "ResumeApplicationApplyDelta"},
        {69, nullptr, "CalculateApplicationApplyDeltaRequiredSize"},
        {70, nullptr, "ResumeAll"},
        {71, nullptr, "GetStorageSize"},
        {80, nullptr, "RequestDownloadApplication"},
        {81, nullptr, "RequestDownloadAddOnContent"},
        {82, nullptr, "DownloadApplication"},
        {83, nullptr, "CheckApplicationResumeRights"},
        {84, nullptr, "GetDynamicCommitEvent"},
        {85, nullptr, "RequestUpdateApplication2"},
        {86, nullptr, "EnableApplicationCrashReport"},
        {87, nullptr, "IsApplicationCrashReportEnabled"},
        {90, nullptr, "BoostSystemMemoryResourceLimit"},
        {91, nullptr, "DeprecatedLaunchApplication"},
        {92, nullptr, "GetRunningApplicationProgramId"},
        {93, nullptr, "GetMainApplicationProgramIndex"},
        {94, nullptr, "LaunchApplication"},
        {95, nullptr, "GetApplicationLaunchInfo"},
        {96, nullptr, "AcquireApplicationLaunchInfo"},
        {97, nullptr, "GetMainApplicationProgramIndexByApplicationLaunchInfo"},
        {98, nullptr, "EnableApplicationAllThreadDumpOnCrash"},
        {99, nullptr, "LaunchDevMenu"},
        {100, nullptr, "ResetToFactorySettings"},
        {101, nullptr, "ResetToFactorySettingsWithoutUserSaveData"},
        {102, nullptr, "ResetToFactorySettingsForRefurbishment"},
        {103, nullptr, "ResetToFactorySettingsWithPlatformRegion"},
        {104, nullptr, "ResetToFactorySettingsWithPlatformRegionAuthentication"},
        {105, nullptr, "RequestResetToFactorySettingsSecurely"},
        {106, nullptr, "RequestResetToFactorySettingsWithPlatformRegionAuthenticationSecurely"},
        {200, nullptr, "CalculateUserSaveDataStatistics"},
        {201, nullptr, "DeleteUserSaveDataAll"},
        {210, nullptr, "DeleteUserSystemSaveData"},
        {211, nullptr, "DeleteSaveData"},
        {220, nullptr, "UnregisterNetworkServiceAccount"},
        {221, nullptr, "UnregisterNetworkServiceAccountWithUserSaveDataDeletion"},
        {300, nullptr, "GetApplicationShellEvent"},
        {301, nullptr, "PopApplicationShellEventInfo"},
        {302, nullptr, "LaunchLibraryApplet"},
        {303, nullptr, "TerminateLibraryApplet"},
        {304, nullptr, "LaunchSystemApplet"},
        {305, nullptr, "TerminateSystemApplet"},
        {306, nullptr, "LaunchOverlayApplet"},
        {307, nullptr, "TerminateOverlayApplet"},
        {400, &IApplicationManagerInterface::GetApplicationControlData, "GetApplicationControlData"},
        {401, nullptr, "InvalidateAllApplicationControlCache"},
        {402, nullptr, "RequestDownloadApplicationControlData"},
        {403, nullptr, "GetMaxApplicationControlCacheCount"},
        {404, nullptr, "InvalidateApplicationControlCache"},
        {405, nullptr, "ListApplicationControlCacheEntryInfo"},
        {406, nullptr, "GetApplicationControlProperty"},
        {407, nullptr, "ListApplicationTitle"},
        {408, nullptr, "ListApplicationIcon"},
        {502, nullptr, "RequestCheckGameCardRegistration"},
        {503, nullptr, "RequestGameCardRegistrationGoldPoint"},
        {504, nullptr, "RequestRegisterGameCard"},
        {505, nullptr, "GetGameCardMountFailureEvent"},
        {506, nullptr, "IsGameCardInserted"},
        {507, nullptr, "EnsureGameCardAccess"},
        {508, nullptr, "GetLastGameCardMountFailureResult"},
        {509, nullptr, "ListApplicationIdOnGameCard"},
        {510, nullptr, "GetGameCardPlatformRegion"},
        {600, nullptr, "CountApplicationContentMeta"},
        {601, nullptr, "ListApplicationContentMetaStatus"},
        {602, nullptr, "ListAvailableAddOnContent"},
        {603, nullptr, "GetOwnedApplicationContentMetaStatus"},
        {604, nullptr, "RegisterContentsExternalKey"},
        {605, nullptr, "ListApplicationContentMetaStatusWithRightsCheck"},
        {606, nullptr, "GetContentMetaStorage"},
        {607, nullptr, "ListAvailableAddOnContent"},
        {609, nullptr, "ListAvailabilityAssuredAddOnContent"},
        {700, nullptr, "PushDownloadTaskList"},
        {701, nullptr, "ClearTaskStatusList"},
        {702, nullptr, "RequestDownloadTaskList"},
        {703, nullptr, "RequestEnsureDownloadTask"},
        {704, nullptr, "ListDownloadTaskStatus"},
        {705, nullptr, "RequestDownloadTaskListData"},
        {800, nullptr, "RequestVersionList"},
        {801, nullptr, "ListVersionList"},
        {802, nullptr, "RequestVersionListData"},
        {900, nullptr, "GetApplicationRecord"},
        {901, nullptr, "GetApplicationRecordProperty"},
        {902, nullptr, "EnableApplicationAutoUpdate"},
        {903, nullptr, "DisableApplicationAutoUpdate"},
        {904, nullptr, "TouchApplication"},
        {905, nullptr, "RequestApplicationUpdate"},
        {906, nullptr, "IsApplicationUpdateRequested"},
        {907, nullptr, "WithdrawApplicationUpdateRequest"},
        {908, nullptr, "ListApplicationRecordInstalledContentMeta"},
        {909, nullptr, "WithdrawCleanupAddOnContentsWithNoRightsRecommendation"},
        {910, nullptr, "HasApplicationRecord"},
        {911, nullptr, "SetPreInstalledApplication"},
        {912, nullptr, "ClearPreInstalledApplicationFlag"},
        {913, nullptr, "ListAllApplicationRecord"},
        {914, nullptr, "HideApplicationRecord"},
        {915, nullptr, "ShowApplicationRecord"},
        {916, nullptr, "IsApplicationAutoDeleteDisabled"},
        {1000, nullptr, "RequestVerifyApplicationDeprecated"},
        {1001, nullptr, "CorruptApplicationForDebug"},
        {1002, nullptr, "RequestVerifyAddOnContentsRights"},
        {1003, nullptr, "RequestVerifyApplication"},
        {1004, nullptr, "CorruptContentForDebug"},
        {1200, nullptr, "NeedsUpdateVulnerability"},
        {1300, nullptr, "IsAnyApplicationEntityInstalled"},
        {1301, nullptr, "DeleteApplicationContentEntities"},
        {1302, nullptr, "CleanupUnrecordedApplicationEntity"},
        {1303, nullptr, "CleanupAddOnContentsWithNoRights"},
        {1304, nullptr, "DeleteApplicationContentEntity"},
        {1305, nullptr, "TryDeleteRunningApplicationEntity"},
        {1306, nullptr, "TryDeleteRunningApplicationCompletely"},
        {1307, nullptr, "TryDeleteRunningApplicationContentEntities"},
        {1308, nullptr, "DeleteApplicationCompletelyForDebug"},
        {1309, nullptr, "CleanupUnavailableAddOnContents"},
        {1310, nullptr, "RequestMoveApplicationEntity"},
        {1311, nullptr, "EstimateSizeToMove"},
        {1312, nullptr, "HasMovableEntity"},
        {1313, nullptr, "CleanupOrphanContents"},
        {1314, nullptr, "CheckPreconditionSatisfiedToMove"},
        {1400, nullptr, "PrepareShutdown"},
        {1500, nullptr, "FormatSdCard"},
        {1501, nullptr, "NeedsSystemUpdateToFormatSdCard"},
        {1502, nullptr, "GetLastSdCardFormatUnexpectedResult"},
        {1504, nullptr, "InsertSdCard"},
        {1505, nullptr, "RemoveSdCard"},
        {1506, nullptr, "GetSdCardStartupStatus"},
        {1600, nullptr, "GetSystemSeedForPseudoDeviceId"},
        {1601, nullptr, "ResetSystemSeedForPseudoDeviceId"},
        {1700, nullptr, "ListApplicationDownloadingContentMeta"},
        {1701, nullptr, "GetApplicationView"},
        {1702, nullptr, "GetApplicationDownloadTaskStatus"},
        {1703, nullptr, "GetApplicationViewDownloadErrorContext"},
        {1704, nullptr, "GetApplicationViewWithPromotionInfo"},
        {1705, nullptr, "IsPatchAutoDeletableApplication"},
        {1800, nullptr, "IsNotificationSetupCompleted"},
        {1801, nullptr, "GetLastNotificationInfoCount"},
        {1802, nullptr, "ListLastNotificationInfo"},
        {1803, nullptr, "ListNotificationTask"},
        {1900, nullptr, "IsActiveAccount"},
        {1901, nullptr, "RequestDownloadApplicationPrepurchasedRights"},
        {1902, nullptr, "GetApplicationTicketInfo"},
        {2000, nullptr, "GetSystemDeliveryInfo"},
        {2001, nullptr, "SelectLatestSystemDeliveryInfo"},
        {2002, nullptr, "VerifyDeliveryProtocolVersion"},
        {2003, nullptr, "GetApplicationDeliveryInfo"},
        {2004, nullptr, "HasAllContentsToDeliver"},
        {2005, nullptr, "CompareApplicationDeliveryInfo"},
        {2006, nullptr, "CanDeliverApplication"},
        {2007, nullptr, "ListContentMetaKeyToDeliverApplication"},
        {2008, nullptr, "NeedsSystemUpdateToDeliverApplication"},
        {2009, nullptr, "EstimateRequiredSize"},
        {2010, nullptr, "RequestReceiveApplication"},
        {2011, nullptr, "CommitReceiveApplication"},
        {2012, nullptr, "GetReceiveApplicationProgress"},
        {2013, nullptr, "RequestSendApplication"},
        {2014, nullptr, "GetSendApplicationProgress"},
        {2015, nullptr, "CompareSystemDeliveryInfo"},
        {2016, nullptr, "ListNotCommittedContentMeta"},
        {2017, nullptr, "CreateDownloadTask"},
        {2018, nullptr, "GetApplicationDeliveryInfoHash"},
        {2050, nullptr, "GetApplicationRightsOnClient"},
        {2051, nullptr, "InvalidateRightsIdCache"},
        {2100, nullptr, "GetApplicationTerminateResult"},
        {2101, nullptr, "GetRawApplicationTerminateResult"},
        {2150, nullptr, "CreateRightsEnvironment"},
        {2151, nullptr, "DestroyRightsEnvironment"},
        {2152, nullptr, "ActivateRightsEnvironment"},
        {2153, nullptr, "DeactivateRightsEnvironment"},
        {2154, nullptr, "ForceActivateRightsContextForExit"},
        {2155, nullptr, "UpdateRightsEnvironmentStatus"},
        {2156, nullptr, "CreateRightsEnvironmentForMicroApplication"},
        {2160, nullptr, "AddTargetApplicationToRightsEnvironment"},
        {2161, nullptr, "SetUsersToRightsEnvironment"},
        {2170, nullptr, "GetRightsEnvironmentStatus"},
        {2171, nullptr, "GetRightsEnvironmentStatusChangedEvent"},
        {2180, nullptr, "RequestExtendRightsInRightsEnvironment"},
        {2181, nullptr, "GetResultOfExtendRightsInRightsEnvironment"},
        {2182, nullptr, "SetActiveRightsContextUsingStateToRightsEnvironment"},
        {2190, nullptr, "GetRightsEnvironmentHandleForApplication"},
        {2199, nullptr, "GetRightsEnvironmentCountForDebug"},
        {2200, nullptr, "GetGameCardApplicationCopyIdentifier"},
        {2201, nullptr, "GetInstalledApplicationCopyIdentifier"},
        {2250, nullptr, "RequestReportActiveELicence"},
        {2300, nullptr, "ListEventLog"},
        {2350, nullptr, "PerformAutoUpdateByApplicationId"},
        {2351, nullptr, "RequestNoDownloadRightsErrorResolution"},
        {2352, nullptr, "RequestResolveNoDownloadRightsError"},
        {2353, nullptr, "GetApplicationDownloadTaskInfo"},
        {2354, nullptr, "PrioritizeApplicationBackgroundTask"},
        {2355, nullptr, "Unknown2355"},
        {2356, nullptr, "Unknown2356"},
        {2400, nullptr, "GetPromotionInfo"},
        {2401, nullptr, "CountPromotionInfo"},
        {2402, nullptr, "ListPromotionInfo"},
        {2403, nullptr, "ImportPromotionJsonForDebug"},
        {2404, nullptr, "ClearPromotionInfoForDebug"},
        {2500, nullptr, "ConfirmAvailableTime"},
        {2510, nullptr, "CreateApplicationResource"},
        {2511, nullptr, "GetApplicationResource"},
        {2513, nullptr, "LaunchMicroApplication"},
        {2514, nullptr, "ClearTaskOfAsyncTaskManager"},
        {2515, nullptr, "CleanupAllPlaceHolderAndFragmentsIfNoTask"},
        {2516, nullptr, "EnsureApplicationCertificate"},
        {2517, nullptr, "CreateApplicationInstance"},
        {2518, nullptr, "UpdateQualificationForDebug"},
        {2519, nullptr, "IsQualificationTransitionSupported"},
        {2520, nullptr, "IsQualificationTransitionSupportedByProcessId"},
        {2521, nullptr, "GetRightsUserChangedEvent"},
        {2800, nullptr, "GetApplicationIdOfPreomia"},
        {3000, nullptr, "RegisterDeviceLockKey"},
        {3001, nullptr, "UnregisterDeviceLockKey"},
        {3002, nullptr, "VerifyDeviceLockKey"},
        {3003, nullptr, "HideApplicationIcon"},
        {3004, nullptr, "ShowApplicationIcon"},
        {3005, nullptr, "HideApplicationTitle"},
        {3006, nullptr, "ShowApplicationTitle"},
        {3007, nullptr, "EnableGameCard"},
        {3008, nullptr, "DisableGameCard"},
        {3009, nullptr, "EnableLocalContentShare"},
        {3010, nullptr, "DisableLocalContentShare"},
        {3011, nullptr, "IsApplicationIconHidden"},
        {3012, nullptr, "IsApplicationTitleHidden"},
        {3013, nullptr, "IsGameCardEnabled"},
        {3014, nullptr, "IsLocalContentShareEnabled"},
        {9999, nullptr, "GetApplicationCertificate"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IApplicationManagerInterface::~IApplicationManagerInterface() = default;

void IApplicationManagerInterface::GetApplicationControlData(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto flag = rp.PopRaw<u64>();
    LOG_DEBUG(Service_NS, "called with flag={:016X}", flag);

    const auto title_id = rp.PopRaw<u64>();

    const auto size = ctx.GetWriteBufferSize();

    const FileSys::PatchManager pm{title_id, system.GetFileSystemController(),
                                   system.GetContentProvider()};
    const auto control = pm.GetControlMetadata();

    std::vector<u8> out;

    if (control.first != nullptr) {
        if (size < 0x4000) {
            LOG_ERROR(Service_NS,
                      "output buffer is too small! (actual={:016X}, expected_min=0x4000)", size);
            IPC::ResponseBuilder rb{ctx, 2};
            // TODO(DarkLordZach): Find a better error code for this.
            rb.Push(ResultUnknown);
            return;
        }

        out.resize(0x4000);
        const auto bytes = control.first->GetRawBytes();
        std::memcpy(out.data(), bytes.data(), bytes.size());
    } else {
        LOG_WARNING(Service_NS, "missing NACP data for title_id={:016X}, defaulting to zeros.",
                    title_id);
        out.resize(std::min<u64>(0x4000, size));
    }

    if (control.second != nullptr) {
        if (size < 0x4000 + control.second->GetSize()) {
            LOG_ERROR(Service_NS,
                      "output buffer is too small! (actual={:016X}, expected_min={:016X})", size,
                      0x4000 + control.second->GetSize());
            IPC::ResponseBuilder rb{ctx, 2};
            // TODO(DarkLordZach): Find a better error code for this.
            rb.Push(ResultUnknown);
            return;
        }

        out.resize(0x4000 + control.second->GetSize());
        control.second->Read(out.data() + 0x4000, control.second->GetSize());
    } else {
        LOG_WARNING(Service_NS, "missing icon data for title_id={:016X}, defaulting to zeros.",
                    title_id);
    }

    ctx.WriteBuffer(out);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(static_cast<u32>(out.size()));
}

void IApplicationManagerInterface::GetApplicationDesiredLanguage(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto supported_languages = rp.Pop<u32>();

    const auto res = GetApplicationDesiredLanguage(supported_languages);
    if (res.Succeeded()) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(*res);
    } else {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res.Code());
    }
}

ResultVal<u8> IApplicationManagerInterface::GetApplicationDesiredLanguage(
    const u32 supported_languages) {
    LOG_DEBUG(Service_NS, "called with supported_languages={:08X}", supported_languages);

    // Get language code from settings
    const auto language_code =
        Set::GetLanguageCodeFromIndex(Settings::values.language_index.GetValue());

    // Convert to application language, get priority list
    const auto application_language = ConvertToApplicationLanguage(language_code);
    if (application_language == std::nullopt) {
        LOG_ERROR(Service_NS, "Could not convert application language! language_code={}",
                  language_code);
        return ERR_APPLICATION_LANGUAGE_NOT_FOUND;
    }
    const auto priority_list = GetApplicationLanguagePriorityList(*application_language);
    if (!priority_list) {
        LOG_ERROR(Service_NS,
                  "Could not find application language priorities! application_language={}",
                  *application_language);
        return ERR_APPLICATION_LANGUAGE_NOT_FOUND;
    }

    // Try to find a valid language.
    for (const auto lang : *priority_list) {
        const auto supported_flag = GetSupportedLanguageFlag(lang);
        if (supported_languages == 0 || (supported_languages & supported_flag) == supported_flag) {
            return static_cast<u8>(lang);
        }
    }

    LOG_ERROR(Service_NS, "Could not find a valid language! supported_languages={:08X}",
              supported_languages);
    return ERR_APPLICATION_LANGUAGE_NOT_FOUND;
}

void IApplicationManagerInterface::ConvertApplicationLanguageToLanguageCode(
    Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto application_language = rp.Pop<u8>();

    const auto res = ConvertApplicationLanguageToLanguageCode(application_language);
    if (res.Succeeded()) {
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(*res);
    } else {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res.Code());
    }
}

ResultVal<u64> IApplicationManagerInterface::ConvertApplicationLanguageToLanguageCode(
    u8 application_language) {
    const auto language_code =
        ConvertToLanguageCode(static_cast<ApplicationLanguage>(application_language));
    if (language_code == std::nullopt) {
        LOG_ERROR(Service_NS, "Language not found! application_language={}", application_language);
        return ERR_APPLICATION_LANGUAGE_NOT_FOUND;
    }

    return static_cast<u64>(*language_code);
}

IApplicationVersionInterface::IApplicationVersionInterface(Core::System& system_)
    : ServiceFramework{system_, "IApplicationVersionInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetLaunchRequiredVersion"},
        {1, nullptr, "UpgradeLaunchRequiredVersion"},
        {35, nullptr, "UpdateVersionList"},
        {36, nullptr, "PushLaunchVersion"},
        {37, nullptr, "ListRequiredVersion"},
        {800, nullptr, "RequestVersionList"},
        {801, nullptr, "ListVersionList"},
        {802, nullptr, "RequestVersionListData"},
        {900, nullptr, "ImportAutoUpdatePolicyJsonForDebug"},
        {901, nullptr, "ListDefaultAutoUpdatePolicy"},
        {902, nullptr, "ListAutoUpdatePolicyForSpecificApplication"},
        {1000, nullptr, "PerformAutoUpdate"},
        {1001, nullptr, "ListAutoUpdateSchedule"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IApplicationVersionInterface::~IApplicationVersionInterface() = default;

IContentManagementInterface::IContentManagementInterface(Core::System& system_)
    : ServiceFramework{system_, "IContentManagementInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {11, nullptr, "CalculateApplicationOccupiedSize"},
        {43, nullptr, "CheckSdCardMountStatus"},
        {47, nullptr, "GetTotalSpaceSize"},
        {48, nullptr, "GetFreeSpaceSize"},
        {600, nullptr, "CountApplicationContentMeta"},
        {601, nullptr, "ListApplicationContentMetaStatus"},
        {605, nullptr, "ListApplicationContentMetaStatusWithRightsCheck"},
        {607, nullptr, "IsAnyApplicationRunning"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IContentManagementInterface::~IContentManagementInterface() = default;

IDocumentInterface::IDocumentInterface(Core::System& system_)
    : ServiceFramework{system_, "IDocumentInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {21, nullptr, "GetApplicationContentPath"},
        {23, nullptr, "ResolveApplicationContentPath"},
        {93, nullptr, "GetRunningApplicationProgramId"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IDocumentInterface::~IDocumentInterface() = default;

IDownloadTaskInterface::IDownloadTaskInterface(Core::System& system_)
    : ServiceFramework{system_, "IDownloadTaskInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {701, nullptr, "ClearTaskStatusList"},
        {702, nullptr, "RequestDownloadTaskList"},
        {703, nullptr, "RequestEnsureDownloadTask"},
        {704, nullptr, "ListDownloadTaskStatus"},
        {705, nullptr, "RequestDownloadTaskListData"},
        {706, nullptr, "TryCommitCurrentApplicationDownloadTask"},
        {707, nullptr, "EnableAutoCommit"},
        {708, nullptr, "DisableAutoCommit"},
        {709, nullptr, "TriggerDynamicCommitEvent"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IDownloadTaskInterface::~IDownloadTaskInterface() = default;

IECommerceInterface::IECommerceInterface(Core::System& system_)
    : ServiceFramework{system_, "IECommerceInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestLinkDevice"},
        {1, nullptr, "RequestCleanupAllPreInstalledApplications"},
        {2, nullptr, "RequestCleanupPreInstalledApplication"},
        {3, nullptr, "RequestSyncRights"},
        {4, nullptr, "RequestUnlinkDevice"},
        {5, nullptr, "RequestRevokeAllELicense"},
        {6, nullptr, "RequestSyncRightsBasedOnAssignedELicenses"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IECommerceInterface::~IECommerceInterface() = default;

IFactoryResetInterface::IFactoryResetInterface(Core::System& system_)
    : ServiceFramework{system_, "IFactoryResetInterface"} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {100, nullptr, "ResetToFactorySettings"},
            {101, nullptr, "ResetToFactorySettingsWithoutUserSaveData"},
            {102, nullptr, "ResetToFactorySettingsForRefurbishment"},
            {103, nullptr, "ResetToFactorySettingsWithPlatformRegion"},
            {104, nullptr, "ResetToFactorySettingsWithPlatformRegionAuthentication"},
            {105, nullptr, "RequestResetToFactorySettingsSecurely"},
            {106, nullptr, "RequestResetToFactorySettingsWithPlatformRegionAuthenticationSecurely"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

IFactoryResetInterface::~IFactoryResetInterface() = default;

IReadOnlyApplicationControlDataInterface::IReadOnlyApplicationControlDataInterface(
    Core::System& system_)
    : ServiceFramework{system_, "IReadOnlyApplicationControlDataInterface"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &IReadOnlyApplicationControlDataInterface::GetApplicationControlData, "GetApplicationControlData"},
        {1, nullptr, "GetApplicationDesiredLanguage"},
        {2, nullptr, "ConvertApplicationLanguageToLanguageCode"},
        {3, nullptr, "ConvertLanguageCodeToApplicationLanguage"},
        {4, nullptr, "SelectApplicationDesiredLanguage"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IReadOnlyApplicationControlDataInterface::~IReadOnlyApplicationControlDataInterface() = default;

void IReadOnlyApplicationControlDataInterface::GetApplicationControlData(
    Kernel::HLERequestContext& ctx) {
    enum class ApplicationControlSource : u8 {
        CacheOnly,
        Storage,
        StorageOnly,
    };

    struct RequestParameters {
        ApplicationControlSource source;
        u64 application_id;
    };
    static_assert(sizeof(RequestParameters) == 0x10, "RequestParameters has incorrect size.");

    IPC::RequestParser rp{ctx};
    const auto parameters{rp.PopRaw<RequestParameters>()};
    const auto nacp_data{system.GetARPManager().GetControlProperty(parameters.application_id)};
    const auto result = nacp_data ? ResultSuccess : ResultUnknown;

    if (nacp_data) {
        ctx.WriteBuffer(nacp_data->data(), nacp_data->size());
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

NS::NS(const char* name, Core::System& system_) : ServiceFramework{system_, name} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {7988, nullptr, "GetDynamicRightsInterface"},
        {7989, &NS::PushInterface<IReadOnlyApplicationControlDataInterface>, "GetReadOnlyApplicationControlDataInterface"},
        {7991, nullptr, "GetReadOnlyApplicationRecordInterface"},
        {7992, &NS::PushInterface<IECommerceInterface>, "GetECommerceInterface"},
        {7993, &NS::PushInterface<IApplicationVersionInterface>, "GetApplicationVersionInterface"},
        {7994, &NS::PushInterface<IFactoryResetInterface>, "GetFactoryResetInterface"},
        {7995, &NS::PushInterface<IAccountProxyInterface>, "GetAccountProxyInterface"},
        {7996, &NS::PushIApplicationManagerInterface, "GetApplicationManagerInterface"},
        {7997, &NS::PushInterface<IDownloadTaskInterface>, "GetDownloadTaskInterface"},
        {7998, &NS::PushInterface<IContentManagementInterface>, "GetContentManagementInterface"},
        {7999, &NS::PushInterface<IDocumentInterface>, "GetDocumentInterface"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

NS::~NS() = default;

std::shared_ptr<IApplicationManagerInterface> NS::GetApplicationManagerInterface() const {
    return GetInterface<IApplicationManagerInterface>(system);
}

class NS_DEV final : public ServiceFramework<NS_DEV> {
public:
    explicit NS_DEV(Core::System& system_) : ServiceFramework{system_, "ns:dev"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "LaunchProgram"},
            {1, nullptr, "TerminateProcess"},
            {2, nullptr, "TerminateProgram"},
            {4, nullptr, "GetShellEvent"},
            {5, nullptr, "GetShellEventInfo"},
            {6, nullptr, "TerminateApplication"},
            {7, nullptr, "PrepareLaunchProgramFromHost"},
            {8, nullptr, "LaunchApplicationFromHost"},
            {9, nullptr, "LaunchApplicationWithStorageIdForDevelop"},
            {10, nullptr, "IsSystemMemoryResourceLimitBoosted"},
            {11, nullptr, "GetRunningApplicationProcessIdForDevelop"},
            {12, nullptr, "SetCurrentApplicationRightsEnvironmentCanBeActiveForDevelop"},
            {13, nullptr, "CreateApplicationResourceForDevelop"},
            {14, nullptr, "IsPreomiaForDevelop"},
            {15, nullptr, "GetApplicationProgramIdFromHost"},
            {16, nullptr, "RefreshCachedDebugValues"},
            {17, nullptr, "PrepareLaunchApplicationFromHost"},
            {18, nullptr, "GetLaunchEvent"},
            {19, nullptr, "GetLaunchResult"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class ISystemUpdateControl final : public ServiceFramework<ISystemUpdateControl> {
public:
    explicit ISystemUpdateControl(Core::System& system_)
        : ServiceFramework{system_, "ISystemUpdateControl"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "HasDownloaded"},
            {1, nullptr, "RequestCheckLatestUpdate"},
            {2, nullptr, "RequestDownloadLatestUpdate"},
            {3, nullptr, "GetDownloadProgress"},
            {4, nullptr, "ApplyDownloadedUpdate"},
            {5, nullptr, "RequestPrepareCardUpdate"},
            {6, nullptr, "GetPrepareCardUpdateProgress"},
            {7, nullptr, "HasPreparedCardUpdate"},
            {8, nullptr, "ApplyCardUpdate"},
            {9, nullptr, "GetDownloadedEulaDataSize"},
            {10, nullptr, "GetDownloadedEulaData"},
            {11, nullptr, "SetupCardUpdate"},
            {12, nullptr, "GetPreparedCardUpdateEulaDataSize"},
            {13, nullptr, "GetPreparedCardUpdateEulaData"},
            {14, nullptr, "SetupCardUpdateViaSystemUpdater"},
            {15, nullptr, "HasReceived"},
            {16, nullptr, "RequestReceiveSystemUpdate"},
            {17, nullptr, "GetReceiveProgress"},
            {18, nullptr, "ApplyReceivedUpdate"},
            {19, nullptr, "GetReceivedEulaDataSize"},
            {20, nullptr, "GetReceivedEulaData"},
            {21, nullptr, "SetupToReceiveSystemUpdate"},
            {22, nullptr, "RequestCheckLatestUpdateIncludesRebootlessUpdate"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class NS_SU final : public ServiceFramework<NS_SU> {
public:
    explicit NS_SU(Core::System& system_) : ServiceFramework{system_, "ns:su"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetBackgroundNetworkUpdateState"},
            {1, &NS_SU::OpenSystemUpdateControl, "OpenSystemUpdateControl"},
            {2, nullptr, "NotifyExFatDriverRequired"},
            {3, nullptr, "ClearExFatDriverStatusForDebug"},
            {4, nullptr, "RequestBackgroundNetworkUpdate"},
            {5, nullptr, "NotifyBackgroundNetworkUpdate"},
            {6, nullptr, "NotifyExFatDriverDownloadedForDebug"},
            {9, nullptr, "GetSystemUpdateNotificationEventForContentDelivery"},
            {10, nullptr, "NotifySystemUpdateForContentDelivery"},
            {11, nullptr, "PrepareShutdown"},
            {12, nullptr, "Unknown12"},
            {13, nullptr, "Unknown13"},
            {14, nullptr, "Unknown14"},
            {15, nullptr, "Unknown15"},
            {16, nullptr, "DestroySystemUpdateTask"},
            {17, nullptr, "RequestSendSystemUpdate"},
            {18, nullptr, "GetSendSystemUpdateProgress"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void OpenSystemUpdateControl(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NS, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<ISystemUpdateControl>(system);
    }
};

class NS_VM final : public ServiceFramework<NS_VM> {
public:
    explicit NS_VM(Core::System& system_) : ServiceFramework{system_, "ns:vm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {1200, &NS_VM::NeedsUpdateVulnerability, "NeedsUpdateVulnerability"},
            {1201, nullptr, "UpdateSafeSystemVersionForDebug"},
            {1202, nullptr, "GetSafeSystemVersion"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void NeedsUpdateVulnerability(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NS, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(false);
    }
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {

    std::make_shared<NS>("ns:am2", system)->InstallAsService(service_manager);
    std::make_shared<NS>("ns:ec", system)->InstallAsService(service_manager);
    std::make_shared<NS>("ns:rid", system)->InstallAsService(service_manager);
    std::make_shared<NS>("ns:rt", system)->InstallAsService(service_manager);
    std::make_shared<NS>("ns:web", system)->InstallAsService(service_manager);
    std::make_shared<NS>("ns:ro", system)->InstallAsService(service_manager);

    std::make_shared<NS_DEV>(system)->InstallAsService(service_manager);
    std::make_shared<NS_SU>(system)->InstallAsService(service_manager);
    std::make_shared<NS_VM>(system)->InstallAsService(service_manager);

    std::make_shared<PDM_QRY>(system)->InstallAsService(service_manager);

    std::make_shared<IPlatformServiceManager>(system, "pl:s")->InstallAsService(service_manager);
    std::make_shared<IPlatformServiceManager>(system, "pl:u")->InstallAsService(service_manager);
}

} // namespace Service::NS
