// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hid/hid_core.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/hid/errors.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/hid/hidbus.h"
#include "core/hle/service/hid/irs.h"
#include "core/hle/service/hid/xcd.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/server_manager.h"
#include "core/memory.h"

#include "core/hle/service/hid/controllers/console_sixaxis.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/controllers/debug_pad.h"
#include "core/hle/service/hid/controllers/gesture.h"
#include "core/hle/service/hid/controllers/keyboard.h"
#include "core/hle/service/hid/controllers/mouse.h"
#include "core/hle/service/hid/controllers/npad.h"
#include "core/hle/service/hid/controllers/palma.h"
#include "core/hle/service/hid/controllers/stubbed.h"
#include "core/hle/service/hid/controllers/touchscreen.h"
#include "core/hle/service/hid/controllers/xpad.h"

namespace Service::HID {

// Updating period for each HID device.
// Period time is obtained by measuring the number of samples in a second on HW using a homebrew
// Correct npad_update_ns is 4ms this is overclocked to lower input lag
constexpr auto npad_update_ns = std::chrono::nanoseconds{1 * 1000 * 1000};    // (1ms, 1000Hz)
constexpr auto default_update_ns = std::chrono::nanoseconds{4 * 1000 * 1000}; // (4ms, 1000Hz)
constexpr auto mouse_keyboard_update_ns = std::chrono::nanoseconds{8 * 1000 * 1000}; // (8ms, 125Hz)
constexpr auto motion_update_ns = std::chrono::nanoseconds{5 * 1000 * 1000};         // (5ms, 200Hz)

IAppletResource::IAppletResource(Core::System& system_,
                                 KernelHelpers::ServiceContext& service_context_)
    : ServiceFramework{system_, "IAppletResource"}, service_context{service_context_} {
    static const FunctionInfo functions[] = {
        {0, &IAppletResource::GetSharedMemoryHandle, "GetSharedMemoryHandle"},
    };
    RegisterHandlers(functions);
    u8* shared_memory = system.Kernel().GetHidSharedMem().GetPointer();
    MakeController<Controller_DebugPad>(HidController::DebugPad, shared_memory);
    MakeController<Controller_Touchscreen>(HidController::Touchscreen, shared_memory);
    MakeController<Controller_Mouse>(HidController::Mouse, shared_memory);
    MakeController<Controller_Keyboard>(HidController::Keyboard, shared_memory);
    MakeController<Controller_XPad>(HidController::XPad, shared_memory);
    MakeController<Controller_Stubbed>(HidController::HomeButton, shared_memory);
    MakeController<Controller_Stubbed>(HidController::SleepButton, shared_memory);
    MakeController<Controller_Stubbed>(HidController::CaptureButton, shared_memory);
    MakeController<Controller_Stubbed>(HidController::InputDetector, shared_memory);
    MakeController<Controller_Stubbed>(HidController::UniquePad, shared_memory);
    MakeControllerWithServiceContext<Controller_NPad>(HidController::NPad, shared_memory);
    MakeController<Controller_Gesture>(HidController::Gesture, shared_memory);
    MakeController<Controller_ConsoleSixAxis>(HidController::ConsoleSixAxisSensor, shared_memory);
    MakeController<Controller_Stubbed>(HidController::DebugMouse, shared_memory);
    MakeControllerWithServiceContext<Controller_Palma>(HidController::Palma, shared_memory);

    // Homebrew doesn't try to activate some controllers, so we activate them by default
    GetController<Controller_NPad>(HidController::NPad).ActivateController();
    GetController<Controller_Touchscreen>(HidController::Touchscreen).ActivateController();

    GetController<Controller_Stubbed>(HidController::HomeButton).SetCommonHeaderOffset(0x4C00);
    GetController<Controller_Stubbed>(HidController::SleepButton).SetCommonHeaderOffset(0x4E00);
    GetController<Controller_Stubbed>(HidController::CaptureButton).SetCommonHeaderOffset(0x5000);
    GetController<Controller_Stubbed>(HidController::InputDetector).SetCommonHeaderOffset(0x5200);
    GetController<Controller_Stubbed>(HidController::UniquePad).SetCommonHeaderOffset(0x5A00);
    GetController<Controller_Stubbed>(HidController::DebugMouse).SetCommonHeaderOffset(0x3DC00);

    // Register update callbacks
    npad_update_event = Core::Timing::CreateEvent(
        "HID::UpdatePadCallback",
        [this](std::uintptr_t user_data, s64 time,
               std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
            const auto guard = LockService();
            UpdateNpad(user_data, ns_late);
            return std::nullopt;
        });
    default_update_event = Core::Timing::CreateEvent(
        "HID::UpdateDefaultCallback",
        [this](std::uintptr_t user_data, s64 time,
               std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
            const auto guard = LockService();
            UpdateControllers(user_data, ns_late);
            return std::nullopt;
        });
    mouse_keyboard_update_event = Core::Timing::CreateEvent(
        "HID::UpdateMouseKeyboardCallback",
        [this](std::uintptr_t user_data, s64 time,
               std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
            const auto guard = LockService();
            UpdateMouseKeyboard(user_data, ns_late);
            return std::nullopt;
        });
    motion_update_event = Core::Timing::CreateEvent(
        "HID::UpdateMotionCallback",
        [this](std::uintptr_t user_data, s64 time,
               std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
            const auto guard = LockService();
            UpdateMotion(user_data, ns_late);
            return std::nullopt;
        });

    system.CoreTiming().ScheduleLoopingEvent(npad_update_ns, npad_update_ns, npad_update_event);
    system.CoreTiming().ScheduleLoopingEvent(default_update_ns, default_update_ns,
                                             default_update_event);
    system.CoreTiming().ScheduleLoopingEvent(mouse_keyboard_update_ns, mouse_keyboard_update_ns,
                                             mouse_keyboard_update_event);
    system.CoreTiming().ScheduleLoopingEvent(motion_update_ns, motion_update_ns,
                                             motion_update_event);

    system.HIDCore().ReloadInputDevices();
}

void IAppletResource::ActivateController(HidController controller) {
    controllers[static_cast<size_t>(controller)]->ActivateController();
}

void IAppletResource::DeactivateController(HidController controller) {
    controllers[static_cast<size_t>(controller)]->DeactivateController();
}

IAppletResource::~IAppletResource() {
    system.CoreTiming().UnscheduleEvent(npad_update_event, 0);
    system.CoreTiming().UnscheduleEvent(default_update_event, 0);
    system.CoreTiming().UnscheduleEvent(mouse_keyboard_update_event, 0);
    system.CoreTiming().UnscheduleEvent(motion_update_event, 0);
}

void IAppletResource::GetSharedMemoryHandle(HLERequestContext& ctx) {
    LOG_DEBUG(Service_HID, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(&system.Kernel().GetHidSharedMem());
}

void IAppletResource::UpdateControllers(std::uintptr_t user_data,
                                        std::chrono::nanoseconds ns_late) {
    auto& core_timing = system.CoreTiming();

    for (const auto& controller : controllers) {
        // Keyboard has it's own update event
        if (controller == controllers[static_cast<size_t>(HidController::Keyboard)]) {
            continue;
        }
        // Mouse has it's own update event
        if (controller == controllers[static_cast<size_t>(HidController::Mouse)]) {
            continue;
        }
        // Npad has it's own update event
        if (controller == controllers[static_cast<size_t>(HidController::NPad)]) {
            continue;
        }
        controller->OnUpdate(core_timing);
    }
}

void IAppletResource::UpdateNpad(std::uintptr_t user_data, std::chrono::nanoseconds ns_late) {
    auto& core_timing = system.CoreTiming();

    controllers[static_cast<size_t>(HidController::NPad)]->OnUpdate(core_timing);
}

void IAppletResource::UpdateMouseKeyboard(std::uintptr_t user_data,
                                          std::chrono::nanoseconds ns_late) {
    auto& core_timing = system.CoreTiming();

    controllers[static_cast<size_t>(HidController::Mouse)]->OnUpdate(core_timing);
    controllers[static_cast<size_t>(HidController::Keyboard)]->OnUpdate(core_timing);
}

void IAppletResource::UpdateMotion(std::uintptr_t user_data, std::chrono::nanoseconds ns_late) {
    auto& core_timing = system.CoreTiming();

    controllers[static_cast<size_t>(HidController::NPad)]->OnMotionUpdate(core_timing);
}

class IActiveVibrationDeviceList final : public ServiceFramework<IActiveVibrationDeviceList> {
public:
    explicit IActiveVibrationDeviceList(Core::System& system_,
                                        std::shared_ptr<IAppletResource> applet_resource_)
        : ServiceFramework{system_, "IActiveVibrationDeviceList"},
          applet_resource(applet_resource_) {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IActiveVibrationDeviceList::InitializeVibrationDevice, "InitializeVibrationDevice"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void InitializeVibrationDevice(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto vibration_device_handle{rp.PopRaw<Core::HID::VibrationDeviceHandle>()};

        if (applet_resource != nullptr) {
            applet_resource->GetController<Controller_NPad>(HidController::NPad)
                .InitializeVibrationDevice(vibration_device_handle);
        }

        LOG_DEBUG(Service_HID, "called, npad_type={}, npad_id={}, device_index={}",
                  vibration_device_handle.npad_type, vibration_device_handle.npad_id,
                  vibration_device_handle.device_index);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    std::shared_ptr<IAppletResource> applet_resource;
};

std::shared_ptr<IAppletResource> Hid::GetAppletResource() {
    if (applet_resource == nullptr) {
        applet_resource = std::make_shared<IAppletResource>(system, service_context);
    }

    return applet_resource;
}

Hid::Hid(Core::System& system_)
    : ServiceFramework{system_, "hid"}, service_context{system_, service_name} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &Hid::CreateAppletResource, "CreateAppletResource"},
        {1, &Hid::ActivateDebugPad, "ActivateDebugPad"},
        {11, &Hid::ActivateTouchScreen, "ActivateTouchScreen"},
        {21, &Hid::ActivateMouse, "ActivateMouse"},
        {26, nullptr, "ActivateDebugMouse"},
        {31, &Hid::ActivateKeyboard, "ActivateKeyboard"},
        {32, &Hid::SendKeyboardLockKeyEvent, "SendKeyboardLockKeyEvent"},
        {40, nullptr, "AcquireXpadIdEventHandle"},
        {41, nullptr, "ReleaseXpadIdEventHandle"},
        {51, &Hid::ActivateXpad, "ActivateXpad"},
        {55, &Hid::GetXpadIDs, "GetXpadIds"},
        {56, nullptr, "ActivateJoyXpad"},
        {58, nullptr, "GetJoyXpadLifoHandle"},
        {59, nullptr, "GetJoyXpadIds"},
        {60, &Hid::ActivateSixAxisSensor, "ActivateSixAxisSensor"},
        {61, &Hid::DeactivateSixAxisSensor, "DeactivateSixAxisSensor"},
        {62, nullptr, "GetSixAxisSensorLifoHandle"},
        {63, nullptr, "ActivateJoySixAxisSensor"},
        {64, nullptr, "DeactivateJoySixAxisSensor"},
        {65, nullptr, "GetJoySixAxisSensorLifoHandle"},
        {66, &Hid::StartSixAxisSensor, "StartSixAxisSensor"},
        {67, &Hid::StopSixAxisSensor, "StopSixAxisSensor"},
        {68, &Hid::IsSixAxisSensorFusionEnabled, "IsSixAxisSensorFusionEnabled"},
        {69, &Hid::EnableSixAxisSensorFusion, "EnableSixAxisSensorFusion"},
        {70, &Hid::SetSixAxisSensorFusionParameters, "SetSixAxisSensorFusionParameters"},
        {71, &Hid::GetSixAxisSensorFusionParameters, "GetSixAxisSensorFusionParameters"},
        {72, &Hid::ResetSixAxisSensorFusionParameters, "ResetSixAxisSensorFusionParameters"},
        {73, nullptr, "SetAccelerometerParameters"},
        {74, nullptr, "GetAccelerometerParameters"},
        {75, nullptr, "ResetAccelerometerParameters"},
        {76, nullptr, "SetAccelerometerPlayMode"},
        {77, nullptr, "GetAccelerometerPlayMode"},
        {78, nullptr, "ResetAccelerometerPlayMode"},
        {79, &Hid::SetGyroscopeZeroDriftMode, "SetGyroscopeZeroDriftMode"},
        {80, &Hid::GetGyroscopeZeroDriftMode, "GetGyroscopeZeroDriftMode"},
        {81, &Hid::ResetGyroscopeZeroDriftMode, "ResetGyroscopeZeroDriftMode"},
        {82, &Hid::IsSixAxisSensorAtRest, "IsSixAxisSensorAtRest"},
        {83, &Hid::IsFirmwareUpdateAvailableForSixAxisSensor, "IsFirmwareUpdateAvailableForSixAxisSensor"},
        {84, &Hid::EnableSixAxisSensorUnalteredPassthrough, "EnableSixAxisSensorUnalteredPassthrough"},
        {85, &Hid::IsSixAxisSensorUnalteredPassthroughEnabled, "IsSixAxisSensorUnalteredPassthroughEnabled"},
        {86, nullptr, "StoreSixAxisSensorCalibrationParameter"},
        {87, &Hid::LoadSixAxisSensorCalibrationParameter, "LoadSixAxisSensorCalibrationParameter"},
        {88, &Hid::GetSixAxisSensorIcInformation, "GetSixAxisSensorIcInformation"},
        {89, &Hid::ResetIsSixAxisSensorDeviceNewlyAssigned, "ResetIsSixAxisSensorDeviceNewlyAssigned"},
        {91, &Hid::ActivateGesture, "ActivateGesture"},
        {100, &Hid::SetSupportedNpadStyleSet, "SetSupportedNpadStyleSet"},
        {101, &Hid::GetSupportedNpadStyleSet, "GetSupportedNpadStyleSet"},
        {102, &Hid::SetSupportedNpadIdType, "SetSupportedNpadIdType"},
        {103, &Hid::ActivateNpad, "ActivateNpad"},
        {104, &Hid::DeactivateNpad, "DeactivateNpad"},
        {106, &Hid::AcquireNpadStyleSetUpdateEventHandle, "AcquireNpadStyleSetUpdateEventHandle"},
        {107, &Hid::DisconnectNpad, "DisconnectNpad"},
        {108, &Hid::GetPlayerLedPattern, "GetPlayerLedPattern"},
        {109, &Hid::ActivateNpadWithRevision, "ActivateNpadWithRevision"},
        {120, &Hid::SetNpadJoyHoldType, "SetNpadJoyHoldType"},
        {121, &Hid::GetNpadJoyHoldType, "GetNpadJoyHoldType"},
        {122, &Hid::SetNpadJoyAssignmentModeSingleByDefault, "SetNpadJoyAssignmentModeSingleByDefault"},
        {123, &Hid::SetNpadJoyAssignmentModeSingle, "SetNpadJoyAssignmentModeSingle"},
        {124, &Hid::SetNpadJoyAssignmentModeDual, "SetNpadJoyAssignmentModeDual"},
        {125, &Hid::MergeSingleJoyAsDualJoy, "MergeSingleJoyAsDualJoy"},
        {126, &Hid::StartLrAssignmentMode, "StartLrAssignmentMode"},
        {127, &Hid::StopLrAssignmentMode, "StopLrAssignmentMode"},
        {128, &Hid::SetNpadHandheldActivationMode, "SetNpadHandheldActivationMode"},
        {129, &Hid::GetNpadHandheldActivationMode, "GetNpadHandheldActivationMode"},
        {130, &Hid::SwapNpadAssignment, "SwapNpadAssignment"},
        {131, &Hid::IsUnintendedHomeButtonInputProtectionEnabled, "IsUnintendedHomeButtonInputProtectionEnabled"},
        {132, &Hid::EnableUnintendedHomeButtonInputProtection, "EnableUnintendedHomeButtonInputProtection"},
        {133, &Hid::SetNpadJoyAssignmentModeSingleWithDestination, "SetNpadJoyAssignmentModeSingleWithDestination"},
        {134, &Hid::SetNpadAnalogStickUseCenterClamp, "SetNpadAnalogStickUseCenterClamp"},
        {135, &Hid::SetNpadCaptureButtonAssignment, "SetNpadCaptureButtonAssignment"},
        {136, &Hid::ClearNpadCaptureButtonAssignment, "ClearNpadCaptureButtonAssignment"},
        {200, &Hid::GetVibrationDeviceInfo, "GetVibrationDeviceInfo"},
        {201, &Hid::SendVibrationValue, "SendVibrationValue"},
        {202, &Hid::GetActualVibrationValue, "GetActualVibrationValue"},
        {203, &Hid::CreateActiveVibrationDeviceList, "CreateActiveVibrationDeviceList"},
        {204, &Hid::PermitVibration, "PermitVibration"},
        {205, &Hid::IsVibrationPermitted, "IsVibrationPermitted"},
        {206, &Hid::SendVibrationValues, "SendVibrationValues"},
        {207, &Hid::SendVibrationGcErmCommand, "SendVibrationGcErmCommand"},
        {208, &Hid::GetActualVibrationGcErmCommand, "GetActualVibrationGcErmCommand"},
        {209, &Hid::BeginPermitVibrationSession, "BeginPermitVibrationSession"},
        {210, &Hid::EndPermitVibrationSession, "EndPermitVibrationSession"},
        {211, &Hid::IsVibrationDeviceMounted, "IsVibrationDeviceMounted"},
        {212, nullptr, "SendVibrationValueInBool"},
        {300, &Hid::ActivateConsoleSixAxisSensor, "ActivateConsoleSixAxisSensor"},
        {301, &Hid::StartConsoleSixAxisSensor, "StartConsoleSixAxisSensor"},
        {302, &Hid::StopConsoleSixAxisSensor, "StopConsoleSixAxisSensor"},
        {303, &Hid::ActivateSevenSixAxisSensor, "ActivateSevenSixAxisSensor"},
        {304, &Hid::StartSevenSixAxisSensor, "StartSevenSixAxisSensor"},
        {305, &Hid::StopSevenSixAxisSensor, "StopSevenSixAxisSensor"},
        {306, &Hid::InitializeSevenSixAxisSensor, "InitializeSevenSixAxisSensor"},
        {307, &Hid::FinalizeSevenSixAxisSensor, "FinalizeSevenSixAxisSensor"},
        {308, nullptr, "SetSevenSixAxisSensorFusionStrength"},
        {309, nullptr, "GetSevenSixAxisSensorFusionStrength"},
        {310, &Hid::ResetSevenSixAxisSensorTimestamp, "ResetSevenSixAxisSensorTimestamp"},
        {400, &Hid::IsUsbFullKeyControllerEnabled, "IsUsbFullKeyControllerEnabled"},
        {401, nullptr, "EnableUsbFullKeyController"},
        {402, nullptr, "IsUsbFullKeyControllerConnected"},
        {403, nullptr, "HasBattery"},
        {404, nullptr, "HasLeftRightBattery"},
        {405, nullptr, "GetNpadInterfaceType"},
        {406, nullptr, "GetNpadLeftRightInterfaceType"},
        {407, nullptr, "GetNpadOfHighestBatteryLevel"},
        {408, nullptr, "GetNpadOfHighestBatteryLevelForJoyRight"},
        {500, &Hid::GetPalmaConnectionHandle, "GetPalmaConnectionHandle"},
        {501, &Hid::InitializePalma, "InitializePalma"},
        {502, &Hid::AcquirePalmaOperationCompleteEvent, "AcquirePalmaOperationCompleteEvent"},
        {503, &Hid::GetPalmaOperationInfo, "GetPalmaOperationInfo"},
        {504, &Hid::PlayPalmaActivity, "PlayPalmaActivity"},
        {505, &Hid::SetPalmaFrModeType, "SetPalmaFrModeType"},
        {506, &Hid::ReadPalmaStep, "ReadPalmaStep"},
        {507, &Hid::EnablePalmaStep, "EnablePalmaStep"},
        {508, &Hid::ResetPalmaStep, "ResetPalmaStep"},
        {509, &Hid::ReadPalmaApplicationSection, "ReadPalmaApplicationSection"},
        {510, &Hid::WritePalmaApplicationSection, "WritePalmaApplicationSection"},
        {511, &Hid::ReadPalmaUniqueCode, "ReadPalmaUniqueCode"},
        {512, &Hid::SetPalmaUniqueCodeInvalid, "SetPalmaUniqueCodeInvalid"},
        {513, &Hid::WritePalmaActivityEntry, "WritePalmaActivityEntry"},
        {514, &Hid::WritePalmaRgbLedPatternEntry, "WritePalmaRgbLedPatternEntry"},
        {515, &Hid::WritePalmaWaveEntry, "WritePalmaWaveEntry"},
        {516, &Hid::SetPalmaDataBaseIdentificationVersion, "SetPalmaDataBaseIdentificationVersion"},
        {517, &Hid::GetPalmaDataBaseIdentificationVersion, "GetPalmaDataBaseIdentificationVersion"},
        {518, &Hid::SuspendPalmaFeature, "SuspendPalmaFeature"},
        {519, &Hid::GetPalmaOperationResult, "GetPalmaOperationResult"},
        {520, &Hid::ReadPalmaPlayLog, "ReadPalmaPlayLog"},
        {521, &Hid::ResetPalmaPlayLog, "ResetPalmaPlayLog"},
        {522, &Hid::SetIsPalmaAllConnectable, "SetIsPalmaAllConnectable"},
        {523, &Hid::SetIsPalmaPairedConnectable, "SetIsPalmaPairedConnectable"},
        {524, &Hid::PairPalma, "PairPalma"},
        {525, &Hid::SetPalmaBoostMode, "SetPalmaBoostMode"},
        {526, &Hid::CancelWritePalmaWaveEntry, "CancelWritePalmaWaveEntry"},
        {527, &Hid::EnablePalmaBoostMode, "EnablePalmaBoostMode"},
        {528, &Hid::GetPalmaBluetoothAddress, "GetPalmaBluetoothAddress"},
        {529, &Hid::SetDisallowedPalmaConnection, "SetDisallowedPalmaConnection"},
        {1000, &Hid::SetNpadCommunicationMode, "SetNpadCommunicationMode"},
        {1001, &Hid::GetNpadCommunicationMode, "GetNpadCommunicationMode"},
        {1002, &Hid::SetTouchScreenConfiguration, "SetTouchScreenConfiguration"},
        {1003, &Hid::IsFirmwareUpdateNeededForNotification, "IsFirmwareUpdateNeededForNotification"},
        {2000, nullptr, "ActivateDigitizer"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

Hid::~Hid() = default;

void Hid::CreateAppletResource(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    if (applet_resource == nullptr) {
        applet_resource = std::make_shared<IAppletResource>(system, service_context);
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IAppletResource>(applet_resource);
}

void Hid::ActivateDebugPad(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::DebugPad);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ActivateTouchScreen(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::Touchscreen);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ActivateMouse(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::Mouse);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ActivateKeyboard(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::Keyboard);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SendKeyboardLockKeyEvent(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto flags{rp.Pop<u32>()};

    LOG_WARNING(Service_HID, "(STUBBED) called. flags={}", flags);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ActivateXpad(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        u32 basic_xpad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    applet_resource->ActivateController(HidController::XPad);

    LOG_DEBUG(Service_HID, "called, basic_xpad_id={}, applet_resource_user_id={}",
              parameters.basic_xpad_id, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetXpadIDs(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_DEBUG(Service_HID, "(STUBBED) called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(0);
}

void Hid::ActivateSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        u32 basic_xpad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    // This function does nothing on 10.0.0+

    LOG_WARNING(Service_HID, "(STUBBED) called, basic_xpad_id={}, applet_resource_user_id={}",
                parameters.basic_xpad_id, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::DeactivateSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        u32 basic_xpad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    // This function does nothing on 10.0.0+

    LOG_WARNING(Service_HID, "(STUBBED) called, basic_xpad_id={}, applet_resource_user_id={}",
                parameters.basic_xpad_id, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::StartSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SetSixAxisEnabled(parameters.sixaxis_handle, true);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::StopSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SetSixAxisEnabled(parameters.sixaxis_handle, false);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::IsSixAxisSensorFusionEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    bool is_enabled{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.IsSixAxisSensorFusionEnabled(parameters.sixaxis_handle, is_enabled);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result);
    rb.Push(is_enabled);
}

void Hid::EnableSixAxisSensorFusion(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool enable_sixaxis_sensor_fusion;
        INSERT_PADDING_BYTES_NOINIT(3);
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SetSixAxisFusionEnabled(parameters.sixaxis_handle,
                                                           parameters.enable_sixaxis_sensor_fusion);

    LOG_DEBUG(Service_HID,
              "called, enable_sixaxis_sensor_fusion={}, npad_type={}, npad_id={}, "
              "device_index={}, applet_resource_user_id={}",
              parameters.enable_sixaxis_sensor_fusion, parameters.sixaxis_handle.npad_type,
              parameters.sixaxis_handle.npad_id, parameters.sixaxis_handle.device_index,
              parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::SetSixAxisSensorFusionParameters(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        Core::HID::SixAxisSensorFusionParameters sixaxis_fusion;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.SetSixAxisFusionParameters(parameters.sixaxis_handle, parameters.sixaxis_fusion);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, parameter1={}, "
              "parameter2={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.sixaxis_fusion.parameter1,
              parameters.sixaxis_fusion.parameter2, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::GetSixAxisSensorFusionParameters(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::SixAxisSensorFusionParameters fusion_parameters{};
    const auto& controller =
        GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.GetSixAxisFusionParameters(parameters.sixaxis_handle, fusion_parameters);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result);
    rb.PushRaw(fusion_parameters);
}

void Hid::ResetSixAxisSensorFusionParameters(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    // Since these parameters are unknown just use what HW outputs
    const Core::HID::SixAxisSensorFusionParameters fusion_parameters{
        .parameter1 = 0.03f,
        .parameter2 = 0.4f,
    };
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result1 =
        controller.SetSixAxisFusionParameters(parameters.sixaxis_handle, fusion_parameters);
    const auto result2 = controller.SetSixAxisFusionEnabled(parameters.sixaxis_handle, true);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    if (result1.IsError()) {
        rb.Push(result1);
        return;
    }
    rb.Push(result2);
}

void Hid::SetGyroscopeZeroDriftMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto sixaxis_handle{rp.PopRaw<Core::HID::SixAxisSensorHandle>()};
    const auto drift_mode{rp.PopEnum<Core::HID::GyroscopeZeroDriftMode>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SetGyroscopeZeroDriftMode(sixaxis_handle, drift_mode);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, drift_mode={}, "
              "applet_resource_user_id={}",
              sixaxis_handle.npad_type, sixaxis_handle.npad_id, sixaxis_handle.device_index,
              drift_mode, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::GetGyroscopeZeroDriftMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto drift_mode{Core::HID::GyroscopeZeroDriftMode::Standard};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.GetGyroscopeZeroDriftMode(parameters.sixaxis_handle, drift_mode);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result);
    rb.PushEnum(drift_mode);
}

void Hid::ResetGyroscopeZeroDriftMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    const auto drift_mode{Core::HID::GyroscopeZeroDriftMode::Standard};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SetGyroscopeZeroDriftMode(parameters.sixaxis_handle, drift_mode);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::IsSixAxisSensorAtRest(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    bool is_at_rest{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    controller.IsSixAxisSensorAtRest(parameters.sixaxis_handle, is_at_rest);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
              parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(is_at_rest);
}

void Hid::IsFirmwareUpdateAvailableForSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    bool is_firmware_available{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    controller.IsFirmwareUpdateAvailableForSixAxisSensor(parameters.sixaxis_handle,
                                                         is_firmware_available);

    LOG_WARNING(
        Service_HID,
        "(STUBBED) called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
        parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
        parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(is_firmware_available);
}

void Hid::EnableSixAxisSensorUnalteredPassthrough(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool enabled;
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.EnableSixAxisSensorUnalteredPassthrough(
        parameters.sixaxis_handle, parameters.enabled);

    LOG_DEBUG(Service_HID,
              "(STUBBED) called, enabled={}, npad_type={}, npad_id={}, device_index={}, "
              "applet_resource_user_id={}",
              parameters.enabled, parameters.sixaxis_handle.npad_type,
              parameters.sixaxis_handle.npad_id, parameters.sixaxis_handle.device_index,
              parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::IsSixAxisSensorUnalteredPassthroughEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    bool is_unaltered_sisxaxis_enabled{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.IsSixAxisSensorUnalteredPassthroughEnabled(
        parameters.sixaxis_handle, is_unaltered_sisxaxis_enabled);

    LOG_DEBUG(
        Service_HID,
        "(STUBBED) called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
        parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
        parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result);
    rb.Push(is_unaltered_sisxaxis_enabled);
}

void Hid::LoadSixAxisSensorCalibrationParameter(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::SixAxisSensorCalibrationParameter calibration{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.LoadSixAxisSensorCalibrationParameter(parameters.sixaxis_handle, calibration);

    LOG_WARNING(
        Service_HID,
        "(STUBBED) called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
        parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
        parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    if (result.IsSuccess()) {
        ctx.WriteBuffer(calibration);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::GetSixAxisSensorIcInformation(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::SixAxisSensorIcInformation ic_information{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.GetSixAxisSensorIcInformation(parameters.sixaxis_handle, ic_information);

    LOG_WARNING(
        Service_HID,
        "(STUBBED) called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
        parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
        parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    if (result.IsSuccess()) {
        ctx.WriteBuffer(ic_information);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ResetIsSixAxisSensorDeviceNewlyAssigned(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::SixAxisSensorHandle sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.ResetIsSixAxisSensorDeviceNewlyAssigned(parameters.sixaxis_handle);

    LOG_WARNING(
        Service_HID,
        "(STUBBED) called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
        parameters.sixaxis_handle.npad_type, parameters.sixaxis_handle.npad_id,
        parameters.sixaxis_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ActivateGesture(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        u32 unknown;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    applet_resource->ActivateController(HidController::Gesture);

    LOG_WARNING(Service_HID, "(STUBBED) called, unknown={}, applet_resource_user_id={}",
                parameters.unknown, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetSupportedNpadStyleSet(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadStyleSet supported_styleset;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .SetSupportedStyleSet({parameters.supported_styleset});

    LOG_DEBUG(Service_HID, "called, supported_styleset={}, applet_resource_user_id={}",
              parameters.supported_styleset, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetSupportedNpadStyleSet(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(applet_resource->GetController<Controller_NPad>(HidController::NPad)
                    .GetSupportedStyleSet()
                    .raw);
}

void Hid::SetSupportedNpadIdType(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    const auto result = applet_resource->GetController<Controller_NPad>(HidController::NPad)
                            .SetSupportedNpadIdTypes(ctx.ReadBuffer());

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ActivateNpad(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::NPad);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::DeactivateNpad(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->DeactivateController(HidController::NPad);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::AcquireNpadStyleSetUpdateEventHandle(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        u64 unknown;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_DEBUG(Service_HID, "called, npad_id={}, applet_resource_user_id={}, unknown={}",
              parameters.npad_id, parameters.applet_resource_user_id, parameters.unknown);

    // Games expect this event to be signaled after calling this function
    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .SignalStyleSetChangedEvent(parameters.npad_id);

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(applet_resource->GetController<Controller_NPad>(HidController::NPad)
                           .GetStyleSetChangedEvent(parameters.npad_id));
}

void Hid::DisconnectNpad(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    controller.DisconnectNpad(parameters.npad_id);

    LOG_DEBUG(Service_HID, "called, npad_id={}, applet_resource_user_id={}", parameters.npad_id,
              parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetPlayerLedPattern(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto npad_id{rp.PopEnum<Core::HID::NpadIdType>()};

    Core::HID::LedPattern pattern{0, 0, 0, 0};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.GetLedPattern(npad_id, pattern);

    LOG_DEBUG(Service_HID, "called, npad_id={}", npad_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result);
    rb.Push(pattern.raw);
}

void Hid::ActivateNpadWithRevision(HLERequestContext& ctx) {
    // Should have no effect with how our npad sets up the data
    IPC::RequestParser rp{ctx};
    struct Parameters {
        s32 revision;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    applet_resource->ActivateController(HidController::NPad);

    LOG_DEBUG(Service_HID, "called, revision={}, applet_resource_user_id={}", parameters.revision,
              parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetNpadJoyHoldType(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};
    const auto hold_type{rp.PopEnum<Controller_NPad::NpadJoyHoldType>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad).SetHoldType(hold_type);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}, hold_type={}",
              applet_resource_user_id, hold_type);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetNpadJoyHoldType(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.PushEnum(applet_resource->GetController<Controller_NPad>(HidController::NPad).GetHoldType());
}

void Hid::SetNpadJoyAssignmentModeSingleByDefault(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::NpadIdType new_npad_id{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    controller.SetNpadMode(new_npad_id, parameters.npad_id,
                           Controller_NPad::NpadJoyDeviceType::Left,
                           Controller_NPad::NpadJoyAssignmentMode::Single);

    LOG_INFO(Service_HID, "called, npad_id={}, applet_resource_user_id={}", parameters.npad_id,
             parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetNpadJoyAssignmentModeSingle(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Controller_NPad::NpadJoyDeviceType npad_joy_device_type;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::NpadIdType new_npad_id{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    controller.SetNpadMode(new_npad_id, parameters.npad_id, parameters.npad_joy_device_type,
                           Controller_NPad::NpadJoyAssignmentMode::Single);

    LOG_INFO(Service_HID, "called, npad_id={}, applet_resource_user_id={}, npad_joy_device_type={}",
             parameters.npad_id, parameters.applet_resource_user_id,
             parameters.npad_joy_device_type);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetNpadJoyAssignmentModeDual(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::NpadIdType new_npad_id{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    controller.SetNpadMode(new_npad_id, parameters.npad_id, {},
                           Controller_NPad::NpadJoyAssignmentMode::Dual);

    LOG_INFO(Service_HID, "called, npad_id={}, applet_resource_user_id={}", parameters.npad_id,
             parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::MergeSingleJoyAsDualJoy(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto npad_id_1{rp.PopEnum<Core::HID::NpadIdType>()};
    const auto npad_id_2{rp.PopEnum<Core::HID::NpadIdType>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.MergeSingleJoyAsDualJoy(npad_id_1, npad_id_2);

    LOG_DEBUG(Service_HID, "called, npad_id_1={}, npad_id_2={}, applet_resource_user_id={}",
              npad_id_1, npad_id_2, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::StartLrAssignmentMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad).StartLRAssignmentMode();

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::StopLrAssignmentMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad).StopLRAssignmentMode();

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetNpadHandheldActivationMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};
    const auto activation_mode{rp.PopEnum<Controller_NPad::NpadHandheldActivationMode>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .SetNpadHandheldActivationMode(activation_mode);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}, activation_mode={}",
              applet_resource_user_id, activation_mode);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetNpadHandheldActivationMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.PushEnum(applet_resource->GetController<Controller_NPad>(HidController::NPad)
                    .GetNpadHandheldActivationMode());
}

void Hid::SwapNpadAssignment(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto npad_id_1{rp.PopEnum<Core::HID::NpadIdType>()};
    const auto npad_id_2{rp.PopEnum<Core::HID::NpadIdType>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SwapNpadAssignment(npad_id_1, npad_id_2);

    LOG_DEBUG(Service_HID, "called, npad_id_1={}, npad_id_2={}, applet_resource_user_id={}",
              npad_id_1, npad_id_2, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::IsUnintendedHomeButtonInputProtectionEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    bool is_enabled = false;
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result =
        controller.IsUnintendedHomeButtonInputProtectionEnabled(parameters.npad_id, is_enabled);

    LOG_WARNING(Service_HID, "(STUBBED) called, npad_id={}, applet_resource_user_id={}",
                parameters.npad_id, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result);
    rb.Push(is_enabled);
}

void Hid::EnableUnintendedHomeButtonInputProtection(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool unintended_home_button_input_protection;
        INSERT_PADDING_BYTES_NOINIT(3);
        Core::HID::NpadIdType npad_id;
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto result = controller.SetUnintendedHomeButtonInputProtectionEnabled(
        parameters.unintended_home_button_input_protection, parameters.npad_id);

    LOG_WARNING(Service_HID,
                "(STUBBED) called, unintended_home_button_input_protection={}, npad_id={},"
                "applet_resource_user_id={}",
                parameters.unintended_home_button_input_protection, parameters.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::SetNpadJoyAssignmentModeSingleWithDestination(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Controller_NPad::NpadJoyDeviceType npad_joy_device_type;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    Core::HID::NpadIdType new_npad_id{};
    auto& controller = GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);
    const auto is_reasigned =
        controller.SetNpadMode(new_npad_id, parameters.npad_id, parameters.npad_joy_device_type,
                               Controller_NPad::NpadJoyAssignmentMode::Single);

    LOG_INFO(Service_HID, "called, npad_id={}, applet_resource_user_id={}, npad_joy_device_type={}",
             parameters.npad_id, parameters.applet_resource_user_id,
             parameters.npad_joy_device_type);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(is_reasigned);
    rb.PushEnum(new_npad_id);
}

void Hid::SetNpadAnalogStickUseCenterClamp(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool analog_stick_use_center_clamp;
        INSERT_PADDING_BYTES_NOINIT(7);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    GetAppletResource()
        ->GetController<Controller_NPad>(HidController::NPad)
        .SetAnalogStickUseCenterClamp(parameters.analog_stick_use_center_clamp);

    LOG_WARNING(Service_HID,
                "(STUBBED) called, analog_stick_use_center_clamp={}, applet_resource_user_id={}",
                parameters.analog_stick_use_center_clamp, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetNpadCaptureButtonAssignment(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadStyleSet npad_styleset;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Core::HID::NpadButton button;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID,
                "(STUBBED) called, npad_styleset={}, applet_resource_user_id={}, button={}",
                parameters.npad_styleset, parameters.applet_resource_user_id, parameters.button);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ClearNpadCaptureButtonAssignment(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetVibrationDeviceInfo(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto vibration_device_handle{rp.PopRaw<Core::HID::VibrationDeviceHandle>()};
    const auto& controller =
        GetAppletResource()->GetController<Controller_NPad>(HidController::NPad);

    Core::HID::VibrationDeviceInfo vibration_device_info;
    bool check_device_index = false;

    switch (vibration_device_handle.npad_type) {
    case Core::HID::NpadStyleIndex::ProController:
    case Core::HID::NpadStyleIndex::Handheld:
    case Core::HID::NpadStyleIndex::JoyconDual:
    case Core::HID::NpadStyleIndex::JoyconLeft:
    case Core::HID::NpadStyleIndex::JoyconRight:
        vibration_device_info.type = Core::HID::VibrationDeviceType::LinearResonantActuator;
        check_device_index = true;
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        vibration_device_info.type = Core::HID::VibrationDeviceType::GcErm;
        break;
    case Core::HID::NpadStyleIndex::N64:
        vibration_device_info.type = Core::HID::VibrationDeviceType::N64;
        break;
    default:
        vibration_device_info.type = Core::HID::VibrationDeviceType::Unknown;
        break;
    }

    vibration_device_info.position = Core::HID::VibrationDevicePosition::None;
    if (check_device_index) {
        switch (vibration_device_handle.device_index) {
        case Core::HID::DeviceIndex::Left:
            vibration_device_info.position = Core::HID::VibrationDevicePosition::Left;
            break;
        case Core::HID::DeviceIndex::Right:
            vibration_device_info.position = Core::HID::VibrationDevicePosition::Right;
            break;
        case Core::HID::DeviceIndex::None:
        default:
            ASSERT_MSG(false, "DeviceIndex should never be None!");
            break;
        }
    }

    LOG_DEBUG(Service_HID, "called, vibration_device_type={}, vibration_device_position={}",
              vibration_device_info.type, vibration_device_info.position);

    const auto result = controller.IsDeviceHandleValid(vibration_device_handle);
    if (result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.PushRaw(vibration_device_info);
}

void Hid::SendVibrationValue(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::VibrationDeviceHandle vibration_device_handle;
        Core::HID::VibrationValue vibration_value;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x20, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .VibrateController(parameters.vibration_device_handle, parameters.vibration_value);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.vibration_device_handle.npad_type,
              parameters.vibration_device_handle.npad_id,
              parameters.vibration_device_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetActualVibrationValue(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::VibrationDeviceHandle vibration_device_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.vibration_device_handle.npad_type,
              parameters.vibration_device_handle.npad_id,
              parameters.vibration_device_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.PushRaw(applet_resource->GetController<Controller_NPad>(HidController::NPad)
                   .GetLastVibration(parameters.vibration_device_handle));
}

void Hid::CreateActiveVibrationDeviceList(HLERequestContext& ctx) {
    LOG_DEBUG(Service_HID, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IActiveVibrationDeviceList>(system, applet_resource);
}

void Hid::PermitVibration(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto can_vibrate{rp.Pop<bool>()};

    // nnSDK saves this value as a float. Since it can only be 1.0f or 0.0f we simplify this value
    // by converting it to a bool
    Settings::values.vibration_enabled.SetValue(can_vibrate);

    LOG_DEBUG(Service_HID, "called, can_vibrate={}", can_vibrate);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::IsVibrationPermitted(HLERequestContext& ctx) {
    LOG_DEBUG(Service_HID, "called");

    // nnSDK checks if a float is greater than zero. We return the bool we stored earlier
    const auto is_enabled = Settings::values.vibration_enabled.GetValue();

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(is_enabled);
}

void Hid::SendVibrationValues(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    const auto handles = ctx.ReadBuffer(0);
    const auto vibrations = ctx.ReadBuffer(1);

    std::vector<Core::HID::VibrationDeviceHandle> vibration_device_handles(
        handles.size() / sizeof(Core::HID::VibrationDeviceHandle));
    std::vector<Core::HID::VibrationValue> vibration_values(vibrations.size() /
                                                            sizeof(Core::HID::VibrationValue));

    std::memcpy(vibration_device_handles.data(), handles.data(), handles.size());
    std::memcpy(vibration_values.data(), vibrations.data(), vibrations.size());

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .VibrateControllers(vibration_device_handles, vibration_values);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SendVibrationGcErmCommand(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::VibrationDeviceHandle vibration_device_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Core::HID::VibrationGcErmCommand gc_erm_command;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    /**
     * Note: This uses yuzu-specific behavior such that the StopHard command produces
     * vibrations where freq_low == 0.0f and freq_high == 0.0f, as defined below,
     * in order to differentiate between Stop and StopHard commands.
     * This is done to reuse the controller vibration functions made for regular controllers.
     */
    const auto vibration_value = [parameters] {
        switch (parameters.gc_erm_command) {
        case Core::HID::VibrationGcErmCommand::Stop:
            return Core::HID::VibrationValue{
                .low_amplitude = 0.0f,
                .low_frequency = 160.0f,
                .high_amplitude = 0.0f,
                .high_frequency = 320.0f,
            };
        case Core::HID::VibrationGcErmCommand::Start:
            return Core::HID::VibrationValue{
                .low_amplitude = 1.0f,
                .low_frequency = 160.0f,
                .high_amplitude = 1.0f,
                .high_frequency = 320.0f,
            };
        case Core::HID::VibrationGcErmCommand::StopHard:
            return Core::HID::VibrationValue{
                .low_amplitude = 0.0f,
                .low_frequency = 0.0f,
                .high_amplitude = 0.0f,
                .high_frequency = 0.0f,
            };
        default:
            return Core::HID::DEFAULT_VIBRATION_VALUE;
        }
    }();

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .VibrateController(parameters.vibration_device_handle, vibration_value);

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}, "
              "gc_erm_command={}",
              parameters.vibration_device_handle.npad_type,
              parameters.vibration_device_handle.npad_id,
              parameters.vibration_device_handle.device_index, parameters.applet_resource_user_id,
              parameters.gc_erm_command);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetActualVibrationGcErmCommand(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::VibrationDeviceHandle vibration_device_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };

    const auto parameters{rp.PopRaw<Parameters>()};

    const auto last_vibration = applet_resource->GetController<Controller_NPad>(HidController::NPad)
                                    .GetLastVibration(parameters.vibration_device_handle);

    const auto gc_erm_command = [last_vibration] {
        if (last_vibration.low_amplitude != 0.0f || last_vibration.high_amplitude != 0.0f) {
            return Core::HID::VibrationGcErmCommand::Start;
        }

        /**
         * Note: This uses yuzu-specific behavior such that the StopHard command produces
         * vibrations where freq_low == 0.0f and freq_high == 0.0f, as defined in the HID function
         * SendVibrationGcErmCommand, in order to differentiate between Stop and StopHard commands.
         * This is done to reuse the controller vibration functions made for regular controllers.
         */
        if (last_vibration.low_frequency == 0.0f && last_vibration.high_frequency == 0.0f) {
            return Core::HID::VibrationGcErmCommand::StopHard;
        }

        return Core::HID::VibrationGcErmCommand::Stop;
    }();

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.vibration_device_handle.npad_type,
              parameters.vibration_device_handle.npad_id,
              parameters.vibration_device_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.PushEnum(gc_erm_command);
}

void Hid::BeginPermitVibrationSession(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .SetPermitVibrationSession(true);

    LOG_DEBUG(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::EndPermitVibrationSession(HLERequestContext& ctx) {
    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .SetPermitVibrationSession(false);

    LOG_DEBUG(Service_HID, "called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::IsVibrationDeviceMounted(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::VibrationDeviceHandle vibration_device_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_DEBUG(Service_HID,
              "called, npad_type={}, npad_id={}, device_index={}, applet_resource_user_id={}",
              parameters.vibration_device_handle.npad_type,
              parameters.vibration_device_handle.npad_id,
              parameters.vibration_device_handle.device_index, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(applet_resource->GetController<Controller_NPad>(HidController::NPad)
                .IsVibrationDeviceMounted(parameters.vibration_device_handle));
}

void Hid::ActivateConsoleSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::ConsoleSixAxisSensor);

    LOG_WARNING(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::StartConsoleSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::ConsoleSixAxisSensorHandle console_sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID,
                "(STUBBED) called, unknown_1={}, unknown_2={}, applet_resource_user_id={}",
                parameters.console_sixaxis_handle.unknown_1,
                parameters.console_sixaxis_handle.unknown_2, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::StopConsoleSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::ConsoleSixAxisSensorHandle console_sixaxis_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID,
                "(STUBBED) called, unknown_1={}, unknown_2={}, applet_resource_user_id={}",
                parameters.console_sixaxis_handle.unknown_1,
                parameters.console_sixaxis_handle.unknown_2, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ActivateSevenSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->ActivateController(HidController::ConsoleSixAxisSensor);

    LOG_WARNING(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::StartSevenSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::StopSevenSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::InitializeSevenSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};
    const auto t_mem_1_size{rp.Pop<u64>()};
    const auto t_mem_2_size{rp.Pop<u64>()};
    const auto t_mem_1_handle{ctx.GetCopyHandle(0)};
    const auto t_mem_2_handle{ctx.GetCopyHandle(1)};

    ASSERT_MSG(t_mem_1_size == 0x1000, "t_mem_1_size is not 0x1000 bytes");
    ASSERT_MSG(t_mem_2_size == 0x7F000, "t_mem_2_size is not 0x7F000 bytes");

    auto t_mem_1 = system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
        t_mem_1_handle);

    if (t_mem_1.IsNull()) {
        LOG_ERROR(Service_HID, "t_mem_1 is a nullptr for handle=0x{:08X}", t_mem_1_handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    auto t_mem_2 = system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
        t_mem_2_handle);

    if (t_mem_2.IsNull()) {
        LOG_ERROR(Service_HID, "t_mem_2 is a nullptr for handle=0x{:08X}", t_mem_2_handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    ASSERT_MSG(t_mem_1->GetSize() == 0x1000, "t_mem_1 has incorrect size");
    ASSERT_MSG(t_mem_2->GetSize() == 0x7F000, "t_mem_2 has incorrect size");

    // Activate console six axis controller
    applet_resource->GetController<Controller_ConsoleSixAxis>(HidController::ConsoleSixAxisSensor)
        .ActivateController();

    applet_resource->GetController<Controller_ConsoleSixAxis>(HidController::ConsoleSixAxisSensor)
        .SetTransferMemoryAddress(t_mem_1->GetSourceAddress());

    LOG_WARNING(Service_HID,
                "called, t_mem_1_handle=0x{:08X}, t_mem_2_handle=0x{:08X}, "
                "applet_resource_user_id={}",
                t_mem_1_handle, t_mem_2_handle, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::FinalizeSevenSixAxisSensor(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ResetSevenSixAxisSensorTimestamp(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    applet_resource->GetController<Controller_ConsoleSixAxis>(HidController::ConsoleSixAxisSensor)
        .ResetTimestamp();

    LOG_WARNING(Service_HID, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::IsUsbFullKeyControllerEnabled(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(false);
}

void Hid::GetPalmaConnectionHandle(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, npad_id={}, applet_resource_user_id={}",
                parameters.npad_id, parameters.applet_resource_user_id);

    Controller_Palma::PalmaConnectionHandle handle;
    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.GetPalmaConnectionHandle(parameters.npad_id, handle);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result);
    rb.PushRaw(handle);
}

void Hid::InitializePalma(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.InitializePalma(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::AcquirePalmaOperationCompleteEvent(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(controller.AcquirePalmaOperationCompleteEvent(connection_handle));
}

void Hid::GetPalmaOperationInfo(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    Controller_Palma::PalmaOperationType operation_type;
    Controller_Palma::PalmaOperationData data;
    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.GetPalmaOperationInfo(connection_handle, operation_type, data);

    if (result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
    }

    ctx.WriteBuffer(data);
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result);
    rb.Push(static_cast<u64>(operation_type));
}

void Hid::PlayPalmaActivity(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};
    const auto palma_activity{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}, palma_activity={}",
                connection_handle.npad_id, palma_activity);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.PlayPalmaActivity(connection_handle, palma_activity);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::SetPalmaFrModeType(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};
    const auto fr_mode{rp.PopEnum<Controller_Palma::PalmaFrModeType>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}, fr_mode={}",
                connection_handle.npad_id, fr_mode);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.SetPalmaFrModeType(connection_handle, fr_mode);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ReadPalmaStep(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.ReadPalmaStep(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::EnablePalmaStep(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool is_enabled;
        INSERT_PADDING_WORDS_NOINIT(1);
        Controller_Palma::PalmaConnectionHandle connection_handle;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}, is_enabled={}",
                parameters.connection_handle.npad_id, parameters.is_enabled);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result =
        controller.EnablePalmaStep(parameters.connection_handle, parameters.is_enabled);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ResetPalmaStep(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    auto& controller = GetAppletResource()->GetController<Controller_Palma>(HidController::Palma);
    const auto result = controller.ResetPalmaStep(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ReadPalmaApplicationSection(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::WritePalmaApplicationSection(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ReadPalmaUniqueCode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .ReadPalmaUniqueCode(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetPalmaUniqueCodeInvalid(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .SetPalmaUniqueCodeInvalid(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::WritePalmaActivityEntry(HLERequestContext& ctx) {
    LOG_CRITICAL(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::WritePalmaRgbLedPatternEntry(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};
    const auto unknown{rp.Pop<u64>()};

    [[maybe_unused]] const auto buffer = ctx.ReadBuffer();

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}, unknown={}",
                connection_handle.npad_id, unknown);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .WritePalmaRgbLedPatternEntry(connection_handle, unknown);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::WritePalmaWaveEntry(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};
    const auto wave_set{rp.PopEnum<Controller_Palma::PalmaWaveSet>()};
    const auto unknown{rp.Pop<u64>()};
    const auto t_mem_size{rp.Pop<u64>()};
    const auto t_mem_handle{ctx.GetCopyHandle(0)};
    const auto size{rp.Pop<u64>()};

    ASSERT_MSG(t_mem_size == 0x3000, "t_mem_size is not 0x3000 bytes");

    auto t_mem = system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
        t_mem_handle);

    if (t_mem.IsNull()) {
        LOG_ERROR(Service_HID, "t_mem is a nullptr for handle=0x{:08X}", t_mem_handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    ASSERT_MSG(t_mem->GetSize() == 0x3000, "t_mem has incorrect size");

    LOG_WARNING(Service_HID,
                "(STUBBED) called, connection_handle={}, wave_set={}, unknown={}, "
                "t_mem_handle=0x{:08X}, t_mem_size={}, size={}",
                connection_handle.npad_id, wave_set, unknown, t_mem_handle, t_mem_size, size);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .WritePalmaWaveEntry(connection_handle, wave_set, t_mem->GetSourceAddress(), t_mem_size);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetPalmaDataBaseIdentificationVersion(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        s32 database_id_version;
        INSERT_PADDING_WORDS_NOINIT(1);
        Controller_Palma::PalmaConnectionHandle connection_handle;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}, database_id_version={}",
                parameters.connection_handle.npad_id, parameters.database_id_version);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .SetPalmaDataBaseIdentificationVersion(parameters.connection_handle,
                                               parameters.database_id_version);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetPalmaDataBaseIdentificationVersion(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .GetPalmaDataBaseIdentificationVersion(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SuspendPalmaFeature(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetPalmaOperationResult(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    const auto result = applet_resource->GetController<Controller_Palma>(HidController::Palma)
                            .GetPalmaOperationResult(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void Hid::ReadPalmaPlayLog(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::ResetPalmaPlayLog(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetIsPalmaAllConnectable(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool is_palma_all_connectable;
        INSERT_PADDING_BYTES_NOINIT(7);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID,
                "(STUBBED) called, is_palma_all_connectable={},applet_resource_user_id={}",
                parameters.is_palma_all_connectable, parameters.applet_resource_user_id);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .SetIsPalmaAllConnectable(parameters.is_palma_all_connectable);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetIsPalmaPairedConnectable(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::PairPalma(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto connection_handle{rp.PopRaw<Controller_Palma::PalmaConnectionHandle>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, connection_handle={}", connection_handle.npad_id);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .PairPalma(connection_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetPalmaBoostMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto palma_boost_mode{rp.Pop<bool>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, palma_boost_mode={}", palma_boost_mode);

    applet_resource->GetController<Controller_Palma>(HidController::Palma)
        .SetPalmaBoostMode(palma_boost_mode);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::CancelWritePalmaWaveEntry(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::EnablePalmaBoostMode(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetPalmaBluetoothAddress(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetDisallowedPalmaConnection(HLERequestContext& ctx) {
    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::SetNpadCommunicationMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};
    const auto communication_mode{rp.PopEnum<Controller_NPad::NpadCommunicationMode>()};

    applet_resource->GetController<Controller_NPad>(HidController::NPad)
        .SetNpadCommunicationMode(communication_mode);

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}, communication_mode={}",
                applet_resource_user_id, communication_mode);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::GetNpadCommunicationMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    LOG_WARNING(Service_HID, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.PushEnum(applet_resource->GetController<Controller_NPad>(HidController::NPad)
                    .GetNpadCommunicationMode());
}

void Hid::SetTouchScreenConfiguration(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto touchscreen_mode{rp.PopRaw<Controller_Touchscreen::TouchScreenConfigurationForNx>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, touchscreen_mode={}, applet_resource_user_id={}",
                touchscreen_mode.mode, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Hid::IsFirmwareUpdateNeededForNotification(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        s32 unknown;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, unknown={}, applet_resource_user_id={}",
                parameters.unknown, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(false);
}

class HidDbg final : public ServiceFramework<HidDbg> {
public:
    explicit HidDbg(Core::System& system_) : ServiceFramework{system_, "hid:dbg"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "DeactivateDebugPad"},
            {1, nullptr, "SetDebugPadAutoPilotState"},
            {2, nullptr, "UnsetDebugPadAutoPilotState"},
            {10, nullptr, "DeactivateTouchScreen"},
            {11, nullptr, "SetTouchScreenAutoPilotState"},
            {12, nullptr, "UnsetTouchScreenAutoPilotState"},
            {13, nullptr, "GetTouchScreenConfiguration"},
            {14, nullptr, "ProcessTouchScreenAutoTune"},
            {15, nullptr, "ForceStopTouchScreenManagement"},
            {16, nullptr, "ForceRestartTouchScreenManagement"},
            {17, nullptr, "IsTouchScreenManaged"},
            {20, nullptr, "DeactivateMouse"},
            {21, nullptr, "SetMouseAutoPilotState"},
            {22, nullptr, "UnsetMouseAutoPilotState"},
            {25, nullptr, "SetDebugMouseAutoPilotState"},
            {26, nullptr, "UnsetDebugMouseAutoPilotState"},
            {30, nullptr, "DeactivateKeyboard"},
            {31, nullptr, "SetKeyboardAutoPilotState"},
            {32, nullptr, "UnsetKeyboardAutoPilotState"},
            {50, nullptr, "DeactivateXpad"},
            {51, nullptr, "SetXpadAutoPilotState"},
            {52, nullptr, "UnsetXpadAutoPilotState"},
            {53, nullptr, "DeactivateJoyXpad"},
            {60, nullptr, "ClearNpadSystemCommonPolicy"},
            {61, nullptr, "DeactivateNpad"},
            {62, nullptr, "ForceDisconnectNpad"},
            {91, nullptr, "DeactivateGesture"},
            {110, nullptr, "DeactivateHomeButton"},
            {111, nullptr, "SetHomeButtonAutoPilotState"},
            {112, nullptr, "UnsetHomeButtonAutoPilotState"},
            {120, nullptr, "DeactivateSleepButton"},
            {121, nullptr, "SetSleepButtonAutoPilotState"},
            {122, nullptr, "UnsetSleepButtonAutoPilotState"},
            {123, nullptr, "DeactivateInputDetector"},
            {130, nullptr, "DeactivateCaptureButton"},
            {131, nullptr, "SetCaptureButtonAutoPilotState"},
            {132, nullptr, "UnsetCaptureButtonAutoPilotState"},
            {133, nullptr, "SetShiftAccelerometerCalibrationValue"},
            {134, nullptr, "GetShiftAccelerometerCalibrationValue"},
            {135, nullptr, "SetShiftGyroscopeCalibrationValue"},
            {136, nullptr, "GetShiftGyroscopeCalibrationValue"},
            {140, nullptr, "DeactivateConsoleSixAxisSensor"},
            {141, nullptr, "GetConsoleSixAxisSensorSamplingFrequency"},
            {142, nullptr, "DeactivateSevenSixAxisSensor"},
            {143, nullptr, "GetConsoleSixAxisSensorCountStates"},
            {144, nullptr, "GetAccelerometerFsr"},
            {145, nullptr, "SetAccelerometerFsr"},
            {146, nullptr, "GetAccelerometerOdr"},
            {147, nullptr, "SetAccelerometerOdr"},
            {148, nullptr, "GetGyroscopeFsr"},
            {149, nullptr, "SetGyroscopeFsr"},
            {150, nullptr, "GetGyroscopeOdr"},
            {151, nullptr, "SetGyroscopeOdr"},
            {152, nullptr, "GetWhoAmI"},
            {201, nullptr, "ActivateFirmwareUpdate"},
            {202, nullptr, "DeactivateFirmwareUpdate"},
            {203, nullptr, "StartFirmwareUpdate"},
            {204, nullptr, "GetFirmwareUpdateStage"},
            {205, nullptr, "GetFirmwareVersion"},
            {206, nullptr, "GetDestinationFirmwareVersion"},
            {207, nullptr, "DiscardFirmwareInfoCacheForRevert"},
            {208, nullptr, "StartFirmwareUpdateForRevert"},
            {209, nullptr, "GetAvailableFirmwareVersionForRevert"},
            {210, nullptr, "IsFirmwareUpdatingDevice"},
            {211, nullptr, "StartFirmwareUpdateIndividual"},
            {215, nullptr, "SetUsbFirmwareForceUpdateEnabled"},
            {216, nullptr, "SetAllKuinaDevicesToFirmwareUpdateMode"},
            {221, nullptr, "UpdateControllerColor"},
            {222, nullptr, "ConnectUsbPadsAsync"},
            {223, nullptr, "DisconnectUsbPadsAsync"},
            {224, nullptr, "UpdateDesignInfo"},
            {225, nullptr, "GetUniquePadDriverState"},
            {226, nullptr, "GetSixAxisSensorDriverStates"},
            {227, nullptr, "GetRxPacketHistory"},
            {228, nullptr, "AcquireOperationEventHandle"},
            {229, nullptr, "ReadSerialFlash"},
            {230, nullptr, "WriteSerialFlash"},
            {231, nullptr, "GetOperationResult"},
            {232, nullptr, "EnableShipmentMode"},
            {233, nullptr, "ClearPairingInfo"},
            {234, nullptr, "GetUniquePadDeviceTypeSetInternal"},
            {235, nullptr, "EnableAnalogStickPower"},
            {236, nullptr, "RequestKuinaUartClockCal"},
            {237, nullptr, "GetKuinaUartClockCal"},
            {238, nullptr, "SetKuinaUartClockTrim"},
            {239, nullptr, "KuinaLoopbackTest"},
            {240, nullptr, "RequestBatteryVoltage"},
            {241, nullptr, "GetBatteryVoltage"},
            {242, nullptr, "GetUniquePadPowerInfo"},
            {243, nullptr, "RebootUniquePad"},
            {244, nullptr, "RequestKuinaFirmwareVersion"},
            {245, nullptr, "GetKuinaFirmwareVersion"},
            {246, nullptr, "GetVidPid"},
            {247, nullptr, "GetAnalogStickCalibrationValue"},
            {248, nullptr, "GetUniquePadIdsFull"},
            {249, nullptr, "ConnectUniquePad"},
            {250, nullptr, "IsVirtual"},
            {251, nullptr, "GetAnalogStickModuleParam"},
            {301, nullptr, "GetAbstractedPadHandles"},
            {302, nullptr, "GetAbstractedPadState"},
            {303, nullptr, "GetAbstractedPadsState"},
            {321, nullptr, "SetAutoPilotVirtualPadState"},
            {322, nullptr, "UnsetAutoPilotVirtualPadState"},
            {323, nullptr, "UnsetAllAutoPilotVirtualPadState"},
            {324, nullptr, "AttachHdlsWorkBuffer"},
            {325, nullptr, "ReleaseHdlsWorkBuffer"},
            {326, nullptr, "DumpHdlsNpadAssignmentState"},
            {327, nullptr, "DumpHdlsStates"},
            {328, nullptr, "ApplyHdlsNpadAssignmentState"},
            {329, nullptr, "ApplyHdlsStateList"},
            {330, nullptr, "AttachHdlsVirtualDevice"},
            {331, nullptr, "DetachHdlsVirtualDevice"},
            {332, nullptr, "SetHdlsState"},
            {350, nullptr, "AddRegisteredDevice"},
            {400, nullptr, "DisableExternalMcuOnNxDevice"},
            {401, nullptr, "DisableRailDeviceFiltering"},
            {402, nullptr, "EnableWiredPairing"},
            {403, nullptr, "EnableShipmentModeAutoClear"},
            {404, nullptr, "SetRailEnabled"},
            {500, nullptr, "SetFactoryInt"},
            {501, nullptr, "IsFactoryBootEnabled"},
            {550, nullptr, "SetAnalogStickModelDataTemporarily"},
            {551, nullptr, "GetAnalogStickModelData"},
            {552, nullptr, "ResetAnalogStickModelData"},
            {600, nullptr, "ConvertPadState"},
            {650, nullptr, "AddButtonPlayData"},
            {651, nullptr, "StartButtonPlayData"},
            {652, nullptr, "StopButtonPlayData"},
            {2000, nullptr, "DeactivateDigitizer"},
            {2001, nullptr, "SetDigitizerAutoPilotState"},
            {2002, nullptr, "UnsetDigitizerAutoPilotState"},
            {2002, nullptr, "ReloadFirmwareDebugSettings"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class HidSys final : public ServiceFramework<HidSys> {
public:
    explicit HidSys(Core::System& system_) : ServiceFramework{system_, "hid:sys"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {31, nullptr, "SendKeyboardLockKeyEvent"},
            {101, nullptr, "AcquireHomeButtonEventHandle"},
            {111, nullptr, "ActivateHomeButton"},
            {121, nullptr, "AcquireSleepButtonEventHandle"},
            {131, nullptr, "ActivateSleepButton"},
            {141, nullptr, "AcquireCaptureButtonEventHandle"},
            {151, nullptr, "ActivateCaptureButton"},
            {161, nullptr, "GetPlatformConfig"},
            {210, nullptr, "AcquireNfcDeviceUpdateEventHandle"},
            {211, nullptr, "GetNpadsWithNfc"},
            {212, nullptr, "AcquireNfcActivateEventHandle"},
            {213, nullptr, "ActivateNfc"},
            {214, nullptr, "GetXcdHandleForNpadWithNfc"},
            {215, nullptr, "IsNfcActivated"},
            {230, nullptr, "AcquireIrSensorEventHandle"},
            {231, nullptr, "ActivateIrSensor"},
            {232, nullptr, "GetIrSensorState"},
            {233, nullptr, "GetXcdHandleForNpadWithIrSensor"},
            {301, nullptr, "ActivateNpadSystem"},
            {303, &HidSys::ApplyNpadSystemCommonPolicy, "ApplyNpadSystemCommonPolicy"},
            {304, nullptr, "EnableAssigningSingleOnSlSrPress"},
            {305, nullptr, "DisableAssigningSingleOnSlSrPress"},
            {306, nullptr, "GetLastActiveNpad"},
            {307, nullptr, "GetNpadSystemExtStyle"},
            {308, nullptr, "ApplyNpadSystemCommonPolicyFull"},
            {309, nullptr, "GetNpadFullKeyGripColor"},
            {310, nullptr, "GetMaskedSupportedNpadStyleSet"},
            {311, nullptr, "SetNpadPlayerLedBlinkingDevice"},
            {312, nullptr, "SetSupportedNpadStyleSetAll"},
            {313, nullptr, "GetNpadCaptureButtonAssignment"},
            {314, nullptr, "GetAppletFooterUiType"},
            {315, nullptr, "GetAppletDetailedUiType"},
            {316, nullptr, "GetNpadInterfaceType"},
            {317, nullptr, "GetNpadLeftRightInterfaceType"},
            {318, nullptr, "HasBattery"},
            {319, nullptr, "HasLeftRightBattery"},
            {321, &HidSys::GetUniquePadsFromNpad, "GetUniquePadsFromNpad"},
            {322, nullptr, "GetIrSensorState"},
            {323, nullptr, "GetXcdHandleForNpadWithIrSensor"},
            {324, nullptr, "GetUniquePadButtonSet"},
            {325, nullptr, "GetUniquePadColor"},
            {326, nullptr, "GetUniquePadAppletDetailedUiType"},
            {327, nullptr, "GetAbstractedPadIdDataFromNpad"},
            {328, nullptr, "AttachAbstractedPadToNpad"},
            {329, nullptr, "DetachAbstractedPadAll"},
            {330, nullptr, "CheckAbstractedPadConnection"},
            {500, nullptr, "SetAppletResourceUserId"},
            {501, nullptr, "RegisterAppletResourceUserId"},
            {502, nullptr, "UnregisterAppletResourceUserId"},
            {503, nullptr, "EnableAppletToGetInput"},
            {504, nullptr, "SetAruidValidForVibration"},
            {505, nullptr, "EnableAppletToGetSixAxisSensor"},
            {506, nullptr, "EnableAppletToGetPadInput"},
            {507, nullptr, "EnableAppletToGetTouchScreen"},
            {510, nullptr, "SetVibrationMasterVolume"},
            {511, nullptr, "GetVibrationMasterVolume"},
            {512, nullptr, "BeginPermitVibrationSession"},
            {513, nullptr, "EndPermitVibrationSession"},
            {514, nullptr, "Unknown514"},
            {520, nullptr, "EnableHandheldHids"},
            {521, nullptr, "DisableHandheldHids"},
            {522, nullptr, "SetJoyConRailEnabled"},
            {523, nullptr, "IsJoyConRailEnabled"},
            {524, nullptr, "IsHandheldHidsEnabled"},
            {525, nullptr, "IsJoyConAttachedOnAllRail"},
            {540, nullptr, "AcquirePlayReportControllerUsageUpdateEvent"},
            {541, nullptr, "GetPlayReportControllerUsages"},
            {542, nullptr, "AcquirePlayReportRegisteredDeviceUpdateEvent"},
            {543, nullptr, "GetRegisteredDevicesOld"},
            {544, nullptr, "AcquireConnectionTriggerTimeoutEvent"},
            {545, nullptr, "SendConnectionTrigger"},
            {546, nullptr, "AcquireDeviceRegisteredEventForControllerSupport"},
            {547, nullptr, "GetAllowedBluetoothLinksCount"},
            {548, nullptr, "GetRegisteredDevices"},
            {549, nullptr, "GetConnectableRegisteredDevices"},
            {700, nullptr, "ActivateUniquePad"},
            {702, nullptr, "AcquireUniquePadConnectionEventHandle"},
            {703, nullptr, "GetUniquePadIds"},
            {751, nullptr, "AcquireJoyDetachOnBluetoothOffEventHandle"},
            {800, nullptr, "ListSixAxisSensorHandles"},
            {801, nullptr, "IsSixAxisSensorUserCalibrationSupported"},
            {802, nullptr, "ResetSixAxisSensorCalibrationValues"},
            {803, nullptr, "StartSixAxisSensorUserCalibration"},
            {804, nullptr, "CancelSixAxisSensorUserCalibration"},
            {805, nullptr, "GetUniquePadBluetoothAddress"},
            {806, nullptr, "DisconnectUniquePad"},
            {807, nullptr, "GetUniquePadType"},
            {808, nullptr, "GetUniquePadInterface"},
            {809, nullptr, "GetUniquePadSerialNumber"},
            {810, nullptr, "GetUniquePadControllerNumber"},
            {811, nullptr, "GetSixAxisSensorUserCalibrationStage"},
            {812, nullptr, "GetConsoleUniqueSixAxisSensorHandle"},
            {821, nullptr, "StartAnalogStickManualCalibration"},
            {822, nullptr, "RetryCurrentAnalogStickManualCalibrationStage"},
            {823, nullptr, "CancelAnalogStickManualCalibration"},
            {824, nullptr, "ResetAnalogStickManualCalibration"},
            {825, nullptr, "GetAnalogStickState"},
            {826, nullptr, "GetAnalogStickManualCalibrationStage"},
            {827, nullptr, "IsAnalogStickButtonPressed"},
            {828, nullptr, "IsAnalogStickInReleasePosition"},
            {829, nullptr, "IsAnalogStickInCircumference"},
            {830, nullptr, "SetNotificationLedPattern"},
            {831, nullptr, "SetNotificationLedPatternWithTimeout"},
            {832, nullptr, "PrepareHidsForNotificationWake"},
            {850, nullptr, "IsUsbFullKeyControllerEnabled"},
            {851, nullptr, "EnableUsbFullKeyController"},
            {852, nullptr, "IsUsbConnected"},
            {870, nullptr, "IsHandheldButtonPressedOnConsoleMode"},
            {900, nullptr, "ActivateInputDetector"},
            {901, nullptr, "NotifyInputDetector"},
            {1000, nullptr, "InitializeFirmwareUpdate"},
            {1001, nullptr, "GetFirmwareVersion"},
            {1002, nullptr, "GetAvailableFirmwareVersion"},
            {1003, nullptr, "IsFirmwareUpdateAvailable"},
            {1004, nullptr, "CheckFirmwareUpdateRequired"},
            {1005, nullptr, "StartFirmwareUpdate"},
            {1006, nullptr, "AbortFirmwareUpdate"},
            {1007, nullptr, "GetFirmwareUpdateState"},
            {1008, nullptr, "ActivateAudioControl"},
            {1009, nullptr, "AcquireAudioControlEventHandle"},
            {1010, nullptr, "GetAudioControlStates"},
            {1011, nullptr, "DeactivateAudioControl"},
            {1050, nullptr, "IsSixAxisSensorAccurateUserCalibrationSupported"},
            {1051, nullptr, "StartSixAxisSensorAccurateUserCalibration"},
            {1052, nullptr, "CancelSixAxisSensorAccurateUserCalibration"},
            {1053, nullptr, "GetSixAxisSensorAccurateUserCalibrationState"},
            {1100, nullptr, "GetHidbusSystemServiceObject"},
            {1120, nullptr, "SetFirmwareHotfixUpdateSkipEnabled"},
            {1130, nullptr, "InitializeUsbFirmwareUpdate"},
            {1131, nullptr, "FinalizeUsbFirmwareUpdate"},
            {1132, nullptr, "CheckUsbFirmwareUpdateRequired"},
            {1133, nullptr, "StartUsbFirmwareUpdate"},
            {1134, nullptr, "GetUsbFirmwareUpdateState"},
            {1150, nullptr, "SetTouchScreenMagnification"},
            {1151, nullptr, "GetTouchScreenFirmwareVersion"},
            {1152, nullptr, "SetTouchScreenDefaultConfiguration"},
            {1153, nullptr, "GetTouchScreenDefaultConfiguration"},
            {1154, nullptr, "IsFirmwareAvailableForNotification"},
            {1155, nullptr, "SetForceHandheldStyleVibration"},
            {1156, nullptr, "SendConnectionTriggerWithoutTimeoutEvent"},
            {1157, nullptr, "CancelConnectionTrigger"},
            {1200, nullptr, "IsButtonConfigSupported"},
            {1201, nullptr, "IsButtonConfigEmbeddedSupported"},
            {1202, nullptr, "DeleteButtonConfig"},
            {1203, nullptr, "DeleteButtonConfigEmbedded"},
            {1204, nullptr, "SetButtonConfigEnabled"},
            {1205, nullptr, "SetButtonConfigEmbeddedEnabled"},
            {1206, nullptr, "IsButtonConfigEnabled"},
            {1207, nullptr, "IsButtonConfigEmbeddedEnabled"},
            {1208, nullptr, "SetButtonConfigEmbedded"},
            {1209, nullptr, "SetButtonConfigFull"},
            {1210, nullptr, "SetButtonConfigLeft"},
            {1211, nullptr, "SetButtonConfigRight"},
            {1212, nullptr, "GetButtonConfigEmbedded"},
            {1213, nullptr, "GetButtonConfigFull"},
            {1214, nullptr, "GetButtonConfigLeft"},
            {1215, nullptr, "GetButtonConfigRight"},
            {1250, nullptr, "IsCustomButtonConfigSupported"},
            {1251, nullptr, "IsDefaultButtonConfigEmbedded"},
            {1252, nullptr, "IsDefaultButtonConfigFull"},
            {1253, nullptr, "IsDefaultButtonConfigLeft"},
            {1254, nullptr, "IsDefaultButtonConfigRight"},
            {1255, nullptr, "IsButtonConfigStorageEmbeddedEmpty"},
            {1256, nullptr, "IsButtonConfigStorageFullEmpty"},
            {1257, nullptr, "IsButtonConfigStorageLeftEmpty"},
            {1258, nullptr, "IsButtonConfigStorageRightEmpty"},
            {1259, nullptr, "GetButtonConfigStorageEmbeddedDeprecated"},
            {1260, nullptr, "GetButtonConfigStorageFullDeprecated"},
            {1261, nullptr, "GetButtonConfigStorageLeftDeprecated"},
            {1262, nullptr, "GetButtonConfigStorageRightDeprecated"},
            {1263, nullptr, "SetButtonConfigStorageEmbeddedDeprecated"},
            {1264, nullptr, "SetButtonConfigStorageFullDeprecated"},
            {1265, nullptr, "SetButtonConfigStorageLeftDeprecated"},
            {1266, nullptr, "SetButtonConfigStorageRightDeprecated"},
            {1267, nullptr, "DeleteButtonConfigStorageEmbedded"},
            {1268, nullptr, "DeleteButtonConfigStorageFull"},
            {1269, nullptr, "DeleteButtonConfigStorageLeft"},
            {1270, nullptr, "DeleteButtonConfigStorageRight"},
            {1271, nullptr, "IsUsingCustomButtonConfig"},
            {1272, nullptr, "IsAnyCustomButtonConfigEnabled"},
            {1273, nullptr, "SetAllCustomButtonConfigEnabled"},
            {1274, nullptr, "SetDefaultButtonConfig"},
            {1275, nullptr, "SetAllDefaultButtonConfig"},
            {1276, nullptr, "SetHidButtonConfigEmbedded"},
            {1277, nullptr, "SetHidButtonConfigFull"},
            {1278, nullptr, "SetHidButtonConfigLeft"},
            {1279, nullptr, "SetHidButtonConfigRight"},
            {1280, nullptr, "GetHidButtonConfigEmbedded"},
            {1281, nullptr, "GetHidButtonConfigFull"},
            {1282, nullptr, "GetHidButtonConfigLeft"},
            {1283, nullptr, "GetHidButtonConfigRight"},
            {1284, nullptr, "GetButtonConfigStorageEmbedded"},
            {1285, nullptr, "GetButtonConfigStorageFull"},
            {1286, nullptr, "GetButtonConfigStorageLeft"},
            {1287, nullptr, "GetButtonConfigStorageRight"},
            {1288, nullptr, "SetButtonConfigStorageEmbedded"},
            {1289, nullptr, "SetButtonConfigStorageFull"},
            {1290, nullptr, "DeleteButtonConfigStorageRight"},
            {1291, nullptr, "DeleteButtonConfigStorageRight"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void ApplyNpadSystemCommonPolicy(HLERequestContext& ctx) {
        // We already do this for homebrew so we can just stub it out
        LOG_WARNING(Service_HID, "called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetUniquePadsFromNpad(HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto npad_id_type{rp.PopEnum<Core::HID::NpadIdType>()};

        const s64 total_entries = 0;
        LOG_WARNING(Service_HID, "(STUBBED) called, npad_id_type={}", npad_id_type);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(total_entries);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("hid", std::make_shared<Hid>(system));
    server_manager->RegisterNamedService("hidbus", std::make_shared<HidBus>(system));
    server_manager->RegisterNamedService("hid:dbg", std::make_shared<HidDbg>(system));
    server_manager->RegisterNamedService("hid:sys", std::make_shared<HidSys>(system));

    server_manager->RegisterNamedService("irs", std::make_shared<Service::IRS::IRS>(system));
    server_manager->RegisterNamedService("irs:sys",
                                         std::make_shared<Service::IRS::IRS_SYS>(system));

    server_manager->RegisterNamedService("xcd:sys", std::make_shared<XCD_SYS>(system));
    system.RunServer(std::move(server_manager));
}

} // namespace Service::HID
