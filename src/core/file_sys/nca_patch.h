// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/crypto/key_manager.h"

namespace FileSys {

#pragma pack(push, 1)
struct RelocationEntry {
    u64_le address_patch;
    u64_le address_source;
    u32 from_patch;
};
#pragma pack(pop)
static_assert(sizeof(RelocationEntry) == 0x14, "RelocationEntry has incorrect size.");

struct RelocationBucketRaw {
    INSERT_PADDING_BYTES(4);
    u32_le number_entries;
    u64_le end_offset;
    std::array<RelocationEntry, 0x332> relocation_entries;
    INSERT_PADDING_BYTES(8);
};
static_assert(sizeof(RelocationBucketRaw) == 0x4000, "RelocationBucketRaw has incorrect size.");

// Vector version of RelocationBucketRaw
struct RelocationBucket {
    u32 number_entries;
    u64 end_offset;
    std::vector<RelocationEntry> entries;
};

struct RelocationBlock {
    INSERT_PADDING_BYTES(4);
    u32_le number_buckets;
    u64_le size;
    std::array<u64, 0x7FE> base_offsets;
};
static_assert(sizeof(RelocationBlock) == 0x4000, "RelocationBlock has incorrect size.");

struct SubsectionEntry {
    u64_le address_patch;
    INSERT_PADDING_BYTES(0x4);
    u32_le ctr;
};
static_assert(sizeof(SubsectionEntry) == 0x10, "SubsectionEntry has incorrect size.");

struct SubsectionBucketRaw {
    INSERT_PADDING_BYTES(4);
    u32_le number_entries;
    u64_le end_offset;
    std::array<SubsectionEntry, 0x3FF> subsection_entries;
};
static_assert(sizeof(SubsectionBucketRaw) == 0x4000, "SubsectionBucketRaw has incorrect size.");

// Vector version of SubsectionBucketRaw
struct SubsectionBucket {
    u32 number_entries;
    u64 end_offset;
    std::vector<SubsectionEntry> entries;
};

struct SubsectionBlock {
    INSERT_PADDING_BYTES(4);
    u32_le number_buckets;
    u64_le size;
    std::array<u64, 0x7FE> base_offsets;
};
static_assert(sizeof(SubsectionBlock) == 0x4000, "SubsectionBlock has incorrect size.");

inline RelocationBucket ConvertRelocationBucketRaw(RelocationBucketRaw raw) {
    return {raw.number_entries,
            raw.end_offset,
            {raw.relocation_entries.begin(), raw.relocation_entries.begin() + raw.number_entries}};
}

inline SubsectionBucket ConvertSubsectionBucketRaw(SubsectionBucketRaw raw) {
    return {raw.number_entries,
            raw.end_offset,
            {raw.subsection_entries.begin(), raw.subsection_entries.begin() + raw.number_entries}};
}

class BKTR : public VfsFile {
public:
    BKTR(VirtualFile base_romfs, VirtualFile bktr_romfs, RelocationBlock relocation,
         std::vector<RelocationBucket> relocation_buckets, SubsectionBlock subsection,
         std::vector<SubsectionBucket> subsection_buckets, bool is_encrypted,
         Core::Crypto::Key128 key, u64 base_offset, u64 ivfc_offset, std::array<u8, 8> section_ctr);
    ~BKTR() override;

    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override;

    std::string GetName() const override;

    std::size_t GetSize() const override;

    bool Resize(std::size_t new_size) override;

    VirtualDir GetContainingDirectory() const override;

    bool IsWritable() const override;

    bool IsReadable() const override;

    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override;

    bool Rename(std::string_view name) override;

private:
    RelocationEntry GetRelocationEntry(u64 offset) const;
    RelocationEntry GetNextRelocationEntry(u64 offset) const;

    SubsectionEntry GetSubsectionEntry(u64 offset) const;
    SubsectionEntry GetNextSubsectionEntry(u64 offset) const;

    RelocationBlock relocation;
    std::vector<RelocationBucket> relocation_buckets;
    SubsectionBlock subsection;
    std::vector<SubsectionBucket> subsection_buckets;

    // Should be the raw base romfs, decrypted.
    VirtualFile base_romfs;
    // Should be the raw BKTR romfs, (located at media_offset with size media_size).
    VirtualFile bktr_romfs;

    bool encrypted;
    Core::Crypto::Key128 key;

    // Base offset into NCA, used for IV calculation.
    u64 base_offset;
    // Distance between IVFC start and RomFS start, used for base reads
    u64 ivfc_offset;
    std::array<u8, 8> section_ctr;
};

} // namespace FileSys
