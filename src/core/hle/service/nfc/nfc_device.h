// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/nfp/nfp_types.h"
#include "core/hle/service/service.h"

namespace Kernel {
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Core {
class System;
} // namespace Core

namespace Core::HID {
class EmulatedController;
enum class ControllerTriggerType;
enum class NpadIdType : u32;
} // namespace Core::HID

namespace Service::NFC {
class NfcDevice {
public:
    NfcDevice(Core::HID::NpadIdType npad_id_, Core::System& system_,
              KernelHelpers::ServiceContext& service_context_,
              Kernel::KEvent* availability_change_event_);
    ~NfcDevice();

    void Initialize();
    void Finalize();

    Result StartDetection(NFP::TagProtocol allowed_protocol);
    Result StopDetection();
    Result Flush();

    Result GetTagInfo(NFP::TagInfo& tag_info, bool is_mifare) const;

    Result MifareRead(const NFP::MifareReadBlockParameter& parameter,
                      NFP::MifareReadBlockData& read_block_data);

    Result MifareWrite(const NFP::MifareWriteBlockParameter& parameter);

    u64 GetHandle() const;
    NFP::DeviceState GetCurrentState() const;
    Core::HID::NpadIdType GetNpadId() const;

    Kernel::KReadableEvent& GetActivateEvent() const;
    Kernel::KReadableEvent& GetDeactivateEvent() const;

private:
    void NpadUpdate(Core::HID::ControllerTriggerType type);
    bool LoadNfcTag(std::span<const u8> data);
    void CloseNfcTag();

    bool is_controller_set{};
    int callback_key;
    const Core::HID::NpadIdType npad_id;
    Core::System& system;
    Core::HID::EmulatedController* npad_device = nullptr;
    KernelHelpers::ServiceContext& service_context;
    Kernel::KEvent* activate_event = nullptr;
    Kernel::KEvent* deactivate_event = nullptr;
    Kernel::KEvent* availability_change_event = nullptr;

    bool is_initalized{};
    NFP::TagProtocol allowed_protocols{};
    NFP::DeviceState device_state{NFP::DeviceState::Unavailable};

    NFP::EncryptedNTAG215File encrypted_tag_data{};
    std::vector<u8> tag_data{};
};

} // namespace Service::NFC
