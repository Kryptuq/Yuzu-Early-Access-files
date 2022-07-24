// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/memory/memory_pool_info.h"

namespace AudioCore::AudioRenderer {

CpuAddr MemoryPoolInfo::GetCpuAddress() const {
    return cpu_address;
}

CpuAddr MemoryPoolInfo::GetDspAddress() const {
    return dsp_address;
}

u64 MemoryPoolInfo::GetSize() const {
    return size;
}

MemoryPoolInfo::Location MemoryPoolInfo::GetLocation() const {
    return location;
}

void MemoryPoolInfo::SetCpuAddress(const CpuAddr address, const u64 size_) {
    cpu_address = address;
    size = size_;
}

void MemoryPoolInfo::SetDspAddress(const CpuAddr address) {
    dsp_address = address;
}

bool MemoryPoolInfo::Contains(const CpuAddr address_, const u64 size_) const {
    return cpu_address <= address_ && (address_ + size_) <= (cpu_address + size);
}

bool MemoryPoolInfo::IsMapped() const {
    return dsp_address != 0;
}

CpuAddr MemoryPoolInfo::Translate(const CpuAddr address, const u64 size_) const {
    if (!Contains(address, size_)) {
        return 0;
    }

    if (!IsMapped()) {
        return 0;
    }

    return dsp_address + (address - cpu_address);
}

void MemoryPoolInfo::SetUsed(const bool used) {
    in_use = used;
}

bool MemoryPoolInfo::IsUsed() const {
    return in_use;
}

} // namespace AudioCore::AudioRenderer
