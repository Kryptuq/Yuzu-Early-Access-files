/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Optimization {

void A32GetSetElimination(IR::Block& block, A32GetSetEliminationOptions opt) {
    using Iterator = IR::Block::iterator;
    struct RegisterInfo {
        IR::Value register_value;
        bool set_instruction_present = false;
        Iterator last_set_instruction;
    };
    std::array<RegisterInfo, 15> reg_info;
    std::array<RegisterInfo, 64> ext_reg_singles_info;
    std::array<RegisterInfo, 32> ext_reg_doubles_info;
    std::array<RegisterInfo, 32> ext_reg_vector_double_info;
    std::array<RegisterInfo, 16> ext_reg_vector_quad_info;
    struct CpsrInfo {
        RegisterInfo nz;
        RegisterInfo c;
        RegisterInfo nzc;
        RegisterInfo nzcv;
        RegisterInfo ge;
    } cpsr_info;

    const auto do_delete_last_set = [&block](RegisterInfo& info) {
        if (info.set_instruction_present) {
            info.set_instruction_present = false;
            info.last_set_instruction->Invalidate();
            block.Instructions().erase(info.last_set_instruction);
        }
        info = {};
    };

    const auto do_set = [&do_delete_last_set](RegisterInfo& info, IR::Value value, Iterator set_inst) {
        do_delete_last_set(info);
        info.register_value = value;
        info.set_instruction_present = true;
        info.last_set_instruction = set_inst;
    };

    const auto do_set_without_inst = [&do_delete_last_set](RegisterInfo& info, IR::Value value) {
        do_delete_last_set(info);
        info.register_value = value;
    };

    const auto do_get = [](RegisterInfo& info, Iterator get_inst) {
        if (info.register_value.IsEmpty()) {
            info.register_value = IR::Value(&*get_inst);
            return;
        }
        get_inst->ReplaceUsesWith(info.register_value);
    };

    // Location and version don't matter here.
    A32::IREmitter ir{block, A32::LocationDescriptor{block.Location()}, {}};

    for (auto inst = block.begin(); inst != block.end(); ++inst) {
        switch (inst->GetOpcode()) {
        case IR::Opcode::A32SetRegister: {
            const A32::Reg reg = inst->GetArg(0).GetA32RegRef();
            if (reg == A32::Reg::PC) {
                break;
            }
            const auto reg_index = static_cast<size_t>(reg);
            do_set(reg_info[reg_index], inst->GetArg(1), inst);
            break;
        }
        case IR::Opcode::A32GetRegister: {
            const A32::Reg reg = inst->GetArg(0).GetA32RegRef();
            ASSERT(reg != A32::Reg::PC);
            const size_t reg_index = static_cast<size_t>(reg);
            do_get(reg_info[reg_index], inst);
            break;
        }
        case IR::Opcode::A32SetExtendedRegister32: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_set(ext_reg_singles_info[reg_index], inst->GetArg(1), inst);

            ext_reg_doubles_info[reg_index / 2] = {};
            ext_reg_vector_double_info[reg_index / 2] = {};
            ext_reg_vector_quad_info[reg_index / 4] = {};
            break;
        }
        case IR::Opcode::A32GetExtendedRegister32: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_get(ext_reg_singles_info[reg_index], inst);

            ext_reg_doubles_info[reg_index / 2] = {};
            ext_reg_vector_double_info[reg_index / 2] = {};
            ext_reg_vector_quad_info[reg_index / 4] = {};
            break;
        }
        case IR::Opcode::A32SetExtendedRegister64: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_set(ext_reg_doubles_info[reg_index], inst->GetArg(1), inst);

            ext_reg_singles_info[reg_index * 2 + 0] = {};
            ext_reg_singles_info[reg_index * 2 + 1] = {};
            ext_reg_vector_double_info[reg_index] = {};
            ext_reg_vector_quad_info[reg_index / 2] = {};
            break;
        }
        case IR::Opcode::A32GetExtendedRegister64: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_get(ext_reg_doubles_info[reg_index], inst);

            ext_reg_singles_info[reg_index * 2 + 0] = {};
            ext_reg_singles_info[reg_index * 2 + 1] = {};
            ext_reg_vector_double_info[reg_index] = {};
            ext_reg_vector_quad_info[reg_index / 2] = {};
            break;
        }
        case IR::Opcode::A32SetVector: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            if (A32::IsDoubleExtReg(reg)) {
                do_set(ext_reg_vector_double_info[reg_index], inst->GetArg(1), inst);

                ext_reg_singles_info[reg_index * 2 + 0] = {};
                ext_reg_singles_info[reg_index * 2 + 1] = {};
                ext_reg_doubles_info[reg_index] = {};
                ext_reg_vector_quad_info[reg_index / 2] = {};
            } else {
                DEBUG_ASSERT(A32::IsQuadExtReg(reg));

                do_set(ext_reg_vector_quad_info[reg_index], inst->GetArg(1), inst);

                ext_reg_singles_info[reg_index * 4 + 0] = {};
                ext_reg_singles_info[reg_index * 4 + 1] = {};
                ext_reg_singles_info[reg_index * 4 + 2] = {};
                ext_reg_singles_info[reg_index * 4 + 3] = {};
                ext_reg_doubles_info[reg_index * 2 + 0] = {};
                ext_reg_doubles_info[reg_index * 2 + 1] = {};
                ext_reg_vector_double_info[reg_index * 2 + 0] = {};
                ext_reg_vector_double_info[reg_index * 2 + 1] = {};
            }
            break;
        }
        case IR::Opcode::A32GetVector: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            if (A32::IsDoubleExtReg(reg)) {
                do_get(ext_reg_vector_double_info[reg_index], inst);

                ext_reg_singles_info[reg_index * 2 + 0] = {};
                ext_reg_singles_info[reg_index * 2 + 1] = {};
                ext_reg_doubles_info[reg_index] = {};
                ext_reg_vector_quad_info[reg_index / 2] = {};
            } else {
                DEBUG_ASSERT(A32::IsQuadExtReg(reg));

                do_get(ext_reg_vector_quad_info[reg_index], inst);

                ext_reg_singles_info[reg_index * 4 + 0] = {};
                ext_reg_singles_info[reg_index * 4 + 1] = {};
                ext_reg_singles_info[reg_index * 4 + 2] = {};
                ext_reg_singles_info[reg_index * 4 + 3] = {};
                ext_reg_doubles_info[reg_index * 2 + 0] = {};
                ext_reg_doubles_info[reg_index * 2 + 1] = {};
                ext_reg_vector_double_info[reg_index * 2 + 0] = {};
                ext_reg_vector_double_info[reg_index * 2 + 1] = {};
            }
            break;
        }
        case IR::Opcode::A32GetCFlag: {
            if (cpsr_info.c.register_value.IsEmpty() && cpsr_info.nzcv.register_value.GetType() == IR::Type::NZCVFlags) {
                ir.SetInsertionPointBefore(inst);
                IR::U1 c = ir.GetCFlagFromNZCV(IR::NZCV{cpsr_info.nzcv.register_value});
                inst->ReplaceUsesWith(c);
                cpsr_info.c.register_value = c;
                break;
            }

            do_get(cpsr_info.c, inst);
            // ensure source is not deleted
            cpsr_info.nzc = {};
            cpsr_info.nzcv = {};
            break;
        }
        case IR::Opcode::A32SetCpsrNZCV:
        case IR::Opcode::A32SetCpsrNZCVRaw: {
            do_delete_last_set(cpsr_info.nz);
            do_delete_last_set(cpsr_info.c);
            do_delete_last_set(cpsr_info.nzc);
            do_set(cpsr_info.nzcv, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32SetCpsrNZCVQ: {
            do_delete_last_set(cpsr_info.nz);
            do_delete_last_set(cpsr_info.c);
            do_delete_last_set(cpsr_info.nzc);
            do_delete_last_set(cpsr_info.nzcv);
            break;
        }
        case IR::Opcode::A32SetCpsrNZ: {
            if (cpsr_info.nzc.set_instruction_present) {
                cpsr_info.nzc.last_set_instruction->SetArg(0, IR::Value::EmptyNZCVImmediateMarker());
            }

            if (opt.convert_nz_to_nzc && !cpsr_info.c.register_value.IsEmpty()) {
                ir.SetInsertionPointAfter(inst);
                ir.SetCpsrNZC(IR::NZCV{inst->GetArg(0)}, ir.GetCFlag());
                inst->Invalidate();
                break;
            }

            // cpsr_info.c remains valid
            cpsr_info.nzc = {};
            cpsr_info.nzcv = {};
            do_set(cpsr_info.nz, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32SetCpsrNZC: {
            if (opt.convert_nzc_to_nz && !inst->GetArg(1).IsImmediate() && inst->GetArg(1).GetInstRecursive()->GetOpcode() == IR::Opcode::A32GetCFlag) {
                ir.SetInsertionPointAfter(inst);
                ir.SetCpsrNZ(IR::NZCV{inst->GetArg(0)});
                inst->Invalidate();
                break;
            }

            cpsr_info.nzcv = {};
            do_set(cpsr_info.nzc, {}, inst);
            do_set_without_inst(cpsr_info.nz, inst->GetArg(0));
            do_set_without_inst(cpsr_info.c, inst->GetArg(1));
            break;
        }
        case IR::Opcode::A32SetGEFlags: {
            do_set(cpsr_info.ge, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32GetGEFlags: {
            do_get(cpsr_info.ge, inst);
            break;
        }
        default: {
            if (inst->ReadsFromCPSR() || inst->WritesToCPSR()) {
                cpsr_info = {};
            }
            break;
        }
        }
    }
}

}  // namespace Dynarmic::Optimization
