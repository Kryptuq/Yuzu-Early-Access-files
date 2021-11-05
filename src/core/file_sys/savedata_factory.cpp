// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/file_sys/savedata_factory.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

constexpr char SAVE_DATA_SIZE_FILENAME[] = ".yuzu_save_size";

namespace {

void PrintSaveDataAttributeWarnings(SaveDataAttribute meta) {
    if (meta.type == SaveDataType::SystemSaveData || meta.type == SaveDataType::SaveData) {
        if (meta.zero_1 != 0) {
            LOG_WARNING(Service_FS,
                        "Possibly incorrect SaveDataAttribute, type is "
                        "SystemSaveData||SaveData but offset 0x28 is non-zero ({:016X}).",
                        meta.zero_1);
        }
        if (meta.zero_2 != 0) {
            LOG_WARNING(Service_FS,
                        "Possibly incorrect SaveDataAttribute, type is "
                        "SystemSaveData||SaveData but offset 0x30 is non-zero ({:016X}).",
                        meta.zero_2);
        }
        if (meta.zero_3 != 0) {
            LOG_WARNING(Service_FS,
                        "Possibly incorrect SaveDataAttribute, type is "
                        "SystemSaveData||SaveData but offset 0x38 is non-zero ({:016X}).",
                        meta.zero_3);
        }
    }

    if (meta.type == SaveDataType::SystemSaveData && meta.title_id != 0) {
        LOG_WARNING(Service_FS,
                    "Possibly incorrect SaveDataAttribute, type is SystemSaveData but title_id is "
                    "non-zero ({:016X}).",
                    meta.title_id);
    }

    if (meta.type == SaveDataType::DeviceSaveData && meta.user_id != u128{0, 0}) {
        LOG_WARNING(Service_FS,
                    "Possibly incorrect SaveDataAttribute, type is DeviceSaveData but user_id is "
                    "non-zero ({:016X}{:016X})",
                    meta.user_id[1], meta.user_id[0]);
    }
}

bool ShouldSaveDataBeAutomaticallyCreated(SaveDataSpaceId space, const SaveDataAttribute& attr) {
    return attr.type == SaveDataType::CacheStorage || attr.type == SaveDataType::TemporaryStorage ||
           (space == SaveDataSpaceId::NandUser && ///< Normal Save Data -- Current Title & User
            (attr.type == SaveDataType::SaveData || attr.type == SaveDataType::DeviceSaveData) &&
            attr.title_id == 0 && attr.save_id == 0);
}

} // Anonymous namespace

std::string SaveDataAttribute::DebugInfo() const {
    return fmt::format("[title_id={:016X}, user_id={:016X}{:016X}, save_id={:016X}, type={:02X}, "
                       "rank={}, index={}]",
                       title_id, user_id[1], user_id[0], save_id, static_cast<u8>(type),
                       static_cast<u8>(rank), index);
}

SaveDataFactory::SaveDataFactory(Core::System& system_, VirtualDir save_directory_)
    : dir{std::move(save_directory_)}, system{system_} {
    // Delete all temporary storages
    // On hardware, it is expected that temporary storage be empty at first use.
    dir->DeleteSubdirectoryRecursive("temp");
}

SaveDataFactory::~SaveDataFactory() = default;

ResultVal<VirtualDir> SaveDataFactory::Create(SaveDataSpaceId space,
                                              const SaveDataAttribute& meta) const {
    PrintSaveDataAttributeWarnings(meta);

    const auto save_directory =
        GetFullPath(system, space, meta.type, meta.title_id, meta.user_id, meta.save_id);

    auto out = dir->CreateDirectoryRelative(save_directory);

    // Return an error if the save data doesn't actually exist.
    if (out == nullptr) {
        // TODO(DarkLordZach): Find out correct error code.
        return ResultUnknown;
    }

    return out;
}

ResultVal<VirtualDir> SaveDataFactory::Open(SaveDataSpaceId space,
                                            const SaveDataAttribute& meta) const {

    const auto save_directory =
        GetFullPath(system, space, meta.type, meta.title_id, meta.user_id, meta.save_id);

    auto out = dir->GetDirectoryRelative(save_directory);

    if (out == nullptr && (ShouldSaveDataBeAutomaticallyCreated(space, meta) && auto_create)) {
        return Create(space, meta);
    }

    // Return an error if the save data doesn't actually exist.
    if (out == nullptr) {
        // TODO(Subv): Find out correct error code.
        return ResultUnknown;
    }

    return out;
}

VirtualDir SaveDataFactory::GetSaveDataSpaceDirectory(SaveDataSpaceId space) const {
    return dir->GetDirectoryRelative(GetSaveDataSpaceIdPath(space));
}

std::string SaveDataFactory::GetSaveDataSpaceIdPath(SaveDataSpaceId space) {
    switch (space) {
    case SaveDataSpaceId::NandSystem:
        return "/system/";
    case SaveDataSpaceId::NandUser:
        return "/user/";
    case SaveDataSpaceId::TemporaryStorage:
        return "/temp/";
    default:
        ASSERT_MSG(false, "Unrecognized SaveDataSpaceId: {:02X}", static_cast<u8>(space));
        return "/unrecognized/"; ///< To prevent corruption when ignoring asserts.
    }
}

std::string SaveDataFactory::GetFullPath(Core::System& system, SaveDataSpaceId space,
                                         SaveDataType type, u64 title_id, u128 user_id,
                                         u64 save_id) {
    // According to switchbrew, if a save is of type SaveData and the title id field is 0, it should
    // be interpreted as the title id of the current process.
    if (type == SaveDataType::SaveData || type == SaveDataType::DeviceSaveData) {
        if (title_id == 0) {
            title_id = system.GetCurrentProcessProgramID();
        }
    }

    std::string out = GetSaveDataSpaceIdPath(space);

    switch (type) {
    case SaveDataType::SystemSaveData:
        return fmt::format("{}save/{:016X}/{:016X}{:016X}", out, save_id, user_id[1], user_id[0]);
    case SaveDataType::SaveData:
    case SaveDataType::DeviceSaveData:
        return fmt::format("{}save/{:016X}/{:016X}{:016X}/{:016X}", out, 0, user_id[1], user_id[0],
                           title_id);
    case SaveDataType::TemporaryStorage:
        return fmt::format("{}{:016X}/{:016X}{:016X}/{:016X}", out, 0, user_id[1], user_id[0],
                           title_id);
    case SaveDataType::CacheStorage:
        return fmt::format("{}save/cache/{:016X}", out, title_id);
    default:
        ASSERT_MSG(false, "Unrecognized SaveDataType: {:02X}", static_cast<u8>(type));
        return fmt::format("{}save/unknown_{:X}/{:016X}", out, static_cast<u8>(type), title_id);
    }
}

SaveDataSize SaveDataFactory::ReadSaveDataSize(SaveDataType type, u64 title_id,
                                               u128 user_id) const {
    const auto path = GetFullPath(system, SaveDataSpaceId::NandUser, type, title_id, user_id, 0);
    const auto relative_dir = GetOrCreateDirectoryRelative(dir, path);

    const auto size_file = relative_dir->GetFile(SAVE_DATA_SIZE_FILENAME);
    if (size_file == nullptr || size_file->GetSize() < sizeof(SaveDataSize)) {
        return {0, 0};
    }

    SaveDataSize out;
    if (size_file->ReadObject(&out) != sizeof(SaveDataSize)) {
        return {0, 0};
    }

    return out;
}

void SaveDataFactory::WriteSaveDataSize(SaveDataType type, u64 title_id, u128 user_id,
                                        SaveDataSize new_value) const {
    const auto path = GetFullPath(system, SaveDataSpaceId::NandUser, type, title_id, user_id, 0);
    const auto relative_dir = GetOrCreateDirectoryRelative(dir, path);

    const auto size_file = relative_dir->CreateFile(SAVE_DATA_SIZE_FILENAME);
    if (size_file == nullptr) {
        return;
    }

    size_file->Resize(sizeof(SaveDataSize));
    size_file->WriteObject(new_value);
}

void SaveDataFactory::SetAutoCreate(bool state) {
    auto_create = state;
}

} // namespace FileSys
