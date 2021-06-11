/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstdint>
#include <optional>
#include <variant>

#include "dynarmic/interface/A32/coprocessor_util.h"

namespace Dynarmic {
namespace A32 {

class Jit;

class Coprocessor {
public:
    virtual ~Coprocessor() = default;

    struct Callback {
        /**
         * @param jit CPU state
         * @param user_arg Set to Callback::user_arg at runtime
         * @param arg0 Purpose of this argument depends on type of callback.
         * @param arg1 Purpose of this argument depends on type of callback.
         * @return Purpose of return value depends on type of callback.
         */
        std::uint64_t (*function)(Jit* jit, void* user_arg, std::uint32_t arg0, std::uint32_t arg1);
        /// If std::nullopt, function will be called with a user_arg parameter containing garbage.
        std::optional<void*> user_arg;
    };

    /**
     * std::monostate: coprocessor exception will be compiled
     * Callback: a call to the Callback will be compiled
     * std::uint32_t*: a write/read to that memory address will be compiled
     */
    using CallbackOrAccessOneWord = std::variant<std::monostate, Callback, std::uint32_t*>;

    /**
     * std::monostate: coprocessor exception will be compiled
     * Callback: a call to the Callback will be compiled
     * std::array<std::uint32_t*, 2>: a write/read to those memory addresses will be compiled
     */
    using CallbackOrAccessTwoWords = std::variant<std::monostate, Callback, std::array<std::uint32_t*, 2>>;

    /**
     * Called when compiling CDP or CDP2 for this coprocessor.
     * A return value of std::nullopt will cause a coprocessor exception to be compiled.
     * arg0, arg1 and return value of callback are ignored.
     */
    virtual std::optional<Callback> CompileInternalOperation(bool two, unsigned opc1, CoprocReg CRd, CoprocReg CRn, CoprocReg CRm, unsigned opc2) = 0;

    /**
     * Called when compiling MCR or MCR2 for this coprocessor.
     * A return value of std::monostate will cause a coprocessor exception to be compiled.
     * arg0 of the callback will contain the word sent to the coprocessor.
     * arg1 and return value of the callback are ignored.
     */
    virtual CallbackOrAccessOneWord CompileSendOneWord(bool two, unsigned opc1, CoprocReg CRn, CoprocReg CRm, unsigned opc2) = 0;

    /**
     * Called when compiling MCRR or MCRR2 for this coprocessor.
     * A return value of std::monostate will cause a coprocessor exception to be compiled.
     * arg0 and arg1 of the callback will contain the words sent to the coprocessor.
     * The return value of the callback is ignored.
     */
    virtual CallbackOrAccessTwoWords CompileSendTwoWords(bool two, unsigned opc, CoprocReg CRm) = 0;

    /**
     * Called when compiling MRC or MRC2 for this coprocessor.
     * A return value of std::monostate will cause a coprocessor exception to be compiled.
     * The return value of the callback should contain word from coprocessor.
     * The low word of the return value will be stored in Rt.
     * arg0 and arg1 of the callback are ignored.
     */
    virtual CallbackOrAccessOneWord CompileGetOneWord(bool two, unsigned opc1, CoprocReg CRn, CoprocReg CRm, unsigned opc2) = 0;

    /**
     * Called when compiling MRRC or MRRC2 for this coprocessor.
     * A return value of std::monostate will cause a coprocessor exception to be compiled.
     * The return value of the callback should contain words from coprocessor.
     * The low word of the return value will be stored in Rt.
     * The high word of the return value will be stored in Rt2.
     * arg0 and arg1 of the callback are ignored.
     */
    virtual CallbackOrAccessTwoWords CompileGetTwoWords(bool two, unsigned opc, CoprocReg CRm) = 0;

    /**
     * Called when compiling LDC or LDC2 for this coprocessor.
     * A return value of std::nullopt will cause a coprocessor exception to be compiled.
     * arg0 of the callback will contain the start address.
     * arg1 and return value of the callback are ignored.
     */
    virtual std::optional<Callback> CompileLoadWords(bool two, bool long_transfer, CoprocReg CRd, std::optional<std::uint8_t> option) = 0;

    /**
     * Called when compiling STC or STC2 for this coprocessor.
     * A return value of std::nullopt will cause a coprocessor exception to be compiled.
     * arg0 of the callback will contain the start address.
     * arg1 and return value of the callback are ignored.
     */
    virtual std::optional<Callback> CompileStoreWords(bool two, bool long_transfer, CoprocReg CRd, std::optional<std::uint8_t> option) = 0;
};

}  // namespace A32
}  // namespace Dynarmic
