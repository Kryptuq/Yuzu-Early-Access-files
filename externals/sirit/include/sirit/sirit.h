/* This file is part of the sirit project.
 * Copyright (c) 2019 sirit
 * This software may be used and distributed according to the terms of the
 * 3-Clause BSD License
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>
#include <spirv/unified1/spirv.hpp11>

namespace Sirit {

constexpr std::uint32_t GENERATOR_MAGIC_NUMBER = 0;

class Op;
class Operand;

using Literal =
    std::variant<std::uint32_t, std::uint64_t, std::int32_t, std::int64_t, float, double>;
using Id = const Op*;

class Module {
public:
    explicit Module(std::uint32_t version = spv::Version);
    ~Module();

    /**
     * Assembles current module into a SPIR-V stream.
     * It can be called multiple times but it's recommended to copy code
     * externally.
     * @return A stream of bytes representing a SPIR-V module.
     */
    std::vector<std::uint32_t> Assemble() const;

    /// Adds a SPIR-V extension.
    void AddExtension(std::string extension_name);

    /// Adds a module capability.
    void AddCapability(spv::Capability capability);

    /// Sets module memory model.
    void SetMemoryModel(spv::AddressingModel addressing_model_, spv::MemoryModel memory_model_);

    /// Adds an entry point.
    void AddEntryPoint(spv::ExecutionModel execution_model, Id entry_point, std::string name,
                       const std::vector<Id>& interfaces = {});

    /// Adds an entry point.
    template <typename... Ts>
    void AddEntryPoint(spv::ExecutionModel execution_model, Id entry_point, std::string name,
                       Ts&&... interfaces) {
        AddEntryPoint(execution_model, std::move(entry_point), name, {interfaces...});
    }

    /// Declare an execution mode for an entry point.
    void AddExecutionMode(Id entry_point, spv::ExecutionMode mode,
                          const std::vector<Literal>& literals = {});

    /// Declare an execution mode for an entry point.
    template <typename... Ts>
    void AddExecutionMode(Id entry_point, spv::ExecutionMode mode, Ts&&... literals) {
        AddExecutionMode(entry_point, mode, {literals...});
    }

    /**
     * Adds an existing label to the code
     * @param label Label to insert into code.
     * @return Returns label.
     */
    Id AddLabel(Id label);

    /**
     * Adds a label to the code
     * @return Returns the created label.
     */
    Id AddLabel() {
        return AddLabel(OpLabel());
    }

    /**
     * Adds a local variable to the code
     * @param variable Variable to insert into code.
     * @return Returns variable.
     */
    Id AddLocalVariable(Id label);

    /**
     * Adds a global variable
     * @param variable Global variable to add.
     * @return Returns variable.
     */
    Id AddGlobalVariable(Id variable);

    // Types

    /// Returns type void.
    Id TypeVoid();

    /// Returns type bool.
    Id TypeBool();

    /// Returns type integer.
    Id TypeInt(int width, bool is_signed);

    /// Returns type float.
    Id TypeFloat(int width);

    /// Returns type vector.
    Id TypeVector(Id component_type, int component_count);

    /// Returns type matrix.
    Id TypeMatrix(Id column_type, int column_count);

    /// Returns type image.
    Id TypeImage(Id sampled_type, spv::Dim dim, int depth, bool arrayed, bool ms, int sampled,
                 spv::ImageFormat image_format,
                 std::optional<spv::AccessQualifier> access_qualifier = {});

    /// Returns type sampler.
    Id TypeSampler();

    /// Returns type sampled image.
    Id TypeSampledImage(Id image_type);

    /// Returns type array.
    Id TypeArray(Id element_type, Id length);

    /// Returns type runtime array.
    Id TypeRuntimeArray(Id element_type);

    /// Returns type struct.
    Id TypeStruct(const std::vector<Id>& members = {});

    /// Returns type struct.
    template <typename... Ts>
    Id TypeStruct(Ts&&... members) {
        return TypeStruct({members...});
    }

    /// Returns type opaque.
    Id TypeOpaque(std::string name);

    /// Returns type pointer.
    Id TypePointer(spv::StorageClass storage_class, Id type);

    /// Returns type function.
    Id TypeFunction(Id return_type, const std::vector<Id>& arguments = {});

    /// Returns type function.
    template <typename... Ts>
    Id TypeFunction(Id return_type, Ts&&... arguments) {
        return OpTypeFunction(return_type, {arguments...});
    }

    /// Returns type event.
    Id TypeEvent();

    /// Returns type device event.
    Id TypeDeviceEvent();

    /// Returns type reserve id.
    Id TypeReserveId();

    /// Returns type queue.
    Id TypeQueue();

    /// Returns type pipe.
    Id TypePipe(spv::AccessQualifier access_qualifier);

    // Constant

    /// Returns a true scalar constant.
    Id ConstantTrue(Id result_type);

    /// Returns a false scalar constant.
    Id ConstantFalse(Id result_type);

    /// Returns a numeric scalar constant.
    Id Constant(Id result_type, const Literal& literal);

    /// Returns a numeric scalar constant.
    Id ConstantComposite(Id result_type, const std::vector<Id>& constituents);

    /// Returns a numeric scalar constant.
    template <typename... Ts>
    Id ConstantComposite(Id result_type, Ts&&... constituents) {
        return ConstantComposite(result_type, {constituents...});
    }

    /// Returns a sampler constant.
    Id ConstantSampler(Id result_type, spv::SamplerAddressingMode addressing_mode, bool normalized,
                       spv::SamplerFilterMode filter_mode);

    /// Returns a null constant value.
    Id ConstantNull(Id result_type);

    // Function

    /// Declares a function.
    Id OpFunction(Id result_type, spv::FunctionControlMask function_control, Id function_type);

    /// Ends a function.
    Id OpFunctionEnd();

    /// Call a function.
    Id OpFunctionCall(Id result_type, Id function, const std::vector<Id>& arguments = {});

    /// Call a function.
    template <typename... Ts>
    Id OpFunctionCall(Id result_type, Id function, Ts&&... arguments) {
        return OpFunctionCall(result_type, function, {arguments...});
    }

    // Flow

    /// Declare a structured loop.
    Id OpLoopMerge(Id merge_block, Id continue_target, spv::LoopControlMask loop_control,
                   const std::vector<Id>& literals = {});

    /// Declare a structured loop.
    template <typename... Ts>
    Id OpLoopMerge(Id merge_block, Id continue_target, spv::LoopControlMask loop_control,
                   Ts&&... literals) {
        return OpLoopMerge(merge_block, continue_target, loop_control, {literals...});
    }

    /// Declare a structured selection.
    Id OpSelectionMerge(Id merge_block, spv::SelectionControlMask selection_control);

    /// The block label instruction: Any reference to a block is through this ref.
    Id OpLabel();

    /// The block label instruction: Any reference to a block is through this ref.
    Id OpLabel(std::string label_name) {
        return Name(OpLabel(), std::move(label_name));
    }

    /// Unconditional jump to label.
    Id OpBranch(Id target_label);

    /// If condition is true branch to true_label, otherwise branch to
    /// false_label.
    Id OpBranchConditional(Id condition, Id true_label, Id false_label,
                           std::uint32_t true_weight = 0, std::uint32_t false_weight = 0);

    /// Multi-way branch to one of the operand label.
    Id OpSwitch(Id selector, Id default_label, const std::vector<Literal>& literals,
                const std::vector<Id>& labels);

    /// Returns with no value from a function with void return type.
    Id OpReturn();

    /// Return a value from a function.
    Id OpReturnValue(Id value);

    /// Fragment-shader discard.
    Id OpKill();

    // Debug

    /// Assign a name string to a reference.
    /// @return target
    Id Name(Id target, std::string name);

    /// Assign a name string to a member of a structure type.
    /// @return type
    Id MemberName(Id type, std::uint32_t member, std::string name);

    /// Assign a Result <id> to a string for use by other debug instructions.
    Id String(std::string string);

    /// Add source-level location information
    Id OpLine(Id file, Literal line, Literal column);

    // Memory

    /// Allocate an object in memory, resulting in a copy to it.
    Id OpVariable(Id result_type, spv::StorageClass storage_class, Id initializer = nullptr);

    /// Form a pointer to a texel of an image. Use of such a pointer is limited to atomic operations.
    Id OpImageTexelPointer(Id result_type, Id image, Id coordinate, Id sample);

    /// Load through a pointer.
    Id OpLoad(Id result_type, Id pointer, std::optional<spv::MemoryAccessMask> memory_access = {});

    /// Store through a pointer.
    Id OpStore(Id pointer, Id object, std::optional<spv::MemoryAccessMask> memory_access = {});

    /// Create a pointer into a composite object that can be used with OpLoad and OpStore.
    Id OpAccessChain(Id result_type, Id base, const std::vector<Id>& indexes = {});

    /// Create a pointer into a composite object that can be used with OpLoad and OpStore.
    template <typename... Ts>
    Id OpAccessChain(Id result_type, Id base, Ts&&... indexes) {
        return OpAccessChain(result_type, base, {indexes...});
    }

    /// Extract a single, dynamically selected, component of a vector.
    Id OpVectorExtractDynamic(Id result_type, Id vector, Id index);

    /// Make a copy of a vector, with a single, variably selected, component modified.
    Id OpVectorInsertDynamic(Id result_type, Id vector, Id component, Id index);

    /// Make a copy of a composite object, while modifying one part of it.
    Id OpCompositeInsert(Id result_type, Id object, Id composite,
                         const std::vector<Literal>& indexes = {});

    /// Make a copy of a composite object, while modifying one part of it.
    template <typename... Ts>
    Id OpCompositeInsert(Id result_type, Id object, Id composite, Ts&&... indexes) {
        return OpCompositeInsert(result_type, object, composite, {indexes...});
    }

    /// Extract a part of a composite object.
    Id OpCompositeExtract(Id result_type, Id composite, const std::vector<Literal>& indexes = {});

    /// Extract a part of a composite object.
    template <typename... Ts>
    Id OpCompositeExtract(Id result_type, Id composite, Ts&&... indexes) {
        return OpCompositeExtract(result_type, composite, {indexes...});
    }

    /// Construct a new composite object from a set of constituent objects that will fully form it.
    Id OpCompositeConstruct(Id result_type, const std::vector<Id>& ids);

    /// Construct a new composite object from a set of constituent objects that will fully form it.
    template <typename... Ts>
    Id OpCompositeConstruct(Id result_type, Ts&&... ids) {
        return OpCompositeConstruct(result_type, {ids...});
    }

    // Annotation

    /// Add a decoration to target.
    Id Decorate(Id target, spv::Decoration decoration, const std::vector<Literal>& literals = {});

    /// Add a decoration to target.
    template <typename... Ts>
    Id Decorate(Id target, spv::Decoration decoration, Ts&&... literals) {
        return Decorate(target, decoration, {literals...});
    }

    Id MemberDecorate(Id structure_type, Literal member, spv::Decoration decoration,
                      const std::vector<Literal>& literals = {});

    template <typename... Ts>
    Id MemberDecorate(Id structure_type, Literal member, spv::Decoration decoration,
                      Ts&&... literals) {
        return MemberDecorate(structure_type, member, decoration, {literals...});
    }

    // Misc

    /// Make an intermediate object whose value is undefined.
    Id OpUndef(Id result_type);

    /// Emits the current values of all output variables to the current output primitive.
    Id OpEmitVertex();

    /// Finish the current primitive and start a new one. No vertex is emitted.
    Id OpEndPrimitive();

    // Barrier

    /// Wait for other invocations of this module to reach the current point of execution.
    Id OpControlBarrier(Id execution, Id memory, Id semantics);

    /// Control the order that memory accesses are observed.
    Id OpMemoryBarrier(Id scope, Id semantics);

    // Logical

    /// Result is true if any component of Vector is true, otherwise result is false.
    Id OpAny(Id result_type, Id vector);

    /// Result is true if all components of Vector are true, otherwise result is false.
    Id OpAll(Id result_type, Id vector);

    /// Result is true if x is an IEEE NaN, otherwise result is false.
    Id OpIsNan(Id result_type, Id operand);

    /// Result is true if x is an IEEE Inf, otherwise result is false.
    Id OpIsInf(Id result_type, Id operand);

    /// Result is true if Operand 1 and Operand 2 have the same value. Result is false if Operand 1
    /// and Operand 2 have different values.
    Id OpLogicalEqual(Id result_type, Id operand_1, Id operand_2);

    /// Result is true if Operand 1 and Operand 2 have different values. Result is false if Operand
    /// 1 and Operand 2 have the same value.
    Id OpLogicalNotEqual(Id result_type, Id operand_1, Id operand_2);

    /// Result is true if either Operand 1 or Operand 2 is true. Result is false if both Operand 1
    /// and Operand 2 are false.
    Id OpLogicalOr(Id result_type, Id operand_1, Id operand_2);

    /// Result is true if both Operand 1 and Operand 2 are true. Result is false if either Operand 1
    /// or Operand 2 are false.
    Id OpLogicalAnd(Id result_type, Id operand_1, Id operand_2);

    /// Result is true if Operand is false. Result is false if Operand is true.
    Id OpLogicalNot(Id result_type, Id operand);

    /// Select components from two objects.
    Id OpSelect(Id result_type, Id condition, Id operand_1, Id operand_2);

    /// Integer comparison for equality.
    Id OpIEqual(Id result_type, Id operand_1, Id operand_2);

    /// Integer comparison for inequality.
    Id OpINotEqual(Id result_type, Id operand_1, Id operand_2);

    /// Unsigned-integer comparison if Operand 1 is greater than Operand 2.
    Id OpUGreaterThan(Id result_type, Id operand_1, Id operand_2);

    /// Signed-integer comparison if Operand 1 is greater than Operand 2.
    Id OpSGreaterThan(Id result_type, Id operand_1, Id operand_2);

    /// Unsigned-integer comparison if Operand 1 is greater than or equal to Operand 2.
    Id OpUGreaterThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Signed-integer comparison if Operand 1 is greater than or equal to Operand 2.
    Id OpSGreaterThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Unsigned-integer comparison if Operand 1 is less than Operand 2.
    Id OpULessThan(Id result_type, Id operand_1, Id operand_2);

    /// Signed-integer comparison if Operand 1 is less than Operand 2.
    Id OpSLessThan(Id result_type, Id operand_1, Id operand_2);

    /// Unsigned-integer comparison if Operand 1 is less than or equal to Operand 2.
    Id OpULessThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Signed-integer comparison if Operand 1 is less than or equal to Operand 2.
    Id OpSLessThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison for being ordered and equal.
    Id OpFOrdEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison for being unordered or equal.
    Id OpFUnordEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison for being ordered and not equal.
    Id OpFOrdNotEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison for being unordered or not equal.
    Id OpFUnordNotEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are ordered and Operand 1 is less than Operand 2.
    Id OpFOrdLessThan(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are unordered or Operand 1 is less than Operand 2.
    Id OpFUnordLessThan(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are ordered and Operand 1 is greater than Operand 2.
    Id OpFOrdGreaterThan(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are unordered or Operand 1 is greater than Operand 2.
    Id OpFUnordGreaterThan(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are ordered and Operand 1 is less than or equal to
    /// Operand 2.
    Id OpFOrdLessThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are unordered or Operand 1 is less than or equal to
    /// Operand 2.
    Id OpFUnordLessThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are ordered and Operand 1 is greater than or equal to
    /// Operand 2.
    Id OpFOrdGreaterThanEqual(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point comparison if operands are unordered or Operand 1 is greater than or equal to
    /// Operand 2.
    Id OpFUnordGreaterThanEqual(Id result_type, Id operand_1, Id operand_2);

    // Conversion

    /// Convert (value preserving) from floating point to unsigned integer, with round toward 0.0.
    Id OpConvertFToU(Id result_type, Id operand);

    /// Convert (value preserving) from floating point to signed integer, with round toward 0.0.
    Id OpConvertFToS(Id result_type, Id operand);

    /// Convert (value preserving) from signed integer to floating point.
    Id OpConvertSToF(Id result_type, Id operand);

    /// Convert (value preserving) from unsigned integer to floating point.
    Id OpConvertUToF(Id result_type, Id operand);

    /// Convert (value preserving) unsigned width. This is either a truncate or a zero extend.
    Id OpUConvert(Id result_type, Id operand);

    /// Convert (value preserving) signed width. This is either a truncate or a sign extend.
    Id OpSConvert(Id result_type, Id operand);

    /// Convert (value preserving) floating-point width.
    Id OpFConvert(Id result_type, Id operand);

    /// Quantize a floating-point value to what is expressible by a 16-bit floating-point value.
    Id OpQuantizeToF16(Id result_type, Id operand);

    /// Bit pattern-preserving type conversion.
    Id OpBitcast(Id result_type, Id operand);

    // Bit

    /// Shift the bits in Base right by the number of bits specified in Shift.
    /// The most-significant bits will be zero filled.
    Id OpShiftRightLogical(Id result_type, Id base, Id shift);

    /// Shift the bits in Base right by the number of bits specified in Shift.
    /// The most-significant bits will be filled with the sign bit from Base.
    Id OpShiftRightArithmetic(Id result_type, Id base, Id shift);

    /// Shift the bits in Base left by the number of bits specified in Shift.
    /// The least-significant bits will be zero filled.
    Id OpShiftLeftLogical(Id result_type, Id base, Id shift);

    /// Does a bitwise Or between operands 1 and 2.
    Id OpBitwiseOr(Id result_type, Id operand_1, Id operand_2);

    /// Does a bitwise Xor between operands 1 and 2.
    Id OpBitwiseXor(Id result_type, Id operand_1, Id operand_2);

    /// Result is 1 if both Operand 1 and Operand 2 are 1. Result is 0 if either
    /// Operand 1 or Operand 2 are 0.
    Id OpBitwiseAnd(Id result_type, Id operand_1, Id operand_2);

    /// Does a bitwise Not on the operand.
    Id OpNot(Id result_type, Id operand);

    /// Make a copy of an object, with a modified bit field that comes from another object.
    Id OpBitFieldInsert(Id result_type, Id base, Id insert, Id offset, Id count);

    /// Extract a bit field from an object, with sign extension.
    Id OpBitFieldSExtract(Id result_type, Id base, Id offset, Id count);

    /// Extract a bit field from an object, without sign extension.
    Id OpBitFieldUExtract(Id result_type, Id base, Id offset, Id count);

    /// Reverse the bits in an object.
    Id OpBitReverse(Id result_type, Id base);

    /// Count the number of set bits in an object.
    Id OpBitCount(Id result_type, Id base);

    // Arithmetic

    /// Floating-point subtract of Operand from zero.
    Id OpSNegate(Id result_type, Id operand);

    /// Floating-point subtract of Operand from zero.
    Id OpFNegate(Id result_type, Id operand);

    /// Integer addition of Operand 1 and Operand 2.
    Id OpIAdd(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point addition of Operand 1 and Operand 2.
    Id OpFAdd(Id result_type, Id operand_1, Id operand_2);

    /// Integer substraction of Operand 1 and Operand 2.
    Id OpISub(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point substraction of Operand 1 and Operand 2.
    Id OpFSub(Id result_type, Id operand_1, Id operand_2);

    /// Integer multiplication of Operand 1 and Operand 2.
    Id OpIMul(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point multiplication of Operand 1 and Operand 2.
    Id OpFMul(Id result_type, Id operand_1, Id operand_2);

    /// Unsigned-integer division of Operand 1 divided by Operand 2.
    Id OpUDiv(Id result_type, Id operand_1, Id operand_2);

    /// signed-integer division of Operand 1 divided by Operand 2.
    Id OpSDiv(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point division of Operand 1 divided by Operand 2.
    Id OpFDiv(Id result_type, Id operand_1, Id operand_2);

    /// Unsigned modulo operation of Operand 1 modulo Operand 2.
    Id OpUMod(Id result_type, Id operand_1, Id operand_2);

    /// Signed modulo operation of Operand 1 modulo Operand 2.
    Id OpSMod(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point modulo operation of Operand 1 modulo Operand 2.
    Id OpFMod(Id result_type, Id operand_1, Id operand_2);

    /// Signed reminder operation of Operand 1 modulo Operand 2.
    Id OpSRem(Id result_type, Id operand_1, Id operand_2);

    /// Floating-point reminder operation of Operand 1 modulo Operand 2.
    Id OpFRem(Id result_type, Id operand_1, Id operand_2);

    /// Result is the unsigned integer addition of Operand 1 and Operand 2, including its carry.
    Id OpIAddCarry(Id result_type, Id operand_1, Id operand_2);

    // Extensions

    /// Execute an instruction in an imported set of extended instructions.
    Id OpExtInst(Id result_type, Id set, std::uint32_t instruction,
                 const std::vector<Id>& operands);

    /// Execute an instruction in an imported set of extended instructions.
    template <typename... Ts>
    Id OpExtInst(Id result_type, Id set, std::uint32_t instruction, Ts&&... operands) {
        return OpExtInst(result_type, set, instruction, {operands...});
    }

    /// Result is x if x >= 0; otherwise result is -x.
    Id OpFAbs(Id result_type, Id x);

    /// Result is x if x >= 0; otherwise result is -x.
    Id OpSAbs(Id result_type, Id x);

    /// Result is the value equal to the nearest whole number to x. The fraction 0.5 will round in a
    /// direction chosen by the implementation, presumably the direction that is fastest.
    Id OpRound(Id result_type, Id x);

    /// Result is the value equal to the nearest whole number to x. A fractional part of 0.5 will
    /// round toward the nearest even whole number.
    Id OpRoundEven(Id result_type, Id x);

    /// Result is the value equal to the nearest whole number to x whose absolute value is not
    /// larger than the absolute value of x.
    Id OpTrunc(Id result_type, Id x);

    /// Result is 1.0 if x > 0, 0.0 if x = 0, or -1.0 if x < 0.
    Id OpFSign(Id result_type, Id x);

    /// Result is 1 if x > 0, 0 if x = 0, or -1 if x < 0, where x is interpreted as a signed
    /// integer.
    Id OpSSign(Id result_type, Id x);

    /// Result is the value equal to the nearest whole number that is less than or equal to x.
    Id OpFloor(Id result_type, Id x);

    /// Result is the value equal to the nearest whole number that is greater than or equal to x.
    Id OpCeil(Id result_type, Id x);

    /// Result is x - floor x.
    Id OpFract(Id result_type, Id x);

    /// The standard trigonometric sine of x radians.
    Id OpSin(Id result_type, Id x);

    /// The standard trigonometric cosine of x radians.
    Id OpCos(Id result_type, Id x);

    /// Arc sine. Result is an angle, in radians, whose sine is x. The range of result values is
    /// [-pi / 2, pi / 2]. Result is undefined if abs x > 1.
    Id OpAsin(Id result_type, Id x);

    /// Arc cosine. Result is an angle, in radians, whose cosine is x. The range of result values is
    /// [0, pi]. Result is undefined if abs x > 1.
    Id OpAcos(Id result_type, Id x);

    /// Result is x raised to the y power. Result is undefined if x < 0. Result is undefined if x =
    /// 0 and y <= 0.
    Id OpPow(Id result_type, Id x, Id y);

    /// Result is the natural exponentiation of x.
    Id OpExp(Id result_type, Id x);

    /// Result is the natural logarithm of x.  Result is undefined if x <= 0.
    Id OpLog(Id result_type, Id x);

    /// Result is 2 raised to the x power.
    Id OpExp2(Id result_type, Id x);

    /// Result is the base-2 logarithm of x. Result is undefined if x <= 0.
    Id OpLog2(Id result_type, Id x);

    /// Result is the square root of x. Result is undefined if x < 0.
    Id OpSqrt(Id result_type, Id x);

    /// Result is the reciprocal of sqrt x. Result is undefined if x <= 0.
    Id OpInverseSqrt(Id result_type, Id x);

    /// Result is y if y < x; otherwise result is x. Which operand is the result is undefined if one
    /// of the operands is a NaN.
    Id OpFMin(Id result_type, Id x, Id y);

    /// Result is y if y < x; otherwise result is x, where x and y are interpreted as unsigned
    /// integers.
    Id OpUMin(Id result_type, Id x, Id y);

    /// Result is y if y < x; otherwise result is x, where x and y are interpreted as signed
    /// integers.
    Id OpSMin(Id result_type, Id x, Id y);

    /// Result is y if x < y; otherwise result is x. Which operand is the result is undefined if one
    /// of the operands is a NaN.
    Id OpFMax(Id result_type, Id x, Id y);

    /// Result is y if x < y; otherwise result is x, where x and y are interpreted as unsigned
    /// integers.
    Id OpUMax(Id result_type, Id x, Id y);

    /// Result is y if x < y; otherwise result is x, where x and y are interpreted as signed
    /// integers.
    Id OpSMax(Id result_type, Id x, Id y);

    /// Result is min(max(x, minVal), maxVal). Result is undefined if minVal > maxVal.The semantics
    /// used by min() and max() are those of FMin and FMax.
    Id OpFClamp(Id result_type, Id x, Id min_val, Id max_val);

    /// Result is min(max(x, minVal), maxVal), where x, minVal and maxVal are interpreted as
    /// unsigned integers. Result is undefined if minVal > maxVal.
    Id OpUClamp(Id result_type, Id x, Id min_val, Id max_val);

    /// Result is min(max(x, minVal), maxVal), where x, minVal and maxVal are interpreted as signed
    /// integers. Result is undefined if minVal > maxVal.
    Id OpSClamp(Id result_type, Id x, Id min_val, Id max_val);

    /// Computes a * b + c.
    Id OpFma(Id result_type, Id a, Id b, Id c);

    /// Result is the unsigned integer obtained by converting the components of a two-component
    /// floating-point vector to the 16-bit OpTypeFloat, and then packing these two 16-bit integers
    /// into a 32-bit unsigned integer.
    Id OpPackHalf2x16(Id result_type, Id v);

    /// Result is the two-component floating-point vector with components obtained by unpacking a
    /// 32-bit unsigned integer into a pair of 16-bit values.
    Id OpUnpackHalf2x16(Id result_type, Id v);

    /// Integer least-significant bit.
    Id OpFindILsb(Id result_type, Id value);

    /// Signed-integer most-significant bit, with value interpreted as a signed integer.
    Id OpFindSMsb(Id result_type, Id value);

    /// Unsigned-integer most-significant bit.
    Id OpFindUMsb(Id result_type, Id value);

    /// Result is the value of the input interpolant sampled at a location inside both the pixel and
    /// the primitive being processed.
    Id OpInterpolateAtCentroid(Id result_type, Id interpolant);

    /// Result is the value of the input interpolant variable at the location of sample number
    /// sample.
    Id OpInterpolateAtSample(Id result_type, Id interpolant, Id sample);

    /// Result is the value of the input interpolant variable sampled at an offset from the center
    /// of the pixel specified by offset.
    Id OpInterpolateAtOffset(Id result_type, Id interpolant, Id offset);

    // Image

    /// Create a sampled image, containing both a sampler and an image.
    Id OpSampledImage(Id result_type, Id image, Id sampler);

    /// Sample an image with an implicit level of detail.
    Id OpImageSampleImplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                std::optional<spv::ImageOperandsMask> image_operands = {},
                                const std::vector<Id>& operands = {});

    /// Sample an image with an implicit level of detail.
    template <typename... Ts>
    Id OpImageSampleImplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleImplicitLod(result_type, sampled_image, coordinate, image_operands,
                                        {operands...});
    }

    /// Sample an image using an explicit level of detail.
    Id OpImageSampleExplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                spv::ImageOperandsMask image_operands,
                                const std::vector<Id>& operands = {});

    /// Sample an image using an explicit level of detail.
    template <typename... Ts>
    Id OpImageSampleExplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleExplicitLod(result_type, sampled_image, coordinate, image_operands,
                                        {operands...});
    }

    /// Sample an image doing depth-comparison with an implicit level of detail.
    Id OpImageSampleDrefImplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                    std::optional<spv::ImageOperandsMask> image_operands = {},
                                    const std::vector<Id>& operands = {});

    /// Sample an image doing depth-comparison with an implicit level of detail.
    template <typename... Ts>
    Id OpImageSampleDrefImplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                    spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleDrefImplicitLod(result_type, sampled_image, coordinate, dref,
                                            image_operands, {operands...});
    }

    /// Sample an image doing depth-comparison using an explicit level of detail.
    Id OpImageSampleDrefExplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                    spv::ImageOperandsMask image_operands,
                                    const std::vector<Id>& operands = {});

    /// Sample an image doing depth-comparison using an explicit level of detail.
    template <typename... Ts>
    Id OpImageSampleDrefExplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                    spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleDrefExplicitLod(result_type, sampled_image, coordinate, dref,
                                            image_operands, {operands...});
    }

    /// Sample an image with with a project coordinate and an implicit level of detail.
    Id OpImageSampleProjImplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                    std::optional<spv::ImageOperandsMask> image_operands = {},
                                    const std::vector<Id>& operands = {});

    /// Sample an image with with a project coordinate and an implicit level of detail.
    template <typename... Ts>
    Id OpImageSampleProjImplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                    spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleProjImplicitLod(result_type, sampled_image, coordinate, image_operands,
                                            {operands...});
    }

    /// Sample an image with a project coordinate using an explicit level of detail.
    Id OpImageSampleProjExplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                    spv::ImageOperandsMask image_operands,
                                    const std::vector<Id>& operands = {});

    /// Sample an image with a project coordinate using an explicit level of detail.
    template <typename... Ts>
    Id OpImageSampleProjExplicitLod(Id result_type, Id sampled_image, Id coordinate,
                                    spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleProjExplicitLod(result_type, sampled_image, coordinate, image_operands,
                                            {operands...});
    }

    /// Sample an image with a project coordinate, doing depth-comparison, with an implicit level of
    /// detail.
    Id OpImageSampleProjDrefImplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                        std::optional<spv::ImageOperandsMask> image_operands = {},
                                        const std::vector<Id>& operands = {});

    /// Sample an image with a project coordinate, doing depth-comparison, with an implicit level of
    /// detail.
    template <typename... Ts>
    Id OpImageSampleProjDrefImplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                        spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleProjDrefImplicitLod(result_type, sampled_image, coordinate, dref,
                                                image_operands, {operands...});
    }

    /// Sample an image with a project coordinate, doing depth-comparison, using an explicit level
    /// of detail.
    Id OpImageSampleProjDrefExplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                        spv::ImageOperandsMask image_operands,
                                        const std::vector<Id>& operands = {});

    /// Sample an image with a project coordinate, doing depth-comparison, using an explicit level
    /// of detail.
    template <typename... Ts>
    Id OpImageSampleProjDrefExplicitLod(Id result_type, Id sampled_image, Id coordinate, Id dref,
                                        spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageSampleProjDrefExplicitLod(result_type, sampled_image, coordinate, dref,
                                                image_operands, {operands...});
    }

    /// Fetch a single texel from an image whose Sampled operand is 1.
    Id OpImageFetch(Id result_type, Id sampled_image, Id coordinate,
                    std::optional<spv::ImageOperandsMask> image_operands = {},
                    const std::vector<Id>& operands = {});

    /// Fetch a single texel from an image whose Sampled operand is 1.
    template <typename... Ts>
    Id OpImageFetch(Id result_type, Id sampled_image, Id coordinate,
                    spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageFetch(result_type, sampled_image, coordinate, image_operands, {operands...});
    }

    /// Gathers the requested component from four texels.
    Id OpImageGather(Id result_type, Id sampled_image, Id coordinate, Id component,
                     std::optional<spv::ImageOperandsMask> image_operands = {},
                     const std::vector<Id>& operands = {});

    /// Gathers the requested component from four texels.
    template <typename... Ts>
    Id OpImageGather(Id result_type, Id sampled_image, Id coordinate, Id component,
                     spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageGather(result_type, sampled_image, coordinate, component, image_operands,
                             {operands...});
    }

    /// Gathers the requested depth-comparison from four texels.
    Id OpImageDrefGather(Id result_type, Id sampled_image, Id coordinate, Id dref,
                         std::optional<spv::ImageOperandsMask> image_operands = {},
                         const std::vector<Id>& operands = {});

    /// Gathers the requested depth-comparison from four texels.
    template <typename... Ts>
    Id OpImageDrefGather(Id result_type, Id sampled_image, Id coordinate, Id dref,
                         spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageDrefGather(result_type, sampled_image, coordinate, dref, image_operands,
                                 {operands...});
    }

    /// Read a texel from an image without a sampler.
    Id OpImageRead(Id result_type, Id sampled_image, Id coordinate,
                   std::optional<spv::ImageOperandsMask> image_operands = {},
                   const std::vector<Id>& operands = {});

    /// Read a texel from an image without a sampler.
    template <typename... Ts>
    Id OpImageRead(Id result_type, Id sampled_image, Id coordinate,
                   spv::ImageOperandsMask image_operands, Ts&&... operands) {
        return OpImageRead(result_type, sampled_image, coordinate, image_operands, {operands...});
    }

    /// Write a texel to an image without a sampler.
    Id OpImageWrite(Id image, Id coordinate, Id texel,
                    std::optional<spv::ImageOperandsMask> image_operands = {},
                    const std::vector<Id>& operands = {});

    /// Write a texel to an image without a sampler.
    template <typename... Ts>
    Id OpImageWrite(Id image, Id coordinate, Id texel, spv::ImageOperandsMask image_operands,
                    Ts&&... operands) {
        return OpImageWrite(image, coordinate, texel, image_operands, {operands...});
    }

    /// Extract the image from a sampled image.
    Id OpImage(Id result_type, Id sampled_image);

    /// Query the dimensions of Image for mipmap level for Level of Detail.
    Id OpImageQuerySizeLod(Id result_type, Id image, Id level_of_detail);

    /// Query the dimensions of Image, with no level of detail.
    Id OpImageQuerySize(Id result_type, Id image);

    /// Query the mipmap level and the level of detail for a hypothetical sampling of Image at
    /// Coordinate using an implicit level of detail.
    Id OpImageQueryLod(Id result_type, Id image, Id coordinate);

    /// Query the number of mipmap levels accessible through Image.
    Id OpImageQueryLevels(Id result_type, Id image);

    /// Query the number of samples available per texel fetch in a multisample image.
    Id OpImageQuerySamples(Id result_type, Id image);

    // Group

    /// Computes a bitfield value combining the Predicate value from all invocations in the current
    /// Subgroup that execute the same dynamic instance of this instruction.
    Id OpSubgroupBallotKHR(Id result_type, Id predicate);

    /// Return the value from the invocation in the subgroup with an invocation ID equal to index.
    /// The index must be the same for all active invocations in the subgroup, otherwise the results
    /// are undefined.
    Id OpSubgroupReadInvocationKHR(Id result_type, Id value, Id index);

    /// TBD
    Id OpSubgroupAllKHR(Id result_type, Id predicate);

    /// TBD
    Id OpSubgroupAnyKHR(Id result_type, Id predicate);

    /// TBD
    Id OpSubgroupAllEqualKHR(Id result_type, Id predicate);

    /// Return the value of the invocation identified by the current invocation's id within the
    /// group xor'ed with mask.
    Id OpGroupNonUniformShuffleXor(Id result_type, spv::Scope scope, Id value, Id mask);

    // Atomic

    /// Atomically load through Pointer using the given Semantics. All subparts of the value that is
    /// loaded will be read atomically with respect to all other atomic accesses to it within Scope.
    Id OpAtomicLoad(Id result_type, Id pointer, Id memory, Id semantics);

    /// Atomically store through Pointer using the given Semantics. All subparts of Value will be
    /// written atomically with respect to all other atomic accesses to it within Scope.
    Id OpAtomicStore(Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value from copying Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicExchange(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value from Value only if Original Value equals Comparator, and
    /// 3) store the New Value back through Pointer only if 'Original Value equaled Comparator.
    Id OpAtomicCompareExchange(Id result_type, Id pointer, Id memory, Id equal, Id unequal,
                               Id value, Id comparator);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value through integer addition of 1 to Original Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicIIncrement(Id result_type, Id pointer, Id memory, Id semantics);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value through integer subtraction of 1 from Original Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicIDecrement(Id result_type, Id pointer, Id memory, Id semantics);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by integer addition of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicIAdd(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by integer subtraction of Value from Original Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicISub(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by finding the smallest signed integer of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicSMin(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by finding the smallest unsigned integer of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicUMin(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by finding the largest signed integer of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicSMax(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by finding the largest unsigned integer of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicUMax(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by the bitwise AND of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicAnd(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by the bitwise OR of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicOr(Id result_type, Id pointer, Id memory, Id semantics, Id value);

    /// Perform the following steps atomically with respect to any other atomic accesses within
    /// Scope to the same location:
    /// 1) load through Pointer to get an Original Value,
    /// 2) get a New Value by the bitwise exclusive OR of Original Value and Value, and
    /// 3) store the New Value back through Pointer.
    Id OpAtomicXor(Id result_type, Id pointer, Id memory, Id semantics, Id value);

private:
    Id AddCode(std::unique_ptr<Op> op);

    Id AddCode(spv::Op opcode, std::optional<std::uint32_t> id = {});

    Id AddDeclaration(std::unique_ptr<Op> op);

    void AddAnnotation(std::unique_ptr<Op> op);

    Id GetGLSLstd450();

    std::uint32_t version{};
    std::uint32_t bound{1};

    std::unordered_set<std::string> extensions;
    std::unordered_set<spv::Capability> capabilities;
    std::unordered_set<std::unique_ptr<Op>> ext_inst_import;
    std::unique_ptr<Op> glsl_std_450;

    spv::AddressingModel addressing_model{spv::AddressingModel::Logical};
    spv::MemoryModel memory_model{spv::MemoryModel::GLSL450};

    std::vector<std::unique_ptr<Op>> entry_points;
    std::vector<std::unique_ptr<Op>> execution_modes;
    std::vector<std::unique_ptr<Op>> debug;
    std::vector<std::unique_ptr<Op>> annotations;
    std::vector<std::unique_ptr<Op>> declarations;

    std::vector<Id> global_variables;

    std::vector<Id> code;

    std::vector<std::unique_ptr<Op>> code_store;
};

} // namespace Sirit
