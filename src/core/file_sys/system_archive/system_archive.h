// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"

namespace FileSys::SystemArchive {

VirtualFile SynthesizeSystemArchive(u64 title_id);

} // namespace FileSys::SystemArchive
