/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>
#include <initializer_list>
#include <tuple>
#include <utility>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <mp/traits/integer_of_size.h>
#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/a32_emit_x64.h"
#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/devirtualize.h"
#include "dynarmic/backend/x64/emit_x64_memory.h"
#include "dynarmic/backend/x64/exclusive_monitor_friend.h"
#include "dynarmic/backend/x64/perf_map.h"
#include "dynarmic/common/x64_disassemble.h"
#include "dynarmic/interface/exclusive_monitor.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

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
    const std::array<std::pair<size_t, ArgCallback>, 4> exclusive_write_callbacks{{
        {8, Devirtualize<&A32::UserCallbacks::MemoryWriteExclusive8>(conf.callbacks)},
        {16, Devirtualize<&A32::UserCallbacks::MemoryWriteExclusive16>(conf.callbacks)},
        {32, Devirtualize<&A32::UserCallbacks::MemoryWriteExclusive32>(conf.callbacks)},
        {64, Devirtualize<&A32::UserCallbacks::MemoryWriteExclusive64>(conf.callbacks)},
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
                code.ZeroExtendFrom(bitsize, Xbyak::Reg64{value_idx});
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

            for (const auto& [bitsize, callback] : exclusive_write_callbacks) {
                code.align();
                exclusive_write_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
                ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLoc::RAX);
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
                code.mov(code.ABI_PARAM4, rax);
                callback.EmitCall(code);
                ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLoc::RAX);
                code.ret();
                PerfMapRegister(exclusive_write_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a32_exclusive_write_fallback_{}", bitsize));
            }
        }
    }
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

    if (iter->second.compile) {
        const auto marker = iter->second.marker;
        do_not_fastmem.emplace(marker);
        InvalidateBasicBlocks({std::get<0>(marker)});
    }

    return FakeCall{
        .call_rip = iter->second.callback,
        .ret_rip = iter->second.resume_rip,
    };
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
        code.ZeroExtendFrom(bitsize, code.ABI_RETURN);
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
                conf.recompile_on_fastmem_failure,
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
                conf.recompile_on_fastmem_failure,
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
    code.ZeroExtendFrom(bitsize, code.ABI_RETURN);
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

template<std::size_t bitsize, auto callback>
void A32EmitX64::ExclusiveReadMemoryInline(A32EmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor && conf.fastmem_pointer);
    if (!exception_handler.SupportsFastmem()) {
        ExclusiveReadMemory<bitsize, callback>(ctx, inst);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 value = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 tmp2 = ctx.reg_alloc.ScratchGpr();

    const auto wrapped_fn = read_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value.getIdx())];

    EmitExclusiveLock(code, conf, tmp, tmp2.cvt32());

    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(1));
    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, conf.processor_id)));
    code.mov(qword[tmp], vaddr);

    const auto fastmem_marker = ShouldFastmem(ctx, inst);
    if (fastmem_marker) {
        Xbyak::Label end;

        const auto src_ptr = r13 + vaddr;

        const auto location = code.getCurr();
        EmitReadMemoryMov<bitsize>(code, value, src_ptr);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
                conf.recompile_on_exclusive_fastmem_failure,
            });

        code.L(end);
    } else {
        code.call(wrapped_fn);
    }

    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorValuePointer(conf.global_monitor, conf.processor_id)));
    EmitWriteMemoryMov<bitsize>(code, tmp, value);

    EmitExclusiveUnlock(code, conf, tmp, tmp2.cvt32());

    ctx.reg_alloc.DefineValue(inst, value);
}

template<std::size_t bitsize, auto callback>
void A32EmitX64::ExclusiveWriteMemoryInline(A32EmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor && conf.fastmem_pointer);
    if (!exception_handler.SupportsFastmem()) {
        ExclusiveWriteMemory<bitsize, callback>(ctx, inst);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
    const Xbyak::Reg64 value = ctx.reg_alloc.UseGpr(args[1]);
    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg32 status = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();

    const auto fallback_fn = exclusive_write_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value.getIdx())];

    EmitExclusiveLock(code, conf, tmp, eax);

    Xbyak::Label end;

    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, conf.processor_id)));
    code.mov(status, u32(1));
    code.cmp(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
    code.je(end, code.T_NEAR);
    code.cmp(qword[tmp], vaddr);
    code.jne(end, code.T_NEAR);

    EmitExclusiveTestAndClear(code, conf, vaddr, tmp, rax);

    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorValuePointer(conf.global_monitor, conf.processor_id)));

    EmitReadMemoryMov<bitsize>(code, rax, tmp);

    const auto fastmem_marker = ShouldFastmem(ctx, inst);
    if (fastmem_marker) {
        const auto dest_ptr = r13 + vaddr;

        const auto location = code.getCurr();

        switch (bitsize) {
        case 8:
            code.lock();
            code.cmpxchg(code.byte[dest_ptr], value.cvt8());
            break;
        case 16:
            code.lock();
            code.cmpxchg(word[dest_ptr], value.cvt16());
            break;
        case 32:
            code.lock();
            code.cmpxchg(dword[dest_ptr], value.cvt32());
            break;
        case 64:
            code.lock();
            code.cmpxchg(qword[dest_ptr], value.cvt64());
            break;
        default:
            UNREACHABLE();
        }

        code.setnz(status.cvt8());

        code.SwitchToFarCode();

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(fallback_fn),
                *fastmem_marker,
                conf.recompile_on_exclusive_fastmem_failure,
            });

        code.cmp(al, 0);
        code.setz(status.cvt8());
        code.movzx(status.cvt32(), status.cvt8());
        code.jmp(end, code.T_NEAR);
        code.SwitchToNearCode();
    } else {
        code.call(fallback_fn);
        code.cmp(al, 0);
        code.setz(status.cvt8());
        code.movzx(status.cvt32(), status.cvt8());
    }

    code.L(end);

    EmitExclusiveUnlock(code, conf, tmp, eax);

    ctx.reg_alloc.DefineValue(inst, status);
}

void A32EmitX64::EmitA32ClearExclusive(A32EmitContext&, IR::Inst*) {
    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
}

void A32EmitX64::EmitA32ExclusiveReadMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveReadMemoryInline<8, &A32::UserCallbacks::MemoryRead8>(ctx, inst);
    } else {
        ExclusiveReadMemory<8, &A32::UserCallbacks::MemoryRead8>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveReadMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveReadMemoryInline<16, &A32::UserCallbacks::MemoryRead16>(ctx, inst);
    } else {
        ExclusiveReadMemory<16, &A32::UserCallbacks::MemoryRead16>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveReadMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveReadMemoryInline<32, &A32::UserCallbacks::MemoryRead32>(ctx, inst);
    } else {
        ExclusiveReadMemory<32, &A32::UserCallbacks::MemoryRead32>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveReadMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveReadMemoryInline<64, &A32::UserCallbacks::MemoryRead64>(ctx, inst);
    } else {
        ExclusiveReadMemory<64, &A32::UserCallbacks::MemoryRead64>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveWriteMemoryInline<8, &A32::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
    } else {
        ExclusiveWriteMemory<8, &A32::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveWriteMemoryInline<16, &A32::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
    } else {
        ExclusiveWriteMemory<16, &A32::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveWriteMemoryInline<32, &A32::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
    } else {
        ExclusiveWriteMemory<32, &A32::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        ExclusiveWriteMemoryInline<64, &A32::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
    } else {
        ExclusiveWriteMemory<64, &A32::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
    }
}

}  // namespace Dynarmic::Backend::X64
