// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/input.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hid/hid_types.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/nfc/nfc_device.h"
#include "core/hle/service/nfc/nfc_result.h"
#include "core/hle/service/nfc/nfc_user.h"

namespace Service::NFC {
NfcDevice::NfcDevice(Core::HID::NpadIdType npad_id_, Core::System& system_,
                     KernelHelpers::ServiceContext& service_context_,
                     Kernel::KEvent* availability_change_event_)
    : npad_id{npad_id_}, system{system_}, service_context{service_context_},
      availability_change_event{availability_change_event_} {
    activate_event = service_context.CreateEvent("IUser:NFCActivateEvent");
    deactivate_event = service_context.CreateEvent("IUser:NFCDeactivateEvent");
    npad_device = system.HIDCore().GetEmulatedController(npad_id);

    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { NpadUpdate(type); },
        .is_npad_service = false,
    };
    is_controller_set = true;
    callback_key = npad_device->SetCallback(engine_callback);
}

NfcDevice::~NfcDevice() {
    activate_event->Close();
    deactivate_event->Close();
    if (!is_controller_set) {
        return;
    }
    npad_device->DeleteCallback(callback_key);
    is_controller_set = false;
};

void NfcDevice::NpadUpdate(Core::HID::ControllerTriggerType type) {
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
        LoadNfcTag(nfc_status.data);
        break;
    case Common::Input::NfcState::AmiiboRemoved:
        if (device_state != NFP::DeviceState::SearchingForTag) {
            CloseNfcTag();
        }
        break;
    default:
        break;
    }
}

bool NfcDevice::LoadNfcTag(std::span<const u8> data) {
    if (device_state != NFP::DeviceState::SearchingForTag) {
        LOG_ERROR(Service_NFC, "Game is not looking for nfc tag, current state {}", device_state);
        return false;
    }

    if (data.size() < sizeof(NFP::EncryptedNTAG215File)) {
        LOG_ERROR(Service_NFC, "Not an amiibo, size={}", data.size());
        return false;
    }

    tag_data.resize(data.size());
    memcpy(tag_data.data(), data.data(), data.size());
    memcpy(&encrypted_tag_data, data.data(), sizeof(NFP::EncryptedNTAG215File));

    device_state = NFP::DeviceState::TagFound;
    deactivate_event->GetReadableEvent().Clear();
    activate_event->Signal();
    return true;
}

void NfcDevice::CloseNfcTag() {
    LOG_INFO(Service_NFC, "Remove nfc tag");

    device_state = NFP::DeviceState::TagRemoved;
    encrypted_tag_data = {};
    activate_event->GetReadableEvent().Clear();
    deactivate_event->Signal();
}

Kernel::KReadableEvent& NfcDevice::GetActivateEvent() const {
    return activate_event->GetReadableEvent();
}

Kernel::KReadableEvent& NfcDevice::GetDeactivateEvent() const {
    return deactivate_event->GetReadableEvent();
}

void NfcDevice::Initialize() {
    device_state =
        npad_device->HasNfc() ? NFP::DeviceState::Initialized : NFP::DeviceState::Unavailable;
    encrypted_tag_data = {};
}

void NfcDevice::Finalize() {
    if (device_state == NFP::DeviceState::SearchingForTag ||
        device_state == NFP::DeviceState::TagRemoved) {
        StopDetection();
    }
    device_state = NFP::DeviceState::Unavailable;
}

Result NfcDevice::StartDetection(NFP::TagProtocol allowed_protocol) {
    if (device_state != NFP::DeviceState::Initialized &&
        device_state != NFP::DeviceState::TagRemoved) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        return WrongDeviceState;
    }

    if (!npad_device->SetPollingMode(Common::Input::PollingMode::NFC)) {
        LOG_ERROR(Service_NFC, "Nfc not supported");
        return NfcDisabled;
    }

    device_state = NFP::DeviceState::SearchingForTag;
    allowed_protocols = allowed_protocol;
    return ResultSuccess;
}

Result NfcDevice::StopDetection() {
    npad_device->SetPollingMode(Common::Input::PollingMode::Active);

    if (device_state == NFP::DeviceState::Initialized) {
        return ResultSuccess;
    }

    if (device_state == NFP::DeviceState::TagFound ||
        device_state == NFP::DeviceState::TagMounted) {
        CloseNfcTag();
        return ResultSuccess;
    }
    if (device_state == NFP::DeviceState::SearchingForTag ||
        device_state == NFP::DeviceState::TagRemoved) {
        device_state = NFP::DeviceState::Initialized;
        return ResultSuccess;
    }

    LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
    return WrongDeviceState;
}

Result NfcDevice::Flush() {
    if (device_state != NFP::DeviceState::TagFound &&
        device_state != NFP::DeviceState::TagMounted) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        if (device_state == NFP::DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (!npad_device->WriteNfc(tag_data)) {
        LOG_ERROR(Service_NFP, "Error writing to file");
        return MifareReadError;
    }

    return ResultSuccess;
}

Result NfcDevice::GetTagInfo(NFP::TagInfo& tag_info, bool is_mifare) const {
    if (device_state != NFP::DeviceState::TagFound &&
        device_state != NFP::DeviceState::TagMounted) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        if (device_state == NFP::DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (is_mifare) {
        tag_info = {
            .uuid = encrypted_tag_data.uuid.uid,
            .uuid_length = static_cast<u8>(encrypted_tag_data.uuid.uid.size()),
            .protocol = NFP::TagProtocol::TypeA,
            .tag_type = NFP::TagType::Type4,
        };
        return ResultSuccess;
    }

    // Protocol and tag type may change here
    tag_info = {
        .uuid = encrypted_tag_data.uuid.uid,
        .uuid_length = static_cast<u8>(encrypted_tag_data.uuid.uid.size()),
        .protocol = NFP::TagProtocol::TypeA,
        .tag_type = NFP::TagType::Type2,
    };

    return ResultSuccess;
}

Result NfcDevice::MifareRead(const NFP::MifareReadBlockParameter& parameter,
                             NFP::MifareReadBlockData& read_block_data) {
    const std::size_t sector_index = parameter.sector_number * sizeof(NFP::DataBlock);
    read_block_data.sector_number = parameter.sector_number;

    if (device_state != NFP::DeviceState::TagFound &&
        device_state != NFP::DeviceState::TagMounted) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        if (device_state == NFP::DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (tag_data.size() < sector_index + sizeof(NFP::DataBlock)) {
        return MifareReadError;
    }

    // TODO: Use parameter.sector_key to read encrypted data
    memcpy(read_block_data.data.data(), tag_data.data() + sector_index, sizeof(NFP::DataBlock));

    return ResultSuccess;
}

Result NfcDevice::MifareWrite(const NFP::MifareWriteBlockParameter& parameter) {
    const std::size_t sector_index = parameter.sector_number * sizeof(NFP::DataBlock);

    if (device_state != NFP::DeviceState::TagFound &&
        device_state != NFP::DeviceState::TagMounted) {
        LOG_ERROR(Service_NFC, "Wrong device state {}", device_state);
        if (device_state == NFP::DeviceState::TagRemoved) {
            return TagRemoved;
        }
        return WrongDeviceState;
    }

    if (tag_data.size() < sector_index + sizeof(NFP::DataBlock)) {
        return MifareReadError;
    }

    // TODO: Use parameter.sector_key to encrypt the data
    memcpy(tag_data.data() + sector_index, parameter.data.data(), sizeof(NFP::DataBlock));

    return ResultSuccess;
}

u64 NfcDevice::GetHandle() const {
    // Generate a handle based of the npad id
    return static_cast<u64>(npad_id);
}

NFP::DeviceState NfcDevice::GetCurrentState() const {
    return device_state;
}

Core::HID::NpadIdType NfcDevice::GetNpadId() const {
    return npad_id;
}

} // namespace Service::NFC
