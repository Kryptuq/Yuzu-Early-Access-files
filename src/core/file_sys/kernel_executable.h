// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <string>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/file_sys/vfs_types.h"

namespace Loader {
enum class ResultStatus : u16;
}

namespace FileSys {

struct KIPSectionHeader {
    u32_le offset;
    u32_le decompressed_size;
    u32_le compressed_size;
    u32_le attribute;
};
static_assert(sizeof(KIPSectionHeader) == 0x10, "KIPSectionHeader has incorrect size.");

struct KIPHeader {
    u32_le magic;
    std::array<char, 0xC> name;
    u64_le title_id;
    u32_le process_category;
    u8 main_thread_priority;
    u8 default_core;
    INSERT_PADDING_BYTES(1);
    u8 flags;
    std::array<KIPSectionHeader, 6> sections;
    std::array<u32, 0x20> capabilities;
};
static_assert(sizeof(KIPHeader) == 0x100, "KIPHeader has incorrect size.");

struct INIHeader {
    u32_le magic;
    u32_le size;
    u32_le kip_count;
    INSERT_PADDING_BYTES(0x4);
};
static_assert(sizeof(INIHeader) == 0x10, "INIHeader has incorrect size.");

// Kernel Internal Process
class KIP {
public:
    explicit KIP(const VirtualFile& file);

    Loader::ResultStatus GetStatus() const;

    std::string GetName() const;
    u64 GetTitleID() const;
    std::vector<u8> GetSectionDecompressed(u8 index) const;

    // Executable Flags
    bool Is64Bit() const;
    bool Is39BitAddressSpace() const;
    bool IsService() const;

    std::vector<u32> GetKernelCapabilities() const;

    s32 GetMainThreadPriority() const;
    u32 GetMainThreadStackSize() const;
    u32 GetMainThreadCpuCore() const;

    const std::vector<u8>& GetTextSection() const;
    const std::vector<u8>& GetRODataSection() const;
    const std::vector<u8>& GetDataSection() const;

    u32 GetTextOffset() const;
    u32 GetRODataOffset() const;
    u32 GetDataOffset() const;

    u32 GetBSSSize() const;
    u32 GetBSSOffset() const;

private:
    Loader::ResultStatus status;

    KIPHeader header{};
    std::array<std::vector<u8>, 6> decompressed_sections;
};

class INI {
public:
    explicit INI(const VirtualFile& file);

    Loader::ResultStatus GetStatus() const;

    const std::vector<KIP>& GetKIPs() const;

private:
    Loader::ResultStatus status;

    INIHeader header{};
    std::vector<KIP> kips;
};

} // namespace FileSys
