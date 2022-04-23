// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/file_sys/system_archive/mii_model.h"
#include "core/file_sys/vfs_vector.h"

namespace FileSys::SystemArchive {

namespace MiiModelData {

constexpr std::array<u8, 0x10> NFTR_STANDARD{'N',  'F',  'T',  'R',  0x01, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr std::array<u8, 0x10> NFSR_STANDARD{'N',  'F',  'S',  'R',  0x01, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

constexpr auto TEXTURE_LOW_LINEAR = NFTR_STANDARD;
constexpr auto TEXTURE_LOW_SRGB = NFTR_STANDARD;
constexpr auto TEXTURE_MID_LINEAR = NFTR_STANDARD;
constexpr auto TEXTURE_MID_SRGB = NFTR_STANDARD;
constexpr auto SHAPE_HIGH = NFSR_STANDARD;
constexpr auto SHAPE_MID = NFSR_STANDARD;

} // namespace MiiModelData

VirtualDir MiiModel() {
    auto out = std::make_shared<VectorVfsDirectory>(std::vector<VirtualFile>{},
                                                    std::vector<VirtualDir>{}, "data");

    out->AddFile(MakeArrayFile(MiiModelData::TEXTURE_LOW_LINEAR, "NXTextureLowLinear.dat"));
    out->AddFile(MakeArrayFile(MiiModelData::TEXTURE_LOW_SRGB, "NXTextureLowSRGB.dat"));
    out->AddFile(MakeArrayFile(MiiModelData::TEXTURE_MID_LINEAR, "NXTextureMidLinear.dat"));
    out->AddFile(MakeArrayFile(MiiModelData::TEXTURE_MID_SRGB, "NXTextureMidSRGB.dat"));
    out->AddFile(MakeArrayFile(MiiModelData::SHAPE_HIGH, "ShapeHigh.dat"));
    out->AddFile(MakeArrayFile(MiiModelData::SHAPE_MID, "ShapeMid.dat"));

    return out;
}

} // namespace FileSys::SystemArchive
