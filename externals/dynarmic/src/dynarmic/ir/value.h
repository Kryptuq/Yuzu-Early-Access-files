/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <type_traits>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/ir/type.h"

namespace Dynarmic::A32 {
enum class ExtReg;
enum class Reg;
}  // namespace Dynarmic::A32

namespace Dynarmic::A64 {
enum class Reg;
enum class Vec;
}  // namespace Dynarmic::A64

namespace Dynarmic::IR {

class Inst;
enum class AccType;
enum class Cond;

/**
 * A representation of a value in the IR.
 * A value may either be an immediate or the result of a microinstruction.
 */
class Value {
public:
    using CoprocessorInfo = std::array<u8, 8>;

    Value()
            : type(Type::Void) {}
    explicit Value(Inst* value);
    explicit Value(A32::Reg value);
    explicit Value(A32::ExtReg value);
    explicit Value(A64::Reg value);
    explicit Value(A64::Vec value);
    explicit Value(bool value);
    explicit Value(u8 value);
    explicit Value(u16 value);
    explicit Value(u32 value);
    explicit Value(u64 value);
    explicit Value(CoprocessorInfo value);
    explicit Value(Cond value);
    explicit Value(AccType value);

    bool IsIdentity() const;
    bool IsEmpty() const;
    bool IsImmediate() const;
    Type GetType() const;

    Inst* GetInst() const;
    Inst* GetInstRecursive() const;
    A32::Reg GetA32RegRef() const;
    A32::ExtReg GetA32ExtRegRef() const;
    A64::Reg GetA64RegRef() const;
    A64::Vec GetA64VecRef() const;
    bool GetU1() const;
    u8 GetU8() const;
    u16 GetU16() const;
    u32 GetU32() const;
    u64 GetU64() const;
    CoprocessorInfo GetCoprocInfo() const;
    Cond GetCond() const;
    AccType GetAccType() const;

    /**
     * Retrieves the immediate of a Value instance as a signed 64-bit value.
     *
     * @pre The value contains either a U1, U8, U16, U32, or U64 value.
     *      Breaking this precondition will cause an assertion to be invoked.
     */
    s64 GetImmediateAsS64() const;

    /**
     * Retrieves the immediate of a Value instance as an unsigned 64-bit value.
     *
     * @pre The value contains either a U1, U8, U16, U32, or U64 value.
     *      Breaking this precondition will cause an assertion to be invoked.
     */
    u64 GetImmediateAsU64() const;

    /**
     * Determines whether or not the contained value matches the provided signed one.
     *
     * Note that this function will always return false if the contained
     * value is not a a constant value. In other words, if IsImmediate()
     * would return false on an instance, then so will this function.
     *
     * @param value The value to check against the contained value.
     */
    bool IsSignedImmediate(s64 value) const;

    /**
     * Determines whether or not the contained value matches the provided unsigned one.
     *
     * Note that this function will always return false if the contained
     * value is not a a constant value. In other words, if IsImmediate()
     * would return false on an instance, then so will this function.
     *
     * @param value The value to check against the contained value.
     */
    bool IsUnsignedImmediate(u64 value) const;

    /**
     * Determines whether or not the contained constant value has all bits set.
     *
     * @pre The value contains either a U1, U8, U16, U32, or U64 value.
     *      Breaking this precondition will cause an assertion to be invoked.
     */
    bool HasAllBitsSet() const;

    /**
     * Whether or not the current value contains a representation of zero.
     *
     * Note that this function will always return false if the contained
     * value is not a a constant value. In other words, if IsImmediate()
     * would return false on an instance, then so will this function.
     */
    bool IsZero() const;

private:
    Type type;

    union {
        Inst* inst;  // type == Type::Opaque
        A32::Reg imm_a32regref;
        A32::ExtReg imm_a32extregref;
        A64::Reg imm_a64regref;
        A64::Vec imm_a64vecref;
        bool imm_u1;
        u8 imm_u8;
        u16 imm_u16;
        u32 imm_u32;
        u64 imm_u64;
        CoprocessorInfo imm_coproc;
        Cond imm_cond;
        AccType imm_acctype;
    } inner;
};
static_assert(sizeof(Value) <= 2 * sizeof(u64), "IR::Value should be kept small in size");

template<Type type_>
class TypedValue final : public Value {
public:
    TypedValue() = default;

    template<Type other_type, typename = std::enable_if_t<(other_type & type_) != Type::Void>>
    /* implicit */ TypedValue(const TypedValue<other_type>& value)
            : Value(value) {
        ASSERT((value.GetType() & type_) != Type::Void);
    }

    explicit TypedValue(const Value& value)
            : Value(value) {
        ASSERT((value.GetType() & type_) != Type::Void);
    }

    explicit TypedValue(Inst* inst)
            : TypedValue(Value(inst)) {}
};

using U1 = TypedValue<Type::U1>;
using U8 = TypedValue<Type::U8>;
using U16 = TypedValue<Type::U16>;
using U32 = TypedValue<Type::U32>;
using U64 = TypedValue<Type::U64>;
using U128 = TypedValue<Type::U128>;
using U32U64 = TypedValue<Type::U32 | Type::U64>;
using U16U32U64 = TypedValue<Type::U16 | Type::U32 | Type::U64>;
using UAny = TypedValue<Type::U8 | Type::U16 | Type::U32 | Type::U64>;
using UAnyU128 = TypedValue<Type::U8 | Type::U16 | Type::U32 | Type::U64 | Type::U128>;
using NZCV = TypedValue<Type::NZCVFlags>;
using Table = TypedValue<Type::Table>;

}  // namespace Dynarmic::IR
