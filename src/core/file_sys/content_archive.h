// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/vfs.h"

namespace Loader {
enum class ResultStatus : u16;
}

namespace FileSys {

union NCASectionHeader;

/// Describes the type of content within an NCA archive.
enum class NCAContentType : u8 {
    /// Executable-related data
    Program = 0,

    /// Metadata.
    Meta = 1,

    /// Access control data.
    Control = 2,

    /// Information related to the game manual
    /// e.g. Legal information, etc.
    Manual = 3,

    /// System data.
    Data = 4,

    /// Data that can be accessed by applications.
    PublicData = 5,
};

enum class NCASectionCryptoType : u8 {
    NONE = 1,
    XTS = 2,
    CTR = 3,
    BKTR = 4,
};

struct NCASectionTableEntry {
    u32_le media_offset;
    u32_le media_end_offset;
    INSERT_PADDING_BYTES(0x8);
};
static_assert(sizeof(NCASectionTableEntry) == 0x10, "NCASectionTableEntry has incorrect size.");

struct NCAHeader {
    std::array<u8, 0x100> rsa_signature_1;
    std::array<u8, 0x100> rsa_signature_2;
    u32_le magic;
    u8 is_system;
    NCAContentType content_type;
    u8 crypto_type;
    u8 key_index;
    u64_le size;
    u64_le title_id;
    INSERT_PADDING_BYTES(0x4);
    u32_le sdk_version;
    u8 crypto_type_2;
    INSERT_PADDING_BYTES(15);
    std::array<u8, 0x10> rights_id;
    std::array<NCASectionTableEntry, 0x4> section_tables;
    std::array<std::array<u8, 0x20>, 0x4> hash_tables;
    std::array<u8, 0x40> key_area;
    INSERT_PADDING_BYTES(0xC0);
};
static_assert(sizeof(NCAHeader) == 0x400, "NCAHeader has incorrect size.");

inline bool IsDirectoryExeFS(const VirtualDir& pfs) {
    // According to switchbrew, an exefs must only contain these two files:
    return pfs->GetFile("main") != nullptr && pfs->GetFile("main.npdm") != nullptr;
}

inline bool IsDirectoryLogoPartition(const VirtualDir& pfs) {
    // NintendoLogo is the static image in the top left corner while StartupMovie is the animation
    // in the bottom right corner.
    return pfs->GetFile("NintendoLogo.png") != nullptr &&
           pfs->GetFile("StartupMovie.gif") != nullptr;
}

// An implementation of VfsDirectory that represents a Nintendo Content Archive (NCA) container.
// After construction, use GetStatus to determine if the file is valid and ready to be used.
class NCA : public ReadOnlyVfsDirectory {
public:
    explicit NCA(VirtualFile file, VirtualFile bktr_base_romfs = nullptr,
                 u64 bktr_base_ivfc_offset = 0);
    ~NCA() override;

    Loader::ResultStatus GetStatus() const;

    std::vector<VirtualFile> GetFiles() const override;
    std::vector<VirtualDir> GetSubdirectories() const override;
    std::string GetName() const override;
    VirtualDir GetParentDirectory() const override;

    NCAContentType GetType() const;
    u64 GetTitleId() const;
    std::array<u8, 0x10> GetRightsId() const;
    u32 GetSDKVersion() const;
    bool IsUpdate() const;

    VirtualFile GetRomFS() const;
    VirtualDir GetExeFS() const;

    VirtualFile GetBaseFile() const;

    // Returns the base ivfc offset used in BKTR patching.
    u64 GetBaseIVFCOffset() const;

    VirtualDir GetLogoPartition() const;

private:
    bool CheckSupportedNCA(const NCAHeader& header);
    bool HandlePotentialHeaderDecryption();

    std::vector<NCASectionHeader> ReadSectionHeaders() const;
    bool ReadSections(const std::vector<NCASectionHeader>& sections, u64 bktr_base_ivfc_offset);
    bool ReadRomFSSection(const NCASectionHeader& section, const NCASectionTableEntry& entry,
                          u64 bktr_base_ivfc_offset);
    bool ReadPFS0Section(const NCASectionHeader& section, const NCASectionTableEntry& entry);

    u8 GetCryptoRevision() const;
    std::optional<Core::Crypto::Key128> GetKeyAreaKey(NCASectionCryptoType type) const;
    std::optional<Core::Crypto::Key128> GetTitlekey();
    VirtualFile Decrypt(const NCASectionHeader& header, VirtualFile in, u64 starting_offset);

    std::vector<VirtualDir> dirs;
    std::vector<VirtualFile> files;

    VirtualFile romfs = nullptr;
    VirtualDir exefs = nullptr;
    VirtualDir logo = nullptr;
    VirtualFile file;
    VirtualFile bktr_base_romfs;
    u64 ivfc_offset = 0;

    NCAHeader header{};
    bool has_rights_id{};

    Loader::ResultStatus status{};

    bool encrypted = false;
    bool is_update = false;

    Core::Crypto::KeyManager& keys;
};

} // namespace FileSys
