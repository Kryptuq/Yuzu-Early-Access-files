// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/file_sys/vfs_types.h"

namespace FileSys::SystemArchive {

VirtualDir FontNintendoExtension();
VirtualDir FontStandard();
VirtualDir FontKorean();
VirtualDir FontChineseTraditional();
VirtualDir FontChineseSimple();

} // namespace FileSys::SystemArchive
