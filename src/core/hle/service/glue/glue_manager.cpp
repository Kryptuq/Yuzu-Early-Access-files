// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/glue/errors.h"
#include "core/hle/service/glue/glue_manager.h"

namespace Service::Glue {

struct ARPManager::MapEntry {
    ApplicationLaunchProperty launch;
    std::vector<u8> control;
};

ARPManager::ARPManager() = default;

ARPManager::~ARPManager() = default;

ResultVal<ApplicationLaunchProperty> ARPManager::GetLaunchProperty(u64 title_id) const {
    if (title_id == 0) {
        return ERR_INVALID_PROCESS_ID;
    }

    const auto iter = entries.find(title_id);
    if (iter == entries.end()) {
        return ERR_NOT_REGISTERED;
    }

    return iter->second.launch;
}

ResultVal<std::vector<u8>> ARPManager::GetControlProperty(u64 title_id) const {
    if (title_id == 0) {
        return ERR_INVALID_PROCESS_ID;
    }

    const auto iter = entries.find(title_id);
    if (iter == entries.end()) {
        return ERR_NOT_REGISTERED;
    }

    return iter->second.control;
}

ResultCode ARPManager::Register(u64 title_id, ApplicationLaunchProperty launch,
                                std::vector<u8> control) {
    if (title_id == 0) {
        return ERR_INVALID_PROCESS_ID;
    }

    const auto iter = entries.find(title_id);
    if (iter != entries.end()) {
        return ERR_INVALID_ACCESS;
    }

    entries.insert_or_assign(title_id, MapEntry{launch, std::move(control)});
    return ResultSuccess;
}

ResultCode ARPManager::Unregister(u64 title_id) {
    if (title_id == 0) {
        return ERR_INVALID_PROCESS_ID;
    }

    const auto iter = entries.find(title_id);
    if (iter == entries.end()) {
        return ERR_NOT_REGISTERED;
    }

    entries.erase(iter);
    return ResultSuccess;
}

void ARPManager::ResetAll() {
    entries.clear();
}

} // namespace Service::Glue
