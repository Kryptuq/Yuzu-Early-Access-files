// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <bitset>
#include <xbyak/xbyak.h>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/x64/xbyak_abi.h"
#include "video_core/macro/macro.h"

namespace Tegra {

namespace Engines {
class Maxwell3D;
}

/// MAX_CODE_SIZE is arbitrarily chosen based on current booting games
constexpr size_t MAX_CODE_SIZE = 0x10000;

class MacroJITx64 final : public MacroEngine {
public:
    explicit MacroJITx64(Engines::Maxwell3D& maxwell3d_);

protected:
    std::unique_ptr<CachedMacro> Compile(const std::vector<u32>& code) override;

private:
    Engines::Maxwell3D& maxwell3d;
};

class MacroJITx64Impl : public Xbyak::CodeGenerator, public CachedMacro {
public:
    explicit MacroJITx64Impl(Engines::Maxwell3D& maxwell3d_, const std::vector<u32>& code_);
    ~MacroJITx64Impl();

    void Execute(const std::vector<u32>& parameters, u32 method) override;

    void Compile_ALU(Macro::Opcode opcode);
    void Compile_AddImmediate(Macro::Opcode opcode);
    void Compile_ExtractInsert(Macro::Opcode opcode);
    void Compile_ExtractShiftLeftImmediate(Macro::Opcode opcode);
    void Compile_ExtractShiftLeftRegister(Macro::Opcode opcode);
    void Compile_Read(Macro::Opcode opcode);
    void Compile_Branch(Macro::Opcode opcode);

private:
    void Optimizer_ScanFlags();

    void Compile();
    bool Compile_NextInstruction();

    Xbyak::Reg32 Compile_FetchParameter();
    Xbyak::Reg32 Compile_GetRegister(u32 index, Xbyak::Reg32 dst);

    void Compile_ProcessResult(Macro::ResultOperation operation, u32 reg);
    void Compile_Send(Xbyak::Reg32 value);

    Macro::Opcode GetOpCode() const;
    std::bitset<32> PersistentCallerSavedRegs() const;

    struct JITState {
        Engines::Maxwell3D* maxwell3d{};
        std::array<u32, Macro::NUM_MACRO_REGISTERS> registers{};
        u32 carry_flag{};
    };
    static_assert(offsetof(JITState, maxwell3d) == 0, "Maxwell3D is not at 0x0");
    using ProgramType = void (*)(JITState*, const u32*);

    struct OptimizerState {
        bool can_skip_carry{};
        bool has_delayed_pc{};
        bool zero_reg_skip{};
        bool skip_dummy_addimmediate{};
        bool optimize_for_method_move{};
        bool enable_asserts{};
    };
    OptimizerState optimizer{};

    std::optional<Macro::Opcode> next_opcode{};
    ProgramType program{nullptr};

    std::array<Xbyak::Label, MAX_CODE_SIZE> labels;
    std::array<Xbyak::Label, MAX_CODE_SIZE> delay_skip;
    Xbyak::Label end_of_code{};

    bool is_delay_slot{};
    u32 pc{};
    std::optional<u32> delayed_pc;

    const std::vector<u32>& code;
    Engines::Maxwell3D& maxwell3d;
};

} // namespace Tegra
