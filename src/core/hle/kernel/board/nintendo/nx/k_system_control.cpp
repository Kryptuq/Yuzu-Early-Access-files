// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <random>

#include "common/common_sizes.h"
#include "core/hle/kernel/board/nintendo/nx/k_system_control.h"
#include "core/hle/kernel/board/nintendo/nx/secure_monitor.h"
#include "core/hle/kernel/k_trace.h"

namespace Kernel::Board::Nintendo::Nx {

namespace impl {

constexpr const std::size_t RequiredNonSecureSystemMemorySizeVi = 0x2238 * 4 * 1024;
constexpr const std::size_t RequiredNonSecureSystemMemorySizeNvservices = 0x710 * 4 * 1024;
constexpr const std::size_t RequiredNonSecureSystemMemorySizeMisc = 0x80 * 4 * 1024;

} // namespace impl

constexpr const std::size_t RequiredNonSecureSystemMemorySize =
    impl::RequiredNonSecureSystemMemorySizeVi + impl::RequiredNonSecureSystemMemorySizeNvservices +
    impl::RequiredNonSecureSystemMemorySizeMisc;

namespace {

u32 GetMemoryModeForInit() {
    return 0x01;
}

u32 GetMemorySizeForInit() {
    return 0;
}

Smc::MemoryArrangement GetMemoryArrangeForInit() {
    switch (GetMemoryModeForInit() & 0x3F) {
    case 0x01:
    default:
        return Smc::MemoryArrangement_4GB;
    case 0x02:
        return Smc::MemoryArrangement_4GBForAppletDev;
    case 0x03:
        return Smc::MemoryArrangement_4GBForSystemDev;
    case 0x11:
        return Smc::MemoryArrangement_6GB;
    case 0x12:
        return Smc::MemoryArrangement_6GBForAppletDev;
    case 0x21:
        return Smc::MemoryArrangement_8GB;
    }
}
} // namespace

// Initialization.
size_t KSystemControl::Init::GetIntendedMemorySize() {
    switch (GetMemorySizeForInit()) {
    case Smc::MemorySize_4GB:
    default: // All invalid modes should go to 4GB.
        return Common::Size_4_GB;
    case Smc::MemorySize_6GB:
        return Common::Size_6_GB;
    case Smc::MemorySize_8GB:
        return Common::Size_8_GB;
    }
}

PAddr KSystemControl::Init::GetKernelPhysicalBaseAddress(u64 base_address) {
    return base_address;
}

bool KSystemControl::Init::ShouldIncreaseThreadResourceLimit() {
    return true;
}

std::size_t KSystemControl::Init::GetApplicationPoolSize() {
    // Get the base pool size.
    const size_t base_pool_size = []() -> size_t {
        switch (GetMemoryArrangeForInit()) {
        case Smc::MemoryArrangement_4GB:
        default:
            return Common::Size_3285_MB;
        case Smc::MemoryArrangement_4GBForAppletDev:
            return Common::Size_2048_MB;
        case Smc::MemoryArrangement_4GBForSystemDev:
            return Common::Size_3285_MB;
        case Smc::MemoryArrangement_6GB:
            return Common::Size_4916_MB;
        case Smc::MemoryArrangement_6GBForAppletDev:
            return Common::Size_3285_MB;
        case Smc::MemoryArrangement_8GB:
            return Common::Size_4916_MB;
        }
    }();

    // Return (possibly) adjusted size.
    return base_pool_size;
}

size_t KSystemControl::Init::GetAppletPoolSize() {
    // Get the base pool size.
    const size_t base_pool_size = []() -> size_t {
        switch (GetMemoryArrangeForInit()) {
        case Smc::MemoryArrangement_4GB:
        default:
            return Common::Size_507_MB;
        case Smc::MemoryArrangement_4GBForAppletDev:
            return Common::Size_1554_MB;
        case Smc::MemoryArrangement_4GBForSystemDev:
            return Common::Size_448_MB;
        case Smc::MemoryArrangement_6GB:
            return Common::Size_562_MB;
        case Smc::MemoryArrangement_6GBForAppletDev:
            return Common::Size_2193_MB;
        case Smc::MemoryArrangement_8GB:
            return Common::Size_2193_MB;
        }
    }();

    // Return (possibly) adjusted size.
    constexpr size_t ExtraSystemMemoryForAtmosphere = Common::Size_33_MB;
    return base_pool_size - ExtraSystemMemoryForAtmosphere - KTraceBufferSize;
}

size_t KSystemControl::Init::GetMinimumNonSecureSystemPoolSize() {
    // Verify that our minimum is at least as large as Nintendo's.
    constexpr size_t MinimumSize = RequiredNonSecureSystemMemorySize;
    static_assert(MinimumSize >= 0x29C8000);

    return MinimumSize;
}

namespace {
template <typename F>
u64 GenerateUniformRange(u64 min, u64 max, F f) {
    // Handle the case where the difference is too large to represent.
    if (max == std::numeric_limits<u64>::max() && min == std::numeric_limits<u64>::min()) {
        return f();
    }

    // Iterate until we get a value in range.
    const u64 range_size = ((max + 1) - min);
    const u64 effective_max = (std::numeric_limits<u64>::max() / range_size) * range_size;
    while (true) {
        if (const u64 rnd = f(); rnd < effective_max) {
            return min + (rnd % range_size);
        }
    }
}

} // Anonymous namespace

u64 KSystemControl::GenerateRandomU64() {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_int_distribution<u64> distribution(1, std::numeric_limits<u64>::max());
    return distribution(gen);
}

u64 KSystemControl::GenerateRandomRange(u64 min, u64 max) {
    return GenerateUniformRange(min, max, GenerateRandomU64);
}

} // namespace Kernel::Board::Nintendo::Nx
