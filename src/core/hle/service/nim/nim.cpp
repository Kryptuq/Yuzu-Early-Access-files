// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <ctime>
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/nim/nim.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::NIM {

class IShopServiceAsync final : public ServiceFramework<IShopServiceAsync> {
public:
    explicit IShopServiceAsync(Core::System& system_)
        : ServiceFramework{system_, "IShopServiceAsync"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Cancel"},
            {1, nullptr, "GetSize"},
            {2, nullptr, "Read"},
            {3, nullptr, "GetErrorCode"},
            {4, nullptr, "Request"},
            {5, nullptr, "Prepare"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IShopServiceAccessor final : public ServiceFramework<IShopServiceAccessor> {
public:
    explicit IShopServiceAccessor(Core::System& system_)
        : ServiceFramework{system_, "IShopServiceAccessor"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IShopServiceAccessor::CreateAsyncInterface, "CreateAsyncInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateAsyncInterface(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IShopServiceAsync>(system);
    }
};

class IShopServiceAccessServer final : public ServiceFramework<IShopServiceAccessServer> {
public:
    explicit IShopServiceAccessServer(Core::System& system_)
        : ServiceFramework{system_, "IShopServiceAccessServer"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IShopServiceAccessServer::CreateAccessorInterface, "CreateAccessorInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateAccessorInterface(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IShopServiceAccessor>(system);
    }
};

class NIM final : public ServiceFramework<NIM> {
public:
    explicit NIM(Core::System& system_) : ServiceFramework{system_, "nim"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "CreateSystemUpdateTask"},
            {1, nullptr, "DestroySystemUpdateTask"},
            {2, nullptr, "ListSystemUpdateTask"},
            {3, nullptr, "RequestSystemUpdateTaskRun"},
            {4, nullptr, "GetSystemUpdateTaskInfo"},
            {5, nullptr, "CommitSystemUpdateTask"},
            {6, nullptr, "CreateNetworkInstallTask"},
            {7, nullptr, "DestroyNetworkInstallTask"},
            {8, nullptr, "ListNetworkInstallTask"},
            {9, nullptr, "RequestNetworkInstallTaskRun"},
            {10, nullptr, "GetNetworkInstallTaskInfo"},
            {11, nullptr, "CommitNetworkInstallTask"},
            {12, nullptr, "RequestLatestSystemUpdateMeta"},
            {14, nullptr, "ListApplicationNetworkInstallTask"},
            {15, nullptr, "ListNetworkInstallTaskContentMeta"},
            {16, nullptr, "RequestLatestVersion"},
            {17, nullptr, "SetNetworkInstallTaskAttribute"},
            {18, nullptr, "AddNetworkInstallTaskContentMeta"},
            {19, nullptr, "GetDownloadedSystemDataPath"},
            {20, nullptr, "CalculateNetworkInstallTaskRequiredSize"},
            {21, nullptr, "IsExFatDriverIncluded"},
            {22, nullptr, "GetBackgroundDownloadStressTaskInfo"},
            {23, nullptr, "RequestDeviceAuthenticationToken"},
            {24, nullptr, "RequestGameCardRegistrationStatus"},
            {25, nullptr, "RequestRegisterGameCard"},
            {26, nullptr, "RequestRegisterNotificationToken"},
            {27, nullptr, "RequestDownloadTaskList"},
            {28, nullptr, "RequestApplicationControl"},
            {29, nullptr, "RequestLatestApplicationControl"},
            {30, nullptr, "RequestVersionList"},
            {31, nullptr, "CreateApplyDeltaTask"},
            {32, nullptr, "DestroyApplyDeltaTask"},
            {33, nullptr, "ListApplicationApplyDeltaTask"},
            {34, nullptr, "RequestApplyDeltaTaskRun"},
            {35, nullptr, "GetApplyDeltaTaskInfo"},
            {36, nullptr, "ListApplyDeltaTask"},
            {37, nullptr, "CommitApplyDeltaTask"},
            {38, nullptr, "CalculateApplyDeltaTaskRequiredSize"},
            {39, nullptr, "PrepareShutdown"},
            {40, nullptr, "ListApplyDeltaTask"},
            {41, nullptr, "ClearNotEnoughSpaceStateOfApplyDeltaTask"},
            {42, nullptr, "CreateApplyDeltaTaskFromDownloadTask"},
            {43, nullptr, "GetBackgroundApplyDeltaStressTaskInfo"},
            {44, nullptr, "GetApplyDeltaTaskRequiredStorage"},
            {45, nullptr, "CalculateNetworkInstallTaskContentsSize"},
            {46, nullptr, "PrepareShutdownForSystemUpdate"},
            {47, nullptr, "FindMaxRequiredApplicationVersionOfTask"},
            {48, nullptr, "CommitNetworkInstallTaskPartially"},
            {49, nullptr, "ListNetworkInstallTaskCommittedContentMeta"},
            {50, nullptr, "ListNetworkInstallTaskNotCommittedContentMeta"},
            {51, nullptr, "FindMaxRequiredSystemVersionOfTask"},
            {52, nullptr, "GetNetworkInstallTaskErrorContext"},
            {53, nullptr, "CreateLocalCommunicationReceiveApplicationTask"},
            {54, nullptr, "DestroyLocalCommunicationReceiveApplicationTask"},
            {55, nullptr, "ListLocalCommunicationReceiveApplicationTask"},
            {56, nullptr, "RequestLocalCommunicationReceiveApplicationTaskRun"},
            {57, nullptr, "GetLocalCommunicationReceiveApplicationTaskInfo"},
            {58, nullptr, "CommitLocalCommunicationReceiveApplicationTask"},
            {59, nullptr, "ListLocalCommunicationReceiveApplicationTaskContentMeta"},
            {60, nullptr, "CreateLocalCommunicationSendApplicationTask"},
            {61, nullptr, "RequestLocalCommunicationSendApplicationTaskRun"},
            {62, nullptr, "GetLocalCommunicationReceiveApplicationTaskErrorContext"},
            {63, nullptr, "GetLocalCommunicationSendApplicationTaskInfo"},
            {64, nullptr, "DestroyLocalCommunicationSendApplicationTask"},
            {65, nullptr, "GetLocalCommunicationSendApplicationTaskErrorContext"},
            {66, nullptr, "CalculateLocalCommunicationReceiveApplicationTaskRequiredSize"},
            {67, nullptr, "ListApplicationLocalCommunicationReceiveApplicationTask"},
            {68, nullptr, "ListApplicationLocalCommunicationSendApplicationTask"},
            {69, nullptr, "CreateLocalCommunicationReceiveSystemUpdateTask"},
            {70, nullptr, "DestroyLocalCommunicationReceiveSystemUpdateTask"},
            {71, nullptr, "ListLocalCommunicationReceiveSystemUpdateTask"},
            {72, nullptr, "RequestLocalCommunicationReceiveSystemUpdateTaskRun"},
            {73, nullptr, "GetLocalCommunicationReceiveSystemUpdateTaskInfo"},
            {74, nullptr, "CommitLocalCommunicationReceiveSystemUpdateTask"},
            {75, nullptr, "GetLocalCommunicationReceiveSystemUpdateTaskErrorContext"},
            {76, nullptr, "CreateLocalCommunicationSendSystemUpdateTask"},
            {77, nullptr, "RequestLocalCommunicationSendSystemUpdateTaskRun"},
            {78, nullptr, "GetLocalCommunicationSendSystemUpdateTaskInfo"},
            {79, nullptr, "DestroyLocalCommunicationSendSystemUpdateTask"},
            {80, nullptr, "GetLocalCommunicationSendSystemUpdateTaskErrorContext"},
            {81, nullptr, "ListLocalCommunicationSendSystemUpdateTask"},
            {82, nullptr, "GetReceivedSystemDataPath"},
            {83, nullptr, "CalculateApplyDeltaTaskOccupiedSize"},
            {84, nullptr, "Unknown84"},
            {85, nullptr, "ListNetworkInstallTaskContentMetaFromInstallMeta"},
            {86, nullptr, "ListNetworkInstallTaskOccupiedSize"},
            {87, nullptr, "Unknown87"},
            {88, nullptr, "Unknown88"},
            {89, nullptr, "Unknown89"},
            {90, nullptr, "Unknown90"},
            {91, nullptr, "Unknown91"},
            {92, nullptr, "Unknown92"},
            {93, nullptr, "Unknown93"},
            {94, nullptr, "Unknown94"},
            {95, nullptr, "Unknown95"},
            {96, nullptr, "Unknown96"},
            {97, nullptr, "Unknown97"},
            {98, nullptr, "Unknown98"},
            {99, nullptr, "Unknown99"},
            {100, nullptr, "Unknown100"},
            {101, nullptr, "Unknown101"},
            {102, nullptr, "Unknown102"},
            {103, nullptr, "Unknown103"},
            {104, nullptr, "Unknown104"},
            {105, nullptr, "Unknown105"},
            {106, nullptr, "Unknown106"},
            {107, nullptr, "Unknown107"},
            {108, nullptr, "Unknown108"},
            {109, nullptr, "Unknown109"},
            {110, nullptr, "Unknown110"},
            {111, nullptr, "Unknown111"},
            {112, nullptr, "Unknown112"},
            {113, nullptr, "Unknown113"},
            {114, nullptr, "Unknown114"},
            {115, nullptr, "Unknown115"},
            {116, nullptr, "Unknown116"},
            {117, nullptr, "Unknown117"},
            {118, nullptr, "Unknown118"},
            {119, nullptr, "Unknown119"},
            {120, nullptr, "Unknown120"},
            {121, nullptr, "Unknown121"},
            {122, nullptr, "Unknown122"},
            {123, nullptr, "Unknown123"},
            {124, nullptr, "Unknown124"},
            {125, nullptr, "Unknown125"},
            {126, nullptr, "Unknown126"},
            {127, nullptr, "Unknown127"},
            {128, nullptr, "Unknown128"},
            {129, nullptr, "Unknown129"},
            {130, nullptr, "Unknown130"},
            {131, nullptr, "Unknown131"},
            {132, nullptr, "Unknown132"},
            {133, nullptr, "Unknown133"},
            {134, nullptr, "Unknown134"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class NIM_ECA final : public ServiceFramework<NIM_ECA> {
public:
    explicit NIM_ECA(Core::System& system_) : ServiceFramework{system_, "nim:eca"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &NIM_ECA::CreateServerInterface, "CreateServerInterface"},
            {1, nullptr, "RefreshDebugAvailability"},
            {2, nullptr, "ClearDebugResponse"},
            {3, nullptr, "RegisterDebugResponse"},
            {4, &NIM_ECA::IsLargeResourceAvailable, "IsLargeResourceAvailable"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateServerInterface(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIM, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IShopServiceAccessServer>(system);
    }

    void IsLargeResourceAvailable(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto unknown{rp.Pop<u64>()};

        LOG_INFO(Service_NIM, "(STUBBED) called, unknown={}", unknown);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(false);
    }
};

class NIM_SHP final : public ServiceFramework<NIM_SHP> {
public:
    explicit NIM_SHP(Core::System& system_) : ServiceFramework{system_, "nim:shp"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "RequestDeviceAuthenticationToken"},
            {1, nullptr, "RequestCachedDeviceAuthenticationToken"},
            {2, nullptr, "RequestEdgeToken"},
            {3, nullptr, "RequestCachedEdgeToken"},
            {100, nullptr, "RequestRegisterDeviceAccount"},
            {101, nullptr, "RequestUnregisterDeviceAccount"},
            {102, nullptr, "RequestDeviceAccountStatus"},
            {103, nullptr, "GetDeviceAccountInfo"},
            {104, nullptr, "RequestDeviceRegistrationInfo"},
            {105, nullptr, "RequestTransferDeviceAccount"},
            {106, nullptr, "RequestSyncRegistration"},
            {107, nullptr, "IsOwnDeviceId"},
            {200, nullptr, "RequestRegisterNotificationToken"},
            {300, nullptr, "RequestUnlinkDevice"},
            {301, nullptr, "RequestUnlinkDeviceIntegrated"},
            {302, nullptr, "RequestLinkDevice"},
            {303, nullptr, "HasDeviceLink"},
            {304, nullptr, "RequestUnlinkDeviceAll"},
            {305, nullptr, "RequestCreateVirtualAccount"},
            {306, nullptr, "RequestDeviceLinkStatus"},
            {400, nullptr, "GetAccountByVirtualAccount"},
            {401, nullptr, "GetVirtualAccount"},
            {500, nullptr, "RequestSyncTicketLegacy"},
            {501, nullptr, "RequestDownloadTicket"},
            {502, nullptr, "RequestDownloadTicketForPrepurchasedContents"},
            {503, nullptr, "RequestSyncTicket"},
            {504, nullptr, "RequestDownloadTicketForPrepurchasedContents2"},
            {505, nullptr, "RequestDownloadTicketForPrepurchasedContentsForAccount"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IEnsureNetworkClockAvailabilityService final
    : public ServiceFramework<IEnsureNetworkClockAvailabilityService> {
public:
    explicit IEnsureNetworkClockAvailabilityService(Core::System& system_)
        : ServiceFramework{system_, "IEnsureNetworkClockAvailabilityService"},
          service_context{system_, "IEnsureNetworkClockAvailabilityService"} {
        static const FunctionInfo functions[] = {
            {0, &IEnsureNetworkClockAvailabilityService::StartTask, "StartTask"},
            {1, &IEnsureNetworkClockAvailabilityService::GetFinishNotificationEvent,
             "GetFinishNotificationEvent"},
            {2, &IEnsureNetworkClockAvailabilityService::GetResult, "GetResult"},
            {3, &IEnsureNetworkClockAvailabilityService::Cancel, "Cancel"},
            {4, &IEnsureNetworkClockAvailabilityService::IsProcessing, "IsProcessing"},
            {5, &IEnsureNetworkClockAvailabilityService::GetServerTime, "GetServerTime"},
        };
        RegisterHandlers(functions);

        finished_event =
            service_context.CreateEvent("IEnsureNetworkClockAvailabilityService:FinishEvent");
    }

    ~IEnsureNetworkClockAvailabilityService() override {
        service_context.CloseEvent(finished_event);
    }

private:
    void StartTask(Kernel::HLERequestContext& ctx) {
        // No need to connect to the internet, just finish the task straight away.
        LOG_DEBUG(Service_NIM, "called");
        finished_event->GetWritableEvent().Signal();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetFinishNotificationEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(finished_event->GetReadableEvent());
    }

    void GetResult(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIM, "called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Cancel(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIM, "called");
        finished_event->GetWritableEvent().Clear();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void IsProcessing(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIM, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushRaw<u32>(0); // We instantly process the request
    }

    void GetServerTime(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIM, "called");

        const s64 server_time{std::chrono::duration_cast<std::chrono::seconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count()};
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.PushRaw<s64>(server_time);
    }

    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* finished_event;
};

class NTC final : public ServiceFramework<NTC> {
public:
    explicit NTC(Core::System& system_) : ServiceFramework{system_, "ntc"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &NTC::OpenEnsureNetworkClockAvailabilityService, "OpenEnsureNetworkClockAvailabilityService"},
            {100, &NTC::SuspendAutonomicTimeCorrection, "SuspendAutonomicTimeCorrection"},
            {101, &NTC::ResumeAutonomicTimeCorrection, "ResumeAutonomicTimeCorrection"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void OpenEnsureNetworkClockAvailabilityService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NIM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IEnsureNetworkClockAvailabilityService>(system);
    }

    // TODO(ogniK): Do we need these?
    void SuspendAutonomicTimeCorrection(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void ResumeAutonomicTimeCorrection(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NIM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<NIM>(system)->InstallAsService(sm);
    std::make_shared<NIM_ECA>(system)->InstallAsService(sm);
    std::make_shared<NIM_SHP>(system)->InstallAsService(sm);
    std::make_shared<NTC>(system)->InstallAsService(sm);
}

} // namespace Service::NIM
