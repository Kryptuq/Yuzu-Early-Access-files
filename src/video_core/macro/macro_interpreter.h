// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once
#include <array>
#include <optional>
#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "video_core/macro/macro.h"

namespace Tegra {
namespace Engines {
class Maxwell3D;
}

class MacroInterpreter final : public MacroEngine {
public:
    explicit MacroInterpreter(Engines::Maxwell3D& maxwell3d_);

protected:
    std::unique_ptr<CachedMacro> Compile(const std::vector<u32>& code) override;

private:
    Engines::Maxwell3D& maxwell3d;
};

class MacroInterpreterImpl : public CachedMacro {
public:
    explicit MacroInterpreterImpl(Engines::Maxwell3D& maxwell3d_, const std::vector<u32>& code_);
    void Execute(const std::vector<u32>& params, u32 method) override;

private:
    /// Resets the execution engine state, zeroing registers, etc.
    void Reset();

    /**
     * Executes a single macro instruction located at the current program counter. Returns whether
     * the interpreter should keep running.
     *
     * @param is_delay_slot Whether the current step is being executed due to a delay slot in a
     *                      previous instruction.
     */
    bool Step(bool is_delay_slot);

    /// Calculates the result of an ALU operation. src_a OP src_b;
    u32 GetALUResult(Macro::ALUOperation operation, u32 src_a, u32 src_b);

    /// Performs the result operation on the input result and stores it in the specified register
    /// (if necessary).
    void ProcessResult(Macro::ResultOperation operation, u32 reg, u32 result);

    /// Evaluates the branch condition and returns whether the branch should be taken or not.
    bool EvaluateBranchCondition(Macro::BranchCondition cond, u32 value) const;

    /// Reads an opcode at the current program counter location.
    Macro::Opcode GetOpcode() const;

    /// Returns the specified register's value. Register 0 is hardcoded to always return 0.
    u32 GetRegister(u32 register_id) const;

    /// Sets the register to the input value.
    void SetRegister(u32 register_id, u32 value);

    /// Sets the method address to use for the next Send instruction.
    void SetMethodAddress(u32 address);

    /// Calls a GPU Engine method with the input parameter.
    void Send(u32 value);

    /// Reads a GPU register located at the method address.
    u32 Read(u32 method) const;

    /// Returns the next parameter in the parameter queue.
    u32 FetchParameter();

    Engines::Maxwell3D& maxwell3d;

    /// Current program counter
    u32 pc;
    /// Program counter to execute at after the delay slot is executed.
    std::optional<u32> delayed_pc;

    /// General purpose macro registers.
    std::array<u32, Macro::NUM_MACRO_REGISTERS> registers = {};

    /// Method address to use for the next Send instruction.
    Macro::MethodAddress method_address = {};

    /// Input parameters of the current macro.
    std::unique_ptr<u32[]> parameters;
    std::size_t num_parameters = 0;
    std::size_t parameters_capacity = 0;
    /// Index of the next parameter that will be fetched by the 'parm' instruction.
    u32 next_parameter_index = 0;

    bool carry_flag = false;
    const std::vector<u32>& code;
};

} // namespace Tegra
