// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>

#include "common/input.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "common/tiny_mt.h"
#include "core/core.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/mii/mii_manager.h"
#include "core/hle/service/mii/types.h"
#include "core/hle/service/nfp/amiibo_crypto.h"
#include "core/hle/service/nfp/nfp_device.h"
#include "core/hle/service/nfp/nfp_result.h"
#include "core/hle/service/nfp/nfp_user.h"
#include "core/hle/service/time/time_manager.h"
#include "core/hle/service/time/time_zone_content_manager.h"
#include "core/hle/service/time/time_zone_types.h"

namespace Service::NFP {
NfpDevice::NfpDevice(Core::HID::NpadIdType npad_id_, Core::System& system_,
                     KernelHelpers::ServiceContext& service_context_,
                     Kernel::KEvent* availability_change_event_)
    : npad_id{npad_id_}, system{system_}, service_context{service_context_},
      availability_change_event{availability_change_event_} {
    activate_event = service_context.CreateEvent("IUser:NFPActivateEvent");
    deactivate_event = service_context.CreateEvent("IUser:NFPDeactivateEvent");
    npad_device = system.HIDCore().GetEmulatedController(npad_id);

    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { NpadUpdate(type); },
        .is_npad_service = false,
    };
    is_controller_set = true;
    callback_key = npad_device->SetCallback(engine_callback);

    auto& standard_steady_clock{system.GetTimeManager().GetStandardSteadyClockCore()};
    current_posix_time = standard_steady_clock.GetCurrentTimePoint(system).time_point;
}

NfpDevice::~NfpDevice() {
    activate_event->Close();
    deactivate_event->Close();
    if (!is_controller_set) {
        return;
    }
    npad_device->DeleteCallback(callback_key);
    is_controller_set = false;
};

void NfpDevice::NpadUpdate(Core::HID::ControllerTriggerType type) {
    if (type == Core::HID::ControllerTriggerType::Connected ||
        type == Core::HID::ControllerTriggerType::Disconnected) {
        availability_change_event->Signal();
        return;
    }

    if (type != Core::HID::ControllerTriggerType::Nfc) {
        return;
    }

    if (!npad_device->IsConnected()) {
        return;
    }

    const auto nfc_status = npad_device->GetNfc();
    switch (nfc_status.state) {
    case Common::Input::NfcState::NewAmiibo:
        LoadAmiibo(nfc_status.data);
        break;
    case Common::Input::NfcState::AmiiboRemoved:
        if (device_state == DeviceState::Initialized || device_state == DeviceState::TagRemoved) {
            break;
        }
        if (device_state != DeviceState::SearchingForTag) {
            CloseAmiibo();
        }
        break;
    default:
        break;
    }
}

bool NfpDevice::LoadAmiibo(std::span<const u8> data) {
    if (device_state != DeviceState::SearchingForTag) {
        LOG_ERROR(Service_NFP, "Game is not looking for amiibos, current state {}", device_state);
        return false;
    }

    if (data.size() != sizeof(EncryptedNTAG215File)) {
        LOG_ERROR(Service_NFP, "Not an amiibo, size={}", data.size());
        return false;
    }

    // TODO: Filter by allowed_protocols here

    memcpy(&encrypted_tag_data, data.data(), sizeof(EncryptedNTAG215File));

    device_state = DeviceState::TagFound;
    deactivate_event->GetReadableEvent().Clear();
    activate_event->Signal();
    return true;
}

void NfpDevice::CloseAmiibo() {
    LOG_INFO(Service_NFP, "Remove amiibo");

    if (device_state == DeviceState::TagMounted) {
        Unmount();
    }

    device_state = DeviceState::TagRemoved;
    encrypted_tag_data = {};
    tag_data = {};
    activate_event->GetReadableEvent().Clear();
    deactivate_event->Signal();
}

Kernel::KReadableEvent& NfpDevice::GetActivateEvent() const {
    return activate_event->GetReadableEvent();
}

Kernel::KReadableEvent& NfpDevice::GetDeactivateEvent() const {
    return deactivate_event->GetReadableEvent();
}

void NfpDevice::Initialize() {
    device_state = npad_device->HasNfc() ? DeviceState::Initialized : DeviceState::Unavailable;
    encrypted_tag_data = {};
    tag_data = {};
}

void NfpDevice::Finalize() {
    if (device_state == DeviceState::TagMounted) {
        Unmount();
    }
    if (device_state == DeviceState::SearchingForTag || device_state == DeviceState::TagRemoved) {
        StopDetection();
    }
    device_state = DeviceState::Unavailable;
}

Result NfpDevice::StartDetection(TagProtocol allowed_protocol) {
    if (device_state != DeviceState::Initialized && device_state != DeviceState::TagRemoved) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return WrongDeviceState;
    }

    if (npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::NFC) !=
        Common::Input::DriverResult::Success) {
        LOG_ERROR(Service_NFP, "Nfc not supported");
        return NfcDisabled;
    }

    device_state = DeviceState::SearchingForTag;
    allowed_protocols = allowed_protocol;
    return ResultSuccess;
}

Result NfpDevice::StopDetection() {
    npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                Common::Input::PollingMode::Active);

    if (device_state == DeviceState::Initialized) {
        return ResultSuccess;
    }

    if (device_state == DeviceState::TagFound || device_state == DeviceState::TagMounted) {
        CloseAmiibo();
    }

    if (device_state == DeviceState::SearchingForTag || device_state == DeviceState::TagRemoved) {
        device_state = DeviceState::Initialized;
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
    return WrongDeviceState;
}

Result NfpDevice::Flush() {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    auto& settings = tag_data.settings;

    const auto& current_date = GetAmiiboDate(current_posix_time);
    if (settings.write_date.raw_date != current_date.raw_date) {
        settings.write_date = current_date;
        UpdateSettingsCrc();
    }

    tag_data.write_counter++;

    if (!AmiiboCrypto::EncodeAmiibo(tag_data, encrypted_tag_data)) {
        LOG_ERROR(Service_NFP, "Failed to encode data");
        return WriteAmiiboFailed;
    }

    std::vector<u8> data(sizeof(encrypted_tag_data));
    memcpy(data.data(), &encrypted_tag_data, sizeof(encrypted_tag_data));

    if (!npad_device->WriteNfc(data)) {
        LOG_ERROR(Service_NFP, "Error writing to file");
        return WriteAmiiboFailed;
    }

    is_data_moddified = false;

    return ResultSuccess;
}

Result NfpDevice::Mount(MountTarget mount_target_) {
    if (device_state != DeviceState::TagFound) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        return WrongDeviceState;
    }

    if (!AmiiboCrypto::IsAmiiboValid(encrypted_tag_data)) {
        LOG_ERROR(Service_NFP, "Not an amiibo");
        return NotAnAmiibo;
    }

    // Mark amiibos as read only when keys are missing
    if (!AmiiboCrypto::IsKeyAvailable()) {
        LOG_ERROR(Service_NFP, "No keys detected");
        device_state = DeviceState::TagMounted;
        mount_target = MountTarget::Rom;
        return ResultSuccess;
    }

    if (!AmiiboCrypto::DecodeAmiibo(encrypted_tag_data, tag_data)) {
        LOG_ERROR(Service_NFP, "Can't decode amiibo {}", device_state);
        return CorruptedData;
    }

    device_state = DeviceState::TagMounted;
    mount_target = mount_target_;
    return ResultSuccess;
}

Result NfpDevice::Unmount() {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    // Save data before unloading the amiibo
    if (is_data_moddified) {
        Flush();
    }

    device_state = DeviceState::TagFound;
    mount_target = MountTarget::None;
    is_app_area_open = false;

    return ResultSuccess;
}

Result NfpDevice::GetTagInfo(TagInfo& tag_info) const {
    if (device_state != DeviceState::TagFound && device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    tag_info = {
        .uuid = encrypted_tag_data.uuid.uid,
        .uuid_length = static_cast<u8>(encrypted_tag_data.uuid.uid.size()),
        .protocol = TagProtocol::TypeA,
        .tag_type = TagType::Type2,
    };

    return ResultSuccess;
}

Result NfpDevice::GetCommonInfo(CommonInfo& common_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    const auto& settings = tag_data.settings;

    // TODO: Validate this data
    common_info = {
        .last_write_date = settings.write_date.GetWriteDate(),
        .write_counter = tag_data.write_counter,
        .version = tag_data.amiibo_version,
        .application_area_size = sizeof(ApplicationArea),
    };
    return ResultSuccess;
}

Result NfpDevice::GetModelInfo(ModelInfo& model_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    const auto& model_info_data = encrypted_tag_data.user_memory.model_info;
    model_info = {
        .character_id = model_info_data.character_id,
        .character_variant = model_info_data.character_variant,
        .amiibo_type = model_info_data.amiibo_type,
        .model_number = model_info_data.model_number,
        .series = model_info_data.series,
    };
    return ResultSuccess;
}

Result NfpDevice::GetRegisterInfo(RegisterInfo& register_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.amiibo_initialized == 0) {
        return RegistrationIsNotInitialized;
    }

    Service::Mii::MiiManager manager;
    const auto& settings = tag_data.settings;

    // TODO: Validate this data
    register_info = {
        .mii_char_info = manager.ConvertV3ToCharInfo(tag_data.owner_mii),
        .creation_date = settings.init_date.GetWriteDate(),
        .amiibo_name = GetAmiiboName(settings),
        .font_region = settings.settings.font_region,
    };

    return ResultSuccess;
}

Result NfpDevice::GetAdminInfo(AdminInfo& admin_info) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFC, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    u8 flags = static_cast<u8>(tag_data.settings.settings.raw >> 0x4);
    if (tag_data.settings.settings.amiibo_initialized == 0) {
        flags = flags & 0xfe;
    }

    u64 application_id = 0;
    u32 application_area_id = 0;
    AppAreaVersion app_area_version = AppAreaVersion::NotSet;
    if (tag_data.settings.settings.appdata_initialized != 0) {
        application_id = tag_data.application_id;
        app_area_version =
            static_cast<AppAreaVersion>(application_id >> application_id_version_offset & 0xf);

        // Restore application id to original value
        if (application_id >> 0x38 != 0) {
            const u8 application_byte = tag_data.application_id_byte & 0xf;
            application_id = RemoveVersionByte(application_id) |
                             (static_cast<u64>(application_byte) << application_id_version_offset);
        }

        application_area_id = tag_data.application_area_id;
    }

    // TODO: Validate this data
    admin_info = {
        .application_id = application_id,
        .application_area_id = application_area_id,
        .crc_change_counter = tag_data.settings.crc_counter,
        .flags = flags,
        .tag_type = PackedTagType::Type2,
        .app_area_version = app_area_version,
    };

    return ResultSuccess;
}

Result NfpDevice::DeleteRegisterInfo() {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFC, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.amiibo_initialized == 0) {
        return RegistrationIsNotInitialized;
    }

    Common::TinyMT rng{};
    rng.GenerateRandomBytes(&tag_data.owner_mii, sizeof(tag_data.owner_mii));
    rng.GenerateRandomBytes(&tag_data.settings.amiibo_name, sizeof(tag_data.settings.amiibo_name));
    rng.GenerateRandomBytes(&tag_data.unknown, sizeof(u8));
    rng.GenerateRandomBytes(&tag_data.unknown2[0], sizeof(u32));
    rng.GenerateRandomBytes(&tag_data.unknown2[1], sizeof(u32));
    rng.GenerateRandomBytes(&tag_data.application_area_crc, sizeof(u32));
    rng.GenerateRandomBytes(&tag_data.settings.init_date, sizeof(u32));
    tag_data.settings.settings.font_region.Assign(0);
    tag_data.settings.settings.amiibo_initialized.Assign(0);

    return Flush();
}

Result NfpDevice::SetRegisterInfoPrivate(const AmiiboName& amiibo_name) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    Service::Mii::MiiManager manager;
    auto& settings = tag_data.settings;

    if (tag_data.settings.settings.amiibo_initialized == 0) {
        settings.init_date = GetAmiiboDate(current_posix_time);
        settings.write_date.raw_date = 0;
    }

    SetAmiiboName(settings, amiibo_name);
    tag_data.owner_mii = manager.ConvertCharInfoToV3(manager.BuildDefault(0));
    tag_data.unknown = 0;
    tag_data.unknown2[6] = 0;
    settings.country_code_id = 0;
    settings.settings.font_region.Assign(0);
    settings.settings.amiibo_initialized.Assign(1);

    // TODO: this is a mix of tag.file input
    std::array<u8, 0x7e> unknown_input{};
    tag_data.application_area_crc = CalculateCrc(unknown_input);

    return Flush();
}

Result NfpDevice::RestoreAmiibo() {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    // TODO: Load amiibo from backup on system
    LOG_ERROR(Service_NFP, "Not Implemented");
    return ResultSuccess;
}

Result NfpDevice::Format() {
    auto result1 = DeleteApplicationArea();
    auto result2 = DeleteRegisterInfo();

    if (result1.IsError()) {
        return result1;
    }

    if (result2.IsError()) {
        return result2;
    }

    return Flush();
}

Result NfpDevice::OpenApplicationArea(u32 access_id) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized.Value() == 0) {
        LOG_WARNING(Service_NFP, "Application area is not initialized");
        return ApplicationAreaIsNotInitialized;
    }

    if (tag_data.application_area_id != access_id) {
        LOG_WARNING(Service_NFP, "Wrong application area id");
        return WrongApplicationAreaId;
    }

    is_app_area_open = true;

    return ResultSuccess;
}

Result NfpDevice::GetApplicationAreaId(u32& application_area_id) const {
    application_area_id = {};

    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized.Value() == 0) {
        LOG_WARNING(Service_NFP, "Application area is not initialized");
        return ApplicationAreaIsNotInitialized;
    }

    application_area_id = tag_data.application_area_id;

    return ResultSuccess;
}

Result NfpDevice::GetApplicationArea(std::vector<u8>& data) const {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (!is_app_area_open) {
        LOG_ERROR(Service_NFP, "Application area is not open");
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized.Value() == 0) {
        LOG_ERROR(Service_NFP, "Application area is not initialized");
        return ApplicationAreaIsNotInitialized;
    }

    if (data.size() > sizeof(ApplicationArea)) {
        data.resize(sizeof(ApplicationArea));
    }

    memcpy(data.data(), tag_data.application_area.data(), data.size());

    return ResultSuccess;
}

Result NfpDevice::SetApplicationArea(std::span<const u8> data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (!is_app_area_open) {
        LOG_ERROR(Service_NFP, "Application area is not open");
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized.Value() == 0) {
        LOG_ERROR(Service_NFP, "Application area is not initialized");
        return ApplicationAreaIsNotInitialized;
    }

    if (data.size() > sizeof(ApplicationArea)) {
        LOG_ERROR(Service_NFP, "Wrong data size {}", data.size());
        return ResultUnknown;
    }

    Common::TinyMT rng{};
    std::memcpy(tag_data.application_area.data(), data.data(), data.size());
    // Fill remaining data with random numbers
    rng.GenerateRandomBytes(tag_data.application_area.data() + data.size(),
                            sizeof(ApplicationArea) - data.size());

    if (tag_data.application_write_counter != counter_limit) {
        tag_data.application_write_counter++;
    }

    is_data_moddified = true;

    return ResultSuccess;
}

Result NfpDevice::CreateApplicationArea(u32 access_id, std::span<const u8> data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized.Value() != 0) {
        LOG_ERROR(Service_NFP, "Application area already exist");
        return ApplicationAreaExist;
    }

    return RecreateApplicationArea(access_id, data);
}

Result NfpDevice::RecreateApplicationArea(u32 access_id, std::span<const u8> data) {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (data.size() > sizeof(ApplicationArea)) {
        LOG_ERROR(Service_NFP, "Wrong data size {}", data.size());
        return WrongApplicationAreaSize;
    }

    Common::TinyMT rng{};
    std::memcpy(tag_data.application_area.data(), data.data(), data.size());
    // Fill remaining data with random numbers
    rng.GenerateRandomBytes(tag_data.application_area.data() + data.size(),
                            sizeof(ApplicationArea) - data.size());

    if (tag_data.application_write_counter != counter_limit) {
        tag_data.application_write_counter++;
    }

    const u64 application_id = system.GetApplicationProcessProgramID();

    tag_data.application_id_byte =
        static_cast<u8>(application_id >> application_id_version_offset & 0xf);
    tag_data.application_id =
        RemoveVersionByte(application_id) |
        (static_cast<u64>(AppAreaVersion::NintendoSwitch) << application_id_version_offset);
    tag_data.settings.settings.appdata_initialized.Assign(1);
    tag_data.application_area_id = access_id;
    tag_data.unknown = {};

    // TODO: this is a mix of tag_data input
    std::array<u8, 0x7e> unknown_input{};
    tag_data.application_area_crc = CalculateCrc(unknown_input);

    return Flush();
}

Result NfpDevice::DeleteApplicationArea() {
    if (device_state != DeviceState::TagMounted) {
        LOG_ERROR(Service_NFP, "Wrong device state {}", device_state);
        if (device_state == DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (mount_target == MountTarget::None || mount_target == MountTarget::Rom) {
        LOG_ERROR(Service_NFP, "Amiibo is read only", device_state);
        return WrongDeviceState;
    }

    if (tag_data.settings.settings.appdata_initialized == 0) {
        return ApplicationAreaIsNotInitialized;
    }

    if (tag_data.application_write_counter != counter_limit) {
        tag_data.application_write_counter++;
    }

    Common::TinyMT rng{};
    rng.GenerateRandomBytes(tag_data.application_area.data(), sizeof(ApplicationArea));
    rng.GenerateRandomBytes(&tag_data.application_id, sizeof(u64));
    rng.GenerateRandomBytes(&tag_data.application_area_id, sizeof(u32));
    rng.GenerateRandomBytes(&tag_data.application_id_byte, sizeof(u8));
    tag_data.settings.settings.appdata_initialized.Assign(0);
    tag_data.unknown = {};

    return Flush();
}

u64 NfpDevice::GetHandle() const {
    // Generate a handle based of the npad id
    return static_cast<u64>(npad_id);
}

u32 NfpDevice::GetApplicationAreaSize() const {
    return sizeof(ApplicationArea);
}

DeviceState NfpDevice::GetCurrentState() const {
    return device_state;
}

Core::HID::NpadIdType NfpDevice::GetNpadId() const {
    return npad_id;
}

AmiiboName NfpDevice::GetAmiiboName(const AmiiboSettings& settings) const {
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

void NfpDevice::SetAmiiboName(AmiiboSettings& settings, const AmiiboName& amiibo_name) {
    std::array<char16_t, amiibo_name_length> settings_amiibo_name{};

    // Convert from utf8 to utf16
    const auto amiibo_name_utf16 = Common::UTF8ToUTF16(amiibo_name.data());
    memcpy(settings_amiibo_name.data(), amiibo_name_utf16.data(),
           amiibo_name_utf16.size() * sizeof(char16_t));

    // Convert from little endian to big endian
    for (std::size_t i = 0; i < amiibo_name_length; i++) {
        settings.amiibo_name[i] = static_cast<u16_be>(settings_amiibo_name[i]);
    }
}

AmiiboDate NfpDevice::GetAmiiboDate(s64 posix_time) const {
    const auto& time_zone_manager =
        system.GetTimeManager().GetTimeZoneContentManager().GetTimeZoneManager();
    Time::TimeZone::CalendarInfo calendar_info{};
    AmiiboDate amiibo_date{};

    amiibo_date.SetYear(2000);
    amiibo_date.SetMonth(1);
    amiibo_date.SetDay(1);

    if (time_zone_manager.ToCalendarTime({}, posix_time, calendar_info) == ResultSuccess) {
        amiibo_date.SetYear(calendar_info.time.year);
        amiibo_date.SetMonth(calendar_info.time.month);
        amiibo_date.SetDay(calendar_info.time.day);
    }

    return amiibo_date;
}

u64 NfpDevice::RemoveVersionByte(u64 application_id) const {
    return application_id & ~(0xfULL << application_id_version_offset);
}

void NfpDevice::UpdateSettingsCrc() {
    auto& settings = tag_data.settings;

    if (settings.crc_counter != counter_limit) {
        settings.crc_counter++;
    }

    // TODO: this reads data from a global, find what it is
    std::array<u8, 8> unknown_input{};
    settings.crc = CalculateCrc(unknown_input);
}

u32 NfpDevice::CalculateCrc(std::span<const u8> data) {
    constexpr u32 magic = 0xedb88320;
    u32 crc = 0xffffffff;

    if (data.size() == 0) {
        return 0;
    }

    for (u8 input : data) {
        u32 temp = (crc ^ input) >> 1;
        if (((crc ^ input) & 1) != 0) {
            temp = temp ^ magic;
        }

        for (std::size_t step = 0; step < 7; ++step) {
            crc = temp >> 1;
            if ((temp & 1) != 0) {
                crc = temp >> 1 ^ magic;
            }
        }
    }

    return ~crc;
}

} // namespace Service::NFP
