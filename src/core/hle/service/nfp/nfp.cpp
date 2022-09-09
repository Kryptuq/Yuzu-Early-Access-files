// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>

#include "common/fs/file.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/mii/mii_manager.h"
#include "core/hle/service/nfp/amiibo_crypto.h"
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/nfp/nfp_user.h"

namespace Service::NFP {
namespace ErrCodes {
constexpr Result DeviceNotFound(ErrorModule::NFP, 64);
constexpr Result WrongDeviceState(ErrorModule::NFP, 73);
constexpr Result NfcDisabled(ErrorModule::NFP, 80);
constexpr Result WriteAmiiboFailed(ErrorModule::NFP, 88);
constexpr Result TagRemoved(ErrorModule::NFP, 97);
constexpr Result ApplicationAreaIsNotInitialized(ErrorModule::NFP, 128);
constexpr Result WrongApplicationAreaId(ErrorModule::NFP, 152);
constexpr Result ApplicationAreaExist(ErrorModule::NFP, 168);
} // namespace ErrCodes

IUser::IUser(Module::Interface& nfp_interface_, Core::System& system_)
    : ServiceFramework{system_, "NFP::IUser"}, service_context{system_, service_name},
      nfp_interface{nfp_interface_} {
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
        {11, nullptr, "Restore"},
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
}

void IUser::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFC, "called");

    state = State::Initialized;

    // TODO(german77): Loop through all interfaces
    nfp_interface.Initialize();

    IPC::ResponseBuilder rb{ctx, 2, 0};
    rb.Push(ResultSuccess);
}

void IUser::Finalize(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFP, "called");

    state = State::NonInitialized;

    // TODO(german77): Loop through all interfaces
    nfp_interface.Finalize();

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IUser::ListDevices(Kernel::HLERequestContext& ctx) {
    LOG_INFO(Service_NFP, "called");

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    std::vector<u64> devices;

    // TODO(german77): Loop through all interfaces
    devices.push_back(nfp_interface.GetHandle());

    if (devices.size() == 0) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::DeviceNotFound);
        return;
    }

    ctx.WriteBuffer(devices);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<s32>(devices.size()));
}

void IUser::StartDetection(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto nfp_protocol{rp.Pop<s32>()};
    LOG_INFO(Service_NFP, "called, device_handle={}, nfp_protocol={}", device_handle, nfp_protocol);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.StartDetection(nfp_protocol);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::StopDetection(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.StopDetection();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
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
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.Mount();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::Unmount(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.Unmount();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::OpenApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto access_id{rp.Pop<u32>()};
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}, access_id={}", device_handle,
                access_id);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.OpenApplicationArea(access_id);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        ApplicationArea data{};
        const auto result = nfp_interface.GetApplicationArea(data);
        ctx.WriteBuffer(data);
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(result);
        rb.Push(static_cast<u32>(data.size()));
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::SetApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto data{ctx.ReadBuffer()};
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}, data_size={}", device_handle,
                data.size());

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.SetApplicationArea(data);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::Flush(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.Flush();
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::CreateApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto access_id{rp.Pop<u32>()};
    const auto data{ctx.ReadBuffer()};
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}, data_size={}, access_id={}",
                device_handle, access_id, data.size());

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.CreateApplicationArea(access_id, data);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetTagInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        TagInfo tag_info{};
        const auto result = nfp_interface.GetTagInfo(tag_info);
        ctx.WriteBuffer(tag_info);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetRegisterInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        RegisterInfo register_info{};
        const auto result = nfp_interface.GetRegisterInfo(register_info);
        ctx.WriteBuffer(register_info);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetCommonInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        CommonInfo common_info{};
        const auto result = nfp_interface.GetCommonInfo(common_info);
        ctx.WriteBuffer(common_info);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetModelInfo(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_INFO(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        ModelInfo model_info{};
        const auto result = nfp_interface.GetModelInfo(model_info);
        ctx.WriteBuffer(model_info);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::AttachActivateEvent(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(nfp_interface.GetActivateEvent());
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::AttachDeactivateEvent(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(nfp_interface.GetDeactivateEvent());
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
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

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(nfp_interface.GetCurrentState());
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetNpadId(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(nfp_interface.GetNpadId());
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::GetApplicationAreaSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    LOG_DEBUG(Service_NFP, "called, device_handle={}", device_handle);

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(sizeof(ApplicationArea));
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::AttachAvailabilityChangeEvent(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFP, "(STUBBED) called");

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
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
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}, data_size={}, access_id={}",
                device_handle, access_id, data.size());

    if (state == State::NonInitialized) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::NfcDisabled);
        return;
    }

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        const auto result = nfp_interface.RecreateApplicationArea(access_id, data);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

Module::Interface::Interface(std::shared_ptr<Module> module_, Core::System& system_,
                             const char* name)
    : ServiceFramework{system_, name}, module{std::move(module_)},
      npad_id{Core::HID::NpadIdType::Player1}, service_context{system_, service_name} {
    activate_event = service_context.CreateEvent("IUser:NFPActivateEvent");
    deactivate_event = service_context.CreateEvent("IUser:NFPDeactivateEvent");
}

Module::Interface::~Interface() = default;

void Module::Interface::CreateUserInterface(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFP, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IUser>(*this, system);
}

bool Module::Interface::LoadAmiiboFile(const std::string& filename) {
    constexpr auto tag_size_without_password = sizeof(NTAG215File) - sizeof(NTAG215Password);
    const Common::FS::IOFile amiibo_file{filename, Common::FS::FileAccessMode::Read,
                                         Common::FS::FileType::BinaryFile};

    if (!amiibo_file.IsOpen()) {
        LOG_ERROR(Service_NFP, "Amiibo is already on use");
        return false;
    }

    // Workaround for files with missing password data
    std::array<u8, sizeof(EncryptedNTAG215File)> buffer{};
    if (amiibo_file.Read(buffer) < tag_size_without_password) {
        LOG_ERROR(Service_NFP, "Failed to read amiibo file");
        return false;
    }
    memcpy(&encrypted_tag_data, buffer.data(), sizeof(EncryptedNTAG215File));

    if (!AmiiboCrypto::IsAmiiboValid(encrypted_tag_data)) {
        LOG_INFO(Service_NFP, "Invalid amiibo");
        return false;
    }

    file_path = filename;
    return true;
}

bool Module::Interface::LoadAmiibo(const std::string& filename) {
    if (device_state != DeviceState::SearchingForTag) {
        LOG_ERROR(Service_NFP, "Game is not looking for amiibos, current state {}", device_state);
        return false;
    }

    if (!LoadAmiiboFile(filename)) {
        return false;
    }

    device_state = DeviceState::TagFound;
    activate_event->GetWritableEvent().Signal();
    return true;
}

void Module::Interface::CloseAmiibo() {
    LOG_INFO(Service_NFP, "Remove amiibo");
    device_state = DeviceState::TagRemoved;
    is_data_decoded = false;
    is_application_area_initialized = false;
    encrypted_tag_data = {};
    tag_data = {};
    deactivate_event->GetWritableEvent().Signal();
}

Kernel::KReadableEvent& Module::Interface::GetActivateEvent() const {
    return activate_event->GetReadableEvent();
}

Kernel::KReadableEvent& Module::Interface::GetDeactivateEvent() const {
    return deactivate_event->GetReadableEvent();
}

void Module::Interface::Initialize() {
    device_state = DeviceState::Initialized;
    is_data_decoded = false;
    is_application_area_initialized = false;
    encrypted_tag_data = {};
    tag_data = {};
}

void Module::Interface::Finalize() {
    if (device_state == DeviceState::TagMounted) {
        Unmount();
    }
    if (device_state == DeviceState::SearchingForTag || device_state == DeviceState::TagRemoved) {
        StopDetection();
    }
    device_state = DeviceState::Unaviable;
}

Result Module::Interface::StartDetection(s32 protocol_) {
    auto npad_device = system.HIDCore().GetEmulatedController(npad_id);

    // TODO(german77): Add callback for when nfc data is available

    if (device_state == DeviceState::Initialized || device_state == DeviceState::TagRemoved) {
        npad_device->SetPollingMode(Common::Input::PollingMode::NFC);
        device_state = DeviceState::SearchingForTag;
        protocol = protocol_;
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
    return ErrCodes::WrongDeviceState;
}

Result Module::Interface::StopDetection() {
    auto npad_device = system.HIDCore().GetEmulatedController(npad_id);
    npad_device->SetPollingMode(Common::Input::PollingMode::Active);

    if (device_state == DeviceState::TagFound || device_state == DeviceState::TagMounted) {
        CloseAmiibo();
        return ResultSuccess;
    }
    if (device_state == DeviceState::SearchingForTag || device_state == DeviceState::TagRemoved) {
        device_state = DeviceState::Initialized;
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
    return ErrCodes::WrongDeviceState;
}

Result Module::Interface::Flush() {
    // Ignore write command if we can't encrypt the data
    if (!is_data_decoded) {
        return ResultSuccess;
    }

    constexpr auto tag_size_without_password = sizeof(NTAG215File) - sizeof(NTAG215Password);
    EncryptedNTAG215File tmp_encrypted_tag_data{};
    const Common::FS::IOFile amiibo_file{file_path, Common::FS::FileAccessMode::ReadWrite,
                                         Common::FS::FileType::BinaryFile};

    if (!amiibo_file.IsOpen()) {
        LOG_ERROR(Core, "Amiibo is already on use");
        return ErrCodes::WriteAmiiboFailed;
    }

    // Workaround for files with missing password data
    std::array<u8, sizeof(EncryptedNTAG215File)> buffer{};
    if (amiibo_file.Read(buffer) < tag_size_without_password) {
        LOG_ERROR(Core, "Failed to read amiibo file");
        return ErrCodes::WriteAmiiboFailed;
    }
    memcpy(&tmp_encrypted_tag_data, buffer.data(), sizeof(EncryptedNTAG215File));

    if (!AmiiboCrypto::IsAmiiboValid(tmp_encrypted_tag_data)) {
        LOG_INFO(Service_NFP, "Invalid amiibo");
        return ErrCodes::WriteAmiiboFailed;
    }

    bool is_uuid_equal = memcmp(tmp_encrypted_tag_data.uuid.data(), tag_data.uuid.data(), 8) == 0;
    bool is_character_equal = tmp_encrypted_tag_data.user_memory.model_info.character_id ==
                              tag_data.model_info.character_id;
    if (!is_uuid_equal || !is_character_equal) {
        LOG_ERROR(Service_NFP, "Not the same amiibo");
        return ErrCodes::WriteAmiiboFailed;
    }

    if (!AmiiboCrypto::EncodeAmiibo(tag_data, encrypted_tag_data)) {
        LOG_ERROR(Service_NFP, "Failed to encode data");
        return ErrCodes::WriteAmiiboFailed;
    }

    // Return to the start of the file
    if (!amiibo_file.Seek(0)) {
        LOG_ERROR(Service_NFP, "Error writting to file");
        return ErrCodes::WriteAmiiboFailed;
    }

    if (!amiibo_file.Write(encrypted_tag_data)) {
        LOG_ERROR(Service_NFP, "Error writting to file");
        return ErrCodes::WriteAmiiboFailed;
    }

    return ResultSuccess;
}

Result Module::Interface::Mount() {
    if (device_state != DeviceState::TagFound) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    is_data_decoded = AmiiboCrypto::DecodeAmiibo(encrypted_tag_data, tag_data);
    LOG_INFO(Service_NFP, "Is amiibo decoded {}", is_data_decoded);

    is_application_area_initialized = false;
    device_state = DeviceState::TagMounted;
    return ResultSuccess;
}

Result Module::Interface::Unmount() {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    is_data_decoded = false;
    is_application_area_initialized = false;
    device_state = DeviceState::TagFound;
    return ResultSuccess;
}

Result Module::Interface::GetTagInfo(TagInfo& tag_info) const {
    if (device_state != DeviceState::TagFound && device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    tag_info = {
        .uuid = encrypted_tag_data.uuid,
        .uuid_length = static_cast<u8>(encrypted_tag_data.uuid.size()),
        .protocol = protocol,
        .tag_type = static_cast<u32>(encrypted_tag_data.user_memory.model_info.amiibo_type),
    };

    return ResultSuccess;
}

Result Module::Interface::GetCommonInfo(CommonInfo& common_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    if (is_data_decoded && tag_data.settings.settings.amiibo_initialized != 0) {
        const auto& settings = tag_data.settings;
        // TODO: Validate this data
        common_info = {
            .last_write_year = settings.write_date.GetYear(),
            .last_write_month = settings.write_date.GetMonth(),
            .last_write_day = settings.write_date.GetDay(),
            .write_counter = settings.crc_counter,
            .version = 1,
            .application_area_size = sizeof(ApplicationArea),
        };
        return ResultSuccess;
    }

    // Generate a generic answer
    common_info = {
        .last_write_year = 2022,
        .last_write_month = 2,
        .last_write_day = 7,
        .write_counter = 0,
        .version = 1,
        .application_area_size = sizeof(ApplicationArea),
    };
    return ResultSuccess;
}

Result Module::Interface::GetModelInfo(ModelInfo& model_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    const auto& model_info_data = encrypted_tag_data.user_memory.model_info;
    model_info = {
        .character_id = model_info_data.character_id,
        .character_variant = model_info_data.character_variant,
        .amiibo_type = model_info_data.amiibo_type,
        .model_number = model_info_data.model_number,
        .series = model_info_data.series,
        .constant_value = model_info_data.constant_value,
    };
    return ResultSuccess;
}

Result Module::Interface::GetRegisterInfo(RegisterInfo& register_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return ErrCodes::TagRemoved;
        }
        return ErrCodes::WrongDeviceState;
    }

    Service::Mii::MiiManager manager;

    if (is_data_decoded && tag_data.settings.settings.amiibo_initialized != 0) {
        const auto& settings = tag_data.settings;

        // TODO: Validate this data
        register_info = {
            .mii_char_info = manager.ConvertV3ToCharInfo(tag_data.owner_mii),
            .first_write_year = settings.init_date.GetYear(),
            .first_write_month = settings.init_date.GetMonth(),
            .first_write_day = settings.init_date.GetDay(),
            .amiibo_name = GetAmiiboName(settings),
            .font_region = {},
        };

        return ResultSuccess;
    }

    // Generate a generic answer
    register_info = {
        .mii_char_info = manager.BuildDefault(0),
        .first_write_year = 2022,
        .first_write_month = 2,
        .first_write_day = 7,
        .amiibo_name = {'Y', 'u', 'z', 'u', 'A', 'm', 'i', 'i', 'b', 'o', 0},
        .font_region = {},
    };
    return ResultSuccess;
}

Result Module::Interface::OpenApplicationArea(u32 access_id) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return ErrCodes::TagRemoved;
        }
        return ErrCodes::WrongDeviceState;
    }

    // Fallback for lack of amiibo keys
    if (!is_data_decoded) {
        LOG_WARNING(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }

    if (tag_data.settings.settings.appdata_initialized == 0) {
        LOG_WARNING(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }

    if (tag_data.application_area_id != access_id) {
        LOG_WARNING(Service_NFP, "Wrong application area id");
        return ErrCodes::WrongApplicationAreaId;
    }

    is_application_area_initialized = true;
    return ResultSuccess;
}

Result Module::Interface::GetApplicationArea(ApplicationArea& data) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return ErrCodes::TagRemoved;
        }
        return ErrCodes::WrongDeviceState;
    }

    if (!is_application_area_initialized) {
        LOG_ERROR(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }

    data = tag_data.application_area;

    return ResultSuccess;
}

Result Module::Interface::SetApplicationArea(const std::vector<u8>& data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return ErrCodes::TagRemoved;
        }
        return ErrCodes::WrongDeviceState;
    }

    if (!is_application_area_initialized) {
        LOG_ERROR(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }

    if (data.size() != sizeof(ApplicationArea)) {
        LOG_ERROR(Service_NFP, "Wrong data size {}", data.size());
        return ResultUnknown;
    }

    std::memcpy(&tag_data.application_area, data.data(), sizeof(ApplicationArea));
    return ResultSuccess;
}

Result Module::Interface::CreateApplicationArea(u32 access_id, const std::vector<u8>& data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return ErrCodes::TagRemoved;
        }
        return ErrCodes::WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized != 0) {
        LOG_ERROR(Service_NFP, "Application area already exist");
        return ErrCodes::ApplicationAreaExist;
    }

    if (data.size() != sizeof(ApplicationArea)) {
        LOG_ERROR(Service_NFP, "Wrong data size {}", data.size());
        return ResultUnknown;
    }

    std::memcpy(&tag_data.application_area, data.data(), sizeof(ApplicationArea));
    tag_data.application_area_id = access_id;

    return ResultSuccess;
}

Result Module::Interface::RecreateApplicationArea(u32 access_id, const std::vector<u8>& data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return ErrCodes::TagRemoved;
        }
        return ErrCodes::WrongDeviceState;
    }

    if (data.size() != sizeof(ApplicationArea)) {
        LOG_ERROR(Service_NFP, "Wrong data size {}", data.size());
        return ResultUnknown;
    }

    std::memcpy(&tag_data.application_area, data.data(), sizeof(ApplicationArea));
    tag_data.application_area_id = access_id;

    return ResultSuccess;
}

u64 Module::Interface::GetHandle() const {
    // Generate a handle based of the npad id
    return static_cast<u64>(npad_id);
}

DeviceState Module::Interface::GetCurrentState() const {
    return device_state;
}

Core::HID::NpadIdType Module::Interface::GetNpadId() const {
    // Return first connected npad id as a workaround for lack of a single nfc interface per
    // controller
    return system.HIDCore().GetFirstNpadId();
}

AmiiboName Module::Interface::GetAmiiboName(const AmiiboSettings& settings) const {
    std::array<char16_t, amiibo_name_length> settings_amiibo_name{};
    AmiiboName amiibo_name{};

    // Convert from big endian to little endian
    for (std::size_t i = 0; i < amiibo_name_length; i++) {
        settings_amiibo_name[i] = static_cast<u16>(settings.amiibo_name[i]);
    }

    // Convert from utf16 to utf8
    const auto amiibo_name_utf8 = Common::UTF16ToUTF8(settings_amiibo_name.data());
    memcpy(amiibo_name.data(), amiibo_name_utf8.data(), amiibo_name_utf8.size());

    return amiibo_name;
}

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    auto module = std::make_shared<Module>();
    std::make_shared<NFP_User>(module, system)->InstallAsService(service_manager);
}

} // namespace Service::NFP
