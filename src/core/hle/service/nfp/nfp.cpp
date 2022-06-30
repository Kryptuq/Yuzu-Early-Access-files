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
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/nfp/nfp_user.h"

namespace Service::NFP {
namespace ErrCodes {
constexpr Result DeviceNotFound(ErrorModule::NFP, 64);
constexpr Result WrongDeviceState(ErrorModule::NFP, 73);
constexpr Result ApplicationAreaIsNotInitialized(ErrorModule::NFP, 128);
constexpr Result ApplicationAreaExist(ErrorModule::NFP, 168);
} // namespace ErrCodes

constexpr u32 ApplicationAreaSize = 0xD8;

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
        {10, nullptr, "Flush"},
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
        {24, nullptr, "RecreateApplicationArea"},
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

    std::vector<u64> devices;

    // TODO(german77): Loop through all interfaces
    devices.push_back(nfp_interface.GetHandle());

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

    // TODO(german77): Loop through all interfaces
    if (device_handle == nfp_interface.GetHandle()) {
        std::vector<u8> data{};
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

void IUser::CreateApplicationArea(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto device_handle{rp.Pop<u64>()};
    const auto access_id{rp.Pop<u32>()};
    const auto data{ctx.ReadBuffer()};
    LOG_WARNING(Service_NFP, "(STUBBED) called, device_handle={}, data_size={}, access_id={}",
                device_handle, access_id, data.size());

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
        rb.Push(ApplicationAreaSize);
        return;
    }

    LOG_ERROR(Service_NFP, "Handle not found, device_handle={}", device_handle);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ErrCodes::DeviceNotFound);
}

void IUser::AttachAvailabilityChangeEvent(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFP, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(availability_change_event->GetReadableEvent());
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

bool Module::Interface::LoadAmiibo(const std::vector<u8>& buffer) {
    if (device_state != DeviceState::SearchingForTag) {
        LOG_ERROR(Service_NFP, "Game is not looking for amiibos, current state {}", device_state);
        return false;
    }

    constexpr auto tag_size = sizeof(NTAG215File);
    constexpr auto tag_size_without_password = sizeof(NTAG215File) - sizeof(NTAG215Password);

    std::vector<u8> amiibo_buffer = buffer;

    if (amiibo_buffer.size() < tag_size_without_password) {
        LOG_ERROR(Service_NFP, "Wrong file size {}", buffer.size());
        return false;
    }

    // Ensure it has the correct size
    if (amiibo_buffer.size() != tag_size) {
        amiibo_buffer.resize(tag_size, 0);
    }

    LOG_INFO(Service_NFP, "Amiibo detected");
    std::memcpy(&tag_data, buffer.data(), tag_size);

    if (!IsAmiiboValid()) {
        return false;
    }

    // This value can't be dumped from a tag. Generate it
    tag_data.password.PWD = GetTagPassword(tag_data.uuid);

    device_state = DeviceState::TagFound;
    activate_event->GetWritableEvent().Signal();
    return true;
}

void Module::Interface::CloseAmiibo() {
    LOG_INFO(Service_NFP, "Remove amiibo");
    device_state = DeviceState::TagRemoved;
    is_application_area_initialized = false;
    application_area_id = 0;
    application_area_data.clear();
    deactivate_event->GetWritableEvent().Signal();
}

bool Module::Interface::IsAmiiboValid() const {
    const auto& amiibo_data = tag_data.user_memory;
    LOG_DEBUG(Service_NFP, "uuid_lock=0x{0:x}", tag_data.lock_bytes);
    LOG_DEBUG(Service_NFP, "compability_container=0x{0:x}", tag_data.compability_container);
    LOG_DEBUG(Service_NFP, "crypto_init=0x{0:x}", amiibo_data.crypto_init);
    LOG_DEBUG(Service_NFP, "write_count={}", amiibo_data.write_count);

    LOG_DEBUG(Service_NFP, "character_id=0x{0:x}", amiibo_data.model_info.character_id);
    LOG_DEBUG(Service_NFP, "character_variant={}", amiibo_data.model_info.character_variant);
    LOG_DEBUG(Service_NFP, "amiibo_type={}", amiibo_data.model_info.amiibo_type);
    LOG_DEBUG(Service_NFP, "model_number=0x{0:x}", amiibo_data.model_info.model_number);
    LOG_DEBUG(Service_NFP, "series={}", amiibo_data.model_info.series);
    LOG_DEBUG(Service_NFP, "fixed_value=0x{0:x}", amiibo_data.model_info.fixed);

    LOG_DEBUG(Service_NFP, "tag_dynamic_lock=0x{0:x}", tag_data.dynamic_lock);
    LOG_DEBUG(Service_NFP, "tag_CFG0=0x{0:x}", tag_data.CFG0);
    LOG_DEBUG(Service_NFP, "tag_CFG1=0x{0:x}", tag_data.CFG1);

    // Check against all know constants on an amiibo binary
    if (tag_data.lock_bytes != 0xE00F) {
        return false;
    }
    if (tag_data.compability_container != 0xEEFF10F1U) {
        return false;
    }
    if ((amiibo_data.crypto_init & 0xFF) != 0xA5) {
        return false;
    }
    if (amiibo_data.model_info.fixed != 0x02) {
        return false;
    }
    if ((tag_data.dynamic_lock & 0xFFFFFF) != 0x0F0001) {
        return false;
    }
    if (tag_data.CFG0 != 0x04000000U) {
        return false;
    }
    if (tag_data.CFG1 != 0x5F) {
        return false;
    }
    return true;
}

Kernel::KReadableEvent& Module::Interface::GetActivateEvent() const {
    return activate_event->GetReadableEvent();
}

Kernel::KReadableEvent& Module::Interface::GetDeactivateEvent() const {
    return deactivate_event->GetReadableEvent();
}

void Module::Interface::Initialize() {
    device_state = DeviceState::Initialized;
}

void Module::Interface::Finalize() {
    device_state = DeviceState::Unaviable;
    is_application_area_initialized = false;
    application_area_id = 0;
    application_area_data.clear();
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

Result Module::Interface::Mount() {
    if (device_state == DeviceState::TagFound) {
        device_state = DeviceState::TagMounted;
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
    return ErrCodes::WrongDeviceState;
}

Result Module::Interface::Unmount() {
    if (device_state == DeviceState::TagMounted) {
        is_application_area_initialized = false;
        application_area_id = 0;
        application_area_data.clear();
        device_state = DeviceState::TagFound;
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
    return ErrCodes::WrongDeviceState;
}

Result Module::Interface::GetTagInfo(TagInfo& tag_info) const {
    if (device_state == DeviceState::TagFound || device_state == DeviceState::TagMounted) {
        tag_info = {
            .uuid = tag_data.uuid,
            .uuid_length = static_cast<u8>(tag_data.uuid.size()),
            .protocol = protocol,
            .tag_type = static_cast<u32>(tag_data.user_memory.model_info.amiibo_type),
        };
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
    return ErrCodes::WrongDeviceState;
}

Result Module::Interface::GetCommonInfo(CommonInfo& common_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    // Read this data from the amiibo save file
    common_info = {
        .last_write_year = 2022,
        .last_write_month = 2,
        .last_write_day = 7,
        .write_counter = tag_data.user_memory.write_count,
        .version = 1,
        .application_area_size = ApplicationAreaSize,
    };
    return ResultSuccess;
}

Result Module::Interface::GetModelInfo(ModelInfo& model_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    model_info = tag_data.user_memory.model_info;
    return ResultSuccess;
}

Result Module::Interface::GetRegisterInfo(RegisterInfo& register_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }

    Service::Mii::MiiManager manager;

    // Read this data from the amiibo save file
    register_info = {
        .mii_char_info = manager.BuildDefault(0),
        .first_write_year = 2022,
        .first_write_month = 2,
        .first_write_day = 7,
        .amiibo_name = {'Y', 'u', 'z', 'u', 'A', 'm', 'i', 'i', 'b', 'o', 0},
        .unknown = {},
    };
    return ResultSuccess;
}

Result Module::Interface::OpenApplicationArea(u32 access_id) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }
    if (AmiiboApplicationDataExist(access_id)) {
        application_area_data = LoadAmiiboApplicationData(access_id);
        application_area_id = access_id;
        is_application_area_initialized = true;
    }
    if (!is_application_area_initialized) {
        LOG_WARNING(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }
    return ResultSuccess;
}

Result Module::Interface::GetApplicationArea(std::vector<u8>& data) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }
    if (!is_application_area_initialized) {
        LOG_ERROR(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }

    data = application_area_data;

    return ResultSuccess;
}

Result Module::Interface::SetApplicationArea(const std::vector<u8>& data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }
    if (!is_application_area_initialized) {
        LOG_ERROR(Service_NFP, "Application area is not initialized");
        return ErrCodes::ApplicationAreaIsNotInitialized;
    }
    application_area_data = data;
    SaveAmiiboApplicationData(application_area_id, application_area_data);
    return ResultSuccess;
}

Result Module::Interface::CreateApplicationArea(u32 access_id, const std::vector<u8>& data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return ErrCodes::WrongDeviceState;
    }
    if (AmiiboApplicationDataExist(access_id)) {
        LOG_ERROR(Service_NFP, "Application area already exist");
        return ErrCodes::ApplicationAreaExist;
    }
    application_area_data = data;
    application_area_id = access_id;
    SaveAmiiboApplicationData(application_area_id, application_area_data);
    return ResultSuccess;
}

bool Module::Interface::AmiiboApplicationDataExist(u32 access_id) const {
    // TODO(german77): Check if file exist
    return false;
}

std::vector<u8> Module::Interface::LoadAmiiboApplicationData(u32 access_id) const {
    // TODO(german77): Read file
    std::vector<u8> data(ApplicationAreaSize);
    return data;
}

void Module::Interface::SaveAmiiboApplicationData(u32 access_id,
                                                  const std::vector<u8>& data) const {
    // TODO(german77): Save file
}

u64 Module::Interface::GetHandle() const {
    // Generate a handle based of the npad id
    return static_cast<u64>(npad_id);
}

DeviceState Module::Interface::GetCurrentState() const {
    return device_state;
}

Core::HID::NpadIdType Module::Interface::GetNpadId() const {
    return npad_id;
}

u32 Module::Interface::GetTagPassword(const TagUuid& uuid) const {
    // Verifiy that the generated password is correct
    u32 password = 0xAA ^ (uuid[1] ^ uuid[3]);
    password &= (0x55 ^ (uuid[2] ^ uuid[4])) << 8;
    password &= (0xAA ^ (uuid[3] ^ uuid[5])) << 16;
    password &= (0x55 ^ (uuid[4] ^ uuid[6])) << 24;
    return password;
}

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    auto module = std::make_shared<Module>();
    std::make_shared<NFP_User>(module, system)->InstallAsService(service_manager);
}

} // namespace Service::NFP
