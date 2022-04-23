// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

#include "common/logging/log.h"
#include "core/crypto/aes_util.h"
#include "core/crypto/ctr_encryption_layer.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/nca_patch.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/vfs_offset.h"
#include "core/loader/loader.h"

namespace FileSys {

// Media offsets in headers are stored divided by 512. Mult. by this to get real offset.
constexpr u64 MEDIA_OFFSET_MULTIPLIER = 0x200;

constexpr u64 SECTION_HEADER_SIZE = 0x200;
constexpr u64 SECTION_HEADER_OFFSET = 0x400;

constexpr u32 IVFC_MAX_LEVEL = 6;

enum class NCASectionFilesystemType : u8 {
    PFS0 = 0x2,
    ROMFS = 0x3,
};

struct IVFCLevel {
    u64_le offset;
    u64_le size;
    u32_le block_size;
    u32_le reserved;
};
static_assert(sizeof(IVFCLevel) == 0x18, "IVFCLevel has incorrect size.");

struct IVFCHeader {
    u32_le magic;
    u32_le magic_number;
    INSERT_PADDING_BYTES_NOINIT(8);
    std::array<IVFCLevel, 6> levels;
    INSERT_PADDING_BYTES_NOINIT(64);
};
static_assert(sizeof(IVFCHeader) == 0xE0, "IVFCHeader has incorrect size.");

struct NCASectionHeaderBlock {
    INSERT_PADDING_BYTES_NOINIT(3);
    NCASectionFilesystemType filesystem_type;
    NCASectionCryptoType crypto_type;
    INSERT_PADDING_BYTES_NOINIT(3);
};
static_assert(sizeof(NCASectionHeaderBlock) == 0x8, "NCASectionHeaderBlock has incorrect size.");

struct NCASectionRaw {
    NCASectionHeaderBlock header;
    std::array<u8, 0x138> block_data;
    std::array<u8, 0x8> section_ctr;
    INSERT_PADDING_BYTES_NOINIT(0xB8);
};
static_assert(sizeof(NCASectionRaw) == 0x200, "NCASectionRaw has incorrect size.");

struct PFS0Superblock {
    NCASectionHeaderBlock header_block;
    std::array<u8, 0x20> hash;
    u32_le size;
    INSERT_PADDING_BYTES_NOINIT(4);
    u64_le hash_table_offset;
    u64_le hash_table_size;
    u64_le pfs0_header_offset;
    u64_le pfs0_size;
    INSERT_PADDING_BYTES_NOINIT(0x1B0);
};
static_assert(sizeof(PFS0Superblock) == 0x200, "PFS0Superblock has incorrect size.");

struct RomFSSuperblock {
    NCASectionHeaderBlock header_block;
    IVFCHeader ivfc;
    INSERT_PADDING_BYTES_NOINIT(0x118);
};
static_assert(sizeof(RomFSSuperblock) == 0x200, "RomFSSuperblock has incorrect size.");

struct BKTRHeader {
    u64_le offset;
    u64_le size;
    u32_le magic;
    INSERT_PADDING_BYTES_NOINIT(0x4);
    u32_le number_entries;
    INSERT_PADDING_BYTES_NOINIT(0x4);
};
static_assert(sizeof(BKTRHeader) == 0x20, "BKTRHeader has incorrect size.");

struct BKTRSuperblock {
    NCASectionHeaderBlock header_block;
    IVFCHeader ivfc;
    INSERT_PADDING_BYTES_NOINIT(0x18);
    BKTRHeader relocation;
    BKTRHeader subsection;
    INSERT_PADDING_BYTES_NOINIT(0xC0);
};
static_assert(sizeof(BKTRSuperblock) == 0x200, "BKTRSuperblock has incorrect size.");

union NCASectionHeader {
    NCASectionRaw raw{};
    PFS0Superblock pfs0;
    RomFSSuperblock romfs;
    BKTRSuperblock bktr;
};
static_assert(sizeof(NCASectionHeader) == 0x200, "NCASectionHeader has incorrect size.");

static bool IsValidNCA(const NCAHeader& header) {
    // TODO(DarkLordZach): Add NCA2/NCA0 support.
    return header.magic == Common::MakeMagic('N', 'C', 'A', '3');
}

NCA::NCA(VirtualFile file_, VirtualFile bktr_base_romfs_, u64 bktr_base_ivfc_offset)
    : file(std::move(file_)),
      bktr_base_romfs(std::move(bktr_base_romfs_)), keys{Core::Crypto::KeyManager::Instance()} {
    if (file == nullptr) {
        status = Loader::ResultStatus::ErrorNullFile;
        return;
    }

    if (sizeof(NCAHeader) != file->ReadObject(&header)) {
        LOG_ERROR(Loader, "File reader errored out during header read.");
        status = Loader::ResultStatus::ErrorBadNCAHeader;
        return;
    }

    if (!HandlePotentialHeaderDecryption()) {
        return;
    }

    has_rights_id = std::ranges::any_of(header.rights_id, [](char c) { return c != '\0'; });

    const std::vector<NCASectionHeader> sections = ReadSectionHeaders();
    is_update = std::ranges::any_of(sections, [](const NCASectionHeader& nca_header) {
        return nca_header.raw.header.crypto_type == NCASectionCryptoType::BKTR;
    });

    if (!ReadSections(sections, bktr_base_ivfc_offset)) {
        return;
    }

    status = Loader::ResultStatus::Success;
}

NCA::~NCA() = default;

bool NCA::CheckSupportedNCA(const NCAHeader& nca_header) {
    if (nca_header.magic == Common::MakeMagic('N', 'C', 'A', '2')) {
        status = Loader::ResultStatus::ErrorNCA2;
        return false;
    }

    if (nca_header.magic == Common::MakeMagic('N', 'C', 'A', '0')) {
        status = Loader::ResultStatus::ErrorNCA0;
        return false;
    }

    return true;
}

bool NCA::HandlePotentialHeaderDecryption() {
    if (IsValidNCA(header)) {
        return true;
    }

    if (!CheckSupportedNCA(header)) {
        return false;
    }

    NCAHeader dec_header{};
    Core::Crypto::AESCipher<Core::Crypto::Key256> cipher(
        keys.GetKey(Core::Crypto::S256KeyType::Header), Core::Crypto::Mode::XTS);
    cipher.XTSTranscode(&header, sizeof(NCAHeader), &dec_header, 0, 0x200,
                        Core::Crypto::Op::Decrypt);
    if (IsValidNCA(dec_header)) {
        header = dec_header;
        encrypted = true;
    } else {
        if (!CheckSupportedNCA(dec_header)) {
            return false;
        }

        if (keys.HasKey(Core::Crypto::S256KeyType::Header)) {
            status = Loader::ResultStatus::ErrorIncorrectHeaderKey;
        } else {
            status = Loader::ResultStatus::ErrorMissingHeaderKey;
        }
        return false;
    }

    return true;
}

std::vector<NCASectionHeader> NCA::ReadSectionHeaders() const {
    const std::ptrdiff_t number_sections =
        std::ranges::count_if(header.section_tables, [](const NCASectionTableEntry& entry) {
            return entry.media_offset > 0;
        });

    std::vector<NCASectionHeader> sections(number_sections);
    const auto length_sections = SECTION_HEADER_SIZE * number_sections;

    if (encrypted) {
        auto raw = file->ReadBytes(length_sections, SECTION_HEADER_OFFSET);
        Core::Crypto::AESCipher<Core::Crypto::Key256> cipher(
            keys.GetKey(Core::Crypto::S256KeyType::Header), Core::Crypto::Mode::XTS);
        cipher.XTSTranscode(raw.data(), length_sections, sections.data(), 2, SECTION_HEADER_SIZE,
                            Core::Crypto::Op::Decrypt);
    } else {
        file->ReadBytes(sections.data(), length_sections, SECTION_HEADER_OFFSET);
    }

    return sections;
}

bool NCA::ReadSections(const std::vector<NCASectionHeader>& sections, u64 bktr_base_ivfc_offset) {
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto& section = sections[i];

        if (section.raw.header.filesystem_type == NCASectionFilesystemType::ROMFS) {
            if (!ReadRomFSSection(section, header.section_tables[i], bktr_base_ivfc_offset)) {
                return false;
            }
        } else if (section.raw.header.filesystem_type == NCASectionFilesystemType::PFS0) {
            if (!ReadPFS0Section(section, header.section_tables[i])) {
                return false;
            }
        }
    }

    return true;
}

bool NCA::ReadRomFSSection(const NCASectionHeader& section, const NCASectionTableEntry& entry,
                           u64 bktr_base_ivfc_offset) {
    const std::size_t base_offset = entry.media_offset * MEDIA_OFFSET_MULTIPLIER;
    ivfc_offset = section.romfs.ivfc.levels[IVFC_MAX_LEVEL - 1].offset;
    const std::size_t romfs_offset = base_offset + ivfc_offset;
    const std::size_t romfs_size = section.romfs.ivfc.levels[IVFC_MAX_LEVEL - 1].size;
    auto raw = std::make_shared<OffsetVfsFile>(file, romfs_size, romfs_offset);
    auto dec = Decrypt(section, raw, romfs_offset);

    if (dec == nullptr) {
        if (status != Loader::ResultStatus::Success)
            return false;
        if (has_rights_id)
            status = Loader::ResultStatus::ErrorIncorrectTitlekeyOrTitlekek;
        else
            status = Loader::ResultStatus::ErrorIncorrectKeyAreaKey;
        return false;
    }

    if (section.raw.header.crypto_type == NCASectionCryptoType::BKTR) {
        if (section.bktr.relocation.magic != Common::MakeMagic('B', 'K', 'T', 'R') ||
            section.bktr.subsection.magic != Common::MakeMagic('B', 'K', 'T', 'R')) {
            status = Loader::ResultStatus::ErrorBadBKTRHeader;
            return false;
        }

        if (section.bktr.relocation.offset + section.bktr.relocation.size !=
            section.bktr.subsection.offset) {
            status = Loader::ResultStatus::ErrorBKTRSubsectionNotAfterRelocation;
            return false;
        }

        const u64 size = MEDIA_OFFSET_MULTIPLIER * (entry.media_end_offset - entry.media_offset);
        if (section.bktr.subsection.offset + section.bktr.subsection.size != size) {
            status = Loader::ResultStatus::ErrorBKTRSubsectionNotAtEnd;
            return false;
        }

        const u64 offset = section.romfs.ivfc.levels[IVFC_MAX_LEVEL - 1].offset;
        RelocationBlock relocation_block{};
        if (dec->ReadObject(&relocation_block, section.bktr.relocation.offset - offset) !=
            sizeof(RelocationBlock)) {
            status = Loader::ResultStatus::ErrorBadRelocationBlock;
            return false;
        }
        SubsectionBlock subsection_block{};
        if (dec->ReadObject(&subsection_block, section.bktr.subsection.offset - offset) !=
            sizeof(RelocationBlock)) {
            status = Loader::ResultStatus::ErrorBadSubsectionBlock;
            return false;
        }

        std::vector<RelocationBucketRaw> relocation_buckets_raw(
            (section.bktr.relocation.size - sizeof(RelocationBlock)) / sizeof(RelocationBucketRaw));
        if (dec->ReadBytes(relocation_buckets_raw.data(),
                           section.bktr.relocation.size - sizeof(RelocationBlock),
                           section.bktr.relocation.offset + sizeof(RelocationBlock) - offset) !=
            section.bktr.relocation.size - sizeof(RelocationBlock)) {
            status = Loader::ResultStatus::ErrorBadRelocationBuckets;
            return false;
        }

        std::vector<SubsectionBucketRaw> subsection_buckets_raw(
            (section.bktr.subsection.size - sizeof(SubsectionBlock)) / sizeof(SubsectionBucketRaw));
        if (dec->ReadBytes(subsection_buckets_raw.data(),
                           section.bktr.subsection.size - sizeof(SubsectionBlock),
                           section.bktr.subsection.offset + sizeof(SubsectionBlock) - offset) !=
            section.bktr.subsection.size - sizeof(SubsectionBlock)) {
            status = Loader::ResultStatus::ErrorBadSubsectionBuckets;
            return false;
        }

        std::vector<RelocationBucket> relocation_buckets(relocation_buckets_raw.size());
        std::ranges::transform(relocation_buckets_raw, relocation_buckets.begin(),
                               &ConvertRelocationBucketRaw);
        std::vector<SubsectionBucket> subsection_buckets(subsection_buckets_raw.size());
        std::ranges::transform(subsection_buckets_raw, subsection_buckets.begin(),
                               &ConvertSubsectionBucketRaw);

        u32 ctr_low;
        std::memcpy(&ctr_low, section.raw.section_ctr.data(), sizeof(ctr_low));
        subsection_buckets.back().entries.push_back({section.bktr.relocation.offset, {0}, ctr_low});
        subsection_buckets.back().entries.push_back({size, {0}, 0});

        std::optional<Core::Crypto::Key128> key;
        if (encrypted) {
            if (has_rights_id) {
                status = Loader::ResultStatus::Success;
                key = GetTitlekey();
                if (!key) {
                    status = Loader::ResultStatus::ErrorMissingTitlekey;
                    return false;
                }
            } else {
                key = GetKeyAreaKey(NCASectionCryptoType::BKTR);
                if (!key) {
                    status = Loader::ResultStatus::ErrorMissingKeyAreaKey;
                    return false;
                }
            }
        }

        if (bktr_base_romfs == nullptr) {
            status = Loader::ResultStatus::ErrorMissingBKTRBaseRomFS;
            return false;
        }

        auto bktr = std::make_shared<BKTR>(
            bktr_base_romfs, std::make_shared<OffsetVfsFile>(file, romfs_size, base_offset),
            relocation_block, relocation_buckets, subsection_block, subsection_buckets, encrypted,
            encrypted ? *key : Core::Crypto::Key128{}, base_offset, bktr_base_ivfc_offset,
            section.raw.section_ctr);

        // BKTR applies to entire IVFC, so make an offset version to level 6
        files.push_back(std::make_shared<OffsetVfsFile>(
            bktr, romfs_size, section.romfs.ivfc.levels[IVFC_MAX_LEVEL - 1].offset));
    } else {
        files.push_back(std::move(dec));
    }

    romfs = files.back();
    return true;
}

bool NCA::ReadPFS0Section(const NCASectionHeader& section, const NCASectionTableEntry& entry) {
    const u64 offset = (static_cast<u64>(entry.media_offset) * MEDIA_OFFSET_MULTIPLIER) +
                       section.pfs0.pfs0_header_offset;
    const u64 size = MEDIA_OFFSET_MULTIPLIER * (entry.media_end_offset - entry.media_offset);

    auto dec = Decrypt(section, std::make_shared<OffsetVfsFile>(file, size, offset), offset);
    if (dec != nullptr) {
        auto npfs = std::make_shared<PartitionFilesystem>(std::move(dec));

        if (npfs->GetStatus() == Loader::ResultStatus::Success) {
            dirs.push_back(std::move(npfs));
            if (IsDirectoryExeFS(dirs.back()))
                exefs = dirs.back();
            else if (IsDirectoryLogoPartition(dirs.back()))
                logo = dirs.back();
        } else {
            if (has_rights_id)
                status = Loader::ResultStatus::ErrorIncorrectTitlekeyOrTitlekek;
            else
                status = Loader::ResultStatus::ErrorIncorrectKeyAreaKey;
            return false;
        }
    } else {
        if (status != Loader::ResultStatus::Success)
            return false;
        if (has_rights_id)
            status = Loader::ResultStatus::ErrorIncorrectTitlekeyOrTitlekek;
        else
            status = Loader::ResultStatus::ErrorIncorrectKeyAreaKey;
        return false;
    }

    return true;
}

u8 NCA::GetCryptoRevision() const {
    u8 master_key_id = header.crypto_type;
    if (header.crypto_type_2 > master_key_id)
        master_key_id = header.crypto_type_2;
    if (master_key_id > 0)
        --master_key_id;
    return master_key_id;
}

std::optional<Core::Crypto::Key128> NCA::GetKeyAreaKey(NCASectionCryptoType type) const {
    const auto master_key_id = GetCryptoRevision();

    if (!keys.HasKey(Core::Crypto::S128KeyType::KeyArea, master_key_id, header.key_index)) {
        return std::nullopt;
    }

    std::vector<u8> key_area(header.key_area.begin(), header.key_area.end());
    Core::Crypto::AESCipher<Core::Crypto::Key128> cipher(
        keys.GetKey(Core::Crypto::S128KeyType::KeyArea, master_key_id, header.key_index),
        Core::Crypto::Mode::ECB);
    cipher.Transcode(key_area.data(), key_area.size(), key_area.data(), Core::Crypto::Op::Decrypt);

    Core::Crypto::Key128 out;
    if (type == NCASectionCryptoType::XTS) {
        std::copy(key_area.begin(), key_area.begin() + 0x10, out.begin());
    } else if (type == NCASectionCryptoType::CTR || type == NCASectionCryptoType::BKTR) {
        std::copy(key_area.begin() + 0x20, key_area.begin() + 0x30, out.begin());
    } else {
        LOG_CRITICAL(Crypto, "Called GetKeyAreaKey on invalid NCASectionCryptoType type={:02X}",
                     type);
    }

    u128 out_128{};
    std::memcpy(out_128.data(), out.data(), sizeof(u128));
    LOG_TRACE(Crypto, "called with crypto_rev={:02X}, kak_index={:02X}, key={:016X}{:016X}",
              master_key_id, header.key_index, out_128[1], out_128[0]);

    return out;
}

std::optional<Core::Crypto::Key128> NCA::GetTitlekey() {
    const auto master_key_id = GetCryptoRevision();

    u128 rights_id{};
    memcpy(rights_id.data(), header.rights_id.data(), 16);
    if (rights_id == u128{}) {
        status = Loader::ResultStatus::ErrorInvalidRightsID;
        return std::nullopt;
    }

    auto titlekey = keys.GetKey(Core::Crypto::S128KeyType::Titlekey, rights_id[1], rights_id[0]);
    if (titlekey == Core::Crypto::Key128{}) {
        status = Loader::ResultStatus::ErrorMissingTitlekey;
        return std::nullopt;
    }

    if (!keys.HasKey(Core::Crypto::S128KeyType::Titlekek, master_key_id)) {
        status = Loader::ResultStatus::ErrorMissingTitlekek;
        return std::nullopt;
    }

    Core::Crypto::AESCipher<Core::Crypto::Key128> cipher(
        keys.GetKey(Core::Crypto::S128KeyType::Titlekek, master_key_id), Core::Crypto::Mode::ECB);
    cipher.Transcode(titlekey.data(), titlekey.size(), titlekey.data(), Core::Crypto::Op::Decrypt);

    return titlekey;
}

VirtualFile NCA::Decrypt(const NCASectionHeader& s_header, VirtualFile in, u64 starting_offset) {
    if (!encrypted)
        return in;

    switch (s_header.raw.header.crypto_type) {
    case NCASectionCryptoType::NONE:
        LOG_TRACE(Crypto, "called with mode=NONE");
        return in;
    case NCASectionCryptoType::CTR:
    // During normal BKTR decryption, this entire function is skipped. This is for the metadata,
    // which uses the same CTR as usual.
    case NCASectionCryptoType::BKTR:
        LOG_TRACE(Crypto, "called with mode=CTR, starting_offset={:016X}", starting_offset);
        {
            std::optional<Core::Crypto::Key128> key;
            if (has_rights_id) {
                status = Loader::ResultStatus::Success;
                key = GetTitlekey();
                if (!key) {
                    if (status == Loader::ResultStatus::Success)
                        status = Loader::ResultStatus::ErrorMissingTitlekey;
                    return nullptr;
                }
            } else {
                key = GetKeyAreaKey(NCASectionCryptoType::CTR);
                if (!key) {
                    status = Loader::ResultStatus::ErrorMissingKeyAreaKey;
                    return nullptr;
                }
            }

            auto out = std::make_shared<Core::Crypto::CTREncryptionLayer>(std::move(in), *key,
                                                                          starting_offset);
            Core::Crypto::CTREncryptionLayer::IVData iv{};
            for (std::size_t i = 0; i < 8; ++i) {
                iv[i] = s_header.raw.section_ctr[8 - i - 1];
            }
            out->SetIV(iv);
            return std::static_pointer_cast<VfsFile>(out);
        }
    case NCASectionCryptoType::XTS:
        // TODO(DarkLordZach): Find a test case for XTS-encrypted NCAs
    default:
        LOG_ERROR(Crypto, "called with unhandled crypto type={:02X}",
                  s_header.raw.header.crypto_type);
        return nullptr;
    }
}

Loader::ResultStatus NCA::GetStatus() const {
    return status;
}

std::vector<VirtualFile> NCA::GetFiles() const {
    if (status != Loader::ResultStatus::Success) {
        return {};
    }
    return files;
}

std::vector<VirtualDir> NCA::GetSubdirectories() const {
    if (status != Loader::ResultStatus::Success) {
        return {};
    }
    return dirs;
}

std::string NCA::GetName() const {
    return file->GetName();
}

VirtualDir NCA::GetParentDirectory() const {
    return file->GetContainingDirectory();
}

NCAContentType NCA::GetType() const {
    return header.content_type;
}

u64 NCA::GetTitleId() const {
    if (is_update || status == Loader::ResultStatus::ErrorMissingBKTRBaseRomFS)
        return header.title_id | 0x800;
    return header.title_id;
}

std::array<u8, 16> NCA::GetRightsId() const {
    return header.rights_id;
}

u32 NCA::GetSDKVersion() const {
    return header.sdk_version;
}

bool NCA::IsUpdate() const {
    return is_update;
}

VirtualFile NCA::GetRomFS() const {
    return romfs;
}

VirtualDir NCA::GetExeFS() const {
    return exefs;
}

VirtualFile NCA::GetBaseFile() const {
    return file;
}

u64 NCA::GetBaseIVFCOffset() const {
    return ivfc_offset;
}

VirtualDir NCA::GetLogoPartition() const {
    return logo;
}

} // namespace FileSys
