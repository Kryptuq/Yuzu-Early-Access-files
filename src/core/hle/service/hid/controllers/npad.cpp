// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstring>
#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/service/hid/controllers/npad.h"
#include "core/hle/service/kernel_helpers.h"

namespace Service::HID {
constexpr std::size_t NPAD_OFFSET = 0x9A00;
constexpr std::array<Core::HID::NpadIdType, 10> npad_id_list{
    Core::HID::NpadIdType::Player1,  Core::HID::NpadIdType::Player2, Core::HID::NpadIdType::Player3,
    Core::HID::NpadIdType::Player4,  Core::HID::NpadIdType::Player5, Core::HID::NpadIdType::Player6,
    Core::HID::NpadIdType::Player7,  Core::HID::NpadIdType::Player8, Core::HID::NpadIdType::Other,
    Core::HID::NpadIdType::Handheld,
};

bool Controller_NPad::IsNpadIdValid(Core::HID::NpadIdType npad_id) {
    switch (npad_id) {
    case Core::HID::NpadIdType::Player1:
    case Core::HID::NpadIdType::Player2:
    case Core::HID::NpadIdType::Player3:
    case Core::HID::NpadIdType::Player4:
    case Core::HID::NpadIdType::Player5:
    case Core::HID::NpadIdType::Player6:
    case Core::HID::NpadIdType::Player7:
    case Core::HID::NpadIdType::Player8:
    case Core::HID::NpadIdType::Other:
    case Core::HID::NpadIdType::Handheld:
        return true;
    default:
        LOG_ERROR(Service_HID, "Invalid npad id {}", npad_id);
        return false;
    }
}

bool Controller_NPad::IsDeviceHandleValid(const Core::HID::VibrationDeviceHandle& device_handle) {
    return IsNpadIdValid(static_cast<Core::HID::NpadIdType>(device_handle.npad_id)) &&
           device_handle.npad_type < Core::HID::NpadStyleIndex::MaxNpadType &&
           device_handle.device_index < Core::HID::DeviceIndex::MaxDeviceIndex;
}

bool Controller_NPad::IsDeviceHandleValid(const Core::HID::SixAxisSensorHandle& device_handle) {
    return IsNpadIdValid(static_cast<Core::HID::NpadIdType>(device_handle.npad_id)) &&
           device_handle.npad_type < Core::HID::NpadStyleIndex::MaxNpadType &&
           device_handle.device_index < Core::HID::DeviceIndex::MaxDeviceIndex;
}

Controller_NPad::Controller_NPad(Core::HID::HIDCore& hid_core_,
                                 KernelHelpers::ServiceContext& service_context_)
    : ControllerBase{hid_core_}, service_context{service_context_} {
    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];
        controller.device = hid_core.GetEmulatedControllerByIndex(i);
        controller.vibration[Core::HID::EmulatedDeviceIndex::LeftIndex].latest_vibration_value =
            DEFAULT_VIBRATION_VALUE;
        controller.vibration[Core::HID::EmulatedDeviceIndex::RightIndex].latest_vibration_value =
            DEFAULT_VIBRATION_VALUE;
        Core::HID::ControllerUpdateCallback engine_callback{
            .on_change = [this,
                          i](Core::HID::ControllerTriggerType type) { ControllerUpdate(type, i); },
            .is_npad_service = true,
        };
        controller.callback_key = controller.device->SetCallback(engine_callback);
    }
}

Controller_NPad::~Controller_NPad() {
    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];
        controller.device->DeleteCallback(controller.callback_key);
    }
    OnRelease();
}

void Controller_NPad::ControllerUpdate(Core::HID::ControllerTriggerType type,
                                       std::size_t controller_idx) {
    if (type == Core::HID::ControllerTriggerType::All) {
        ControllerUpdate(Core::HID::ControllerTriggerType::Connected, controller_idx);
        ControllerUpdate(Core::HID::ControllerTriggerType::Battery, controller_idx);
        return;
    }
    if (controller_idx >= controller_data.size()) {
        return;
    }

    auto& controller = controller_data[controller_idx];
    const auto is_connected = controller.device->IsConnected();
    const auto npad_type = controller.device->GetNpadStyleIndex();
    const auto npad_id = controller.device->GetNpadIdType();
    switch (type) {
    case Core::HID::ControllerTriggerType::Connected:
    case Core::HID::ControllerTriggerType::Disconnected:
        if (is_connected == controller.is_connected) {
            return;
        }
        UpdateControllerAt(npad_type, npad_id, is_connected);
        break;
    case Core::HID::ControllerTriggerType::Battery: {
        if (!controller.is_connected) {
            return;
        }
        auto& shared_memory = controller.shared_memory_entry;
        const auto& battery_level = controller.device->GetBattery();
        shared_memory.battery_level_dual = battery_level.dual.battery_level;
        shared_memory.battery_level_left = battery_level.left.battery_level;
        shared_memory.battery_level_right = battery_level.right.battery_level;
        break;
    }
    default:
        break;
    }
}

void Controller_NPad::InitNewlyAddedController(Core::HID::NpadIdType npad_id) {
    LOG_DEBUG(Service_HID, "Npad connected {}", npad_id);
    auto& controller = GetControllerFromNpadIdType(npad_id);
    const auto controller_type = controller.device->GetNpadStyleIndex();
    auto& shared_memory = controller.shared_memory_entry;
    if (controller_type == Core::HID::NpadStyleIndex::None) {
        controller.styleset_changed_event->GetWritableEvent().Signal();
        return;
    }
    shared_memory.style_tag.raw = Core::HID::NpadStyleSet::None;
    shared_memory.device_type.raw = 0;
    shared_memory.system_properties.raw = 0;
    switch (controller_type) {
    case Core::HID::NpadStyleIndex::None:
        UNREACHABLE();
        break;
    case Core::HID::NpadStyleIndex::ProController:
        shared_memory.style_tag.fullkey.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        shared_memory.applet_footer.type = AppletFooterUiType::SwitchProController;
        break;
    case Core::HID::NpadStyleIndex::Handheld:
        shared_memory.style_tag.handheld.Assign(1);
        shared_memory.device_type.handheld_left.Assign(1);
        shared_memory.device_type.handheld_right.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.system_properties.use_directional_buttons.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Dual;
        shared_memory.applet_footer.type = AppletFooterUiType::HandheldJoyConLeftJoyConRight;
        break;
    case Core::HID::NpadStyleIndex::JoyconDual:
        shared_memory.style_tag.joycon_dual.Assign(1);
        shared_memory.device_type.joycon_left.Assign(1);
        shared_memory.device_type.joycon_right.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.system_properties.use_directional_buttons.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Dual;
        shared_memory.applet_footer.type = AppletFooterUiType::JoyDual;
        break;
    case Core::HID::NpadStyleIndex::JoyconLeft:
        shared_memory.style_tag.joycon_left.Assign(1);
        shared_memory.device_type.joycon_left.Assign(1);
        shared_memory.system_properties.is_horizontal.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        shared_memory.applet_footer.type = AppletFooterUiType::JoyLeftHorizontal;
        break;
    case Core::HID::NpadStyleIndex::JoyconRight:
        shared_memory.style_tag.joycon_right.Assign(1);
        shared_memory.device_type.joycon_right.Assign(1);
        shared_memory.system_properties.is_horizontal.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        shared_memory.applet_footer.type = AppletFooterUiType::JoyRightHorizontal;
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        shared_memory.style_tag.gamecube.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        break;
    case Core::HID::NpadStyleIndex::Pokeball:
        shared_memory.style_tag.palma.Assign(1);
        shared_memory.device_type.palma.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        break;
    case Core::HID::NpadStyleIndex::NES:
        shared_memory.style_tag.lark.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        break;
    case Core::HID::NpadStyleIndex::SNES:
        shared_memory.style_tag.lucia.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.applet_footer.type = AppletFooterUiType::Lucia;
        break;
    case Core::HID::NpadStyleIndex::N64:
        shared_memory.style_tag.lagoon.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.applet_footer.type = AppletFooterUiType::Lagon;
        break;
    case Core::HID::NpadStyleIndex::SegaGenesis:
        shared_memory.style_tag.lager.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        break;
    default:
        break;
    }

    const auto& body_colors = controller.device->GetColors();

    shared_memory.fullkey_color.attribute = ColorAttribute::Ok;
    shared_memory.fullkey_color.fullkey = body_colors.fullkey;

    shared_memory.joycon_color.attribute = ColorAttribute::Ok;
    shared_memory.joycon_color.left = body_colors.left;
    shared_memory.joycon_color.right = body_colors.right;

    // TODO: Investigate when we should report all batery types
    const auto& battery_level = controller.device->GetBattery();
    shared_memory.battery_level_dual = battery_level.dual.battery_level;
    shared_memory.battery_level_left = battery_level.left.battery_level;
    shared_memory.battery_level_right = battery_level.right.battery_level;

    controller.is_connected = true;
    controller.device->Connect();
    SignalStyleSetChangedEvent(npad_id);
    WriteEmptyEntry(controller.shared_memory_entry);
}

void Controller_NPad::OnInit() {
    if (!IsControllerActivated()) {
        return;
    }

    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];
        controller.styleset_changed_event =
            service_context.CreateEvent(fmt::format("npad:NpadStyleSetChanged_{}", i));
    }

    if (hid_core.GetSupportedStyleTag().raw == Core::HID::NpadStyleSet::None) {
        // We want to support all controllers
        Core::HID::NpadStyleTag style{};
        style.handheld.Assign(1);
        style.joycon_left.Assign(1);
        style.joycon_right.Assign(1);
        style.joycon_dual.Assign(1);
        style.fullkey.Assign(1);
        style.gamecube.Assign(1);
        style.palma.Assign(1);
        style.lark.Assign(1);
        style.lucia.Assign(1);
        style.lagoon.Assign(1);
        style.lager.Assign(1);
        hid_core.SetSupportedStyleTag(style);
    }

    supported_npad_id_types.resize(npad_id_list.size());
    std::memcpy(supported_npad_id_types.data(), npad_id_list.data(),
                npad_id_list.size() * sizeof(Core::HID::NpadIdType));

    // Prefill controller buffers
    for (auto& controller : controller_data) {
        auto& npad = controller.shared_memory_entry;
        npad.fullkey_color = {
            .attribute = ColorAttribute::NoController,
            .fullkey = {},
        };
        npad.joycon_color = {
            .attribute = ColorAttribute::NoController,
            .left = {},
            .right = {},
        };
        // HW seems to initialize the first 19 entries
        for (std::size_t i = 0; i < 19; ++i) {
            WriteEmptyEntry(npad);
        }
    }

    // Connect controllers
    for (auto& controller : controller_data) {
        const auto& device = controller.device;
        if (device->IsConnected()) {
            AddNewControllerAt(device->GetNpadStyleIndex(), device->GetNpadIdType());
        }
    }
}

void Controller_NPad::WriteEmptyEntry(NpadInternalState& npad) {
    NPadGenericState dummy_pad_state{};
    NpadGcTriggerState dummy_gc_state{};
    dummy_pad_state.sampling_number = npad.fullkey_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.fullkey_lifo.WriteNextEntry(dummy_pad_state);
    dummy_pad_state.sampling_number = npad.handheld_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.handheld_lifo.WriteNextEntry(dummy_pad_state);
    dummy_pad_state.sampling_number = npad.joy_dual_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.joy_dual_lifo.WriteNextEntry(dummy_pad_state);
    dummy_pad_state.sampling_number = npad.joy_left_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.joy_left_lifo.WriteNextEntry(dummy_pad_state);
    dummy_pad_state.sampling_number = npad.joy_right_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.joy_right_lifo.WriteNextEntry(dummy_pad_state);
    dummy_pad_state.sampling_number = npad.palma_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.palma_lifo.WriteNextEntry(dummy_pad_state);
    dummy_pad_state.sampling_number = npad.system_ext_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.system_ext_lifo.WriteNextEntry(dummy_pad_state);
    dummy_gc_state.sampling_number = npad.gc_trigger_lifo.ReadCurrentEntry().sampling_number + 1;
    npad.gc_trigger_lifo.WriteNextEntry(dummy_gc_state);
}

void Controller_NPad::OnRelease() {
    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];
        service_context.CloseEvent(controller.styleset_changed_event);
        for (std::size_t device_idx = 0; device_idx < controller.vibration.size(); ++device_idx) {
            VibrateControllerAtIndex(controller.device->GetNpadIdType(), device_idx, {});
        }
    }
}

void Controller_NPad::RequestPadStateUpdate(Core::HID::NpadIdType npad_id) {
    std::lock_guard lock{mutex};
    auto& controller = GetControllerFromNpadIdType(npad_id);
    const auto controller_type = controller.device->GetNpadStyleIndex();
    if (!controller.device->IsConnected()) {
        return;
    }

    auto& pad_entry = controller.npad_pad_state;
    auto& trigger_entry = controller.npad_trigger_state;
    const auto button_state = controller.device->GetNpadButtons();
    const auto stick_state = controller.device->GetSticks();

    using btn = Core::HID::NpadButton;
    pad_entry.npad_buttons.raw = btn::None;
    if (controller_type != Core::HID::NpadStyleIndex::JoyconLeft) {
        constexpr btn right_button_mask = btn::A | btn::B | btn::X | btn::Y | btn::StickR | btn::R |
                                          btn::ZR | btn::Plus | btn::StickRLeft | btn::StickRUp |
                                          btn::StickRRight | btn::StickRDown;
        pad_entry.npad_buttons.raw = button_state.raw & right_button_mask;
        pad_entry.r_stick = stick_state.right;
    }

    if (controller_type != Core::HID::NpadStyleIndex::JoyconRight) {
        constexpr btn left_button_mask =
            btn::Left | btn::Up | btn::Right | btn::Down | btn::StickL | btn::L | btn::ZL |
            btn::Minus | btn::StickLLeft | btn::StickLUp | btn::StickLRight | btn::StickLDown;
        pad_entry.npad_buttons.raw |= button_state.raw & left_button_mask;
        pad_entry.l_stick = stick_state.left;
    }

    if (controller_type == Core::HID::NpadStyleIndex::JoyconLeft) {
        pad_entry.npad_buttons.left_sl.Assign(button_state.left_sl);
        pad_entry.npad_buttons.left_sr.Assign(button_state.left_sr);
    }

    if (controller_type == Core::HID::NpadStyleIndex::JoyconRight) {
        pad_entry.npad_buttons.right_sl.Assign(button_state.right_sl);
        pad_entry.npad_buttons.right_sr.Assign(button_state.right_sr);
    }

    if (controller_type == Core::HID::NpadStyleIndex::GameCube) {
        const auto& trigger_state = controller.device->GetTriggers();
        trigger_entry.l_analog = trigger_state.left;
        trigger_entry.r_analog = trigger_state.right;
        pad_entry.npad_buttons.zl.Assign(false);
        pad_entry.npad_buttons.zr.Assign(button_state.r);
        pad_entry.npad_buttons.l.Assign(button_state.zl);
        pad_entry.npad_buttons.r.Assign(button_state.zr);
    }
}

void Controller_NPad::OnUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                               std::size_t data_len) {
    if (!IsControllerActivated()) {
        return;
    }

    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];
        auto& npad = controller.shared_memory_entry;

        const auto& controller_type = controller.device->GetNpadStyleIndex();

        if (controller_type == Core::HID::NpadStyleIndex::None ||
            !controller.device->IsConnected()) {
            // Refresh shared memory
            std::memcpy(data + NPAD_OFFSET + (i * sizeof(NpadInternalState)),
                        &controller.shared_memory_entry, sizeof(NpadInternalState));
            continue;
        }

        RequestPadStateUpdate(controller.device->GetNpadIdType());
        auto& pad_state = controller.npad_pad_state;
        auto& libnx_state = controller.npad_libnx_state;
        auto& trigger_state = controller.npad_trigger_state;

        // LibNX exclusively uses this section, so we always update it since LibNX doesn't activate
        // any controllers.
        libnx_state.connection_status.raw = 0;
        libnx_state.connection_status.is_connected.Assign(1);
        switch (controller_type) {
        case Core::HID::NpadStyleIndex::None:
            UNREACHABLE();
            break;
        case Core::HID::NpadStyleIndex::ProController:
        case Core::HID::NpadStyleIndex::NES:
        case Core::HID::NpadStyleIndex::SNES:
        case Core::HID::NpadStyleIndex::N64:
        case Core::HID::NpadStyleIndex::SegaGenesis:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.connection_status.is_wired.Assign(1);

            libnx_state.connection_status.is_wired.Assign(1);
            pad_state.sampling_number =
                npad.fullkey_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.fullkey_lifo.WriteNextEntry(pad_state);
            break;
        case Core::HID::NpadStyleIndex::Handheld:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.connection_status.is_wired.Assign(1);
            pad_state.connection_status.is_left_connected.Assign(1);
            pad_state.connection_status.is_right_connected.Assign(1);
            pad_state.connection_status.is_left_wired.Assign(1);
            pad_state.connection_status.is_right_wired.Assign(1);

            libnx_state.connection_status.is_wired.Assign(1);
            libnx_state.connection_status.is_left_connected.Assign(1);
            libnx_state.connection_status.is_right_connected.Assign(1);
            libnx_state.connection_status.is_left_wired.Assign(1);
            libnx_state.connection_status.is_right_wired.Assign(1);
            pad_state.sampling_number =
                npad.handheld_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.handheld_lifo.WriteNextEntry(pad_state);
            break;
        case Core::HID::NpadStyleIndex::JoyconDual:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.connection_status.is_left_connected.Assign(1);
            pad_state.connection_status.is_right_connected.Assign(1);

            libnx_state.connection_status.is_left_connected.Assign(1);
            libnx_state.connection_status.is_right_connected.Assign(1);
            pad_state.sampling_number =
                npad.joy_dual_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.joy_dual_lifo.WriteNextEntry(pad_state);
            break;
        case Core::HID::NpadStyleIndex::JoyconLeft:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.connection_status.is_left_connected.Assign(1);

            libnx_state.connection_status.is_left_connected.Assign(1);
            pad_state.sampling_number =
                npad.joy_left_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.joy_left_lifo.WriteNextEntry(pad_state);
            break;
        case Core::HID::NpadStyleIndex::JoyconRight:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.connection_status.is_right_connected.Assign(1);

            libnx_state.connection_status.is_right_connected.Assign(1);
            pad_state.sampling_number =
                npad.joy_right_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.joy_right_lifo.WriteNextEntry(pad_state);
            break;
        case Core::HID::NpadStyleIndex::GameCube:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.connection_status.is_wired.Assign(1);

            libnx_state.connection_status.is_wired.Assign(1);
            pad_state.sampling_number =
                npad.fullkey_lifo.ReadCurrentEntry().state.sampling_number + 1;
            trigger_state.sampling_number =
                npad.gc_trigger_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.fullkey_lifo.WriteNextEntry(pad_state);
            npad.gc_trigger_lifo.WriteNextEntry(trigger_state);
            break;
        case Core::HID::NpadStyleIndex::Pokeball:
            pad_state.connection_status.raw = 0;
            pad_state.connection_status.is_connected.Assign(1);
            pad_state.sampling_number =
                npad.palma_lifo.ReadCurrentEntry().state.sampling_number + 1;
            npad.palma_lifo.WriteNextEntry(pad_state);
            break;
        default:
            break;
        }

        libnx_state.npad_buttons.raw = pad_state.npad_buttons.raw;
        libnx_state.l_stick = pad_state.l_stick;
        libnx_state.r_stick = pad_state.r_stick;
        npad.system_ext_lifo.WriteNextEntry(pad_state);

        press_state |= static_cast<u32>(pad_state.npad_buttons.raw);

        std::memcpy(data + NPAD_OFFSET + (i * sizeof(NpadInternalState)),
                    &controller.shared_memory_entry, sizeof(NpadInternalState));
    }
}

void Controller_NPad::OnMotionUpdate(const Core::Timing::CoreTiming& core_timing, u8* data,
                                     std::size_t data_len) {
    if (!IsControllerActivated()) {
        return;
    }

    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];

        const auto& controller_type = controller.device->GetNpadStyleIndex();

        if (controller_type == Core::HID::NpadStyleIndex::None ||
            !controller.device->IsConnected()) {
            continue;
        }

        auto& npad = controller.shared_memory_entry;
        const auto& motion_state = controller.device->GetMotions();
        auto& sixaxis_fullkey_state = controller.sixaxis_fullkey_state;
        auto& sixaxis_handheld_state = controller.sixaxis_handheld_state;
        auto& sixaxis_dual_left_state = controller.sixaxis_dual_left_state;
        auto& sixaxis_dual_right_state = controller.sixaxis_dual_right_state;
        auto& sixaxis_left_lifo_state = controller.sixaxis_left_lifo_state;
        auto& sixaxis_right_lifo_state = controller.sixaxis_right_lifo_state;

        if (controller.sixaxis_sensor_enabled && Settings::values.motion_enabled.GetValue()) {
            controller.sixaxis_at_rest = true;
            for (std::size_t e = 0; e < motion_state.size(); ++e) {
                controller.sixaxis_at_rest =
                    controller.sixaxis_at_rest && motion_state[e].is_at_rest;
            }
        }

        switch (controller_type) {
        case Core::HID::NpadStyleIndex::None:
            UNREACHABLE();
            break;
        case Core::HID::NpadStyleIndex::ProController:
            sixaxis_fullkey_state.attribute.raw = 0;
            if (controller.sixaxis_sensor_enabled) {
                sixaxis_fullkey_state.attribute.is_connected.Assign(1);
                sixaxis_fullkey_state.accel = motion_state[0].accel;
                sixaxis_fullkey_state.gyro = motion_state[0].gyro;
                sixaxis_fullkey_state.rotation = motion_state[0].rotation;
                sixaxis_fullkey_state.orientation = motion_state[0].orientation;
            }
            break;
        case Core::HID::NpadStyleIndex::Handheld:
            sixaxis_handheld_state.attribute.raw = 0;
            if (controller.sixaxis_sensor_enabled) {
                sixaxis_handheld_state.attribute.is_connected.Assign(1);
                sixaxis_handheld_state.accel = motion_state[0].accel;
                sixaxis_handheld_state.gyro = motion_state[0].gyro;
                sixaxis_handheld_state.rotation = motion_state[0].rotation;
                sixaxis_handheld_state.orientation = motion_state[0].orientation;
            }
            break;
        case Core::HID::NpadStyleIndex::JoyconDual:
            sixaxis_dual_left_state.attribute.raw = 0;
            sixaxis_dual_right_state.attribute.raw = 0;
            if (controller.sixaxis_sensor_enabled) {
                // Set motion for the left joycon
                sixaxis_dual_left_state.attribute.is_connected.Assign(1);
                sixaxis_dual_left_state.accel = motion_state[0].accel;
                sixaxis_dual_left_state.gyro = motion_state[0].gyro;
                sixaxis_dual_left_state.rotation = motion_state[0].rotation;
                sixaxis_dual_left_state.orientation = motion_state[0].orientation;
            }
            if (controller.sixaxis_sensor_enabled) {
                // Set motion for the right joycon
                sixaxis_dual_right_state.attribute.is_connected.Assign(1);
                sixaxis_dual_right_state.accel = motion_state[1].accel;
                sixaxis_dual_right_state.gyro = motion_state[1].gyro;
                sixaxis_dual_right_state.rotation = motion_state[1].rotation;
                sixaxis_dual_right_state.orientation = motion_state[1].orientation;
            }
            break;
        case Core::HID::NpadStyleIndex::JoyconLeft:
            sixaxis_left_lifo_state.attribute.raw = 0;
            if (controller.sixaxis_sensor_enabled) {
                sixaxis_left_lifo_state.attribute.is_connected.Assign(1);
                sixaxis_left_lifo_state.accel = motion_state[0].accel;
                sixaxis_left_lifo_state.gyro = motion_state[0].gyro;
                sixaxis_left_lifo_state.rotation = motion_state[0].rotation;
                sixaxis_left_lifo_state.orientation = motion_state[0].orientation;
            }
            break;
        case Core::HID::NpadStyleIndex::JoyconRight:
            sixaxis_right_lifo_state.attribute.raw = 0;
            if (controller.sixaxis_sensor_enabled) {
                sixaxis_right_lifo_state.attribute.is_connected.Assign(1);
                sixaxis_right_lifo_state.accel = motion_state[1].accel;
                sixaxis_right_lifo_state.gyro = motion_state[1].gyro;
                sixaxis_right_lifo_state.rotation = motion_state[1].rotation;
                sixaxis_right_lifo_state.orientation = motion_state[1].orientation;
            }
            break;
        default:
            break;
        }

        sixaxis_fullkey_state.sampling_number =
            npad.sixaxis_fullkey_lifo.ReadCurrentEntry().state.sampling_number + 1;
        sixaxis_handheld_state.sampling_number =
            npad.sixaxis_handheld_lifo.ReadCurrentEntry().state.sampling_number + 1;
        sixaxis_dual_left_state.sampling_number =
            npad.sixaxis_dual_left_lifo.ReadCurrentEntry().state.sampling_number + 1;
        sixaxis_dual_right_state.sampling_number =
            npad.sixaxis_dual_right_lifo.ReadCurrentEntry().state.sampling_number + 1;
        sixaxis_left_lifo_state.sampling_number =
            npad.sixaxis_left_lifo.ReadCurrentEntry().state.sampling_number + 1;
        sixaxis_right_lifo_state.sampling_number =
            npad.sixaxis_right_lifo.ReadCurrentEntry().state.sampling_number + 1;

        if (Core::HID::IndexToNpadIdType(i) == Core::HID::NpadIdType::Handheld) {
            // This buffer only is updated on handheld on HW
            npad.sixaxis_handheld_lifo.WriteNextEntry(sixaxis_handheld_state);
        } else {
            // Hanheld doesn't update this buffer on HW
            npad.sixaxis_fullkey_lifo.WriteNextEntry(sixaxis_fullkey_state);
        }

        npad.sixaxis_dual_left_lifo.WriteNextEntry(sixaxis_dual_left_state);
        npad.sixaxis_dual_right_lifo.WriteNextEntry(sixaxis_dual_right_state);
        npad.sixaxis_left_lifo.WriteNextEntry(sixaxis_left_lifo_state);
        npad.sixaxis_right_lifo.WriteNextEntry(sixaxis_right_lifo_state);
        std::memcpy(data + NPAD_OFFSET + (i * sizeof(NpadInternalState)),
                    &controller.shared_memory_entry, sizeof(NpadInternalState));
    }
}

void Controller_NPad::SetSupportedStyleSet(Core::HID::NpadStyleTag style_set) {
    hid_core.SetSupportedStyleTag(style_set);
}

Core::HID::NpadStyleTag Controller_NPad::GetSupportedStyleSet() const {
    return hid_core.GetSupportedStyleTag();
}

void Controller_NPad::SetSupportedNpadIdTypes(u8* data, std::size_t length) {
    ASSERT(length > 0 && (length % sizeof(u32)) == 0);
    supported_npad_id_types.clear();
    supported_npad_id_types.resize(length / sizeof(u32));
    std::memcpy(supported_npad_id_types.data(), data, length);
}

void Controller_NPad::GetSupportedNpadIdTypes(u32* data, std::size_t max_length) {
    ASSERT(max_length < supported_npad_id_types.size());
    std::memcpy(data, supported_npad_id_types.data(), supported_npad_id_types.size());
}

std::size_t Controller_NPad::GetSupportedNpadIdTypesSize() const {
    return supported_npad_id_types.size();
}

void Controller_NPad::SetHoldType(NpadJoyHoldType joy_hold_type) {
    hold_type = joy_hold_type;
}

Controller_NPad::NpadJoyHoldType Controller_NPad::GetHoldType() const {
    return hold_type;
}

void Controller_NPad::SetNpadHandheldActivationMode(NpadHandheldActivationMode activation_mode) {
    handheld_activation_mode = activation_mode;
}

Controller_NPad::NpadHandheldActivationMode Controller_NPad::GetNpadHandheldActivationMode() const {
    return handheld_activation_mode;
}

void Controller_NPad::SetNpadCommunicationMode(NpadCommunicationMode communication_mode_) {
    communication_mode = communication_mode_;
}

Controller_NPad::NpadCommunicationMode Controller_NPad::GetNpadCommunicationMode() const {
    return communication_mode;
}

void Controller_NPad::SetNpadMode(Core::HID::NpadIdType npad_id,
                                  NpadJoyAssignmentMode assignment_mode) {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        return;
    }

    auto& controller = GetControllerFromNpadIdType(npad_id);
    if (controller.shared_memory_entry.assignment_mode != assignment_mode) {
        controller.shared_memory_entry.assignment_mode = assignment_mode;
    }
}

bool Controller_NPad::VibrateControllerAtIndex(Core::HID::NpadIdType npad_id,
                                               std::size_t device_index,
                                               const Core::HID::VibrationValue& vibration_value) {
    auto& controller = GetControllerFromNpadIdType(npad_id);
    if (!controller.device->IsConnected()) {
        return false;
    }

    if (!controller.device->IsVibrationEnabled()) {
        if (controller.vibration[device_index].latest_vibration_value.low_amplitude != 0.0f ||
            controller.vibration[device_index].latest_vibration_value.high_amplitude != 0.0f) {
            // Send an empty vibration to stop any vibrations.
            Core::HID::VibrationValue vibration{0.0f, 160.0f, 0.0f, 320.0f};
            controller.device->SetVibration(device_index, vibration);
            // Then reset the vibration value to its default value.
            controller.vibration[device_index].latest_vibration_value = DEFAULT_VIBRATION_VALUE;
        }

        return false;
    }

    if (!Settings::values.enable_accurate_vibrations.GetValue()) {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::steady_clock;

        const auto now = steady_clock::now();

        // Filter out non-zero vibrations that are within 10ms of each other.
        if ((vibration_value.low_amplitude != 0.0f || vibration_value.high_amplitude != 0.0f) &&
            duration_cast<milliseconds>(
                now - controller.vibration[device_index].last_vibration_timepoint) <
                milliseconds(10)) {
            return false;
        }

        controller.vibration[device_index].last_vibration_timepoint = now;
    }

    Core::HID::VibrationValue vibration{
        vibration_value.low_amplitude, vibration_value.low_frequency,
        vibration_value.high_amplitude, vibration_value.high_frequency};
    return controller.device->SetVibration(device_index, vibration);
}

void Controller_NPad::VibrateController(
    const Core::HID::VibrationDeviceHandle& vibration_device_handle,
    const Core::HID::VibrationValue& vibration_value) {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return;
    }

    if (!Settings::values.vibration_enabled.GetValue() && !permit_vibration_session_enabled) {
        return;
    }

    auto& controller = GetControllerFromHandle(vibration_device_handle);
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);

    if (!controller.vibration[device_index].device_mounted || !controller.device->IsConnected()) {
        return;
    }

    if (vibration_device_handle.device_index == Core::HID::DeviceIndex::None) {
        UNREACHABLE_MSG("DeviceIndex should never be None!");
        return;
    }

    // Some games try to send mismatched parameters in the device handle, block these.
    if ((controller.device->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconLeft &&
         (vibration_device_handle.npad_type == Core::HID::NpadStyleIndex::JoyconRight ||
          vibration_device_handle.device_index == Core::HID::DeviceIndex::Right)) ||
        (controller.device->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconRight &&
         (vibration_device_handle.npad_type == Core::HID::NpadStyleIndex::JoyconLeft ||
          vibration_device_handle.device_index == Core::HID::DeviceIndex::Left))) {
        return;
    }

    // Filter out vibrations with equivalent values to reduce unnecessary state changes.
    if (vibration_value.low_amplitude ==
            controller.vibration[device_index].latest_vibration_value.low_amplitude &&
        vibration_value.high_amplitude ==
            controller.vibration[device_index].latest_vibration_value.high_amplitude) {
        return;
    }

    if (VibrateControllerAtIndex(controller.device->GetNpadIdType(), device_index,
                                 vibration_value)) {
        controller.vibration[device_index].latest_vibration_value = vibration_value;
    }
}

void Controller_NPad::VibrateControllers(
    const std::vector<Core::HID::VibrationDeviceHandle>& vibration_device_handles,
    const std::vector<Core::HID::VibrationValue>& vibration_values) {
    if (!Settings::values.vibration_enabled.GetValue() && !permit_vibration_session_enabled) {
        return;
    }

    ASSERT_OR_EXECUTE_MSG(
        vibration_device_handles.size() == vibration_values.size(), { return; },
        "The amount of device handles does not match with the amount of vibration values,"
        "this is undefined behavior!");

    for (std::size_t i = 0; i < vibration_device_handles.size(); ++i) {
        VibrateController(vibration_device_handles[i], vibration_values[i]);
    }
}

Core::HID::VibrationValue Controller_NPad::GetLastVibration(
    const Core::HID::VibrationDeviceHandle& vibration_device_handle) const {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return {};
    }

    const auto& controller = GetControllerFromHandle(vibration_device_handle);
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);
    return controller.vibration[device_index].latest_vibration_value;
}

void Controller_NPad::InitializeVibrationDevice(
    const Core::HID::VibrationDeviceHandle& vibration_device_handle) {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return;
    }

    const auto npad_index = static_cast<Core::HID::NpadIdType>(vibration_device_handle.npad_id);
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);
    InitializeVibrationDeviceAtIndex(npad_index, device_index);
}

void Controller_NPad::InitializeVibrationDeviceAtIndex(Core::HID::NpadIdType npad_id,
                                                       std::size_t device_index) {
    auto& controller = GetControllerFromNpadIdType(npad_id);
    if (!Settings::values.vibration_enabled.GetValue()) {
        controller.vibration[device_index].device_mounted = false;
        return;
    }

    controller.vibration[device_index].device_mounted =
        controller.device->TestVibration(device_index);
}

void Controller_NPad::SetPermitVibrationSession(bool permit_vibration_session) {
    permit_vibration_session_enabled = permit_vibration_session;
}

bool Controller_NPad::IsVibrationDeviceMounted(
    const Core::HID::VibrationDeviceHandle& vibration_device_handle) const {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return false;
    }

    const auto& controller = GetControllerFromHandle(vibration_device_handle);
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);
    return controller.vibration[device_index].device_mounted;
}

Kernel::KReadableEvent& Controller_NPad::GetStyleSetChangedEvent(Core::HID::NpadIdType npad_id) {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        // Fallback to player 1
        const auto& controller = GetControllerFromNpadIdType(Core::HID::NpadIdType::Player1);
        return controller.styleset_changed_event->GetReadableEvent();
    }

    const auto& controller = GetControllerFromNpadIdType(npad_id);
    return controller.styleset_changed_event->GetReadableEvent();
}

void Controller_NPad::SignalStyleSetChangedEvent(Core::HID::NpadIdType npad_id) const {
    const auto& controller = GetControllerFromNpadIdType(npad_id);
    controller.styleset_changed_event->GetWritableEvent().Signal();
}

void Controller_NPad::AddNewControllerAt(Core::HID::NpadStyleIndex controller,
                                         Core::HID::NpadIdType npad_id) {
    UpdateControllerAt(controller, npad_id, true);
}

void Controller_NPad::UpdateControllerAt(Core::HID::NpadStyleIndex type,
                                         Core::HID::NpadIdType npad_id, bool connected) {
    auto& controller = GetControllerFromNpadIdType(npad_id);
    if (!connected) {
        DisconnectNpad(npad_id);
        return;
    }

    controller.device->SetNpadStyleIndex(type);
    InitNewlyAddedController(npad_id);
}

void Controller_NPad::DisconnectNpad(Core::HID::NpadIdType npad_id) {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        return;
    }

    LOG_DEBUG(Service_HID, "Npad disconnected {}", npad_id);
    auto& controller = GetControllerFromNpadIdType(npad_id);
    for (std::size_t device_idx = 0; device_idx < controller.vibration.size(); ++device_idx) {
        // Send an empty vibration to stop any vibrations.
        VibrateControllerAtIndex(npad_id, device_idx, {});
        controller.vibration[device_idx].device_mounted = false;
    }

    auto& shared_memory_entry = controller.shared_memory_entry;
    shared_memory_entry.style_tag.raw = Core::HID::NpadStyleSet::None; // Zero out
    shared_memory_entry.device_type.raw = 0;
    shared_memory_entry.system_properties.raw = 0;
    shared_memory_entry.button_properties.raw = 0;
    shared_memory_entry.battery_level_dual = 0;
    shared_memory_entry.battery_level_left = 0;
    shared_memory_entry.battery_level_right = 0;
    shared_memory_entry.fullkey_color = {
        .attribute = ColorAttribute::NoController,
        .fullkey = {},
    };
    shared_memory_entry.joycon_color = {
        .attribute = ColorAttribute::NoController,
        .left = {},
        .right = {},
    };
    shared_memory_entry.assignment_mode = NpadJoyAssignmentMode::Dual;
    shared_memory_entry.applet_footer.type = AppletFooterUiType::None;

    controller.is_connected = false;
    controller.device->Disconnect();
    SignalStyleSetChangedEvent(npad_id);
    WriteEmptyEntry(controller.shared_memory_entry);
}

void Controller_NPad::SetGyroscopeZeroDriftMode(Core::HID::SixAxisSensorHandle sixaxis_handle,
                                                GyroscopeZeroDriftMode drift_mode) {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        return;
    }
    auto& controller = GetControllerFromHandle(sixaxis_handle);
    controller.gyroscope_zero_drift_mode = drift_mode;
}

Controller_NPad::GyroscopeZeroDriftMode Controller_NPad::GetGyroscopeZeroDriftMode(
    Core::HID::SixAxisSensorHandle sixaxis_handle) const {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        // Return the default value
        return GyroscopeZeroDriftMode::Standard;
    }
    const auto& controller = GetControllerFromHandle(sixaxis_handle);
    return controller.gyroscope_zero_drift_mode;
}

bool Controller_NPad::IsSixAxisSensorAtRest(Core::HID::SixAxisSensorHandle sixaxis_handle) const {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        // Return the default value
        return true;
    }
    const auto& controller = GetControllerFromHandle(sixaxis_handle);
    return controller.sixaxis_at_rest;
}

void Controller_NPad::SetSixAxisEnabled(Core::HID::SixAxisSensorHandle sixaxis_handle,
                                        bool sixaxis_status) {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        return;
    }
    auto& controller = GetControllerFromHandle(sixaxis_handle);
    controller.sixaxis_sensor_enabled = sixaxis_status;
}

void Controller_NPad::SetSixAxisFusionEnabled(Core::HID::SixAxisSensorHandle sixaxis_handle,
                                              bool sixaxis_fusion_status) {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        return;
    }
    auto& controller = GetControllerFromHandle(sixaxis_handle);
    controller.sixaxis_fusion_enabled = sixaxis_fusion_status;
}

void Controller_NPad::SetSixAxisFusionParameters(
    Core::HID::SixAxisSensorHandle sixaxis_handle,
    Core::HID::SixAxisSensorFusionParameters sixaxis_fusion_parameters) {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        return;
    }
    auto& controller = GetControllerFromHandle(sixaxis_handle);
    controller.sixaxis_fusion = sixaxis_fusion_parameters;
}

Core::HID::SixAxisSensorFusionParameters Controller_NPad::GetSixAxisFusionParameters(
    Core::HID::SixAxisSensorHandle sixaxis_handle) {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        // Since these parameters are unknow just return zeros
        return {};
    }
    auto& controller = GetControllerFromHandle(sixaxis_handle);
    return controller.sixaxis_fusion;
}

void Controller_NPad::ResetSixAxisFusionParameters(Core::HID::SixAxisSensorHandle sixaxis_handle) {
    if (!IsDeviceHandleValid(sixaxis_handle)) {
        LOG_ERROR(Service_HID, "Invalid handle");
        return;
    }
    auto& controller = GetControllerFromHandle(sixaxis_handle);
    // Since these parameters are unknow just fill with zeros
    controller.sixaxis_fusion = {};
}

void Controller_NPad::MergeSingleJoyAsDualJoy(Core::HID::NpadIdType npad_id_1,
                                              Core::HID::NpadIdType npad_id_2) {
    if (!IsNpadIdValid(npad_id_1) || !IsNpadIdValid(npad_id_2)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id_1:{}, npad_id_2:{}", npad_id_1,
                  npad_id_2);
        return;
    }
    auto& controller_1 = GetControllerFromNpadIdType(npad_id_1).device;
    auto& controller_2 = GetControllerFromNpadIdType(npad_id_2).device;

    // If the controllers at both npad indices form a pair of left and right joycons, merge them.
    // Otherwise, do nothing.
    if ((controller_1->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconLeft &&
         controller_2->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconRight) ||
        (controller_2->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconLeft &&
         controller_1->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconRight)) {
        // Disconnect the joycon at the second id and connect the dual joycon at the first index.
        DisconnectNpad(npad_id_2);
        AddNewControllerAt(Core::HID::NpadStyleIndex::JoyconDual, npad_id_1);
    }
}

void Controller_NPad::StartLRAssignmentMode() {
    // Nothing internally is used for lr assignment mode. Since we have the ability to set the
    // controller types from boot, it doesn't really matter about showing a selection screen
    is_in_lr_assignment_mode = true;
}

void Controller_NPad::StopLRAssignmentMode() {
    is_in_lr_assignment_mode = false;
}

bool Controller_NPad::SwapNpadAssignment(Core::HID::NpadIdType npad_id_1,
                                         Core::HID::NpadIdType npad_id_2) {
    if (!IsNpadIdValid(npad_id_1) || !IsNpadIdValid(npad_id_2)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id_1:{}, npad_id_2:{}", npad_id_1,
                  npad_id_2);
        return false;
    }
    if (npad_id_1 == Core::HID::NpadIdType::Handheld ||
        npad_id_2 == Core::HID::NpadIdType::Handheld || npad_id_1 == Core::HID::NpadIdType::Other ||
        npad_id_2 == Core::HID::NpadIdType::Other) {
        return true;
    }
    const auto& controller_1 = GetControllerFromNpadIdType(npad_id_1).device;
    const auto& controller_2 = GetControllerFromNpadIdType(npad_id_2).device;
    const auto type_index_1 = controller_1->GetNpadStyleIndex();
    const auto type_index_2 = controller_2->GetNpadStyleIndex();

    if (!IsControllerSupported(type_index_1) || !IsControllerSupported(type_index_2)) {
        return false;
    }

    AddNewControllerAt(type_index_2, npad_id_1);
    AddNewControllerAt(type_index_1, npad_id_2);

    return true;
}

Core::HID::LedPattern Controller_NPad::GetLedPattern(Core::HID::NpadIdType npad_id) {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        return Core::HID::LedPattern{0, 0, 0, 0};
    }
    const auto& controller = GetControllerFromNpadIdType(npad_id).device;
    return controller->GetLedPattern();
}

bool Controller_NPad::IsUnintendedHomeButtonInputProtectionEnabled(
    Core::HID::NpadIdType npad_id) const {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        // Return the default value
        return false;
    }
    const auto& controller = GetControllerFromNpadIdType(npad_id);
    return controller.unintended_home_button_input_protection;
}

void Controller_NPad::SetUnintendedHomeButtonInputProtectionEnabled(bool is_protection_enabled,
                                                                    Core::HID::NpadIdType npad_id) {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        return;
    }
    auto& controller = GetControllerFromNpadIdType(npad_id);
    controller.unintended_home_button_input_protection = is_protection_enabled;
}

void Controller_NPad::SetAnalogStickUseCenterClamp(bool use_center_clamp) {
    analog_stick_use_center_clamp = use_center_clamp;
}

void Controller_NPad::ClearAllConnectedControllers() {
    for (auto& controller : controller_data) {
        if (controller.device->IsConnected() &&
            controller.device->GetNpadStyleIndex() != Core::HID::NpadStyleIndex::None) {
            controller.device->Disconnect();
            controller.device->SetNpadStyleIndex(Core::HID::NpadStyleIndex::None);
        }
    }
}

void Controller_NPad::DisconnectAllConnectedControllers() {
    for (auto& controller : controller_data) {
        controller.device->Disconnect();
    }
}

void Controller_NPad::ConnectAllDisconnectedControllers() {
    for (auto& controller : controller_data) {
        if (controller.device->GetNpadStyleIndex() != Core::HID::NpadStyleIndex::None &&
            !controller.device->IsConnected()) {
            controller.device->Connect();
        }
    }
}

void Controller_NPad::ClearAllControllers() {
    for (auto& controller : controller_data) {
        controller.device->Disconnect();
        controller.device->SetNpadStyleIndex(Core::HID::NpadStyleIndex::None);
    }
}

u32 Controller_NPad::GetAndResetPressState() {
    return press_state.exchange(0);
}

bool Controller_NPad::IsControllerSupported(Core::HID::NpadStyleIndex controller) const {
    if (controller == Core::HID::NpadStyleIndex::Handheld) {
        const bool support_handheld =
            std::find(supported_npad_id_types.begin(), supported_npad_id_types.end(),
                      Core::HID::NpadIdType::Handheld) != supported_npad_id_types.end();
        // Handheld is not even a supported type, lets stop here
        if (!support_handheld) {
            return false;
        }
        // Handheld shouldn't be supported in docked mode
        if (Settings::values.use_docked_mode.GetValue()) {
            return false;
        }

        return true;
    }

    if (std::any_of(supported_npad_id_types.begin(), supported_npad_id_types.end(),
                    [](Core::HID::NpadIdType npad_id) {
                        return npad_id <= Core::HID::NpadIdType::Player8;
                    })) {
        Core::HID::NpadStyleTag style = GetSupportedStyleSet();
        switch (controller) {
        case Core::HID::NpadStyleIndex::ProController:
            return style.fullkey;
        case Core::HID::NpadStyleIndex::JoyconDual:
            return style.joycon_dual;
        case Core::HID::NpadStyleIndex::JoyconLeft:
            return style.joycon_left;
        case Core::HID::NpadStyleIndex::JoyconRight:
            return style.joycon_right;
        case Core::HID::NpadStyleIndex::GameCube:
            return style.gamecube;
        case Core::HID::NpadStyleIndex::Pokeball:
            return style.palma;
        case Core::HID::NpadStyleIndex::NES:
            return style.lark;
        case Core::HID::NpadStyleIndex::SNES:
            return style.lucia;
        case Core::HID::NpadStyleIndex::N64:
            return style.lagoon;
        case Core::HID::NpadStyleIndex::SegaGenesis:
            return style.lager;
        default:
            return false;
        }
    }

    return false;
}

Controller_NPad::NpadControllerData& Controller_NPad::GetControllerFromHandle(
    const Core::HID::SixAxisSensorHandle& device_handle) {
    const auto npad_id = static_cast<Core::HID::NpadIdType>(device_handle.npad_id);
    return GetControllerFromNpadIdType(npad_id);
}

const Controller_NPad::NpadControllerData& Controller_NPad::GetControllerFromHandle(
    const Core::HID::SixAxisSensorHandle& device_handle) const {
    const auto npad_id = static_cast<Core::HID::NpadIdType>(device_handle.npad_id);
    return GetControllerFromNpadIdType(npad_id);
}

Controller_NPad::NpadControllerData& Controller_NPad::GetControllerFromHandle(
    const Core::HID::VibrationDeviceHandle& device_handle) {
    const auto npad_id = static_cast<Core::HID::NpadIdType>(device_handle.npad_id);
    return GetControllerFromNpadIdType(npad_id);
}

const Controller_NPad::NpadControllerData& Controller_NPad::GetControllerFromHandle(
    const Core::HID::VibrationDeviceHandle& device_handle) const {
    const auto npad_id = static_cast<Core::HID::NpadIdType>(device_handle.npad_id);
    return GetControllerFromNpadIdType(npad_id);
}

Controller_NPad::NpadControllerData& Controller_NPad::GetControllerFromNpadIdType(
    Core::HID::NpadIdType npad_id) {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        npad_id = Core::HID::NpadIdType::Player1;
    }
    const auto npad_index = Core::HID::NpadIdTypeToIndex(npad_id);
    return controller_data[npad_index];
}

const Controller_NPad::NpadControllerData& Controller_NPad::GetControllerFromNpadIdType(
    Core::HID::NpadIdType npad_id) const {
    if (!IsNpadIdValid(npad_id)) {
        LOG_ERROR(Service_HID, "Invalid NpadIdType npad_id:{}", npad_id);
        npad_id = Core::HID::NpadIdType::Player1;
    }
    const auto npad_index = Core::HID::NpadIdTypeToIndex(npad_id);
    return controller_data[npad_index];
}

} // namespace Service::HID
