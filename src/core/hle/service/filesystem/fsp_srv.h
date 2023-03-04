// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "core/hle/service/service.h"

namespace Core {
class Reporter;
}

namespace FileSys {
class ContentProvider;
class FileSystemBackend;
} // namespace FileSys

namespace Service::FileSystem {

enum class AccessLogVersion : u32 {
    V7_0_0 = 2,

    Latest = V7_0_0,
};

enum class AccessLogMode : u32 {
    None,
    Log,
    SdCard,
};

class FSP_SRV final : public ServiceFramework<FSP_SRV> {
public:
    explicit FSP_SRV(Core::System& system_);
    ~FSP_SRV() override;

private:
    void SetCurrentProcess(HLERequestContext& ctx);
    void OpenFileSystemWithPatch(HLERequestContext& ctx);
    void OpenSdCardFileSystem(HLERequestContext& ctx);
    void CreateSaveDataFileSystem(HLERequestContext& ctx);
    void OpenSaveDataFileSystem(HLERequestContext& ctx);
    void OpenReadOnlySaveDataFileSystem(HLERequestContext& ctx);
    void OpenSaveDataInfoReaderBySaveDataSpaceId(HLERequestContext& ctx);
    void WriteSaveDataFileSystemExtraDataBySaveDataAttribute(HLERequestContext& ctx);
    void ReadSaveDataFileSystemExtraDataWithMaskBySaveDataAttribute(HLERequestContext& ctx);
    void OpenDataStorageByCurrentProcess(HLERequestContext& ctx);
    void OpenDataStorageByDataId(HLERequestContext& ctx);
    void OpenPatchDataStorageByCurrentProcess(HLERequestContext& ctx);
    void OpenDataStorageWithProgramIndex(HLERequestContext& ctx);
    void DisableAutoSaveDataCreation(HLERequestContext& ctx);
    void SetGlobalAccessLogMode(HLERequestContext& ctx);
    void GetGlobalAccessLogMode(HLERequestContext& ctx);
    void OutputAccessLogToSdCard(HLERequestContext& ctx);
    void GetProgramIndexForAccessLog(HLERequestContext& ctx);
    void OpenMultiCommitManager(HLERequestContext& ctx);
    void GetCacheStorageSize(HLERequestContext& ctx);

    FileSystemController& fsc;
    const FileSys::ContentProvider& content_provider;
    const Core::Reporter& reporter;

    FileSys::VirtualFile romfs;
    u64 current_process_id = 0;
    u32 access_log_program_index = 0;
    AccessLogMode access_log_mode = AccessLogMode::None;
};

} // namespace Service::FileSystem
