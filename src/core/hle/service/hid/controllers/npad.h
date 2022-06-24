// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <mutex>

#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/vector_math.h"

#include "core/hid/hid_types.h"
#include "core/hle/service/hid/controllers/controller_base.h"
#include "core/hle/service/hid/ring_lifo.h"

namespace Core::HID {
class EmulatedController;
enum class ControllerTriggerType;
} // namespace Core::HID

namespace Kernel {
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Service::KernelHelpers {
class ServiceContext;
} // namespace Service::KernelHelpers

union ResultCode;

namespace Service::HID {

class Controller_NPad final : public ControllerBase {
public:
    explicit Controller_NPad(Core::HID::HIDCore& hid_core_, u8* raw_shared_memory_,
                             KernelHelpers::ServiceContext& service_context_);
    ~Controller_NPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing) override;

    // When the controller is requesting a motion update for the shared memory
    void OnMotionUpdate(const Core::Timing::CoreTiming& core_timing) override;

    // This is nn::hid::GyroscopeZeroDriftMode
    enum class GyroscopeZeroDriftMode : u32 {
        Loose = 0,
        Standard = 1,
        Tight = 2,
    };

    // This is nn::hid::NpadJoyHoldType
    enum class NpadJoyHoldType : u64 {
        Vertical = 0,
        Horizontal = 1,
    };

    // This is nn::hid::NpadJoyAssignmentMode
    enum class NpadJoyAssignmentMode : u32 {
        Dual = 0,
        Single = 1,
    };

    // This is nn::hid::NpadJoyDeviceType
    enum class NpadJoyDeviceType : s64 {
        Left = 0,
        Right = 1,
    };

    // This is nn::hid::NpadHandheldActivationMode
    enum class NpadHandheldActivationMode : u64 {
        Dual = 0,
        Single = 1,
        None = 2,
    };

    // This is nn::hid::NpadCommunicationMode
    enum class NpadCommunicationMode : u64 {
        Mode_5ms = 0,
        Mode_10ms = 1,
        Mode_15ms = 2,
        Default = 3,
    };

    void SetSupportedStyleSet(Core::HID::NpadStyleTag style_set);
    Core::HID::NpadStyleTag GetSupportedStyleSet() const;

    void SetSupportedNpadIdTypes(u8* data, std::size_t length);
    void GetSupportedNpadIdTypes(u32* data, std::size_t max_length);
    std::size_t GetSupportedNpadIdTypesSize() const;

    void SetHoldType(NpadJoyHoldType joy_hold_type);
    NpadJoyHoldType GetHoldType() const;

    void SetNpadHandheldActivationMode(NpadHandheldActivationMode activation_mode);
    NpadHandheldActivationMode GetNpadHandheldActivationMode() const;

    void SetNpadCommunicationMode(NpadCommunicationMode communication_mode_);
    NpadCommunicationMode GetNpadCommunicationMode() const;

    ResultCode SetNpadMode(Core::HID::NpadIdType npad_id, NpadJoyDeviceType npad_device_type,
                           NpadJoyAssignmentMode assignment_mode);

    bool VibrateControllerAtIndex(Core::HID::NpadIdType npad_id, std::size_t device_index,
                                  const Core::HID::VibrationValue& vibration_value);

    void VibrateController(const Core::HID::VibrationDeviceHandle& vibration_device_handle,
                           const Core::HID::VibrationValue& vibration_value);

    void VibrateControllers(
        const std::vector<Core::HID::VibrationDeviceHandle>& vibration_device_handles,
        const std::vector<Core::HID::VibrationValue>& vibration_values);

    Core::HID::VibrationValue GetLastVibration(
        const Core::HID::VibrationDeviceHandle& vibration_device_handle) const;

    void InitializeVibrationDevice(const Core::HID::VibrationDeviceHandle& vibration_device_handle);

    void InitializeVibrationDeviceAtIndex(Core::HID::NpadIdType npad_id, std::size_t device_index);

    void SetPermitVibrationSession(bool permit_vibration_session);

    bool IsVibrationDeviceMounted(
        const Core::HID::VibrationDeviceHandle& vibration_device_handle) const;

    Kernel::KReadableEvent& GetStyleSetChangedEvent(Core::HID::NpadIdType npad_id);
    void SignalStyleSetChangedEvent(Core::HID::NpadIdType npad_id) const;

    // Adds a new controller at an index.
    void AddNewControllerAt(Core::HID::NpadStyleIndex controller, Core::HID::NpadIdType npad_id);
    // Adds a new controller at an index with connection status.
    void UpdateControllerAt(Core::HID::NpadStyleIndex controller, Core::HID::NpadIdType npad_id,
                            bool connected);

    ResultCode DisconnectNpad(Core::HID::NpadIdType npad_id);

    ResultCode SetGyroscopeZeroDriftMode(const Core::HID::SixAxisSensorHandle& sixaxis_handle,
                                         GyroscopeZeroDriftMode drift_mode);
    ResultCode GetGyroscopeZeroDriftMode(const Core::HID::SixAxisSensorHandle& sixaxis_handle,
                                         GyroscopeZeroDriftMode& drift_mode) const;
    ResultCode IsSixAxisSensorAtRest(const Core::HID::SixAxisSensorHandle& sixaxis_handle,
                                     bool& is_at_rest) const;
    ResultCode IsFirmwareUpdateAvailableForSixAxisSensor(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle, bool& is_firmware_available) const;
    ResultCode EnableSixAxisSensorUnalteredPassthrough(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle, bool is_enabled);
    ResultCode IsSixAxisSensorUnalteredPassthroughEnabled(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle, bool& is_enabled) const;
    ResultCode LoadSixAxisSensorCalibrationParameter(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle,
        Core::HID::SixAxisSensorCalibrationParameter& calibration) const;
    ResultCode GetSixAxisSensorIcInformation(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle,
        Core::HID::SixAxisSensorIcInformation& ic_information) const;
    ResultCode ResetIsSixAxisSensorDeviceNewlyAssigned(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle);
    ResultCode SetSixAxisEnabled(const Core::HID::SixAxisSensorHandle& sixaxis_handle,
                                 bool sixaxis_status);
    ResultCode IsSixAxisSensorFusionEnabled(const Core::HID::SixAxisSensorHandle& sixaxis_handle,
                                            bool& is_fusion_enabled) const;
    ResultCode SetSixAxisFusionEnabled(const Core::HID::SixAxisSensorHandle& sixaxis_handle,
                                       bool is_fusion_enabled);
    ResultCode SetSixAxisFusionParameters(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle,
        Core::HID::SixAxisSensorFusionParameters sixaxis_fusion_parameters);
    ResultCode GetSixAxisFusionParameters(
        const Core::HID::SixAxisSensorHandle& sixaxis_handle,
        Core::HID::SixAxisSensorFusionParameters& parameters) const;
    ResultCode GetLedPattern(Core::HID::NpadIdType npad_id, Core::HID::LedPattern& pattern) const;
    ResultCode IsUnintendedHomeButtonInputProtectionEnabled(Core::HID::NpadIdType npad_id,
                                                            bool& is_enabled) const;
    ResultCode SetUnintendedHomeButtonInputProtectionEnabled(bool is_protection_enabled,
                                                             Core::HID::NpadIdType npad_id);
    void SetAnalogStickUseCenterClamp(bool use_center_clamp);
    void ClearAllConnectedControllers();
    void DisconnectAllConnectedControllers();
    void ConnectAllDisconnectedControllers();
    void ClearAllControllers();

    ResultCode MergeSingleJoyAsDualJoy(Core::HID::NpadIdType npad_id_1,
                                       Core::HID::NpadIdType npad_id_2);
    void StartLRAssignmentMode();
    void StopLRAssignmentMode();
    ResultCode SwapNpadAssignment(Core::HID::NpadIdType npad_id_1, Core::HID::NpadIdType npad_id_2);

    // Logical OR for all buttons presses on all controllers
    // Specifically for cheat engine and other features.
    Core::HID::NpadButton GetAndResetPressState();

    static bool IsNpadIdValid(Core::HID::NpadIdType npad_id);
    static ResultCode IsDeviceHandleValid(const Core::HID::VibrationDeviceHandle& device_handle);
    static ResultCode VerifyValidSixAxisSensorHandle(
        const Core::HID::SixAxisSensorHandle& device_handle);

private:
    static constexpr std::size_t NPAD_COUNT = 10;

    // This is nn::hid::detail::ColorAttribute
    enum class ColorAttribute : u32 {
        Ok = 0,
        ReadError = 1,
        NoController = 2,
    };
    static_assert(sizeof(ColorAttribute) == 4, "ColorAttribute is an invalid size");

    // This is nn::hid::detail::NpadFullKeyColorState
    struct NpadFullKeyColorState {
        ColorAttribute attribute{ColorAttribute::NoController};
        Core::HID::NpadControllerColor fullkey{};
    };
    static_assert(sizeof(NpadFullKeyColorState) == 0xC, "NpadFullKeyColorState is an invalid size");

    // This is nn::hid::detail::NpadJoyColorState
    struct NpadJoyColorState {
        ColorAttribute attribute{ColorAttribute::NoController};
        Core::HID::NpadControllerColor left{};
        Core::HID::NpadControllerColor right{};
    };
    static_assert(sizeof(NpadJoyColorState) == 0x14, "NpadJoyColorState is an invalid size");

    // This is nn::hid::NpadAttribute
    struct NpadAttribute {
        union {
            u32 raw{};
            BitField<0, 1, u32> is_connected;
            BitField<1, 1, u32> is_wired;
            BitField<2, 1, u32> is_left_connected;
            BitField<3, 1, u32> is_left_wired;
            BitField<4, 1, u32> is_right_connected;
            BitField<5, 1, u32> is_right_wired;
        };
    };
    static_assert(sizeof(NpadAttribute) == 4, "NpadAttribute is an invalid size");

    // This is nn::hid::NpadFullKeyState
    // This is nn::hid::NpadHandheldState
    // This is nn::hid::NpadJoyDualState
    // This is nn::hid::NpadJoyLeftState
    // This is nn::hid::NpadJoyRightState
    // This is nn::hid::NpadPalmaState
    // This is nn::hid::NpadSystemExtState
    struct NPadGenericState {
        s64_le sampling_number{};
        Core::HID::NpadButtonState npad_buttons{};
        Core::HID::AnalogStickState l_stick{};
        Core::HID::AnalogStickState r_stick{};
        NpadAttribute connection_status{};
        INSERT_PADDING_BYTES(4); // Reserved
    };
    static_assert(sizeof(NPadGenericState) == 0x28, "NPadGenericState is an invalid size");

    // This is nn::hid::SixAxisSensorAttribute
    struct SixAxisSensorAttribute {
        union {
            u32 raw{};
            BitField<0, 1, u32> is_connected;
            BitField<1, 1, u32> is_interpolated;
        };
    };
    static_assert(sizeof(SixAxisSensorAttribute) == 4, "SixAxisSensorAttribute is an invalid size");

    // This is nn::hid::SixAxisSensorState
    struct SixAxisSensorState {
        s64 delta_time{};
        s64 sampling_number{};
        Common::Vec3f accel{};
        Common::Vec3f gyro{};
        Common::Vec3f rotation{};
        std::array<Common::Vec3f, 3> orientation{};
        SixAxisSensorAttribute attribute{};
        INSERT_PADDING_BYTES(4); // Reserved
    };
    static_assert(sizeof(SixAxisSensorState) == 0x60, "SixAxisSensorState is an invalid size");

    // This is nn::hid::server::NpadGcTriggerState
    struct NpadGcTriggerState {
        s64 sampling_number{};
        s32 l_analog{};
        s32 r_analog{};
    };
    static_assert(sizeof(NpadGcTriggerState) == 0x10, "NpadGcTriggerState is an invalid size");

    // This is nn::hid::NpadSystemProperties
    struct NPadSystemProperties {
        union {
            s64 raw{};
            BitField<0, 1, s64> is_charging_joy_dual;
            BitField<1, 1, s64> is_charging_joy_left;
            BitField<2, 1, s64> is_charging_joy_right;
            BitField<3, 1, s64> is_powered_joy_dual;
            BitField<4, 1, s64> is_powered_joy_left;
            BitField<5, 1, s64> is_powered_joy_right;
            BitField<9, 1, s64> is_system_unsupported_button;
            BitField<10, 1, s64> is_system_ext_unsupported_button;
            BitField<11, 1, s64> is_vertical;
            BitField<12, 1, s64> is_horizontal;
            BitField<13, 1, s64> use_plus;
            BitField<14, 1, s64> use_minus;
            BitField<15, 1, s64> use_directional_buttons;
        };
    };
    static_assert(sizeof(NPadSystemProperties) == 0x8, "NPadSystemProperties is an invalid size");

    // This is nn::hid::NpadSystemButtonProperties
    struct NpadSystemButtonProperties {
        union {
            s32 raw{};
            BitField<0, 1, s32> is_home_button_protection_enabled;
        };
    };
    static_assert(sizeof(NpadSystemButtonProperties) == 0x4,
                  "NPadButtonProperties is an invalid size");

    // This is nn::hid::system::DeviceType
    struct DeviceType {
        union {
            u32 raw{};
            BitField<0, 1, s32> fullkey;
            BitField<1, 1, s32> debug_pad;
            BitField<2, 1, s32> handheld_left;
            BitField<3, 1, s32> handheld_right;
            BitField<4, 1, s32> joycon_left;
            BitField<5, 1, s32> joycon_right;
            BitField<6, 1, s32> palma;
            BitField<7, 1, s32> lark_hvc_left;
            BitField<8, 1, s32> lark_hvc_right;
            BitField<9, 1, s32> lark_nes_left;
            BitField<10, 1, s32> lark_nes_right;
            BitField<11, 1, s32> handheld_lark_hvc_left;
            BitField<12, 1, s32> handheld_lark_hvc_right;
            BitField<13, 1, s32> handheld_lark_nes_left;
            BitField<14, 1, s32> handheld_lark_nes_right;
            BitField<15, 1, s32> lucia;
            BitField<16, 1, s32> lagon;
            BitField<17, 1, s32> lager;
            BitField<31, 1, s32> system;
        };
    };

    // This is nn::hid::detail::NfcXcdDeviceHandleStateImpl
    struct NfcXcdDeviceHandleStateImpl {
        u64 handle{};
        bool is_available{};
        bool is_activated{};
        INSERT_PADDING_BYTES(0x6); // Reserved
        u64 sampling_number{};
    };
    static_assert(sizeof(NfcXcdDeviceHandleStateImpl) == 0x18,
                  "NfcXcdDeviceHandleStateImpl is an invalid size");

    // This is nn::hid::system::AppletFooterUiAttributesSet
    struct AppletFooterUiAttributes {
        INSERT_PADDING_BYTES(0x4);
    };

    // This is nn::hid::system::AppletFooterUiType
    enum class AppletFooterUiType : u8 {
        None = 0,
        HandheldNone = 1,
        HandheldJoyConLeftOnly = 1,
        HandheldJoyConRightOnly = 3,
        HandheldJoyConLeftJoyConRight = 4,
        JoyDual = 5,
        JoyDualLeftOnly = 6,
        JoyDualRightOnly = 7,
        JoyLeftHorizontal = 8,
        JoyLeftVertical = 9,
        JoyRightHorizontal = 10,
        JoyRightVertical = 11,
        SwitchProController = 12,
        CompatibleProController = 13,
        CompatibleJoyCon = 14,
        LarkHvc1 = 15,
        LarkHvc2 = 16,
        LarkNesLeft = 17,
        LarkNesRight = 18,
        Lucia = 19,
        Verification = 20,
        Lagon = 21,
    };

    struct AppletFooterUi {
        AppletFooterUiAttributes attributes{};
        AppletFooterUiType type{AppletFooterUiType::None};
        INSERT_PADDING_BYTES(0x5B); // Reserved
    };
    static_assert(sizeof(AppletFooterUi) == 0x60, "AppletFooterUi is an invalid size");

    // This is nn::hid::NpadLarkType
    enum class NpadLarkType : u32 {
        Invalid,
        H1,
        H2,
        NL,
        NR,
    };

    // This is nn::hid::NpadLuciaType
    enum class NpadLuciaType : u32 {
        Invalid,
        J,
        E,
        U,
    };

    // This is nn::hid::NpadLagonType
    enum class NpadLagonType : u32 {
        Invalid,
    };

    // This is nn::hid::NpadLagerType
    enum class NpadLagerType : u32 {
        Invalid,
        J,
        E,
        U,
    };

    struct AppletNfcXcd {
        union {
            AppletFooterUi applet_footer{};
            Lifo<NfcXcdDeviceHandleStateImpl, 0x2> nfc_xcd_device_lifo;
        };
    };

    // This is nn::hid::detail::NpadInternalState
    struct NpadInternalState {
        Core::HID::NpadStyleTag style_tag{Core::HID::NpadStyleSet::None};
        NpadJoyAssignmentMode assignment_mode{NpadJoyAssignmentMode::Dual};
        NpadFullKeyColorState fullkey_color{};
        NpadJoyColorState joycon_color{};
        Lifo<NPadGenericState, hid_entry_count> fullkey_lifo{};
        Lifo<NPadGenericState, hid_entry_count> handheld_lifo{};
        Lifo<NPadGenericState, hid_entry_count> joy_dual_lifo{};
        Lifo<NPadGenericState, hid_entry_count> joy_left_lifo{};
        Lifo<NPadGenericState, hid_entry_count> joy_right_lifo{};
        Lifo<NPadGenericState, hid_entry_count> palma_lifo{};
        Lifo<NPadGenericState, hid_entry_count> system_ext_lifo{};
        Lifo<SixAxisSensorState, hid_entry_count> sixaxis_fullkey_lifo{};
        Lifo<SixAxisSensorState, hid_entry_count> sixaxis_handheld_lifo{};
        Lifo<SixAxisSensorState, hid_entry_count> sixaxis_dual_left_lifo{};
        Lifo<SixAxisSensorState, hid_entry_count> sixaxis_dual_right_lifo{};
        Lifo<SixAxisSensorState, hid_entry_count> sixaxis_left_lifo{};
        Lifo<SixAxisSensorState, hid_entry_count> sixaxis_right_lifo{};
        DeviceType device_type{};
        INSERT_PADDING_BYTES(0x4); // Reserved
        NPadSystemProperties system_properties{};
        NpadSystemButtonProperties button_properties{};
        Core::HID::NpadBatteryLevel battery_level_dual{};
        Core::HID::NpadBatteryLevel battery_level_left{};
        Core::HID::NpadBatteryLevel battery_level_right{};
        AppletNfcXcd applet_nfc_xcd{};
        INSERT_PADDING_BYTES(0x20); // Unknown
        Lifo<NpadGcTriggerState, hid_entry_count> gc_trigger_lifo{};
        NpadLarkType lark_type_l_and_main{};
        NpadLarkType lark_type_r{};
        NpadLuciaType lucia_type{};
        NpadLagonType lagon_type{};
        NpadLagerType lager_type{};
        Core::HID::SixAxisSensorProperties sixaxis_fullkey_properties;
        Core::HID::SixAxisSensorProperties sixaxis_handheld_properties;
        Core::HID::SixAxisSensorProperties sixaxis_dual_left_properties;
        Core::HID::SixAxisSensorProperties sixaxis_dual_right_properties;
        Core::HID::SixAxisSensorProperties sixaxis_left_properties;
        Core::HID::SixAxisSensorProperties sixaxis_right_properties;
        INSERT_PADDING_BYTES(0xc06); // Unknown
    };
    static_assert(sizeof(NpadInternalState) == 0x5000, "NpadInternalState is an invalid size");

    struct VibrationData {
        bool device_mounted{};
        Core::HID::VibrationValue latest_vibration_value{};
        std::chrono::steady_clock::time_point last_vibration_timepoint{};
    };

    struct SixaxisParameters {
        bool is_fusion_enabled{true};
        bool unaltered_passtrough{false};
        Core::HID::SixAxisSensorFusionParameters fusion{};
        Core::HID::SixAxisSensorCalibrationParameter calibration{};
        Core::HID::SixAxisSensorIcInformation ic_information{};
        GyroscopeZeroDriftMode gyroscope_zero_drift_mode{GyroscopeZeroDriftMode::Standard};
    };

    struct NpadControllerData {
        Kernel::KEvent* styleset_changed_event{};
        NpadInternalState* shared_memory = nullptr;
        Core::HID::EmulatedController* device = nullptr;

        std::array<VibrationData, 2> vibration{};
        bool unintended_home_button_input_protection{};
        bool is_connected{};

        // Dual joycons can have only one side connected
        bool is_dual_left_connected{true};
        bool is_dual_right_connected{true};

        // Motion parameters
        bool sixaxis_at_rest{true};
        bool sixaxis_sensor_enabled{true};
        SixaxisParameters sixaxis_fullkey{};
        SixaxisParameters sixaxis_handheld{};
        SixaxisParameters sixaxis_dual_left{};
        SixaxisParameters sixaxis_dual_right{};
        SixaxisParameters sixaxis_left{};
        SixaxisParameters sixaxis_right{};
        SixaxisParameters sixaxis_unknown{};

        // Current pad state
        NPadGenericState npad_pad_state{};
        NPadGenericState npad_libnx_state{};
        NpadGcTriggerState npad_trigger_state{};
        SixAxisSensorState sixaxis_fullkey_state{};
        SixAxisSensorState sixaxis_handheld_state{};
        SixAxisSensorState sixaxis_dual_left_state{};
        SixAxisSensorState sixaxis_dual_right_state{};
        SixAxisSensorState sixaxis_left_lifo_state{};
        SixAxisSensorState sixaxis_right_lifo_state{};
        int callback_key{};
    };

    void ControllerUpdate(Core::HID::ControllerTriggerType type, std::size_t controller_idx);
    void InitNewlyAddedController(Core::HID::NpadIdType npad_id);
    bool IsControllerSupported(Core::HID::NpadStyleIndex controller) const;
    void RequestPadStateUpdate(Core::HID::NpadIdType npad_id);
    void WriteEmptyEntry(NpadInternalState* npad);

    NpadControllerData& GetControllerFromHandle(
        const Core::HID::SixAxisSensorHandle& device_handle);
    const NpadControllerData& GetControllerFromHandle(
        const Core::HID::SixAxisSensorHandle& device_handle) const;
    NpadControllerData& GetControllerFromHandle(
        const Core::HID::VibrationDeviceHandle& device_handle);
    const NpadControllerData& GetControllerFromHandle(
        const Core::HID::VibrationDeviceHandle& device_handle) const;
    NpadControllerData& GetControllerFromNpadIdType(Core::HID::NpadIdType npad_id);
    const NpadControllerData& GetControllerFromNpadIdType(Core::HID::NpadIdType npad_id) const;

    Core::HID::SixAxisSensorProperties& GetSixaxisProperties(
        const Core::HID::SixAxisSensorHandle& device_handle);
    const Core::HID::SixAxisSensorProperties& GetSixaxisProperties(
        const Core::HID::SixAxisSensorHandle& device_handle) const;
    SixaxisParameters& GetSixaxisState(const Core::HID::SixAxisSensorHandle& device_handle);
    const SixaxisParameters& GetSixaxisState(
        const Core::HID::SixAxisSensorHandle& device_handle) const;

    std::atomic<u64> press_state{};

    std::array<NpadControllerData, NPAD_COUNT> controller_data{};
    KernelHelpers::ServiceContext& service_context;
    std::mutex mutex;
    std::vector<Core::HID::NpadIdType> supported_npad_id_types{};
    NpadJoyHoldType hold_type{NpadJoyHoldType::Vertical};
    NpadHandheldActivationMode handheld_activation_mode{NpadHandheldActivationMode::Dual};
    NpadCommunicationMode communication_mode{NpadCommunicationMode::Default};
    bool permit_vibration_session_enabled{false};
    bool analog_stick_use_center_clamp{false};
    bool is_in_lr_assignment_mode{false};
    bool is_controller_initialized{false};
};
} // namespace Service::HID
