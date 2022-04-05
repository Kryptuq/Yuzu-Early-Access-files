/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/macro_util.h"

#define AxxEmitX64 CONCATENATE_TOKENS(Axx, EmitX64)
#define AxxEmitContext CONCATENATE_TOKENS(Axx, EmitContext)
#define AxxJitState CONCATENATE_TOKENS(Axx, JitState)
#define AxxUserConfig Axx::UserConfig

namespace {
using Vector = std::array<u64, 2>;
}

std::optional<AxxEmitX64::DoNotFastmemMarker> AxxEmitX64::ShouldFastmem(AxxEmitContext& ctx, IR::Inst* inst) const {
    if (!conf.fastmem_pointer || !exception_handler.SupportsFastmem()) {
        return std::nullopt;
    }

    const auto marker = std::make_tuple(ctx.Location(), ctx.GetInstOffset(inst));
    if (do_not_fastmem.count(marker) > 0) {
        return std::nullopt;
    }
    return marker;
}

FakeCall AxxEmitX64::FastmemCallback(u64 rip_) {
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

template<std::size_t bitsize, auto callback>
void AxxEmitX64::EmitMemoryRead(AxxEmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool ordered = IsOrdered(args[1].GetImmediateAccType());
    const auto fastmem_marker = ShouldFastmem(ctx, inst);

    if (!conf.page_table && !fastmem_marker) {
        // Neither fastmem nor page table: Use callbacks
        if constexpr (bitsize == 128) {
            ctx.reg_alloc.HostCall(nullptr, {}, args[0]);
            if (ordered) {
                code.mfence();
            }
            code.CallFunction(memory_read_128);
            ctx.reg_alloc.DefineValue(inst, xmm1);
        } else {
            ctx.reg_alloc.HostCall(inst, {}, args[0]);
            if (ordered) {
                code.mfence();
            }
            Devirtualize<callback>(conf.callbacks).EmitCall(code);
            code.ZeroExtendFrom(bitsize, code.ABI_RETURN);
        }
        return;
    }

    if (ordered && bitsize == 128) {
        // Required for atomic 128-bit loads/stores
        ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RBX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RCX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const int value_idx = bitsize == 128 ? ctx.reg_alloc.ScratchXmm().getIdx() : ctx.reg_alloc.ScratchGpr().getIdx();

    const auto wrapped_fn = read_fallbacks[std::make_tuple(ordered, bitsize, vaddr.getIdx(), value_idx)];

    Xbyak::Label abort, end;
    bool require_abort_handling = false;

    if (fastmem_marker) {
        // Use fastmem
        const auto src_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling);

        const auto location = EmitReadMemoryMov<bitsize>(code, value_idx, src_ptr, ordered);

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
        EmitReadMemoryMov<bitsize>(code, value_idx, src_ptr, ordered);
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
void AxxEmitX64::EmitMemoryWrite(AxxEmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool ordered = IsOrdered(args[2].GetImmediateAccType());
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
        if (ordered) {
            code.mfence();
        }
        return;
    }

    if (ordered && bitsize == 128) {
        // Required for atomic 128-bit loads/stores
        ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RBX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RCX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const int value_idx = bitsize == 128
                            ? ctx.reg_alloc.UseXmm(args[1]).getIdx()
                            : (ordered ? ctx.reg_alloc.UseScratchGpr(args[1]).getIdx() : ctx.reg_alloc.UseGpr(args[1]).getIdx());

    const auto wrapped_fn = write_fallbacks[std::make_tuple(ordered, bitsize, vaddr.getIdx(), value_idx)];

    Xbyak::Label abort, end;
    bool require_abort_handling = false;

    if (fastmem_marker) {
        // Use fastmem
        const auto dest_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling);

        const auto location = EmitWriteMemoryMov<bitsize>(code, dest_ptr, value_idx, ordered);

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
        EmitWriteMemoryMov<bitsize>(code, dest_ptr, value_idx, ordered);
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

template<std::size_t bitsize, auto callback>
void AxxEmitX64::EmitExclusiveReadMemory(AxxEmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor != nullptr);
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool ordered = IsOrdered(args[1].GetImmediateAccType());

    if constexpr (bitsize != 128) {
        using T = mp::unsigned_integer_of_size<bitsize>;

        ctx.reg_alloc.HostCall(inst, {}, args[0]);

        code.mov(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(1));
        code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
        if (ordered) {
            code.mfence();
        }
        code.CallLambda(
            [](AxxUserConfig& conf, Axx::VAddr vaddr) -> T {
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

        code.mov(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(1));
        code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
        ctx.reg_alloc.AllocStackSpace(16 + ABI_SHADOW_SPACE);
        code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE]);
        if (ordered) {
            code.mfence();
        }
        code.CallLambda(
            [](AxxUserConfig& conf, Axx::VAddr vaddr, Vector& ret) {
                ret = conf.global_monitor->ReadAndMark<Vector>(conf.processor_id, vaddr, [&]() -> Vector {
                    return (conf.callbacks->*callback)(vaddr);
                });
            });
        code.movups(result, xword[rsp + ABI_SHADOW_SPACE]);
        ctx.reg_alloc.ReleaseStackSpace(16 + ABI_SHADOW_SPACE);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

template<std::size_t bitsize, auto callback>
void AxxEmitX64::EmitExclusiveWriteMemory(AxxEmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor != nullptr);
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool ordered = IsOrdered(args[2].GetImmediateAccType());

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
    code.cmp(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(0));
    code.je(end);
    code.mov(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(0));
    code.mov(code.ABI_PARAM1, reinterpret_cast<u64>(&conf));
    if constexpr (bitsize != 128) {
        using T = mp::unsigned_integer_of_size<bitsize>;

        code.CallLambda(
            [](AxxUserConfig& conf, Axx::VAddr vaddr, T value) -> u32 {
                return conf.global_monitor->DoExclusiveOperation<T>(conf.processor_id, vaddr,
                                                                    [&](T expected) -> bool {
                                                                        return (conf.callbacks->*callback)(vaddr, value, expected);
                                                                    })
                         ? 0
                         : 1;
            });
        if (ordered) {
            code.mfence();
        }
    } else {
        ctx.reg_alloc.AllocStackSpace(16 + ABI_SHADOW_SPACE);
        code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE]);
        code.movaps(xword[code.ABI_PARAM3], xmm1);
        code.CallLambda(
            [](AxxUserConfig& conf, Axx::VAddr vaddr, Vector& value) -> u32 {
                return conf.global_monitor->DoExclusiveOperation<Vector>(conf.processor_id, vaddr,
                                                                         [&](Vector expected) -> bool {
                                                                             return (conf.callbacks->*callback)(vaddr, value, expected);
                                                                         })
                         ? 0
                         : 1;
            });
        if (ordered) {
            code.mfence();
        }
        ctx.reg_alloc.ReleaseStackSpace(16 + ABI_SHADOW_SPACE);
    }
    code.L(end);
}

template<std::size_t bitsize, auto callback>
void AxxEmitX64::EmitExclusiveReadMemoryInline(AxxEmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor && conf.fastmem_pointer);
    if (!exception_handler.SupportsFastmem()) {
        EmitExclusiveReadMemory<bitsize, callback>(ctx, inst);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    constexpr bool ordered = true;

    if constexpr (ordered && bitsize == 128) {
        // Required for atomic 128-bit loads/stores
        ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RBX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RCX);
        ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    }

    const Xbyak::Reg64 vaddr = ctx.reg_alloc.UseGpr(args[0]);
    const int value_idx = bitsize == 128 ? ctx.reg_alloc.ScratchXmm().getIdx() : ctx.reg_alloc.ScratchGpr().getIdx();
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 tmp2 = ctx.reg_alloc.ScratchGpr();

    const auto wrapped_fn = read_fallbacks[std::make_tuple(ordered, bitsize, vaddr.getIdx(), value_idx)];

    EmitExclusiveLock(code, conf, tmp, tmp2.cvt32());

    code.mov(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(1));
    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, conf.processor_id)));
    code.mov(qword[tmp], vaddr);

    const auto fastmem_marker = ShouldFastmem(ctx, inst);
    if (fastmem_marker) {
        Xbyak::Label abort, end;
        bool require_abort_handling = false;

        const auto src_ptr = EmitFastmemVAddr(code, ctx, abort, vaddr, require_abort_handling);

        const auto location = EmitReadMemoryMov<bitsize>(code, value_idx, src_ptr, ordered);

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
    EmitWriteMemoryMov<bitsize>(code, tmp, value_idx, false);

    EmitExclusiveUnlock(code, conf, tmp, tmp2.cvt32());

    if constexpr (bitsize == 128) {
        ctx.reg_alloc.DefineValue(inst, Xbyak::Xmm{value_idx});
    } else {
        ctx.reg_alloc.DefineValue(inst, Xbyak::Reg64{value_idx});
    }
}

template<std::size_t bitsize, auto callback>
void AxxEmitX64::EmitExclusiveWriteMemoryInline(AxxEmitContext& ctx, IR::Inst* inst) {
    ASSERT(conf.global_monitor && conf.fastmem_pointer);
    if (!exception_handler.SupportsFastmem()) {
        EmitExclusiveWriteMemory<bitsize, callback>(ctx, inst);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    constexpr bool ordered = true;

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

    const auto wrapped_fn = exclusive_write_fallbacks[std::make_tuple(ordered, bitsize, vaddr.getIdx(), value.getIdx())];

    EmitExclusiveLock(code, conf, tmp, eax);

    Xbyak::Label end;

    code.mov(tmp, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, conf.processor_id)));
    code.mov(status, u32(1));
    code.cmp(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(0));
    code.je(end, code.T_NEAR);
    code.cmp(qword[tmp], vaddr);
    code.jne(end, code.T_NEAR);

    EmitExclusiveTestAndClear(code, conf, vaddr, tmp, rax);

    code.mov(code.byte[r15 + offsetof(AxxJitState, exclusive_state)], u8(0));
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
        EmitReadMemoryMov<bitsize>(code, rax.getIdx(), tmp, false);
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
        code.call(wrapped_fn);

        fastmem_patch_info.emplace(
            Common::BitCast<u64>(location),
            FastmemPatchInfo{
                Common::BitCast<u64>(code.getCurr()),
                Common::BitCast<u64>(wrapped_fn),
                *fastmem_marker,
                conf.recompile_on_exclusive_fastmem_failure,
            });

        code.cmp(al, 0);
        code.setz(status.cvt8());
        code.movzx(status.cvt32(), status.cvt8());
        code.jmp(end, code.T_NEAR);
        code.SwitchToNearCode();
    } else {
        code.call(wrapped_fn);
        code.cmp(al, 0);
        code.setz(status.cvt8());
        code.movzx(status.cvt32(), status.cvt8());
    }

    code.L(end);

    EmitExclusiveUnlock(code, conf, tmp, eax);

    ctx.reg_alloc.DefineValue(inst, status);
}

#undef AxxEmitX64
#undef AxxEmitContext
#undef AxxJitState
#undef AxxUserConfig
