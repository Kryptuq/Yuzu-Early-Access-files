// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/mii/mii_manager.h"
#include "core/hle/service/nfp/nfp_device.h"
#include "core/hle/service/nfp/nfp_result.h"
#include "core/hle/service/nfp/nfp_user.h"

namespace Service::NFP {

IUser::IUser(Core::System& system_)
    : ServiceFramework{system_, "NFP::IUser"}, service_context{system_, service_name} {
    static const FunctionInfo functions[] = {
        {0, &IUser::Initialize, "Initialize"},
        {1, &IUser::Finalize, "Finalize"},
        {2, &IUser::ListDevices, "ListDevices"},
        {3, &IUser::StartDetection, "StartDetection"},
        {4, &IUser::StopDetection, "StopDetection"},
        {5, &IUser::Mount, "Mount"},
        {6, &IUser::Unmount, "Unmount"},
        {7, &IUser::OpenApplicationArea, "OpenApplicationArea"},
        {8, &IUser::GetApplicationArea, "GetApplicationArea"},
        {9, &IUser::SetApplicationArea, "SetApplicationArea"},
        {10, &IUser::Flush, "Flush"},
        {11, &IUser::Restore, "Restore"},
        {12, &IUser::CreateApplicationArea, "CreateApplicationArea"},
        {13, &IUser::GetTagInfo, "GetTagInfo"},
        {14, &IUser::GetRegisterInfo, "GetRegisterInfo"},
        {15, &IUser::GetCommonInfo, "GetCommonInfo"},
        {16, &IUser::GetModelInfo, "GetModelInfo"},
        {17, &IUser::AttachActivateEvent, "AttachActivateEvent"},
        {18, &IUser::AttachDeactivateEvent, "AttachDeactivateEvent"},
        {19, &IUser::GetState, "GetState"},
        {20, &IUser::GetDeviceState, "GetDeviceState"},
        {21, &IUser::GetNpadId, "GetNpadId"},
        {22, &IUser::GetApplicationAreaSize, "GetApplicationAreaSize"},
        {23, &IUser::AttachAvailabilityChangeEvent, "AttachAvailabilityChangeEvent"},
        {24, &IUser::RecreateApplicationArea, "RecreateApplicationArea"},
    };
    RegisterHandlers(functions);

    availability_change_event = service_context.CreateEvent("IUser:AvailabilityChangeEvent");

    for (u32 device_index = 0; device_index < 10; device_index++) {
        devices[device_index] =
            std::make_shared<NfpDevice>(Core::HID::IndexToNpadIdType(device_index), system,
                                        service_context, availability_change_event);
    }
}

void IUser::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFC, "called");

    state = State::Initialized;

    for (auto& device : devices) {
        device->Initialize();
    }

    IPC::ResponseBuilder rb{ctx, 2, 0};
    rb.Push(ResultSuccess);
}

void IUser::Finalize(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFP, "called");

    state = State::NonInitialized;

    for (auto& device : devices) {
        device->Finalize();
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IUser::ListDevices(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFP, "called");

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    std::vector<u64> nfp_devices;
    const std::size_t max_allowed_devices = ctx.GetWriteBufferSize() / sizeof(u64);

    for (auto& device : devices) {
        if (nfp_devices.size() >= max_allowed_devices) {
            continue;
        }
        if (device->GetCurrentState() != DeviceState::Unavailable) {
            nfp_devices.push_back(device->GetHandle());
        }
    }

    if (nfp_devices.size() == 0) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    ctx.WriteBuffer(nfp_devices);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<s32>(nfp_devices.size()));
}

void IUser::StartDetection(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto nfp_protocol{rp.Pop<s32>()};
    LOG_INFO(Service_NFP, "called, device_handle={}, nfp_protocol={}", device_handle, nfp_protocol);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->StartDetection(nfp_protocol);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::StopDetection(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->StopDetection();
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::Mount(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto model_type{rp.PopEnum<ModelType>()};
    const auto mount_target{rp.PopEnum<MountTarget>()};
    LOG_INFO(Service_NFP, "called, device_handle={}, model_type={}, mount_target={}", device_handle,
             model_type, mount_target);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->Mount(mount_target);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::Unmount(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->Unmount();
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::OpenApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto access_id{rp.Pop<u32>()};
    LOG_INFO(Service_NFP, "called, device_handle={}, access_id={}", device_handle, access_id);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->OpenApplicationArea(access_id);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::GetApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto data_size = ctx.GetWriteBufferSize();
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    std::vector<u8> data(data_size);
    const auto result = device.value()->GetApplicationArea(data);
    ctx.WriteBuffer(data);
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(result);
    rb.Push(static_cast<u32>(data_size));
}

void IUser::SetApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto data{ctx.ReadBuffer()};
    LOG_INFO(Service_NFP, "called, device_handle={}, data_size={}", device_handle, data.size());

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->SetApplicationArea(data);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::Flush(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->Flush();
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::Restore(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->RestoreAmiibo();
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::CreateApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto access_id{rp.Pop<u32>()};
    const auto data{ctx.ReadBuffer()};
    LOG_INFO(Service_NFP, "called, device_handle={}, data_size={}, access_id={}", device_handle,
             access_id, data.size());

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->CreateApplicationArea(access_id, data);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::GetTagInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    TagInfo tag_info{};
    const auto result = device.value()->GetTagInfo(tag_info);
    ctx.WriteBuffer(tag_info);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::GetRegisterInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    RegisterInfo register_info{};
    const auto result = device.value()->GetRegisterInfo(register_info);
    ctx.WriteBuffer(register_info);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::GetCommonInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    CommonInfo common_info{};
    const auto result = device.value()->GetCommonInfo(common_info);
    ctx.WriteBuffer(common_info);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::GetModelInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    ModelInfo model_info{};
    const auto result = device.value()->GetModelInfo(model_info);
    ctx.WriteBuffer(model_info);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IUser::AttachActivateEvent(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(device.value()->GetActivateEvent());
}

void IUser::AttachDeactivateEvent(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(device.value()->GetDeactivateEvent());
}

void IUser::GetState(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFC, "called");

    IPC::ResponseBuilder rb{ctx, 3, 0};
    rb.Push(ResultSuccess);
    rb.PushEnum(state);
}

void IUser::GetDeviceState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(device.value()->GetCurrentState());
}

void IUser::GetNpadId(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(device.value()->GetNpadId());
}

void IUser::GetApplicationAreaSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(device.value()->GetApplicationAreaSize());
}

void IUser::AttachAvailabilityChangeEvent(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFP, "called");

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(availability_change_event->GetReadableEvent());
}

void IUser::RecreateApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto access_id{rp.Pop<u32>()};
    const auto data{ctx.ReadBuffer()};
    LOG_INFO(Service_NFP, "called, device_handle={}, data_size={}, access_id={}", device_handle,
             access_id, data.size());

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(NfcDisabled);
        return;
    }

    auto device = GetNfpDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(DeviceNotFound);
        return;
    }

    const auto result = device.value()->RecreateApplicationArea(access_id, data);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

std::optional<std::shared_ptr<NfpDevice>> IUser::GetNfpDevice(u64 handle) {
    for (auto& device : devices) {
        if (device->GetHandle() == handle) {
            return device;
        }
    }
    return std::nullopt;
}

} // namespace Service::NFP
