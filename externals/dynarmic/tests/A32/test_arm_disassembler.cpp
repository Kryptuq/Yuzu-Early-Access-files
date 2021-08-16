/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch2/catch.hpp>

#include "dynarmic/frontend/A32/disassembler/disassembler.h"

using Dynarmic::A32::DisassembleArm;

TEST_CASE("Disassemble branch instructions", "[arm][disassembler]") {
    REQUIRE(DisassembleArm(0xEAFFFFFE) == "b +#0");
    REQUIRE(DisassembleArm(0xEB000008) == "bl +#40");
    REQUIRE(DisassembleArm(0xFBFFFFFE) == "blx +#2");
    REQUIRE(DisassembleArm(0xFAFFFFFF) == "blx +#4");
    REQUIRE(DisassembleArm(0xFBE1E7FE) == "blx -#7888894");
    REQUIRE(DisassembleArm(0xE12FFF3D) == "blx sp");
    REQUIRE(DisassembleArm(0x312FFF13) == "bxcc r3");
    REQUIRE(DisassembleArm(0x012FFF29) == "bxjeq r9");
}

TEST_CASE("Disassemble data processing instructions", "[arm][disassembler]") {
    REQUIRE(DisassembleArm(0xE2A21004) == "adc r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0A21143) == "adc r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0A21103) == "adc r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0A21123) == "adc r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0A21163) == "adc r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0A21003) == "adc r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0A21063) == "adc r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0A21453) == "adc r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0A21413) == "adc r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0A21433) == "adc r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0A21473) == "adc r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE2B21004) == "adcs r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0B21143) == "adcs r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0B21103) == "adcs r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0B21123) == "adcs r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0B21163) == "adcs r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0B21003) == "adcs r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0B21063) == "adcs r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0B21453) == "adcs r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0B21413) == "adcs r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0B21433) == "adcs r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0B21473) == "adcs r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE2853004) == "add r3, r5, #4");
    REQUIRE(DisassembleArm(0xE0821143) == "add r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0821103) == "add r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0821123) == "add r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0821163) == "add r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0821003) == "add r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0821453) == "add r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0821413) == "add r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0821433) == "add r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0821473) == "add r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE0821063) == "add r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE2953004) == "adds r3, r5, #4");
    REQUIRE(DisassembleArm(0xE0921143) == "adds r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0921103) == "adds r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0921123) == "adds r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0921163) == "adds r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0921003) == "adds r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0921063) == "adds r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0921453) == "adds r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0921413) == "adds r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0921433) == "adds r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0921473) == "adds r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE2021004) == "and r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0021143) == "and r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0021103) == "and r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0021123) == "and r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0021163) == "and r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0021003) == "and r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0021453) == "and r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0021413) == "and r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0021433) == "and r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0021473) == "and r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE0021063) == "and r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE2121004) == "ands r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0121143) == "ands r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0121103) == "ands r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0121123) == "ands r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0121163) == "ands r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0121003) == "ands r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0121063) == "ands r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0121453) == "ands r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0121413) == "ands r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0121433) == "ands r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0121473) == "ands r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE3C21004) == "bic r1, r2, #4");
    REQUIRE(DisassembleArm(0xE1C21143) == "bic r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE1C21103) == "bic r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE1C21123) == "bic r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE1C21163) == "bic r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE1C21003) == "bic r1, r2, r3");
    REQUIRE(DisassembleArm(0xE1C21453) == "bic r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE1C21413) == "bic r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE1C21433) == "bic r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE1C21473) == "bic r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE1C21063) == "bic r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE3D21004) == "bics r1, r2, #4");
    REQUIRE(DisassembleArm(0xE1D21143) == "bics r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE1D21103) == "bics r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE1D21123) == "bics r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE1D21163) == "bics r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE1D21003) == "bics r1, r2, r3");
    REQUIRE(DisassembleArm(0xE1D21063) == "bics r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE1D21453) == "bics r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE1D21413) == "bics r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE1D21433) == "bics r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE1D21473) == "bics r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE3710004) == "cmn r1, #4");
    REQUIRE(DisassembleArm(0xE1710142) == "cmn r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1710102) == "cmn r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1710122) == "cmn r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1710162) == "cmn r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1710002) == "cmn r1, r2");
    REQUIRE(DisassembleArm(0xE1710062) == "cmn r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1710352) == "cmn r1, r2, asr r3");
    REQUIRE(DisassembleArm(0xE1710312) == "cmn r1, r2, lsl r3");
    REQUIRE(DisassembleArm(0xE1710332) == "cmn r1, r2, lsr r3");
    REQUIRE(DisassembleArm(0xE1710372) == "cmn r1, r2, ror r3");

    REQUIRE(DisassembleArm(0xE3510004) == "cmp r1, #4");
    REQUIRE(DisassembleArm(0xE1510142) == "cmp r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1510102) == "cmp r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1510122) == "cmp r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1510162) == "cmp r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1510002) == "cmp r1, r2");
    REQUIRE(DisassembleArm(0xE1510062) == "cmp r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1510352) == "cmp r1, r2, asr r3");
    REQUIRE(DisassembleArm(0xE1510312) == "cmp r1, r2, lsl r3");
    REQUIRE(DisassembleArm(0xE1510332) == "cmp r1, r2, lsr r3");
    REQUIRE(DisassembleArm(0xE1510372) == "cmp r1, r2, ror r3");

    REQUIRE(DisassembleArm(0xE2221004) == "eor r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0221243) == "eor r1, r2, r3, asr #4");
    REQUIRE(DisassembleArm(0xE0221203) == "eor r1, r2, r3, lsl #4");
    REQUIRE(DisassembleArm(0xE0221223) == "eor r1, r2, r3, lsr #4");
    REQUIRE(DisassembleArm(0xE0221263) == "eor r1, r2, r3, ror #4");
    REQUIRE(DisassembleArm(0xE0221003) == "eor r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0221453) == "eor r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0221413) == "eor r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0221433) == "eor r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0221473) == "eor r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE0221063) == "eor r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE2321004) == "eors r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0321243) == "eors r1, r2, r3, asr #4");
    REQUIRE(DisassembleArm(0xE0321203) == "eors r1, r2, r3, lsl #4");
    REQUIRE(DisassembleArm(0xE0321223) == "eors r1, r2, r3, lsr #4");
    REQUIRE(DisassembleArm(0xE0321263) == "eors r1, r2, r3, ror #4");
    REQUIRE(DisassembleArm(0xE0321003) == "eors r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0321453) == "eors r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0321413) == "eors r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0321433) == "eors r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0321473) == "eors r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE0321063) == "eors r1, r2, r3, rrx");

    REQUIRE(DisassembleArm(0xE3A010FF) == "mov r1, #255");
    REQUIRE(DisassembleArm(0xE1A01142) == "mov r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1A01102) == "mov r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1A01122) == "mov r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1A01162) == "mov r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1A01062) == "mov r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1A0E00F) == "mov lr, pc");
    REQUIRE(DisassembleArm(0xE3B010FF) == "movs r1, #255");
    REQUIRE(DisassembleArm(0xE1B0E00F) == "movs lr, pc");
    REQUIRE(DisassembleArm(0xE1B01142) == "movs r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1B01102) == "movs r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1B01122) == "movs r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1B01162) == "movs r1, r2, ror #2");

    REQUIRE(DisassembleArm(0xE3E01004) == "mvn r1, #4");
    REQUIRE(DisassembleArm(0xE1E01142) == "mvn r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1E01102) == "mvn r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1E01122) == "mvn r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1E01162) == "mvn r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1E01062) == "mvn r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1E01002) == "mvn r1, r2");
    REQUIRE(DisassembleArm(0xE1E01352) == "mvn r1, r2, asr r3");
    REQUIRE(DisassembleArm(0xE1E01312) == "mvn r1, r2, lsl r3");
    REQUIRE(DisassembleArm(0xE1E01332) == "mvn r1, r2, lsr r3");
    REQUIRE(DisassembleArm(0xE1E01372) == "mvn r1, r2, ror r3");
    REQUIRE(DisassembleArm(0xE3F01004) == "mvns r1, #4");
    REQUIRE(DisassembleArm(0xE1F01142) == "mvns r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1F01102) == "mvns r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1F01122) == "mvns r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1F01162) == "mvns r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1F01062) == "mvns r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1F01002) == "mvns r1, r2");
    REQUIRE(DisassembleArm(0xE1F01352) == "mvns r1, r2, asr r3");
    REQUIRE(DisassembleArm(0xE1F01312) == "mvns r1, r2, lsl r3");
    REQUIRE(DisassembleArm(0xE1F01332) == "mvns r1, r2, lsr r3");
    REQUIRE(DisassembleArm(0xE1F01372) == "mvns r1, r2, ror r3");

    REQUIRE(DisassembleArm(0xE3821004) == "orr r1, r2, #4");
    REQUIRE(DisassembleArm(0xE1821143) == "orr r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE1821103) == "orr r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE1821123) == "orr r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE1821163) == "orr r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE1821063) == "orr r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE1821003) == "orr r1, r2, r3");
    REQUIRE(DisassembleArm(0xE1821453) == "orr r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE1821413) == "orr r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE1821433) == "orr r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE1821473) == "orr r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE3921004) == "orrs r1, r2, #4");
    REQUIRE(DisassembleArm(0xE1921143) == "orrs r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE1921103) == "orrs r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE1921123) == "orrs r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE1921163) == "orrs r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE1921063) == "orrs r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE1921003) == "orrs r1, r2, r3");
    REQUIRE(DisassembleArm(0xE1921453) == "orrs r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE1921413) == "orrs r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE1921433) == "orrs r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE1921473) == "orrs r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE2621004) == "rsb r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0621143) == "rsb r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0621103) == "rsb r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0621123) == "rsb r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0621163) == "rsb r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0621063) == "rsb r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0621003) == "rsb r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0621453) == "rsb r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0621413) == "rsb r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0621433) == "rsb r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0621473) == "rsb r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE2721004) == "rsbs r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0721143) == "rsbs r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0721103) == "rsbs r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0721123) == "rsbs r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0721163) == "rsbs r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0721063) == "rsbs r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0721003) == "rsbs r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0721453) == "rsbs r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0721413) == "rsbs r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0721433) == "rsbs r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0721473) == "rsbs r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE2E21004) == "rsc r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0E21143) == "rsc r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0E21103) == "rsc r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0E21123) == "rsc r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0E21163) == "rsc r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0E21063) == "rsc r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0E21003) == "rsc r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0E21453) == "rsc r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0E21413) == "rsc r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0E21433) == "rsc r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0E21473) == "rsc r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE2F21004) == "rscs r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0F21143) == "rscs r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0F21103) == "rscs r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0F21123) == "rscs r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0F21163) == "rscs r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0F21063) == "rscs r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0F21003) == "rscs r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0F21453) == "rscs r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0F21413) == "rscs r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0F21433) == "rscs r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0F21473) == "rscs r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE2C21004) == "sbc r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0C21143) == "sbc r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0C21103) == "sbc r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0C21123) == "sbc r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0C21163) == "sbc r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0C21063) == "sbc r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0C21003) == "sbc r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0C21453) == "sbc r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0C21413) == "sbc r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0C21433) == "sbc r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0C21473) == "sbc r1, r2, r3, ror r4");
    REQUIRE(DisassembleArm(0xE2D21004) == "sbcs r1, r2, #4");
    REQUIRE(DisassembleArm(0xE0D21143) == "sbcs r1, r2, r3, asr #2");
    REQUIRE(DisassembleArm(0xE0D21103) == "sbcs r1, r2, r3, lsl #2");
    REQUIRE(DisassembleArm(0xE0D21123) == "sbcs r1, r2, r3, lsr #2");
    REQUIRE(DisassembleArm(0xE0D21163) == "sbcs r1, r2, r3, ror #2");
    REQUIRE(DisassembleArm(0xE0D21063) == "sbcs r1, r2, r3, rrx");
    REQUIRE(DisassembleArm(0xE0D21003) == "sbcs r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0D21453) == "sbcs r1, r2, r3, asr r4");
    REQUIRE(DisassembleArm(0xE0D21413) == "sbcs r1, r2, r3, lsl r4");
    REQUIRE(DisassembleArm(0xE0D21433) == "sbcs r1, r2, r3, lsr r4");
    REQUIRE(DisassembleArm(0xE0D21473) == "sbcs r1, r2, r3, ror r4");

    REQUIRE(DisassembleArm(0xE3310004) == "teq r1, #4");
    REQUIRE(DisassembleArm(0xE1310142) == "teq r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1310102) == "teq r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1310122) == "teq r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1310162) == "teq r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1310002) == "teq r1, r2");
    REQUIRE(DisassembleArm(0xE1310062) == "teq r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1310352) == "teq r1, r2, asr r3");
    REQUIRE(DisassembleArm(0xE1310312) == "teq r1, r2, lsl r3");
    REQUIRE(DisassembleArm(0xE1310332) == "teq r1, r2, lsr r3");
    REQUIRE(DisassembleArm(0xE1310372) == "teq r1, r2, ror r3");

    REQUIRE(DisassembleArm(0xE3110004) == "tst r1, #4");
    REQUIRE(DisassembleArm(0xE1110142) == "tst r1, r2, asr #2");
    REQUIRE(DisassembleArm(0xE1110102) == "tst r1, r2, lsl #2");
    REQUIRE(DisassembleArm(0xE1110122) == "tst r1, r2, lsr #2");
    REQUIRE(DisassembleArm(0xE1110162) == "tst r1, r2, ror #2");
    REQUIRE(DisassembleArm(0xE1110002) == "tst r1, r2");
    REQUIRE(DisassembleArm(0xE1110062) == "tst r1, r2, rrx");
    REQUIRE(DisassembleArm(0xE1110352) == "tst r1, r2, asr r3");
    REQUIRE(DisassembleArm(0xE1110312) == "tst r1, r2, lsl r3");
    REQUIRE(DisassembleArm(0xE1110332) == "tst r1, r2, lsr r3");
    REQUIRE(DisassembleArm(0xE1110372) == "tst r1, r2, ror r3");
}

TEST_CASE("Disassemble half-word multiply and multiply accumulate instructions", "[arm][disassembler]") {
    REQUIRE(DisassembleArm(0xE1003281) == "smlabb r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE10032C1) == "smlabt r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE10032A1) == "smlatb r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE10032E1) == "smlatt r0, r1, r2, r3");

    REQUIRE(DisassembleArm(0xE1203281) == "smlawb r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE12032C1) == "smlawt r0, r1, r2, r3");

    REQUIRE(DisassembleArm(0xE12002A1) == "smulwb r0, r1, r2");
    REQUIRE(DisassembleArm(0xE12002E1) == "smulwt r0, r1, r2");

    REQUIRE(DisassembleArm(0xE1410382) == "smlalbb r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE14103C2) == "smlalbt r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE14103A2) == "smlaltb r0, r1, r2, r3");
    REQUIRE(DisassembleArm(0xE14103E2) == "smlaltt r0, r1, r2, r3");

    REQUIRE(DisassembleArm(0xE1600281) == "smulbb r0, r1, r2");
    REQUIRE(DisassembleArm(0xE16002C1) == "smulbt r0, r1, r2");
    REQUIRE(DisassembleArm(0xE16002A1) == "smultb r0, r1, r2");
    REQUIRE(DisassembleArm(0xE16002E1) == "smultt r0, r1, r2");
}

TEST_CASE("Disassemble multiply and multiply accumulate instructions", "[arm][disassembler]") {
    REQUIRE(DisassembleArm(0xE0214392) == "mla r1, r2, r3, r4");
    REQUIRE(DisassembleArm(0xE0314392) == "mlas r1, r2, r3, r4");

    REQUIRE(DisassembleArm(0xE0010392) == "mul r1, r2, r3");
    REQUIRE(DisassembleArm(0xE0110392) == "muls r1, r2, r3");

    // TODO: MLS should be here whenever it's supported.

    REQUIRE(DisassembleArm(0xE0E21493) == "smlal r1, r2, r3, r4");
    REQUIRE(DisassembleArm(0xE0F21493) == "smlals r1, r2, r3, r4");

    REQUIRE(DisassembleArm(0xE0C21493) == "smull r1, r2, r3, r4");
    REQUIRE(DisassembleArm(0xE0D21493) == "smulls r1, r2, r3, r4");

    REQUIRE(DisassembleArm(0xE0421493) == "umaal r1, r2, r3, r4");

    REQUIRE(DisassembleArm(0xE0A21493) == "umlal r1, r2, r3, r4");
    REQUIRE(DisassembleArm(0xE0B21493) == "umlals r1, r2, r3, r4");

    REQUIRE(DisassembleArm(0xE0821493) == "umull r1, r2, r3, r4");
    REQUIRE(DisassembleArm(0xE0921493) == "umulls r1, r2, r3, r4");
}

TEST_CASE("Disassemble synchronization primitive instructions", "[arm][disassembler]") {
    REQUIRE(DisassembleArm(0xE1921F9F) == "ldrex r1, [r2]");
    REQUIRE(DisassembleArm(0xE1D21F9F) == "ldrexb r1, [r2]");
    REQUIRE(DisassembleArm(0xE1B31F9F) == "ldrexd r1, r2, [r3]");
    REQUIRE(DisassembleArm(0xE1F21F9F) == "ldrexh r1, [r2]");

    REQUIRE(DisassembleArm(0xE1831F92) == "strex r1, r2, [r3]");
    REQUIRE(DisassembleArm(0xE1C31F92) == "strexb r1, r2, [r3]");
    REQUIRE(DisassembleArm(0xE1A41F92) == "strexd r1, r2, r3, [r4]");
    REQUIRE(DisassembleArm(0xE1E31F92) == "strexh r1, r2, [r3]");

    REQUIRE(DisassembleArm(0xE1031092) == "swp r1, r2, [r3]");
    REQUIRE(DisassembleArm(0xE1431092) == "swpb r1, r2, [r3]");
}

TEST_CASE("Disassemble load / store multiple instructions", "[arm][disassembler]") {
    REQUIRE(DisassembleArm(0xE92D500F) == "stmdb sp!, {r0, r1, r2, r3, r12, lr}");
}
