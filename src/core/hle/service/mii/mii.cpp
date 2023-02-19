// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/mii/mii.h"
#include "core/hle/service/mii/mii_manager.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::Mii {

constexpr Result ERROR_INVALID_ARGUMENT{ErrorModule::Mii, 1};

class IDatabaseService final : public ServiceFramework<IDatabaseService> {
public:
    explicit IDatabaseService(Core::System& system_)
        : ServiceFramework{system_, "IDatabaseService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IDatabaseService::IsUpdated, "IsUpdated"},
            {1, &IDatabaseService::IsFullDatabase, "IsFullDatabase"},
            {2, &IDatabaseService::GetCount, "GetCount"},
            {3, &IDatabaseService::Get, "Get"},
            {4, &IDatabaseService::Get1, "Get1"},
            {5, &IDatabaseService::UpdateLatest, "UpdateLatest"},
            {6, &IDatabaseService::BuildRandom, "BuildRandom"},
            {7, &IDatabaseService::BuildDefault, "BuildDefault"},
            {8, nullptr, "Get2"},
            {9, nullptr, "Get3"},
            {10, nullptr, "UpdateLatest1"},
            {11, nullptr, "FindIndex"},
            {12, nullptr, "Move"},
            {13, nullptr, "AddOrReplace"},
            {14, nullptr, "Delete"},
            {15, nullptr, "DestroyFile"},
            {16, nullptr, "DeleteFile"},
            {17, nullptr, "Format"},
            {18, nullptr, "Import"},
            {19, nullptr, "Export"},
            {20, nullptr, "IsBrokenDatabaseWithClearFlag"},
            {21, &IDatabaseService::GetIndex, "GetIndex"},
            {22, &IDatabaseService::SetInterfaceVersion, "SetInterfaceVersion"},
            {23, &IDatabaseService::Convert, "Convert"},
            {24, nullptr, "ConvertCoreDataToCharInfo"},
            {25, nullptr, "ConvertCharInfoToCoreData"},
            {26, nullptr, "Append"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    template <typename T>
    std::vector<u8> SerializeArray(const std::vector<T>& values) {
        std::vector<u8> out(values.size() * sizeof(T));
        std::size_t offset{};
        for (const auto& value : values) {
            std::memcpy(out.data() + offset, &value, sizeof(T));
            offset += sizeof(T);
        }
        return out;
    }

    void IsUpdated(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto source_flag{rp.PopRaw<SourceFlag>()};

        LOG_DEBUG(Service_Mii, "called with source_flag={}", source_flag);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(manager.CheckAndResetUpdateCounter(source_flag, current_update_counter));
    }

    void IsFullDatabase(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Mii, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(manager.IsFullDatabase());
    }

    void GetCount(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto source_flag{rp.PopRaw<SourceFlag>()};

        LOG_DEBUG(Service_Mii, "called with source_flag={}", source_flag);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(manager.GetCount(source_flag));
    }

    void Get(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto source_flag{rp.PopRaw<SourceFlag>()};

        LOG_DEBUG(Service_Mii, "called with source_flag={}", source_flag);

        const auto result{manager.GetDefault(source_flag)};
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        if (result->size() > 0) {
            ctx.WriteBuffer(SerializeArray(*result));
        }

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(static_cast<u32>(result->size()));
    }

    void Get1(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto source_flag{rp.PopRaw<SourceFlag>()};

        LOG_DEBUG(Service_Mii, "called with source_flag={}", source_flag);

        const auto result{manager.GetDefault(source_flag)};
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        std::vector<CharInfo> values;
        for (const auto& element : *result) {
            values.emplace_back(element.info);
        }

        ctx.WriteBuffer(SerializeArray(values));

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(static_cast<u32>(result->size()));
    }

    void UpdateLatest(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto info{rp.PopRaw<CharInfo>()};
        const auto source_flag{rp.PopRaw<SourceFlag>()};

        LOG_DEBUG(Service_Mii, "called with source_flag={}", source_flag);

        const auto result{manager.UpdateLatest(info, source_flag)};
        if (result.Failed()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result.Code());
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2 + sizeof(CharInfo) / sizeof(u32)};
        rb.Push(ResultSuccess);
        rb.PushRaw<CharInfo>(*result);
    }

    void BuildRandom(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto age{rp.PopRaw<Age>()};
        const auto gender{rp.PopRaw<Gender>()};
        const auto race{rp.PopRaw<Race>()};

        LOG_DEBUG(Service_Mii, "called with age={}, gender={}, race={}", age, gender, race);

        if (age > Age::All) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_ARGUMENT);
            LOG_ERROR(Service_Mii, "invalid age={}", age);
            return;
        }

        if (gender > Gender::All) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_ARGUMENT);
            LOG_ERROR(Service_Mii, "invalid gender={}", gender);
            return;
        }

        if (race > Race::All) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_ARGUMENT);
            LOG_ERROR(Service_Mii, "invalid race={}", race);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2 + sizeof(CharInfo) / sizeof(u32)};
        rb.Push(ResultSuccess);
        rb.PushRaw<CharInfo>(manager.BuildRandom(age, gender, race));
    }

    void BuildDefault(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto index{rp.Pop<u32>()};

        LOG_DEBUG(Service_Mii, "called with index={}", index);

        if (index > 5) {
            LOG_ERROR(Service_Mii, "invalid argument, index cannot be greater than 5 but is {:08X}",
                      index);
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_ARGUMENT);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2 + sizeof(CharInfo) / sizeof(u32)};
        rb.Push(ResultSuccess);
        rb.PushRaw<CharInfo>(manager.BuildDefault(index));
    }

    void GetIndex(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto info{rp.PopRaw<CharInfo>()};

        LOG_DEBUG(Service_Mii, "called");

        u32 index{};
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(manager.GetIndex(info, index));
        rb.Push(index);
    }

    void SetInterfaceVersion(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        current_interface_version = rp.PopRaw<u32>();

        LOG_DEBUG(Service_Mii, "called, interface_version={:08X}", current_interface_version);

        UNIMPLEMENTED_IF(current_interface_version != 1);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Convert(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto mii_v3{rp.PopRaw<Ver3StoreData>()};

        LOG_INFO(Service_Mii, "called");

        IPC::ResponseBuilder rb{ctx, 2 + sizeof(CharInfo) / sizeof(u32)};
        rb.Push(ResultSuccess);
        rb.PushRaw<CharInfo>(manager.ConvertV3ToCharInfo(mii_v3));
    }

    constexpr bool IsInterfaceVersionSupported(u32 interface_version) const {
        return current_interface_version >= interface_version;
    }

    MiiManager manager;

    u32 current_interface_version{};
    u64 current_update_counter{};
};

class MiiDBModule final : public ServiceFramework<MiiDBModule> {
public:
    explicit MiiDBModule(Core::System& system_, const char* name_)
        : ServiceFramework{system_, name_} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &MiiDBModule::GetDatabaseService, "GetDatabaseService"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetDatabaseService(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IDatabaseService>(system);

        LOG_DEBUG(Service_Mii, "called");
    }
};

class MiiImg final : public ServiceFramework<MiiImg> {
public:
    explicit MiiImg(Core::System& system_) : ServiceFramework{system_, "miiimg"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {10, nullptr, "Reload"},
            {11, nullptr, "GetCount"},
            {12, nullptr, "IsEmpty"},
            {13, nullptr, "IsFull"},
            {14, nullptr, "GetAttribute"},
            {15, nullptr, "LoadImage"},
            {16, nullptr, "AddOrUpdateImage"},
            {17, nullptr, "DeleteImages"},
            {100, nullptr, "DeleteFile"},
            {101, nullptr, "DestroyFile"},
            {102, nullptr, "ImportFile"},
            {103, nullptr, "ExportFile"},
            {104, nullptr, "ForceInitialize"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("mii:e", std::make_shared<MiiDBModule>(system, "mii:e"));
    server_manager->RegisterNamedService("mii:u", std::make_shared<MiiDBModule>(system, "mii:u"));
    server_manager->RegisterNamedService("miiimg", std::make_shared<MiiImg>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::Mii
