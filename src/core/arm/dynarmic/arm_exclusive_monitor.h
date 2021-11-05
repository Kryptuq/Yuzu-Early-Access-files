// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <unordered_map>

#include <dynarmic/interface/exclusive_monitor.h>

#include "common/common_types.h"
#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"
#include "core/arm/exclusive_monitor.h"

namespace Core::Memory {
class Memory;
}

namespace Core {

class DynarmicExclusiveMonitor final : public ExclusiveMonitor {
public:
    explicit DynarmicExclusiveMonitor(Memory::Memory& memory_, std::size_t core_count_);
    ~DynarmicExclusiveMonitor() override;

    u8 ExclusiveRead8(std::size_t core_index, VAddr addr) override;
    u16 ExclusiveRead16(std::size_t core_index, VAddr addr) override;
    u32 ExclusiveRead32(std::size_t core_index, VAddr addr) override;
    u64 ExclusiveRead64(std::size_t core_index, VAddr addr) override;
    u128 ExclusiveRead128(std::size_t core_index, VAddr addr) override;
    void ClearExclusive() override;

    bool ExclusiveWrite8(std::size_t core_index, VAddr vaddr, u8 value) override;
    bool ExclusiveWrite16(std::size_t core_index, VAddr vaddr, u16 value) override;
    bool ExclusiveWrite32(std::size_t core_index, VAddr vaddr, u32 value) override;
    bool ExclusiveWrite64(std::size_t core_index, VAddr vaddr, u64 value) override;
    bool ExclusiveWrite128(std::size_t core_index, VAddr vaddr, u128 value) override;

private:
    friend class ARM_Dynarmic_32;
    friend class ARM_Dynarmic_64;
    Dynarmic::ExclusiveMonitor monitor;
    Core::Memory::Memory& memory;
};

} // namespace Core
