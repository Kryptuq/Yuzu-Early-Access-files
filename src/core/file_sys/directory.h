// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <iterator>
#include "common/common_funcs.h"
#include "common/common_types.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// FileSys namespace

namespace FileSys {

enum class EntryType : u8 {
    Directory = 0,
    File = 1,
};

// Structure of a directory entry, from
// http://switchbrew.org/index.php?title=Filesystem_services#DirectoryEntry
struct Entry {
    Entry(std::string_view view, EntryType entry_type, u64 entry_size)
        : type{entry_type}, file_size{entry_size} {
        const std::size_t copy_size = view.copy(filename, std::size(filename) - 1);
        filename[copy_size] = '\0';
    }

    char filename[0x301];
    INSERT_PADDING_BYTES(3);
    EntryType type;
    INSERT_PADDING_BYTES(3);
    u64 file_size;
};
static_assert(sizeof(Entry) == 0x310, "Directory Entry struct isn't exactly 0x310 bytes long!");
static_assert(offsetof(Entry, type) == 0x304, "Wrong offset for type in Entry.");
static_assert(offsetof(Entry, file_size) == 0x308, "Wrong offset for file_size in Entry.");

} // namespace FileSys
