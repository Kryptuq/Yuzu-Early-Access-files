// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "common/common_funcs.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/mii/types.h"
#include "core/hle/service/nfp/amiibo_types.h"
#include "core/hle/service/service.h"

namespace Kernel {
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Core::HID {
enum class NpadIdType : u32;
} // namespace Core::HID

namespace Service::NFP {
using AmiiboName = std::array<char, (amiibo_name_length * 4) + 1>;

struct TagInfo {
    TagUuid uuid;
    u8 uuid_length;
    INSERT_PADDING_BYTES(0x15);
    s32 protocol;
    u32 tag_type;
    INSERT_PADDING_BYTES(0x30);
};
static_assert(sizeof(TagInfo) == 0x58, "TagInfo is an invalid size");

struct CommonInfo {
    u16 last_write_year;
    u8 last_write_month;
    u8 last_write_day;
    u16 write_counter;
    u16 version;
    u32 application_area_size;
    INSERT_PADDING_BYTES(0x34);
};
static_assert(sizeof(CommonInfo) == 0x40, "CommonInfo is an invalid size");

struct ModelInfo {
    u16 character_id;
    u8 character_variant;
    AmiiboType amiibo_type;
    u16 model_number;
    AmiiboSeries series;
    u8 constant_value;          // Must be 02
    INSERT_PADDING_BYTES(0x38); // Unknown
};
static_assert(sizeof(ModelInfo) == 0x40, "ModelInfo is an invalid size");

struct RegisterInfo {
    Service::Mii::CharInfo mii_char_info;
    u16 first_write_year;
    u8 first_write_month;
    u8 first_write_day;
    AmiiboName amiibo_name;
    u8 font_region;
    INSERT_PADDING_BYTES(0x7A);
};
static_assert(sizeof(RegisterInfo) == 0x100, "RegisterInfo is an invalid size");

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> module_, Core::System& system_,
                           const char* name);
        ~Interface() override;

        void CreateUserInterface(Kernel::HLERequestContext& ctx);
        bool LoadAmiibo(const std::string& filename);
        bool LoadAmiiboFile(const std::string& filename);
        void CloseAmiibo();

        void Initialize();
        void Finalize();

        Result StartDetection(s32 protocol_);
        Result StopDetection();
        Result Mount();
        Result Unmount();
        Result Flush();

        Result GetTagInfo(TagInfo& tag_info) const;
        Result GetCommonInfo(CommonInfo& common_info) const;
        Result GetModelInfo(ModelInfo& model_info) const;
        Result GetRegisterInfo(RegisterInfo& register_info) const;

        Result OpenApplicationArea(u32 access_id);
        Result GetApplicationArea(ApplicationArea& data) const;
        Result SetApplicationArea(const std::vector<u8>& data);
        Result CreateApplicationArea(u32 access_id, const std::vector<u8>& data);
        Result RecreateApplicationArea(u32 access_id, const std::vector<u8>& data);

        u64 GetHandle() const;
        DeviceState GetCurrentState() const;
        Core::HID::NpadIdType GetNpadId() const;

        Kernel::KReadableEvent& GetActivateEvent() const;
        Kernel::KReadableEvent& GetDeactivateEvent() const;

    protected:
        std::shared_ptr<Module> module;

    private:
        AmiiboName GetAmiiboName(const AmiiboSettings& settings) const;

        const Core::HID::NpadIdType npad_id;

        bool is_data_decoded{};
        bool is_application_area_initialized{};
        s32 protocol;
        std::string file_path{};
        Kernel::KEvent* activate_event;
        Kernel::KEvent* deactivate_event;
        DeviceState device_state{DeviceState::Unaviable};
        KernelHelpers::ServiceContext service_context;

        NTAG215File tag_data{};
        EncryptedNTAG215File encrypted_tag_data{};
    };
};

class IUser final : public ServiceFramework<IUser> {
public:
    explicit IUser(Module::Interface& nfp_interface_, Core::System& system_);

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void Finalize(Kernel::HLERequestContext& ctx);
    void ListDevices(Kernel::HLERequestContext& ctx);
    void StartDetection(Kernel::HLERequestContext& ctx);
    void StopDetection(Kernel::HLERequestContext& ctx);
    void Mount(Kernel::HLERequestContext& ctx);
    void Unmount(Kernel::HLERequestContext& ctx);
    void OpenApplicationArea(Kernel::HLERequestContext& ctx);
    void GetApplicationArea(Kernel::HLERequestContext& ctx);
    void SetApplicationArea(Kernel::HLERequestContext& ctx);
    void Flush(Kernel::HLERequestContext& ctx);
    void CreateApplicationArea(Kernel::HLERequestContext& ctx);
    void GetTagInfo(Kernel::HLERequestContext& ctx);
    void GetRegisterInfo(Kernel::HLERequestContext& ctx);
    void GetCommonInfo(Kernel::HLERequestContext& ctx);
    void GetModelInfo(Kernel::HLERequestContext& ctx);
    void AttachActivateEvent(Kernel::HLERequestContext& ctx);
    void AttachDeactivateEvent(Kernel::HLERequestContext& ctx);
    void GetState(Kernel::HLERequestContext& ctx);
    void GetDeviceState(Kernel::HLERequestContext& ctx);
    void GetNpadId(Kernel::HLERequestContext& ctx);
    void GetApplicationAreaSize(Kernel::HLERequestContext& ctx);
    void AttachAvailabilityChangeEvent(Kernel::HLERequestContext& ctx);
    void RecreateApplicationArea(Kernel::HLERequestContext& ctx);

    KernelHelpers::ServiceContext service_context;

    // TODO(german77): We should have a vector of interfaces
    Module::Interface& nfp_interface;

    State state{State::NonInitialized};
    Kernel::KEvent* availability_change_event;
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::NFP
