/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/a32_emit_x64.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <mp/traits/integer_of_size.h>

#include "dynarmic/backend/x64/a32_jitstate.h"
#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/devirtualize.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/backend/x64/nzcv_util.h"
#include "dynarmic/backend/x64/perf_map.h"
#include "dynarmic/backend/x64/stack_layout.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/scope_exit.h"
#include "dynarmic/common/variant_util.h"
#include "dynarmic/common/x64_disassemble.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/interface/A32/coprocessor.h"
#include "dynarmic/interface/exclusive_monitor.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

// TODO: Have ARM flags in host flags and not have them use up GPR registers unless necessary.
// TODO: Actually implement that proper instruction selector you've always wanted to sweetheart.

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

static Xbyak::Address MJitStateReg(A32::Reg reg) {
    return dword[r15 + offsetof(A32JitState, Reg) + sizeof(u32) * static_cast<size_t>(reg)];
}

static Xbyak::Address MJitStateExtReg(A32::ExtReg reg) {
    if (A32::IsSingleExtReg(reg)) {
        const size_t index = static_cast<size_t>(reg) - static_cast<size_t>(A32::ExtReg::S0);
        return dword[r15 + offsetof(A32JitState, ExtReg) + sizeof(u32) * index];
    }
    if (A32::IsDoubleExtReg(reg)) {
        const size_t index = static_cast<size_t>(reg) - static_cast<size_t>(A32::ExtReg::D0);
        return qword[r15 + offsetof(A32JitState, ExtReg) + sizeof(u64) * index];
    }
    if (A32::IsQuadExtReg(reg)) {
        const size_t index = static_cast<size_t>(reg) - static_cast<size_t>(A32::ExtReg::Q0);
        return xword[r15 + offsetof(A32JitState, ExtReg) + 2 * sizeof(u64) * index];
    }
    ASSERT_FALSE("Should never happen.");
}

A32EmitContext::A32EmitContext(const A32::UserConfig& conf, RegAlloc& reg_alloc, IR::Block& block)
        : EmitContext(reg_alloc, block), conf(conf) {}

A32::LocationDescriptor A32EmitContext::Location() const {
    return A32::LocationDescriptor{block.Location()};
}

A32::LocationDescriptor A32EmitContext::EndLocation() const {
    return A32::LocationDescriptor{block.EndLocation()};
}

bool A32EmitContext::IsSingleStep() const {
    return Location().SingleStepping();
}

FP::FPCR A32EmitContext::FPCR(bool fpcr_controlled) const {
    const FP::FPCR fpcr = FP::FPCR{Location().FPSCR().Value()};
    return fpcr_controlled ? fpcr : fpcr.ASIMDStandardValue();
}

A32EmitX64::A32EmitX64(BlockOfCode& code, A32::UserConfig conf, A32::Jit* jit_interface)
        : EmitX64(code), conf(std::move(conf)), jit_interface(jit_interface) {
    GenFastmemFallbacks();
    GenTerminalHandlers();
    code.PreludeComplete();
    ClearFastDispatchTable();

    exception_handler.SetFastmemCallback([this](u64 rip_) {
        return FastmemCallback(rip_);
    });
}

A32EmitX64::~A32EmitX64() = default;

A32EmitX64::BlockDescriptor A32EmitX64::Emit(IR::Block& block) {
    code.EnableWriting();
    SCOPE_EXIT { code.DisableWriting(); };

    const std::vector<HostLoc> gpr_order = [this] {
        std::vector<HostLoc> gprs{any_gpr};
        if (conf.page_table) {
            gprs.erase(std::find(gprs.begin(), gprs.end(), HostLoc::R14));
        }
        if (conf.fastmem_pointer) {
            gprs.erase(std::find(gprs.begin(), gprs.end(), HostLoc::R13));
        }
        return gprs;
    }();

    RegAlloc reg_alloc{code, gpr_order, any_xmm};
    A32EmitContext ctx{conf, reg_alloc, block};

    // Start emitting.
    code.align();
    const u8* const entrypoint = code.getCurr();
    code.SwitchToFarCode();
    const u8* const entrypoint_far = code.getCurr();
    code.SwitchToNearCode();

    EmitCondPrelude(ctx);

    for (auto iter = block.begin(); iter != block.end(); ++iter) {
        IR::Inst* inst = &*iter;

        // Call the relevant Emit* member function.
        switch (inst->GetOpcode()) {
#define OPCODE(name, type, ...)            \
    case IR::Opcode::name:                 \
        A32EmitX64::Emit##name(ctx, inst); \
        break;
#define A32OPC(name, type, ...)               \
    case IR::Opcode::A32##name:               \
        A32EmitX64::EmitA32##name(ctx, inst); \
        break;
#define A64OPC(...)
#include "dynarmic/ir/opcodes.inc"
#undef OPCODE
#undef A32OPC
#undef A64OPC

        default:
            ASSERT_FALSE("Invalid opcode: {}", inst->GetOpcode());
            break;
        }

        reg_alloc.EndOfAllocScope();
    }

    reg_alloc.AssertNoMoreUses();

    EmitAddCycles(block.CycleCount());
    EmitX64::EmitTerminal(block.GetTerminal(), ctx.Location().SetSingleStepping(false), ctx.IsSingleStep());
    code.int3();

    const size_t size = static_cast<size_t>(code.getCurr() - entrypoint);

    const A32::LocationDescriptor descriptor{block.Location()};
    const A32::LocationDescriptor end_location{block.EndLocation()};

    const auto range = boost::icl::discrete_interval<u32>::closed(descriptor.PC(), end_location.PC() - 1);
    block_ranges.AddRange(range, descriptor);

    return RegisterBlock(descriptor, entrypoint, entrypoint_far, size);
}

void A32EmitX64::ClearCache() {
    EmitX64::ClearCache();
    block_ranges.ClearCache();
    ClearFastDispatchTable();
    fastmem_patch_info.clear();
}

void A32EmitX64::InvalidateCacheRanges(const boost::icl::interval_set<u32>& ranges) {
    InvalidateBasicBlocks(block_ranges.InvalidateRanges(ranges));
}

void A32EmitX64::EmitCondPrelude(const A32EmitContext& ctx) {
    if (ctx.block.GetCondition() == IR::Cond::AL) {
        ASSERT(!ctx.block.HasConditionFailedLocation());
        return;
    }

    ASSERT(ctx.block.HasConditionFailedLocation());

    Xbyak::Label pass = EmitCond(ctx.block.GetCondition());
    EmitAddCycles(ctx.block.ConditionFailedCycleCount());
    EmitTerminal(IR::Term::LinkBlock{ctx.block.ConditionFailedLocation()}, ctx.Location().SetSingleStepping(false), ctx.IsSingleStep());
    code.L(pass);
}

void A32EmitX64::ClearFastDispatchTable() {
    if (conf.HasOptimization(OptimizationFlag::FastDispatch)) {
        fast_dispatch_table.fill({});
    }
}

void A32EmitX64::GenFastmemFallbacks() {
    const std::initializer_list<int> idxes{0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    const std::array<std::pair<size_t, ArgCallback>, 4> read_callbacks{{
        {8, Devirtualize<&A32::UserCallbacks::MemoryRead8>(conf.callbacks)},
        {16, Devirtualize<&A32::UserCallbacks::MemoryRead16>(conf.callbacks)},
        {32, Devirtualize<&A32::UserCallbacks::MemoryRead32>(conf.callbacks)},
        {64, Devirtualize<&A32::UserCallbacks::MemoryRead64>(conf.callbacks)},
    }};
    const std::array<std::pair<size_t, ArgCallback>, 4> write_callbacks{{
        {8, Devirtualize<&A32::UserCallbacks::MemoryWrite8>(conf.callbacks)},
        {16, Devirtualize<&A32::UserCallbacks::MemoryWrite16>(conf.callbacks)},
        {32, Devirtualize<&A32::UserCallbacks::MemoryWrite32>(conf.callbacks)},
        {64, Devirtualize<&A32::UserCallbacks::MemoryWrite64>(conf.callbacks)},
    }};

    for (int vaddr_idx : idxes) {
        for (int value_idx : idxes) {
            for (const auto& [bitsize, callback] : read_callbacks) {
                code.align();
                read_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
                ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocRegIdx(value_idx));
                if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                    code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
                }
                callback.EmitCall(code);
                if (value_idx != code.ABI_RETURN.getIdx()) {
                    code.mov(Xbyak::Reg64{value_idx}, code.ABI_RETURN);
                }
                ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocRegIdx(value_idx));
                code.ret();
                PerfMapRegister(read_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a32_read_fallback_{}", bitsize));
            }

            for (const auto& [bitsize, callback] : write_callbacks) {
                code.align();
                write_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
                ABI_PushCallerSaveRegistersAndAdjustStack(code);
                if (vaddr_idx == code.ABI_PARAM3.getIdx() && value_idx == code.ABI_PARAM2.getIdx()) {
                    code.xchg(code.ABI_PARAM2, code.ABI_PARAM3);
                } else if (vaddr_idx == code.ABI_PARAM3.getIdx()) {
                    code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
                    if (value_idx != code.ABI_PARAM3.getIdx()) {
                        code.mov(code.ABI_PARAM3, Xbyak::Reg64{value_idx});
                    }
                } else {
                    if (value_idx != code.ABI_PARAM3.getIdx()) {
                        code.mov(code.ABI_PARAM3, Xbyak::Reg64{value_idx});
                    }
                    if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                        code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
                    }
                }
                callback.EmitCall(code);
                ABI_PopCallerSaveRegistersAndAdjustStack(code);
                code.ret();
                PerfMapRegister(write_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a32_write_fallback_{}", bitsize));
            }
        }
    }
}

void A32EmitX64::GenTerminalHandlers() {
    // PC ends up in ebp, location_descriptor ends up in rbx
    const auto calculate_location_descriptor = [this] {
        // This calculation has to match up with IREmitter::PushRSB
        code.mov(ebx, dword[r15 + offsetof(A32JitState, upper_location_descriptor)]);
        code.shl(rbx, 32);
        code.mov(ecx, MJitStateReg(A32::Reg::PC));
        code.mov(ebp, ecx);
        code.or_(rbx, rcx);
    };

    Xbyak::Label fast_dispatch_cache_miss, rsb_cache_miss;

    code.align();
    terminal_handler_pop_rsb_hint = code.getCurr<const void*>();
    calculate_location_descriptor();
    code.mov(eax, dword[r15 + offsetof(A32JitState, rsb_ptr)]);
    code.sub(eax, 1);
    code.and_(eax, u32(A32JitState::RSBPtrMask));
    code.mov(dword[r15 + offsetof(A32JitState, rsb_ptr)], eax);
    code.cmp(rbx, qword[r15 + offsetof(A32JitState, rsb_location_descriptors) + rax * sizeof(u64)]);
    if (conf.HasOptimization(OptimizationFlag::FastDispatch)) {
        code.jne(rsb_cache_miss);
    } else {
        code.jne(code.GetReturnFromRunCodeAddress());
    }
    code.mov(rax, qword[r15 + offsetof(A32JitState, rsb_codeptrs) + rax * sizeof(u64)]);
    code.jmp(rax);
    PerfMapRegister(terminal_handler_pop_rsb_hint, code.getCurr(), "a32_terminal_handler_pop_rsb_hint");

    if (conf.HasOptimization(OptimizationFlag::FastDispatch)) {
        code.align();
        terminal_handler_fast_dispatch_hint = code.getCurr<const void*>();
        calculate_location_descriptor();
        code.L(rsb_cache_miss);
        code.mov(r12, reinterpret_cast<u64>(fast_dispatch_table.data()));
        if (code.HasHostFeature(HostFeature::SSE42)) {
            code.crc32(ebp, r12d);
        }
        code.and_(ebp, fast_dispatch_table_mask);
        code.lea(rbp, ptr[r12 + rbp]);
        code.cmp(rbx, qword[rbp + offsetof(FastDispatchEntry, location_descriptor)]);
        code.jne(fast_dispatch_cache_miss);
        code.jmp(ptr[rbp + offsetof(FastDispatchEntry, code_ptr)]);
        code.L(fast_dispatch_cache_miss);
        code.mov(qword[rbp + offsetof(FastDispatchEntry, location_descriptor)], rbx);
        code.LookupBlock();
        code.mov(ptr[rbp + offsetof(FastDispatchEntry, code_ptr)], rax);
        code.jmp(rax);
        PerfMapRegister(terminal_handler_fast_dispatch_hint, code.getCurr(), "a32_terminal_handler_fast_dispatch_hint");

        code.align();
        fast_dispatch_table_lookup = code.getCurr<FastDispatchEntry& (*)(u64)>();
        code.mov(code.ABI_PARAM2, reinterpret_cast<u64>(fast_dispatch_table.data()));
        if (code.HasHostFeature(HostFeature::SSE42)) {
            code.crc32(code.ABI_PARAM1.cvt32(), code.ABI_PARAM2.cvt32());
        }
        code.and_(code.ABI_PARAM1.cvt32(), fast_dispatch_table_mask);
        code.lea(code.ABI_RETURN, code.ptr[code.ABI_PARAM1 + code.ABI_PARAM2]);
        code.ret();
    }
}

void A32EmitX64::EmitA32SetCheckBit(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg8 to_store = ctx.reg_alloc.UseGpr(args[0]).cvt8();
    code.mov(code.byte[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, check_bit)], to_store);
}

void A32EmitX64::EmitA32GetRegister(A32EmitContext& ctx, IR::Inst* inst) {
    const A32::Reg reg = inst->GetArg(0).GetA32RegRef();
    const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();

    code.mov(result, MJitStateReg(reg));
    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32GetExtendedRegister32(A32EmitContext& ctx, IR::Inst* inst) {
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsSingleExtReg(reg));

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    code.movss(result, MJitStateExtReg(reg));
    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32GetExtendedRegister64(A32EmitContext& ctx, IR::Inst* inst) {
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsDoubleExtReg(reg));

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    code.movsd(result, MJitStateExtReg(reg));
    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32GetVector(A32EmitContext& ctx, IR::Inst* inst) {
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsDoubleExtReg(reg) || A32::IsQuadExtReg(reg));

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    if (A32::IsDoubleExtReg(reg)) {
        code.movsd(result, MJitStateExtReg(reg));
    } else {
        code.movaps(result, MJitStateExtReg(reg));
    }
    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32SetRegister(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const A32::Reg reg = inst->GetArg(0).GetA32RegRef();

    if (args[1].IsImmediate()) {
        code.mov(MJitStateReg(reg), args[1].GetImmediateU32());
    } else if (args[1].IsInXmm()) {
        const Xbyak::Xmm to_store = ctx.reg_alloc.UseXmm(args[1]);
        code.movd(MJitStateReg(reg), to_store);
    } else {
        const Xbyak::Reg32 to_store = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        code.mov(MJitStateReg(reg), to_store);
    }
}

void A32EmitX64::EmitA32SetExtendedRegister32(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsSingleExtReg(reg));

    if (args[1].IsInXmm()) {
        Xbyak::Xmm to_store = ctx.reg_alloc.UseXmm(args[1]);
        code.movss(MJitStateExtReg(reg), to_store);
    } else {
        Xbyak::Reg32 to_store = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        code.mov(MJitStateExtReg(reg), to_store);
    }
}

void A32EmitX64::EmitA32SetExtendedRegister64(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsDoubleExtReg(reg));

    if (args[1].IsInXmm()) {
        const Xbyak::Xmm to_store = ctx.reg_alloc.UseXmm(args[1]);
        code.movsd(MJitStateExtReg(reg), to_store);
    } else {
        const Xbyak::Reg64 to_store = ctx.reg_alloc.UseGpr(args[1]);
        code.mov(MJitStateExtReg(reg), to_store);
    }
}

void A32EmitX64::EmitA32SetVector(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsDoubleExtReg(reg) || A32::IsQuadExtReg(reg));

    const Xbyak::Xmm to_store = ctx.reg_alloc.UseXmm(args[1]);
    if (A32::IsDoubleExtReg(reg)) {
        code.movsd(MJitStateExtReg(reg), to_store);
    } else {
        code.movaps(MJitStateExtReg(reg), to_store);
    }
}

void A32EmitX64::EmitA32GetCpsr(A32EmitContext& ctx, IR::Inst* inst) {
    const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 tmp2 = ctx.reg_alloc.ScratchGpr().cvt32();

    if (code.HasHostFeature(HostFeature::FastBMI2)) {
        // Here we observe that cpsr_et and cpsr_ge are right next to each other in memory,
        // so we load them both at the same time with one 64-bit read. This allows us to
        // extract all of their bits together at once with one pext.
        static_assert(offsetof(A32JitState, upper_location_descriptor) + 4 == offsetof(A32JitState, cpsr_ge));
        code.mov(result.cvt64(), qword[r15 + offsetof(A32JitState, upper_location_descriptor)]);
        code.mov(tmp.cvt64(), 0x80808080'00000003ull);
        code.pext(result.cvt64(), result.cvt64(), tmp.cvt64());
        code.mov(tmp, 0x000f0220);
        code.pdep(result, result, tmp);
    } else {
        code.mov(result, dword[r15 + offsetof(A32JitState, upper_location_descriptor)]);
        code.imul(result, result, 0x12);
        code.and_(result, 0x00000220);

        code.mov(tmp, dword[r15 + offsetof(A32JitState, cpsr_ge)]);
        code.and_(tmp, 0x80808080);
        code.imul(tmp, tmp, 0x00204081);
        code.shr(tmp, 12);
        code.and_(tmp, 0x000f0000);
        code.or_(result, tmp);
    }

    code.mov(tmp, dword[r15 + offsetof(A32JitState, cpsr_q)]);
    code.shl(tmp, 27);
    code.or_(result, tmp);

    code.mov(tmp2, dword[r15 + offsetof(A32JitState, cpsr_nzcv)]);
    if (code.HasHostFeature(HostFeature::FastBMI2)) {
        code.mov(tmp, NZCV::x64_mask);
        code.pext(tmp2, tmp2, tmp);
        code.shl(tmp2, 28);
    } else {
        code.and_(tmp2, NZCV::x64_mask);
        code.imul(tmp2, tmp2, NZCV::from_x64_multiplier);
        code.and_(tmp2, NZCV::arm_mask);
    }
    code.or_(result, tmp2);

    code.or_(result, dword[r15 + offsetof(A32JitState, cpsr_jaifm)]);

    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32SetCpsr(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 cpsr = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 tmp2 = ctx.reg_alloc.ScratchGpr().cvt32();

    if (conf.always_little_endian) {
        code.and_(cpsr, 0xFFFFFDFF);
    }

    // cpsr_q
    code.bt(cpsr, 27);
    code.setc(code.byte[r15 + offsetof(A32JitState, cpsr_q)]);

    // cpsr_nzcv
    code.mov(tmp, cpsr);
    code.shr(tmp, 28);
    if (code.HasHostFeature(HostFeature::FastBMI2)) {
        code.mov(tmp2, NZCV::x64_mask);
        code.pdep(tmp, tmp, tmp2);
    } else {
        code.imul(tmp, tmp, NZCV::to_x64_multiplier);
        code.and_(tmp, NZCV::x64_mask);
    }
    code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], tmp);

    // cpsr_jaifm
    code.mov(tmp, cpsr);
    code.and_(tmp, 0x07F0FDDF);
    code.mov(dword[r15 + offsetof(A32JitState, cpsr_jaifm)], tmp);

    if (code.HasHostFeature(HostFeature::FastBMI2)) {
        // cpsr_et and cpsr_ge
        static_assert(offsetof(A32JitState, upper_location_descriptor) + 4 == offsetof(A32JitState, cpsr_ge));
        // This mask is 0x7FFF0000, because we do not want the MSB to be sign extended to the upper dword.
        static_assert((A32::LocationDescriptor::FPSCR_MODE_MASK & ~0x7FFF0000) == 0);

        code.and_(qword[r15 + offsetof(A32JitState, upper_location_descriptor)], u32(0x7FFF0000));
        code.mov(tmp, 0x000f0220);
        code.pext(cpsr, cpsr, tmp);
        code.mov(tmp.cvt64(), 0x01010101'00000003ull);
        code.pdep(cpsr.cvt64(), cpsr.cvt64(), tmp.cvt64());
        // We perform SWAR partitioned subtraction here, to negate the GE bytes.
        code.mov(tmp.cvt64(), 0x80808080'00000003ull);
        code.mov(tmp2.cvt64(), tmp.cvt64());
        code.sub(tmp.cvt64(), cpsr.cvt64());
        code.xor_(tmp.cvt64(), tmp2.cvt64());
        code.or_(qword[r15 + offsetof(A32JitState, upper_location_descriptor)], tmp.cvt64());
    } else {
        code.and_(dword[r15 + offsetof(A32JitState, upper_location_descriptor)], u32(0xFFFF0000));
        code.mov(tmp, cpsr);
        code.and_(tmp, 0x00000220);
        code.imul(tmp, tmp, 0x00900000);
        code.shr(tmp, 28);
        code.or_(dword[r15 + offsetof(A32JitState, upper_location_descriptor)], tmp);

        code.and_(cpsr, 0x000f0000);
        code.shr(cpsr, 16);
        code.imul(cpsr, cpsr, 0x00204081);
        code.and_(cpsr, 0x01010101);
        code.mov(tmp, 0x80808080);
        code.sub(tmp, cpsr);
        code.xor_(tmp, 0x80808080);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_ge)], tmp);
    }
}

void A32EmitX64::EmitA32SetCpsrNZCV(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg32 to_store = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], to_store);
}

void A32EmitX64::EmitA32SetCpsrNZCVRaw(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (args[0].IsImmediate()) {
        const u32 imm = args[0].GetImmediateU32();

        code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], NZCV::ToX64(imm));
    } else if (code.HasHostFeature(HostFeature::FastBMI2)) {
        const Xbyak::Reg32 a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 b = ctx.reg_alloc.ScratchGpr().cvt32();

        code.shr(a, 28);
        code.mov(b, NZCV::x64_mask);
        code.pdep(a, a, b);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], a);
    } else {
        const Xbyak::Reg32 a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

        code.shr(a, 28);
        code.imul(a, a, NZCV::to_x64_multiplier);
        code.and_(a, NZCV::x64_mask);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], a);
    }
}

void A32EmitX64::EmitA32SetCpsrNZCVQ(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (args[0].IsImmediate()) {
        const u32 imm = args[0].GetImmediateU32();

        code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], NZCV::ToX64(imm));
        code.mov(code.byte[r15 + offsetof(A32JitState, cpsr_q)], u8((imm & 0x08000000) != 0 ? 1 : 0));
    } else if (code.HasHostFeature(HostFeature::FastBMI2)) {
        const Xbyak::Reg32 a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 b = ctx.reg_alloc.ScratchGpr().cvt32();

        code.shr(a, 28);
        code.setc(code.byte[r15 + offsetof(A32JitState, cpsr_q)]);
        code.mov(b, NZCV::x64_mask);
        code.pdep(a, a, b);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], a);
    } else {
        const Xbyak::Reg32 a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

        code.shr(a, 28);
        code.setc(code.byte[r15 + offsetof(A32JitState, cpsr_q)]);
        code.imul(a, a, NZCV::to_x64_multiplier);
        code.and_(a, NZCV::x64_mask);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], a);
    }
}

static void EmitGetFlag(BlockOfCode& code, A32EmitContext& ctx, IR::Inst* inst, size_t flag_bit) {
    const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
    code.mov(result, dword[r15 + offsetof(A32JitState, cpsr_nzcv)]);
    if (flag_bit != 0) {
        code.shr(result, static_cast<int>(flag_bit));
    }
    code.and_(result, 1);
    ctx.reg_alloc.DefineValue(inst, result);
}

static void EmitSetFlag(BlockOfCode& code, A32EmitContext& ctx, IR::Inst* inst, size_t flag_bit) {
    const u32 flag_mask = 1u << flag_bit;
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (args[0].IsImmediate()) {
        if (args[0].GetImmediateU1()) {
            code.or_(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], flag_mask);
        } else {
            code.and_(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], ~flag_mask);
        }
    } else {
        const Xbyak::Reg32 to_store = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

        if (flag_bit != 0) {
            code.shl(to_store, static_cast<int>(flag_bit));
            code.and_(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], ~flag_mask);
            code.or_(dword[r15 + offsetof(A32JitState, cpsr_nzcv)], to_store);
        } else {
            code.mov(code.byte[r15 + offsetof(A32JitState, cpsr_nzcv)], to_store.cvt8());
        }
    }
}

void A32EmitX64::EmitA32SetNFlag(A32EmitContext& ctx, IR::Inst* inst) {
    EmitSetFlag(code, ctx, inst, NZCV::x64_n_flag_bit);
}

void A32EmitX64::EmitA32SetZFlag(A32EmitContext& ctx, IR::Inst* inst) {
    EmitSetFlag(code, ctx, inst, NZCV::x64_z_flag_bit);
}

void A32EmitX64::EmitA32GetCFlag(A32EmitContext& ctx, IR::Inst* inst) {
    EmitGetFlag(code, ctx, inst, NZCV::x64_c_flag_bit);
}

void A32EmitX64::EmitA32SetCFlag(A32EmitContext& ctx, IR::Inst* inst) {
    EmitSetFlag(code, ctx, inst, NZCV::x64_c_flag_bit);
}

void A32EmitX64::EmitA32SetVFlag(A32EmitContext& ctx, IR::Inst* inst) {
    EmitSetFlag(code, ctx, inst, NZCV::x64_v_flag_bit);
}

void A32EmitX64::EmitA32OrQFlag(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (args[0].IsImmediate()) {
        if (args[0].GetImmediateU1()) {
            code.mov(dword[r15 + offsetof(A32JitState, cpsr_q)], 1);
        }
    } else {
        const Xbyak::Reg8 to_store = ctx.reg_alloc.UseGpr(args[0]).cvt8();

        code.or_(code.byte[r15 + offsetof(A32JitState, cpsr_q)], to_store);
    }
}

void A32EmitX64::EmitA32GetGEFlags(A32EmitContext& ctx, IR::Inst* inst) {
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    code.movd(result, dword[r15 + offsetof(A32JitState, cpsr_ge)]);
    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32SetGEFlags(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(!args[0].IsImmediate());

    if (args[0].IsInXmm()) {
        const Xbyak::Xmm to_store = ctx.reg_alloc.UseXmm(args[0]);
        code.movd(dword[r15 + offsetof(A32JitState, cpsr_ge)], to_store);
    } else {
        const Xbyak::Reg32 to_store = ctx.reg_alloc.UseGpr(args[0]).cvt32();
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_ge)], to_store);
    }
}

void A32EmitX64::EmitA32SetGEFlagsCompressed(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (args[0].IsImmediate()) {
        const u32 imm = args[0].GetImmediateU32();
        u32 ge = 0;
        ge |= Common::Bit<19>(imm) ? 0xFF000000 : 0;
        ge |= Common::Bit<18>(imm) ? 0x00FF0000 : 0;
        ge |= Common::Bit<17>(imm) ? 0x0000FF00 : 0;
        ge |= Common::Bit<16>(imm) ? 0x000000FF : 0;

        code.mov(dword[r15 + offsetof(A32JitState, cpsr_ge)], ge);
    } else if (code.HasHostFeature(HostFeature::FastBMI2)) {
        const Xbyak::Reg32 a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 b = ctx.reg_alloc.ScratchGpr().cvt32();

        code.mov(b, 0x01010101);
        code.shr(a, 16);
        code.pdep(a, a, b);
        code.imul(a, a, 0xFF);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_ge)], a);
    } else {
        const Xbyak::Reg32 a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

        code.shr(a, 16);
        code.and_(a, 0xF);
        code.imul(a, a, 0x00204081);
        code.and_(a, 0x01010101);
        code.imul(a, a, 0xFF);
        code.mov(dword[r15 + offsetof(A32JitState, cpsr_ge)], a);
    }
}

void A32EmitX64::EmitA32DataSynchronizationBarrier(A32EmitContext&, IR::Inst*) {
    code.mfence();
    code.lfence();
}

void A32EmitX64::EmitA32DataMemoryBarrier(A32EmitContext&, IR::Inst*) {
    code.mfence();
}

void A32EmitX64::EmitA32InstructionSynchronizationBarrier(A32EmitContext& ctx, IR::Inst*) {
    if (!conf.hook_isb) {
        return;
    }

    ctx.reg_alloc.HostCall(nullptr);
    Devirtualize<&A32::UserCallbacks::InstructionSynchronizationBarrierRaised>(conf.callbacks).EmitCall(code);
}

void A32EmitX64::EmitA32BXWritePC(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& arg = args[0];

    const u32 upper_without_t = (ctx.EndLocation().SetSingleStepping(false).UniqueHash() >> 32) & 0xFFFFFFFE;

    // Pseudocode:
    // if (new_pc & 1) {
    //    new_pc &= 0xFFFFFFFE;
    //    cpsr.T = true;
    // } else {
    //    new_pc &= 0xFFFFFFFC;
    //    cpsr.T = false;
    // }
    // We rely on the fact we disallow EFlag from changing within a block.

    if (arg.IsImmediate()) {
        const u32 new_pc = arg.GetImmediateU32();
        const u32 mask = Common::Bit<0>(new_pc) ? 0xFFFFFFFE : 0xFFFFFFFC;
        const u32 new_upper = upper_without_t | (Common::Bit<0>(new_pc) ? 1 : 0);

        code.mov(MJitStateReg(A32::Reg::PC), new_pc & mask);
        code.mov(dword[r15 + offsetof(A32JitState, upper_location_descriptor)], new_upper);
    } else {
        const Xbyak::Reg32 new_pc = ctx.reg_alloc.UseScratchGpr(arg).cvt32();
        const Xbyak::Reg32 mask = ctx.reg_alloc.ScratchGpr().cvt32();
        const Xbyak::Reg32 new_upper = ctx.reg_alloc.ScratchGpr().cvt32();

        code.mov(mask, new_pc);
        code.and_(mask, 1);
        code.lea(new_upper, ptr[mask.cvt64() + upper_without_t]);
        code.lea(mask, ptr[mask.cvt64() + mask.cvt64() * 1 - 4]);  // mask = pc & 1 ? 0xFFFFFFFE : 0xFFFFFFFC
        code.and_(new_pc, mask);
        code.mov(MJitStateReg(A32::Reg::PC), new_pc);
        code.mov(dword[r15 + offsetof(A32JitState, upper_location_descriptor)], new_upper);
    }
}

void A32EmitX64::EmitA32UpdateUpperLocationDescriptor(A32EmitContext& ctx, IR::Inst*) {
    for (auto& inst : ctx.block) {
        if (inst.GetOpcode() == IR::Opcode::A32BXWritePC) {
            return;
        }
    }
    EmitSetUpperLocationDescriptor(ctx.EndLocation(), ctx.Location());
}

void A32EmitX64::EmitA32CallSupervisor(A32EmitContext& ctx, IR::Inst* inst) {
    ctx.reg_alloc.HostCall(nullptr);

    code.SwitchMxcsrOnExit();
    code.mov(code.ABI_PARAM2, qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)]);
    code.sub(code.ABI_PARAM2, qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)]);
    Devirtualize<&A32::UserCallbacks::AddTicks>(conf.callbacks).EmitCall(code);
    ctx.reg_alloc.EndOfAllocScope();

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.HostCall(nullptr, {}, args[0]);
    Devirtualize<&A32::UserCallbacks::CallSVC>(conf.callbacks).EmitCall(code);

    Devirtualize<&A32::UserCallbacks::GetTicksRemaining>(conf.callbacks).EmitCall(code);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)], code.ABI_RETURN);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], code.ABI_RETURN);
    code.SwitchMxcsrOnEntry();
}

void A32EmitX64::EmitA32ExceptionRaised(A32EmitContext& ctx, IR::Inst* inst) {
    ctx.reg_alloc.HostCall(nullptr);

    code.SwitchMxcsrOnExit();
    code.mov(code.ABI_PARAM2, qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)]);
    code.sub(code.ABI_PARAM2, qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)]);
    Devirtualize<&A32::UserCallbacks::AddTicks>(conf.callbacks).EmitCall(code);
    ctx.reg_alloc.EndOfAllocScope();

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[0].IsImmediate() && args[1].IsImmediate());
    const u32 pc = args[0].GetImmediateU32();
    const u64 exception = args[1].GetImmediateU64();
    Devirtualize<&A32::UserCallbacks::ExceptionRaised>(conf.callbacks).EmitCall(code, [&](RegList param) {
        code.mov(param[0], pc);
        code.mov(param[1], exception);
    });

    Devirtualize<&A32::UserCallbacks::GetTicksRemaining>(conf.callbacks).EmitCall(code);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_to_run)], code.ABI_RETURN);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], code.ABI_RETURN);
    code.SwitchMxcsrOnEntry();
}

static u32 GetFpscrImpl(A32JitState* jit_state) {
    return jit_state->Fpscr();
}

void A32EmitX64::EmitA32GetFpscr(A32EmitContext& ctx, IR::Inst* inst) {
    ctx.reg_alloc.HostCall(inst);
    code.mov(code.ABI_PARAM1, code.r15);

    code.stmxcsr(code.dword[code.r15 + offsetof(A32JitState, guest_MXCSR)]);
    code.CallFunction(&GetFpscrImpl);
}

static void SetFpscrImpl(u32 value, A32JitState* jit_state) {
    jit_state->SetFpscr(value);
}

void A32EmitX64::EmitA32SetFpscr(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.HostCall(nullptr, args[0]);
    code.mov(code.ABI_PARAM2, code.r15);

    code.CallFunction(&SetFpscrImpl);
    code.ldmxcsr(code.dword[code.r15 + offsetof(A32JitState, guest_MXCSR)]);
}

void A32EmitX64::EmitA32GetFpscrNZCV(A32EmitContext& ctx, IR::Inst* inst) {
    const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
    code.mov(result, dword[r15 + offsetof(A32JitState, fpsr_nzcv)]);
    ctx.reg_alloc.DefineValue(inst, result);
}

void A32EmitX64::EmitA32SetFpscrNZCV(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::FastBMI2)) {
        const Xbyak::Reg32 value = ctx.reg_alloc.UseGpr(args[0]).cvt32();
        const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();

        code.mov(tmp, NZCV::x64_mask);
        code.pext(tmp, value, tmp);
        code.shl(tmp, 28);
        code.mov(dword[r15 + offsetof(A32JitState, fpsr_nzcv)], tmp);

        return;
    }

    const Xbyak::Reg32 value = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

    code.and_(value, NZCV::x64_mask);
    code.imul(value, value, NZCV::from_x64_multiplier);
    code.and_(value, NZCV::arm_mask);
    code.mov(dword[r15 + offsetof(A32JitState, fpsr_nzcv)], value);
}

void A32EmitX64::EmitA32ClearExclusive(A32EmitContext&, IR::Inst*) {
    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
}

std::optional<A32EmitX64::DoNotFastmemMarker> A32EmitX64::ShouldFastmem(A32EmitContext& ctx, IR::Inst* inst) const {
    if (!conf.fastmem_pointer || !exception_handler.SupportsFastmem()) {
        return std::nullopt;
    }

    const auto marker = std::make_tuple(ctx.Location(), ctx.GetInstOffset(inst));
    if (do_not_fastmem.count(marker) > 0) {
        return std::nullopt;
    }
    return marker;
}

FakeCall A32EmitX64::FastmemCallback(u64 rip_) {
    const auto iter = fastmem_patch_info.find(rip_);

    if (iter == fastmem_patch_info.end()) {
        fmt::print("dynarmic: Segfault happened within JITted code at rip = {:016x}\n", rip_);
        fmt::print("Segfault wasn't at a fastmem patch location!\n");
        fmt::print("Now dumping code.......\n\n");
        Common::DumpDisassembledX64((void*)(rip_ & ~u64(0xFFF)), 0x1000);
        ASSERT_FALSE("iter != fastmem_patch_info.end()");
    }

    if (conf.recompile_on_fastmem_failure) {
        const auto marker = iter->second.marker;
        do_not_fastmem.emplace(marker);
        InvalidateBasicBlocks({std::get<0>(marker)});
    }
    FakeCall ret;
    ret.call_rip = iter->second.callback;
    ret.ret_rip = iter->second.resume_rip;
    return ret;
}

namespace {

constexpr size_t page_bits = 12;
constexpr size_t page_size = 1 << page_bits;
constexpr size_t page_mask = (1 << page_bits) - 1;

void EmitDetectMisaignedVAddr(BlockOfCode& code, A32EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg32 vaddr, Xbyak::Reg32 tmp) {
    if (bitsize == 8 || (ctx.conf.detect_misaligned_access_via_page_table & bitsize) == 0) {
        return;
    }

    const u32 align_mask = [bitsize]() -> u32 {
        switch (bitsize) {
        case 16:
            return 0b1;
        case 32:
            return 0b11;
        case 64:
            return 0b111;
        }
        UNREACHABLE();
    }();

    code.test(vaddr, align_mask);

    if (!ctx.conf.only_detect_misalignment_via_page_table_on_page_boundary) {
        code.jnz(abort, code.T_NEAR);
        return;
    }

    const u32 page_align_mask = static_cast<u32>(page_size - 1) & ~align_mask;

    Xbyak::Label detect_boundary, resume;

    code.jnz(detect_boundary, code.T_NEAR);
    code.L(resume);

    code.SwitchToFarCode();
    code.L(detect_boundary);
    code.mov(tmp, vaddr);
    code.and_(tmp, page_align_mask);
    code.cmp(tmp, page_align_mask);
    code.jne(resume, code.T_NEAR);
    // NOTE: We expect to fallthrough into abort code here.
    code.SwitchToNearCode();
}

Xbyak::RegExp EmitVAddrLookup(BlockOfCode& code, A32EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr) {
    const Xbyak::Reg64 page = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg32 tmp = ctx.conf.absolute_offset_page_table ? page.cvt32() : ctx.reg_alloc.ScratchGpr().cvt32();

    EmitDetectMisaignedVAddr(code, ctx, bitsize, abort, vaddr.cvt32(), tmp);

    // TODO: This code assumes vaddr has been zext from 32-bits to 64-bits.

    code.mov(tmp, vaddr.cvt32());
    code.shr(tmp, static_cast<int>(page_bits));
    code.mov(page, qword[r14 + tmp.cvt64() * sizeof(void*)]);
    if (ctx.conf.page_table_pointer_mask_bits == 0) {
        code.test(page, page);
    } else {
        code.and_(page, ~u32(0) << ctx.conf.page_table_pointer_mask_bits);
    }
    code.jz(abort, code.T_NEAR);
    if (ctx.conf.absolute_offset_page_table) {
        return page + vaddr;
    }
    code.mov(tmp, vaddr.cvt32());
    code.and_(tmp, static_cast<u32>(page_mask));
    return page + tmp.cvt64();
}

template<std::size_t bitsize>
void EmitReadMemoryMov(BlockOfCode& code, const Xbyak::Reg64& value, const Xbyak::RegExp& addr) {
    switch (bitsize) {
    case 8:
        code.movzx(value.cvt32(), code.byte[addr]);
        return;
    case 16:
        code.movzx(value.cvt32(), word[addr]);
        return;
    case 32:
        code.mov(value.cvt32(), dword[addr]);
        return;
    case 64:
        code.mov(value, qword[addr]);
        return;
    default:
        ASSERT_FALSE("Invalid bitsize");
    }
}

template<std::size_t bitsize>
void EmitWriteMemoryMov(BlockOfCode& code, const Xbyak::RegExp& addr, const Xbyak::Reg64& value) {
    switch (bitsize) {
    case 8:
        code.mov(code.byte[addr], value.cvt8());
        return;
    case 16:
        code.mov(word[addr], value.cvt16());
        return;
    case 32:
        code.mov(dword[addr], value.cvt32());
        return;
    case 64:
        code.mov(qword[addr], value);
        return;
    default:
        ASSERT_FALSE("Invalid bitsize");
    }
}

}  // anonymous namespace

template<std::size_t bitsize, auto callback>
void A32EmitX64::EmitMemoryRead(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto fastmem_marker = ShouldFastmem(ctx, inst);

    if (!conf.page_table && !fastmem_marker) {
        // Neither fastmem nor page table: Use callbacks
        ctx.reg_alloc.HostCall(inst, {}, args[0]);
        Devirtualize<callback>(conf.callbacks).EmitCall(code);
        return;
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 value = ctx.reg_alloc.ScratchGpr();

    const auto wrapped_fn = read_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value.getIdx())];

    if (fastmem_marker) {
        // Use fastmem
        const auto src_ptr = r13 + vaddr;

        const auto location = code.getCurr();
        EmitReadMemoryMov<bitsize>(code, value, src_ptr);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
            });

        ctx.reg_alloc.DefineValue(inst, value);
        return;
    }

    // Use page table
    ASSERT(conf.page_table);
    Xbyak::Label abort, end;

    const auto src_ptr = EmitVAddrLookup(code, ctx, bitsize, abort, vaddr);
    EmitReadMemoryMov<bitsize>(code, value, src_ptr);
    code.L(end);

    code.SwitchToFarCode();
    code.L(abort);
    code.call(wrapped_fn);
    code.jmp(end, code.T_NEAR);
    code.SwitchToNearCode();

    ctx.reg_alloc.DefineValue(inst, value);
}

template<std::size_t bitsize, auto callback>
void A32EmitX64::EmitMemoryWrite(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto fastmem_marker = ShouldFastmem(ctx, inst);

    if (!conf.page_table && !fastmem_marker) {
        // Neither fastmem nor page table: Use callbacks
        ctx.reg_alloc.HostCall(nullptr, {}, args[0], args[1]);
        Devirtualize<callback>(conf.callbacks).EmitCall(code);
        return;
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 value = ctx.reg_alloc.UseGpr(args[1]);

    const auto wrapped_fn = write_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value.getIdx())];

    if (fastmem_marker) {
        // Use fastmem
        const auto dest_ptr = r13 + vaddr;

        const auto location = code.getCurr();
        EmitWriteMemoryMov<bitsize>(code, dest_ptr, value);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
            });

        return;
    }

    // Use page table
    ASSERT(conf.page_table);
    Xbyak::Label abort, end;

    const auto dest_ptr = EmitVAddrLookup(code, ctx, bitsize, abort, vaddr);
    EmitWriteMemoryMov<bitsize>(code, dest_ptr, value);
    code.L(end);

    code.SwitchToFarCode();
    code.L(abort);
    code.call(wrapped_fn);
    code.jmp(end, code.T_NEAR);
    code.SwitchToNearCode();
}

void A32EmitX64::EmitA32ReadMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<8, &A32::UserCallbacks::MemoryRead8>(ctx, inst);
}

void A32EmitX64::EmitA32ReadMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<16, &A32::UserCallbacks::MemoryRead16>(ctx, inst);
}

void A32EmitX64::EmitA32ReadMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<32, &A32::UserCallbacks::MemoryRead32>(ctx, inst);
}

void A32EmitX64::EmitA32ReadMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<64, &A32::UserCallbacks::MemoryRead64>(ctx, inst);
}

void A32EmitX64::EmitA32WriteMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<8, &A32::UserCallbacks::MemoryWrite8>(ctx, inst);
}

void A32EmitX64::EmitA32WriteMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<16, &A32::UserCallbacks::MemoryWrite16>(ctx, inst);
}

void A32EmitX64::EmitA32WriteMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<32, &A32::UserCallbacks::MemoryWrite32>(ctx, inst);
}

void A32EmitX64::EmitA32WriteMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<64, &A32::UserCallbacks::MemoryWrite64>(ctx, inst);
}

template<size_t bitsize, auto callback>
void A32EmitX64::ExclusiveReadMemory(A32EmitContext& ctx, IR::Inst* inst) {
    using T = mp::unsigned_integer_of_size<bitsize>;

    ASSERT(conf.global_monitor != nullptr);
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.HostCall(inst, {}, args[0]);

    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(1));
    code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
    code.CallLambda(
        [](A32::UserConfig& conf, u32 vaddr) -> T {
            return conf.global_monitor->ReadAndMark<T>(conf.processor_id, vaddr, [&]() -> T {
                return (conf.callbacks->*callback)(vaddr);
            });
        });
}

template<size_t bitsize, auto callback>
void A32EmitX64::ExclusiveWriteMemory(A32EmitContext& ctx, IR::Inst* inst) {
    using T = mp::unsigned_integer_of_size<bitsize>;

    ASSERT(conf.global_monitor != nullptr);
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.HostCall(inst, {}, args[0], args[1]);

    Xbyak::Label end;

    code.mov(code.ABI_RETURN, u32(1));
    code.cmp(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
    code.je(end);
    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
    code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
    code.CallLambda(
        [](A32::UserConfig& conf, u32 vaddr, T value) -> u32 {
            return conf.global_monitor->DoExclusiveOperation<T>(conf.processor_id, vaddr,
                                                                [&](T expected) -> bool {
                                                                    return (conf.callbacks->*callback)(vaddr, value, expected);
                                                                })
                     ? 0
                     : 1;
        });
    code.L(end);
}

void A32EmitX64::EmitA32ExclusiveReadMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveReadMemory<8, &A32::UserCallbacks::MemoryRead8>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveReadMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveReadMemory<16, &A32::UserCallbacks::MemoryRead16>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveReadMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveReadMemory<32, &A32::UserCallbacks::MemoryRead32>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveReadMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveReadMemory<64, &A32::UserCallbacks::MemoryRead64>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveWriteMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveWriteMemory<8, &A32::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveWriteMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveWriteMemory<16, &A32::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveWriteMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveWriteMemory<32, &A32::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
}

void A32EmitX64::EmitA32ExclusiveWriteMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    ExclusiveWriteMemory<64, &A32::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
}

static void EmitCoprocessorException() {
    ASSERT_FALSE("Should raise coproc exception here");
}

static void CallCoprocCallback(BlockOfCode& code, RegAlloc& reg_alloc, A32::Jit* jit_interface, A32::Coprocessor::Callback callback, IR::Inst* inst = nullptr, std::optional<Argument::copyable_reference> arg0 = {}, std::optional<Argument::copyable_reference> arg1 = {}) {
    reg_alloc.HostCall(inst, {}, {}, arg0, arg1);

    code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(jit_interface));
    if (callback.user_arg) {
        code.mov(code.ABI_PARAM2, reinterpret_cast<u64>(*callback.user_arg));
    }

    code.CallFunction(callback.function);
}

void A32EmitX64::EmitA32CoprocInternalOperation(A32EmitContext& ctx, IR::Inst* inst) {
    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();
    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const auto opc1 = static_cast<unsigned>(coproc_info[2]);
    const auto CRd = static_cast<A32::CoprocReg>(coproc_info[3]);
    const auto CRn = static_cast<A32::CoprocReg>(coproc_info[4]);
    const auto CRm = static_cast<A32::CoprocReg>(coproc_info[5]);
    const auto opc2 = static_cast<unsigned>(coproc_info[6]);

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    const auto action = coproc->CompileInternalOperation(two, opc1, CRd, CRn, CRm, opc2);
    if (!action) {
        EmitCoprocessorException();
        return;
    }

    CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *action);
}

void A32EmitX64::EmitA32CoprocSendOneWord(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();
    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const auto opc1 = static_cast<unsigned>(coproc_info[2]);
    const auto CRn = static_cast<A32::CoprocReg>(coproc_info[3]);
    const auto CRm = static_cast<A32::CoprocReg>(coproc_info[4]);
    const auto opc2 = static_cast<unsigned>(coproc_info[5]);

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    const auto action = coproc->CompileSendOneWord(two, opc1, CRn, CRm, opc2);

    if (std::holds_alternative<std::monostate>(action)) {
        EmitCoprocessorException();
        return;
    }

    if (const auto cb = std::get_if<A32::Coprocessor::Callback>(&action)) {
        CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *cb, nullptr, args[1]);
        return;
    }

    if (const auto destination_ptr = std::get_if<u32*>(&action)) {
        const Xbyak::Reg32 reg_word = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        const Xbyak::Reg64 reg_destination_addr = ctx.reg_alloc.ScratchGpr();

        code.mov(reg_destination_addr, reinterpret_cast<u64>(*destination_ptr));
        code.mov(code.dword[reg_destination_addr], reg_word);

        return;
    }

    UNREACHABLE();
}

void A32EmitX64::EmitA32CoprocSendTwoWords(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();
    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const auto opc = static_cast<unsigned>(coproc_info[2]);
    const auto CRm = static_cast<A32::CoprocReg>(coproc_info[3]);

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    const auto action = coproc->CompileSendTwoWords(two, opc, CRm);

    if (std::holds_alternative<std::monostate>(action)) {
        EmitCoprocessorException();
        return;
    }

    if (const auto cb = std::get_if<A32::Coprocessor::Callback>(&action)) {
        CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *cb, nullptr, args[1], args[2]);
        return;
    }

    if (const auto destination_ptrs = std::get_if<std::array<u32*, 2>>(&action)) {
        const Xbyak::Reg32 reg_word1 = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        const Xbyak::Reg32 reg_word2 = ctx.reg_alloc.UseGpr(args[2]).cvt32();
        const Xbyak::Reg64 reg_destination_addr = ctx.reg_alloc.ScratchGpr();

        code.mov(reg_destination_addr, reinterpret_cast<u64>((*destination_ptrs)[0]));
        code.mov(code.dword[reg_destination_addr], reg_word1);
        code.mov(reg_destination_addr, reinterpret_cast<u64>((*destination_ptrs)[1]));
        code.mov(code.dword[reg_destination_addr], reg_word2);

        return;
    }

    UNREACHABLE();
}

void A32EmitX64::EmitA32CoprocGetOneWord(A32EmitContext& ctx, IR::Inst* inst) {
    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();

    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const auto opc1 = static_cast<unsigned>(coproc_info[2]);
    const auto CRn = static_cast<A32::CoprocReg>(coproc_info[3]);
    const auto CRm = static_cast<A32::CoprocReg>(coproc_info[4]);
    const auto opc2 = static_cast<unsigned>(coproc_info[5]);

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    const auto action = coproc->CompileGetOneWord(two, opc1, CRn, CRm, opc2);

    if (std::holds_alternative<std::monostate>(action)) {
        EmitCoprocessorException();
        return;
    }

    if (const auto cb = std::get_if<A32::Coprocessor::Callback>(&action)) {
        CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *cb, inst);
        return;
    }

    if (const auto source_ptr = std::get_if<u32*>(&action)) {
        const Xbyak::Reg32 reg_word = ctx.reg_alloc.ScratchGpr().cvt32();
        const Xbyak::Reg64 reg_source_addr = ctx.reg_alloc.ScratchGpr();

        code.mov(reg_source_addr, reinterpret_cast<u64>(*source_ptr));
        code.mov(reg_word, code.dword[reg_source_addr]);

        ctx.reg_alloc.DefineValue(inst, reg_word);

        return;
    }

    UNREACHABLE();
}

void A32EmitX64::EmitA32CoprocGetTwoWords(A32EmitContext& ctx, IR::Inst* inst) {
    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();
    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const unsigned opc = coproc_info[2];
    const auto CRm = static_cast<A32::CoprocReg>(coproc_info[3]);

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    auto action = coproc->CompileGetTwoWords(two, opc, CRm);

    if (std::holds_alternative<std::monostate>(action)) {
        EmitCoprocessorException();
        return;
    }

    if (const auto cb = std::get_if<A32::Coprocessor::Callback>(&action)) {
        CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *cb, inst);
        return;
    }

    if (const auto source_ptrs = std::get_if<std::array<u32*, 2>>(&action)) {
        const Xbyak::Reg64 reg_result = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 reg_destination_addr = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 reg_tmp = ctx.reg_alloc.ScratchGpr();

        code.mov(reg_destination_addr, reinterpret_cast<u64>((*source_ptrs)[1]));
        code.mov(reg_result.cvt32(), code.dword[reg_destination_addr]);
        code.shl(reg_result, 32);
        code.mov(reg_destination_addr, reinterpret_cast<u64>((*source_ptrs)[0]));
        code.mov(reg_tmp.cvt32(), code.dword[reg_destination_addr]);
        code.or_(reg_result, reg_tmp);

        ctx.reg_alloc.DefineValue(inst, reg_result);

        return;
    }

    UNREACHABLE();
}

void A32EmitX64::EmitA32CoprocLoadWords(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();
    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const bool long_transfer = coproc_info[2] != 0;
    const auto CRd = static_cast<A32::CoprocReg>(coproc_info[3]);
    const bool has_option = coproc_info[4] != 0;

    std::optional<u8> option = std::nullopt;
    if (has_option) {
        option = coproc_info[5];
    }

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    const auto action = coproc->CompileLoadWords(two, long_transfer, CRd, option);
    if (!action) {
        EmitCoprocessorException();
        return;
    }

    CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *action, nullptr, args[1]);
}

void A32EmitX64::EmitA32CoprocStoreWords(A32EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const auto coproc_info = inst->GetArg(0).GetCoprocInfo();
    const size_t coproc_num = coproc_info[0];
    const bool two = coproc_info[1] != 0;
    const bool long_transfer = coproc_info[2] != 0;
    const auto CRd = static_cast<A32::CoprocReg>(coproc_info[3]);
    const bool has_option = coproc_info[4] != 0;

    std::optional<u8> option = std::nullopt;
    if (has_option) {
        option = coproc_info[5];
    }

    std::shared_ptr<A32::Coprocessor> coproc = conf.coprocessors[coproc_num];
    if (!coproc) {
        EmitCoprocessorException();
        return;
    }

    const auto action = coproc->CompileStoreWords(two, long_transfer, CRd, option);
    if (!action) {
        EmitCoprocessorException();
        return;
    }

    CallCoprocCallback(code, ctx.reg_alloc, jit_interface, *action, nullptr, args[1]);
}

std::string A32EmitX64::LocationDescriptorToFriendlyName(const IR::LocationDescriptor& ir_descriptor) const {
    const A32::LocationDescriptor descriptor{ir_descriptor};
    return fmt::format("a32_{}{:08X}_{}_fpcr{:08X}",
                       descriptor.TFlag() ? "t" : "a",
                       descriptor.PC(),
                       descriptor.EFlag() ? "be" : "le",
                       descriptor.FPSCR().Value());
}

void A32EmitX64::EmitTerminalImpl(IR::Term::Interpret terminal, IR::LocationDescriptor initial_location, bool) {
    ASSERT_MSG(A32::LocationDescriptor{terminal.next}.TFlag() == A32::LocationDescriptor{initial_location}.TFlag(), "Unimplemented");
    ASSERT_MSG(A32::LocationDescriptor{terminal.next}.EFlag() == A32::LocationDescriptor{initial_location}.EFlag(), "Unimplemented");
    ASSERT_MSG(terminal.num_instructions == 1, "Unimplemented");

    code.mov(code.ABI_PARAM2.cvt32(), A32::LocationDescriptor{terminal.next}.PC());
    code.mov(code.ABI_PARAM3.cvt32(), 1);
    code.mov(MJitStateReg(A32::Reg::PC), code.ABI_PARAM2.cvt32());
    code.SwitchMxcsrOnExit();
    Devirtualize<&A32::UserCallbacks::InterpreterFallback>(conf.callbacks).EmitCall(code);
    code.ReturnFromRunCode(true);  // TODO: Check cycles
}

void A32EmitX64::EmitTerminalImpl(IR::Term::ReturnToDispatch, IR::LocationDescriptor, bool) {
    code.ReturnFromRunCode();
}

void A32EmitX64::EmitSetUpperLocationDescriptor(IR::LocationDescriptor new_location, IR::LocationDescriptor old_location) {
    auto get_upper = [](const IR::LocationDescriptor& desc) -> u32 {
        return static_cast<u32>(A32::LocationDescriptor{desc}.SetSingleStepping(false).UniqueHash() >> 32);
    };

    const u32 old_upper = get_upper(old_location);
    const u32 new_upper = [&] {
        const u32 mask = ~u32(conf.always_little_endian ? 0x2 : 0);
        return get_upper(new_location) & mask;
    }();

    if (old_upper != new_upper) {
        code.mov(dword[r15 + offsetof(A32JitState, upper_location_descriptor)], new_upper);
    }
}

void A32EmitX64::EmitTerminalImpl(IR::Term::LinkBlock terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    EmitSetUpperLocationDescriptor(terminal.next, initial_location);

    if (!conf.HasOptimization(OptimizationFlag::BlockLinking) || is_single_step) {
        code.mov(MJitStateReg(A32::Reg::PC), A32::LocationDescriptor{terminal.next}.PC());
        code.ReturnFromRunCode();
        return;
    }

    code.cmp(qword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, cycles_remaining)], 0);

    patch_information[terminal.next].jg.emplace_back(code.getCurr());
    if (const auto next_bb = GetBasicBlock(terminal.next)) {
        EmitPatchJg(terminal.next, next_bb->entrypoint);
    } else {
        EmitPatchJg(terminal.next);
    }
    Xbyak::Label dest;
    code.jmp(dest, Xbyak::CodeGenerator::T_NEAR);

    code.SwitchToFarCode();
    code.align(16);
    code.L(dest);
    code.mov(MJitStateReg(A32::Reg::PC), A32::LocationDescriptor{terminal.next}.PC());
    PushRSBHelper(rax, rbx, terminal.next);
    code.ForceReturnFromRunCode();
    code.SwitchToNearCode();
}

void A32EmitX64::EmitTerminalImpl(IR::Term::LinkBlockFast terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    EmitSetUpperLocationDescriptor(terminal.next, initial_location);

    if (!conf.HasOptimization(OptimizationFlag::BlockLinking) || is_single_step) {
        code.mov(MJitStateReg(A32::Reg::PC), A32::LocationDescriptor{terminal.next}.PC());
        code.ReturnFromRunCode();
        return;
    }

    patch_information[terminal.next].jmp.emplace_back(code.getCurr());
    if (const auto next_bb = GetBasicBlock(terminal.next)) {
        EmitPatchJmp(terminal.next, next_bb->entrypoint);
    } else {
        EmitPatchJmp(terminal.next);
    }
}

void A32EmitX64::EmitTerminalImpl(IR::Term::PopRSBHint, IR::LocationDescriptor, bool is_single_step) {
    if (!conf.HasOptimization(OptimizationFlag::ReturnStackBuffer) || is_single_step) {
        code.ReturnFromRunCode();
        return;
    }

    code.jmp(terminal_handler_pop_rsb_hint);
}

void A32EmitX64::EmitTerminalImpl(IR::Term::FastDispatchHint, IR::LocationDescriptor, bool is_single_step) {
    if (!conf.HasOptimization(OptimizationFlag::FastDispatch) || is_single_step) {
        code.ReturnFromRunCode();
        return;
    }

    code.jmp(terminal_handler_fast_dispatch_hint);
}

void A32EmitX64::EmitTerminalImpl(IR::Term::If terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    Xbyak::Label pass = EmitCond(terminal.if_);
    EmitTerminal(terminal.else_, initial_location, is_single_step);
    code.L(pass);
    EmitTerminal(terminal.then_, initial_location, is_single_step);
}

void A32EmitX64::EmitTerminalImpl(IR::Term::CheckBit terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    Xbyak::Label fail;
    code.cmp(code.byte[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, check_bit)], u8(0));
    code.jz(fail);
    EmitTerminal(terminal.then_, initial_location, is_single_step);
    code.L(fail);
    EmitTerminal(terminal.else_, initial_location, is_single_step);
}

void A32EmitX64::EmitTerminalImpl(IR::Term::CheckHalt terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    code.cmp(code.byte[r15 + offsetof(A32JitState, halt_requested)], u8(0));
    code.jne(code.GetForceReturnFromRunCodeAddress());
    EmitTerminal(terminal.else_, initial_location, is_single_step);
}

void A32EmitX64::EmitPatchJg(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr) {
    const CodePtr patch_location = code.getCurr();
    if (target_code_ptr) {
        code.jg(target_code_ptr);
    } else {
        code.mov(MJitStateReg(A32::Reg::PC), A32::LocationDescriptor{target_desc}.PC());
        code.jg(code.GetReturnFromRunCodeAddress());
    }
    code.EnsurePatchLocationSize(patch_location, 14);
}

void A32EmitX64::EmitPatchJmp(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr) {
    const CodePtr patch_location = code.getCurr();
    if (target_code_ptr) {
        code.jmp(target_code_ptr);
    } else {
        code.mov(MJitStateReg(A32::Reg::PC), A32::LocationDescriptor{target_desc}.PC());
        code.jmp(code.GetReturnFromRunCodeAddress());
    }
    code.EnsurePatchLocationSize(patch_location, 13);
}

void A32EmitX64::EmitPatchMovRcx(CodePtr target_code_ptr) {
    if (!target_code_ptr) {
        target_code_ptr = code.GetReturnFromRunCodeAddress();
    }
    const CodePtr patch_location = code.getCurr();
    code.mov(code.rcx, reinterpret_cast<u64>(target_code_ptr));
    code.EnsurePatchLocationSize(patch_location, 10);
}

void A32EmitX64::Unpatch(const IR::LocationDescriptor& location) {
    EmitX64::Unpatch(location);
    if (conf.HasOptimization(OptimizationFlag::FastDispatch)) {
        code.DisableWriting();
        (*fast_dispatch_table_lookup)(location.Value()) = {};
        code.EnableWriting();
    }
}

}  // namespace Dynarmic::Backend::X64
