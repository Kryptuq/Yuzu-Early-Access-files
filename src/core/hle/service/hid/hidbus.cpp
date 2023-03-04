// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/core_timing_util.h"
#include "core/hid/hid_types.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/service/hid/hidbus.h"
#include "core/hle/service/hid/hidbus/ringcon.h"
#include "core/hle/service/hid/hidbus/starlink.h"
#include "core/hle/service/hid/hidbus/stubbed.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/service.h"
#include "core/memory.h"

namespace Service::HID {
// (15ms, 66Hz)
constexpr auto hidbus_update_ns = std::chrono::nanoseconds{15 * 1000 * 1000};

HidBus::HidBus(Core::System& system_)
    : ServiceFramework{system_, "hidbus"}, service_context{system_, service_name} {

    // clang-format off
    static const FunctionInfo functions[] = {
            {1, &HidBus::GetBusHandle, "GetBusHandle"},
            {2, &HidBus::IsExternalDeviceConnected, "IsExternalDeviceConnected"},
            {3, &HidBus::Initialize, "Initialize"},
            {4, &HidBus::Finalize, "Finalize"},
            {5, &HidBus::EnableExternalDevice, "EnableExternalDevice"},
            {6, &HidBus::GetExternalDeviceId, "GetExternalDeviceId"},
            {7, &HidBus::SendCommandAsync, "SendCommandAsync"},
            {8, &HidBus::GetSendCommandAsynceResult, "GetSendCommandAsynceResult"},
            {9, &HidBus::SetEventForSendCommandAsycResult, "SetEventForSendCommandAsycResult"},
            {10, &HidBus::GetSharedMemoryHandle, "GetSharedMemoryHandle"},
            {11, &HidBus::EnableJoyPollingReceiveMode, "EnableJoyPollingReceiveMode"},
            {12, &HidBus::DisableJoyPollingReceiveMode, "DisableJoyPollingReceiveMode"},
            {13, nullptr, "GetPollingData"},
            {14, &HidBus::SetStatusManagerType, "SetStatusManagerType"},
    };
    // clang-format on

    RegisterHandlers(functions);

    // Register update callbacks
    hidbus_update_event = Core::Timing::CreateEvent(
        "Hidbus::UpdateCallback",
        [this](std::uintptr_t user_data, s64 time,
               std::chrono::nanoseconds ns_late) -> std::optional<std::chrono::nanoseconds> {
            const auto guard = LockService();
            UpdateHidbus(user_data, ns_late);
            return std::nullopt;
        });

    system_.CoreTiming().ScheduleLoopingEvent(hidbus_update_ns, hidbus_update_ns,
                                              hidbus_update_event);
}

HidBus::~HidBus() {
    system.CoreTiming().UnscheduleEvent(hidbus_update_event, 0);
}

void HidBus::UpdateHidbus(std::uintptr_t user_data, std::chrono::nanoseconds ns_late) {
    if (is_hidbus_enabled) {
        for (std::size_t i = 0; i < devices.size(); ++i) {
            if (!devices[i].is_device_initializated) {
                continue;
            }
            auto& device = devices[i].device;
            device->OnUpdate();
            auto& cur_entry = hidbus_status.entries[devices[i].handle.internal_index];
            cur_entry.is_polling_mode = device->IsPollingMode();
            cur_entry.polling_mode = device->GetPollingMode();
            cur_entry.is_enabled = device->IsEnabled();

            u8* shared_memory = system.Kernel().GetHidBusSharedMem().GetPointer();
            std::memcpy(shared_memory + (i * sizeof(HidbusStatusManagerEntry)), &hidbus_status,
                        sizeof(HidbusStatusManagerEntry));
        }
    }
}

std::optional<std::size_t> HidBus::GetDeviceIndexFromHandle(BusHandle handle) const {
    for (std::size_t i = 0; i < devices.size(); ++i) {
        const auto& device_handle = devices[i].handle;
        if (handle.abstracted_pad_id == device_handle.abstracted_pad_id &&
            handle.internal_index == device_handle.internal_index &&
            handle.player_number == device_handle.player_number &&
            handle.bus_type_id == device_handle.bus_type_id &&
            handle.is_valid == device_handle.is_valid) {
            return i;
        }
    }
    return std::nullopt;
}

void HidBus::GetBusHandle(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::HID::NpadIdType npad_id;
        INSERT_PADDING_WORDS_NOINIT(1);
        BusType bus_type;
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_INFO(Service_HID, "called, npad_id={}, bus_type={}, applet_resource_user_id={}",
             parameters.npad_id, parameters.bus_type, parameters.applet_resource_user_id);

    bool is_handle_found = 0;
    std::size_t handle_index = 0;

    for (std::size_t i = 0; i < devices.size(); i++) {
        const auto& handle = devices[i].handle;
        if (!handle.is_valid) {
            continue;
        }
        if (static_cast<Core::HID::NpadIdType>(handle.player_number) == parameters.npad_id &&
            handle.bus_type_id == static_cast<u8>(parameters.bus_type)) {
            is_handle_found = true;
            handle_index = i;
            break;
        }
    }

    // Handle not found. Create a new one
    if (!is_handle_found) {
        for (std::size_t i = 0; i < devices.size(); i++) {
            if (devices[i].handle.is_valid) {
                continue;
            }
            devices[i].handle = {
                .abstracted_pad_id = static_cast<u8>(i),
                .internal_index = static_cast<u8>(i),
                .player_number = static_cast<u8>(parameters.npad_id),
                .bus_type_id = static_cast<u8>(parameters.bus_type),
                .is_valid = true,
            };
            handle_index = i;
            break;
        }
    }

    struct OutData {
        bool is_valid;
        INSERT_PADDING_BYTES(7);
        BusHandle handle;
    };
    static_assert(sizeof(OutData) == 0x10, "OutData has incorrect size.");

    const OutData out_data{
        .is_valid = true,
        .handle = devices[handle_index].handle,
    };

    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.PushRaw(out_data);
}

void HidBus::IsExternalDeviceConnected(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    LOG_INFO(Service_HID,
             "Called, abstracted_pad_id={}, bus_type={}, internal_index={}, "
             "player_number={}, is_valid={}",
             bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
             bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        const auto& device = devices[device_index.value()].device;
        const bool is_attached = device->IsDeviceActivated();

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(is_attached);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::Initialize(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_INFO(Service_HID,
             "called, abstracted_pad_id={} bus_type={} internal_index={} "
             "player_number={} is_valid={}, applet_resource_user_id={}",
             bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
             bus_handle_.player_number, bus_handle_.is_valid, applet_resource_user_id);

    is_hidbus_enabled = true;

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        const auto entry_index = devices[device_index.value()].handle.internal_index;
        auto& cur_entry = hidbus_status.entries[entry_index];

        if (bus_handle_.internal_index == 0 && Settings::values.enable_ring_controller) {
            MakeDevice<RingController>(bus_handle_);
            devices[device_index.value()].is_device_initializated = true;
            devices[device_index.value()].device->ActivateDevice();
            cur_entry.is_in_focus = true;
            cur_entry.is_connected = true;
            cur_entry.is_connected_result = ResultSuccess;
            cur_entry.is_enabled = false;
            cur_entry.is_polling_mode = false;
        } else {
            MakeDevice<HidbusStubbed>(bus_handle_);
            devices[device_index.value()].is_device_initializated = true;
            cur_entry.is_in_focus = true;
            cur_entry.is_connected = false;
            cur_entry.is_connected_result = ResultSuccess;
            cur_entry.is_enabled = false;
            cur_entry.is_polling_mode = false;
        }

        std::memcpy(system.Kernel().GetHidBusSharedMem().GetPointer(), &hidbus_status,
                    sizeof(hidbus_status));

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::Finalize(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_INFO(Service_HID,
             "called, abstracted_pad_id={}, bus_type={}, internal_index={}, "
             "player_number={}, is_valid={}, applet_resource_user_id={}",
             bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
             bus_handle_.player_number, bus_handle_.is_valid, applet_resource_user_id);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        const auto entry_index = devices[device_index.value()].handle.internal_index;
        auto& cur_entry = hidbus_status.entries[entry_index];
        auto& device = devices[device_index.value()].device;
        devices[device_index.value()].is_device_initializated = false;
        device->DeactivateDevice();

        cur_entry.is_in_focus = true;
        cur_entry.is_connected = false;
        cur_entry.is_connected_result = ResultSuccess;
        cur_entry.is_enabled = false;
        cur_entry.is_polling_mode = false;
        std::memcpy(system.Kernel().GetHidBusSharedMem().GetPointer(), &hidbus_status,
                    sizeof(hidbus_status));

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::EnableExternalDevice(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        bool enable;
        INSERT_PADDING_BYTES_NOINIT(7);
        BusHandle bus_handle;
        u64 inval;
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x20, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_DEBUG(Service_HID,
              "called, enable={}, abstracted_pad_id={}, bus_type={}, internal_index={}, "
              "player_number={}, is_valid={}, inval={}, applet_resource_user_id{}",
              parameters.enable, parameters.bus_handle.abstracted_pad_id,
              parameters.bus_handle.bus_type_id, parameters.bus_handle.internal_index,
              parameters.bus_handle.player_number, parameters.bus_handle.is_valid, parameters.inval,
              parameters.applet_resource_user_id);

    const auto device_index = GetDeviceIndexFromHandle(parameters.bus_handle);

    if (device_index) {
        auto& device = devices[device_index.value()].device;
        device->Enable(parameters.enable);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::GetExternalDeviceId(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    LOG_DEBUG(Service_HID,
              "called, abstracted_pad_id={}, bus_type={}, internal_index={}, player_number={}, "
              "is_valid={}",
              bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
              bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        const auto& device = devices[device_index.value()].device;
        u32 device_id = device->GetDeviceId();
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(device_id);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::SendCommandAsync(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto data = ctx.ReadBuffer();
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    LOG_DEBUG(Service_HID,
              "called, data_size={}, abstracted_pad_id={}, bus_type={}, internal_index={}, "
              "player_number={}, is_valid={}",
              data.size(), bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id,
              bus_handle_.internal_index, bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        auto& device = devices[device_index.value()].device;
        device->SetCommand(data);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
};

void HidBus::GetSendCommandAsynceResult(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    LOG_DEBUG(Service_HID,
              "called, abstracted_pad_id={}, bus_type={}, internal_index={}, player_number={}, "
              "is_valid={}",
              bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
              bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        const auto& device = devices[device_index.value()].device;
        const std::vector<u8> data = device->GetReply();
        const u64 data_size = ctx.WriteBuffer(data);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u64>(data_size);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
};

void HidBus::SetEventForSendCommandAsycResult(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    LOG_INFO(Service_HID,
             "called, abstracted_pad_id={}, bus_type={}, internal_index={}, player_number={}, "
             "is_valid={}",
             bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
             bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        const auto& device = devices[device_index.value()].device;
        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(device->GetSendCommandAsycEvent());
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
};

void HidBus::GetSharedMemoryHandle(HLERequestContext& ctx) {
    LOG_DEBUG(Service_HID, "called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(&system.Kernel().GetHidBusSharedMem());
}

void HidBus::EnableJoyPollingReceiveMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto t_mem_size{rp.Pop<u32>()};
    const auto t_mem_handle{ctx.GetCopyHandle(0)};
    const auto polling_mode_{rp.PopEnum<JoyPollingMode>()};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    ASSERT_MSG(t_mem_size == 0x1000, "t_mem_size is not 0x1000 bytes");

    auto t_mem = system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
        t_mem_handle);

    if (t_mem.IsNull()) {
        LOG_ERROR(Service_HID, "t_mem is a nullptr for handle=0x{:08X}", t_mem_handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    ASSERT_MSG(t_mem->GetSize() == 0x1000, "t_mem has incorrect size");

    LOG_INFO(Service_HID,
             "called, t_mem_handle=0x{:08X}, polling_mode={}, abstracted_pad_id={}, bus_type={}, "
             "internal_index={}, player_number={}, is_valid={}",
             t_mem_handle, polling_mode_, bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id,
             bus_handle_.internal_index, bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        auto& device = devices[device_index.value()].device;
        device->SetPollingMode(polling_mode_);
        device->SetTransferMemoryAddress(t_mem->GetSourceAddress());

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::DisableJoyPollingReceiveMode(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto bus_handle_{rp.PopRaw<BusHandle>()};

    LOG_INFO(Service_HID,
             "called, abstracted_pad_id={}, bus_type={}, internal_index={}, player_number={}, "
             "is_valid={}",
             bus_handle_.abstracted_pad_id, bus_handle_.bus_type_id, bus_handle_.internal_index,
             bus_handle_.player_number, bus_handle_.is_valid);

    const auto device_index = GetDeviceIndexFromHandle(bus_handle_);

    if (device_index) {
        auto& device = devices[device_index.value()].device;
        device->DisablePollingMode();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
        return;
    }

    LOG_ERROR(Service_HID, "Invalid handle");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultUnknown);
    return;
}

void HidBus::SetStatusManagerType(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto manager_type{rp.PopEnum<StatusManagerType>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, manager_type={}", manager_type);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
};
} // namespace Service::HID
