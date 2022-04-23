// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/file_sys/vfs_types.h"

namespace FileSys::SystemArchive {

VirtualDir FontNintendoExtension();
VirtualDir FontStandard();
VirtualDir FontKorean();
VirtualDir FontChineseTraditional();
VirtualDir FontChineseSimple();

} // namespace FileSys::SystemArchive
