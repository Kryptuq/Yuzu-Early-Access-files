/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Optimization {

void A32GetSetElimination(IR::Block& block) {
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
        RegisterInfo n;
        RegisterInfo z;
        RegisterInfo c;
        RegisterInfo v;
        RegisterInfo ge;
    } cpsr_info;

    const auto do_set = [&block](RegisterInfo& info, IR::Value value, Iterator set_inst) {
        if (info.set_instruction_present) {
            info.last_set_instruction->Invalidate();
            block.Instructions().erase(info.last_set_instruction);
        }

        info.register_value = value;
        info.set_instruction_present = true;
        info.last_set_instruction = set_inst;
    };

    const auto do_get = [](RegisterInfo& info, Iterator get_inst) {
        if (info.register_value.IsEmpty()) {
            info.register_value = IR::Value(&*get_inst);
            return;
        }
        get_inst->ReplaceUsesWith(info.register_value);
    };

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
        case IR::Opcode::A32SetNFlag: {
            do_set(cpsr_info.n, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32SetZFlag: {
            do_set(cpsr_info.z, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32SetCFlag: {
            do_set(cpsr_info.c, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32GetCFlag: {
            do_get(cpsr_info.c, inst);
            break;
        }
        case IR::Opcode::A32SetVFlag: {
            do_set(cpsr_info.v, inst->GetArg(0), inst);
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
