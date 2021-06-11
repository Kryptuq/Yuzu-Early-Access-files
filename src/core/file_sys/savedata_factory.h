// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace FileSys {

enum class SaveDataSpaceId : u8 {
    NandSystem = 0,
    NandUser = 1,
    SdCardSystem = 2,
    TemporaryStorage = 3,
    SdCardUser = 4,
    ProperSystem = 100,
    SafeMode = 101,
};

enum class SaveDataType : u8 {
    SystemSaveData = 0,
    SaveData = 1,
    BcatDeliveryCacheStorage = 2,
    DeviceSaveData = 3,
    TemporaryStorage = 4,
    CacheStorage = 5,
    SystemBcat = 6,
};

enum class SaveDataRank : u8 {
    Primary = 0,
    Secondary = 1,
};

enum class SaveDataFlags : u32 {
    None = (0 << 0),
    KeepAfterResettingSystemSaveData = (1 << 0),
    KeepAfterRefurbishment = (1 << 1),
    KeepAfterResettingSystemSaveDataWithoutUserSaveData = (1 << 2),
    NeedsSecureDelete = (1 << 3),
};

struct SaveDataAttribute {
    u64 title_id;
    u128 user_id;
    u64 save_id;
    SaveDataType type;
    SaveDataRank rank;
    u16 index;
    INSERT_PADDING_BYTES_NOINIT(4);
    u64 zero_1;
    u64 zero_2;
    u64 zero_3;

    std::string DebugInfo() const;
};
static_assert(sizeof(SaveDataAttribute) == 0x40, "SaveDataAttribute has incorrect size.");

struct SaveDataExtraData {
    SaveDataAttribute attr;
    u64 owner_id;
    s64 timestamp;
    SaveDataFlags flags;
    INSERT_PADDING_BYTES_NOINIT(4);
    s64 available_size;
    s64 journal_size;
    s64 commit_id;
    std::array<u8, 0x190> unused;
};
static_assert(sizeof(SaveDataExtraData) == 0x200, "SaveDataExtraData has incorrect size.");

struct SaveDataSize {
    u64 normal;
    u64 journal;
};

/// File system interface to the SaveData archive
class SaveDataFactory {
public:
    explicit SaveDataFactory(Core::System& system_, VirtualDir save_directory_);
    ~SaveDataFactory();

    ResultVal<VirtualDir> Create(SaveDataSpaceId space, const SaveDataAttribute& meta) const;
    ResultVal<VirtualDir> Open(SaveDataSpaceId space, const SaveDataAttribute& meta) const;

    VirtualDir GetSaveDataSpaceDirectory(SaveDataSpaceId space) const;

    static std::string GetSaveDataSpaceIdPath(SaveDataSpaceId space);
    static std::string GetFullPath(Core::System& system, SaveDataSpaceId space, SaveDataType type,
                                   u64 title_id, u128 user_id, u64 save_id);

    SaveDataSize ReadSaveDataSize(SaveDataType type, u64 title_id, u128 user_id) const;
    void WriteSaveDataSize(SaveDataType type, u64 title_id, u128 user_id,
                           SaveDataSize new_value) const;

    void SetAutoCreate(bool state);

private:
    VirtualDir dir;
    Core::System& system;
    bool auto_create{true};
};

} // namespace FileSys
