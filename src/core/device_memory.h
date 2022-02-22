// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/host_memory.h"

namespace Core {

namespace DramMemoryMap {
enum : u64 {
    Base = 0x80000000ULL,
    KernelReserveBase = Base + 0x60000,
    SlabHeapBase = KernelReserveBase + 0x85000,
};
}; // namespace DramMemoryMap

class DeviceMemory {
public:
    explicit DeviceMemory();
    ~DeviceMemory();

    DeviceMemory& operator=(const DeviceMemory&) = delete;
    DeviceMemory(const DeviceMemory&) = delete;

    template <typename T>
    PAddr GetPhysicalAddr(const T* ptr) const {
        return (reinterpret_cast<uintptr_t>(ptr) -
                reinterpret_cast<uintptr_t>(buffer.BackingBasePointer())) +
               DramMemoryMap::Base;
    }

    u8* GetPointer(PAddr addr) {
        return buffer.BackingBasePointer() + (addr - DramMemoryMap::Base);
    }

    const u8* GetPointer(PAddr addr) const {
        return buffer.BackingBasePointer() + (addr - DramMemoryMap::Base);
    }

    Common::HostMemory buffer;
};

} // namespace Core
