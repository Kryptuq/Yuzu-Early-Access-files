// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/memory/address_info.h"
#include "audio_core/renderer/memory/pool_mapper.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/svc.h"

namespace AudioCore::AudioRenderer {

PoolMapper::PoolMapper(u32 process_handle_, bool force_map_)
    : process_handle{process_handle_}, force_map{force_map_} {}

PoolMapper::PoolMapper(u32 process_handle_, std::span<MemoryPoolInfo> pool_infos_, u32 pool_count_,
                       bool force_map_)
    : process_handle{process_handle_}, pool_infos{pool_infos_.data()},
      pool_count{pool_count_}, force_map{force_map_} {}

void PoolMapper::ClearUseState(std::span<MemoryPoolInfo> pools, const u32 count) {
    for (u32 i = 0; i < count; i++) {
        pools[i].SetUsed(false);
    }
}

MemoryPoolInfo* PoolMapper::FindMemoryPool(MemoryPoolInfo* pools, const u64 count,
                                           const CpuAddr address, const u64 size) const {
    auto pool{pools};
    for (u64 i = 0; i < count; i++, pool++) {
        if (pool->Contains(address, size)) {
            return pool;
        }
    }
    return nullptr;
}

MemoryPoolInfo* PoolMapper::FindMemoryPool(const CpuAddr address, const u64 size) const {
    auto pool{pool_infos};
    for (u64 i = 0; i < pool_count; i++, pool++) {
        if (pool->Contains(address, size)) {
            return pool;
        }
    }
    return nullptr;
}

bool PoolMapper::FillDspAddr(AddressInfo& address_info, MemoryPoolInfo* pools,
                             const u32 count) const {
    if (address_info.GetCpuAddr() == 0) {
        address_info.SetPool(nullptr);
        return false;
    }

    auto found_pool{
        FindMemoryPool(pools, count, address_info.GetCpuAddr(), address_info.GetSize())};
    if (found_pool != nullptr) {
        address_info.SetPool(found_pool);
        return true;
    }

    if (force_map) {
        address_info.SetForceMappedDspAddr(address_info.GetCpuAddr());
    } else {
        address_info.SetPool(nullptr);
    }

    return false;
}

bool PoolMapper::FillDspAddr(AddressInfo& address_info) const {
    if (address_info.GetCpuAddr() == 0) {
        address_info.SetPool(nullptr);
        return false;
    }

    auto found_pool{FindMemoryPool(address_info.GetCpuAddr(), address_info.GetSize())};
    if (found_pool != nullptr) {
        address_info.SetPool(found_pool);
        return true;
    }

    if (force_map) {
        address_info.SetForceMappedDspAddr(address_info.GetCpuAddr());
    } else {
        address_info.SetPool(nullptr);
    }

    return false;
}

bool PoolMapper::TryAttachBuffer(BehaviorInfo::ErrorInfo& error_info, AddressInfo& address_info,
                                 const CpuAddr address, const u64 size) const {
    address_info.Setup(address, size);

    if (!FillDspAddr(address_info)) {
        error_info.error_code = Service::Audio::ERR_POOL_MAPPING_FAILED;
        error_info.address = address;
        return force_map;
    }

    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
    return true;
}

bool PoolMapper::IsForceMapEnabled() const {
    return force_map;
}

u32 PoolMapper::GetProcessHandle(const MemoryPoolInfo* pool) const {
    switch (pool->GetLocation()) {
    case MemoryPoolInfo::Location::CPU:
        return process_handle;
    case MemoryPoolInfo::Location::DSP:
        return Kernel::Svc::CurrentProcess;
    }
    LOG_WARNING(Service_Audio, "Invalid MemoryPoolInfo location!");
    return Kernel::Svc::CurrentProcess;
}

bool PoolMapper::Map([[maybe_unused]] const u32 handle, [[maybe_unused]] const CpuAddr cpu_addr,
                     [[maybe_unused]] const u64 size) const {
    // nn::audio::dsp::MapUserPointer(handle, cpu_addr, size);
    return true;
}

bool PoolMapper::Map(MemoryPoolInfo& pool) const {
    switch (pool.GetLocation()) {
    case MemoryPoolInfo::Location::CPU:
        // Map with process_handle
        pool.SetDspAddress(pool.GetCpuAddress());
        return true;
    case MemoryPoolInfo::Location::DSP:
        // Map with Kernel::Svc::CurrentProcess
        pool.SetDspAddress(pool.GetCpuAddress());
        return true;
    default:
        LOG_WARNING(Service_Audio, "Invalid MemoryPoolInfo location={}!",
                    static_cast<u32>(pool.GetLocation()));
        return false;
    }
}

bool PoolMapper::Unmap([[maybe_unused]] const u32 handle, [[maybe_unused]] const CpuAddr cpu_addr,
                       [[maybe_unused]] const u64 size) const {
    // nn::audio::dsp::UnmapUserPointer(handle, cpu_addr, size);
    return true;
}

bool PoolMapper::Unmap(MemoryPoolInfo& pool) const {
    [[maybe_unused]] u32 handle{0};

    switch (pool.GetLocation()) {
    case MemoryPoolInfo::Location::CPU:
        handle = process_handle;
        break;
    case MemoryPoolInfo::Location::DSP:
        handle = Kernel::Svc::CurrentProcess;
        break;
    }
    // nn::audio::dsp::UnmapUserPointer(handle, pool->cpu_address, pool->size);
    pool.SetCpuAddress(0, 0);
    pool.SetDspAddress(0);
    return true;
}

void PoolMapper::ForceUnmapPointer(const AddressInfo& address_info) const {
    if (force_map) {
        [[maybe_unused]] auto found_pool{
            FindMemoryPool(address_info.GetCpuAddr(), address_info.GetSize())};
        // nn::audio::dsp::UnmapUserPointer(this->processHandle, address_info.GetCpuAddr(), 0);
    }
}

MemoryPoolInfo::ResultState PoolMapper::Update(MemoryPoolInfo& pool,
                                               const MemoryPoolInfo::InParameter& in_params,
                                               MemoryPoolInfo::OutStatus& out_params) const {
    if (in_params.state != MemoryPoolInfo::State::RequestAttach &&
        in_params.state != MemoryPoolInfo::State::RequestDetach) {
        return MemoryPoolInfo::ResultState::Success;
    }

    if (in_params.address == 0 || in_params.size == 0 || !Common::Is4KBAligned(in_params.address) ||
        !Common::Is4KBAligned(in_params.size)) {
        return MemoryPoolInfo::ResultState::BadParam;
    }

    switch (in_params.state) {
    case MemoryPoolInfo::State::RequestAttach:
        pool.SetCpuAddress(in_params.address, in_params.size);

        Map(pool);

        if (pool.IsMapped()) {
            out_params.state = MemoryPoolInfo::State::Attached;
            return MemoryPoolInfo::ResultState::Success;
        }
        pool.SetCpuAddress(0, 0);
        return MemoryPoolInfo::ResultState::MapFailed;

    case MemoryPoolInfo::State::RequestDetach:
        if (pool.GetCpuAddress() != in_params.address || pool.GetSize() != in_params.size) {
            return MemoryPoolInfo::ResultState::BadParam;
        }

        if (pool.IsUsed()) {
            return MemoryPoolInfo::ResultState::InUse;
        }

        Unmap(pool);

        pool.SetCpuAddress(0, 0);
        pool.SetDspAddress(0);
        out_params.state = MemoryPoolInfo::State::Detached;
        return MemoryPoolInfo::ResultState::Success;

    default:
        LOG_ERROR(Service_Audio, "Invalid MemoryPoolInfo::State!");
        break;
    }

    return MemoryPoolInfo::ResultState::Success;
}

bool PoolMapper::InitializeSystemPool(MemoryPoolInfo& pool, const u8* memory,
                                      const u64 size_) const {
    switch (pool.GetLocation()) {
    case MemoryPoolInfo::Location::CPU:
        return false;
    case MemoryPoolInfo::Location::DSP:
        pool.SetCpuAddress(reinterpret_cast<u64>(memory), size_);
        if (Map(Kernel::Svc::CurrentProcess, reinterpret_cast<u64>(memory), size_)) {
            pool.SetDspAddress(pool.GetCpuAddress());
            return true;
        }
        return false;
    default:
        LOG_WARNING(Service_Audio, "Invalid MemoryPoolInfo location={}!",
                    static_cast<u32>(pool.GetLocation()));
        return false;
    }
}

} // namespace AudioCore::AudioRenderer
