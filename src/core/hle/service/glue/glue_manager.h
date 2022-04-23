// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <vector>
#include "common/common_types.h"
#include "core/file_sys/romfs_factory.h"
#include "core/hle/result.h"

namespace Service::Glue {

struct ApplicationLaunchProperty {
    u64 title_id;
    u32 version;
    FileSys::StorageId base_game_storage_id;
    FileSys::StorageId update_storage_id;
    u8 program_index;
    u8 reserved;
};
static_assert(sizeof(ApplicationLaunchProperty) == 0x10,
              "ApplicationLaunchProperty has incorrect size.");

// A class to manage state related to the arp:w and arp:r services, specifically the registration
// and unregistration of launch and control properties.
class ARPManager {
public:
    ARPManager();
    ~ARPManager();

    // Returns the ApplicationLaunchProperty corresponding to the provided title ID if it was
    // previously registered, otherwise ERR_NOT_REGISTERED if it was never registered or
    // ERR_INVALID_PROCESS_ID if the title ID is 0.
    ResultVal<ApplicationLaunchProperty> GetLaunchProperty(u64 title_id) const;

    // Returns a vector of the raw bytes of NACP data (necessarily 0x4000 in size) corresponding to
    // the provided title ID if it was previously registered, otherwise ERR_NOT_REGISTERED if it was
    // never registered or ERR_INVALID_PROCESS_ID if the title ID is 0.
    ResultVal<std::vector<u8>> GetControlProperty(u64 title_id) const;

    // Adds a new entry to the internal database with the provided parameters, returning
    // ERR_INVALID_ACCESS if attempting to re-register a title ID without an intermediate Unregister
    // step, and ERR_INVALID_PROCESS_ID if the title ID is 0.
    ResultCode Register(u64 title_id, ApplicationLaunchProperty launch, std::vector<u8> control);

    // Removes the registration for the provided title ID from the database, returning
    // ERR_NOT_REGISTERED if it doesn't exist in the database and ERR_INVALID_PROCESS_ID if the
    // title ID is 0.
    ResultCode Unregister(u64 title_id);

    // Removes all entries from the database, always succeeds. Should only be used when resetting
    // system state.
    void ResetAll();

private:
    struct MapEntry;
    std::map<u64, MapEntry> entries;
};

} // namespace Service::Glue
