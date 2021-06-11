// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace Kernel {
class KernelCore;
class KEvent;
class KReadableEvent;
} // namespace Kernel

namespace Service::BCAT {

struct DeliveryCacheProgressImpl;

using DirectoryGetter = std::function<FileSys::VirtualDir(u64)>;
using Passphrase = std::array<u8, 0x20>;

struct TitleIDVersion {
    u64 title_id;
    u64 build_id;
};

using DirectoryName = std::array<char, 0x20>;
using FileName = std::array<char, 0x20>;

struct DeliveryCacheProgressImpl {
    enum class Status : s32 {
        None = 0x0,
        Queued = 0x1,
        Connecting = 0x2,
        ProcessingDataList = 0x3,
        Downloading = 0x4,
        Committing = 0x5,
        Done = 0x9,
    };

    Status status;
    ResultCode result = ResultSuccess;
    DirectoryName current_directory;
    FileName current_file;
    s64 current_downloaded_bytes; ///< Bytes downloaded on current file.
    s64 current_total_bytes;      ///< Bytes total on current file.
    s64 total_downloaded_bytes;   ///< Bytes downloaded on overall download.
    s64 total_bytes;              ///< Bytes total on overall download.
    INSERT_PADDING_BYTES(
        0x198); ///< Appears to be unused in official code, possibly reserved for future use.
};
static_assert(sizeof(DeliveryCacheProgressImpl) == 0x200,
              "DeliveryCacheProgressImpl has incorrect size.");

// A class to manage the signalling to the game about BCAT download progress.
// Some of this class is implemented in module.cpp to avoid exposing the implementation structure.
class ProgressServiceBackend {
    friend class IBcatService;

public:
    // Clients should call this with true if any of the functions are going to be called from a
    // non-HLE thread and this class need to lock the hle mutex. (default is false)
    void SetNeedHLELock(bool need);

    // Sets the number of bytes total in the entire download.
    void SetTotalSize(u64 size);

    // Notifies the application that the backend has started connecting to the server.
    void StartConnecting();
    // Notifies the application that the backend has begun accumulating and processing metadata.
    void StartProcessingDataList();

    // Notifies the application that a file is starting to be downloaded.
    void StartDownloadingFile(std::string_view dir_name, std::string_view file_name, u64 file_size);
    // Updates the progress of the current file to the size passed.
    void UpdateFileProgress(u64 downloaded);
    // Notifies the application that the current file has completed download.
    void FinishDownloadingFile();

    // Notifies the application that all files in this directory have completed and are being
    // finalized.
    void CommitDirectory(std::string_view dir_name);

    // Notifies the application that the operation completed with result code result.
    void FinishDownload(ResultCode result);

private:
    explicit ProgressServiceBackend(Kernel::KernelCore& kernel, std::string_view event_name);

    Kernel::KReadableEvent& GetEvent();
    DeliveryCacheProgressImpl& GetImpl();

    void SignalUpdate();

    DeliveryCacheProgressImpl impl{};
    Kernel::KEvent update_event;
    bool need_hle_lock = false;
};

// A class representing an abstract backend for BCAT functionality.
class Backend {
public:
    explicit Backend(DirectoryGetter getter);
    virtual ~Backend();

    // Called when the backend is needed to synchronize the data for the game with title ID and
    // version in title. A ProgressServiceBackend object is provided to alert the application of
    // status.
    virtual bool Synchronize(TitleIDVersion title, ProgressServiceBackend& progress) = 0;
    // Very similar to Synchronize, but only for the directory provided. Backends should not alter
    // the data for any other directories.
    virtual bool SynchronizeDirectory(TitleIDVersion title, std::string name,
                                      ProgressServiceBackend& progress) = 0;

    // Removes all cached data associated with title id provided.
    virtual bool Clear(u64 title_id) = 0;

    // Sets the BCAT Passphrase to be used with the associated title ID.
    virtual void SetPassphrase(u64 title_id, const Passphrase& passphrase) = 0;

    // Gets the launch parameter used by AM associated with the title ID and version provided.
    virtual std::optional<std::vector<u8>> GetLaunchParameter(TitleIDVersion title) = 0;

protected:
    DirectoryGetter dir_getter;
};

// A backend of BCAT that provides no operation.
class NullBackend : public Backend {
public:
    explicit NullBackend(DirectoryGetter getter);
    ~NullBackend() override;

    bool Synchronize(TitleIDVersion title, ProgressServiceBackend& progress) override;
    bool SynchronizeDirectory(TitleIDVersion title, std::string name,
                              ProgressServiceBackend& progress) override;

    bool Clear(u64 title_id) override;

    void SetPassphrase(u64 title_id, const Passphrase& passphrase) override;

    std::optional<std::vector<u8>> GetLaunchParameter(TitleIDVersion title) override;
};

std::unique_ptr<Backend> CreateBackendFromSettings(Core::System& system, DirectoryGetter getter);

} // namespace Service::BCAT
