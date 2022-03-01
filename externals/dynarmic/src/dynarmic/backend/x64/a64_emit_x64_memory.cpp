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

#include "dynarmic/backend/x64/a64_emit_x64.h"
#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/devirtualize.h"
#include "dynarmic/backend/x64/emit_x64_memory.h"
#include "dynarmic/backend/x64/exclusive_monitor_friend.h"
#include "dynarmic/backend/x64/perf_map.h"
#include "dynarmic/common/spin_lock_x64.h"
#include "dynarmic/common/x64_disassemble.h"
#include "dynarmic/interface/exclusive_monitor.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

void A64EmitX64::GenMemory128Accessors() {
    code.align();
    memory_read_128 = code.getCurr<void (*)()>();
#ifdef _WIN32
    Devirtualize<&A64::UserCallbacks::MemoryRead128>(conf.callbacks).EmitCallWithReturnPointer(code, [&](Xbyak::Reg64 return_value_ptr, [[maybe_unused]] RegList args) {
        code.mov(code.ABI_PARAM3, code.ABI_PARAM2);
        code.sub(rsp, 8 + 16 + ABI_SHADOW_SPACE);
        code.lea(return_value_ptr, ptr[rsp + ABI_SHADOW_SPACE]);
    });
    code.movups(xmm1, xword[code.ABI_RETURN]);
    code.add(rsp, 8 + 16 + ABI_SHADOW_SPACE);
#else
    code.sub(rsp, 8);
    Devirtualize<&A64::UserCallbacks::MemoryRead128>(conf.callbacks).EmitCall(code);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.movq(xmm1, code.ABI_RETURN);
        code.pinsrq(xmm1, code.ABI_RETURN2, 1);
    } else {
        code.movq(xmm1, code.ABI_RETURN);
        code.movq(xmm2, code.ABI_RETURN2);
        code.punpcklqdq(xmm1, xmm2);
    }
    code.add(rsp, 8);
#endif
    code.ret();
    PerfMapRegister(memory_read_128, code.getCurr(), "a64_memory_read_128");

    code.align();
    memory_write_128 = code.getCurr<void (*)()>();
#ifdef _WIN32
    code.sub(rsp, 8 + 16 + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE]);
    code.movaps(xword[code.ABI_PARAM3], xmm1);
    Devirtualize<&A64::UserCallbacks::MemoryWrite128>(conf.callbacks).EmitCall(code);
    code.add(rsp, 8 + 16 + ABI_SHADOW_SPACE);
#else
    code.sub(rsp, 8);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.movq(code.ABI_PARAM3, xmm1);
        code.pextrq(code.ABI_PARAM4, xmm1, 1);
    } else {
        code.movq(code.ABI_PARAM3, xmm1);
        code.punpckhqdq(xmm1, xmm1);
        code.movq(code.ABI_PARAM4, xmm1);
    }
    Devirtualize<&A64::UserCallbacks::MemoryWrite128>(conf.callbacks).EmitCall(code);
    code.add(rsp, 8);
#endif
    code.ret();
    PerfMapRegister(memory_write_128, code.getCurr(), "a64_memory_write_128");

    code.align();
    memory_exclusive_write_128 = code.getCurr<void (*)()>();
#ifdef _WIN32
    code.sub(rsp, 8 + 32 + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE]);
    code.lea(code.ABI_PARAM4, ptr[rsp + ABI_SHADOW_SPACE + 16]);
    code.movaps(xword[code.ABI_PARAM3], xmm1);
    code.movaps(xword[code.ABI_PARAM4], xmm2);
    Devirtualize<&A64::UserCallbacks::MemoryWriteExclusive128>(conf.callbacks).EmitCall(code);
    code.add(rsp, 8 + 16 + ABI_SHADOW_SPACE);
#else
    code.sub(rsp, 8);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.movq(code.ABI_PARAM3, xmm1);
        code.pextrq(code.ABI_PARAM4, xmm1, 1);
        code.movq(code.ABI_PARAM5, xmm2);
        code.pextrq(code.ABI_PARAM6, xmm2, 1);
    } else {
        code.movq(code.ABI_PARAM3, xmm1);
        code.punpckhqdq(xmm1, xmm1);
        code.movq(code.ABI_PARAM4, xmm1);
        code.movq(code.ABI_PARAM5, xmm2);
        code.punpckhqdq(xmm2, xmm2);
        code.movq(code.ABI_PARAM6, xmm2);
    }
    Devirtualize<&A64::UserCallbacks::MemoryWriteExclusive128>(conf.callbacks).EmitCall(code);
    code.add(rsp, 8);
#endif
    code.ret();
    PerfMapRegister(memory_exclusive_write_128, code.getCurr(), "a64_memory_exclusive_write_128");
}

void A64EmitX64::GenFastmemFallbacks() {
    const std::initializer_list<int> idxes{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const std::array<std::pair<size_t, ArgCallback>, 4> read_callbacks{{
        {8, Devirtualize<&A64::UserCallbacks::MemoryRead8>(conf.callbacks)},
        {16, Devirtualize<&A64::UserCallbacks::MemoryRead16>(conf.callbacks)},
        {32, Devirtualize<&A64::UserCallbacks::MemoryRead32>(conf.callbacks)},
        {64, Devirtualize<&A64::UserCallbacks::MemoryRead64>(conf.callbacks)},
    }};
    const std::array<std::pair<size_t, ArgCallback>, 4> write_callbacks{{
        {8, Devirtualize<&A64::UserCallbacks::MemoryWrite8>(conf.callbacks)},
        {16, Devirtualize<&A64::UserCallbacks::MemoryWrite16>(conf.callbacks)},
        {32, Devirtualize<&A64::UserCallbacks::MemoryWrite32>(conf.callbacks)},
        {64, Devirtualize<&A64::UserCallbacks::MemoryWrite64>(conf.callbacks)},
    }};
    const std::array<std::pair<size_t, ArgCallback>, 4> exclusive_write_callbacks{{
        {8, Devirtualize<&A64::UserCallbacks::MemoryWriteExclusive8>(conf.callbacks)},
        {16, Devirtualize<&A64::UserCallbacks::MemoryWriteExclusive16>(conf.callbacks)},
        {32, Devirtualize<&A64::UserCallbacks::MemoryWriteExclusive32>(conf.callbacks)},
        {64, Devirtualize<&A64::UserCallbacks::MemoryWriteExclusive64>(conf.callbacks)},
    }};

    for (int vaddr_idx : idxes) {
        if (vaddr_idx == 4 || vaddr_idx == 15) {
            continue;
        }

        for (int value_idx : idxes) {
            code.align();
            read_fallbacks[std::make_tuple(128, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(value_idx));
            if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
            }
            code.call(memory_read_128);
            if (value_idx != 1) {
                code.movaps(Xbyak::Xmm{value_idx}, xmm1);
            }
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(value_idx));
            code.ret();
            PerfMapRegister(read_fallbacks[std::make_tuple(128, vaddr_idx, value_idx)], code.getCurr(), "a64_read_fallback_128");

            code.align();
            write_fallbacks[std::make_tuple(128, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
            ABI_PushCallerSaveRegistersAndAdjustStack(code);
            if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
            }
            if (value_idx != 1) {
                code.movaps(xmm1, Xbyak::Xmm{value_idx});
            }
            code.call(memory_write_128);
            ABI_PopCallerSaveRegistersAndAdjustStack(code);
            code.ret();
            PerfMapRegister(write_fallbacks[std::make_tuple(128, vaddr_idx, value_idx)], code.getCurr(), "a64_write_fallback_128");

            code.align();
            exclusive_write_fallbacks[std::make_tuple(128, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLoc::RAX);
            if (value_idx != 1) {
                code.movaps(xmm1, Xbyak::Xmm{value_idx});
            }
            if (code.HasHostFeature(HostFeature::SSE41)) {
                code.movq(xmm2, rax);
                code.pinsrq(xmm2, rdx, 1);
            } else {
                code.movq(xmm2, rax);
                code.movq(xmm0, rdx);
                code.punpcklqdq(xmm2, xmm0);
            }
            if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
            }
            code.call(memory_exclusive_write_128);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLoc::RAX);
            code.ret();
            PerfMapRegister(exclusive_write_fallbacks[std::make_tuple(128, vaddr_idx, value_idx)], code.getCurr(), "a64_write_fallback_128");

            if (value_idx == 4 || value_idx == 15) {
                continue;
            }

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
                PerfMapRegister(read_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a64_read_fallback_{}", bitsize));
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
                PerfMapRegister(write_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a64_write_fallback_{}", bitsize));
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
                PerfMapRegister(exclusive_write_fallbacks[std::make_tuple(bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a64_exclusive_write_fallback_{}", bitsize));
            }
        }
    }
}

std::optional<A64EmitX64::DoNotFastmemMarker> A64EmitX64::ShouldFastmem(A64EmitContext& ctx, IR::Inst* inst) const {
    if (!conf.fastmem_pointer || !exception_handler.SupportsFastmem()) {
        return std::nullopt;
    }

    const auto marker = std::make_tuple(ctx.Location(), ctx.GetInstOffset(inst));
    if (do_not_fastmem.count(marker) > 0) {
        return std::nullopt;
    }
    return marker;
}

FakeCall A64EmitX64::FastmemCallback(u64 rip_) {
    const auto iter = fastmem_patch_info.find(rip_);

    if (iter == fastmem_patch_info.end()) {
        fmt::print("dynarmic: Segfault happened within JITted code at rip = {:016x}\n", rip_);
        fmt::print("Segfault wasn't at a fastmem patch location!\n");
        fmt::print("Now dumping code.......\n\n");
        Common::DumpDisassembledX64((void*)(rip_ & ~u64(0xFFF)), 0x1000);
        ASSERT_FALSE("iter != fastmem_patch_info.end()");
    }

    if (iter->second.recompile) {
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

void EmitDetectMisaignedVAddr(BlockOfCode& code, A64EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr, Xbyak::Reg64 tmp) {
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
        case 128:
            return 0b1111;
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

Xbyak::RegExp EmitVAddrLookup(BlockOfCode& code, A64EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr) {
    const size_t valid_page_index_bits = ctx.conf.page_table_address_space_bits - page_bits;
    const size_t unused_top_bits = 64 - ctx.conf.page_table_address_space_bits;

    const Xbyak::Reg64 page = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 tmp = ctx.conf.absolute_offset_page_table ? page : ctx.reg_alloc.ScratchGpr();

    EmitDetectMisaignedVAddr(code, ctx, bitsize, abort, vaddr, tmp);

    if (unused_top_bits == 0) {
        code.mov(tmp, vaddr);
        code.shr(tmp, int(page_bits));
    } else if (ctx.conf.silently_mirror_page_table) {
        if (valid_page_index_bits >= 32) {
            if (code.HasHostFeature(HostFeature::BMI2)) {
                const Xbyak::Reg64 bit_count = ctx.reg_alloc.ScratchGpr();
                code.mov(bit_count, unused_top_bits);
                code.bzhi(tmp, vaddr, bit_count);
                code.shr(tmp, int(page_bits));
                ctx.reg_alloc.Release(bit_count);
            } else {
                code.mov(tmp, vaddr);
                code.shl(tmp, int(unused_top_bits));
                code.shr(tmp, int(unused_top_bits + page_bits));
            }
        } else {
            code.mov(tmp, vaddr);
            code.shr(tmp, int(page_bits));
            code.and_(tmp, u32((1 << valid_page_index_bits) - 1));
        }
    } else {
        ASSERT(valid_page_index_bits < 32);
        code.mov(tmp, vaddr);
        code.shr(tmp, int(page_bits));
        code.test(tmp, u32(-(1 << valid_page_index_bits)));
        code.jnz(abort, code.T_NEAR);
    }
    code.mov(page, qword[r14 + tmp * sizeof(void*)]);
    if (ctx.conf.page_table_pointer_mask_bits == 0) {
        code.test(page, page);
    } else {
        code.and_(page, ~u32(0) << ctx.conf.page_table_pointer_mask_bits);
    }
    code.jz(abort, code.T_NEAR);
    if (ctx.conf.absolute_offset_page_table) {
        return page + vaddr;
    }
    code.mov(tmp, vaddr);
    code.and_(tmp, static_cast<u32>(page_mask));
    return page + tmp;
}

Xbyak::RegExp EmitFastmemVAddr(BlockOfCode& code, A64EmitContext& ctx, Xbyak::Label& abort, Xbyak::Reg64 vaddr, bool& require_abort_handling, std::optional<Xbyak::Reg64> tmp = std::nullopt) {
    const size_t unused_top_bits = 64 - ctx.conf.fastmem_address_space_bits;

    if (unused_top_bits == 0) {
        return r13 + vaddr;
    } else if (ctx.conf.silently_mirror_fastmem) {
        if (!tmp) {
            tmp = ctx.reg_alloc.ScratchGpr();
        }
        if (unused_top_bits < 32) {
            code.mov(*tmp, vaddr);
            code.shl(*tmp, int(unused_top_bits));
            code.shr(*tmp, int(unused_top_bits));
        } else if (unused_top_bits == 32) {
            code.mov(tmp->cvt32(), vaddr.cvt32());
        } else {
            code.mov(tmp->cvt32(), vaddr.cvt32());
            code.and_(*tmp, u32((1 << ctx.conf.fastmem_address_space_bits) - 1));
        }
        return r13 + *tmp;
    } else {
        if (ctx.conf.fastmem_address_space_bits < 32) {
            code.test(vaddr, u32(-(1 << ctx.conf.fastmem_address_space_bits)));
            code.jnz(abort, code.T_NEAR);
            require_abort_handling = true;
        } else {
            // TODO: Consider having TEST as above but coalesce 64-bit constant in register allocator
            if (!tmp) {
                tmp = ctx.reg_alloc.ScratchGpr();
            }
            code.mov(*tmp, vaddr);
            code.shr(*tmp, int(ctx.conf.fastmem_address_space_bits));
            code.jnz(abort, code.T_NEAR);
            require_abort_handling = true;
        }
        return r13 + vaddr;
    }
}

template<std::size_t bitsize>
void EmitReadMemoryMov(BlockOfCode& code, int value_idx, const Xbyak::RegExp& addr) {
    switch (bitsize) {
    case 8:
        code.movzx(Xbyak::Reg32{value_idx}, code.byte[addr]);
        return;
    case 16:
        code.movzx(Xbyak::Reg32{value_idx}, word[addr]);
        return;
    case 32:
        code.mov(Xbyak::Reg32{value_idx}, dword[addr]);
        return;
    case 64:
        code.mov(Xbyak::Reg64{value_idx}, qword[addr]);
        return;
    case 128:
        code.movups(Xbyak::Xmm{value_idx}, xword[addr]);
        return;
    default:
        ASSERT_FALSE("Invalid bitsize");
    }
}

template<std::size_t bitsize>
void EmitWriteMemoryMov(BlockOfCode& code, const Xbyak::RegExp& addr, int value_idx) {
    switch (bitsize) {
    case 8:
        code.mov(code.byte[addr], Xbyak::Reg64{value_idx}.cvt8());
        return;
    case 16:
        code.mov(word[addr], Xbyak::Reg16{value_idx});
        return;
    case 32:
        code.mov(dword[addr], Xbyak::Reg32{value_idx});
        return;
    case 64:
        code.mov(qword[addr], Xbyak::Reg64{value_idx});
        return;
    case 128:
        code.movups(xword[addr], Xbyak::Xmm{value_idx});
        return;
    default:
        ASSERT_FALSE("Invalid bitsize");
    }
}

}  // namespace

template<std::size_t bitsize, auto callback>
void A64EmitX64::EmitMemoryRead(A64EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto fastmem_marker = ShouldFastmem(ctx, inst);

    if (!conf.page_table && !fastmem_marker) {
        // Neither fastmem nor page table: Use callbacks
        if constexpr (bitsize == 128) {
            ctx.reg_alloc.HostCall(nullptr, {}, args[0]);
            code.CallFunction(memory_read_128);
            ctx.reg_alloc.DefineValue(inst, xmm1);
        } else {
            ctx.reg_alloc.HostCall(inst, {}, args[0]);
            Devirtualize<callback>(conf.callbacks).EmitCall(code);
            code.ZeroExtendFrom(bitsize, code.ABI_RETURN);
        }
        return;
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const int value_idx = bitsize == 128 ? ctx.reg_alloc.ScratchXmm().getIdx() : ctx.reg_alloc.ScratchGpr().getIdx();

    const auto wrapped_fn = read_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value_idx)];

    Xbyak::Label abort, end;
    bool require_abort_handling = false;

    if (fastmem_marker) {
        // Use fastmem
        const auto src_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling);

        const auto location = code.getCurr();
        EmitReadMemoryMov<bitsize>(code, value_idx, src_ptr);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
                conf.recompile_on_fastmem_failure,
            });
    } else {
        // Use page table
        ASSERT(conf.page_table);
        const auto src_ptr = EmitVAddrLookup(code, ctx, bitsize, abort, vaddr);
        require_abort_handling = true;
        EmitReadMemoryMov<bitsize>(code, value_idx, src_ptr);
    }
    code.L(end);

    if (require_abort_handling) {
        code.SwitchToFarCode();
        code.L(abort);
        code.call(wrapped_fn);
        code.jmp(end, code.T_NEAR);
        code.SwitchToNearCode();
    }

    if constexpr (bitsize == 128) {
        ctx.reg_alloc.DefineValue(inst, Xbyak::Xmm{value_idx});
    } else {
        ctx.reg_alloc.DefineValue(inst, Xbyak::Reg64{value_idx});
    }
}

template<std::size_t bitsize, auto callback>
void A64EmitX64::EmitMemoryWrite(A64EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto fastmem_marker = ShouldFastmem(ctx, inst);

    if (!conf.page_table && !fastmem_marker) {
        // Neither fastmem nor page table: Use callbacks
        if constexpr (bitsize == 128) {
            ctx.reg_alloc.Use(args[0], ABI_PARAM2);
            ctx.reg_alloc.Use(args[1], HostLoc::XMM1);
            ctx.reg_alloc.EndOfAllocScope();
            ctx.reg_alloc.HostCall(nullptr);
            code.CallFunction(memory_write_128);
        } else {
            ctx.reg_alloc.HostCall(nullptr, {}, args[0], args[1]);
            Devirtualize<callback>(conf.callbacks).EmitCall(code);
        }
        return;
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const int value_idx = bitsize == 128 ? ctx.reg_alloc.UseXmm(args[1]).getIdx() : ctx.reg_alloc.UseGpr(args[1]).getIdx();

    const auto wrapped_fn = write_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value_idx)];

    Xbyak::Label abort, end;
    bool require_abort_handling = false;

    if (fastmem_marker) {
        // Use fastmem
        const auto dest_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling);

        const auto location = code.getCurr();
        EmitWriteMemoryMov<bitsize>(code, dest_ptr, value_idx);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
                conf.recompile_on_fastmem_failure,
            });
    } else {
        // Use page table
        ASSERT(conf.page_table);
        const auto dest_ptr = EmitVAddrLookup(code, ctx, bitsize, abort, vaddr);
        require_abort_handling = true;
        EmitWriteMemoryMov<bitsize>(code, dest_ptr, value_idx);
    }
    code.L(end);

    if (require_abort_handling) {
        code.SwitchToFarCode();
        code.L(abort);
        code.call(wrapped_fn);
        code.jmp(end, code.T_NEAR);
        code.SwitchToNearCode();
    }
}

void A64EmitX64::EmitA64ReadMemory8(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<8, &A64::UserCallbacks::MemoryRead8>(ctx, inst);
}

void A64EmitX64::EmitA64ReadMemory16(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<16, &A64::UserCallbacks::MemoryRead16>(ctx, inst);
}

void A64EmitX64::EmitA64ReadMemory32(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<32, &A64::UserCallbacks::MemoryRead32>(ctx, inst);
}

void A64EmitX64::EmitA64ReadMemory64(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<64, &A64::UserCallbacks::MemoryRead64>(ctx, inst);
}

void A64EmitX64::EmitA64ReadMemory128(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryRead<128, &A64::UserCallbacks::MemoryRead128>(ctx, inst);
}

void A64EmitX64::EmitA64WriteMemory8(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<8, &A64::UserCallbacks::MemoryWrite8>(ctx, inst);
}

void A64EmitX64::EmitA64WriteMemory16(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<16, &A64::UserCallbacks::MemoryWrite16>(ctx, inst);
}

void A64EmitX64::EmitA64WriteMemory32(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<32, &A64::UserCallbacks::MemoryWrite32>(ctx, inst);
}

void A64EmitX64::EmitA64WriteMemory64(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<64, &A64::UserCallbacks::MemoryWrite64>(ctx, inst);
}

void A64EmitX64::EmitA64WriteMemory128(A64EmitContext& ctx, IR::Inst* inst) {
    EmitMemoryWrite<128, &A64::UserCallbacks::MemoryWrite64>(ctx, inst);
}

template<std::size_t bitsize, auto callback>
void A64EmitX64::EmitExclusiveReadMemory(A64EmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor != nullptr);
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if constexpr (bitsize != 128) {
        using T = mp::unsigned_integer_of_size<bitsize>;

        ctx.reg_alloc.HostCall(inst, {}, args[0]);

        code.mov(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(1));
        code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
        code.CallLambda(
            [](A64::UserConfig& conf, u64 vaddr) -> T {
                return conf.global_monitor->ReadAndMark<T>(conf.processor_id, vaddr, [&]() -> T {
                    return (conf.callbacks->*callback)(vaddr);
                });
            });
        code.ZeroExtendFrom(bitsize, code.ABI_RETURN);
    } else {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        ctx.reg_alloc.Use(args[0], ABI_PARAM2);
        ctx.reg_alloc.EndOfAllocScope();
        ctx.reg_alloc.HostCall(nullptr);

        code.mov(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(1));
        code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
        ctx.reg_alloc.AllocStackSpace(16 + ABI_SHADOW_SPACE);
        code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE]);
        code.CallLambda(
            [](A64::UserConfig& conf, u64 vaddr, A64::Vector& ret) {
                ret = conf.global_monitor->ReadAndMark<A64::Vector>(conf.processor_id, vaddr, [&]() -> A64::Vector {
                    return (conf.callbacks->*callback)(vaddr);
                });
            });
        code.movups(result, xword[rsp + ABI_SHADOW_SPACE]);
        ctx.reg_alloc.ReleaseStackSpace(16 + ABI_SHADOW_SPACE);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

template<std::size_t bitsize, auto callback>
void A64EmitX64::EmitExclusiveWriteMemory(A64EmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor != nullptr);
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if constexpr (bitsize != 128) {
        ctx.reg_alloc.HostCall(inst, {}, args[0], args[1]);
    } else {
        ctx.reg_alloc.Use(args[0], ABI_PARAM2);
        ctx.reg_alloc.Use(args[1], HostLoc::XMM1);
        ctx.reg_alloc.EndOfAllocScope();
        ctx.reg_alloc.HostCall(inst);
    }

    Xbyak::Label end;

    code.mov(code.ABI_RETURN, u32(1));
    code.cmp(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(0));
    code.je(end);
    code.mov(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(0));
    code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
    if constexpr (bitsize != 128) {
        using T = mp::unsigned_integer_of_size<bitsize>;

        code.CallLambda(
            [](A64::UserConfig& conf, u64 vaddr, T value) -> u32 {
                return conf.global_monitor->DoExclusiveOperation<T>(conf.processor_id, vaddr,
                                                                    [&](T expected) -> bool {
                                                                        return (conf.callbacks->*callback)(vaddr, value, expected);
                                                                    })
                         ? 0
                         : 1;
            });
    } else {
        ctx.reg_alloc.AllocStackSpace(16 + ABI_SHADOW_SPACE);
        code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE]);
        code.movaps(xword[code.ABI_PARAM3], xmm1);
        code.CallLambda(
            [](A64::UserConfig& conf, u64 vaddr, A64::Vector& value) -> u32 {
                return conf.global_monitor->DoExclusiveOperation<A64::Vector>(conf.processor_id, vaddr,
                                                                              [&](A64::Vector expected) -> bool {
                                                                                  return (conf.callbacks->*callback)(vaddr, value, expected);
                                                                              })
                         ? 0
                         : 1;
            });
        ctx.reg_alloc.ReleaseStackSpace(16 + ABI_SHADOW_SPACE);
    }
    code.L(end);
}

template<std::size_t bitsize, auto callback>
void A64EmitX64::EmitExclusiveReadMemoryInline(A64EmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor && conf.fastmem_pointer);
    if (!exception_handler.SupportsFastmem()) {
        EmitExclusiveReadMemory<bitsize, callback>(ctx, inst);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const int value_idx = bitsize == 128 ? ctx.reg_alloc.ScratchXmm().getIdx() : ctx.reg_alloc.ScratchGpr().getIdx();
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 tmp2 = ctx.reg_alloc.ScratchGpr();

    const auto wrapped_fn = read_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value_idx)];

    EmitExclusiveLock(code, conf, tmp, tmp2.cvt32());

    code.mov(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(1));
    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, conf.processor_id)));
    code.mov(qword[tmp], vaddr);

    const auto fastmem_marker = ShouldFastmem(ctx, inst);
    if (fastmem_marker) {
        Xbyak::Label abort, end;
        bool require_abort_handling = false;

        const auto src_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling);

        const auto location = code.getCurr();
        EmitReadMemoryMov<bitsize>(code, value_idx, src_ptr);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
                conf.recompile_on_exclusive_fastmem_failure,
            });

        code.L(end);

        if (require_abort_handling) {
            code.SwitchToFarCode();
            code.L(abort);
            code.call(wrapped_fn);
            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();
        }
    } else {
        code.call(wrapped_fn);
    }

    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorValuePointer(conf.global_monitor, conf.processor_id)));
    EmitWriteMemoryMov<bitsize>(code, tmp, value_idx);

    EmitExclusiveUnlock(code, conf, tmp, tmp2.cvt32());

    if constexpr (bitsize == 128) {
        ctx.reg_alloc.DefineValue(inst, Xbyak::Xmm{value_idx});
    } else {
        ctx.reg_alloc.DefineValue(inst, Xbyak::Reg64{value_idx});
    }
}

template<std::size_t bitsize, auto callback>
void A64EmitX64::EmitExclusiveWriteMemoryInline(A64EmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor && conf.fastmem_pointer);
    if (!exception_handler.SupportsFastmem()) {
        EmitExclusiveWriteMemory<bitsize, callback>(ctx, inst);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const auto value = [&] {
        if constexpr (bitsize == 128) {
            ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
            ctx.reg_alloc.ScratchGpr(HostLoc::RBX);
            ctx.reg_alloc.ScratchGpr(HostLoc::RCX);
            ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
            return ctx.reg_alloc.UseXmm(args[1]);
        } else {
            ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
            return ctx.reg_alloc.UseGpr(args[1]);
        }
    }();
    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg32 status = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();

    const auto fallback_fn = exclusive_write_fallbacks[std::make_tuple(bitsize, vaddr.getIdx(), value.getIdx())];

    EmitExclusiveLock(code, conf, tmp, eax);

    Xbyak::Label end;

    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, conf.processor_id)));
    code.mov(status, u32(1));
    code.cmp(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(0));
    code.je(end, code.T_NEAR);
    code.cmp(qword[tmp], vaddr);
    code.jne(end, code.T_NEAR);

    EmitExclusiveTestAndClear(code, conf, vaddr, tmp, rax);

    code.mov(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(0));
    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorValuePointer(conf.global_monitor, conf.processor_id)));

    if constexpr (bitsize == 128) {
        code.mov(rax, qword[tmp + 0]);
        code.mov(rdx, qword[tmp + 8]);
        if (code.HasHostFeature(HostFeature::SSE41)) {
            code.movq(rbx, value);
            code.pextrq(rcx, value, 1);
        } else {
            code.movaps(xmm0, value);
            code.movq(rbx, xmm0);
            code.punpckhqdq(xmm0, xmm0);
            code.movq(rcx, xmm0);
        }
    } else {
        EmitReadMemoryMov<bitsize>(code, rax.getIdx(), tmp);
    }

    const auto fastmem_marker = ShouldFastmem(ctx, inst);
    if (fastmem_marker) {
        Xbyak::Label abort;
        bool require_abort_handling = false;

        const auto dest_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling, tmp);

        const auto location = code.getCurr();

        if constexpr (bitsize == 128) {
            code.lock();
            code.cmpxchg16b(ptr[dest_ptr]);
        } else {
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
        }

        code.setnz(status.cvt8());

        code.SwitchToFarCode();
        code.L(abort);
        code.call(fallback_fn);

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

void A64EmitX64::EmitA64ClearExclusive(A64EmitContext&, IR::Inst*) {
    code.mov(code.byte[r15 + offsetof(A64JitState, exclusive_state)], u8(0));
}

void A64EmitX64::EmitA64ExclusiveReadMemory8(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<8, &A64::UserCallbacks::MemoryRead8>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<8, &A64::UserCallbacks::MemoryRead8>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveReadMemory16(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<16, &A64::UserCallbacks::MemoryRead16>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<16, &A64::UserCallbacks::MemoryRead16>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveReadMemory32(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<32, &A64::UserCallbacks::MemoryRead32>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<32, &A64::UserCallbacks::MemoryRead32>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveReadMemory64(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<64, &A64::UserCallbacks::MemoryRead64>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<64, &A64::UserCallbacks::MemoryRead64>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveReadMemory128(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<128, &A64::UserCallbacks::MemoryRead128>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<128, &A64::UserCallbacks::MemoryRead128>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveWriteMemory8(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<8, &A64::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<8, &A64::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveWriteMemory16(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<16, &A64::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<16, &A64::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveWriteMemory32(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<32, &A64::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<32, &A64::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveWriteMemory64(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<64, &A64::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<64, &A64::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
    }
}

void A64EmitX64::EmitA64ExclusiveWriteMemory128(A64EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<128, &A64::UserCallbacks::MemoryWriteExclusive128>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<128, &A64::UserCallbacks::MemoryWriteExclusive128>(ctx, inst);
    }
}

}  // namespace Dynarmic::Backend::X64
