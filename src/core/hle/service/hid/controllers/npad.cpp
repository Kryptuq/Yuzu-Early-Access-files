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
constexpr u32 MAX_NPAD_ID = 7;
constexpr std::size_t HANDHELD_INDEX = 8;
constexpr std::array<u32, 10> npad_id_list{
    0, 1, 2, 3, 4, 5, 6, 7, NPAD_HANDHELD, NPAD_UNKNOWN,
};

std::size_t Controller_NPad::NPadIdToIndex(u32 npad_id) {
    switch (npad_id) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        return npad_id;
    case HANDHELD_INDEX:
    case NPAD_HANDHELD:
        return HANDHELD_INDEX;
    case 9:
    case NPAD_UNKNOWN:
        return 9;
    default:
        UNIMPLEMENTED_MSG("Unknown npad id {}", npad_id);
        return 0;
    }
}

u32 Controller_NPad::IndexToNPad(std::size_t index) {
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        return static_cast<u32>(index);
    case HANDHELD_INDEX:
        return NPAD_HANDHELD;
    case 9:
        return NPAD_UNKNOWN;
    default:
        UNIMPLEMENTED_MSG("Unknown npad index {}", index);
        return 0;
    }
}

bool Controller_NPad::IsNpadIdValid(u32 npad_id) {
    switch (npad_id) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case NPAD_UNKNOWN:
    case NPAD_HANDHELD:
        return true;
    default:
        LOG_ERROR(Service_HID, "Invalid npad id {}", npad_id);
        return false;
    }
}

bool Controller_NPad::IsDeviceHandleValid(const DeviceHandle& device_handle) {
    return IsNpadIdValid(device_handle.npad_id) &&
           device_handle.npad_type < Core::HID::NpadStyleIndex::MaxNpadType &&
           device_handle.device_index < DeviceIndex::MaxDeviceIndex;
}

Controller_NPad::Controller_NPad(Core::HID::HIDCore& hid_core_,
                                 KernelHelpers::ServiceContext& service_context_)
    : ControllerBase{hid_core_}, service_context{service_context_} {
    for (std::size_t i = 0; i < controller_data.size(); ++i) {
        auto& controller = controller_data[i];
        controller.device = hid_core.GetEmulatedControllerByIndex(i);
        controller.vibration[Core::HID::DeviceIndex::LeftIndex].latest_vibration_value =
            DEFAULT_VIBRATION_VALUE;
        controller.vibration[Core::HID::DeviceIndex::RightIndex].latest_vibration_value =
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

    auto& controller = controller_data[controller_idx];
    const auto is_connected = controller.device->IsConnected();
    const auto npad_type = controller.device->GetNpadStyleIndex();
    switch (type) {
    case Core::HID::ControllerTriggerType::Connected:
    case Core::HID::ControllerTriggerType::Disconnected:
        if (is_connected == controller.is_connected) {
            return;
        }
        UpdateControllerAt(npad_type, controller_idx, is_connected);
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

void Controller_NPad::InitNewlyAddedController(std::size_t controller_idx) {
    auto& controller = controller_data[controller_idx];
    const auto controller_type = controller.device->GetNpadStyleIndex();
    auto& shared_memory = controller.shared_memory_entry;
    if (controller_type == Core::HID::NpadStyleIndex::None) {
        controller.styleset_changed_event->GetWritableEvent().Signal();
        return;
    }
    shared_memory.style_set.raw = 0; // Zero out
    shared_memory.device_type.raw = 0;
    shared_memory.system_properties.raw = 0;
    switch (controller_type) {
    case Core::HID::NpadStyleIndex::None:
        UNREACHABLE();
        break;
    case Core::HID::NpadStyleIndex::ProController:
        shared_memory.style_set.fullkey.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        shared_memory.applet_footer.type = AppletFooterUiType::SwitchProController;
        break;
    case Core::HID::NpadStyleIndex::Handheld:
        shared_memory.style_set.handheld.Assign(1);
        shared_memory.device_type.handheld_left.Assign(1);
        shared_memory.device_type.handheld_right.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Dual;
        shared_memory.applet_footer.type = AppletFooterUiType::HandheldJoyConLeftJoyConRight;
        break;
    case Core::HID::NpadStyleIndex::JoyconDual:
        shared_memory.style_set.joycon_dual.Assign(1);
        shared_memory.device_type.joycon_left.Assign(1);
        shared_memory.device_type.joycon_right.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Dual;
        shared_memory.applet_footer.type = AppletFooterUiType::JoyDual;
        break;
    case Core::HID::NpadStyleIndex::JoyconLeft:
        shared_memory.style_set.joycon_left.Assign(1);
        shared_memory.device_type.joycon_left.Assign(1);
        shared_memory.system_properties.is_horizontal.Assign(1);
        shared_memory.system_properties.use_minus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        shared_memory.applet_footer.type = AppletFooterUiType::JoyLeftHorizontal;
        break;
    case Core::HID::NpadStyleIndex::JoyconRight:
        shared_memory.style_set.joycon_right.Assign(1);
        shared_memory.device_type.joycon_right.Assign(1);
        shared_memory.system_properties.is_horizontal.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        shared_memory.applet_footer.type = AppletFooterUiType::JoyRightHorizontal;
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        shared_memory.style_set.gamecube.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.system_properties.is_vertical.Assign(1);
        shared_memory.system_properties.use_plus.Assign(1);
        break;
    case Core::HID::NpadStyleIndex::Pokeball:
        shared_memory.style_set.palma.Assign(1);
        shared_memory.device_type.palma.Assign(1);
        shared_memory.assignment_mode = NpadJoyAssignmentMode::Single;
        break;
    case Core::HID::NpadStyleIndex::NES:
        shared_memory.style_set.lark.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        break;
    case Core::HID::NpadStyleIndex::SNES:
        shared_memory.style_set.lucia.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.applet_footer.type = AppletFooterUiType::Lucia;
        break;
    case Core::HID::NpadStyleIndex::N64:
        shared_memory.style_set.lagoon.Assign(1);
        shared_memory.device_type.fullkey.Assign(1);
        shared_memory.applet_footer.type = AppletFooterUiType::Lagon;
        break;
    case Core::HID::NpadStyleIndex::SegaGenesis:
        shared_memory.style_set.lager.Assign(1);
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
    SignalStyleSetChangedEvent(IndexToNPad(controller_idx));
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

    if (hid_core.GetSupportedStyleTag().raw == 0) {
        // We want to support all controllers
        Core::HID::NpadStyleTag style{};
        style.handheld.Assign(1);
        style.joycon_left.Assign(1);
        style.joycon_right.Assign(1);
        style.joycon_dual.Assign(1);
        style.fullkey.Assign(1);
        style.gamecube.Assign(1);
        style.palma.Assign(1);
        hid_core.SetSupportedStyleTag(style);
    }

    supported_npad_id_types.resize(npad_id_list.size());
    std::memcpy(supported_npad_id_types.data(), npad_id_list.data(),
                npad_id_list.size() * sizeof(u32));

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
            const std::size_t index = Core::HID::NpadIdTypeToIndex(device->GetNpadIdType());
            AddNewControllerAt(device->GetNpadStyleIndex(), index);
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
            VibrateControllerAtIndex(i, device_idx, {});
        }
    }
}

void Controller_NPad::RequestPadStateUpdate(u32 npad_id) {
    std::lock_guard lock{mutex};
    const auto controller_idx = NPadIdToIndex(npad_id);
    auto& controller = controller_data[controller_idx];
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
        pad_entry.npad_buttons.raw |= button_state.raw & right_button_mask;
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
        const u32 npad_index = static_cast<u32>(i);

        RequestPadStateUpdate(npad_index);
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

        if (sixaxis_sensors_enabled && Settings::values.motion_enabled.GetValue()) {
            sixaxis_at_rest = true;
            for (std::size_t e = 0; e < motion_state.size(); ++e) {
                sixaxis_at_rest = sixaxis_at_rest && motion_state[e].is_at_rest;
            }
        }

        switch (controller_type) {
        case Core::HID::NpadStyleIndex::None:
            UNREACHABLE();
            break;
        case Core::HID::NpadStyleIndex::ProController:
            sixaxis_fullkey_state.attribute.raw = 0;
            if (sixaxis_sensors_enabled) {
                sixaxis_fullkey_state.attribute.is_connected.Assign(1);
                sixaxis_fullkey_state.accel = motion_state[0].accel;
                sixaxis_fullkey_state.gyro = motion_state[0].gyro;
                sixaxis_fullkey_state.rotation = motion_state[0].rotation;
                sixaxis_fullkey_state.orientation = motion_state[0].orientation;
            }
            break;
        case Core::HID::NpadStyleIndex::Handheld:
            sixaxis_handheld_state.attribute.raw = 0;
            if (sixaxis_sensors_enabled) {
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
            if (sixaxis_sensors_enabled) {
                // Set motion for the left joycon
                sixaxis_dual_left_state.attribute.is_connected.Assign(1);
                sixaxis_dual_left_state.accel = motion_state[0].accel;
                sixaxis_dual_left_state.gyro = motion_state[0].gyro;
                sixaxis_dual_left_state.rotation = motion_state[0].rotation;
                sixaxis_dual_left_state.orientation = motion_state[0].orientation;
            }
            if (sixaxis_sensors_enabled) {
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
            if (sixaxis_sensors_enabled) {
                sixaxis_left_lifo_state.attribute.is_connected.Assign(1);
                sixaxis_left_lifo_state.accel = motion_state[0].accel;
                sixaxis_left_lifo_state.gyro = motion_state[0].gyro;
                sixaxis_left_lifo_state.rotation = motion_state[0].rotation;
                sixaxis_left_lifo_state.orientation = motion_state[0].orientation;
            }
            break;
        case Core::HID::NpadStyleIndex::JoyconRight:
            sixaxis_right_lifo_state.attribute.raw = 0;
            if (sixaxis_sensors_enabled) {
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

void Controller_NPad::SetNpadMode(u32 npad_id, NpadJoyAssignmentMode assignment_mode) {
    const std::size_t npad_index = NPadIdToIndex(npad_id);
    ASSERT(npad_index < controller_data.size());
    auto& controller = controller_data[npad_index];
    if (controller.shared_memory_entry.assignment_mode != assignment_mode) {
        controller.shared_memory_entry.assignment_mode = assignment_mode;
    }
}

bool Controller_NPad::VibrateControllerAtIndex(std::size_t npad_index, std::size_t device_index,
                                               const VibrationValue& vibration_value) {
    auto& controller = controller_data[npad_index];

    if (!controller.device->IsConnected()) {
        return false;
    }

    if (!controller.device->IsVibrationEnabled()) {
        if (controller.vibration[device_index].latest_vibration_value.amp_low != 0.0f ||
            controller.vibration[device_index].latest_vibration_value.amp_high != 0.0f) {
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
        if ((vibration_value.amp_low != 0.0f || vibration_value.amp_high != 0.0f) &&
            duration_cast<milliseconds>(
                now - controller.vibration[device_index].last_vibration_timepoint) <
                milliseconds(10)) {
            return false;
        }

        controller.vibration[device_index].last_vibration_timepoint = now;
    }

    Core::HID::VibrationValue vibration{vibration_value.amp_low, vibration_value.freq_low,
                                        vibration_value.amp_high, vibration_value.freq_high};
    return controller.device->SetVibration(device_index, vibration);
}

void Controller_NPad::VibrateController(const DeviceHandle& vibration_device_handle,
                                        const VibrationValue& vibration_value) {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return;
    }

    if (!Settings::values.vibration_enabled.GetValue() && !permit_vibration_session_enabled) {
        return;
    }

    const auto npad_index = NPadIdToIndex(vibration_device_handle.npad_id);
    auto& controller = controller_data[npad_index];
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);

    if (!controller.vibration[device_index].device_mounted || !controller.device->IsConnected()) {
        return;
    }

    if (vibration_device_handle.device_index == DeviceIndex::None) {
        UNREACHABLE_MSG("DeviceIndex should never be None!");
        return;
    }

    // Some games try to send mismatched parameters in the device handle, block these.
    if ((controller.device->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconLeft &&
         (vibration_device_handle.npad_type == Core::HID::NpadStyleIndex::JoyconRight ||
          vibration_device_handle.device_index == DeviceIndex::Right)) ||
        (controller.device->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconRight &&
         (vibration_device_handle.npad_type == Core::HID::NpadStyleIndex::JoyconLeft ||
          vibration_device_handle.device_index == DeviceIndex::Left))) {
        return;
    }

    // Filter out vibrations with equivalent values to reduce unnecessary state changes.
    if (vibration_value.amp_low ==
            controller.vibration[device_index].latest_vibration_value.amp_low &&
        vibration_value.amp_high ==
            controller.vibration[device_index].latest_vibration_value.amp_high) {
        return;
    }

    if (VibrateControllerAtIndex(npad_index, device_index, vibration_value)) {
        controller.vibration[device_index].latest_vibration_value = vibration_value;
    }
}

void Controller_NPad::VibrateControllers(const std::vector<DeviceHandle>& vibration_device_handles,
                                         const std::vector<VibrationValue>& vibration_values) {
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

Controller_NPad::VibrationValue Controller_NPad::GetLastVibration(
    const DeviceHandle& vibration_device_handle) const {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return {};
    }

    const auto npad_index = NPadIdToIndex(vibration_device_handle.npad_id);
    const auto& controller = controller_data[npad_index];
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);
    return controller.vibration[device_index].latest_vibration_value;
}

void Controller_NPad::InitializeVibrationDevice(const DeviceHandle& vibration_device_handle) {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return;
    }

    const auto npad_index = NPadIdToIndex(vibration_device_handle.npad_id);
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);
    InitializeVibrationDeviceAtIndex(npad_index, device_index);
}

void Controller_NPad::InitializeVibrationDeviceAtIndex(std::size_t npad_index,
                                                       std::size_t device_index) {
    auto& controller = controller_data[npad_index];
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

bool Controller_NPad::IsVibrationDeviceMounted(const DeviceHandle& vibration_device_handle) const {
    if (!IsDeviceHandleValid(vibration_device_handle)) {
        return false;
    }

    const auto npad_index = NPadIdToIndex(vibration_device_handle.npad_id);
    const auto& controller = controller_data[npad_index];
    const auto device_index = static_cast<std::size_t>(vibration_device_handle.device_index);
    return controller.vibration[device_index].device_mounted;
}

Kernel::KReadableEvent& Controller_NPad::GetStyleSetChangedEvent(u32 npad_id) {
    const auto& controller = controller_data[NPadIdToIndex(npad_id)];
    return controller.styleset_changed_event->GetReadableEvent();
}

void Controller_NPad::SignalStyleSetChangedEvent(u32 npad_id) const {
    const auto& controller = controller_data[NPadIdToIndex(npad_id)];
    controller.styleset_changed_event->GetWritableEvent().Signal();
}

void Controller_NPad::AddNewControllerAt(Core::HID::NpadStyleIndex controller,
                                         std::size_t npad_index) {
    UpdateControllerAt(controller, npad_index, true);
}

void Controller_NPad::UpdateControllerAt(Core::HID::NpadStyleIndex type, std::size_t npad_index,
                                         bool connected) {
    auto& controller = controller_data[npad_index];
    if (!connected) {
        DisconnectNpadAtIndex(npad_index);
        return;
    }

    controller.device->SetNpadStyleIndex(type);
    InitNewlyAddedController(npad_index);
}

void Controller_NPad::DisconnectNpad(u32 npad_id) {
    DisconnectNpadAtIndex(NPadIdToIndex(npad_id));
}

void Controller_NPad::DisconnectNpadAtIndex(std::size_t npad_index) {
    auto& controller = controller_data[npad_index];
    for (std::size_t device_idx = 0; device_idx < controller.vibration.size(); ++device_idx) {
        // Send an empty vibration to stop any vibrations.
        VibrateControllerAtIndex(npad_index, device_idx, {});
        controller.vibration[device_idx].device_mounted = false;
    }

    auto& shared_memory_entry = controller.shared_memory_entry;
    shared_memory_entry.style_set.raw = 0; // Zero out
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
    SignalStyleSetChangedEvent(IndexToNPad(npad_index));
    WriteEmptyEntry(controller.shared_memory_entry);
}

void Controller_NPad::SetGyroscopeZeroDriftMode(GyroscopeZeroDriftMode drift_mode) {
    gyroscope_zero_drift_mode = drift_mode;
}

Controller_NPad::GyroscopeZeroDriftMode Controller_NPad::GetGyroscopeZeroDriftMode() const {
    return gyroscope_zero_drift_mode;
}

bool Controller_NPad::IsSixAxisSensorAtRest() const {
    return sixaxis_at_rest;
}

void Controller_NPad::SetSixAxisEnabled(bool six_axis_status) {
    sixaxis_sensors_enabled = six_axis_status;
}

void Controller_NPad::SetSixAxisFusionParameters(f32 parameter1, f32 parameter2) {
    sixaxis_fusion_parameter1 = parameter1;
    sixaxis_fusion_parameter2 = parameter2;
}

std::pair<f32, f32> Controller_NPad::GetSixAxisFusionParameters() {
    return {
        sixaxis_fusion_parameter1,
        sixaxis_fusion_parameter2,
    };
}

void Controller_NPad::ResetSixAxisFusionParameters() {
    sixaxis_fusion_parameter1 = 0.0f;
    sixaxis_fusion_parameter2 = 0.0f;
}

void Controller_NPad::MergeSingleJoyAsDualJoy(u32 npad_id_1, u32 npad_id_2) {
    const auto npad_index_1 = NPadIdToIndex(npad_id_1);
    const auto npad_index_2 = NPadIdToIndex(npad_id_2);
    const auto& controller_1 = controller_data[npad_index_1].device;
    const auto& controller_2 = controller_data[npad_index_2].device;

    // If the controllers at both npad indices form a pair of left and right joycons, merge them.
    // Otherwise, do nothing.
    if ((controller_1->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconLeft &&
         controller_2->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconRight) ||
        (controller_2->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconLeft &&
         controller_1->GetNpadStyleIndex() == Core::HID::NpadStyleIndex::JoyconRight)) {
        // Disconnect the joycon at the second id and connect the dual joycon at the first index.
        DisconnectNpad(npad_id_2);
        AddNewControllerAt(Core::HID::NpadStyleIndex::JoyconDual, npad_index_1);
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

bool Controller_NPad::SwapNpadAssignment(u32 npad_id_1, u32 npad_id_2) {
    if (npad_id_1 == NPAD_HANDHELD || npad_id_2 == NPAD_HANDHELD || npad_id_1 == NPAD_UNKNOWN ||
        npad_id_2 == NPAD_UNKNOWN) {
        return true;
    }
    const auto npad_index_1 = NPadIdToIndex(npad_id_1);
    const auto npad_index_2 = NPadIdToIndex(npad_id_2);
    const auto& controller_1 = controller_data[npad_index_1].device;
    const auto& controller_2 = controller_data[npad_index_2].device;
    const auto type_index_1 = controller_1->GetNpadStyleIndex();
    const auto type_index_2 = controller_2->GetNpadStyleIndex();

    if (!IsControllerSupported(type_index_1) || !IsControllerSupported(type_index_2)) {
        return false;
    }

    AddNewControllerAt(type_index_2, npad_index_1);
    AddNewControllerAt(type_index_1, npad_index_2);

    return true;
}

Core::HID::LedPattern Controller_NPad::GetLedPattern(u32 npad_id) {
    if (npad_id == npad_id_list.back() || npad_id == npad_id_list[npad_id_list.size() - 2]) {
        // These are controllers without led patterns
        return Core::HID::LedPattern{0, 0, 0, 0};
    }
    return controller_data[npad_id].device->GetLedPattern();
}

bool Controller_NPad::IsUnintendedHomeButtonInputProtectionEnabled(u32 npad_id) const {
    auto& controller = controller_data[NPadIdToIndex(npad_id)];
    return controller.unintended_home_button_input_protection;
}

void Controller_NPad::SetUnintendedHomeButtonInputProtectionEnabled(bool is_protection_enabled,
                                                                    u32 npad_id) {
    auto& controller = controller_data[NPadIdToIndex(npad_id)];
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
                      NPAD_HANDHELD) != supported_npad_id_types.end();
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
                    [](u32 npad_id) { return npad_id <= MAX_NPAD_ID; })) {
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

} // namespace Service::HID
