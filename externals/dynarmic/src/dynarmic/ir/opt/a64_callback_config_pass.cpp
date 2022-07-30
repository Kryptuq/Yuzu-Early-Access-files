/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/a64_ir_emitter.h"
#include "dynarmic/interface/A64/config.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Optimization {

void A64CallbackConfigPass(IR::Block& block, const A64::UserConfig& conf) {
    if (conf.hook_data_cache_operations) {
        return;
    }

    for (auto& inst : block) {
        if (inst.GetOpcode() != IR::Opcode::A64DataCacheOperationRaised) {
            continue;
        }

        const auto op = static_cast<A64::DataCacheOperation>(inst.GetArg(1).GetU64());
        if (op == A64::DataCacheOperation::ZeroByVA) {
            A64::IREmitter ir{block};
            ir.current_location = A64::LocationDescriptor{IR::LocationDescriptor{inst.GetArg(0).GetU64()}};
            ir.SetInsertionPointBefore(&inst);

            size_t bytes = 4 << static_cast<size_t>(conf.dczid_el0 & 0b1111);
            IR::U64 addr{inst.GetArg(2)};

            const IR::U128 zero_u128 = ir.ZeroExtendToQuad(ir.Imm64(0));
            while (bytes >= 16) {
                ir.WriteMemory128(addr, zero_u128, IR::AccType::DCZVA);
                addr = ir.Add(addr, ir.Imm64(16));
                bytes -= 16;
            }

            while (bytes >= 8) {
                ir.WriteMemory64(addr, ir.Imm64(0), IR::AccType::DCZVA);
                addr = ir.Add(addr, ir.Imm64(8));
                bytes -= 8;
            }

            while (bytes >= 4) {
                ir.WriteMemory32(addr, ir.Imm32(0), IR::AccType::DCZVA);
                addr = ir.Add(addr, ir.Imm64(4));
                bytes -= 4;
            }
        }
        inst.Invalidate();
    }
}

}  // namespace Dynarmic::Optimization
