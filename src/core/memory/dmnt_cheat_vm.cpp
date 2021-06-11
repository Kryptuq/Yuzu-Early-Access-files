/*
 * Copyright (c) 2018-2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Adapted by DarkLordZach for use/interaction with yuzu
 *
 * Modifications Copyright 2019 yuzu emulator team
 * Licensed under GPLv2 or any later version
 * Refer to the license.txt file included.
 */

#include "common/assert.h"
#include "common/scope_exit.h"
#include "core/memory/dmnt_cheat_types.h"
#include "core/memory/dmnt_cheat_vm.h"

namespace Core::Memory {

DmntCheatVm::DmntCheatVm(std::unique_ptr<Callbacks> callbacks_)
    : callbacks(std::move(callbacks_)) {}

DmntCheatVm::~DmntCheatVm() = default;

void DmntCheatVm::DebugLog(u32 log_id, u64 value) {
    callbacks->DebugLog(static_cast<u8>(log_id), value);
}

void DmntCheatVm::LogOpcode(const CheatVmOpcode& opcode) {
    if (auto store_static = std::get_if<StoreStaticOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Store Static");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", store_static->bit_width));
        callbacks->CommandLog(
            fmt::format("Mem Type:  {:X}", static_cast<u32>(store_static->mem_type)));
        callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", store_static->offset_register));
        callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", store_static->rel_address));
        callbacks->CommandLog(fmt::format("Value:     {:X}", store_static->value.bit64));
    } else if (auto begin_cond = std::get_if<BeginConditionalOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Begin Conditional");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", begin_cond->bit_width));
        callbacks->CommandLog(
            fmt::format("Mem Type:  {:X}", static_cast<u32>(begin_cond->mem_type)));
        callbacks->CommandLog(
            fmt::format("Cond Type: {:X}", static_cast<u32>(begin_cond->cond_type)));
        callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", begin_cond->rel_address));
        callbacks->CommandLog(fmt::format("Value:     {:X}", begin_cond->value.bit64));
    } else if (std::holds_alternative<EndConditionalOpcode>(opcode.opcode)) {
        callbacks->CommandLog("Opcode: End Conditional");
    } else if (auto ctrl_loop = std::get_if<ControlLoopOpcode>(&opcode.opcode)) {
        if (ctrl_loop->start_loop) {
            callbacks->CommandLog("Opcode: Start Loop");
            callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", ctrl_loop->reg_index));
            callbacks->CommandLog(fmt::format("Num Iters: {:X}", ctrl_loop->num_iters));
        } else {
            callbacks->CommandLog("Opcode: End Loop");
            callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", ctrl_loop->reg_index));
        }
    } else if (auto ldr_static = std::get_if<LoadRegisterStaticOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Load Register Static");
        callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", ldr_static->reg_index));
        callbacks->CommandLog(fmt::format("Value:     {:X}", ldr_static->value));
    } else if (auto ldr_memory = std::get_if<LoadRegisterMemoryOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Load Register Memory");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", ldr_memory->bit_width));
        callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", ldr_memory->reg_index));
        callbacks->CommandLog(
            fmt::format("Mem Type:  {:X}", static_cast<u32>(ldr_memory->mem_type)));
        callbacks->CommandLog(fmt::format("From Reg:  {:d}", ldr_memory->load_from_reg));
        callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", ldr_memory->rel_address));
    } else if (auto str_static = std::get_if<StoreStaticToAddressOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Store Static to Address");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", str_static->bit_width));
        callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", str_static->reg_index));
        if (str_static->add_offset_reg) {
            callbacks->CommandLog(fmt::format("O Reg Idx: {:X}", str_static->offset_reg_index));
        }
        callbacks->CommandLog(fmt::format("Incr Reg:  {:d}", str_static->increment_reg));
        callbacks->CommandLog(fmt::format("Value:     {:X}", str_static->value));
    } else if (auto perform_math_static =
                   std::get_if<PerformArithmeticStaticOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Perform Static Arithmetic");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", perform_math_static->bit_width));
        callbacks->CommandLog(fmt::format("Reg Idx:   {:X}", perform_math_static->reg_index));
        callbacks->CommandLog(
            fmt::format("Math Type: {:X}", static_cast<u32>(perform_math_static->math_type)));
        callbacks->CommandLog(fmt::format("Value:     {:X}", perform_math_static->value));
    } else if (auto begin_keypress_cond =
                   std::get_if<BeginKeypressConditionalOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Begin Keypress Conditional");
        callbacks->CommandLog(fmt::format("Key Mask:  {:X}", begin_keypress_cond->key_mask));
    } else if (auto perform_math_reg =
                   std::get_if<PerformArithmeticRegisterOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Perform Register Arithmetic");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", perform_math_reg->bit_width));
        callbacks->CommandLog(fmt::format("Dst Idx:   {:X}", perform_math_reg->dst_reg_index));
        callbacks->CommandLog(fmt::format("Src1 Idx:  {:X}", perform_math_reg->src_reg_1_index));
        if (perform_math_reg->has_immediate) {
            callbacks->CommandLog(fmt::format("Value:     {:X}", perform_math_reg->value.bit64));
        } else {
            callbacks->CommandLog(
                fmt::format("Src2 Idx:  {:X}", perform_math_reg->src_reg_2_index));
        }
    } else if (auto str_register = std::get_if<StoreRegisterToAddressOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Store Register to Address");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", str_register->bit_width));
        callbacks->CommandLog(fmt::format("S Reg Idx: {:X}", str_register->str_reg_index));
        callbacks->CommandLog(fmt::format("A Reg Idx: {:X}", str_register->addr_reg_index));
        callbacks->CommandLog(fmt::format("Incr Reg:  {:d}", str_register->increment_reg));
        switch (str_register->ofs_type) {
        case StoreRegisterOffsetType::None:
            break;
        case StoreRegisterOffsetType::Reg:
            callbacks->CommandLog(fmt::format("O Reg Idx: {:X}", str_register->ofs_reg_index));
            break;
        case StoreRegisterOffsetType::Imm:
            callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", str_register->rel_address));
            break;
        case StoreRegisterOffsetType::MemReg:
            callbacks->CommandLog(
                fmt::format("Mem Type:  {:X}", static_cast<u32>(str_register->mem_type)));
            break;
        case StoreRegisterOffsetType::MemImm:
        case StoreRegisterOffsetType::MemImmReg:
            callbacks->CommandLog(
                fmt::format("Mem Type:  {:X}", static_cast<u32>(str_register->mem_type)));
            callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", str_register->rel_address));
            break;
        }
    } else if (auto begin_reg_cond = std::get_if<BeginRegisterConditionalOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Begin Register Conditional");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", begin_reg_cond->bit_width));
        callbacks->CommandLog(
            fmt::format("Cond Type: {:X}", static_cast<u32>(begin_reg_cond->cond_type)));
        callbacks->CommandLog(fmt::format("V Reg Idx: {:X}", begin_reg_cond->val_reg_index));
        switch (begin_reg_cond->comp_type) {
        case CompareRegisterValueType::StaticValue:
            callbacks->CommandLog("Comp Type: Static Value");
            callbacks->CommandLog(fmt::format("Value:     {:X}", begin_reg_cond->value.bit64));
            break;
        case CompareRegisterValueType::OtherRegister:
            callbacks->CommandLog("Comp Type: Other Register");
            callbacks->CommandLog(fmt::format("X Reg Idx: {:X}", begin_reg_cond->other_reg_index));
            break;
        case CompareRegisterValueType::MemoryRelAddr:
            callbacks->CommandLog("Comp Type: Memory Relative Address");
            callbacks->CommandLog(
                fmt::format("Mem Type:  {:X}", static_cast<u32>(begin_reg_cond->mem_type)));
            callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", begin_reg_cond->rel_address));
            break;
        case CompareRegisterValueType::MemoryOfsReg:
            callbacks->CommandLog("Comp Type: Memory Offset Register");
            callbacks->CommandLog(
                fmt::format("Mem Type:  {:X}", static_cast<u32>(begin_reg_cond->mem_type)));
            callbacks->CommandLog(fmt::format("O Reg Idx: {:X}", begin_reg_cond->ofs_reg_index));
            break;
        case CompareRegisterValueType::RegisterRelAddr:
            callbacks->CommandLog("Comp Type: Register Relative Address");
            callbacks->CommandLog(fmt::format("A Reg Idx: {:X}", begin_reg_cond->addr_reg_index));
            callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", begin_reg_cond->rel_address));
            break;
        case CompareRegisterValueType::RegisterOfsReg:
            callbacks->CommandLog("Comp Type: Register Offset Register");
            callbacks->CommandLog(fmt::format("A Reg Idx: {:X}", begin_reg_cond->addr_reg_index));
            callbacks->CommandLog(fmt::format("O Reg Idx: {:X}", begin_reg_cond->ofs_reg_index));
            break;
        }
    } else if (auto save_restore_reg = std::get_if<SaveRestoreRegisterOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Save or Restore Register");
        callbacks->CommandLog(fmt::format("Dst Idx:   {:X}", save_restore_reg->dst_index));
        callbacks->CommandLog(fmt::format("Src Idx:   {:X}", save_restore_reg->src_index));
        callbacks->CommandLog(
            fmt::format("Op Type:   {:d}", static_cast<u32>(save_restore_reg->op_type)));
    } else if (auto save_restore_regmask =
                   std::get_if<SaveRestoreRegisterMaskOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Save or Restore Register Mask");
        callbacks->CommandLog(
            fmt::format("Op Type:   {:d}", static_cast<u32>(save_restore_regmask->op_type)));
        for (std::size_t i = 0; i < NumRegisters; i++) {
            callbacks->CommandLog(
                fmt::format("Act[{:02X}]:   {:d}", i, save_restore_regmask->should_operate[i]));
        }
    } else if (auto rw_static_reg = std::get_if<ReadWriteStaticRegisterOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Read/Write Static Register");
        if (rw_static_reg->static_idx < NumReadableStaticRegisters) {
            callbacks->CommandLog("Op Type: ReadStaticRegister");
        } else {
            callbacks->CommandLog("Op Type: WriteStaticRegister");
        }
        callbacks->CommandLog(fmt::format("Reg Idx   {:X}", rw_static_reg->idx));
        callbacks->CommandLog(fmt::format("Stc Idx   {:X}", rw_static_reg->static_idx));
    } else if (auto debug_log = std::get_if<DebugLogOpcode>(&opcode.opcode)) {
        callbacks->CommandLog("Opcode: Debug Log");
        callbacks->CommandLog(fmt::format("Bit Width: {:X}", debug_log->bit_width));
        callbacks->CommandLog(fmt::format("Log ID:    {:X}", debug_log->log_id));
        callbacks->CommandLog(
            fmt::format("Val Type:  {:X}", static_cast<u32>(debug_log->val_type)));
        switch (debug_log->val_type) {
        case DebugLogValueType::RegisterValue:
            callbacks->CommandLog("Val Type:  Register Value");
            callbacks->CommandLog(fmt::format("X Reg Idx: {:X}", debug_log->val_reg_index));
            break;
        case DebugLogValueType::MemoryRelAddr:
            callbacks->CommandLog("Val Type:  Memory Relative Address");
            callbacks->CommandLog(
                fmt::format("Mem Type:  {:X}", static_cast<u32>(debug_log->mem_type)));
            callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", debug_log->rel_address));
            break;
        case DebugLogValueType::MemoryOfsReg:
            callbacks->CommandLog("Val Type:  Memory Offset Register");
            callbacks->CommandLog(
                fmt::format("Mem Type:  {:X}", static_cast<u32>(debug_log->mem_type)));
            callbacks->CommandLog(fmt::format("O Reg Idx: {:X}", debug_log->ofs_reg_index));
            break;
        case DebugLogValueType::RegisterRelAddr:
            callbacks->CommandLog("Val Type:  Register Relative Address");
            callbacks->CommandLog(fmt::format("A Reg Idx: {:X}", debug_log->addr_reg_index));
            callbacks->CommandLog(fmt::format("Rel Addr:  {:X}", debug_log->rel_address));
            break;
        case DebugLogValueType::RegisterOfsReg:
            callbacks->CommandLog("Val Type:  Register Offset Register");
            callbacks->CommandLog(fmt::format("A Reg Idx: {:X}", debug_log->addr_reg_index));
            callbacks->CommandLog(fmt::format("O Reg Idx: {:X}", debug_log->ofs_reg_index));
            break;
        }
    } else if (auto instr = std::get_if<UnrecognizedInstruction>(&opcode.opcode)) {
        callbacks->CommandLog(fmt::format("Unknown opcode: {:X}", static_cast<u32>(instr->opcode)));
    }
}

DmntCheatVm::Callbacks::~Callbacks() = default;

bool DmntCheatVm::DecodeNextOpcode(CheatVmOpcode& out) {
    // If we've ever seen a decode failure, return false.
    bool valid = decode_success;
    CheatVmOpcode opcode = {};
    SCOPE_EXIT({
        decode_success &= valid;
        if (valid) {
            out = opcode;
        }
    });

    // Helper function for getting instruction dwords.
    const auto GetNextDword = [&] {
        if (instruction_ptr >= num_opcodes) {
            valid = false;
            return static_cast<u32>(0);
        }
        return program[instruction_ptr++];
    };

    // Helper function for parsing a VmInt.
    const auto GetNextVmInt = [&](const u32 bit_width) {
        VmInt val{};

        const u32 first_dword = GetNextDword();
        switch (bit_width) {
        case 1:
            val.bit8 = static_cast<u8>(first_dword);
            break;
        case 2:
            val.bit16 = static_cast<u16>(first_dword);
            break;
        case 4:
            val.bit32 = first_dword;
            break;
        case 8:
            val.bit64 = (static_cast<u64>(first_dword) << 32ul) | static_cast<u64>(GetNextDword());
            break;
        }

        return val;
    };

    // Read opcode.
    const u32 first_dword = GetNextDword();
    if (!valid) {
        return valid;
    }

    auto opcode_type = static_cast<CheatVmOpcodeType>(((first_dword >> 28) & 0xF));
    if (opcode_type >= CheatVmOpcodeType::ExtendedWidth) {
        opcode_type = static_cast<CheatVmOpcodeType>((static_cast<u32>(opcode_type) << 4) |
                                                     ((first_dword >> 24) & 0xF));
    }
    if (opcode_type >= CheatVmOpcodeType::DoubleExtendedWidth) {
        opcode_type = static_cast<CheatVmOpcodeType>((static_cast<u32>(opcode_type) << 4) |
                                                     ((first_dword >> 20) & 0xF));
    }

    // detect condition start.
    switch (opcode_type) {
    case CheatVmOpcodeType::BeginConditionalBlock:
    case CheatVmOpcodeType::BeginKeypressConditionalBlock:
    case CheatVmOpcodeType::BeginRegisterConditionalBlock:
        opcode.begin_conditional_block = true;
        break;
    default:
        opcode.begin_conditional_block = false;
        break;
    }

    switch (opcode_type) {
    case CheatVmOpcodeType::StoreStatic: {
        // 0TMR00AA AAAAAAAA YYYYYYYY (YYYYYYYY)
        // Read additional words.
        const u32 second_dword = GetNextDword();
        const u32 bit_width = (first_dword >> 24) & 0xF;

        opcode.opcode = StoreStaticOpcode{
            .bit_width = bit_width,
            .mem_type = static_cast<MemoryAccessType>((first_dword >> 20) & 0xF),
            .offset_register = (first_dword >> 16) & 0xF,
            .rel_address = (static_cast<u64>(first_dword & 0xFF) << 32) | second_dword,
            .value = GetNextVmInt(bit_width),
        };
    } break;
    case CheatVmOpcodeType::BeginConditionalBlock: {
        // 1TMC00AA AAAAAAAA YYYYYYYY (YYYYYYYY)
        // Read additional words.
        const u32 second_dword = GetNextDword();
        const u32 bit_width = (first_dword >> 24) & 0xF;

        opcode.opcode = BeginConditionalOpcode{
            .bit_width = bit_width,
            .mem_type = static_cast<MemoryAccessType>((first_dword >> 20) & 0xF),
            .cond_type = static_cast<ConditionalComparisonType>((first_dword >> 16) & 0xF),
            .rel_address = (static_cast<u64>(first_dword & 0xFF) << 32) | second_dword,
            .value = GetNextVmInt(bit_width),
        };
    } break;
    case CheatVmOpcodeType::EndConditionalBlock: {
        // 20000000
        // There's actually nothing left to process here!
        opcode.opcode = EndConditionalOpcode{};
    } break;
    case CheatVmOpcodeType::ControlLoop: {
        // 300R0000 VVVVVVVV
        // 310R0000
        // Parse register, whether loop start or loop end.
        ControlLoopOpcode ctrl_loop{
            .start_loop = ((first_dword >> 24) & 0xF) == 0,
            .reg_index = (first_dword >> 20) & 0xF,
            .num_iters = 0,
        };

        // Read number of iters if loop start.
        if (ctrl_loop.start_loop) {
            ctrl_loop.num_iters = GetNextDword();
        }
        opcode.opcode = ctrl_loop;
    } break;
    case CheatVmOpcodeType::LoadRegisterStatic: {
        // 400R0000 VVVVVVVV VVVVVVVV
        // Read additional words.
        opcode.opcode = LoadRegisterStaticOpcode{
            .reg_index = (first_dword >> 16) & 0xF,
            .value = (static_cast<u64>(GetNextDword()) << 32) | GetNextDword(),
        };
    } break;
    case CheatVmOpcodeType::LoadRegisterMemory: {
        // 5TMRI0AA AAAAAAAA
        // Read additional words.
        const u32 second_dword = GetNextDword();
        opcode.opcode = LoadRegisterMemoryOpcode{
            .bit_width = (first_dword >> 24) & 0xF,
            .mem_type = static_cast<MemoryAccessType>((first_dword >> 20) & 0xF),
            .reg_index = ((first_dword >> 16) & 0xF),
            .load_from_reg = ((first_dword >> 12) & 0xF) != 0,
            .rel_address = (static_cast<u64>(first_dword & 0xFF) << 32) | second_dword,
        };
    } break;
    case CheatVmOpcodeType::StoreStaticToAddress: {
        // 6T0RIor0 VVVVVVVV VVVVVVVV
        // Read additional words.
        opcode.opcode = StoreStaticToAddressOpcode{
            .bit_width = (first_dword >> 24) & 0xF,
            .reg_index = (first_dword >> 16) & 0xF,
            .increment_reg = ((first_dword >> 12) & 0xF) != 0,
            .add_offset_reg = ((first_dword >> 8) & 0xF) != 0,
            .offset_reg_index = (first_dword >> 4) & 0xF,
            .value = (static_cast<u64>(GetNextDword()) << 32) | GetNextDword(),
        };
    } break;
    case CheatVmOpcodeType::PerformArithmeticStatic: {
        // 7T0RC000 VVVVVVVV
        // Read additional words.
        opcode.opcode = PerformArithmeticStaticOpcode{
            .bit_width = (first_dword >> 24) & 0xF,
            .reg_index = ((first_dword >> 16) & 0xF),
            .math_type = static_cast<RegisterArithmeticType>((first_dword >> 12) & 0xF),
            .value = GetNextDword(),
        };
    } break;
    case CheatVmOpcodeType::BeginKeypressConditionalBlock: {
        // 8kkkkkkk
        // Just parse the mask.
        opcode.opcode = BeginKeypressConditionalOpcode{
            .key_mask = first_dword & 0x0FFFFFFF,
        };
    } break;
    case CheatVmOpcodeType::PerformArithmeticRegister: {
        // 9TCRSIs0 (VVVVVVVV (VVVVVVVV))
        PerformArithmeticRegisterOpcode perform_math_reg{
            .bit_width = (first_dword >> 24) & 0xF,
            .math_type = static_cast<RegisterArithmeticType>((first_dword >> 20) & 0xF),
            .dst_reg_index = (first_dword >> 16) & 0xF,
            .src_reg_1_index = (first_dword >> 12) & 0xF,
            .src_reg_2_index = 0,
            .has_immediate = ((first_dword >> 8) & 0xF) != 0,
            .value = {},
        };
        if (perform_math_reg.has_immediate) {
            perform_math_reg.src_reg_2_index = 0;
            perform_math_reg.value = GetNextVmInt(perform_math_reg.bit_width);
        } else {
            perform_math_reg.src_reg_2_index = ((first_dword >> 4) & 0xF);
        }
        opcode.opcode = perform_math_reg;
    } break;
    case CheatVmOpcodeType::StoreRegisterToAddress: {
        // ATSRIOxa (aaaaaaaa)
        // A = opcode 10
        // T = bit width
        // S = src register index
        // R = address register index
        // I = 1 if increment address register, 0 if not increment address register
        // O = offset type, 0 = None, 1 = Register, 2 = Immediate, 3 = Memory Region,
        //       4 = Memory Region + Relative Address (ignore address register), 5 = Memory Region +
        //  Relative Address
        // x = offset register (for offset type 1), memory type (for offset type 3)
        // a = relative address (for offset type 2+3)
        StoreRegisterToAddressOpcode str_register{
            .bit_width = (first_dword >> 24) & 0xF,
            .str_reg_index = (first_dword >> 20) & 0xF,
            .addr_reg_index = (first_dword >> 16) & 0xF,
            .increment_reg = ((first_dword >> 12) & 0xF) != 0,
            .ofs_type = static_cast<StoreRegisterOffsetType>(((first_dword >> 8) & 0xF)),
            .mem_type = MemoryAccessType::MainNso,
            .ofs_reg_index = (first_dword >> 4) & 0xF,
            .rel_address = 0,
        };
        switch (str_register.ofs_type) {
        case StoreRegisterOffsetType::None:
        case StoreRegisterOffsetType::Reg:
            // Nothing more to do
            break;
        case StoreRegisterOffsetType::Imm:
            str_register.rel_address = (static_cast<u64>(first_dword & 0xF) << 32) | GetNextDword();
            break;
        case StoreRegisterOffsetType::MemReg:
            str_register.mem_type = static_cast<MemoryAccessType>((first_dword >> 4) & 0xF);
            break;
        case StoreRegisterOffsetType::MemImm:
        case StoreRegisterOffsetType::MemImmReg:
            str_register.mem_type = static_cast<MemoryAccessType>((first_dword >> 4) & 0xF);
            str_register.rel_address = (static_cast<u64>(first_dword & 0xF) << 32) | GetNextDword();
            break;
        default:
            str_register.ofs_type = StoreRegisterOffsetType::None;
            break;
        }
        opcode.opcode = str_register;
    } break;
    case CheatVmOpcodeType::BeginRegisterConditionalBlock: {
        // C0TcSX##
        // C0TcS0Ma aaaaaaaa
        // C0TcS1Mr
        // C0TcS2Ra aaaaaaaa
        // C0TcS3Rr
        // C0TcS400 VVVVVVVV (VVVVVVVV)
        // C0TcS5X0
        // C0 = opcode 0xC0
        // T = bit width
        // c = condition type.
        // S = source register.
        // X = value operand type, 0 = main/heap with relative offset, 1 = main/heap with offset
        // register,
        //     2 = register with relative offset, 3 = register with offset register, 4 = static
        // value, 5 = other register.
        // M = memory type.
        // R = address register.
        // a = relative address.
        // r = offset register.
        // X = other register.
        // V = value.

        BeginRegisterConditionalOpcode begin_reg_cond{
            .bit_width = (first_dword >> 20) & 0xF,
            .cond_type = static_cast<ConditionalComparisonType>((first_dword >> 16) & 0xF),
            .val_reg_index = (first_dword >> 12) & 0xF,
            .comp_type = static_cast<CompareRegisterValueType>((first_dword >> 8) & 0xF),
            .mem_type = MemoryAccessType::MainNso,
            .addr_reg_index = 0,
            .other_reg_index = 0,
            .ofs_reg_index = 0,
            .rel_address = 0,
            .value = {},
        };

        switch (begin_reg_cond.comp_type) {
        case CompareRegisterValueType::StaticValue:
            begin_reg_cond.value = GetNextVmInt(begin_reg_cond.bit_width);
            break;
        case CompareRegisterValueType::OtherRegister:
            begin_reg_cond.other_reg_index = ((first_dword >> 4) & 0xF);
            break;
        case CompareRegisterValueType::MemoryRelAddr:
            begin_reg_cond.mem_type = static_cast<MemoryAccessType>((first_dword >> 4) & 0xF);
            begin_reg_cond.rel_address =
                (static_cast<u64>(first_dword & 0xF) << 32) | GetNextDword();
            break;
        case CompareRegisterValueType::MemoryOfsReg:
            begin_reg_cond.mem_type = static_cast<MemoryAccessType>((first_dword >> 4) & 0xF);
            begin_reg_cond.ofs_reg_index = (first_dword & 0xF);
            break;
        case CompareRegisterValueType::RegisterRelAddr:
            begin_reg_cond.addr_reg_index = (first_dword >> 4) & 0xF;
            begin_reg_cond.rel_address =
                (static_cast<u64>(first_dword & 0xF) << 32) | GetNextDword();
            break;
        case CompareRegisterValueType::RegisterOfsReg:
            begin_reg_cond.addr_reg_index = (first_dword >> 4) & 0xF;
            begin_reg_cond.ofs_reg_index = first_dword & 0xF;
            break;
        }
        opcode.opcode = begin_reg_cond;
    } break;
    case CheatVmOpcodeType::SaveRestoreRegister: {
        // C10D0Sx0
        // C1 = opcode 0xC1
        // D = destination index.
        // S = source index.
        // x = 3 if clearing reg, 2 if clearing saved value, 1 if saving a register, 0 if restoring
        // a register.
        // NOTE: If we add more save slots later, current encoding is backwards compatible.
        opcode.opcode = SaveRestoreRegisterOpcode{
            .dst_index = (first_dword >> 16) & 0xF,
            .src_index = (first_dword >> 8) & 0xF,
            .op_type = static_cast<SaveRestoreRegisterOpType>((first_dword >> 4) & 0xF),
        };
    } break;
    case CheatVmOpcodeType::SaveRestoreRegisterMask: {
        // C2x0XXXX
        // C2 = opcode 0xC2
        // x = 3 if clearing reg, 2 if clearing saved value, 1 if saving, 0 if restoring.
        // X = 16-bit bitmask, bit i --> save or restore register i.
        SaveRestoreRegisterMaskOpcode save_restore_regmask{
            .op_type = static_cast<SaveRestoreRegisterOpType>((first_dword >> 20) & 0xF),
            .should_operate = {},
        };
        for (std::size_t i = 0; i < NumRegisters; i++) {
            save_restore_regmask.should_operate[i] = (first_dword & (1U << i)) != 0;
        }
        opcode.opcode = save_restore_regmask;
    } break;
    case CheatVmOpcodeType::ReadWriteStaticRegister: {
        // C3000XXx
        // C3 = opcode 0xC3.
        // XX = static register index.
        // x  = register index.
        opcode.opcode = ReadWriteStaticRegisterOpcode{
            .static_idx = (first_dword >> 4) & 0xFF,
            .idx = first_dword & 0xF,
        };
    } break;
    case CheatVmOpcodeType::DebugLog: {
        // FFFTIX##
        // FFFTI0Ma aaaaaaaa
        // FFFTI1Mr
        // FFFTI2Ra aaaaaaaa
        // FFFTI3Rr
        // FFFTI4X0
        // FFF = opcode 0xFFF
        // T = bit width.
        // I = log id.
        // X = value operand type, 0 = main/heap with relative offset, 1 = main/heap with offset
        // register,
        //     2 = register with relative offset, 3 = register with offset register, 4 = register
        // value.
        // M = memory type.
        // R = address register.
        // a = relative address.
        // r = offset register.
        // X = value register.
        DebugLogOpcode debug_log{
            .bit_width = (first_dword >> 16) & 0xF,
            .log_id = (first_dword >> 12) & 0xF,
            .val_type = static_cast<DebugLogValueType>((first_dword >> 8) & 0xF),
            .mem_type = MemoryAccessType::MainNso,
            .addr_reg_index = 0,
            .val_reg_index = 0,
            .ofs_reg_index = 0,
            .rel_address = 0,
        };

        switch (debug_log.val_type) {
        case DebugLogValueType::RegisterValue:
            debug_log.val_reg_index = (first_dword >> 4) & 0xF;
            break;
        case DebugLogValueType::MemoryRelAddr:
            debug_log.mem_type = static_cast<MemoryAccessType>((first_dword >> 4) & 0xF);
            debug_log.rel_address = (static_cast<u64>(first_dword & 0xF) << 32) | GetNextDword();
            break;
        case DebugLogValueType::MemoryOfsReg:
            debug_log.mem_type = static_cast<MemoryAccessType>((first_dword >> 4) & 0xF);
            debug_log.ofs_reg_index = first_dword & 0xF;
            break;
        case DebugLogValueType::RegisterRelAddr:
            debug_log.addr_reg_index = (first_dword >> 4) & 0xF;
            debug_log.rel_address = (static_cast<u64>(first_dword & 0xF) << 32) | GetNextDword();
            break;
        case DebugLogValueType::RegisterOfsReg:
            debug_log.addr_reg_index = (first_dword >> 4) & 0xF;
            debug_log.ofs_reg_index = first_dword & 0xF;
            break;
        }
        opcode.opcode = debug_log;
    } break;
    case CheatVmOpcodeType::ExtendedWidth:
    case CheatVmOpcodeType::DoubleExtendedWidth:
    default:
        // Unrecognized instruction cannot be decoded.
        valid = false;
        opcode.opcode = UnrecognizedInstruction{opcode_type};
        break;
    }

    // End decoding.
    return valid;
}

void DmntCheatVm::SkipConditionalBlock() {
    if (condition_depth > 0) {
        // We want to continue until we're out of the current block.
        const std::size_t desired_depth = condition_depth - 1;

        CheatVmOpcode skip_opcode{};
        while (condition_depth > desired_depth && DecodeNextOpcode(skip_opcode)) {
            // Decode instructions until we see end of the current conditional block.
            // NOTE: This is broken in gateway's implementation.
            // Gateway currently checks for "0x2" instead of "0x20000000"
            // In addition, they do a linear scan instead of correctly decoding opcodes.
            // This causes issues if "0x2" appears as an immediate in the conditional block...

            // We also support nesting of conditional blocks, and Gateway does not.
            if (skip_opcode.begin_conditional_block) {
                condition_depth++;
            } else if (std::holds_alternative<EndConditionalOpcode>(skip_opcode.opcode)) {
                condition_depth--;
            }
        }
    } else {
        // Skipping, but condition_depth = 0.
        // This is an error condition.
        // However, I don't actually believe it is possible for this to happen.
        // I guess we'll throw a fatal error here, so as to encourage me to fix the VM
        // in the event that someone triggers it? I don't know how you'd do that.
        UNREACHABLE_MSG("Invalid condition depth in DMNT Cheat VM");
    }
}

u64 DmntCheatVm::GetVmInt(VmInt value, u32 bit_width) {
    switch (bit_width) {
    case 1:
        return value.bit8;
    case 2:
        return value.bit16;
    case 4:
        return value.bit32;
    case 8:
        return value.bit64;
    default:
        // Invalid bit width -> return 0.
        return 0;
    }
}

u64 DmntCheatVm::GetCheatProcessAddress(const CheatProcessMetadata& metadata,
                                        MemoryAccessType mem_type, u64 rel_address) {
    switch (mem_type) {
    case MemoryAccessType::MainNso:
    default:
        return metadata.main_nso_extents.base + rel_address;
    case MemoryAccessType::Heap:
        return metadata.heap_extents.base + rel_address;
    }
}

void DmntCheatVm::ResetState() {
    registers.fill(0);
    saved_values.fill(0);
    loop_tops.fill(0);
    static_registers.fill(0);
    instruction_ptr = 0;
    condition_depth = 0;
    decode_success = true;
}

bool DmntCheatVm::LoadProgram(const std::vector<CheatEntry>& entries) {
    // Reset opcode count.
    num_opcodes = 0;

    for (std::size_t i = 0; i < entries.size(); i++) {
        if (entries[i].enabled) {
            // Bounds check.
            if (entries[i].definition.num_opcodes + num_opcodes > MaximumProgramOpcodeCount) {
                num_opcodes = 0;
                return false;
            }

            for (std::size_t n = 0; n < entries[i].definition.num_opcodes; n++) {
                program[num_opcodes++] = entries[i].definition.opcodes[n];
            }
        }
    }

    return true;
}

void DmntCheatVm::Execute(const CheatProcessMetadata& metadata) {
    CheatVmOpcode cur_opcode{};

    // Get Keys down.
    u64 kDown = callbacks->HidKeysDown();

    callbacks->CommandLog("Started VM execution.");
    callbacks->CommandLog(fmt::format("Main NSO:  {:012X}", metadata.main_nso_extents.base));
    callbacks->CommandLog(fmt::format("Heap:      {:012X}", metadata.main_nso_extents.base));
    callbacks->CommandLog(fmt::format("Keys Down: {:08X}", static_cast<u32>(kDown & 0x0FFFFFFF)));

    // Clear VM state.
    ResetState();

    // Loop until program finishes.
    while (DecodeNextOpcode(cur_opcode)) {
        callbacks->CommandLog(
            fmt::format("Instruction Ptr: {:04X}", static_cast<u32>(instruction_ptr)));

        for (std::size_t i = 0; i < NumRegisters; i++) {
            callbacks->CommandLog(fmt::format("Registers[{:02X}]: {:016X}", i, registers[i]));
        }

        for (std::size_t i = 0; i < NumRegisters; i++) {
            callbacks->CommandLog(fmt::format("SavedRegs[{:02X}]: {:016X}", i, saved_values[i]));
        }
        LogOpcode(cur_opcode);

        // Increment conditional depth, if relevant.
        if (cur_opcode.begin_conditional_block) {
            condition_depth++;
        }

        if (auto store_static = std::get_if<StoreStaticOpcode>(&cur_opcode.opcode)) {
            // Calculate address, write value to memory.
            u64 dst_address = GetCheatProcessAddress(metadata, store_static->mem_type,
                                                     store_static->rel_address +
                                                         registers[store_static->offset_register]);
            u64 dst_value = GetVmInt(store_static->value, store_static->bit_width);
            switch (store_static->bit_width) {
            case 1:
            case 2:
            case 4:
            case 8:
                callbacks->MemoryWrite(dst_address, &dst_value, store_static->bit_width);
                break;
            }
        } else if (auto begin_cond = std::get_if<BeginConditionalOpcode>(&cur_opcode.opcode)) {
            // Read value from memory.
            u64 src_address =
                GetCheatProcessAddress(metadata, begin_cond->mem_type, begin_cond->rel_address);
            u64 src_value = 0;
            switch (store_static->bit_width) {
            case 1:
            case 2:
            case 4:
            case 8:
                callbacks->MemoryRead(src_address, &src_value, begin_cond->bit_width);
                break;
            }
            // Check against condition.
            u64 cond_value = GetVmInt(begin_cond->value, begin_cond->bit_width);
            bool cond_met = false;
            switch (begin_cond->cond_type) {
            case ConditionalComparisonType::GT:
                cond_met = src_value > cond_value;
                break;
            case ConditionalComparisonType::GE:
                cond_met = src_value >= cond_value;
                break;
            case ConditionalComparisonType::LT:
                cond_met = src_value < cond_value;
                break;
            case ConditionalComparisonType::LE:
                cond_met = src_value <= cond_value;
                break;
            case ConditionalComparisonType::EQ:
                cond_met = src_value == cond_value;
                break;
            case ConditionalComparisonType::NE:
                cond_met = src_value != cond_value;
                break;
            }
            // Skip conditional block if condition not met.
            if (!cond_met) {
                SkipConditionalBlock();
            }
        } else if (std::holds_alternative<EndConditionalOpcode>(cur_opcode.opcode)) {
            // Decrement the condition depth.
            // We will assume, graciously, that mismatched conditional block ends are a nop.
            if (condition_depth > 0) {
                condition_depth--;
            }
        } else if (auto ctrl_loop = std::get_if<ControlLoopOpcode>(&cur_opcode.opcode)) {
            if (ctrl_loop->start_loop) {
                // Start a loop.
                registers[ctrl_loop->reg_index] = ctrl_loop->num_iters;
                loop_tops[ctrl_loop->reg_index] = instruction_ptr;
            } else {
                // End a loop.
                registers[ctrl_loop->reg_index]--;
                if (registers[ctrl_loop->reg_index] != 0) {
                    instruction_ptr = loop_tops[ctrl_loop->reg_index];
                }
            }
        } else if (auto ldr_static = std::get_if<LoadRegisterStaticOpcode>(&cur_opcode.opcode)) {
            // Set a register to a static value.
            registers[ldr_static->reg_index] = ldr_static->value;
        } else if (auto ldr_memory = std::get_if<LoadRegisterMemoryOpcode>(&cur_opcode.opcode)) {
            // Choose source address.
            u64 src_address;
            if (ldr_memory->load_from_reg) {
                src_address = registers[ldr_memory->reg_index] + ldr_memory->rel_address;
            } else {
                src_address =
                    GetCheatProcessAddress(metadata, ldr_memory->mem_type, ldr_memory->rel_address);
            }
            // Read into register. Gateway only reads on valid bitwidth.
            switch (ldr_memory->bit_width) {
            case 1:
            case 2:
            case 4:
            case 8:
                callbacks->MemoryRead(src_address, &registers[ldr_memory->reg_index],
                                      ldr_memory->bit_width);
                break;
            }
        } else if (auto str_static = std::get_if<StoreStaticToAddressOpcode>(&cur_opcode.opcode)) {
            // Calculate address.
            u64 dst_address = registers[str_static->reg_index];
            u64 dst_value = str_static->value;
            if (str_static->add_offset_reg) {
                dst_address += registers[str_static->offset_reg_index];
            }
            // Write value to memory. Gateway only writes on valid bitwidth.
            switch (str_static->bit_width) {
            case 1:
            case 2:
            case 4:
            case 8:
                callbacks->MemoryWrite(dst_address, &dst_value, str_static->bit_width);
                break;
            }
            // Increment register if relevant.
            if (str_static->increment_reg) {
                registers[str_static->reg_index] += str_static->bit_width;
            }
        } else if (auto perform_math_static =
                       std::get_if<PerformArithmeticStaticOpcode>(&cur_opcode.opcode)) {
            // Do requested math.
            switch (perform_math_static->math_type) {
            case RegisterArithmeticType::Addition:
                registers[perform_math_static->reg_index] +=
                    static_cast<u64>(perform_math_static->value);
                break;
            case RegisterArithmeticType::Subtraction:
                registers[perform_math_static->reg_index] -=
                    static_cast<u64>(perform_math_static->value);
                break;
            case RegisterArithmeticType::Multiplication:
                registers[perform_math_static->reg_index] *=
                    static_cast<u64>(perform_math_static->value);
                break;
            case RegisterArithmeticType::LeftShift:
                registers[perform_math_static->reg_index] <<=
                    static_cast<u64>(perform_math_static->value);
                break;
            case RegisterArithmeticType::RightShift:
                registers[perform_math_static->reg_index] >>=
                    static_cast<u64>(perform_math_static->value);
                break;
            default:
                // Do not handle extensions here.
                break;
            }
            // Apply bit width.
            switch (perform_math_static->bit_width) {
            case 1:
                registers[perform_math_static->reg_index] =
                    static_cast<u8>(registers[perform_math_static->reg_index]);
                break;
            case 2:
                registers[perform_math_static->reg_index] =
                    static_cast<u16>(registers[perform_math_static->reg_index]);
                break;
            case 4:
                registers[perform_math_static->reg_index] =
                    static_cast<u32>(registers[perform_math_static->reg_index]);
                break;
            case 8:
                registers[perform_math_static->reg_index] =
                    static_cast<u64>(registers[perform_math_static->reg_index]);
                break;
            }
        } else if (auto begin_keypress_cond =
                       std::get_if<BeginKeypressConditionalOpcode>(&cur_opcode.opcode)) {
            // Check for keypress.
            if ((begin_keypress_cond->key_mask & kDown) != begin_keypress_cond->key_mask) {
                // Keys not pressed. Skip conditional block.
                SkipConditionalBlock();
            }
        } else if (auto perform_math_reg =
                       std::get_if<PerformArithmeticRegisterOpcode>(&cur_opcode.opcode)) {
            const u64 operand_1_value = registers[perform_math_reg->src_reg_1_index];
            const u64 operand_2_value =
                perform_math_reg->has_immediate
                    ? GetVmInt(perform_math_reg->value, perform_math_reg->bit_width)
                    : registers[perform_math_reg->src_reg_2_index];

            u64 res_val = 0;
            // Do requested math.
            switch (perform_math_reg->math_type) {
            case RegisterArithmeticType::Addition:
                res_val = operand_1_value + operand_2_value;
                break;
            case RegisterArithmeticType::Subtraction:
                res_val = operand_1_value - operand_2_value;
                break;
            case RegisterArithmeticType::Multiplication:
                res_val = operand_1_value * operand_2_value;
                break;
            case RegisterArithmeticType::LeftShift:
                res_val = operand_1_value << operand_2_value;
                break;
            case RegisterArithmeticType::RightShift:
                res_val = operand_1_value >> operand_2_value;
                break;
            case RegisterArithmeticType::LogicalAnd:
                res_val = operand_1_value & operand_2_value;
                break;
            case RegisterArithmeticType::LogicalOr:
                res_val = operand_1_value | operand_2_value;
                break;
            case RegisterArithmeticType::LogicalNot:
                res_val = ~operand_1_value;
                break;
            case RegisterArithmeticType::LogicalXor:
                res_val = operand_1_value ^ operand_2_value;
                break;
            case RegisterArithmeticType::None:
                res_val = operand_1_value;
                break;
            }

            // Apply bit width.
            switch (perform_math_reg->bit_width) {
            case 1:
                res_val = static_cast<u8>(res_val);
                break;
            case 2:
                res_val = static_cast<u16>(res_val);
                break;
            case 4:
                res_val = static_cast<u32>(res_val);
                break;
            case 8:
                res_val = static_cast<u64>(res_val);
                break;
            }

            // Save to register.
            registers[perform_math_reg->dst_reg_index] = res_val;
        } else if (auto str_register =
                       std::get_if<StoreRegisterToAddressOpcode>(&cur_opcode.opcode)) {
            // Calculate address.
            u64 dst_value = registers[str_register->str_reg_index];
            u64 dst_address = registers[str_register->addr_reg_index];
            switch (str_register->ofs_type) {
            case StoreRegisterOffsetType::None:
                // Nothing more to do
                break;
            case StoreRegisterOffsetType::Reg:
                dst_address += registers[str_register->ofs_reg_index];
                break;
            case StoreRegisterOffsetType::Imm:
                dst_address += str_register->rel_address;
                break;
            case StoreRegisterOffsetType::MemReg:
                dst_address = GetCheatProcessAddress(metadata, str_register->mem_type,
                                                     registers[str_register->addr_reg_index]);
                break;
            case StoreRegisterOffsetType::MemImm:
                dst_address = GetCheatProcessAddress(metadata, str_register->mem_type,
                                                     str_register->rel_address);
                break;
            case StoreRegisterOffsetType::MemImmReg:
                dst_address = GetCheatProcessAddress(metadata, str_register->mem_type,
                                                     registers[str_register->addr_reg_index] +
                                                         str_register->rel_address);
                break;
            }

            // Write value to memory. Write only on valid bitwidth.
            switch (str_register->bit_width) {
            case 1:
            case 2:
            case 4:
            case 8:
                callbacks->MemoryWrite(dst_address, &dst_value, str_register->bit_width);
                break;
            }

            // Increment register if relevant.
            if (str_register->increment_reg) {
                registers[str_register->addr_reg_index] += str_register->bit_width;
            }
        } else if (auto begin_reg_cond =
                       std::get_if<BeginRegisterConditionalOpcode>(&cur_opcode.opcode)) {
            // Get value from register.
            u64 src_value = 0;
            switch (begin_reg_cond->bit_width) {
            case 1:
                src_value = static_cast<u8>(registers[begin_reg_cond->val_reg_index] & 0xFFul);
                break;
            case 2:
                src_value = static_cast<u16>(registers[begin_reg_cond->val_reg_index] & 0xFFFFul);
                break;
            case 4:
                src_value =
                    static_cast<u32>(registers[begin_reg_cond->val_reg_index] & 0xFFFFFFFFul);
                break;
            case 8:
                src_value = static_cast<u64>(registers[begin_reg_cond->val_reg_index] &
                                             0xFFFFFFFFFFFFFFFFul);
                break;
            }

            // Read value from memory.
            u64 cond_value = 0;
            if (begin_reg_cond->comp_type == CompareRegisterValueType::StaticValue) {
                cond_value = GetVmInt(begin_reg_cond->value, begin_reg_cond->bit_width);
            } else if (begin_reg_cond->comp_type == CompareRegisterValueType::OtherRegister) {
                switch (begin_reg_cond->bit_width) {
                case 1:
                    cond_value =
                        static_cast<u8>(registers[begin_reg_cond->other_reg_index] & 0xFFul);
                    break;
                case 2:
                    cond_value =
                        static_cast<u16>(registers[begin_reg_cond->other_reg_index] & 0xFFFFul);
                    break;
                case 4:
                    cond_value =
                        static_cast<u32>(registers[begin_reg_cond->other_reg_index] & 0xFFFFFFFFul);
                    break;
                case 8:
                    cond_value = static_cast<u64>(registers[begin_reg_cond->other_reg_index] &
                                                  0xFFFFFFFFFFFFFFFFul);
                    break;
                }
            } else {
                u64 cond_address = 0;
                switch (begin_reg_cond->comp_type) {
                case CompareRegisterValueType::MemoryRelAddr:
                    cond_address = GetCheatProcessAddress(metadata, begin_reg_cond->mem_type,
                                                          begin_reg_cond->rel_address);
                    break;
                case CompareRegisterValueType::MemoryOfsReg:
                    cond_address = GetCheatProcessAddress(metadata, begin_reg_cond->mem_type,
                                                          registers[begin_reg_cond->ofs_reg_index]);
                    break;
                case CompareRegisterValueType::RegisterRelAddr:
                    cond_address =
                        registers[begin_reg_cond->addr_reg_index] + begin_reg_cond->rel_address;
                    break;
                case CompareRegisterValueType::RegisterOfsReg:
                    cond_address = registers[begin_reg_cond->addr_reg_index] +
                                   registers[begin_reg_cond->ofs_reg_index];
                    break;
                default:
                    break;
                }
                switch (begin_reg_cond->bit_width) {
                case 1:
                case 2:
                case 4:
                case 8:
                    callbacks->MemoryRead(cond_address, &cond_value, begin_reg_cond->bit_width);
                    break;
                }
            }

            // Check against condition.
            bool cond_met = false;
            switch (begin_reg_cond->cond_type) {
            case ConditionalComparisonType::GT:
                cond_met = src_value > cond_value;
                break;
            case ConditionalComparisonType::GE:
                cond_met = src_value >= cond_value;
                break;
            case ConditionalComparisonType::LT:
                cond_met = src_value < cond_value;
                break;
            case ConditionalComparisonType::LE:
                cond_met = src_value <= cond_value;
                break;
            case ConditionalComparisonType::EQ:
                cond_met = src_value == cond_value;
                break;
            case ConditionalComparisonType::NE:
                cond_met = src_value != cond_value;
                break;
            }

            // Skip conditional block if condition not met.
            if (!cond_met) {
                SkipConditionalBlock();
            }
        } else if (auto save_restore_reg =
                       std::get_if<SaveRestoreRegisterOpcode>(&cur_opcode.opcode)) {
            // Save or restore a register.
            switch (save_restore_reg->op_type) {
            case SaveRestoreRegisterOpType::ClearRegs:
                registers[save_restore_reg->dst_index] = 0ul;
                break;
            case SaveRestoreRegisterOpType::ClearSaved:
                saved_values[save_restore_reg->dst_index] = 0ul;
                break;
            case SaveRestoreRegisterOpType::Save:
                saved_values[save_restore_reg->dst_index] = registers[save_restore_reg->src_index];
                break;
            case SaveRestoreRegisterOpType::Restore:
            default:
                registers[save_restore_reg->dst_index] = saved_values[save_restore_reg->src_index];
                break;
            }
        } else if (auto save_restore_regmask =
                       std::get_if<SaveRestoreRegisterMaskOpcode>(&cur_opcode.opcode)) {
            // Save or restore register mask.
            u64* src;
            u64* dst;
            switch (save_restore_regmask->op_type) {
            case SaveRestoreRegisterOpType::ClearSaved:
            case SaveRestoreRegisterOpType::Save:
                src = registers.data();
                dst = saved_values.data();
                break;
            case SaveRestoreRegisterOpType::ClearRegs:
            case SaveRestoreRegisterOpType::Restore:
            default:
                src = saved_values.data();
                dst = registers.data();
                break;
            }
            for (std::size_t i = 0; i < NumRegisters; i++) {
                if (save_restore_regmask->should_operate[i]) {
                    switch (save_restore_regmask->op_type) {
                    case SaveRestoreRegisterOpType::ClearSaved:
                    case SaveRestoreRegisterOpType::ClearRegs:
                        dst[i] = 0ul;
                        break;
                    case SaveRestoreRegisterOpType::Save:
                    case SaveRestoreRegisterOpType::Restore:
                    default:
                        dst[i] = src[i];
                        break;
                    }
                }
            }
        } else if (auto rw_static_reg =
                       std::get_if<ReadWriteStaticRegisterOpcode>(&cur_opcode.opcode)) {
            if (rw_static_reg->static_idx < NumReadableStaticRegisters) {
                // Load a register with a static register.
                registers[rw_static_reg->idx] = static_registers[rw_static_reg->static_idx];
            } else {
                // Store a register to a static register.
                static_registers[rw_static_reg->static_idx] = registers[rw_static_reg->idx];
            }
        } else if (auto debug_log = std::get_if<DebugLogOpcode>(&cur_opcode.opcode)) {
            // Read value from memory.
            u64 log_value = 0;
            if (debug_log->val_type == DebugLogValueType::RegisterValue) {
                switch (debug_log->bit_width) {
                case 1:
                    log_value = static_cast<u8>(registers[debug_log->val_reg_index] & 0xFFul);
                    break;
                case 2:
                    log_value = static_cast<u16>(registers[debug_log->val_reg_index] & 0xFFFFul);
                    break;
                case 4:
                    log_value =
                        static_cast<u32>(registers[debug_log->val_reg_index] & 0xFFFFFFFFul);
                    break;
                case 8:
                    log_value = static_cast<u64>(registers[debug_log->val_reg_index] &
                                                 0xFFFFFFFFFFFFFFFFul);
                    break;
                }
            } else {
                u64 val_address = 0;
                switch (debug_log->val_type) {
                case DebugLogValueType::MemoryRelAddr:
                    val_address = GetCheatProcessAddress(metadata, debug_log->mem_type,
                                                         debug_log->rel_address);
                    break;
                case DebugLogValueType::MemoryOfsReg:
                    val_address = GetCheatProcessAddress(metadata, debug_log->mem_type,
                                                         registers[debug_log->ofs_reg_index]);
                    break;
                case DebugLogValueType::RegisterRelAddr:
                    val_address = registers[debug_log->addr_reg_index] + debug_log->rel_address;
                    break;
                case DebugLogValueType::RegisterOfsReg:
                    val_address =
                        registers[debug_log->addr_reg_index] + registers[debug_log->ofs_reg_index];
                    break;
                default:
                    break;
                }
                switch (debug_log->bit_width) {
                case 1:
                case 2:
                case 4:
                case 8:
                    callbacks->MemoryRead(val_address, &log_value, debug_log->bit_width);
                    break;
                }
            }

            // Log value.
            DebugLog(debug_log->log_id, log_value);
        }
    }
}

} // namespace Core::Memory
