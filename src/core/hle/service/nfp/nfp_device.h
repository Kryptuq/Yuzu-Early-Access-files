// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

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

namespace Service::NFP {
class NfpDevice {
public:
    NfpDevice(Core::HID::NpadIdType npad_id_, Core::System& system_,
              KernelHelpers::ServiceContext& service_context_,
              Kernel::KEvent* availability_change_event_);
    ~NfpDevice();

    void Initialize();
    void Finalize();

    Result StartDetection(TagProtocol allowed_protocol);
    Result StopDetection();
    Result Mount(MountTarget mount_target);
    Result Unmount();
    Result Flush();

    Result GetTagInfo(TagInfo& tag_info) const;
    Result GetCommonInfo(CommonInfo& common_info) const;
    Result GetModelInfo(ModelInfo& model_info) const;
    Result GetRegisterInfo(RegisterInfo& register_info) const;
    Result GetAdminInfo(AdminInfo& admin_info) const;

    Result SetNicknameAndOwner(const AmiiboName& amiibo_name);
    Result RestoreAmiibo();
    Result DeleteAllData();

    Result OpenApplicationArea(u32 access_id);
    Result GetApplicationAreaId(u32& application_area_id) const;
    Result GetApplicationArea(std::vector<u8>& data) const;
    Result SetApplicationArea(std::span<const u8> data);
    Result CreateApplicationArea(u32 access_id, std::span<const u8> data);
    Result RecreateApplicationArea(u32 access_id, std::span<const u8> data);
    Result DeleteApplicationArea();

    u64 GetHandle() const;
    u32 GetApplicationAreaSize() const;
    DeviceState GetCurrentState() const;
    Core::HID::NpadIdType GetNpadId() const;

    Kernel::KReadableEvent& GetActivateEvent() const;
    Kernel::KReadableEvent& GetDeactivateEvent() const;

private:
    void NpadUpdate(Core::HID::ControllerTriggerType type);
    bool LoadAmiibo(std::span<const u8> data);
    void CloseAmiibo();

    AmiiboName GetAmiiboName(const AmiiboSettings& settings) const;
    void SetAmiiboName(AmiiboSettings& settings, const AmiiboName& amiibo_name);
    AmiiboDate GetAmiiboDate(s64 posix_time) const;
    void UpdateSettingsCrc();
    u32 CalculateCrc(std::span<u8> data);

    bool is_controller_set{};
    int callback_key;
    const Core::HID::NpadIdType npad_id;
    Core::System& system;
    Core::HID::EmulatedController* npad_device = nullptr;
    KernelHelpers::ServiceContext& service_context;
    Kernel::KEvent* activate_event = nullptr;
    Kernel::KEvent* deactivate_event = nullptr;
    Kernel::KEvent* availability_change_event = nullptr;

    bool is_data_moddified{};
    bool is_app_area_open{};
    TagProtocol allowed_protocols{};
    s64 current_posix_time{};
    MountTarget mount_target{MountTarget::None};
    DeviceState device_state{DeviceState::Unavailable};

    NTAG215File tag_data{};
    EncryptedNTAG215File encrypted_tag_data{};
};

} // namespace Service::NFP
