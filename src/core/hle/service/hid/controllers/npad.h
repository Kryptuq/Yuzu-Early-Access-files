// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <mutex>

#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/quaternion.h"
#include "common/settings.h"
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
}

namespace Service::HID {

constexpr u32 NPAD_HANDHELD = 32;
constexpr u32 NPAD_UNKNOWN = 16; // TODO(ogniK): What is this?

class Controller_NPad final : public ControllerBase {
public:
    explicit Controller_NPad(Core::System& system_,
                             KernelHelpers::ServiceContext& service_context_);
    ~Controller_NPad() override;

    // Called when the controller is initialized
    void OnInit() override;

    // When the controller is released
    void OnRelease() override;

    // When the controller is requesting an update for the shared memory
    void OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data, std::size_t size) override;

    // When the controller is requesting a motion update for the shared memory
    void OnMotionUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                        std::size_t size) override;

    enum class DeviceIndex : u8 {
        Left = 0,
        Right = 1,
        None = 2,
        MaxDeviceIndex = 3,
    };

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

    struct DeviceHandle {
        Core::HID::NpadStyleIndex npad_type;
        u8 npad_id;
        DeviceIndex device_index;
        INSERT_PADDING_BYTES_NOINIT(1);
    };
    static_assert(sizeof(DeviceHandle) == 4, "DeviceHandle is an invalid size");

    // This is nn::hid::VibrationValue
    struct VibrationValue {
        f32 amp_low;
        f32 freq_low;
        f32 amp_high;
        f32 freq_high;
    };
    static_assert(sizeof(VibrationValue) == 0x10, "Vibration is an invalid size");

    static constexpr VibrationValue DEFAULT_VIBRATION_VALUE{
        .amp_low = 0.0f,
        .freq_low = 160.0f,
        .amp_high = 0.0f,
        .freq_high = 320.0f,
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

    void SetNpadMode(u32 npad_id, NpadJoyAssignmentMode assignment_mode);

    bool VibrateControllerAtIndex(std::size_t npad_index, std::size_t device_index,
                                  const VibrationValue& vibration_value);

    void VibrateController(const DeviceHandle& vibration_device_handle,
                           const VibrationValue& vibration_value);

    void VibrateControllers(const std::vector<DeviceHandle>& vibration_device_handles,
                            const std::vector<VibrationValue>& vibration_values);

    VibrationValue GetLastVibration(const DeviceHandle& vibration_device_handle) const;

    void InitializeVibrationDevice(const DeviceHandle& vibration_device_handle);

    void InitializeVibrationDeviceAtIndex(std::size_t npad_index, std::size_t device_index);

    void SetPermitVibrationSession(bool permit_vibration_session);

    bool IsVibrationDeviceMounted(const DeviceHandle& vibration_device_handle) const;

    Kernel::KReadableEvent& GetStyleSetChangedEvent(u32 npad_id);
    void SignalStyleSetChangedEvent(u32 npad_id) const;

    // Adds a new controller at an index.
    void AddNewControllerAt(Core::HID::NpadStyleIndex controller, std::size_t npad_index);
    // Adds a new controller at an index with connection status.
    void UpdateControllerAt(Core::HID::NpadStyleIndex controller, std::size_t npad_index,
                            bool connected);

    void DisconnectNpad(u32 npad_id);
    void DisconnectNpadAtIndex(std::size_t index);

    void SetGyroscopeZeroDriftMode(GyroscopeZeroDriftMode drift_mode);
    GyroscopeZeroDriftMode GetGyroscopeZeroDriftMode() const;
    bool IsSixAxisSensorAtRest() const;
    void SetSixAxisEnabled(bool six_axis_status);
    void SetSixAxisFusionParameters(f32 parameter1, f32 parameter2);
    std::pair<f32, f32> GetSixAxisFusionParameters();
    void ResetSixAxisFusionParameters();
    Core::HID::LedPattern GetLedPattern(u32 npad_id);
    bool IsUnintendedHomeButtonInputProtectionEnabled(u32 npad_id) const;
    void SetUnintendedHomeButtonInputProtectionEnabled(bool is_protection_enabled, u32 npad_id);
    void SetAnalogStickUseCenterClamp(bool use_center_clamp);
    void ClearAllConnectedControllers();
    void DisconnectAllConnectedControllers();
    void ConnectAllDisconnectedControllers();
    void ClearAllControllers();

    void MergeSingleJoyAsDualJoy(u32 npad_id_1, u32 npad_id_2);
    void StartLRAssignmentMode();
    void StopLRAssignmentMode();
    bool SwapNpadAssignment(u32 npad_id_1, u32 npad_id_2);

    // Logical OR for all buttons presses on all controllers
    // Specifically for cheat engine and other features.
    u32 GetAndResetPressState();

    static std::size_t NPadIdToIndex(u32 npad_id);
    static u32 IndexToNPad(std::size_t index);
    static bool IsNpadIdValid(u32 npad_id);
    static bool IsDeviceHandleValid(const DeviceHandle& device_handle);

private:
    // This is nn::hid::detail::ColorAttribute
    enum class ColorAttribute : u32 {
        Ok = 0,
        ReadError = 1,
        NoController = 2,
    };
    static_assert(sizeof(ColorAttribute) == 4, "ColorAttribute is an invalid size");

    // This is nn::hid::detail::NpadFullKeyColorState
    struct NpadFullKeyColorState {
        ColorAttribute attribute;
        Core::HID::NpadControllerColor fullkey;
    };
    static_assert(sizeof(NpadFullKeyColorState) == 0xC, "NpadFullKeyColorState is an invalid size");

    // This is nn::hid::detail::NpadJoyColorState
    struct NpadJoyColorState {
        ColorAttribute attribute;
        Core::HID::NpadControllerColor left;
        Core::HID::NpadControllerColor right;
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
        s64_le sampling_number;
        Core::HID::NpadButtonState npad_buttons;
        Core::HID::AnalogStickState l_stick;
        Core::HID::AnalogStickState r_stick;
        NpadAttribute connection_status;
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
        SixAxisSensorAttribute attribute;
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
        u64 handle;
        bool is_available;
        bool is_activated;
        INSERT_PADDING_BYTES(0x6); // Reserved
        u64 sampling_number;
    };
    static_assert(sizeof(NfcXcdDeviceHandleStateImpl) == 0x18,
                  "NfcXcdDeviceHandleStateImpl is an invalid size");

    // nn::hid::detail::NfcXcdDeviceHandleStateImplAtomicStorage
    struct NfcXcdDeviceHandleStateImplAtomicStorage {
        u64 sampling_number;
        NfcXcdDeviceHandleStateImpl nfc_xcd_device_handle_state;
    };
    static_assert(sizeof(NfcXcdDeviceHandleStateImplAtomicStorage) == 0x20,
                  "NfcXcdDeviceHandleStateImplAtomicStorage is an invalid size");

    // This is nn::hid::detail::NfcXcdDeviceHandleState
    struct NfcXcdDeviceHandleState {
        // TODO(german77): Make this struct a ring lifo object
        INSERT_PADDING_BYTES(0x8); // Unused
        s64 total_buffer_count = max_buffer_size;
        s64 buffer_tail{};
        s64 buffer_count{};
        std::array<NfcXcdDeviceHandleStateImplAtomicStorage, 2> nfc_xcd_device_handle_storage;
    };
    static_assert(sizeof(NfcXcdDeviceHandleState) == 0x60,
                  "NfcXcdDeviceHandleState is an invalid size");

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
        AppletFooterUiAttributes attributes;
        AppletFooterUiType type;
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

    // This is nn::hid::detail::NpadInternalState
    struct NpadInternalState {
        Core::HID::NpadStyleTag style_set;
        NpadJoyAssignmentMode assignment_mode;
        NpadFullKeyColorState fullkey_color;
        NpadJoyColorState joycon_color;
        Lifo<NPadGenericState> fullkey_lifo;
        Lifo<NPadGenericState> handheld_lifo;
        Lifo<NPadGenericState> joy_dual_lifo;
        Lifo<NPadGenericState> joy_left_lifo;
        Lifo<NPadGenericState> joy_right_lifo;
        Lifo<NPadGenericState> palma_lifo;
        Lifo<NPadGenericState> system_ext_lifo;
        Lifo<SixAxisSensorState> sixaxis_fullkey_lifo;
        Lifo<SixAxisSensorState> sixaxis_handheld_lifo;
        Lifo<SixAxisSensorState> sixaxis_dual_left_lifo;
        Lifo<SixAxisSensorState> sixaxis_dual_right_lifo;
        Lifo<SixAxisSensorState> sixaxis_left_lifo;
        Lifo<SixAxisSensorState> sixaxis_right_lifo;
        DeviceType device_type;
        INSERT_PADDING_BYTES(0x4); // Reserved
        NPadSystemProperties system_properties;
        NpadSystemButtonProperties button_properties;
        Core::HID::BatteryLevel battery_level_dual;
        Core::HID::BatteryLevel battery_level_left;
        Core::HID::BatteryLevel battery_level_right;
        union {
            NfcXcdDeviceHandleState nfc_xcd_device_handle;
            AppletFooterUi applet_footer;
        };
        INSERT_PADDING_BYTES(0x20); // Unknown
        Lifo<NpadGcTriggerState> gc_trigger_lifo;
        NpadLarkType lark_type_l_and_main;
        NpadLarkType lark_type_r;
        NpadLuciaType lucia_type;
        NpadLagonType lagon_type;
        NpadLagerType lager_type;
        INSERT_PADDING_BYTES(
            0x4); // FW 13.x Investigate there is some sort of bitflag related to joycons
        INSERT_PADDING_BYTES(0xc08); // Unknown
    };
    static_assert(sizeof(NpadInternalState) == 0x5000, "NpadInternalState is an invalid size");

    struct VibrationData {
        bool device_mounted{};
        VibrationValue latest_vibration_value{};
        std::chrono::steady_clock::time_point last_vibration_timepoint{};
    };

    struct ControllerData {
        Core::HID::EmulatedController* device;
        Kernel::KEvent* styleset_changed_event{};
        NpadInternalState shared_memory_entry{};

        std::array<VibrationData, 2> vibration{};
        bool unintended_home_button_input_protection{};
        bool is_connected{};
        Core::HID::NpadStyleIndex npad_type{Core::HID::NpadStyleIndex::None};

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
        int callback_key;
    };

    void ControllerUpdate(Core::HID::ControllerTriggerType type, std::size_t controller_idx);
    void InitNewlyAddedController(std::size_t controller_idx);
    bool IsControllerSupported(Core::HID::NpadStyleIndex controller) const;
    void RequestPadStateUpdate(u32 npad_id);
    void WriteEmptyEntry(NpadInternalState& npad);

    std::atomic<u32> press_state{};

    std::array<ControllerData, 10> controller_data{};
    KernelHelpers::ServiceContext& service_context;
    std::mutex mutex;
    std::vector<u32> supported_npad_id_types{};
    NpadJoyHoldType hold_type{NpadJoyHoldType::Vertical};
    NpadHandheldActivationMode handheld_activation_mode{NpadHandheldActivationMode::Dual};
    NpadCommunicationMode communication_mode{NpadCommunicationMode::Default};
    bool permit_vibration_session_enabled{false};
    bool analog_stick_use_center_clamp{};
    GyroscopeZeroDriftMode gyroscope_zero_drift_mode{GyroscopeZeroDriftMode::Standard};
    bool sixaxis_sensors_enabled{true};
    f32 sixaxis_fusion_parameter1{};
    f32 sixaxis_fusion_parameter2{};
    bool sixaxis_at_rest{true};
    bool is_in_lr_assignment_mode{false};
};
} // namespace Service::HID
