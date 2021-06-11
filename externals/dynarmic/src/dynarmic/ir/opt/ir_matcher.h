/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>
#include <tuple>

#include <mp/metafunction/apply.h>
#include <mp/typelist/concat.h>
#include <mp/typelist/drop.h>
#include <mp/typelist/get.h>
#include <mp/typelist/head.h>
#include <mp/typelist/list.h>
#include <mp/typelist/prepend.h>

#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Optimization::IRMatcher {

struct CaptureValue {
    using ReturnType = std::tuple<IR::Value>;

    static std::optional<ReturnType> Match(IR::Value value) {
        return std::tuple(value);
    }
};

struct CaptureInst {
    using ReturnType = std::tuple<IR::Inst*>;

    static std::optional<ReturnType> Match(IR::Value value) {
        if (value.IsImmediate())
            return std::nullopt;
        return std::tuple(value.GetInstRecursive());
    }
};

struct CaptureUImm {
    using ReturnType = std::tuple<u64>;

    static std::optional<ReturnType> Match(IR::Value value) {
        return std::tuple(value.GetImmediateAsU64());
    }
};

struct CaptureSImm {
    using ReturnType = std::tuple<s64>;

    static std::optional<ReturnType> Match(IR::Value value) {
        return std::tuple(value.GetImmediateAsS64());
    }
};

template<u64 Value>
struct UImm {
    using ReturnType = std::tuple<>;

    static std::optional<std::tuple<>> Match(IR::Value value) {
        if (value.GetImmediateAsU64() == Value)
            return std::tuple();
        return std::nullopt;
    }
};

template<s64 Value>
struct SImm {
    using ReturnType = std::tuple<>;

    static std::optional<std::tuple<>> Match(IR::Value value) {
        if (value.GetImmediateAsS64() == Value)
            return std::tuple();
        return std::nullopt;
    }
};

template<IR::Opcode Opcode, typename... Args>
struct Inst {
public:
    using ReturnType = mp::concat<std::tuple<>, typename Args::ReturnType...>;

    static std::optional<ReturnType> Match(const IR::Inst& inst) {
        if (inst.GetOpcode() != Opcode)
            return std::nullopt;
        if (inst.HasAssociatedPseudoOperation())
            return std::nullopt;
        return MatchArgs<0>(inst);
    }

    static std::optional<ReturnType> Match(IR::Value value) {
        if (value.IsImmediate())
            return std::nullopt;
        return Match(*value.GetInstRecursive());
    }

private:
    template<size_t I>
    static auto MatchArgs(const IR::Inst& inst) -> std::optional<mp::apply<mp::concat, mp::prepend<mp::drop<I, mp::list<typename Args::ReturnType...>>, std::tuple<>>>> {
        if constexpr (I >= sizeof...(Args)) {
            return std::tuple();
        } else {
            using Arg = mp::get<I, mp::list<Args...>>;

            if (const auto arg = Arg::Match(inst.GetArg(I))) {
                if (const auto rest = MatchArgs<I + 1>(inst)) {
                    return std::tuple_cat(*arg, *rest);
                }
            }

            return std::nullopt;
        }
    }
};

inline bool IsSameInst(std::tuple<IR::Inst*, IR::Inst*> t) {
    return std::get<0>(t) == std::get<1>(t);
}

inline bool IsSameInst(std::tuple<IR::Inst*, IR::Inst*, IR::Inst*> t) {
    return std::get<0>(t) == std::get<1>(t) && std::get<0>(t) == std::get<2>(t);
}

}  // namespace Dynarmic::Optimization::IRMatcher
