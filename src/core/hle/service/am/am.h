// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>
#include <queue>

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Kernel {
class KernelCore;
class KTransferMemory;
} // namespace Kernel

namespace Service::NVFlinger {
class NVFlinger;
}

namespace Service::AM {

// This is nn::settings::Language
enum SystemLanguage {
    Japanese = 0,
    English = 1, // en-US
    French = 2,
    German = 3,
    Italian = 4,
    Spanish = 5,
    Chinese = 6,
    Korean = 7,
    Dutch = 8,
    Portuguese = 9,
    Russian = 10,
    Taiwanese = 11,
    BritishEnglish = 12, // en-GB
    CanadianFrench = 13,
    LatinAmericanSpanish = 14, // es-419
    // 4.0.0+
    SimplifiedChinese = 15,
    TraditionalChinese = 16,
    // 10.1.0+
    BrazilianPortuguese = 17,
};

class AppletMessageQueue {
public:
    // This is nn::am::AppletMessage
    enum class AppletMessage : u32 {
        None = 0,
        ChangeIntoForeground = 1,
        ChangeIntoBackground = 2,
        Exit = 4,
        ApplicationExited = 6,
        FocusStateChanged = 15,
        Resume = 16,
        DetectShortPressingHomeButton = 20,
        DetectLongPressingHomeButton = 21,
        DetectShortPressingPowerButton = 22,
        DetectMiddlePressingPowerButton = 23,
        DetectLongPressingPowerButton = 24,
        RequestToPrepareSleep = 25,
        FinishedSleepSequence = 26,
        SleepRequiredByHighTemperature = 27,
        SleepRequiredByLowBattery = 28,
        AutoPowerDown = 29,
        OperationModeChanged = 30,
        PerformanceModeChanged = 31,
        DetectReceivingCecSystemStandby = 32,
        SdCardRemoved = 33,
        LaunchApplicationRequested = 50,
        RequestToDisplay = 51,
        ShowApplicationLogo = 55,
        HideApplicationLogo = 56,
        ForceHideApplicationLogo = 57,
        FloatingApplicationDetected = 60,
        DetectShortPressingCaptureButton = 90,
        AlbumScreenShotTaken = 92,
        AlbumRecordingSaved = 93,
    };

    explicit AppletMessageQueue(Core::System& system);
    ~AppletMessageQueue();

    Kernel::KReadableEvent& GetMessageReceiveEvent();
    Kernel::KReadableEvent& GetOperationModeChangedEvent();
    void PushMessage(AppletMessage msg);
    AppletMessage PopMessage();
    std::size_t GetMessageCount() const;
    void RequestExit();
    void RequestResume();
    void FocusStateChanged();
    void OperationModeChanged();

private:
    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* on_new_message;
    Kernel::KEvent* on_operation_mode_changed;

    std::queue<AppletMessage> messages;
};

class IWindowController final : public ServiceFramework<IWindowController> {
public:
    explicit IWindowController(Core::System& system_);
    ~IWindowController() override;

private:
    void GetAppletResourceUserId(Kernel::HLERequestContext& ctx);
    void AcquireForegroundRights(Kernel::HLERequestContext& ctx);
};

class IAudioController final : public ServiceFramework<IAudioController> {
public:
    explicit IAudioController(Core::System& system_);
    ~IAudioController() override;

private:
    void SetExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void GetMainAppletExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void GetLibraryAppletExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void ChangeMainAppletMasterVolume(Kernel::HLERequestContext& ctx);
    void SetTransparentAudioRate(Kernel::HLERequestContext& ctx);

    static constexpr float min_allowed_volume = 0.0f;
    static constexpr float max_allowed_volume = 1.0f;

    float main_applet_volume{0.25f};
    float library_applet_volume{max_allowed_volume};
    float transparent_volume_rate{min_allowed_volume};

    // Volume transition fade time in nanoseconds.
    // e.g. If the main applet volume was 0% and was changed to 50%
    //      with a fade of 50ns, then over the course of 50ns,
    //      the volume will gradually fade up to 50%
    std::chrono::nanoseconds fade_time_ns{0};
};

class IDisplayController final : public ServiceFramework<IDisplayController> {
public:
    explicit IDisplayController(Core::System& system_);
    ~IDisplayController() override;
};

class IDebugFunctions final : public ServiceFramework<IDebugFunctions> {
public:
    explicit IDebugFunctions(Core::System& system_);
    ~IDebugFunctions() override;
};

class ISelfController final : public ServiceFramework<ISelfController> {
public:
    explicit ISelfController(Core::System& system_, NVFlinger::NVFlinger& nvflinger_);
    ~ISelfController() override;

private:
    void Exit(Kernel::HLERequestContext& ctx);
    void LockExit(Kernel::HLERequestContext& ctx);
    void UnlockExit(Kernel::HLERequestContext& ctx);
    void EnterFatalSection(Kernel::HLERequestContext& ctx);
    void LeaveFatalSection(Kernel::HLERequestContext& ctx);
    void GetLibraryAppletLaunchableEvent(Kernel::HLERequestContext& ctx);
    void SetScreenShotPermission(Kernel::HLERequestContext& ctx);
    void SetOperationModeChangedNotification(Kernel::HLERequestContext& ctx);
    void SetPerformanceModeChangedNotification(Kernel::HLERequestContext& ctx);
    void SetFocusHandlingMode(Kernel::HLERequestContext& ctx);
    void SetRestartMessageEnabled(Kernel::HLERequestContext& ctx);
    void SetOutOfFocusSuspendingEnabled(Kernel::HLERequestContext& ctx);
    void SetAlbumImageOrientation(Kernel::HLERequestContext& ctx);
    void CreateManagedDisplayLayer(Kernel::HLERequestContext& ctx);
    void CreateManagedDisplaySeparableLayer(Kernel::HLERequestContext& ctx);
    void SetHandlesRequestToDisplay(Kernel::HLERequestContext& ctx);
    void SetIdleTimeDetectionExtension(Kernel::HLERequestContext& ctx);
    void GetIdleTimeDetectionExtension(Kernel::HLERequestContext& ctx);
    void ReportUserIsActive(Kernel::HLERequestContext& ctx);
    void SetAutoSleepDisabled(Kernel::HLERequestContext& ctx);
    void IsAutoSleepDisabled(Kernel::HLERequestContext& ctx);
    void GetAccumulatedSuspendedTickValue(Kernel::HLERequestContext& ctx);
    void GetAccumulatedSuspendedTickChangedEvent(Kernel::HLERequestContext& ctx);
    void SetAlbumImageTakenNotificationEnabled(Kernel::HLERequestContext& ctx);
    void SaveCurrentScreenshot(Kernel::HLERequestContext& ctx);

    enum class ScreenshotPermission : u32 {
        Inherit = 0,
        Enable = 1,
        Disable = 2,
    };

    NVFlinger::NVFlinger& nvflinger;

    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* launchable_event;
    Kernel::KEvent* accumulated_suspended_tick_changed_event;

    u32 idle_time_detection_extension = 0;
    u64 num_fatal_sections_entered = 0;
    bool is_auto_sleep_disabled = false;
    ScreenshotPermission screenshot_permission = ScreenshotPermission::Inherit;
};

class ICommonStateGetter final : public ServiceFramework<ICommonStateGetter> {
public:
    explicit ICommonStateGetter(Core::System& system_,
                                std::shared_ptr<AppletMessageQueue> msg_queue_);
    ~ICommonStateGetter() override;

private:
    // This is nn::oe::FocusState
    enum class FocusState : u8 {
        InFocus = 1,
        NotInFocus = 2,
        Background = 3,
    };

    // This is nn::oe::OperationMode
    enum class OperationMode : u8 {
        Handheld = 0,
        Docked = 1,
    };

    // This is nn::am::service::SystemButtonType
    enum class SystemButtonType {
        None,
        HomeButtonShortPressing,
        HomeButtonLongPressing,
        PowerButtonShortPressing,
        PowerButtonLongPressing,
        ShutdownSystem,
        CaptureButtonShortPressing,
        CaptureButtonLongPressing,
    };

    void GetEventHandle(Kernel::HLERequestContext& ctx);
    void ReceiveMessage(Kernel::HLERequestContext& ctx);
    void GetCurrentFocusState(Kernel::HLERequestContext& ctx);
    void GetDefaultDisplayResolutionChangeEvent(Kernel::HLERequestContext& ctx);
    void GetOperationMode(Kernel::HLERequestContext& ctx);
    void GetPerformanceMode(Kernel::HLERequestContext& ctx);
    void GetBootMode(Kernel::HLERequestContext& ctx);
    void IsVrModeEnabled(Kernel::HLERequestContext& ctx);
    void SetVrModeEnabled(Kernel::HLERequestContext& ctx);
    void SetLcdBacklighOffEnabled(Kernel::HLERequestContext& ctx);
    void BeginVrModeEx(Kernel::HLERequestContext& ctx);
    void EndVrModeEx(Kernel::HLERequestContext& ctx);
    void GetDefaultDisplayResolution(Kernel::HLERequestContext& ctx);
    void SetCpuBoostMode(Kernel::HLERequestContext& ctx);
    void PerformSystemButtonPressingIfInFocus(Kernel::HLERequestContext& ctx);
    void SetRequestExitToLibraryAppletAtExecuteNextProgramEnabled(Kernel::HLERequestContext& ctx);

    std::shared_ptr<AppletMessageQueue> msg_queue;
    bool vr_mode_state{};
};

class IStorageImpl {
public:
    virtual ~IStorageImpl();
    virtual std::vector<u8>& GetData() = 0;
    virtual const std::vector<u8>& GetData() const = 0;
    virtual std::size_t GetSize() const = 0;
};

class IStorage final : public ServiceFramework<IStorage> {
public:
    explicit IStorage(Core::System& system_, std::vector<u8>&& buffer);
    ~IStorage() override;

    std::vector<u8>& GetData() {
        return impl->GetData();
    }

    const std::vector<u8>& GetData() const {
        return impl->GetData();
    }

    std::size_t GetSize() const {
        return impl->GetSize();
    }

private:
    void Register();
    void Open(Kernel::HLERequestContext& ctx);

    std::shared_ptr<IStorageImpl> impl;
};

class IStorageAccessor final : public ServiceFramework<IStorageAccessor> {
public:
    explicit IStorageAccessor(Core::System& system_, IStorage& backing_);
    ~IStorageAccessor() override;

private:
    void GetSize(Kernel::HLERequestContext& ctx);
    void Write(Kernel::HLERequestContext& ctx);
    void Read(Kernel::HLERequestContext& ctx);

    IStorage& backing;
};

class ILibraryAppletCreator final : public ServiceFramework<ILibraryAppletCreator> {
public:
    explicit ILibraryAppletCreator(Core::System& system_);
    ~ILibraryAppletCreator() override;

private:
    void CreateLibraryApplet(Kernel::HLERequestContext& ctx);
    void CreateStorage(Kernel::HLERequestContext& ctx);
    void CreateTransferMemoryStorage(Kernel::HLERequestContext& ctx);
    void CreateHandleStorage(Kernel::HLERequestContext& ctx);
};

class IApplicationFunctions final : public ServiceFramework<IApplicationFunctions> {
public:
    explicit IApplicationFunctions(Core::System& system_);
    ~IApplicationFunctions() override;

private:
    void PopLaunchParameter(Kernel::HLERequestContext& ctx);
    void CreateApplicationAndRequestToStartForQuest(Kernel::HLERequestContext& ctx);
    void EnsureSaveData(Kernel::HLERequestContext& ctx);
    void SetTerminateResult(Kernel::HLERequestContext& ctx);
    void GetDisplayVersion(Kernel::HLERequestContext& ctx);
    void GetDesiredLanguage(Kernel::HLERequestContext& ctx);
    void IsGamePlayRecordingSupported(Kernel::HLERequestContext& ctx);
    void InitializeGamePlayRecording(Kernel::HLERequestContext& ctx);
    void SetGamePlayRecordingState(Kernel::HLERequestContext& ctx);
    void NotifyRunning(Kernel::HLERequestContext& ctx);
    void GetPseudoDeviceId(Kernel::HLERequestContext& ctx);
    void ExtendSaveData(Kernel::HLERequestContext& ctx);
    void GetSaveDataSize(Kernel::HLERequestContext& ctx);
    void BeginBlockingHomeButtonShortAndLongPressed(Kernel::HLERequestContext& ctx);
    void EndBlockingHomeButtonShortAndLongPressed(Kernel::HLERequestContext& ctx);
    void BeginBlockingHomeButton(Kernel::HLERequestContext& ctx);
    void EndBlockingHomeButton(Kernel::HLERequestContext& ctx);
    void EnableApplicationCrashReport(Kernel::HLERequestContext& ctx);
    void InitializeApplicationCopyrightFrameBuffer(Kernel::HLERequestContext& ctx);
    void SetApplicationCopyrightImage(Kernel::HLERequestContext& ctx);
    void SetApplicationCopyrightVisibility(Kernel::HLERequestContext& ctx);
    void QueryApplicationPlayStatistics(Kernel::HLERequestContext& ctx);
    void QueryApplicationPlayStatisticsByUid(Kernel::HLERequestContext& ctx);
    void ExecuteProgram(Kernel::HLERequestContext& ctx);
    void ClearUserChannel(Kernel::HLERequestContext& ctx);
    void UnpopToUserChannel(Kernel::HLERequestContext& ctx);
    void GetPreviousProgramIndex(Kernel::HLERequestContext& ctx);
    void GetGpuErrorDetectedSystemEvent(Kernel::HLERequestContext& ctx);
    void GetFriendInvitationStorageChannelEvent(Kernel::HLERequestContext& ctx);
    void TryPopFromFriendInvitationStorageChannel(Kernel::HLERequestContext& ctx);
    void GetNotificationStorageChannelEvent(Kernel::HLERequestContext& ctx);
    void GetHealthWarningDisappearedSystemEvent(Kernel::HLERequestContext& ctx);
    void PrepareForJit(Kernel::HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;

    bool launch_popped_application_specific = false;
    bool launch_popped_account_preselect = false;
    s32 previous_program_index{-1};
    Kernel::KEvent* gpu_error_detected_event;
    Kernel::KEvent* friend_invitation_storage_channel_event;
    Kernel::KEvent* notification_storage_channel_event;
    Kernel::KEvent* health_warning_disappeared_system_event;
};

class IHomeMenuFunctions final : public ServiceFramework<IHomeMenuFunctions> {
public:
    explicit IHomeMenuFunctions(Core::System& system_);
    ~IHomeMenuFunctions() override;

private:
    void RequestToGetForeground(Kernel::HLERequestContext& ctx);
    void GetPopFromGeneralChannelEvent(Kernel::HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* pop_from_general_channel_event;
};

class IGlobalStateController final : public ServiceFramework<IGlobalStateController> {
public:
    explicit IGlobalStateController(Core::System& system_);
    ~IGlobalStateController() override;
};

class IApplicationCreator final : public ServiceFramework<IApplicationCreator> {
public:
    explicit IApplicationCreator(Core::System& system_);
    ~IApplicationCreator() override;
};

class IProcessWindingController final : public ServiceFramework<IProcessWindingController> {
public:
    explicit IProcessWindingController(Core::System& system_);
    ~IProcessWindingController() override;
};

/// Registers all AM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, NVFlinger::NVFlinger& nvflinger,
                       Core::System& system);

} // namespace Service::AM
