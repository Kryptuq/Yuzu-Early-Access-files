// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hid/hid_types.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/nfc/mifare_user.h"
#include "core/hle/service/nfc/nfc_device.h"
#include "core/hle/service/nfc/nfc_result.h"

namespace Service::NFC {

MFIUser::MFIUser(Core::System& system_)
    : ServiceFramework{system_, "NFC::MFIUser"}, service_context{system_, service_name} {
    static const FunctionInfo functions[] = {
        {0, &MFIUser::Initialize, "Initialize"},
        {1, &MFIUser::Finalize, "Finalize"},
        {2, &MFIUser::ListDevices, "ListDevices"},
        {3, &MFIUser::StartDetection, "StartDetection"},
        {4, &MFIUser::StopDetection, "StopDetection"},
        {5, &MFIUser::Read, "Read"},
        {6, &MFIUser::Write, "Write"},
        {7, &MFIUser::GetTagInfo, "GetTagInfo"},
        {8, &MFIUser::GetActivateEventHandle, "GetActivateEventHandle"},
        {9, &MFIUser::GetDeactivateEventHandle, "GetDeactivateEventHandle"},
        {10, &MFIUser::GetState, "GetState"},
        {11, &MFIUser::GetDeviceState, "GetDeviceState"},
        {12, &MFIUser::GetNpadId, "GetNpadId"},
        {13, &MFIUser::GetAvailabilityChangeEventHandle, "GetAvailabilityChangeEventHandle"},
    };
    RegisterHandlers(functions);

    availability_change_event = service_context.CreateEvent("MFIUser:AvailabilityChangeEvent");

    for (u32 device_index = 0; device_index < 10; device_index++) {
        devices[device_index] =
            std::make_shared<NfcDevice>(Core::HID::IndexToNpadIdType(device_index), system,
                                        service_context, availability_change_event);
    }
}

MFIUser ::~MFIUser() {
    availability_change_event->Close();
}

void MFIUser::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFC, "called");

    state = State::Initialized;

    for (auto& device : devices) {
        device->Initialize();
    }

    IPC::ResponseBuilder rb{ctx, 2, 0};
    rb.Push(ResultSuccess);
}

void MFIUser::Finalize(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFC, "called");

    state = State::NonInitialized;

    for (auto& device : devices) {
        device->Finalize();
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void MFIUser::ListDevices(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFC, "called");

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    if (!ctx.CanWriteBuffer()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareInvalidArgument);
        return;
    }

    if (ctx.GetWriteBufferSize() == 0) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareInvalidArgument);
        return;
    }

    std::vector<u64> nfp_devices;
    const std::size_t max_allowed_devices = ctx.GetWriteBufferNumElements<u64>();

    for (const auto& device : devices) {
        if (nfp_devices.size() >= max_allowed_devices) {
            continue;
        }
        if (device->GetCurrentState() != NFP::DeviceState::Unavailable) {
            nfp_devices.push_back(device->GetHandle());
        }
    }

    if (nfp_devices.empty()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    ctx.WriteBuffer(nfp_devices);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<s32>(nfp_devices.size()));
}

void MFIUser::StartDetection(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFC, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    const auto result = device.value()->StartDetection(NFP::TagProtocol::All);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void MFIUser::StopDetection(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFC, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    const auto result = device.value()->StopDetection();
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void MFIUser::Read(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto buffer{ctx.ReadBuffer()};
    const auto number_of_commands{ctx.GetReadBufferNumElements<NFP::MifareReadBlockParameter>()};
    std::vector<NFP::MifareReadBlockParameter> read_commands(number_of_commands);

    memcpy(read_commands.data(), buffer.data(),
           number_of_commands * sizeof(NFP::MifareReadBlockParameter));

    LOG_INFO(Service_NFC, "(STUBBED) called, device_handle={}, read_commands_size={}",
             device_handle, number_of_commands);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    Result result = ResultSuccess;
    std::vector<NFP::MifareReadBlockData> out_data(number_of_commands);
    for (std::size_t i = 0; i < number_of_commands; i++) {
        result = device.value()->MifareRead(read_commands[i], out_data[i]);
        if (result.IsError()) {
            break;
        }
    }

    ctx.WriteBuffer(out_data);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void MFIUser::Write(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto buffer{ctx.ReadBuffer()};
    const auto number_of_commands{ctx.GetReadBufferNumElements<NFP::MifareWriteBlockParameter>()};
    std::vector<NFP::MifareWriteBlockParameter> write_commands(number_of_commands);

    memcpy(write_commands.data(), buffer.data(),
           number_of_commands * sizeof(NFP::MifareWriteBlockParameter));

    LOG_INFO(Service_NFC, "(STUBBED) called, device_handle={}, write_commands_size={}",
             device_handle, number_of_commands);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    Result result = ResultSuccess;
    std::vector<NFP::MifareReadBlockData> out_data(number_of_commands);
    for (std::size_t i = 0; i < number_of_commands; i++) {
        result = device.value()->MifareWrite(write_commands[i]);
        if (result.IsError()) {
            break;
        }
    }

    if (result.IsSuccess()) {
        result = device.value()->Flush();
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void MFIUser::GetTagInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFC, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    NFP::TagInfo tag_info{};
    const auto result = device.value()->GetTagInfo(tag_info, true);
    ctx.WriteBuffer(tag_info);
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void MFIUser::GetActivateEventHandle(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFC, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(device.value()->GetActivateEvent());
}

void MFIUser::GetDeactivateEventHandle(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFC, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(device.value()->GetDeactivateEvent());
}

void MFIUser::GetState(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFC, "called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(state);
}

void MFIUser::GetDeviceState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFC, "called, device_handle={}", device_handle);

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(device.value()->GetCurrentState());
}

void MFIUser::GetNpadId(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFC, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    auto device = GetNfcDevice(device_handle);

    if (!device.has_value()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareDeviceNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(device.value()->GetNpadId());
}

void MFIUser::GetAvailabilityChangeEventHandle(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFC, "called");

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(MifareNfcDisabled);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(availability_change_event->GetReadableEvent());
}

std::optional<std::shared_ptr<NfcDevice>> MFIUser::GetNfcDevice(u64 handle) {
    for (auto& device : devices) {
        if (device->GetHandle() == handle) {
            return device;
        }
    }
    return std::nullopt;
}

} // namespace Service::NFC
