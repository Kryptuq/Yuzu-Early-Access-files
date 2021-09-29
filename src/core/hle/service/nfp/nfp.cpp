// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <atomic>

#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/lock.h"
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/nfp/nfp_user.h"

namespace Service::NFP {
namespace ErrCodes {
constexpr ResultCode ERR_NO_APPLICATION_AREA(ErrorModule::NFP, 152);
} // namespace ErrCodes

Module::Interface::Interface(std::shared_ptr<Module> module_, Core::System& system_,
                             const char* name)
    : ServiceFramework{system_, name}, module{std::move(module_)}, service_context{system_,
                                                                                   "NFP::IUser"} {
    nfc_tag_load = service_context.CreateEvent("NFP::IUser:NFCTagDetected");
}

Module::Interface::~Interface() {
    service_context.CloseEvent(nfc_tag_load);
}

class IUser final : public ServiceFramework<IUser> {
public:
    explicit IUser(Module::Interface& nfp_interface_, Core::System& system_,
                   KernelHelpers::ServiceContext& service_context_)
        : ServiceFramework{system_, "NFP::IUser"}, nfp_interface{nfp_interface_},
          service_context{service_context_} {
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
            {9, nullptr, "SetApplicationArea"},
            {10, nullptr, "Flush"},
            {11, nullptr, "Restore"},
            {12, nullptr, "CreateApplicationArea"},
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

        deactivate_event = service_context.CreateEvent("NFP::IUser:DeactivateEvent");
        availability_change_event =
            service_context.CreateEvent("NFP::IUser:AvailabilityChangeEvent");
    }

    ~IUser() override {
        service_context.CloseEvent(deactivate_event);
        service_context.CloseEvent(availability_change_event);
    }

private:
    struct TagInfo {
        std::array<u8, 10> uuid;
        u8 uuid_length; // TODO(ogniK): Figure out if this is actual the uuid length or does it
                        // mean something else
        std::array<u8, 0x15> padding_1;
        u32_le protocol;
        u32_le tag_type;
        std::array<u8, 0x2c> padding_2;
    };
    static_assert(sizeof(TagInfo) == 0x54, "TagInfo is an invalid size");

    enum class State : u32 {
        NonInitialized = 0,
        Initialized = 1,
    };

    enum class DeviceState : u32 {
        Initialized = 0,
        SearchingForTag = 1,
        TagFound = 2,
        TagRemoved = 3,
        TagNearby = 4,
        Unknown5 = 5,
        Finalized = 6
    };

    struct CommonInfo {
        u16_be last_write_year;
        u8 last_write_month;
        u8 last_write_day;
        u16_be write_counter;
        u16_be version;
        u32_be application_area_size;
        INSERT_PADDING_BYTES(0x34);
    };
    static_assert(sizeof(CommonInfo) == 0x40, "CommonInfo is an invalid size");

    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0};
        rb.Push(ResultSuccess);

        state = State::Initialized;
    }

    void GetState(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFC, "called");

        IPC::ResponseBuilder rb{ctx, 3, 0};
        rb.Push(ResultSuccess);
        rb.PushRaw<u32>(static_cast<u32>(state));
    }

    void ListDevices(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u32 array_size = rp.Pop<u32>();
        LOG_DEBUG(Service_NFP, "called, array_size={}", array_size);

        ctx.WriteBuffer(device_handle);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(1);
    }

    void GetNpadId(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 dev_handle = rp.Pop<u64>();
        LOG_DEBUG(Service_NFP, "called, dev_handle=0x{:X}", dev_handle);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(npad_id);
    }

    void AttachActivateEvent(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 dev_handle = rp.Pop<u64>();
        LOG_DEBUG(Service_NFP, "called, dev_handle=0x{:X}", dev_handle);

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(nfp_interface.GetNFCEvent());
        has_attached_handle = true;
    }

    void AttachDeactivateEvent(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const u64 dev_handle = rp.Pop<u64>();
        LOG_DEBUG(Service_NFP, "called, dev_handle=0x{:X}", dev_handle);

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(deactivate_event->GetReadableEvent());
    }

    void StopDetection(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        switch (device_state) {
        case DeviceState::TagFound:
        case DeviceState::TagNearby:
            deactivate_event->GetWritableEvent().Signal();
            device_state = DeviceState::Initialized;
            break;
        case DeviceState::SearchingForTag:
        case DeviceState::TagRemoved:
            device_state = DeviceState::Initialized;
            break;
        default:
            break;
        }
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetDeviceState(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(static_cast<u32>(device_state));
    }

    void StartDetection(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        if (device_state == DeviceState::Initialized || device_state == DeviceState::TagRemoved) {
            device_state = DeviceState::SearchingForTag;
        }
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetTagInfo(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        IPC::ResponseBuilder rb{ctx, 2};
        const auto& amiibo = nfp_interface.GetAmiiboBuffer();
        const TagInfo tag_info{
            .uuid = amiibo.uuid,
            .uuid_length = static_cast<u8>(amiibo.uuid.size()),
            .padding_1 = {},
            .protocol = 1, // TODO(ogniK): Figure out actual values
            .tag_type = 2,
            .padding_2 = {},
        };
        ctx.WriteBuffer(tag_info);
        rb.Push(ResultSuccess);
    }

    void Mount(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        device_state = DeviceState::TagNearby;
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetModelInfo(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        IPC::ResponseBuilder rb{ctx, 2};
        const auto& amiibo = nfp_interface.GetAmiiboBuffer();
        ctx.WriteBuffer(amiibo.model_info);
        rb.Push(ResultSuccess);
    }

    void Unmount(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        device_state = DeviceState::TagFound;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Finalize(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        device_state = DeviceState::Finalized;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void AttachAvailabilityChangeEvent(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFP, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(availability_change_event->GetReadableEvent());
    }

    void GetRegisterInfo(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFP, "(STUBBED) called");

        // TODO(ogniK): Pull Mii and owner data from amiibo

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetCommonInfo(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFP, "(STUBBED) called");

        // TODO(ogniK): Pull common information from amiibo

        CommonInfo common_info{};
        common_info.application_area_size = 0;
        ctx.WriteBuffer(common_info);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void OpenApplicationArea(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFP, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ErrCodes::ERR_NO_APPLICATION_AREA);
    }

    void GetApplicationAreaSize(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFP, "(STUBBED) called");
        // We don't need to worry about this since we can just open the file
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushRaw<u32>(0); // This is from the GetCommonInfo stub
    }

    void GetApplicationArea(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_NFP, "(STUBBED) called");

        // TODO(ogniK): Pull application area from amiibo

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushRaw<u32>(0); // This is from the GetCommonInfo stub
    }

    Module::Interface& nfp_interface;
    KernelHelpers::ServiceContext& service_context;

    bool has_attached_handle{};
    const u64 device_handle{0}; // Npad device 1
    const u32 npad_id{0};       // Player 1 controller
    State state{State::NonInitialized};
    DeviceState device_state{DeviceState::Initialized};
    Kernel::KEvent* deactivate_event;
    Kernel::KEvent* availability_change_event;
};

void Module::Interface::CreateUserInterface(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NFP, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IUser>(*this, system, service_context);
}

bool Module::Interface::LoadAmiibo(const std::vector<u8>& buffer) {
    std::lock_guard lock{HLE::g_hle_lock};
    if (buffer.size() < sizeof(AmiiboFile)) {
        return false;
    }

    std::memcpy(&amiibo, buffer.data(), sizeof(amiibo));
    nfc_tag_load->GetWritableEvent().Signal();
    return true;
}

Kernel::KReadableEvent& Module::Interface::GetNFCEvent() {
    return nfc_tag_load->GetReadableEvent();
}

const Module::Interface::AmiiboFile& Module::Interface::GetAmiiboBuffer() const {
    return amiibo;
}

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    auto module = std::make_shared<Module>();
    std::make_shared<NFP_User>(module, system)->InstallAsService(service_manager);
}

} // namespace Service::NFP
