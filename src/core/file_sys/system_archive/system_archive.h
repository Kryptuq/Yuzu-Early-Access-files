// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"

namespace FileSys::SystemArchive {

VirtualFile SynthesizeSystemArchive(u64 title_id);

} // namespace FileSys::SystemArchive
