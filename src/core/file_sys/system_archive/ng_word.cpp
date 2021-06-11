// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fmt/format.h>
#include "common/common_types.h"
#include "core/file_sys/system_archive/ng_word.h"
#include "core/file_sys/vfs_vector.h"

namespace FileSys::SystemArchive {

namespace NgWord1Data {

constexpr std::size_t NUMBER_WORD_TXT_FILES = 0x10;

// Should this archive replacement mysteriously not work on a future game, consider updating.
constexpr std::array<u8, 4> VERSION_DAT{0x0, 0x0, 0x0, 0x20}; // 11.0.1 System Version

constexpr std::array<u8, 30> WORD_TXT{
    0xFE, 0xFF, 0x00, 0x5E, 0x00, 0x76, 0x00, 0x65, 0x00, 0x72, 0x00, 0x79, 0x00, 0x62, 0x00,
    0x61, 0x00, 0x64, 0x00, 0x77, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x64, 0x00, 0x24, 0x00, 0x0A,
}; // "^verybadword$" in UTF-16

} // namespace NgWord1Data

VirtualDir NgWord1() {
    std::vector<VirtualFile> files;
    files.reserve(NgWord1Data::NUMBER_WORD_TXT_FILES);

    for (std::size_t i = 0; i < files.size(); ++i) {
        files.push_back(MakeArrayFile(NgWord1Data::WORD_TXT, fmt::format("{}.txt", i)));
    }

    files.push_back(MakeArrayFile(NgWord1Data::WORD_TXT, "common.txt"));
    files.push_back(MakeArrayFile(NgWord1Data::VERSION_DAT, "version.dat"));

    return std::make_shared<VectorVfsDirectory>(std::move(files), std::vector<VirtualDir>{},
                                                "data");
}

namespace NgWord2Data {

constexpr std::size_t NUMBER_AC_NX_FILES = 0x10;

// Should this archive replacement mysteriously not work on a future game, consider updating.
constexpr std::array<u8, 4> VERSION_DAT{0x0, 0x0, 0x0, 0x1A}; // 11.0.1 System Version

constexpr std::array<u8, 0x2C> AC_NX_DATA{
    0x1F, 0x8B, 0x08, 0x08, 0xD5, 0x2C, 0x09, 0x5C, 0x04, 0x00, 0x61, 0x63, 0x72, 0x61, 0x77,
    0x00, 0xED, 0xC1, 0x01, 0x0D, 0x00, 0x00, 0x00, 0xC2, 0x20, 0xFB, 0xA7, 0xB6, 0xC7, 0x07,
    0x0C, 0x00, 0x00, 0x00, 0xC8, 0x3B, 0x11, 0x00, 0x1C, 0xC7, 0x00, 0x10, 0x00, 0x00,
}; // Deserializes to no bad words

} // namespace NgWord2Data

VirtualDir NgWord2() {
    std::vector<VirtualFile> files;
    files.reserve(NgWord2Data::NUMBER_AC_NX_FILES * 3);

    for (std::size_t i = 0; i < NgWord2Data::NUMBER_AC_NX_FILES; ++i) {
        files.push_back(MakeArrayFile(NgWord2Data::AC_NX_DATA, fmt::format("ac_{}_b1_nx", i)));
        files.push_back(MakeArrayFile(NgWord2Data::AC_NX_DATA, fmt::format("ac_{}_b2_nx", i)));
        files.push_back(MakeArrayFile(NgWord2Data::AC_NX_DATA, fmt::format("ac_{}_not_b_nx", i)));
    }

    files.push_back(MakeArrayFile(NgWord2Data::AC_NX_DATA, "ac_common_b1_nx"));
    files.push_back(MakeArrayFile(NgWord2Data::AC_NX_DATA, "ac_common_b2_nx"));
    files.push_back(MakeArrayFile(NgWord2Data::AC_NX_DATA, "ac_common_not_b_nx"));
    files.push_back(MakeArrayFile(NgWord2Data::VERSION_DAT, "version.dat"));

    return std::make_shared<VectorVfsDirectory>(std::move(files), std::vector<VirtualDir>{},
                                                "data");
}

} // namespace FileSys::SystemArchive
