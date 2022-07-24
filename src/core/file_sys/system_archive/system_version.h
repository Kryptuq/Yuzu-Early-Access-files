// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include "core/file_sys/vfs_types.h"

namespace FileSys::SystemArchive {

std::string GetLongDisplayVersion();

VirtualDir SystemVersion();

} // namespace FileSys::SystemArchive
