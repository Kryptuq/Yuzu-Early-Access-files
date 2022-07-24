/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>
#include <climits>

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/crypto/crc32.h"
#include "dynarmic/ir/microinstruction.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;
namespace CRC32 = Common::Crypto::CRC32;

static void EmitCRC32Castagnoli(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, const int data_size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::SSE42)) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg value = ctx.reg_alloc.UseGpr(args[1]).changeBit(data_size);
        code.crc32(crc, value);
        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1], {});
    code.mov(code.ABI_PARAM3, data_size / CHAR_BIT);
    code.CallFunction(&CRC32::ComputeCRC32Castagnoli);
}

static void EmitCRC32ISO(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, const int data_size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::PCLMULQDQ) && data_size < 32) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg64 value = ctx.reg_alloc.UseScratchGpr(args[1]);
        const Xbyak::Xmm xmm_value = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_const = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_tmp = ctx.reg_alloc.ScratchXmm();

        code.movdqa(xmm_const, code.XmmConst(xword, 0xb4e5b025'f7011641, 0x00000001'DB710641));

        code.movzx(value.cvt32(), value.changeBit(data_size));
        code.xor_(value.cvt32(), crc);
        code.movd(xmm_tmp, value.cvt32());
        code.pslldq(xmm_tmp, (64 - data_size) / 8);

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpclmulqdq(xmm_value, xmm_tmp, xmm_const, 0x00);
            code.pclmulqdq(xmm_value, xmm_const, 0x10);
            code.pxor(xmm_value, xmm_tmp);
        } else {
            code.movdqa(xmm_value, xmm_tmp);
            code.pclmulqdq(xmm_value, xmm_const, 0x00);
            code.pclmulqdq(xmm_value, xmm_const, 0x10);
            code.pxor(xmm_value, xmm_tmp);
        }

        code.pextrd(crc, xmm_value, 2);

        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    if (code.HasHostFeature(HostFeature::PCLMULQDQ) && data_size == 32) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 value = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        const Xbyak::Xmm xmm_value = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_const = ctx.reg_alloc.ScratchXmm();

        code.movdqa(xmm_const, code.XmmConst(xword, 0xb4e5b025'f7011641, 0x00000001'DB710641));

        code.xor_(crc, value);
        code.shl(crc.cvt64(), 32);
        code.movq(xmm_value, crc.cvt64());

        code.pclmulqdq(xmm_value, xmm_const, 0x00);
        code.pclmulqdq(xmm_value, xmm_const, 0x10);

        code.pextrd(crc, xmm_value, 2);

        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    if (code.HasHostFeature(HostFeature::PCLMULQDQ) && data_size == 64) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg64 value = ctx.reg_alloc.UseGpr(args[1]);
        const Xbyak::Xmm xmm_value = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_const = ctx.reg_alloc.ScratchXmm();

        code.movdqa(xmm_const, code.XmmConst(xword, 0xb4e5b025'f7011641, 0x00000001'DB710641));

        code.mov(crc, crc);
        code.xor_(crc.cvt64(), value);
        code.movq(xmm_value, crc.cvt64());

        code.pclmulqdq(xmm_value, xmm_const, 0x00);
        code.pclmulqdq(xmm_value, xmm_const, 0x10);

        code.pextrd(crc, xmm_value, 2);

        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1], {});
    code.mov(code.ABI_PARAM3, data_size / CHAR_BIT);
    code.CallFunction(&CRC32::ComputeCRC32ISO);
}

void EmitX64::EmitCRC32Castagnoli8(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 8);
}

void EmitX64::EmitCRC32Castagnoli16(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 16);
}

void EmitX64::EmitCRC32Castagnoli32(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 32);
}

void EmitX64::EmitCRC32Castagnoli64(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 64);
}

void EmitX64::EmitCRC32ISO8(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 8);
}

void EmitX64::EmitCRC32ISO16(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 16);
}

void EmitX64::EmitCRC32ISO32(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 32);
}

void EmitX64::EmitCRC32ISO64(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 64);
}

}  // namespace Dynarmic::Backend::X64
