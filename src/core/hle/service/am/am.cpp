// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstring>
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/savedata_factory.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/service/acc/profile_manager.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applet_ae.h"
#include "core/hle/service/am/applet_oe.h"
#include "core/hle/service/am/applets/applet_profile_select.h"
#include "core/hle/service/am/applets/applet_web_browser.h"
#include "core/hle/service/am/applets/applets.h"
#include "core/hle/service/am/idle.h"
#include "core/hle/service/am/omm.h"
#include "core/hle/service/am/spsm.h"
#include "core/hle/service/apm/apm_controller.h"
#include "core/hle/service/apm/apm_interface.h"
#include "core/hle/service/bcat/backend/backend.h"
#include "core/hle/service/caps/caps.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/ns/ns.h"
#include "core/hle/service/nvnflinger/nvnflinger.h"
#include "core/hle/service/pm/pm.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/sm/sm.h"
#include "core/hle/service/vi/vi.h"
#include "core/memory.h"

namespace Service::AM {

constexpr Result ResultNoDataInChannel{ErrorModule::AM, 2};
constexpr Result ResultNoMessages{ErrorModule::AM, 3};
constexpr Result ResultInvalidOffset{ErrorModule::AM, 503};

enum class LaunchParameterKind : u32 {
    ApplicationSpecific = 1,
    AccountPreselectedUser = 2,
};

constexpr u32 LAUNCH_PARAMETER_ACCOUNT_PRESELECTED_USER_MAGIC = 0xC79497CA;

struct LaunchParameterAccountPreselectedUser {
    u32_le magic;
    u32_le is_account_selected;
    Common::UUID current_user;
    INSERT_PADDING_BYTES(0x70);
};
static_assert(sizeof(LaunchParameterAccountPreselectedUser) == 0x88);

IWindowController::IWindowController(Core::System& system_)
    : ServiceFramework{system_, "IWindowController"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CreateWindow"},
        {1, &IWindowController::GetAppletResourceUserId, "GetAppletResourceUserId"},
        {2, nullptr, "GetAppletResourceUserIdOfCallerApplet"},
        {10, &IWindowController::AcquireForegroundRights, "AcquireForegroundRights"},
        {11, nullptr, "ReleaseForegroundRights"},
        {12, nullptr, "RejectToChangeIntoBackground"},
        {20, nullptr, "SetAppletWindowVisibility"},
        {21, nullptr, "SetAppletGpuTimeSlice"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IWindowController::~IWindowController() = default;

void IWindowController::GetAppletResourceUserId(HLERequestContext& ctx) {
    const u64 process_id = system.ApplicationProcess()->GetProcessId();

    LOG_DEBUG(Service_AM, "called. Process ID=0x{:016X}", process_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<u64>(process_id);
}

void IWindowController::AcquireForegroundRights(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

IAudioController::IAudioController(Core::System& system_)
    : ServiceFramework{system_, "IAudioController"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &IAudioController::SetExpectedMasterVolume, "SetExpectedMasterVolume"},
        {1, &IAudioController::GetMainAppletExpectedMasterVolume, "GetMainAppletExpectedMasterVolume"},
        {2, &IAudioController::GetLibraryAppletExpectedMasterVolume, "GetLibraryAppletExpectedMasterVolume"},
        {3, &IAudioController::ChangeMainAppletMasterVolume, "ChangeMainAppletMasterVolume"},
        {4, &IAudioController::SetTransparentAudioRate, "SetTransparentVolumeRate"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IAudioController::~IAudioController() = default;

void IAudioController::SetExpectedMasterVolume(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const float main_applet_volume_tmp = rp.Pop<float>();
    const float library_applet_volume_tmp = rp.Pop<float>();

    LOG_DEBUG(Service_AM, "called. main_applet_volume={}, library_applet_volume={}",
              main_applet_volume_tmp, library_applet_volume_tmp);

    // Ensure the volume values remain within the 0-100% range
    main_applet_volume = std::clamp(main_applet_volume_tmp, min_allowed_volume, max_allowed_volume);
    library_applet_volume =
        std::clamp(library_applet_volume_tmp, min_allowed_volume, max_allowed_volume);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IAudioController::GetMainAppletExpectedMasterVolume(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called. main_applet_volume={}", main_applet_volume);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(main_applet_volume);
}

void IAudioController::GetLibraryAppletExpectedMasterVolume(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called. library_applet_volume={}", library_applet_volume);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(library_applet_volume);
}

void IAudioController::ChangeMainAppletMasterVolume(HLERequestContext& ctx) {
    struct Parameters {
        float volume;
        s64 fade_time_ns;
    };
    static_assert(sizeof(Parameters) == 16);

    IPC::RequestParser rp{ctx};
    const auto parameters = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_AM, "called. volume={}, fade_time_ns={}", parameters.volume,
              parameters.fade_time_ns);

    main_applet_volume = std::clamp(parameters.volume, min_allowed_volume, max_allowed_volume);
    fade_time_ns = std::chrono::nanoseconds{parameters.fade_time_ns};

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IAudioController::SetTransparentAudioRate(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const float transparent_volume_rate_tmp = rp.Pop<float>();

    LOG_DEBUG(Service_AM, "called. transparent_volume_rate={}", transparent_volume_rate_tmp);

    // Clamp volume range to 0-100%.
    transparent_volume_rate =
        std::clamp(transparent_volume_rate_tmp, min_allowed_volume, max_allowed_volume);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

IDisplayController::IDisplayController(Core::System& system_)
    : ServiceFramework{system_, "IDisplayController"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetLastForegroundCaptureImage"},
        {1, nullptr, "UpdateLastForegroundCaptureImage"},
        {2, nullptr, "GetLastApplicationCaptureImage"},
        {3, nullptr, "GetCallerAppletCaptureImage"},
        {4, nullptr, "UpdateCallerAppletCaptureImage"},
        {5, nullptr, "GetLastForegroundCaptureImageEx"},
        {6, nullptr, "GetLastApplicationCaptureImageEx"},
        {7, nullptr, "GetCallerAppletCaptureImageEx"},
        {8, nullptr, "TakeScreenShotOfOwnLayer"},
        {9, nullptr, "CopyBetweenCaptureBuffers"},
        {10, nullptr, "AcquireLastApplicationCaptureBuffer"},
        {11, nullptr, "ReleaseLastApplicationCaptureBuffer"},
        {12, nullptr, "AcquireLastForegroundCaptureBuffer"},
        {13, nullptr, "ReleaseLastForegroundCaptureBuffer"},
        {14, nullptr, "AcquireCallerAppletCaptureBuffer"},
        {15, nullptr, "ReleaseCallerAppletCaptureBuffer"},
        {16, nullptr, "AcquireLastApplicationCaptureBufferEx"},
        {17, nullptr, "AcquireLastForegroundCaptureBufferEx"},
        {18, nullptr, "AcquireCallerAppletCaptureBufferEx"},
        {20, nullptr, "ClearCaptureBuffer"},
        {21, nullptr, "ClearAppletTransitionBuffer"},
        {22, nullptr, "AcquireLastApplicationCaptureSharedBuffer"},
        {23, nullptr, "ReleaseLastApplicationCaptureSharedBuffer"},
        {24, nullptr, "AcquireLastForegroundCaptureSharedBuffer"},
        {25, nullptr, "ReleaseLastForegroundCaptureSharedBuffer"},
        {26, nullptr, "AcquireCallerAppletCaptureSharedBuffer"},
        {27, nullptr, "ReleaseCallerAppletCaptureSharedBuffer"},
        {28, nullptr, "TakeScreenShotOfOwnLayerEx"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IDisplayController::~IDisplayController() = default;

IDebugFunctions::IDebugFunctions(Core::System& system_)
    : ServiceFramework{system_, "IDebugFunctions"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "NotifyMessageToHomeMenuForDebug"},
        {1, nullptr, "OpenMainApplication"},
        {10, nullptr, "PerformSystemButtonPressing"},
        {20, nullptr, "InvalidateTransitionLayer"},
        {30, nullptr, "RequestLaunchApplicationWithUserAndArgumentForDebug"},
        {31, nullptr, "RequestLaunchApplicationByApplicationLaunchInfoForDebug"},
        {40, nullptr, "GetAppletResourceUsageInfo"},
        {50, nullptr, "AddSystemProgramIdAndAppletIdForDebug"},
        {51, nullptr, "AddOperationConfirmedLibraryAppletIdForDebug"},
        {100, nullptr, "SetCpuBoostModeForApplet"},
        {101, nullptr, "CancelCpuBoostModeForApplet"},
        {110, nullptr, "PushToAppletBoundChannelForDebug"},
        {111, nullptr, "TryPopFromAppletBoundChannelForDebug"},
        {120, nullptr, "AlarmSettingNotificationEnableAppEventReserve"},
        {121, nullptr, "AlarmSettingNotificationDisableAppEventReserve"},
        {122, nullptr, "AlarmSettingNotificationPushAppEventNotify"},
        {130, nullptr, "FriendInvitationSetApplicationParameter"},
        {131, nullptr, "FriendInvitationClearApplicationParameter"},
        {132, nullptr, "FriendInvitationPushApplicationParameter"},
        {140, nullptr, "RestrictPowerOperationForSecureLaunchModeForDebug"},
        {200, nullptr, "CreateFloatingLibraryAppletAccepterForDebug"},
        {300, nullptr, "TerminateAllRunningApplicationsForDebug"},
        {900, nullptr, "GetGrcProcessLaunchedSystemEvent"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IDebugFunctions::~IDebugFunctions() = default;

ISelfController::ISelfController(Core::System& system_, Nvnflinger::Nvnflinger& nvnflinger_)
    : ServiceFramework{system_, "ISelfController"}, nvnflinger{nvnflinger_},
      service_context{system, "ISelfController"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &ISelfController::Exit, "Exit"},
        {1, &ISelfController::LockExit, "LockExit"},
        {2, &ISelfController::UnlockExit, "UnlockExit"},
        {3, &ISelfController::EnterFatalSection, "EnterFatalSection"},
        {4, &ISelfController::LeaveFatalSection, "LeaveFatalSection"},
        {9, &ISelfController::GetLibraryAppletLaunchableEvent, "GetLibraryAppletLaunchableEvent"},
        {10, &ISelfController::SetScreenShotPermission, "SetScreenShotPermission"},
        {11, &ISelfController::SetOperationModeChangedNotification, "SetOperationModeChangedNotification"},
        {12, &ISelfController::SetPerformanceModeChangedNotification, "SetPerformanceModeChangedNotification"},
        {13, &ISelfController::SetFocusHandlingMode, "SetFocusHandlingMode"},
        {14, &ISelfController::SetRestartMessageEnabled, "SetRestartMessageEnabled"},
        {15, nullptr, "SetScreenShotAppletIdentityInfo"},
        {16, &ISelfController::SetOutOfFocusSuspendingEnabled, "SetOutOfFocusSuspendingEnabled"},
        {17, nullptr, "SetControllerFirmwareUpdateSection"},
        {18, nullptr, "SetRequiresCaptureButtonShortPressedMessage"},
        {19, &ISelfController::SetAlbumImageOrientation, "SetAlbumImageOrientation"},
        {20, nullptr, "SetDesirableKeyboardLayout"},
        {21, nullptr, "GetScreenShotProgramId"},
        {40, &ISelfController::CreateManagedDisplayLayer, "CreateManagedDisplayLayer"},
        {41, nullptr, "IsSystemBufferSharingEnabled"},
        {42, nullptr, "GetSystemSharedLayerHandle"},
        {43, nullptr, "GetSystemSharedBufferHandle"},
        {44, &ISelfController::CreateManagedDisplaySeparableLayer, "CreateManagedDisplaySeparableLayer"},
        {45, nullptr, "SetManagedDisplayLayerSeparationMode"},
        {46, nullptr, "SetRecordingLayerCompositionEnabled"},
        {50, &ISelfController::SetHandlesRequestToDisplay, "SetHandlesRequestToDisplay"},
        {51, nullptr, "ApproveToDisplay"},
        {60, nullptr, "OverrideAutoSleepTimeAndDimmingTime"},
        {61, nullptr, "SetMediaPlaybackState"},
        {62, &ISelfController::SetIdleTimeDetectionExtension, "SetIdleTimeDetectionExtension"},
        {63, &ISelfController::GetIdleTimeDetectionExtension, "GetIdleTimeDetectionExtension"},
        {64, nullptr, "SetInputDetectionSourceSet"},
        {65, &ISelfController::ReportUserIsActive, "ReportUserIsActive"},
        {66, nullptr, "GetCurrentIlluminance"},
        {67, nullptr, "IsIlluminanceAvailable"},
        {68, &ISelfController::SetAutoSleepDisabled, "SetAutoSleepDisabled"},
        {69, &ISelfController::IsAutoSleepDisabled, "IsAutoSleepDisabled"},
        {70, nullptr, "ReportMultimediaError"},
        {71, nullptr, "GetCurrentIlluminanceEx"},
        {72, nullptr, "SetInputDetectionPolicy"},
        {80, nullptr, "SetWirelessPriorityMode"},
        {90, &ISelfController::GetAccumulatedSuspendedTickValue, "GetAccumulatedSuspendedTickValue"},
        {91, &ISelfController::GetAccumulatedSuspendedTickChangedEvent, "GetAccumulatedSuspendedTickChangedEvent"},
        {100, &ISelfController::SetAlbumImageTakenNotificationEnabled, "SetAlbumImageTakenNotificationEnabled"},
        {110, nullptr, "SetApplicationAlbumUserData"},
        {120, &ISelfController::SaveCurrentScreenshot, "SaveCurrentScreenshot"},
        {130, &ISelfController::SetRecordVolumeMuted, "SetRecordVolumeMuted"},
        {1000, nullptr, "GetDebugStorageChannel"},
    };
    // clang-format on

    RegisterHandlers(functions);

    launchable_event = service_context.CreateEvent("ISelfController:LaunchableEvent");

    // This event is created by AM on the first time GetAccumulatedSuspendedTickChangedEvent() is
    // called. Yuzu can just create it unconditionally, since it doesn't need to support multiple
    // ISelfControllers. The event is signaled on creation, and on transition from suspended -> not
    // suspended if the event has previously been created by a call to
    // GetAccumulatedSuspendedTickChangedEvent.

    accumulated_suspended_tick_changed_event =
        service_context.CreateEvent("ISelfController:AccumulatedSuspendedTickChangedEvent");
    accumulated_suspended_tick_changed_event->Signal();
}

ISelfController::~ISelfController() {
    service_context.CloseEvent(launchable_event);
    service_context.CloseEvent(accumulated_suspended_tick_changed_event);
}

void ISelfController::Exit(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);

    system.Exit();
}

void ISelfController::LockExit(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    system.SetExitLock(true);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::UnlockExit(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    system.SetExitLock(false);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::EnterFatalSection(HLERequestContext& ctx) {
    ++num_fatal_sections_entered;
    LOG_DEBUG(Service_AM, "called. Num fatal sections entered: {}", num_fatal_sections_entered);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::LeaveFatalSection(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called.");

    // Entry and exit of fatal sections must be balanced.
    if (num_fatal_sections_entered == 0) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(Result{ErrorModule::AM, 512});
        return;
    }

    --num_fatal_sections_entered;

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::GetLibraryAppletLaunchableEvent(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    launchable_event->Signal();

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(launchable_event->GetReadableEvent());
}

void ISelfController::SetScreenShotPermission(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto permission = rp.PopEnum<ScreenshotPermission>();
    LOG_DEBUG(Service_AM, "called, permission={}", permission);

    screenshot_permission = permission;

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetOperationModeChangedNotification(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    bool flag = rp.Pop<bool>();
    LOG_WARNING(Service_AM, "(STUBBED) called flag={}", flag);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetPerformanceModeChangedNotification(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    bool flag = rp.Pop<bool>();
    LOG_WARNING(Service_AM, "(STUBBED) called flag={}", flag);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetFocusHandlingMode(HLERequestContext& ctx) {
    // Takes 3 input u8s with each field located immediately after the previous
    // u8, these are bool flags. No output.
    IPC::RequestParser rp{ctx};

    struct FocusHandlingModeParams {
        u8 unknown0;
        u8 unknown1;
        u8 unknown2;
    };
    const auto flags = rp.PopRaw<FocusHandlingModeParams>();

    LOG_WARNING(Service_AM, "(STUBBED) called. unknown0={}, unknown1={}, unknown2={}",
                flags.unknown0, flags.unknown1, flags.unknown2);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetRestartMessageEnabled(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetOutOfFocusSuspendingEnabled(HLERequestContext& ctx) {
    // Takes 3 input u8s with each field located immediately after the previous
    // u8, these are bool flags. No output.
    IPC::RequestParser rp{ctx};

    bool enabled = rp.Pop<bool>();
    LOG_WARNING(Service_AM, "(STUBBED) called enabled={}", enabled);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetAlbumImageOrientation(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::CreateManagedDisplayLayer(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    // TODO(Subv): Find out how AM determines the display to use, for now just
    // create the layer in the Default display.
    const auto display_id = nvnflinger.OpenDisplay("Default");
    const auto layer_id = nvnflinger.CreateLayer(*display_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(*layer_id);
}

void ISelfController::CreateManagedDisplaySeparableLayer(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    // TODO(Subv): Find out how AM determines the display to use, for now just
    // create the layer in the Default display.
    // This calls nn::vi::CreateRecordingLayer() which creates another layer.
    // Currently we do not support more than 1 layer per display, output 1 layer id for now.
    // Outputting 1 layer id instead of the expected 2 has not been observed to cause any adverse
    // side effects.
    // TODO: Support multiple layers
    const auto display_id = nvnflinger.OpenDisplay("Default");
    const auto layer_id = nvnflinger.CreateLayer(*display_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(*layer_id);
}

void ISelfController::SetHandlesRequestToDisplay(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetIdleTimeDetectionExtension(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    idle_time_detection_extension = rp.Pop<u32>();
    LOG_WARNING(Service_AM, "(STUBBED) called idle_time_detection_extension={}",
                idle_time_detection_extension);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::GetIdleTimeDetectionExtension(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(idle_time_detection_extension);
}

void ISelfController::ReportUserIsActive(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetAutoSleepDisabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    is_auto_sleep_disabled = rp.Pop<bool>();

    // On the system itself, if the previous state of is_auto_sleep_disabled
    // differed from the current value passed in, it'd signify the internal
    // window manager to update (and also increment some statistics like update counts)
    //
    // It'd also indicate this change to an idle handling context.
    //
    // However, given we're emulating this behavior, most of this can be ignored
    // and it's sufficient to simply set the member variable for querying via
    // IsAutoSleepDisabled().

    LOG_DEBUG(Service_AM, "called. is_auto_sleep_disabled={}", is_auto_sleep_disabled);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::IsAutoSleepDisabled(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called.");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(is_auto_sleep_disabled);
}

void ISelfController::GetAccumulatedSuspendedTickValue(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called.");

    // This command returns the total number of system ticks since ISelfController creation
    // where the game was suspended. Since Yuzu doesn't implement game suspension, this command
    // can just always return 0 ticks.
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<u64>(0);
}

void ISelfController::GetAccumulatedSuspendedTickChangedEvent(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called.");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(accumulated_suspended_tick_changed_event->GetReadableEvent());
}

void ISelfController::SetAlbumImageTakenNotificationEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    // This service call sets an internal flag whether a notification is shown when an image is
    // captured. Currently we do not support capturing images via the capture button, so this can be
    // stubbed for now.
    const bool album_image_taken_notification_enabled = rp.Pop<bool>();

    LOG_WARNING(Service_AM, "(STUBBED) called. album_image_taken_notification_enabled={}",
                album_image_taken_notification_enabled);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SaveCurrentScreenshot(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const auto album_report_option = rp.PopEnum<Capture::AlbumReportOption>();

    LOG_WARNING(Service_AM, "(STUBBED) called. album_report_option={}", album_report_option);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ISelfController::SetRecordVolumeMuted(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const auto is_record_volume_muted = rp.Pop<bool>();

    LOG_WARNING(Service_AM, "(STUBBED) called. is_record_volume_muted={}", is_record_volume_muted);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

AppletMessageQueue::AppletMessageQueue(Core::System& system)
    : service_context{system, "AppletMessageQueue"} {
    on_new_message = service_context.CreateEvent("AMMessageQueue:OnMessageReceived");
    on_operation_mode_changed = service_context.CreateEvent("AMMessageQueue:OperationModeChanged");
}

AppletMessageQueue::~AppletMessageQueue() {
    service_context.CloseEvent(on_new_message);
    service_context.CloseEvent(on_operation_mode_changed);
}

Kernel::KReadableEvent& AppletMessageQueue::GetMessageReceiveEvent() {
    return on_new_message->GetReadableEvent();
}

Kernel::KReadableEvent& AppletMessageQueue::GetOperationModeChangedEvent() {
    return on_operation_mode_changed->GetReadableEvent();
}

void AppletMessageQueue::PushMessage(AppletMessage msg) {
    messages.push(msg);
    on_new_message->Signal();
}

AppletMessageQueue::AppletMessage AppletMessageQueue::PopMessage() {
    if (messages.empty()) {
        on_new_message->Clear();
        return AppletMessage::None;
    }
    auto msg = messages.front();
    messages.pop();
    if (messages.empty()) {
        on_new_message->Clear();
    }
    return msg;
}

std::size_t AppletMessageQueue::GetMessageCount() const {
    return messages.size();
}

void AppletMessageQueue::RequestExit() {
    PushMessage(AppletMessage::Exit);
}

void AppletMessageQueue::RequestResume() {
    PushMessage(AppletMessage::Resume);
}

void AppletMessageQueue::FocusStateChanged() {
    PushMessage(AppletMessage::FocusStateChanged);
}

void AppletMessageQueue::OperationModeChanged() {
    PushMessage(AppletMessage::OperationModeChanged);
    PushMessage(AppletMessage::PerformanceModeChanged);
    on_operation_mode_changed->Signal();
}

ICommonStateGetter::ICommonStateGetter(Core::System& system_,
                                       std::shared_ptr<AppletMessageQueue> msg_queue_)
    : ServiceFramework{system_, "ICommonStateGetter"}, msg_queue{std::move(msg_queue_)} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &ICommonStateGetter::GetEventHandle, "GetEventHandle"},
        {1, &ICommonStateGetter::ReceiveMessage, "ReceiveMessage"},
        {2, nullptr, "GetThisAppletKind"},
        {3, nullptr, "AllowToEnterSleep"},
        {4, nullptr, "DisallowToEnterSleep"},
        {5, &ICommonStateGetter::GetOperationMode, "GetOperationMode"},
        {6, &ICommonStateGetter::GetPerformanceMode, "GetPerformanceMode"},
        {7, nullptr, "GetCradleStatus"},
        {8, &ICommonStateGetter::GetBootMode, "GetBootMode"},
        {9, &ICommonStateGetter::GetCurrentFocusState, "GetCurrentFocusState"},
        {10, nullptr, "RequestToAcquireSleepLock"},
        {11, nullptr, "ReleaseSleepLock"},
        {12, nullptr, "ReleaseSleepLockTransiently"},
        {13, nullptr, "GetAcquiredSleepLockEvent"},
        {14, nullptr, "GetWakeupCount"},
        {20, nullptr, "PushToGeneralChannel"},
        {30, nullptr, "GetHomeButtonReaderLockAccessor"},
        {31, nullptr, "GetReaderLockAccessorEx"},
        {32, nullptr, "GetWriterLockAccessorEx"},
        {40, nullptr, "GetCradleFwVersion"},
        {50, &ICommonStateGetter::IsVrModeEnabled, "IsVrModeEnabled"},
        {51, &ICommonStateGetter::SetVrModeEnabled, "SetVrModeEnabled"},
        {52, &ICommonStateGetter::SetLcdBacklighOffEnabled, "SetLcdBacklighOffEnabled"},
        {53, &ICommonStateGetter::BeginVrModeEx, "BeginVrModeEx"},
        {54, &ICommonStateGetter::EndVrModeEx, "EndVrModeEx"},
        {55, nullptr, "IsInControllerFirmwareUpdateSection"},
        {59, nullptr, "SetVrPositionForDebug"},
        {60, &ICommonStateGetter::GetDefaultDisplayResolution, "GetDefaultDisplayResolution"},
        {61, &ICommonStateGetter::GetDefaultDisplayResolutionChangeEvent, "GetDefaultDisplayResolutionChangeEvent"},
        {62, nullptr, "GetHdcpAuthenticationState"},
        {63, nullptr, "GetHdcpAuthenticationStateChangeEvent"},
        {64, nullptr, "SetTvPowerStateMatchingMode"},
        {65, nullptr, "GetApplicationIdByContentActionName"},
        {66, &ICommonStateGetter::SetCpuBoostMode, "SetCpuBoostMode"},
        {67, nullptr, "CancelCpuBoostMode"},
        {68, nullptr, "GetBuiltInDisplayType"},
        {80, &ICommonStateGetter::PerformSystemButtonPressingIfInFocus, "PerformSystemButtonPressingIfInFocus"},
        {90, nullptr, "SetPerformanceConfigurationChangedNotification"},
        {91, nullptr, "GetCurrentPerformanceConfiguration"},
        {100, nullptr, "SetHandlingHomeButtonShortPressedEnabled"},
        {110, nullptr, "OpenMyGpuErrorHandler"},
        {120, nullptr, "GetAppletLaunchedHistory"},
        {200, nullptr, "GetOperationModeSystemInfo"},
        {300, nullptr, "GetSettingsPlatformRegion"},
        {400, nullptr, "ActivateMigrationService"},
        {401, nullptr, "DeactivateMigrationService"},
        {500, nullptr, "DisableSleepTillShutdown"},
        {501, nullptr, "SuppressDisablingSleepTemporarily"},
        {502, nullptr, "IsSleepEnabled"},
        {503, nullptr, "IsDisablingSleepSuppressed"},
        {900, &ICommonStateGetter::SetRequestExitToLibraryAppletAtExecuteNextProgramEnabled, "SetRequestExitToLibraryAppletAtExecuteNextProgramEnabled"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

ICommonStateGetter::~ICommonStateGetter() = default;

void ICommonStateGetter::GetBootMode(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u8>(static_cast<u8>(Service::PM::SystemBootMode::Normal)); // Normal boot mode
}

void ICommonStateGetter::GetEventHandle(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(msg_queue->GetMessageReceiveEvent());
}

void ICommonStateGetter::ReceiveMessage(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    const auto message = msg_queue->PopMessage();
    IPC::ResponseBuilder rb{ctx, 3};

    if (message == AppletMessageQueue::AppletMessage::None) {
        LOG_ERROR(Service_AM, "Message queue is empty");
        rb.Push(AM::ResultNoMessages);
        rb.PushEnum<AppletMessageQueue::AppletMessage>(message);
        return;
    }

    rb.Push(ResultSuccess);
    rb.PushEnum<AppletMessageQueue::AppletMessage>(message);
}

void ICommonStateGetter::GetCurrentFocusState(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<u8>(FocusState::InFocus));
}

void ICommonStateGetter::IsVrModeEnabled(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(vr_mode_state);
}

void ICommonStateGetter::SetVrModeEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    vr_mode_state = rp.Pop<bool>();

    LOG_WARNING(Service_AM, "VR Mode is {}", vr_mode_state ? "on" : "off");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ICommonStateGetter::SetLcdBacklighOffEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto is_lcd_backlight_off_enabled = rp.Pop<bool>();

    LOG_WARNING(Service_AM, "(STUBBED) called. is_lcd_backlight_off_enabled={}",
                is_lcd_backlight_off_enabled);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ICommonStateGetter::BeginVrModeEx(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ICommonStateGetter::EndVrModeEx(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ICommonStateGetter::GetDefaultDisplayResolutionChangeEvent(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(msg_queue->GetOperationModeChangedEvent());
}

void ICommonStateGetter::GetDefaultDisplayResolution(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);

    if (Settings::values.use_docked_mode.GetValue()) {
        rb.Push(static_cast<u32>(Service::VI::DisplayResolution::DockedWidth));
        rb.Push(static_cast<u32>(Service::VI::DisplayResolution::DockedHeight));
    } else {
        rb.Push(static_cast<u32>(Service::VI::DisplayResolution::UndockedWidth));
        rb.Push(static_cast<u32>(Service::VI::DisplayResolution::UndockedHeight));
    }
}

void ICommonStateGetter::SetCpuBoostMode(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called, forwarding to APM:SYS");

    const auto& sm = system.ServiceManager();
    const auto apm_sys = sm.GetService<APM::APM_Sys>("apm:sys");
    ASSERT(apm_sys != nullptr);

    apm_sys->SetCpuBoostMode(ctx);
}

void ICommonStateGetter::PerformSystemButtonPressingIfInFocus(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto system_button{rp.PopEnum<SystemButtonType>()};

    LOG_WARNING(Service_AM, "(STUBBED) called, system_button={}", system_button);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ICommonStateGetter::SetRequestExitToLibraryAppletAtExecuteNextProgramEnabled(
    HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

IStorageImpl::~IStorageImpl() = default;

class StorageDataImpl final : public IStorageImpl {
public:
    explicit StorageDataImpl(std::vector<u8>&& buffer_) : buffer{std::move(buffer_)} {}

    std::vector<u8>& GetData() override {
        return buffer;
    }

    const std::vector<u8>& GetData() const override {
        return buffer;
    }

    std::size_t GetSize() const override {
        return buffer.size();
    }

private:
    std::vector<u8> buffer;
};

IStorage::IStorage(Core::System& system_, std::vector<u8>&& buffer)
    : ServiceFramework{system_, "IStorage"}, impl{std::make_shared<StorageDataImpl>(
                                                 std::move(buffer))} {
    Register();
}

void IStorage::Register() {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IStorage::Open, "Open"},
            {1, nullptr, "OpenTransferStorage"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

IStorage::~IStorage() = default;

void IStorage::Open(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IStorageAccessor>(system, *this);
}

void ICommonStateGetter::GetOperationMode(HLERequestContext& ctx) {
    const bool use_docked_mode{Settings::values.use_docked_mode.GetValue()};
    LOG_DEBUG(Service_AM, "called, use_docked_mode={}", use_docked_mode);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<u8>(use_docked_mode ? OperationMode::Docked : OperationMode::Handheld));
}

void ICommonStateGetter::GetPerformanceMode(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(system.GetAPMController().GetCurrentPerformanceMode());
}

class ILibraryAppletAccessor final : public ServiceFramework<ILibraryAppletAccessor> {
public:
    explicit ILibraryAppletAccessor(Core::System& system_, std::shared_ptr<Applets::Applet> applet_)
        : ServiceFramework{system_, "ILibraryAppletAccessor"}, applet{std::move(applet_)} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &ILibraryAppletAccessor::GetAppletStateChangedEvent, "GetAppletStateChangedEvent"},
            {1, &ILibraryAppletAccessor::IsCompleted, "IsCompleted"},
            {10, &ILibraryAppletAccessor::Start, "Start"},
            {20, nullptr, "RequestExit"},
            {25, nullptr, "Terminate"},
            {30, &ILibraryAppletAccessor::GetResult, "GetResult"},
            {50, nullptr, "SetOutOfFocusApplicationSuspendingEnabled"},
            {60, &ILibraryAppletAccessor::PresetLibraryAppletGpuTimeSliceZero, "PresetLibraryAppletGpuTimeSliceZero"},
            {100, &ILibraryAppletAccessor::PushInData, "PushInData"},
            {101, &ILibraryAppletAccessor::PopOutData, "PopOutData"},
            {102, nullptr, "PushExtraStorage"},
            {103, &ILibraryAppletAccessor::PushInteractiveInData, "PushInteractiveInData"},
            {104, &ILibraryAppletAccessor::PopInteractiveOutData, "PopInteractiveOutData"},
            {105, &ILibraryAppletAccessor::GetPopOutDataEvent, "GetPopOutDataEvent"},
            {106, &ILibraryAppletAccessor::GetPopInteractiveOutDataEvent, "GetPopInteractiveOutDataEvent"},
            {110, nullptr, "NeedsToExitProcess"},
            {120, nullptr, "GetLibraryAppletInfo"},
            {150, nullptr, "RequestForAppletToGetForeground"},
            {160, &ILibraryAppletAccessor::GetIndirectLayerConsumerHandle, "GetIndirectLayerConsumerHandle"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetAppletStateChangedEvent(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(applet->GetBroker().GetStateChangedEvent());
    }

    void IsCompleted(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(applet->TransactionComplete());
    }

    void GetResult(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(applet->GetStatus());
    }

    void PresetLibraryAppletGpuTimeSliceZero(HLERequestContext& ctx) {
        LOG_WARNING(Service_AM, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Start(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        ASSERT(applet != nullptr);

        applet->Initialize();
        applet->Execute();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void PushInData(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::RequestParser rp{ctx};
        applet->GetBroker().PushNormalDataFromGame(rp.PopIpcInterface<IStorage>().lock());

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void PopOutData(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        auto storage = applet->GetBroker().PopNormalDataToGame();
        if (storage == nullptr) {
            LOG_DEBUG(Service_AM,
                      "storage is a nullptr. There is no data in the current normal channel");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(AM::ResultNoDataInChannel);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IStorage>(std::move(storage));
    }

    void PushInteractiveInData(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::RequestParser rp{ctx};
        applet->GetBroker().PushInteractiveDataFromGame(rp.PopIpcInterface<IStorage>().lock());

        ASSERT(applet->IsInitialized());
        applet->ExecuteInteractive();
        applet->Execute();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void PopInteractiveOutData(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        auto storage = applet->GetBroker().PopInteractiveDataToGame();
        if (storage == nullptr) {
            LOG_DEBUG(Service_AM,
                      "storage is a nullptr. There is no data in the current interactive channel");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(AM::ResultNoDataInChannel);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IStorage>(std::move(storage));
    }

    void GetPopOutDataEvent(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(applet->GetBroker().GetNormalDataEvent());
    }

    void GetPopInteractiveOutDataEvent(HLERequestContext& ctx) {
        LOG_DEBUG(Service_AM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(applet->GetBroker().GetInteractiveDataEvent());
    }

    void GetIndirectLayerConsumerHandle(HLERequestContext& ctx) {
        LOG_WARNING(Service_AM, "(STUBBED) called");

        // We require a non-zero handle to be valid. Using 0xdeadbeef allows us to trace if this is
        // actually used anywhere
        constexpr u64 handle = 0xdeadbeef;

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(handle);
    }

    std::shared_ptr<Applets::Applet> applet;
};

IStorageAccessor::IStorageAccessor(Core::System& system_, IStorage& backing_)
    : ServiceFramework{system_, "IStorageAccessor"}, backing{backing_} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IStorageAccessor::GetSize, "GetSize"},
            {10, &IStorageAccessor::Write, "Write"},
            {11, &IStorageAccessor::Read, "Read"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

IStorageAccessor::~IStorageAccessor() = default;

void IStorageAccessor::GetSize(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(ResultSuccess);
    rb.Push(static_cast<u64>(backing.GetSize()));
}

void IStorageAccessor::Write(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const u64 offset{rp.Pop<u64>()};
    const auto data{ctx.ReadBuffer()};
    const std::size_t size{std::min<u64>(data.size(), backing.GetSize() - offset)};

    LOG_DEBUG(Service_AM, "called, offset={}, size={}", offset, size);

    if (offset > backing.GetSize()) {
        LOG_ERROR(Service_AM,
                  "offset is out of bounds, backing_buffer_sz={}, data_size={}, offset={}",
                  backing.GetSize(), size, offset);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(AM::ResultInvalidOffset);
        return;
    }

    std::memcpy(backing.GetData().data() + offset, data.data(), size);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IStorageAccessor::Read(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const u64 offset{rp.Pop<u64>()};
    const std::size_t size{std::min<u64>(ctx.GetWriteBufferSize(), backing.GetSize() - offset)};

    LOG_DEBUG(Service_AM, "called, offset={}, size={}", offset, size);

    if (offset > backing.GetSize()) {
        LOG_ERROR(Service_AM, "offset is out of bounds, backing_buffer_sz={}, size={}, offset={}",
                  backing.GetSize(), size, offset);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(AM::ResultInvalidOffset);
        return;
    }

    ctx.WriteBuffer(backing.GetData().data() + offset, size);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

ILibraryAppletCreator::ILibraryAppletCreator(Core::System& system_)
    : ServiceFramework{system_, "ILibraryAppletCreator"} {
    static const FunctionInfo functions[] = {
        {0, &ILibraryAppletCreator::CreateLibraryApplet, "CreateLibraryApplet"},
        {1, nullptr, "TerminateAllLibraryApplets"},
        {2, nullptr, "AreAnyLibraryAppletsLeft"},
        {10, &ILibraryAppletCreator::CreateStorage, "CreateStorage"},
        {11, &ILibraryAppletCreator::CreateTransferMemoryStorage, "CreateTransferMemoryStorage"},
        {12, &ILibraryAppletCreator::CreateHandleStorage, "CreateHandleStorage"},
    };
    RegisterHandlers(functions);
}

ILibraryAppletCreator::~ILibraryAppletCreator() = default;

void ILibraryAppletCreator::CreateLibraryApplet(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const auto applet_id = rp.PopRaw<Applets::AppletId>();
    const auto applet_mode = rp.PopRaw<Applets::LibraryAppletMode>();

    LOG_DEBUG(Service_AM, "called with applet_id={:08X}, applet_mode={:08X}", applet_id,
              applet_mode);

    const auto& applet_manager{system.GetAppletManager()};
    const auto applet = applet_manager.GetApplet(applet_id, applet_mode);

    if (applet == nullptr) {
        LOG_ERROR(Service_AM, "Applet doesn't exist! applet_id={}", applet_id);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ILibraryAppletAccessor>(system, applet);
}

void ILibraryAppletCreator::CreateStorage(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s64 size{rp.Pop<s64>()};

    LOG_DEBUG(Service_AM, "called, size={}", size);

    if (size <= 0) {
        LOG_ERROR(Service_AM, "size is less than or equal to 0");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    std::vector<u8> buffer(size);

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IStorage>(system, std::move(buffer));
}

void ILibraryAppletCreator::CreateTransferMemoryStorage(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    struct Parameters {
        u8 permissions;
        s64 size;
    };

    const auto parameters{rp.PopRaw<Parameters>()};
    const auto handle{ctx.GetCopyHandle(0)};

    LOG_DEBUG(Service_AM, "called, permissions={}, size={}, handle={:08X}", parameters.permissions,
              parameters.size, handle);

    if (parameters.size <= 0) {
        LOG_ERROR(Service_AM, "size is less than or equal to 0");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    auto transfer_mem =
        system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(handle);

    if (transfer_mem.IsNull()) {
        LOG_ERROR(Service_AM, "transfer_mem is a nullptr for handle={:08X}", handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    std::vector<u8> memory(transfer_mem->GetSize());
    system.ApplicationMemory().ReadBlock(transfer_mem->GetSourceAddress(), memory.data(),
                                         memory.size());

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IStorage>(system, std::move(memory));
}

void ILibraryAppletCreator::CreateHandleStorage(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s64 size{rp.Pop<s64>()};
    const auto handle{ctx.GetCopyHandle(0)};

    LOG_DEBUG(Service_AM, "called, size={}, handle={:08X}", size, handle);

    if (size <= 0) {
        LOG_ERROR(Service_AM, "size is less than or equal to 0");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    auto transfer_mem =
        system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(handle);

    if (transfer_mem.IsNull()) {
        LOG_ERROR(Service_AM, "transfer_mem is a nullptr for handle={:08X}", handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    std::vector<u8> memory(transfer_mem->GetSize());
    system.ApplicationMemory().ReadBlock(transfer_mem->GetSourceAddress(), memory.data(),
                                         memory.size());

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IStorage>(system, std::move(memory));
}

IApplicationFunctions::IApplicationFunctions(Core::System& system_)
    : ServiceFramework{system_, "IApplicationFunctions"}, service_context{system,
                                                                          "IApplicationFunctions"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {1, &IApplicationFunctions::PopLaunchParameter, "PopLaunchParameter"},
        {10, nullptr, "CreateApplicationAndPushAndRequestToStart"},
        {11, nullptr, "CreateApplicationAndPushAndRequestToStartForQuest"},
        {12, nullptr, "CreateApplicationAndRequestToStart"},
        {13, &IApplicationFunctions::CreateApplicationAndRequestToStartForQuest, "CreateApplicationAndRequestToStartForQuest"},
        {14, nullptr, "CreateApplicationWithAttributeAndPushAndRequestToStartForQuest"},
        {15, nullptr, "CreateApplicationWithAttributeAndRequestToStartForQuest"},
        {20, &IApplicationFunctions::EnsureSaveData, "EnsureSaveData"},
        {21, &IApplicationFunctions::GetDesiredLanguage, "GetDesiredLanguage"},
        {22, &IApplicationFunctions::SetTerminateResult, "SetTerminateResult"},
        {23, &IApplicationFunctions::GetDisplayVersion, "GetDisplayVersion"},
        {24, nullptr, "GetLaunchStorageInfoForDebug"},
        {25, &IApplicationFunctions::ExtendSaveData, "ExtendSaveData"},
        {26, &IApplicationFunctions::GetSaveDataSize, "GetSaveDataSize"},
        {27, nullptr, "CreateCacheStorage"},
        {28, nullptr, "GetSaveDataSizeMax"},
        {29, nullptr, "GetCacheStorageMax"},
        {30, &IApplicationFunctions::BeginBlockingHomeButtonShortAndLongPressed, "BeginBlockingHomeButtonShortAndLongPressed"},
        {31, &IApplicationFunctions::EndBlockingHomeButtonShortAndLongPressed, "EndBlockingHomeButtonShortAndLongPressed"},
        {32, &IApplicationFunctions::BeginBlockingHomeButton, "BeginBlockingHomeButton"},
        {33, &IApplicationFunctions::EndBlockingHomeButton, "EndBlockingHomeButton"},
        {34, nullptr, "SelectApplicationLicense"},
        {35, nullptr, "GetDeviceSaveDataSizeMax"},
        {36, nullptr, "GetLimitedApplicationLicense"},
        {37, nullptr, "GetLimitedApplicationLicenseUpgradableEvent"},
        {40, &IApplicationFunctions::NotifyRunning, "NotifyRunning"},
        {50, &IApplicationFunctions::GetPseudoDeviceId, "GetPseudoDeviceId"},
        {60, nullptr, "SetMediaPlaybackStateForApplication"},
        {65, &IApplicationFunctions::IsGamePlayRecordingSupported, "IsGamePlayRecordingSupported"},
        {66, &IApplicationFunctions::InitializeGamePlayRecording, "InitializeGamePlayRecording"},
        {67, &IApplicationFunctions::SetGamePlayRecordingState, "SetGamePlayRecordingState"},
        {68, nullptr, "RequestFlushGamePlayingMovieForDebug"},
        {70, nullptr, "RequestToShutdown"},
        {71, nullptr, "RequestToReboot"},
        {72, nullptr, "RequestToSleep"},
        {80, nullptr, "ExitAndRequestToShowThanksMessage"},
        {90, &IApplicationFunctions::EnableApplicationCrashReport, "EnableApplicationCrashReport"},
        {100, &IApplicationFunctions::InitializeApplicationCopyrightFrameBuffer, "InitializeApplicationCopyrightFrameBuffer"},
        {101, &IApplicationFunctions::SetApplicationCopyrightImage, "SetApplicationCopyrightImage"},
        {102, &IApplicationFunctions::SetApplicationCopyrightVisibility, "SetApplicationCopyrightVisibility"},
        {110, &IApplicationFunctions::QueryApplicationPlayStatistics, "QueryApplicationPlayStatistics"},
        {111, &IApplicationFunctions::QueryApplicationPlayStatisticsByUid, "QueryApplicationPlayStatisticsByUid"},
        {120, &IApplicationFunctions::ExecuteProgram, "ExecuteProgram"},
        {121, &IApplicationFunctions::ClearUserChannel, "ClearUserChannel"},
        {122, &IApplicationFunctions::UnpopToUserChannel, "UnpopToUserChannel"},
        {123, &IApplicationFunctions::GetPreviousProgramIndex, "GetPreviousProgramIndex"},
        {124, nullptr, "EnableApplicationAllThreadDumpOnCrash"},
        {130, &IApplicationFunctions::GetGpuErrorDetectedSystemEvent, "GetGpuErrorDetectedSystemEvent"},
        {131, nullptr, "SetDelayTimeToAbortOnGpuError"},
        {140, &IApplicationFunctions::GetFriendInvitationStorageChannelEvent, "GetFriendInvitationStorageChannelEvent"},
        {141, &IApplicationFunctions::TryPopFromFriendInvitationStorageChannel, "TryPopFromFriendInvitationStorageChannel"},
        {150, &IApplicationFunctions::GetNotificationStorageChannelEvent, "GetNotificationStorageChannelEvent"},
        {151, nullptr, "TryPopFromNotificationStorageChannel"},
        {160, &IApplicationFunctions::GetHealthWarningDisappearedSystemEvent, "GetHealthWarningDisappearedSystemEvent"},
        {170, nullptr, "SetHdcpAuthenticationActivated"},
        {180, nullptr, "GetLaunchRequiredVersion"},
        {181, nullptr, "UpgradeLaunchRequiredVersion"},
        {190, nullptr, "SendServerMaintenanceOverlayNotification"},
        {200, nullptr, "GetLastApplicationExitReason"},
        {500, nullptr, "StartContinuousRecordingFlushForDebug"},
        {1000, nullptr, "CreateMovieMaker"},
        {1001, &IApplicationFunctions::PrepareForJit, "PrepareForJit"},
    };
    // clang-format on

    RegisterHandlers(functions);

    gpu_error_detected_event =
        service_context.CreateEvent("IApplicationFunctions:GpuErrorDetectedSystemEvent");
    friend_invitation_storage_channel_event =
        service_context.CreateEvent("IApplicationFunctions:FriendInvitationStorageChannelEvent");
    notification_storage_channel_event =
        service_context.CreateEvent("IApplicationFunctions:NotificationStorageChannelEvent");
    health_warning_disappeared_system_event =
        service_context.CreateEvent("IApplicationFunctions:HealthWarningDisappearedSystemEvent");
}

IApplicationFunctions::~IApplicationFunctions() {
    service_context.CloseEvent(gpu_error_detected_event);
    service_context.CloseEvent(friend_invitation_storage_channel_event);
    service_context.CloseEvent(notification_storage_channel_event);
    service_context.CloseEvent(health_warning_disappeared_system_event);
}

void IApplicationFunctions::EnableApplicationCrashReport(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::InitializeApplicationCopyrightFrameBuffer(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::SetApplicationCopyrightImage(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::SetApplicationCopyrightVisibility(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto is_visible = rp.Pop<bool>();

    LOG_WARNING(Service_AM, "(STUBBED) called, is_visible={}", is_visible);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::BeginBlockingHomeButtonShortAndLongPressed(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::EndBlockingHomeButtonShortAndLongPressed(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::BeginBlockingHomeButton(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::EndBlockingHomeButton(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::PopLaunchParameter(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto kind = rp.PopEnum<LaunchParameterKind>();

    LOG_DEBUG(Service_AM, "called, kind={:08X}", kind);

    if (kind == LaunchParameterKind::ApplicationSpecific && !launch_popped_application_specific) {
        const auto backend = BCAT::CreateBackendFromSettings(system, [this](u64 tid) {
            return system.GetFileSystemController().GetBCATDirectory(tid);
        });
        const auto build_id_full = system.GetApplicationProcessBuildID();
        u64 build_id{};
        std::memcpy(&build_id, build_id_full.data(), sizeof(u64));

        auto data =
            backend->GetLaunchParameter({system.GetApplicationProcessProgramID(), build_id});
        if (data.has_value()) {
            IPC::ResponseBuilder rb{ctx, 2, 0, 1};
            rb.Push(ResultSuccess);
            rb.PushIpcInterface<IStorage>(system, std::move(*data));
            launch_popped_application_specific = true;
            return;
        }
    } else if (kind == LaunchParameterKind::AccountPreselectedUser &&
               !launch_popped_account_preselect) {
        LaunchParameterAccountPreselectedUser params{};

        params.magic = LAUNCH_PARAMETER_ACCOUNT_PRESELECTED_USER_MAGIC;
        params.is_account_selected = 1;

        Account::ProfileManager profile_manager{};
        const auto uuid = profile_manager.GetUser(static_cast<s32>(Settings::values.current_user));
        ASSERT(uuid.has_value() && uuid->IsValid());
        params.current_user = *uuid;

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(ResultSuccess);

        std::vector<u8> buffer(sizeof(LaunchParameterAccountPreselectedUser));
        std::memcpy(buffer.data(), &params, buffer.size());

        rb.PushIpcInterface<IStorage>(system, std::move(buffer));
        launch_popped_account_preselect = true;
        return;
    }

    LOG_ERROR(Service_AM, "Attempted to load launch parameter but none was found!");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(AM::ResultNoDataInChannel);
}

void IApplicationFunctions::CreateApplicationAndRequestToStartForQuest(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::EnsureSaveData(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    u128 user_id = rp.PopRaw<u128>();

    LOG_DEBUG(Service_AM, "called, uid={:016X}{:016X}", user_id[1], user_id[0]);

    FileSys::SaveDataAttribute attribute{};
    attribute.title_id = system.GetApplicationProcessProgramID();
    attribute.user_id = user_id;
    attribute.type = FileSys::SaveDataType::SaveData;
    const auto res = system.GetFileSystemController().CreateSaveData(
        FileSys::SaveDataSpaceId::NandUser, attribute);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(res.Code());
    rb.Push<u64>(0);
}

void IApplicationFunctions::SetTerminateResult(HLERequestContext& ctx) {
    // Takes an input u32 Result, no output.
    // For example, in some cases official apps use this with error 0x2A2 then
    // uses svcBreak.

    IPC::RequestParser rp{ctx};
    u32 result = rp.Pop<u32>();
    LOG_WARNING(Service_AM, "(STUBBED) called, result=0x{:08X}", result);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::GetDisplayVersion(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    std::array<u8, 0x10> version_string{};

    const auto res = [this] {
        const auto title_id = system.GetApplicationProcessProgramID();

        const FileSys::PatchManager pm{title_id, system.GetFileSystemController(),
                                       system.GetContentProvider()};
        auto metadata = pm.GetControlMetadata();
        if (metadata.first != nullptr) {
            return metadata;
        }

        const FileSys::PatchManager pm_update{FileSys::GetUpdateTitleID(title_id),
                                              system.GetFileSystemController(),
                                              system.GetContentProvider()};
        return pm_update.GetControlMetadata();
    }();

    if (res.first != nullptr) {
        const auto& version = res.first->GetVersionString();
        std::copy(version.begin(), version.end(), version_string.begin());
    } else {
        static constexpr char default_version[]{"1.0.0"};
        std::memcpy(version_string.data(), default_version, sizeof(default_version));
    }

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.PushRaw(version_string);
}

void IApplicationFunctions::GetDesiredLanguage(HLERequestContext& ctx) {
    // TODO(bunnei): This should be configurable
    LOG_DEBUG(Service_AM, "called");

    // Get supported languages from NACP, if possible
    // Default to 0 (all languages supported)
    u32 supported_languages = 0;

    const auto res = [this] {
        const auto title_id = system.GetApplicationProcessProgramID();

        const FileSys::PatchManager pm{title_id, system.GetFileSystemController(),
                                       system.GetContentProvider()};
        auto metadata = pm.GetControlMetadata();
        if (metadata.first != nullptr) {
            return metadata;
        }

        const FileSys::PatchManager pm_update{FileSys::GetUpdateTitleID(title_id),
                                              system.GetFileSystemController(),
                                              system.GetContentProvider()};
        return pm_update.GetControlMetadata();
    }();

    if (res.first != nullptr) {
        supported_languages = res.first->GetSupportedLanguages();
    }

    // Call IApplicationManagerInterface implementation.
    auto& service_manager = system.ServiceManager();
    auto ns_am2 = service_manager.GetService<NS::NS>("ns:am2");
    auto app_man = ns_am2->GetApplicationManagerInterface();

    // Get desired application language
    const auto res_lang = app_man->GetApplicationDesiredLanguage(supported_languages);
    if (res_lang.Failed()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res_lang.Code());
        return;
    }

    // Convert to settings language code.
    const auto res_code = app_man->ConvertApplicationLanguageToLanguageCode(*res_lang);
    if (res_code.Failed()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(res_code.Code());
        return;
    }

    LOG_DEBUG(Service_AM, "got desired_language={:016X}", *res_code);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(*res_code);
}

void IApplicationFunctions::IsGamePlayRecordingSupported(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    constexpr bool gameplay_recording_supported = false;

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(gameplay_recording_supported);
}

void IApplicationFunctions::InitializeGamePlayRecording(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::SetGamePlayRecordingState(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::NotifyRunning(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u8>(0); // Unknown, seems to be ignored by official processes
}

void IApplicationFunctions::GetPseudoDeviceId(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);

    // Returns a 128-bit UUID
    rb.Push<u64>(0);
    rb.Push<u64>(0);
}

void IApplicationFunctions::ExtendSaveData(HLERequestContext& ctx) {
    struct Parameters {
        FileSys::SaveDataType type;
        u128 user_id;
        u64 new_normal_size;
        u64 new_journal_size;
    };
    static_assert(sizeof(Parameters) == 40);

    IPC::RequestParser rp{ctx};
    const auto [type, user_id, new_normal_size, new_journal_size] = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_AM,
              "called with type={:02X}, user_id={:016X}{:016X}, new_normal={:016X}, "
              "new_journal={:016X}",
              static_cast<u8>(type), user_id[1], user_id[0], new_normal_size, new_journal_size);

    system.GetFileSystemController().WriteSaveDataSize(
        type, system.GetApplicationProcessProgramID(), user_id,
        {new_normal_size, new_journal_size});

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);

    // The following value is used upon failure to help the system recover.
    // Since we always succeed, this should be 0.
    rb.Push<u64>(0);
}

void IApplicationFunctions::GetSaveDataSize(HLERequestContext& ctx) {
    struct Parameters {
        FileSys::SaveDataType type;
        u128 user_id;
    };
    static_assert(sizeof(Parameters) == 24);

    IPC::RequestParser rp{ctx};
    const auto [type, user_id] = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_AM, "called with type={:02X}, user_id={:016X}{:016X}", type, user_id[1],
              user_id[0]);

    const auto size = system.GetFileSystemController().ReadSaveDataSize(
        type, system.GetApplicationProcessProgramID(), user_id);

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.Push(size.normal);
    rb.Push(size.journal);
}

void IApplicationFunctions::QueryApplicationPlayStatistics(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(0);
}

void IApplicationFunctions::QueryApplicationPlayStatisticsByUid(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(0);
}

void IApplicationFunctions::ExecuteProgram(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::RequestParser rp{ctx};
    [[maybe_unused]] const auto unk_1 = rp.Pop<u32>();
    [[maybe_unused]] const auto unk_2 = rp.Pop<u32>();
    const auto program_index = rp.Pop<u64>();

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);

    system.ExecuteProgram(program_index);
}

void IApplicationFunctions::ClearUserChannel(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::UnpopToUserChannel(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IApplicationFunctions::GetPreviousProgramIndex(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<s32>(previous_program_index);
}

void IApplicationFunctions::GetGpuErrorDetectedSystemEvent(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(gpu_error_detected_event->GetReadableEvent());
}

void IApplicationFunctions::GetFriendInvitationStorageChannelEvent(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(friend_invitation_storage_channel_event->GetReadableEvent());
}

void IApplicationFunctions::TryPopFromFriendInvitationStorageChannel(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(AM::ResultNoDataInChannel);
}

void IApplicationFunctions::GetNotificationStorageChannelEvent(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(notification_storage_channel_event->GetReadableEvent());
}

void IApplicationFunctions::GetHealthWarningDisappearedSystemEvent(HLERequestContext& ctx) {
    LOG_DEBUG(Service_AM, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(health_warning_disappeared_system_event->GetReadableEvent());
}

void IApplicationFunctions::PrepareForJit(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void LoopProcess(Nvnflinger::Nvnflinger& nvnflinger, Core::System& system) {
    auto message_queue = std::make_shared<AppletMessageQueue>(system);
    // Needed on game boot
    message_queue->PushMessage(AppletMessageQueue::AppletMessage::FocusStateChanged);

    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService(
        "appletAE", std::make_shared<AppletAE>(nvnflinger, message_queue, system));
    server_manager->RegisterNamedService(
        "appletOE", std::make_shared<AppletOE>(nvnflinger, message_queue, system));
    server_manager->RegisterNamedService("idle:sys", std::make_shared<IdleSys>(system));
    server_manager->RegisterNamedService("omm", std::make_shared<OMM>(system));
    server_manager->RegisterNamedService("spsm", std::make_shared<SPSM>(system));
    ServerManager::RunServer(std::move(server_manager));
}

IHomeMenuFunctions::IHomeMenuFunctions(Core::System& system_)
    : ServiceFramework{system_, "IHomeMenuFunctions"}, service_context{system,
                                                                       "IHomeMenuFunctions"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {10, &IHomeMenuFunctions::RequestToGetForeground, "RequestToGetForeground"},
        {11, nullptr, "LockForeground"},
        {12, nullptr, "UnlockForeground"},
        {20, nullptr, "PopFromGeneralChannel"},
        {21, &IHomeMenuFunctions::GetPopFromGeneralChannelEvent, "GetPopFromGeneralChannelEvent"},
        {30, nullptr, "GetHomeButtonWriterLockAccessor"},
        {31, nullptr, "GetWriterLockAccessorEx"},
        {40, nullptr, "IsSleepEnabled"},
        {41, nullptr, "IsRebootEnabled"},
        {50, nullptr, "LaunchSystemApplet"},
        {51, nullptr, "LaunchStarter"},
        {100, nullptr, "PopRequestLaunchApplicationForDebug"},
        {110, nullptr, "IsForceTerminateApplicationDisabledForDebug"},
        {200, nullptr, "LaunchDevMenu"},
        {1000, nullptr, "SetLastApplicationExitReason"},
    };
    // clang-format on

    RegisterHandlers(functions);

    pop_from_general_channel_event =
        service_context.CreateEvent("IHomeMenuFunctions:PopFromGeneralChannelEvent");
}

IHomeMenuFunctions::~IHomeMenuFunctions() {
    service_context.CloseEvent(pop_from_general_channel_event);
}

void IHomeMenuFunctions::RequestToGetForeground(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IHomeMenuFunctions::GetPopFromGeneralChannelEvent(HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(pop_from_general_channel_event->GetReadableEvent());
}

IGlobalStateController::IGlobalStateController(Core::System& system_)
    : ServiceFramework{system_, "IGlobalStateController"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "RequestToEnterSleep"},
        {1, nullptr, "EnterSleep"},
        {2, nullptr, "StartSleepSequence"},
        {3, nullptr, "StartShutdownSequence"},
        {4, nullptr, "StartRebootSequence"},
        {9, nullptr, "IsAutoPowerDownRequested"},
        {10, nullptr, "LoadAndApplyIdlePolicySettings"},
        {11, nullptr, "NotifyCecSettingsChanged"},
        {12, nullptr, "SetDefaultHomeButtonLongPressTime"},
        {13, nullptr, "UpdateDefaultDisplayResolution"},
        {14, nullptr, "ShouldSleepOnBoot"},
        {15, nullptr, "GetHdcpAuthenticationFailedEvent"},
        {30, nullptr, "OpenCradleFirmwareUpdater"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IGlobalStateController::~IGlobalStateController() = default;

IApplicationCreator::IApplicationCreator(Core::System& system_)
    : ServiceFramework{system_, "IApplicationCreator"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CreateApplication"},
        {1, nullptr, "PopLaunchRequestedApplication"},
        {10, nullptr, "CreateSystemApplication"},
        {100, nullptr, "PopFloatingApplicationForDevelopment"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IApplicationCreator::~IApplicationCreator() = default;

IProcessWindingController::IProcessWindingController(Core::System& system_)
    : ServiceFramework{system_, "IProcessWindingController"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetLaunchReason"},
        {11, nullptr, "OpenCallingLibraryApplet"},
        {21, nullptr, "PushContext"},
        {22, nullptr, "PopContext"},
        {23, nullptr, "CancelWindingReservation"},
        {30, nullptr, "WindAndDoReserved"},
        {40, nullptr, "ReserveToStartAndWaitAndUnwindThis"},
        {41, nullptr, "ReserveToStartAndWait"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IProcessWindingController::~IProcessWindingController() = default;
} // namespace Service::AM
