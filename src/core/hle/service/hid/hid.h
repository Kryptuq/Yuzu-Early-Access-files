// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>

#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Core::Timing {
struct EventType;
}

namespace Service::SM {
class ServiceManager;
}

namespace Service::HID {

enum class HidController : std::size_t {
    DebugPad,
    Touchscreen,
    Mouse,
    Keyboard,
    XPad,
    HomeButton,
    SleepButton,
    CaptureButton,
    InputDetector,
    UniquePad,
    NPad,
    Gesture,
    ConsoleSixAxisSensor,
    DebugMouse,
    Palma,

    MaxControllers,
};

class IAppletResource final : public ServiceFramework<IAppletResource> {
public:
    explicit IAppletResource(Core::System& system_,
                             KernelHelpers::ServiceContext& service_context_);
    ~IAppletResource() override;

    void ActivateController(HidController controller);
    void DeactivateController(HidController controller);

    template <typename T>
    T& GetController(HidController controller) {
        return static_cast<T&>(*controllers[static_cast<size_t>(controller)]);
    }

    template <typename T>
    const T& GetController(HidController controller) const {
        return static_cast<T&>(*controllers[static_cast<size_t>(controller)]);
    }

private:
    template <typename T>
    void MakeController(HidController controller, u8* shared_memory) {
        if constexpr (std::is_constructible_v<T, Core::System&, u8*>) {
            controllers[static_cast<std::size_t>(controller)] =
                std::make_unique<T>(system, shared_memory);
        } else {
            controllers[static_cast<std::size_t>(controller)] =
                std::make_unique<T>(system.HIDCore(), shared_memory);
        }
    }

    template <typename T>
    void MakeControllerWithServiceContext(HidController controller, u8* shared_memory) {
        controllers[static_cast<std::size_t>(controller)] =
            std::make_unique<T>(system.HIDCore(), shared_memory, service_context);
    }

    void GetSharedMemoryHandle(HLERequestContext& ctx);
    void UpdateControllers(std::uintptr_t user_data, std::chrono::nanoseconds ns_late);
    void UpdateNpad(std::uintptr_t user_data, std::chrono::nanoseconds ns_late);
    void UpdateMouseKeyboard(std::uintptr_t user_data, std::chrono::nanoseconds ns_late);
    void UpdateMotion(std::uintptr_t user_data, std::chrono::nanoseconds ns_late);

    KernelHelpers::ServiceContext& service_context;

    std::shared_ptr<Core::Timing::EventType> npad_update_event;
    std::shared_ptr<Core::Timing::EventType> default_update_event;
    std::shared_ptr<Core::Timing::EventType> mouse_keyboard_update_event;
    std::shared_ptr<Core::Timing::EventType> motion_update_event;

    std::array<std::unique_ptr<ControllerBase>, static_cast<size_t>(HidController::MaxControllers)>
        controllers{};
};

class Hid final : public ServiceFramework<Hid> {
public:
    explicit Hid(Core::System& system_);
    ~Hid() override;

    std::shared_ptr<IAppletResource> GetAppletResource();

private:
    void CreateAppletResource(HLERequestContext& ctx);
    void ActivateDebugPad(HLERequestContext& ctx);
    void ActivateTouchScreen(HLERequestContext& ctx);
    void ActivateMouse(HLERequestContext& ctx);
    void ActivateKeyboard(HLERequestContext& ctx);
    void SendKeyboardLockKeyEvent(HLERequestContext& ctx);
    void ActivateXpad(HLERequestContext& ctx);
    void GetXpadIDs(HLERequestContext& ctx);
    void ActivateSixAxisSensor(HLERequestContext& ctx);
    void DeactivateSixAxisSensor(HLERequestContext& ctx);
    void StartSixAxisSensor(HLERequestContext& ctx);
    void StopSixAxisSensor(HLERequestContext& ctx);
    void IsSixAxisSensorFusionEnabled(HLERequestContext& ctx);
    void EnableSixAxisSensorFusion(HLERequestContext& ctx);
    void SetSixAxisSensorFusionParameters(HLERequestContext& ctx);
    void GetSixAxisSensorFusionParameters(HLERequestContext& ctx);
    void ResetSixAxisSensorFusionParameters(HLERequestContext& ctx);
    void SetGyroscopeZeroDriftMode(HLERequestContext& ctx);
    void GetGyroscopeZeroDriftMode(HLERequestContext& ctx);
    void ResetGyroscopeZeroDriftMode(HLERequestContext& ctx);
    void IsSixAxisSensorAtRest(HLERequestContext& ctx);
    void IsFirmwareUpdateAvailableForSixAxisSensor(HLERequestContext& ctx);
    void EnableSixAxisSensorUnalteredPassthrough(HLERequestContext& ctx);
    void IsSixAxisSensorUnalteredPassthroughEnabled(HLERequestContext& ctx);
    void LoadSixAxisSensorCalibrationParameter(HLERequestContext& ctx);
    void GetSixAxisSensorIcInformation(HLERequestContext& ctx);
    void ResetIsSixAxisSensorDeviceNewlyAssigned(HLERequestContext& ctx);
    void ActivateGesture(HLERequestContext& ctx);
    void SetSupportedNpadStyleSet(HLERequestContext& ctx);
    void GetSupportedNpadStyleSet(HLERequestContext& ctx);
    void SetSupportedNpadIdType(HLERequestContext& ctx);
    void ActivateNpad(HLERequestContext& ctx);
    void DeactivateNpad(HLERequestContext& ctx);
    void AcquireNpadStyleSetUpdateEventHandle(HLERequestContext& ctx);
    void DisconnectNpad(HLERequestContext& ctx);
    void GetPlayerLedPattern(HLERequestContext& ctx);
    void ActivateNpadWithRevision(HLERequestContext& ctx);
    void SetNpadJoyHoldType(HLERequestContext& ctx);
    void GetNpadJoyHoldType(HLERequestContext& ctx);
    void SetNpadJoyAssignmentModeSingleByDefault(HLERequestContext& ctx);
    void SetNpadJoyAssignmentModeSingle(HLERequestContext& ctx);
    void SetNpadJoyAssignmentModeDual(HLERequestContext& ctx);
    void MergeSingleJoyAsDualJoy(HLERequestContext& ctx);
    void StartLrAssignmentMode(HLERequestContext& ctx);
    void StopLrAssignmentMode(HLERequestContext& ctx);
    void SetNpadHandheldActivationMode(HLERequestContext& ctx);
    void GetNpadHandheldActivationMode(HLERequestContext& ctx);
    void SwapNpadAssignment(HLERequestContext& ctx);
    void IsUnintendedHomeButtonInputProtectionEnabled(HLERequestContext& ctx);
    void EnableUnintendedHomeButtonInputProtection(HLERequestContext& ctx);
    void SetNpadAnalogStickUseCenterClamp(HLERequestContext& ctx);
    void SetNpadCaptureButtonAssignment(HLERequestContext& ctx);
    void ClearNpadCaptureButtonAssignment(HLERequestContext& ctx);
    void GetVibrationDeviceInfo(HLERequestContext& ctx);
    void SendVibrationValue(HLERequestContext& ctx);
    void GetActualVibrationValue(HLERequestContext& ctx);
    void CreateActiveVibrationDeviceList(HLERequestContext& ctx);
    void PermitVibration(HLERequestContext& ctx);
    void IsVibrationPermitted(HLERequestContext& ctx);
    void SendVibrationValues(HLERequestContext& ctx);
    void SendVibrationGcErmCommand(HLERequestContext& ctx);
    void GetActualVibrationGcErmCommand(HLERequestContext& ctx);
    void BeginPermitVibrationSession(HLERequestContext& ctx);
    void EndPermitVibrationSession(HLERequestContext& ctx);
    void IsVibrationDeviceMounted(HLERequestContext& ctx);
    void ActivateConsoleSixAxisSensor(HLERequestContext& ctx);
    void StartConsoleSixAxisSensor(HLERequestContext& ctx);
    void StopConsoleSixAxisSensor(HLERequestContext& ctx);
    void ActivateSevenSixAxisSensor(HLERequestContext& ctx);
    void StartSevenSixAxisSensor(HLERequestContext& ctx);
    void StopSevenSixAxisSensor(HLERequestContext& ctx);
    void InitializeSevenSixAxisSensor(HLERequestContext& ctx);
    void FinalizeSevenSixAxisSensor(HLERequestContext& ctx);
    void ResetSevenSixAxisSensorTimestamp(HLERequestContext& ctx);
    void IsUsbFullKeyControllerEnabled(HLERequestContext& ctx);
    void GetPalmaConnectionHandle(HLERequestContext& ctx);
    void InitializePalma(HLERequestContext& ctx);
    void AcquirePalmaOperationCompleteEvent(HLERequestContext& ctx);
    void GetPalmaOperationInfo(HLERequestContext& ctx);
    void PlayPalmaActivity(HLERequestContext& ctx);
    void SetPalmaFrModeType(HLERequestContext& ctx);
    void ReadPalmaStep(HLERequestContext& ctx);
    void EnablePalmaStep(HLERequestContext& ctx);
    void ResetPalmaStep(HLERequestContext& ctx);
    void ReadPalmaApplicationSection(HLERequestContext& ctx);
    void WritePalmaApplicationSection(HLERequestContext& ctx);
    void ReadPalmaUniqueCode(HLERequestContext& ctx);
    void SetPalmaUniqueCodeInvalid(HLERequestContext& ctx);
    void WritePalmaActivityEntry(HLERequestContext& ctx);
    void WritePalmaRgbLedPatternEntry(HLERequestContext& ctx);
    void WritePalmaWaveEntry(HLERequestContext& ctx);
    void SetPalmaDataBaseIdentificationVersion(HLERequestContext& ctx);
    void GetPalmaDataBaseIdentificationVersion(HLERequestContext& ctx);
    void SuspendPalmaFeature(HLERequestContext& ctx);
    void GetPalmaOperationResult(HLERequestContext& ctx);
    void ReadPalmaPlayLog(HLERequestContext& ctx);
    void ResetPalmaPlayLog(HLERequestContext& ctx);
    void SetIsPalmaAllConnectable(HLERequestContext& ctx);
    void SetIsPalmaPairedConnectable(HLERequestContext& ctx);
    void PairPalma(HLERequestContext& ctx);
    void SetPalmaBoostMode(HLERequestContext& ctx);
    void CancelWritePalmaWaveEntry(HLERequestContext& ctx);
    void EnablePalmaBoostMode(HLERequestContext& ctx);
    void GetPalmaBluetoothAddress(HLERequestContext& ctx);
    void SetDisallowedPalmaConnection(HLERequestContext& ctx);
    void SetNpadCommunicationMode(HLERequestContext& ctx);
    void GetNpadCommunicationMode(HLERequestContext& ctx);
    void SetTouchScreenConfiguration(HLERequestContext& ctx);
    void IsFirmwareUpdateNeededForNotification(HLERequestContext& ctx);

    std::shared_ptr<IAppletResource> applet_resource;

    KernelHelpers::ServiceContext service_context;
};

void LoopProcess(Core::System& system);

} // namespace Service::HID
