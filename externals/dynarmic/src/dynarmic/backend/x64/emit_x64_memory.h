/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mcl/bit_cast.hpp>
#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/a32_emit_x64.h"
#include "dynarmic/backend/x64/a64_emit_x64.h"
#include "dynarmic/backend/x64/exclusive_monitor_friend.h"
#include "dynarmic/common/spin_lock_x64.h"
#include "dynarmic/interface/exclusive_monitor.h"
#include "dynarmic/ir/acc_type.h"

namespace Dynarmic::Backend::X64 {

namespace {

using namespace Xbyak::util;

constexpr size_t page_bits = 12;
constexpr size_t page_size = 1 << page_bits;
constexpr size_t page_mask = (1 << page_bits) - 1;

template<typename EmitContext>
void EmitDetectMisalignedVAddr(BlockOfCode& code, EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr, Xbyak::Reg64 tmp) {
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
        default:
            UNREACHABLE();
        }
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

template<typename EmitContext>
Xbyak::RegExp EmitVAddrLookup(BlockOfCode& code, EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr);

template<>
[[maybe_unused]] Xbyak::RegExp EmitVAddrLookup<A32EmitContext>(BlockOfCode& code, A32EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr) {
    const Xbyak::Reg64 page = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg32 tmp = ctx.conf.absolute_offset_page_table ? page.cvt32() : ctx.reg_alloc.ScratchGpr().cvt32();

    EmitDetectMisalignedVAddr(code, ctx, bitsize, abort, vaddr, tmp.cvt64());

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

template<>
[[maybe_unused]] Xbyak::RegExp EmitVAddrLookup<A64EmitContext>(BlockOfCode& code, A64EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr) {
    const size_t valid_page_index_bits = ctx.conf.page_table_address_space_bits - page_bits;
    const size_t unused_top_bits = 64 - ctx.conf.page_table_address_space_bits;

    const Xbyak::Reg64 page = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 tmp = ctx.conf.absolute_offset_page_table ? page : ctx.reg_alloc.ScratchGpr();

    EmitDetectMisalignedVAddr(code, ctx, bitsize, abort, vaddr, tmp);

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

template<typename EmitContext>
Xbyak::RegExp EmitFastmemVAddr(BlockOfCode& code, EmitContext& ctx, Xbyak::Label& abort, Xbyak::Reg64 vaddr, bool& require_abort_handling, std::optional<Xbyak::Reg64> tmp = std::nullopt);

template<>
[[maybe_unused]] Xbyak::RegExp EmitFastmemVAddr<A32EmitContext>(BlockOfCode&, A32EmitContext&, Xbyak::Label&, Xbyak::Reg64 vaddr, bool&, std::optional<Xbyak::Reg64>) {
    return r13 + vaddr;
}

template<>
[[maybe_unused]] Xbyak::RegExp EmitFastmemVAddr<A64EmitContext>(BlockOfCode& code, A64EmitContext& ctx, Xbyak::Label& abort, Xbyak::Reg64 vaddr, bool& require_abort_handling, std::optional<Xbyak::Reg64> tmp) {
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
const void* EmitReadMemoryMov(BlockOfCode& code, int value_idx, const Xbyak::RegExp& addr, bool ordered) {
    if (ordered) {
        if constexpr (bitsize != 128) {
            code.xor_(Xbyak::Reg32{value_idx}, Xbyak::Reg32{value_idx});
        } else {
            code.xor_(eax, eax);
            code.xor_(ebx, ebx);
            code.xor_(ecx, ecx);
            code.xor_(edx, edx);
        }

        const void* fastmem_location = code.getCurr();
        switch (bitsize) {
        case 8:
            code.lock();
            code.xadd(code.byte[addr], Xbyak::Reg32{value_idx}.cvt8());
            break;
        case 16:
            code.lock();
            code.xadd(word[addr], Xbyak::Reg16{value_idx});
            break;
        case 32:
            code.lock();
            code.xadd(dword[addr], Xbyak::Reg32{value_idx});
            break;
        case 64:
            code.lock();
            code.xadd(qword[addr], Xbyak::Reg64{value_idx});
            break;
        case 128:
            code.lock();
            code.cmpxchg16b(xword[addr]);
            if (code.HasHostFeature(HostFeature::SSE41)) {
                code.movq(Xbyak::Xmm{value_idx}, rax);
                code.pinsrq(Xbyak::Xmm{value_idx}, rdx, 1);
            } else {
                code.movq(Xbyak::Xmm{value_idx}, rax);
                code.movq(xmm0, rdx);
                code.punpcklqdq(Xbyak::Xmm{value_idx}, xmm0);
            }
            break;
        default:
            ASSERT_FALSE("Invalid bitsize");
        }
        return fastmem_location;
    }

    const void* fastmem_location = code.getCurr();
    switch (bitsize) {
    case 8:
        code.movzx(Xbyak::Reg32{value_idx}, code.byte[addr]);
        break;
    case 16:
        code.movzx(Xbyak::Reg32{value_idx}, word[addr]);
        break;
    case 32:
        code.mov(Xbyak::Reg32{value_idx}, dword[addr]);
        break;
    case 64:
        code.mov(Xbyak::Reg64{value_idx}, qword[addr]);
        break;
    case 128:
        code.movups(Xbyak::Xmm{value_idx}, xword[addr]);
        break;
    default:
        ASSERT_FALSE("Invalid bitsize");
    }
    return fastmem_location;
}

template<std::size_t bitsize>
const void* EmitWriteMemoryMov(BlockOfCode& code, const Xbyak::RegExp& addr, int value_idx, bool ordered) {
    if (ordered) {
        if constexpr (bitsize == 128) {
            code.xor_(eax, eax);
            code.xor_(edx, edx);
            if (code.HasHostFeature(HostFeature::SSE41)) {
                code.movq(rbx, Xbyak::Xmm{value_idx});
                code.pextrq(rcx, Xbyak::Xmm{value_idx}, 1);
            } else {
                code.movaps(xmm0, Xbyak::Xmm{value_idx});
                code.movq(rbx, xmm0);
                code.punpckhqdq(xmm0, xmm0);
                code.movq(rcx, xmm0);
            }
        }

        const void* fastmem_location = code.getCurr();
        switch (bitsize) {
        case 8:
            code.xchg(code.byte[addr], Xbyak::Reg64{value_idx}.cvt8());
            break;
        case 16:
            code.xchg(word[addr], Xbyak::Reg16{value_idx});
            break;
        case 32:
            code.xchg(dword[addr], Xbyak::Reg32{value_idx});
            break;
        case 64:
            code.xchg(qword[addr], Xbyak::Reg64{value_idx});
            break;
        case 128: {
            Xbyak::Label loop;
            code.L(loop);
            code.lock();
            code.cmpxchg16b(xword[addr]);
            code.jnz(loop);
            break;
        }
        default:
            ASSERT_FALSE("Invalid bitsize");
        }
        return fastmem_location;
    }

    const void* fastmem_location = code.getCurr();
    switch (bitsize) {
    case 8:
        code.mov(code.byte[addr], Xbyak::Reg64{value_idx}.cvt8());
        break;
    case 16:
        code.mov(word[addr], Xbyak::Reg16{value_idx});
        break;
    case 32:
        code.mov(dword[addr], Xbyak::Reg32{value_idx});
        break;
    case 64:
        code.mov(qword[addr], Xbyak::Reg64{value_idx});
        break;
    case 128:
        code.movups(xword[addr], Xbyak::Xmm{value_idx});
        break;
    default:
        ASSERT_FALSE("Invalid bitsize");
    }
    return fastmem_location;
}

template<typename UserConfig>
void EmitExclusiveLock(BlockOfCode& code, const UserConfig& conf, Xbyak::Reg64 pointer, Xbyak::Reg32 tmp) {
    if (conf.HasOptimization(OptimizationFlag::Unsafe_IgnoreGlobalMonitor)) {
        return;
    }

    code.mov(pointer, mcl::bit_cast<u64>(GetExclusiveMonitorLockPointer(conf.global_monitor)));
    EmitSpinLockLock(code, pointer, tmp);
}

template<typename UserConfig>
void EmitExclusiveUnlock(BlockOfCode& code, const UserConfig& conf, Xbyak::Reg64 pointer, Xbyak::Reg32 tmp) {
    if (conf.HasOptimization(OptimizationFlag::Unsafe_IgnoreGlobalMonitor)) {
        return;
    }

    code.mov(pointer, mcl::bit_cast<u64>(GetExclusiveMonitorLockPointer(conf.global_monitor)));
    EmitSpinLockUnlock(code, pointer, tmp);
}

template<typename UserConfig>
void EmitExclusiveTestAndClear(BlockOfCode& code, const UserConfig& conf, Xbyak::Reg64 vaddr, Xbyak::Reg64 pointer, Xbyak::Reg64 tmp) {
    if (conf.HasOptimization(OptimizationFlag::Unsafe_IgnoreGlobalMonitor)) {
        return;
    }

    code.mov(tmp, 0xDEAD'DEAD'DEAD'DEAD);
    const size_t processor_count = GetExclusiveMonitorProcessorCount(conf.global_monitor);
    for (size_t processor_index = 0; processor_index < processor_count; processor_index++) {
        if (processor_index == conf.processor_id) {
            continue;
        }
        Xbyak::Label ok;
        code.mov(pointer, mcl::bit_cast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, processor_index)));
        code.cmp(qword[pointer], vaddr);
        code.jne(ok);
        code.mov(qword[pointer], tmp);
        code.L(ok);
    }
}

inline bool IsOrdered(IR::AccType acctype) {
    return acctype == IR::AccType::ORDERED || acctype == IR::AccType::ORDEREDRW || acctype == IR::AccType::LIMITEDORDERED;
}

}  // namespace

}  // namespace Dynarmic::Backend::X64
