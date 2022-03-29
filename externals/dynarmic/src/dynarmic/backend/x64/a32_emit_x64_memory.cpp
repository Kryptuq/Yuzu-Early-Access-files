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

    for (bool ordered : {false, true}) {
        for (int vaddr_idx : idxes) {
            for (int value_idx : idxes) {
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
                    PerfMapRegister(read_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a32_read_fallback_{}", bitsize));
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
                    PerfMapRegister(write_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a32_write_fallback_{}", bitsize));
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
                    PerfMapRegister(exclusive_write_fallbacks[std::make_tuple(ordered, bitsize, vaddr_idx, value_idx)], code.getCurr(), fmt::format("a32_exclusive_write_fallback_{}", bitsize));
                }
            }
        }
    }
}

#define Axx A32
#include "emit_x64_memory.cpp.inc"
#undef Axx

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

void A32EmitX64::EmitA32ClearExclusive(A32EmitContext&, IR::Inst*) {
    code.mov(code.byte[r15 + offsetof(A32JitState, exclusive_state)], u8(0));
}

void A32EmitX64::EmitA32ExclusiveReadMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<8, &A32::UserCallbacks::MemoryRead8>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<8, &A32::UserCallbacks::MemoryRead8>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveReadMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<16, &A32::UserCallbacks::MemoryRead16>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<16, &A32::UserCallbacks::MemoryRead16>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveReadMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<32, &A32::UserCallbacks::MemoryRead32>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<32, &A32::UserCallbacks::MemoryRead32>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveReadMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveReadMemoryInline<64, &A32::UserCallbacks::MemoryRead64>(ctx, inst);
    } else {
        EmitExclusiveReadMemory<64, &A32::UserCallbacks::MemoryRead64>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory8(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<8, &A32::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<8, &A32::UserCallbacks::MemoryWriteExclusive8>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory16(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<16, &A32::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<16, &A32::UserCallbacks::MemoryWriteExclusive16>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory32(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<32, &A32::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<32, &A32::UserCallbacks::MemoryWriteExclusive32>(ctx, inst);
    }
}

void A32EmitX64::EmitA32ExclusiveWriteMemory64(A32EmitContext& ctx, IR::Inst* inst) {
    if (conf.fastmem_exclusive_access) {
        EmitExclusiveWriteMemoryInline<64, &A32::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
    } else {
        EmitExclusiveWriteMemory<64, &A32::UserCallbacks::MemoryWriteExclusive64>(ctx, inst);
    }
}

}  // namespace Dynarmic::Backend::X64
