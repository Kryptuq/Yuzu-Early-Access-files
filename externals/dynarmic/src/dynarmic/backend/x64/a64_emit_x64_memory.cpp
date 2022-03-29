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

    for (bool ordered : {false, true}) {
        for (int vaddr_idx : idxes) {
            if (vaddr_idx == 4 || vaddr_idx == 15) {
                continue;
            }

            for (int value_idx : idxes) {
                code.align();
                read_fallbacks[std::make_tuple(ordered, 128, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
                ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(value_idx));
                if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                    code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
                }
                if (ordered) {
                    code.mfence();
                }
                code.call(memory_read_128);
                if (value_idx != 1) {
                    code.movaps(Xbyak::Xmm{value_idx}, xmm1);
                }
                ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(value_idx));
                code.ret();
                PerfMapRegister(read_fallbacks[std::make_tuple(ordered, 128, vaddr_idx, value_idx)], code.getCurr(), "a64_read_fallback_128");

                code.align();
                write_fallbacks[std::make_tuple(ordered, 128, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
                ABI_PushCallerSaveRegistersAndAdjustStack(code);
                if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                    code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
                }
                if (value_idx != 1) {
                    code.movaps(xmm1, Xbyak::Xmm{value_idx});
                }
                code.call(memory_write_128);
                if (ordered) {
                    code.mfence();
                }
                ABI_PopCallerSaveRegistersAndAdjustStack(code);
                code.ret();
                PerfMapRegister(write_fallbacks[std::make_tuple(ordered, 128, vaddr_idx, value_idx)], code.getCurr(), "a64_write_fallback_128");

                code.align();
                exclusive_write_fallbacks[std::make_tuple(ordered, 128, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
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
                PerfMapRegister(exclusive_write_fallbacks[std::make_tuple(ordered, 128, vaddr_idx, value_idx)], code.getCurr(), "a64_exclusive_write_fallback_128");

                if (value_idx == 4 || value_idx == 15) {
                    continue;
                }

                for (const auto& [bitsize, callback] : read_callbacks) {
                    code.align();
                    read_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
                    ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocRegIdx(value_idx));
                    if (vaddr_idx != code.ABI_PARAM2.getIdx()) {
                        code.mov(code.ABI_PARAM2, Xbyak::Reg64{vaddr_idx});
                    }
                    if (ordered) {
                        code.mfence();
                    }
                    callback.EmitCall(code);
                    if (value_idx != code.ABI_RETURN.getIdx()) {
                        code.mov(Xbyak::Reg64{value_idx}, code.ABI_RETURN);
                    }
                    ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocRegIdx(value_idx));
                    code.ZeroExtendFrom(bitsize, Xbyak::Reg64{value_idx});
                    code.ret();
                    PerfMapRegister(read_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a64_read_fallback_{}", bitsize));
                }

                for (const auto& [bitsize, callback] : write_callbacks) {
                    code.align();
                    write_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
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
                    code.ZeroExtendFrom(bitsize, code.ABI_PARAM3);
                    callback.EmitCall(code);
                    if (ordered) {
                        code.mfence();
                    }
                    ABI_PopCallerSaveRegistersAndAdjustStack(code);
                    code.ret();
                    PerfMapRegister(write_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a64_write_fallback_{}", bitsize));
                }

                for (const auto& [bitsize, callback] : exclusive_write_callbacks) {
                    code.align();
                    exclusive_write_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)] = code.getCurr<void (*)()>();
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
                    code.ZeroExtendFrom(bitsize, code.ABI_PARAM3);
                    code.mov(code.ABI_PARAM4, rax);
                    code.ZeroExtendFrom(bitsize, code.ABI_PARAM4);
                    callback.EmitCall(code);
                    ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLoc::RAX);
                    code.ret();
                    PerfMapRegister(exclusive_write_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a64_exclusive_write_fallback_{}", bitsize));
                }
            }
        }
    }
}

#define Axx A64
#include "emit_x64_memory.cpp.inc"
#undef Axx

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
