/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/abi.h"

#include <algorithm>
#include <vector>

#include <mcl/iterator/reverse.hpp>
#include <mcl/stdint.hpp>
#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/block_of_code.h"

namespace Dynarmic::Backend::X64 {

constexpr size_t XMM_SIZE = 16;

struct FrameInfo {
    size_t stack_subtraction;
    size_t xmm_offset;
    size_t frame_offset;
};

static FrameInfo CalculateFrameInfo(size_t num_gprs, size_t num_xmms, size_t frame_size) {
    // We are initially 8 byte aligned because the return value is pushed onto an aligned stack after a call.
    const size_t rsp_alignment = (num_gprs % 2 == 0) ? 8 : 0;
    const size_t total_xmm_size = num_xmms * XMM_SIZE;

    if (frame_size & 0xF) {
        frame_size += 0x10 - (frame_size & 0xF);
    }

    return {
        rsp_alignment + total_xmm_size + frame_size + ABI_SHADOW_SPACE,
        frame_size + ABI_SHADOW_SPACE,
        ABI_SHADOW_SPACE,
    };
}

template<typename RegisterArrayT>
void ABI_PushRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size, const RegisterArrayT& regs) {
    using namespace Xbyak::util;

    const size_t num_gprs = std::count_if(regs.begin(), regs.end(), HostLocIsGPR);
    const size_t num_xmms = std::count_if(regs.begin(), regs.end(), HostLocIsXMM);

    FrameInfo frame_info = CalculateFrameInfo(num_gprs, num_xmms, frame_size);

    for (HostLoc gpr : regs) {
        if (HostLocIsGPR(gpr)) {
            code.push(HostLocToReg64(gpr));
        }
    }

    if (frame_info.stack_subtraction != 0) {
        code.sub(rsp, u32(frame_info.stack_subtraction));
    }

    size_t xmm_offset = frame_info.xmm_offset;
    for (HostLoc xmm : regs) {
        if (HostLocIsXMM(xmm)) {
            if (code.HasHostFeature(HostFeature::AVX)) {
                code.vmovaps(code.xword[rsp + xmm_offset], HostLocToXmm(xmm));
            } else {
                code.movaps(code.xword[rsp + xmm_offset], HostLocToXmm(xmm));
            }
            xmm_offset += XMM_SIZE;
        }
    }
}

template<typename RegisterArrayT>
void ABI_PopRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size, const RegisterArrayT& regs) {
    using namespace Xbyak::util;

    const size_t num_gprs = std::count_if(regs.begin(), regs.end(), HostLocIsGPR);
    const size_t num_xmms = std::count_if(regs.begin(), regs.end(), HostLocIsXMM);

    FrameInfo frame_info = CalculateFrameInfo(num_gprs, num_xmms, frame_size);

    size_t xmm_offset = frame_info.xmm_offset;
    for (HostLoc xmm : regs) {
        if (HostLocIsXMM(xmm)) {
            if (code.HasHostFeature(HostFeature::AVX)) {
                code.vmovaps(HostLocToXmm(xmm), code.xword[rsp + xmm_offset]);
            } else {
                code.movaps(HostLocToXmm(xmm), code.xword[rsp + xmm_offset]);
            }
            xmm_offset += XMM_SIZE;
        }
    }

    if (frame_info.stack_subtraction != 0) {
        code.add(rsp, u32(frame_info.stack_subtraction));
    }

    for (HostLoc gpr : mcl::iterator::reverse(regs)) {
        if (HostLocIsGPR(gpr)) {
            code.pop(HostLocToReg64(gpr));
        }
    }
}

void ABI_PushCalleeSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size) {
    ABI_PushRegistersAndAdjustStack(code, frame_size, ABI_ALL_CALLEE_SAVE);
}

void ABI_PopCalleeSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size) {
    ABI_PopRegistersAndAdjustStack(code, frame_size, ABI_ALL_CALLEE_SAVE);
}

void ABI_PushCallerSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size) {
    ABI_PushRegistersAndAdjustStack(code, frame_size, ABI_ALL_CALLER_SAVE);
}

void ABI_PopCallerSaveRegistersAndAdjustStack(BlockOfCode& code, size_t frame_size) {
    ABI_PopRegistersAndAdjustStack(code, frame_size, ABI_ALL_CALLER_SAVE);
}

void ABI_PushCallerSaveRegistersAndAdjustStackExcept(BlockOfCode& code, HostLoc exception) {
    std::vector<HostLoc> regs;
    std::remove_copy(ABI_ALL_CALLER_SAVE.begin(), ABI_ALL_CALLER_SAVE.end(), std::back_inserter(regs), exception);
    ABI_PushRegistersAndAdjustStack(code, 0, regs);
}

void ABI_PopCallerSaveRegistersAndAdjustStackExcept(BlockOfCode& code, HostLoc exception) {
    std::vector<HostLoc> regs;
    std::remove_copy(ABI_ALL_CALLER_SAVE.begin(), ABI_ALL_CALLER_SAVE.end(), std::back_inserter(regs), exception);
    ABI_PopRegistersAndAdjustStack(code, 0, regs);
}

}  // namespace Dynarmic::Backend::X64
