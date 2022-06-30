// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/result.h"

namespace FileSys {

constexpr Result ERROR_PATH_NOT_FOUND{ErrorModule::FS, 1};
constexpr Result ERROR_PATH_ALREADY_EXISTS{ErrorModule::FS, 2};
constexpr Result ERROR_ENTITY_NOT_FOUND{ErrorModule::FS, 1002};
constexpr Result ERROR_SD_CARD_NOT_FOUND{ErrorModule::FS, 2001};
constexpr Result ERROR_OUT_OF_BOUNDS{ErrorModule::FS, 3005};
constexpr Result ERROR_FAILED_MOUNT_ARCHIVE{ErrorModule::FS, 3223};
constexpr Result ERROR_INVALID_ARGUMENT{ErrorModule::FS, 6001};
constexpr Result ERROR_INVALID_OFFSET{ErrorModule::FS, 6061};
constexpr Result ERROR_INVALID_SIZE{ErrorModule::FS, 6062};

} // namespace FileSys
