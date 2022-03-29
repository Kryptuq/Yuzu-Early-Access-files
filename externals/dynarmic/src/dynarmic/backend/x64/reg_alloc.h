/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/hostloc.h"
#include "dynarmic/backend/x64/oparg.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/ir/cond.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::IR {
enum class AccType;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::X64 {

class RegAlloc;

struct HostLocInfo {
public:
    bool IsLocked() const;
    bool IsEmpty() const;
    bool IsLastUse() const;

    void ReadLock();
    void WriteLock();
    void AddArgReference();
    void ReleaseOne();
    void ReleaseAll();

    bool ContainsValue(const IR::Inst* inst) const;
    size_t GetMaxBitWidth() const;

    void AddValue(IR::Inst* inst);

private:
    // Current instruction state
    size_t is_being_used_count = 0;
    bool is_scratch = false;

    // Block state
    size_t current_references = 0;
    size_t accumulated_uses = 0;
    size_t total_uses = 0;

    // Value state
    std::vector<IR::Inst*> values;
    size_t max_bit_width = 0;
};

struct Argument {
public:
    using copyable_reference = std::reference_wrapper<Argument>;

    IR::Type GetType() const;
    bool IsImmediate() const;
    bool IsVoid() const;

    bool FitsInImmediateU32() const;
    bool FitsInImmediateS32() const;

    bool GetImmediateU1() const;
    u8 GetImmediateU8() const;
    u16 GetImmediateU16() const;
    u32 GetImmediateU32() const;
    u64 GetImmediateS32() const;
    u64 GetImmediateU64() const;
    IR::Cond GetImmediateCond() const;
    IR::AccType GetImmediateAccType() const;

    /// Is this value currently in a GPR?
    bool IsInGpr() const;
    /// Is this value currently in a XMM?
    bool IsInXmm() const;
    /// Is this value currently in memory?
    bool IsInMemory() const;

private:
    friend class RegAlloc;
    explicit Argument(RegAlloc& reg_alloc)
            : reg_alloc(reg_alloc) {}

    bool allocated = false;
    RegAlloc& reg_alloc;
    IR::Value value;
};

class RegAlloc final {
public:
    using ArgumentInfo = std::array<Argument, IR::max_arg_count>;

    explicit RegAlloc(BlockOfCode& code, std::vector<HostLoc> gpr_order, std::vector<HostLoc> xmm_order);

    ArgumentInfo GetArgumentInfo(IR::Inst* inst);

    Xbyak::Reg64 UseGpr(Argument& arg);
    Xbyak::Xmm UseXmm(Argument& arg);
    OpArg UseOpArg(Argument& arg);
    void Use(Argument& arg, HostLoc host_loc);

    Xbyak::Reg64 UseScratchGpr(Argument& arg);
    Xbyak::Xmm UseScratchXmm(Argument& arg);
    void UseScratch(Argument& arg, HostLoc host_loc);

    void DefineValue(IR::Inst* inst, const Xbyak::Reg& reg);
    void DefineValue(IR::Inst* inst, Argument& arg);

    void Release(const Xbyak::Reg& reg);

    Xbyak::Reg64 ScratchGpr();
    Xbyak::Reg64 ScratchGpr(HostLoc desired_location);
    Xbyak::Xmm ScratchXmm();
    Xbyak::Xmm ScratchXmm(HostLoc desired_location);

    void HostCall(IR::Inst* result_def = nullptr,
                  std::optional<Argument::copyable_reference> arg0 = {},
                  std::optional<Argument::copyable_reference> arg1 = {},
                  std::optional<Argument::copyable_reference> arg2 = {},
                  std::optional<Argument::copyable_reference> arg3 = {});

    // TODO: Values in host flags

    void AllocStackSpace(size_t stack_space);
    void ReleaseStackSpace(size_t stack_space);

    void EndOfAllocScope();

    void AssertNoMoreUses();

private:
    friend struct Argument;

    std::vector<HostLoc> gpr_order;
    std::vector<HostLoc> xmm_order;

    HostLoc SelectARegister(const std::vector<HostLoc>& desired_locations) const;
    std::optional<HostLoc> ValueLocation(const IR::Inst* value) const;

    HostLoc UseImpl(IR::Value use_value, const std::vector<HostLoc>& desired_locations);
    HostLoc UseScratchImpl(IR::Value use_value, const std::vector<HostLoc>& desired_locations);
    HostLoc ScratchImpl(const std::vector<HostLoc>& desired_locations);
    void DefineValueImpl(IR::Inst* def_inst, HostLoc host_loc);
    void DefineValueImpl(IR::Inst* def_inst, const IR::Value& use_inst);

    HostLoc LoadImmediate(IR::Value imm, HostLoc host_loc);
    void Move(HostLoc to, HostLoc from);
    void CopyToScratch(size_t bit_width, HostLoc to, HostLoc from);
    void Exchange(HostLoc a, HostLoc b);
    void MoveOutOfTheWay(HostLoc reg);

    void SpillRegister(HostLoc loc);
    HostLoc FindFreeSpill() const;

    std::vector<HostLocInfo> hostloc_info;
    HostLocInfo& LocInfo(HostLoc loc);
    const HostLocInfo& LocInfo(HostLoc loc) const;

    BlockOfCode& code;
    size_t reserved_stack_space = 0;
    void EmitMove(size_t bit_width, HostLoc to, HostLoc from);
    void EmitExchange(HostLoc a, HostLoc b);

    Xbyak::Address SpillToOpArg(HostLoc loc);
};

}  // namespace Dynarmic::Backend::X64
