// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <numeric>
#include <vector>

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/common_funcs.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/aoc/aoc_u.h"
#include "core/loader/loader.h"

namespace Service::AOC {

static bool CheckAOCTitleIDMatchesBase(u64 title_id, u64 base) {
    return FileSys::GetBaseTitleID(title_id) == base;
}

static std::vector<u64> AccumulateAOCTitleIDs(Core::System& system) {
    std::vector<u64> add_on_content;
    const auto& rcu = system.GetContentProvider();
    const auto list =
        rcu.ListEntriesFilter(FileSys::TitleType::AOC, FileSys::ContentRecordType::Data);
    std::transform(list.begin(), list.end(), std::back_inserter(add_on_content),
                   [](const FileSys::ContentProviderEntry& rce) { return rce.title_id; });
    add_on_content.erase(
        std::remove_if(
            add_on_content.begin(), add_on_content.end(),
            [&rcu](u64 tid) {
                return rcu.GetEntry(tid, FileSys::ContentRecordType::Data)->GetStatus() !=
                       Loader::ResultStatus::Success;
            }),
        add_on_content.end());
    return add_on_content;
}

class IPurchaseEventManager final : public ServiceFramework<IPurchaseEventManager> {
public:
    explicit IPurchaseEventManager(Core::System& system_)
        : ServiceFramework{system_, "IPurchaseEventManager"}, purchased_event{system.Kernel()} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IPurchaseEventManager::SetDefaultDeliveryTarget, "SetDefaultDeliveryTarget"},
            {1, &IPurchaseEventManager::SetDeliveryTarget, "SetDeliveryTarget"},
            {2, &IPurchaseEventManager::GetPurchasedEventReadableHandle, "GetPurchasedEventReadableHandle"},
            {3, nullptr, "PopPurchasedProductInfo"},
            {4, nullptr, "PopPurchasedProductInfoWithUid"},
        };
        // clang-format on

        RegisterHandlers(functions);

        Kernel::KAutoObject::Create(std::addressof(purchased_event));
        purchased_event.Initialize("IPurchaseEventManager:PurchasedEvent");
    }

private:
    void SetDefaultDeliveryTarget(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto unknown_1 = rp.Pop<u64>();
        [[maybe_unused]] const auto unknown_2 = ctx.ReadBuffer();

        LOG_WARNING(Service_AOC, "(STUBBED) called, unknown_1={}", unknown_1);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void SetDeliveryTarget(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto unknown_1 = rp.Pop<u64>();
        [[maybe_unused]] const auto unknown_2 = ctx.ReadBuffer();

        LOG_WARNING(Service_AOC, "(STUBBED) called, unknown_1={}", unknown_1);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetPurchasedEventReadableHandle(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_AOC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(purchased_event.GetReadableEvent());
    }

    Kernel::KEvent purchased_event;
};

AOC_U::AOC_U(Core::System& system_)
    : ServiceFramework{system_, "aoc:u"}, add_on_content{AccumulateAOCTitleIDs(system)},
      aoc_change_event{system.Kernel()} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CountAddOnContentByApplicationId"},
        {1, nullptr, "ListAddOnContentByApplicationId"},
        {2, &AOC_U::CountAddOnContent, "CountAddOnContent"},
        {3, &AOC_U::ListAddOnContent, "ListAddOnContent"},
        {4, nullptr, "GetAddOnContentBaseIdByApplicationId"},
        {5, &AOC_U::GetAddOnContentBaseId, "GetAddOnContentBaseId"},
        {6, nullptr, "PrepareAddOnContentByApplicationId"},
        {7, &AOC_U::PrepareAddOnContent, "PrepareAddOnContent"},
        {8, &AOC_U::GetAddOnContentListChangedEvent, "GetAddOnContentListChangedEvent"},
        {9, nullptr, "GetAddOnContentLostErrorCode"},
        {10, &AOC_U::GetAddOnContentListChangedEventWithProcessId, "GetAddOnContentListChangedEventWithProcessId"},
        {100, &AOC_U::CreateEcPurchasedEventManager, "CreateEcPurchasedEventManager"},
        {101, &AOC_U::CreatePermanentEcPurchasedEventManager, "CreatePermanentEcPurchasedEventManager"},
        {110, nullptr, "CreateContentsServiceManager"},
    };
    // clang-format on

    RegisterHandlers(functions);

    Kernel::KAutoObject::Create(std::addressof(aoc_change_event));
    aoc_change_event.Initialize("GetAddOnContentListChanged:Event");
}

AOC_U::~AOC_U() = default;

void AOC_U::CountAddOnContent(Kernel::HLERequestContext& ctx) {
    struct Parameters {
        u64 process_id;
    };
    static_assert(sizeof(Parameters) == 8);

    IPC::RequestParser rp{ctx};
    const auto params = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_AOC, "called. process_id={}", params.process_id);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);

    const auto current = system.CurrentProcess()->GetTitleID();

    const auto& disabled = Settings::values.disabled_addons[current];
    if (std::find(disabled.begin(), disabled.end(), "DLC") != disabled.end()) {
        rb.Push<u32>(0);
        return;
    }

    rb.Push<u32>(static_cast<u32>(
        std::count_if(add_on_content.begin(), add_on_content.end(),
                      [current](u64 tid) { return CheckAOCTitleIDMatchesBase(tid, current); })));
}

void AOC_U::ListAddOnContent(Kernel::HLERequestContext& ctx) {
    struct Parameters {
        u32 offset;
        u32 count;
        u64 process_id;
    };
    static_assert(sizeof(Parameters) == 16);

    IPC::RequestParser rp{ctx};
    const auto [offset, count, process_id] = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_AOC, "called with offset={}, count={}, process_id={}", offset, count,
              process_id);

    const auto current = system.CurrentProcess()->GetTitleID();

    std::vector<u32> out;
    const auto& disabled = Settings::values.disabled_addons[current];
    if (std::find(disabled.begin(), disabled.end(), "DLC") == disabled.end()) {
        for (u64 content_id : add_on_content) {
            if (FileSys::GetBaseTitleID(content_id) != current) {
                continue;
            }

            out.push_back(static_cast<u32>(FileSys::GetAOCID(content_id)));
        }
    }

    if (out.size() < offset) {
        IPC::ResponseBuilder rb{ctx, 2};
        // TODO(DarkLordZach): Find the correct error code.
        rb.Push(ResultUnknown);
        return;
    }

    const auto out_count = static_cast<u32>(std::min<size_t>(out.size() - offset, count));
    std::rotate(out.begin(), out.begin() + offset, out.end());
    out.resize(out_count);

    ctx.WriteBuffer(out);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(out_count);
}

void AOC_U::GetAddOnContentBaseId(Kernel::HLERequestContext& ctx) {
    struct Parameters {
        u64 process_id;
    };
    static_assert(sizeof(Parameters) == 8);

    IPC::RequestParser rp{ctx};
    const auto params = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_AOC, "called. process_id={}", params.process_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);

    const auto title_id = system.CurrentProcess()->GetTitleID();
    const FileSys::PatchManager pm{title_id, system.GetFileSystemController(),
                                   system.GetContentProvider()};

    const auto res = pm.GetControlMetadata();
    if (res.first == nullptr) {
        rb.Push(FileSys::GetAOCBaseTitleID(title_id));
        return;
    }

    rb.Push(res.first->GetDLCBaseTitleId());
}

void AOC_U::PrepareAddOnContent(Kernel::HLERequestContext& ctx) {
    struct Parameters {
        s32 addon_index;
        u64 process_id;
    };
    static_assert(sizeof(Parameters) == 16);

    IPC::RequestParser rp{ctx};
    const auto [addon_index, process_id] = rp.PopRaw<Parameters>();

    LOG_WARNING(Service_AOC, "(STUBBED) called with addon_index={}, process_id={}", addon_index,
                process_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void AOC_U::GetAddOnContentListChangedEvent(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_AOC, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(aoc_change_event.GetReadableEvent());
}

void AOC_U::GetAddOnContentListChangedEventWithProcessId(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_AOC, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(aoc_change_event.GetReadableEvent());
}

void AOC_U::CreateEcPurchasedEventManager(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_AOC, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IPurchaseEventManager>(system);
}

void AOC_U::CreatePermanentEcPurchasedEventManager(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_AOC, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IPurchaseEventManager>(system);
}

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<AOC_U>(system)->InstallAsService(service_manager);
}

} // namespace Service::AOC
