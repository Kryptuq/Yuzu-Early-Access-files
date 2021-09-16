// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/file_sys/directory.h"
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace FileSys {
class BISFactory;
class RegisteredCache;
class RegisteredCacheUnion;
class PlaceholderCache;
class RomFSFactory;
class SaveDataFactory;
class SDMCFactory;
class XCI;

enum class BisPartitionId : u32;
enum class ContentRecordType : u8;
enum class Mode : u32;
enum class SaveDataSpaceId : u8;
enum class SaveDataType : u8;
enum class StorageId : u8;

struct SaveDataAttribute;
struct SaveDataSize;
} // namespace FileSys

namespace Service {

namespace SM {
class ServiceManager;
} // namespace SM

namespace FileSystem {

enum class ContentStorageId : u32 {
    System,
    User,
    SdCard,
};

enum class ImageDirectoryId : u32 {
    NAND,
    SdCard,
};

class FileSystemController {
public:
    explicit FileSystemController(Core::System& system_);
    ~FileSystemController();

    ResultCode RegisterRomFS(std::unique_ptr<FileSys::RomFSFactory>&& factory);
    ResultCode RegisterSaveData(std::unique_ptr<FileSys::SaveDataFactory>&& factory);
    ResultCode RegisterSDMC(std::unique_ptr<FileSys::SDMCFactory>&& factory);
    ResultCode RegisterBIS(std::unique_ptr<FileSys::BISFactory>&& factory);

    void SetPackedUpdate(FileSys::VirtualFile update_raw);
    ResultVal<FileSys::VirtualFile> OpenRomFSCurrentProcess() const;
    ResultVal<FileSys::VirtualFile> OpenPatchedRomFS(u64 title_id,
                                                     FileSys::ContentRecordType type) const;
    ResultVal<FileSys::VirtualFile> OpenPatchedRomFSWithProgramIndex(
        u64 title_id, u8 program_index, FileSys::ContentRecordType type) const;
    ResultVal<FileSys::VirtualFile> OpenRomFS(u64 title_id, FileSys::StorageId storage_id,
                                              FileSys::ContentRecordType type) const;
    ResultVal<FileSys::VirtualDir> CreateSaveData(
        FileSys::SaveDataSpaceId space, const FileSys::SaveDataAttribute& save_struct) const;
    ResultVal<FileSys::VirtualDir> OpenSaveData(
        FileSys::SaveDataSpaceId space, const FileSys::SaveDataAttribute& save_struct) const;
    ResultVal<FileSys::VirtualDir> OpenSaveDataSpace(FileSys::SaveDataSpaceId space) const;
    ResultVal<FileSys::VirtualDir> OpenSDMC() const;
    ResultVal<FileSys::VirtualDir> OpenBISPartition(FileSys::BisPartitionId id) const;
    ResultVal<FileSys::VirtualFile> OpenBISPartitionStorage(FileSys::BisPartitionId id) const;

    u64 GetFreeSpaceSize(FileSys::StorageId id) const;
    u64 GetTotalSpaceSize(FileSys::StorageId id) const;

    FileSys::SaveDataSize ReadSaveDataSize(FileSys::SaveDataType type, u64 title_id,
                                           u128 user_id) const;
    void WriteSaveDataSize(FileSys::SaveDataType type, u64 title_id, u128 user_id,
                           FileSys::SaveDataSize new_value) const;

    void SetGameCard(FileSys::VirtualFile file);
    FileSys::XCI* GetGameCard() const;

    FileSys::RegisteredCache* GetSystemNANDContents() const;
    FileSys::RegisteredCache* GetUserNANDContents() const;
    FileSys::RegisteredCache* GetSDMCContents() const;
    FileSys::RegisteredCache* GetGameCardContents() const;

    FileSys::PlaceholderCache* GetSystemNANDPlaceholder() const;
    FileSys::PlaceholderCache* GetUserNANDPlaceholder() const;
    FileSys::PlaceholderCache* GetSDMCPlaceholder() const;
    FileSys::PlaceholderCache* GetGameCardPlaceholder() const;

    FileSys::RegisteredCache* GetRegisteredCacheForStorage(FileSys::StorageId id) const;
    FileSys::PlaceholderCache* GetPlaceholderCacheForStorage(FileSys::StorageId id) const;

    FileSys::VirtualDir GetSystemNANDContentDirectory() const;
    FileSys::VirtualDir GetUserNANDContentDirectory() const;
    FileSys::VirtualDir GetSDMCContentDirectory() const;

    FileSys::VirtualDir GetNANDImageDirectory() const;
    FileSys::VirtualDir GetSDMCImageDirectory() const;

    FileSys::VirtualDir GetContentDirectory(ContentStorageId id) const;
    FileSys::VirtualDir GetImageDirectory(ImageDirectoryId id) const;

    FileSys::VirtualDir GetSDMCModificationLoadRoot(u64 title_id) const;
    FileSys::VirtualDir GetModificationLoadRoot(u64 title_id) const;
    FileSys::VirtualDir GetModificationDumpRoot(u64 title_id) const;

    FileSys::VirtualDir GetBCATDirectory(u64 title_id) const;

    void SetAutoSaveDataCreation(bool enable);

    // Creates the SaveData, SDMC, and BIS Factories. Should be called once and before any function
    // above is called.
    void CreateFactories(FileSys::VfsFilesystem& vfs, bool overwrite = true);

private:
    std::unique_ptr<FileSys::RomFSFactory> romfs_factory;
    std::unique_ptr<FileSys::SaveDataFactory> save_data_factory;
    std::unique_ptr<FileSys::SDMCFactory> sdmc_factory;
    std::unique_ptr<FileSys::BISFactory> bis_factory;

    std::unique_ptr<FileSys::XCI> gamecard;
    std::unique_ptr<FileSys::RegisteredCache> gamecard_registered;
    std::unique_ptr<FileSys::PlaceholderCache> gamecard_placeholder;

    Core::System& system;
};

void InstallInterfaces(Core::System& system);

// A class that wraps a VfsDirectory with methods that return ResultVal and ResultCode instead of
// pointers and booleans. This makes using a VfsDirectory with switch services much easier and
// avoids repetitive code.
class VfsDirectoryServiceWrapper {
public:
    explicit VfsDirectoryServiceWrapper(FileSys::VirtualDir backing);
    ~VfsDirectoryServiceWrapper();

    /**
     * Get a descriptive name for the archive (e.g. "RomFS", "SaveData", etc.)
     */
    std::string GetName() const;

    /**
     * Create a file specified by its path
     * @param path Path relative to the Archive
     * @param size The size of the new file, filled with zeroes
     * @return Result of the operation
     */
    ResultCode CreateFile(const std::string& path, u64 size) const;

    /**
     * Delete a file specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode DeleteFile(const std::string& path) const;

    /**
     * Create a directory specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode CreateDirectory(const std::string& path) const;

    /**
     * Delete a directory specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode DeleteDirectory(const std::string& path) const;

    /**
     * Delete a directory specified by its path and anything under it
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    ResultCode DeleteDirectoryRecursively(const std::string& path) const;

    /**
     * Cleans the specified directory. This is similar to DeleteDirectoryRecursively,
     * in that it deletes all the contents of the specified directory, however, this
     * function does *not* delete the directory itself. It only deletes everything
     * within it.
     *
     * @param path Path relative to the archive.
     *
     * @return Result of the operation.
     */
    ResultCode CleanDirectoryRecursively(const std::string& path) const;

    /**
     * Rename a File specified by its path
     * @param src_path Source path relative to the archive
     * @param dest_path Destination path relative to the archive
     * @return Result of the operation
     */
    ResultCode RenameFile(const std::string& src_path, const std::string& dest_path) const;

    /**
     * Rename a Directory specified by its path
     * @param src_path Source path relative to the archive
     * @param dest_path Destination path relative to the archive
     * @return Result of the operation
     */
    ResultCode RenameDirectory(const std::string& src_path, const std::string& dest_path) const;

    /**
     * Open a file specified by its path, using the specified mode
     * @param path Path relative to the archive
     * @param mode Mode to open the file with
     * @return Opened file, or error code
     */
    ResultVal<FileSys::VirtualFile> OpenFile(const std::string& path, FileSys::Mode mode) const;

    /**
     * Open a directory specified by its path
     * @param path Path relative to the archive
     * @return Opened directory, or error code
     */
    ResultVal<FileSys::VirtualDir> OpenDirectory(const std::string& path);

    /**
     * Get the type of the specified path
     * @return The type of the specified path or error code
     */
    ResultVal<FileSys::EntryType> GetEntryType(const std::string& path) const;

    /**
     * Get the timestamp of the specified path
     * @return The timestamp of the specified path or error code
     */
    ResultVal<FileSys::FileTimeStampRaw> GetFileTimeStampRaw(const std::string& path) const;

private:
    FileSys::VirtualDir backing;
};

} // namespace FileSystem
} // namespace Service
