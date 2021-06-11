/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>

#include "dynarmic/common/common_types.h"
#include "dynarmic/frontend/A64/types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Optimization {

void A64GetSetElimination(IR::Block& block) {
    using Iterator = IR::Block::iterator;

    enum class TrackingType {
        W,
        X,
        S,
        D,
        Q,
        SP,
        NZCV,
        NZCVRaw,
    };
    struct RegisterInfo {
        IR::Value register_value;
        TrackingType tracking_type;
        bool set_instruction_present = false;
        Iterator last_set_instruction;
    };
    std::array<RegisterInfo, 31> reg_info;
    std::array<RegisterInfo, 32> vec_info;
    RegisterInfo sp_info;
    RegisterInfo nzcv_info;

    const auto do_set = [&block](RegisterInfo& info, IR::Value value, Iterator set_inst, TrackingType tracking_type) {
        if (info.set_instruction_present) {
            info.last_set_instruction->Invalidate();
            block.Instructions().erase(info.last_set_instruction);
        }

        info.register_value = value;
        info.tracking_type = tracking_type;
        info.set_instruction_present = true;
        info.last_set_instruction = set_inst;
    };

    const auto do_get = [](RegisterInfo& info, Iterator get_inst, TrackingType tracking_type) {
        const auto do_nothing = [&] {
            info = {};
            info.register_value = IR::Value(&*get_inst);
            info.tracking_type = tracking_type;
        };

        if (info.register_value.IsEmpty()) {
            do_nothing();
            return;
        }

        if (info.tracking_type == tracking_type) {
            get_inst->ReplaceUsesWith(info.register_value);
            return;
        }

        do_nothing();
    };

    for (auto inst = block.begin(); inst != block.end(); ++inst) {
        switch (inst->GetOpcode()) {
        case IR::Opcode::A64GetW: {
            const size_t index = A64::RegNumber(inst->GetArg(0).GetA64RegRef());
            do_get(reg_info.at(index), inst, TrackingType::W);
            break;
        }
        case IR::Opcode::A64GetX: {
            const size_t index = A64::RegNumber(inst->GetArg(0).GetA64RegRef());
            do_get(reg_info.at(index), inst, TrackingType::X);
            break;
        }
        case IR::Opcode::A64GetS: {
            const size_t index = A64::VecNumber(inst->GetArg(0).GetA64VecRef());
            do_get(vec_info.at(index), inst, TrackingType::S);
            break;
        }
        case IR::Opcode::A64GetD: {
            const size_t index = A64::VecNumber(inst->GetArg(0).GetA64VecRef());
            do_get(vec_info.at(index), inst, TrackingType::D);
            break;
        }
        case IR::Opcode::A64GetQ: {
            const size_t index = A64::VecNumber(inst->GetArg(0).GetA64VecRef());
            do_get(vec_info.at(index), inst, TrackingType::Q);
            break;
        }
        case IR::Opcode::A64GetSP: {
            do_get(sp_info, inst, TrackingType::SP);
            break;
        }
        case IR::Opcode::A64GetNZCVRaw: {
            do_get(nzcv_info, inst, TrackingType::NZCVRaw);
            break;
        }
        case IR::Opcode::A64SetW: {
            const size_t index = A64::RegNumber(inst->GetArg(0).GetA64RegRef());
            do_set(reg_info.at(index), inst->GetArg(1), inst, TrackingType::W);
            break;
        }
        case IR::Opcode::A64SetX: {
            const size_t index = A64::RegNumber(inst->GetArg(0).GetA64RegRef());
            do_set(reg_info.at(index), inst->GetArg(1), inst, TrackingType::X);
            break;
        }
        case IR::Opcode::A64SetS: {
            const size_t index = A64::VecNumber(inst->GetArg(0).GetA64VecRef());
            do_set(vec_info.at(index), inst->GetArg(1), inst, TrackingType::S);
            break;
        }
        case IR::Opcode::A64SetD: {
            const size_t index = A64::VecNumber(inst->GetArg(0).GetA64VecRef());
            do_set(vec_info.at(index), inst->GetArg(1), inst, TrackingType::D);
            break;
        }
        case IR::Opcode::A64SetQ: {
            const size_t index = A64::VecNumber(inst->GetArg(0).GetA64VecRef());
            do_set(vec_info.at(index), inst->GetArg(1), inst, TrackingType::Q);
            break;
        }
        case IR::Opcode::A64SetSP: {
            do_set(sp_info, inst->GetArg(0), inst, TrackingType::SP);
            break;
        }
        case IR::Opcode::A64SetNZCV: {
            do_set(nzcv_info, inst->GetArg(0), inst, TrackingType::NZCV);
            break;
        }
        case IR::Opcode::A64SetNZCVRaw: {
            do_set(nzcv_info, inst->GetArg(0), inst, TrackingType::NZCVRaw);
            break;
        }
        default: {
            if (inst->ReadsFromCPSR() || inst->WritesToCPSR()) {
                nzcv_info = {};
            }
            if (inst->ReadsFromCoreRegister() || inst->WritesToCoreRegister()) {
                reg_info = {};
                vec_info = {};
                sp_info = {};
            }
            break;
        }
        }
    }
}

}  // namespace Dynarmic::Optimization
