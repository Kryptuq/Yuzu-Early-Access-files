// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <utility>

#include "common/logging/log.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/vfs_offset.h"
#include "core/loader/loader.h"

namespace FileSys {

bool PartitionFilesystem::Header::HasValidMagicValue() const {
    return magic == Common::MakeMagic('H', 'F', 'S', '0') ||
           magic == Common::MakeMagic('P', 'F', 'S', '0');
}

PartitionFilesystem::PartitionFilesystem(VirtualFile file) {
    // At least be as large as the header
    if (file->GetSize() < sizeof(Header)) {
        status = Loader::ResultStatus::ErrorBadPFSHeader;
        return;
    }

    // For cartridges, HFSs can get very large, so we need to calculate the size up to
    // the actual content itself instead of just blindly reading in the entire file.
    if (sizeof(Header) != file->ReadObject(&pfs_header)) {
        status = Loader::ResultStatus::ErrorBadPFSHeader;
        return;
    }

    if (!pfs_header.HasValidMagicValue()) {
        status = Loader::ResultStatus::ErrorBadPFSHeader;
        return;
    }

    is_hfs = pfs_header.magic == Common::MakeMagic('H', 'F', 'S', '0');

    std::size_t entry_size = is_hfs ? sizeof(HFSEntry) : sizeof(PFSEntry);
    std::size_t metadata_size =
        sizeof(Header) + (pfs_header.num_entries * entry_size) + pfs_header.strtab_size;

    // Actually read in now...
    std::vector<u8> file_data = file->ReadBytes(metadata_size);
    const std::size_t total_size = file_data.size();

    if (total_size != metadata_size) {
        status = Loader::ResultStatus::ErrorIncorrectPFSFileSize;
        return;
    }

    std::size_t entries_offset = sizeof(Header);
    std::size_t strtab_offset = entries_offset + (pfs_header.num_entries * entry_size);
    content_offset = strtab_offset + pfs_header.strtab_size;
    for (u16 i = 0; i < pfs_header.num_entries; i++) {
        FSEntry entry;

        memcpy(&entry, &file_data[entries_offset + (i * entry_size)], sizeof(FSEntry));
        std::string name(
            reinterpret_cast<const char*>(&file_data[strtab_offset + entry.strtab_offset]));

        offsets.insert_or_assign(name, content_offset + entry.offset);
        sizes.insert_or_assign(name, entry.size);

        pfs_files.emplace_back(std::make_shared<OffsetVfsFile>(
            file, entry.size, content_offset + entry.offset, std::move(name)));
    }

    status = Loader::ResultStatus::Success;
}

PartitionFilesystem::~PartitionFilesystem() = default;

Loader::ResultStatus PartitionFilesystem::GetStatus() const {
    return status;
}

std::map<std::string, u64> PartitionFilesystem::GetFileOffsets() const {
    return offsets;
}

std::map<std::string, u64> PartitionFilesystem::GetFileSizes() const {
    return sizes;
}

std::vector<VirtualFile> PartitionFilesystem::GetFiles() const {
    return pfs_files;
}

std::vector<VirtualDir> PartitionFilesystem::GetSubdirectories() const {
    return {};
}

std::string PartitionFilesystem::GetName() const {
    return is_hfs ? "HFS0" : "PFS0";
}

VirtualDir PartitionFilesystem::GetParentDirectory() const {
    // TODO(DarkLordZach): Add support for nested containers.
    return nullptr;
}

void PartitionFilesystem::PrintDebugInfo() const {
    LOG_DEBUG(Service_FS, "Magic:                  {:.4}", pfs_header.magic);
    LOG_DEBUG(Service_FS, "Files:                  {}", pfs_header.num_entries);
    for (u32 i = 0; i < pfs_header.num_entries; i++) {
        LOG_DEBUG(Service_FS, " > File {}:              {} (0x{:X} bytes)", i,
                  pfs_files[i]->GetName(), pfs_files[i]->GetSize());
    }
}
} // namespace FileSys
