// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/device_memory.h"

namespace Core {

DeviceMemory::DeviceMemory() : buffer{DramMemoryMap::Size, 1ULL << 39} {}
DeviceMemory::~DeviceMemory() = default;

} // namespace Core
