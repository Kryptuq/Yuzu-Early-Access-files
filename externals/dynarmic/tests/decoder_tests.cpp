/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstring>
#include <iomanip>
#include <iostream>

#include <catch.hpp>

#include "dynarmic/common/assert.h"
#include "dynarmic/frontend/A32/decoder/asimd.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"
#include "dynarmic/interface/A32/config.h"
#include "dynarmic/ir/opcodes.h"

using namespace Dynarmic;

TEST_CASE("ASIMD Decoder: Ensure table order correctness", "[decode][a32][.]") {
    const auto table = A32::GetASIMDDecodeTable<A32::TranslatorVisitor>();

    const auto get_ir = [](const A32::ASIMDMatcher<A32::TranslatorVisitor>& matcher, u32 instruction) {
        ASSERT(matcher.Matches(instruction));

        const A32::LocationDescriptor location{0, {}, {}};
        IR::Block block{location};
        A32::TranslatorVisitor visitor{block, location, {}};
        matcher.call(visitor, instruction);

        return block;
    };

    const auto is_decode_error = [&get_ir](const A32::ASIMDMatcher<A32::TranslatorVisitor>& matcher, u32 instruction) {
        const auto block = get_ir(matcher, instruction);

        for (const auto& ir_inst : block) {
            if (ir_inst.GetOpcode() == IR::Opcode::A32ExceptionRaised) {
                if (static_cast<A32::Exception>(ir_inst.GetArg(1).GetU64()) == A32::Exception::DecodeError) {
                    return true;
                }
            }
        }
        return false;
    };

    for (auto iter = table.cbegin(); iter != table.cend(); ++iter) {
        if (std::strncmp(iter->GetName(), "UNALLOCATED", 11) == 0) {
            continue;
        }

        const u32 expect = iter->GetExpected();
        const u32 mask = iter->GetMask();
        u32 x = 0;
        do {
            const u32 instruction = expect | x;

            const bool iserr = is_decode_error(*iter, instruction);
            const auto alternative = std::find_if(table.cbegin(), iter, [instruction](const auto& m) { return m.Matches(instruction); });
            const bool altiserr = is_decode_error(*alternative, instruction);

            INFO("Instruction: " << std::hex << std::setfill('0') << std::setw(8) << instruction);
            INFO("Expect:      " << std::hex << std::setfill('0') << std::setw(8) << expect);
            INFO("Fill:        " << std::hex << std::setfill('0') << std::setw(8) << x);
            INFO("Name:        " << iter->GetName());
            INFO("iserr:       " << iserr);
            INFO("alternative: " << alternative->GetName());
            INFO("altiserr:    " << altiserr);

            REQUIRE(((!iserr && alternative == iter) || (iserr && alternative != iter && !altiserr)));

            x = ((x | mask) + 1) & ~mask;
        } while (x != 0);
    }
}
