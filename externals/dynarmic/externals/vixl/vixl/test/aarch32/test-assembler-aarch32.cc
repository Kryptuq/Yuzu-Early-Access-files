// Copyright 2017, VIXL authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstdio>
#include <iostream>
#include <string>

#include "test-runner.h"
#include "test-utils.h"
#include "aarch32/test-utils-aarch32.h"

#include "aarch32/disasm-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

namespace vixl {
namespace aarch32 {

#define STRINGIFY(x) #x

#ifdef VIXL_INCLUDE_TARGET_A32_ONLY
#define TEST_T32(Name) \
  void Test##Name##Impl(InstructionSet isa __attribute__((unused)))
#else
// Tests declared with this macro will only target T32.
#define TEST_T32(Name)                                          \
  void Test##Name##Impl(InstructionSet isa);                    \
  void Test##Name() { Test##Name##Impl(T32); }                  \
  Test test_##Name(STRINGIFY(AARCH32_T32_##Name), &Test##Name); \
  void Test##Name##Impl(InstructionSet isa __attribute__((unused)))
#endif

#ifdef VIXL_INCLUDE_TARGET_T32_ONLY
#define TEST_A32(Name) \
  void Test##Name##Impl(InstructionSet isa __attribute__((unused)))
#else
// Test declared with this macro will only target A32.
#define TEST_A32(Name)                                          \
  void Test##Name##Impl(InstructionSet isa);                    \
  void Test##Name() { Test##Name##Impl(A32); }                  \
  Test test_##Name(STRINGIFY(AARCH32_A32_##Name), &Test##Name); \
  void Test##Name##Impl(InstructionSet isa __attribute__((unused)))
#endif

// Tests declared with this macro will be run twice: once targeting A32 and
// once targeting T32.
#if defined(VIXL_INCLUDE_TARGET_A32_ONLY)
#define TEST(Name) TEST_A32(Name)
#elif defined(VIXL_INCLUDE_TARGET_T32_ONLY)
#define TEST(Name) TEST_T32(Name)
#else
#define TEST(Name)                                              \
  void Test##Name##Impl(InstructionSet isa);                    \
  void Test##Name() {                                           \
    Test##Name##Impl(A32);                                      \
    printf(" > A32 done\n");                                    \
    Test##Name##Impl(T32);                                      \
    printf(" > T32 done\n");                                    \
  }                                                             \
  Test test_##Name(STRINGIFY(AARCH32_ASM_##Name), &Test##Name); \
  void Test##Name##Impl(InstructionSet isa __attribute__((unused)))
#endif

// Tests declared with this macro are not expected to use any provided test
// helpers such as SETUP, RUN, etc.
#define TEST_NOASM(Name)                                    \
  void Test##Name();                                        \
  Test test_##Name(STRINGIFY(AARCH32_##Name), &Test##Name); \
  void Test##Name()

#define __ masm.
#define __TESTOBJ test.
#define BUF_SIZE (4096)

#define CHECK_POOL_SIZE(size)                    \
  do {                                           \
    VIXL_CHECK(__TESTOBJ GetPoolSize() == size); \
  } while (false)

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
// No simulator yet.

#define SETUP()                       \
  MacroAssembler masm(BUF_SIZE, isa); \
  TestMacroAssembler test(&masm);

#define START() masm.GetBuffer()->Reset();

#define END() \
  __ Hlt(0);  \
  __ FinalizeCode();

#define RUN() DISASSEMBLE();

#else  // ifdef VIXL_INCLUDE_SIMULATOR_AARCH32.

#define SETUP()                       \
  RegisterDump core;                  \
  MacroAssembler masm(BUF_SIZE, isa); \
  TestMacroAssembler test(&masm);     \
  UseScratchRegisterScope harness_scratch;

#define START()                 \
  harness_scratch.Open(&masm);  \
  harness_scratch.ExcludeAll(); \
  masm.GetBuffer()->Reset();    \
  __ Push(r4);                  \
  __ Push(r5);                  \
  __ Push(r6);                  \
  __ Push(r7);                  \
  __ Push(r8);                  \
  __ Push(r9);                  \
  __ Push(r10);                 \
  __ Push(r11);                 \
  __ Push(ip);                  \
  __ Push(lr);                  \
  __ Mov(r0, 0);                \
  __ Msr(APSR_nzcvq, r0);       \
  __ Vmsr(FPSCR, r0);           \
  harness_scratch.Include(ip);

#define END()                  \
  harness_scratch.Exclude(ip); \
  core.Dump(&masm);            \
  __ Pop(lr);                  \
  __ Pop(ip);                  \
  __ Pop(r11);                 \
  __ Pop(r10);                 \
  __ Pop(r9);                  \
  __ Pop(r8);                  \
  __ Pop(r7);                  \
  __ Pop(r6);                  \
  __ Pop(r5);                  \
  __ Pop(r4);                  \
  __ Bx(lr);                   \
  __ FinalizeCode();           \
  harness_scratch.Close();

// Execute the generated code from the MacroAssembler's automatic code buffer.
// Note the offset for ExecuteMemory since the PCS requires that
// the address be odd in the case of branching to T32 code.
#define RUN()                                                 \
  DISASSEMBLE();                                              \
  {                                                           \
    int pcs_offset = masm.IsUsingT32() ? 1 : 0;               \
    masm.GetBuffer()->SetExecutable();                        \
    ExecuteMemory(masm.GetBuffer()->GetStartAddress<byte*>(), \
                  masm.GetSizeOfCodeGenerated(),              \
                  pcs_offset);                                \
    masm.GetBuffer()->SetWritable();                          \
  }

#endif  // ifdef VIXL_INCLUDE_SIMULATOR_AARCH32

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
// No simulator yet. We can't test the results.

#define ASSERT_EQUAL_32(expected, result)

#define ASSERT_EQUAL_64(expected, result)

#define ASSERT_EQUAL_128(expected_h, expected_l, result)

#define ASSERT_EQUAL_FP32(expected, result)

#define ASSERT_EQUAL_FP64(expected, result)

#define ASSERT_EQUAL_NZCV(expected)

#else

#define ASSERT_EQUAL_32(expected, result) \
  VIXL_CHECK(Equal32(expected, &core, result))

#define ASSERT_EQUAL_64(expected, result) \
  VIXL_CHECK(Equal64(expected, &core, result))

#define ASSERT_EQUAL_128(expected_h, expected_l, result) \
  VIXL_CHECK(Equal128(expected_h, expected_l, &core, result))

#define ASSERT_EQUAL_FP32(expected, result) \
  VIXL_CHECK(EqualFP32(expected, &core, result))

#define ASSERT_EQUAL_FP64(expected, result) \
  VIXL_CHECK(EqualFP64(expected, &core, result))

#define ASSERT_EQUAL_NZCV(expected) \
  VIXL_CHECK(EqualNzcv(expected, core.flags_nzcv()))

#endif

#define DISASSEMBLE()                                                          \
  if (Test::disassemble()) {                                                   \
    PrintDisassembler dis(std::cout, 0);                                       \
    if (masm.IsUsingT32()) {                                                   \
      dis.DisassembleT32Buffer(masm.GetBuffer()->GetStartAddress<uint16_t*>(), \
                               masm.GetCursorOffset());                        \
    } else {                                                                   \
      dis.DisassembleA32Buffer(masm.GetBuffer()->GetStartAddress<uint32_t*>(), \
                               masm.GetCursorOffset());                        \
    }                                                                          \
  }


// TODO: Add SBC to the ADC tests.


TEST(adc_shift) {
  SETUP();

  START();
  // Initialize registers.
  __ Mov(r0, 0);
  __ Mov(r1, 1);
  __ Mov(r2, 0x01234567);
  __ Mov(r3, 0xfedcba98);

  // Clear the C flag.
  __ Adds(r0, r0, 0);

  __ Adc(r4, r2, r3);
  __ Adc(r5, r0, Operand(r1, LSL, 30));
  __ Adc(r6, r0, Operand(r2, LSR, 16));
  __ Adc(r7, r2, Operand(r3, ASR, 4));
  __ Adc(r8, r2, Operand(r3, ROR, 8));
  __ Adc(r9, r2, Operand(r3, RRX));
  END();

  RUN();

  ASSERT_EQUAL_32(0xffffffff, r4);
  ASSERT_EQUAL_32(INT32_C(1) << 30, r5);
  ASSERT_EQUAL_32(0x00000123, r6);
  ASSERT_EQUAL_32(0x01111110, r7);
  ASSERT_EQUAL_32(0x9a222221, r8);
  ASSERT_EQUAL_32(0x8091a2b3, r9);

  START();
  // Initialize registers.
  __ Mov(r0, 0);
  __ Mov(r1, 1);
  __ Mov(r2, 0x01234567);
  __ Mov(r3, 0xfedcba98);
  __ Mov(r4, 0xffffffff);

  // Set the C flag.
  __ Adds(r0, r4, r1);

  __ Adc(r5, r2, r3);
  __ Adc(r6, r0, Operand(r1, LSL, 30));
  __ Adc(r7, r0, Operand(r2, LSR, 16));
  __ Adc(r8, r2, Operand(r3, ASR, 4));
  __ Adc(r9, r2, Operand(r3, ROR, 8));
  __ Adc(r10, r2, Operand(r3, RRX));
  END();

  RUN();

  ASSERT_EQUAL_32(0xffffffff + 1, r5);
  ASSERT_EQUAL_32((INT32_C(1) << 30) + 1, r6);
  ASSERT_EQUAL_32(0x00000123 + 1, r7);
  ASSERT_EQUAL_32(0x01111110 + 1, r8);
  ASSERT_EQUAL_32(0x9a222221 + 1, r9);
  ASSERT_EQUAL_32(0x0091a2b3 + 1, r10);

  // Check that adc correctly sets the condition flags.
  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xffffffff);
  __ Mov(r2, 1);

  // Clear the C flag.
  __ Adds(r0, r0, 0);
  __ Adcs(r3, r2, r1);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0, r3);

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0x80000000);
  __ Mov(r2, 1);

  // Clear the C flag.
  __ Adds(r0, r0, 0);
  __ Adcs(r3, r2, Operand(r1, ASR, 31));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0, r3);

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0x80000000);
  __ Mov(r2, 0xffffffff);

  // Clear the C flag.
  __ Adds(r0, r0, 0);
  __ Adcs(r3, r2, Operand(r1, LSR, 31));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0, r3);

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0x07ffffff);
  __ Mov(r2, 0x10);

  // Clear the C flag.
  __ Adds(r0, r0, 0);
  __ Adcs(r3, r2, Operand(r1, LSL, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NVFlag);
  ASSERT_EQUAL_32(0x080000000, r3);

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xffffff00);
  __ Mov(r2, 0xff000001);

  // Clear the C flag.
  __ Adds(r0, r0, 0);
  __ Adcs(r3, r2, Operand(r1, ROR, 8));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0, r3);

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xffffffff);
  __ Mov(r2, 0x1);

  // Clear the C flag, forcing RRX to insert 0 in r1's most significant bit.
  __ Adds(r0, r0, 0);
  __ Adcs(r3, r2, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NVFlag);
  ASSERT_EQUAL_32(0x80000000, r3);

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xffffffff);
  __ Mov(r2, 0x1);

  // Set the C flag, forcing RRX to insert 1 in r1's most significant bit.
  __ Adds(r0, r1, r2);
  __ Adcs(r3, r2, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(CFlag);
  ASSERT_EQUAL_32(1, r3);
}


TEST(adc_wide_imm) {
  SETUP();

  START();
  __ Mov(r0, 0);

  // Clear the C flag.
  __ Adds(r0, r0, 0);

  __ Adc(r1, r0, 0x12345678);
  __ Adc(r2, r0, 0xffffffff);

  // Set the C flag.
  __ Cmp(r0, r0);

  __ Adc(r3, r0, 0x12345678);
  __ Adc(r4, r0, 0xffffffff);
  END();

  RUN();

  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0xffffffff, r2);
  ASSERT_EQUAL_32(0x12345678 + 1, r3);
  ASSERT_EQUAL_32(0, r4);
}


// TODO: Add SUB tests to the ADD tests.


TEST(add_imm) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0x1111);
  __ Mov(r2, 0xffffffff);
  __ Mov(r3, 0x80000000);

  __ Add(r4, r0, 0x12);
  __ Add(r5, r1, 0x120000);
  __ Add(r6, r0, 0xab << 12);
  __ Add(r7, r2, 1);

  END();

  RUN();

  ASSERT_EQUAL_32(0x12, r4);
  ASSERT_EQUAL_32(0x121111, r5);
  ASSERT_EQUAL_32(0xab000, r6);
  ASSERT_EQUAL_32(0x0, r7);
}


TEST(add_wide_imm) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 1);

  __ Add(r2, r0, 0x12345678);
  __ Add(r3, r1, 0xffff);
  END();

  RUN();

  ASSERT_EQUAL_32(0x12345678, r2);
  ASSERT_EQUAL_32(0x00010000, r3);
}


TEST(add_shifted) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0x01234567);
  __ Mov(r2, 0x76543210);
  __ Mov(r3, 0xffffffff);

  __ Add(r4, r1, r2);
  __ Add(r5, r0, Operand(r1, LSL, 8));
  __ Add(r6, r0, Operand(r1, LSR, 8));
  __ Add(r7, r0, Operand(r1, ASR, 8));
  __ Add(r8, r3, Operand(r1, ROR, 8));

  // Set the C flag.
  __ Adds(r0, r3, 1);
  __ Add(r9, r3, Operand(r1, RRX));

  // Clear the C flag.
  __ Adds(r0, r0, 0);
  __ Add(r10, r3, Operand(r1, RRX));

  END();

  RUN();

  ASSERT_EQUAL_32(0x77777777, r4);
  ASSERT_EQUAL_32(0x23456700, r5);
  ASSERT_EQUAL_32(0x00012345, r6);
  ASSERT_EQUAL_32(0x00012345, r7);
  ASSERT_EQUAL_32(0x67012344, r8);
  ASSERT_EQUAL_32(0x8091a2b2, r9);
  ASSERT_EQUAL_32(0x0091a2b2, r10);
}


TEST(and_) {
  SETUP();

  START();
  __ Mov(r0, 0x0000fff0);
  __ Mov(r1, 0xf00000ff);
  __ Mov(r2, 0xffffffff);

  __ And(r3, r0, r1);
  __ And(r4, r0, Operand(r1, LSL, 4));
  __ And(r5, r0, Operand(r1, LSR, 1));
  __ And(r6, r0, Operand(r1, ASR, 20));
  __ And(r7, r0, Operand(r1, ROR, 28));
  __ And(r8, r0, 0xff);

  // Set the C flag.
  __ Adds(r9, r2, 1);
  __ And(r9, r1, Operand(r1, RRX));

  // Clear the C flag.
  __ Adds(r10, r0, 0);
  __ And(r10, r1, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_32(0x000000f0, r3);
  ASSERT_EQUAL_32(0x00000ff0, r4);
  ASSERT_EQUAL_32(0x00000070, r5);
  ASSERT_EQUAL_32(0x0000ff00, r6);
  ASSERT_EQUAL_32(0x00000ff0, r7);
  ASSERT_EQUAL_32(0x000000f0, r8);
  ASSERT_EQUAL_32(0xf000007f, r9);
  ASSERT_EQUAL_32(0x7000007f, r10);
}


TEST(ands) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xf00000ff);

  __ Ands(r0, r1, r1);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NFlag);
  ASSERT_EQUAL_32(0xf00000ff, r0);

  START();
  __ Mov(r0, 0x00fff000);
  __ Mov(r1, 0xf00000ff);

  __ Ands(r0, r0, Operand(r1, LSL, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0x00000000, r0);

  START();
  __ Mov(r0, 0x0000fff0);
  __ Mov(r1, 0xf00000ff);

  __ Ands(r0, r0, Operand(r1, LSR, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0x00000000, r0);

  START();
  __ Mov(r0, 0xf000fff0);
  __ Mov(r1, 0xf00000ff);

  __ Ands(r0, r0, Operand(r1, ASR, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NCFlag);
  ASSERT_EQUAL_32(0xf0000000, r0);

  START();
  __ Mov(r0, 0x80000000);
  __ Mov(r1, 0x00000001);

  __ Ands(r0, r0, Operand(r1, ROR, 1));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NCFlag);
  ASSERT_EQUAL_32(0x80000000, r0);

  START();
  __ Mov(r0, 0x80000000);
  __ Mov(r1, 0x80000001);

  // Clear the C flag, forcing RRX to insert 0 in r1's most significant bit.
  __ Adds(r2, r0, 0);
  __ Ands(r2, r0, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0, r2);

  START();
  __ Mov(r0, 0x80000000);
  __ Mov(r1, 0x80000001);
  __ Mov(r2, 0xffffffff);

  // Set the C flag, forcing RRX to insert 1 in r1's most significant bit.
  __ Adds(r2, r2, 1);
  __ Ands(r2, r0, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NCFlag);
  ASSERT_EQUAL_32(0x80000000, r2);

  START();
  __ Mov(r0, 0xfff0);

  __ Ands(r0, r0, 0xf);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZFlag);
  ASSERT_EQUAL_32(0x00000000, r0);

  START();
  __ Mov(r0, 0xff000000);

  __ Ands(r0, r0, 0x80000000);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NCFlag);
  ASSERT_EQUAL_32(0x80000000, r0);
}


TEST(adr_in_range) {
  SETUP();

  Label label_1, label_2, label_3, label_4;

  START();
  {
    size_t size_of_generated_code;
    if (masm.IsUsingA32()) {
      size_of_generated_code = 18 * kA32InstructionSizeInBytes;
    } else {
      size_of_generated_code = 18 * k32BitT32InstructionSizeInBytes +
                               3 * k16BitT32InstructionSizeInBytes;
    }
    ExactAssemblyScope scope(&masm,
                             size_of_generated_code,
                             ExactAssemblyScope::kExactSize);

    __ mov(r0, 0x0);  // Set to zero to indicate success.
    __ adr(r1, &label_3);

    __ adr(r2, &label_1);  // Multiple forward references to the same label.
    __ adr(r3, &label_1);
    __ adr(r4, &label_1);

    __ bind(&label_2);
    __ eor(r5, r2, r3);  // Ensure that r2, r3 and r4 are identical.
    __ eor(r6, r2, r4);
    __ orr(r0, r5, r6);
    if (masm.IsUsingT32()) {
      // The jump target needs to have its least significant bit set to indicate
      // that we are jumping into thumb mode.
      __ orr(r2, r2, 1);
    }
    __ bx(r2);  // label_1, label_3

    __ bind(&label_3);
    __ adr(r2, &label_3);  // Self-reference (offset 0).
    __ eor(r1, r1, r2);
    __ adr(r2, &label_4);  // Simple forward reference.
    if (masm.IsUsingT32()) {
      // The jump target needs to have its least significant bit set to indicate
      // that we are jumping into thumb mode.
      __ orr(r2, r2, 1);
    }
    __ bx(r2);  // label_4

    __ bind(&label_1);
    __ adr(r2, &label_3);  // Multiple reverse references to the same label.
    __ adr(r3, &label_3);
    __ adr(r4, &label_3);
    __ adr(r5, &label_2);  // Simple reverse reference.
    if (masm.IsUsingT32()) {
      // The jump target needs to have its least significant bit set to indicate
      // that we are jumping into thumb mode.
      __ orr(r5, r5, 1);
    }
    __ bx(r5);  // label_2

    __ bind(&label_4);
  }
  END();

  RUN();

  ASSERT_EQUAL_32(0x0, r0);
  ASSERT_EQUAL_32(0x0, r1);
}


// Check that we can use adr with any alignement.
TEST(adr_unaligned) {
  SETUP();

  Label label_end;

  START();
  {
    Location label_0, label_1, label_2, label_3;
    // 5 instructions.
    ExactAssemblyScope scope(&masm,
                             5 * kA32InstructionSizeInBytes + 4,
                             ExactAssemblyScope::kExactSize);
    __ adr(Wide, r0, &label_0);
    __ adr(Wide, r1, &label_1);
    __ adr(Wide, r2, &label_2);
    __ adr(Wide, r3, &label_3);
    __ b(Wide, &label_end);
    __ bind(&label_0);
    __ GetBuffer()->EmitData("a", 1);
    __ bind(&label_1);
    __ GetBuffer()->EmitData("b", 1);
    __ bind(&label_2);
    __ GetBuffer()->EmitData("c", 1);
    __ bind(&label_3);
    __ GetBuffer()->EmitData("d", 1);
  }
  {
    __ Bind(&label_end);
    __ Ldrb(r0, MemOperand(r0));
    __ Ldrb(r1, MemOperand(r1));
    __ Ldrb(r2, MemOperand(r2));
    __ Ldrb(r3, MemOperand(r3));
  }
  END();

  RUN();

  ASSERT_EQUAL_32('a', r0);
  ASSERT_EQUAL_32('b', r1);
  ASSERT_EQUAL_32('c', r2);
  ASSERT_EQUAL_32('d', r3);
}


TEST(shift_imm) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xfedcba98);
  __ Mov(r2, 0xffffffff);

  __ Lsl(r3, r1, 4);
  __ Lsr(r4, r1, 8);
  __ Asr(r5, r1, 16);
  __ Ror(r6, r1, 20);
  END();

  RUN();

  ASSERT_EQUAL_32(0xedcba980, r3);
  ASSERT_EQUAL_32(0x00fedcba, r4);
  ASSERT_EQUAL_32(0xfffffedc, r5);
  ASSERT_EQUAL_32(0xcba98fed, r6);
}


TEST(shift_reg) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xfedcba98);
  __ Mov(r2, 0xffffffff);

  __ Add(r9, r0, 4);
  __ Lsl(r3, r1, r9);

  __ Add(r9, r0, 8);
  __ Lsr(r4, r1, r9);

  __ Add(r9, r0, 16);
  __ Asr(r5, r1, r9);

  __ Add(r9, r0, 20);
  __ Ror(r6, r1, r9);

  // Set the C flag.
  __ Adds(r7, r2, 1);
  __ Rrx(r7, r1);

  // Clear the C flag.
  __ Adds(r8, r0, 0);
  __ Rrx(r8, r1);
  END();

  RUN();

  ASSERT_EQUAL_32(0xedcba980, r3);
  ASSERT_EQUAL_32(0x00fedcba, r4);
  ASSERT_EQUAL_32(0xfffffedc, r5);
  ASSERT_EQUAL_32(0xcba98fed, r6);
  ASSERT_EQUAL_32(0xff6e5d4c, r7);
  ASSERT_EQUAL_32(0x7f6e5d4c, r8);
}


TEST(branch_cond) {
  SETUP();

  Label done, wrong;

  START();
  __ Mov(r0, 0x0);
  __ Mov(r1, 0x1);
  __ Mov(r2, 0x80000000);
  // TODO: Use r0 instead of r3 when r0 becomes available.
  __ Mov(r3, 0x1);

  // For each 'cmp' instruction below, condition codes other than the ones
  // following it would branch.

  __ Cmp(r1, 0);
  __ B(eq, &wrong);
  __ B(lo, &wrong);
  __ B(mi, &wrong);
  __ B(vs, &wrong);
  __ B(ls, &wrong);
  __ B(lt, &wrong);
  __ B(le, &wrong);
  Label ok_1;
  __ B(ne, &ok_1);
  // TODO: Use __ Mov(r0, 0x0) instead.
  __ Add(r3, r0, 0x0);
  __ Bind(&ok_1);

  __ Cmp(r1, 1);
  __ B(ne, &wrong);
  __ B(lo, &wrong);
  __ B(mi, &wrong);
  __ B(vs, &wrong);
  __ B(hi, &wrong);
  __ B(lt, &wrong);
  __ B(gt, &wrong);
  Label ok_2;
  __ B(pl, &ok_2);
  // TODO: Use __ Mov(r0, 0x0) instead.
  __ Add(r3, r0, 0x0);
  __ Bind(&ok_2);

  __ Cmp(r1, 2);
  __ B(eq, &wrong);
  __ B(hs, &wrong);
  __ B(pl, &wrong);
  __ B(vs, &wrong);
  __ B(hi, &wrong);
  __ B(ge, &wrong);
  __ B(gt, &wrong);
  Label ok_3;
  __ B(vc, &ok_3);
  // TODO: Use __ Mov(r0, 0x0) instead.
  __ Add(r3, r0, 0x0);
  __ Bind(&ok_3);

  __ Cmp(r2, 1);
  __ B(eq, &wrong);
  __ B(lo, &wrong);
  __ B(mi, &wrong);
  __ B(vc, &wrong);
  __ B(ls, &wrong);
  __ B(ge, &wrong);
  __ B(gt, &wrong);
  Label ok_4;
  __ B(le, &ok_4);
  // TODO: Use __ Mov(r0, 0x0) instead.
  __ Add(r3, r0, 0x0);
  __ Bind(&ok_4);

  Label ok_5;
  __ B(&ok_5);
  // TODO: Use __ Mov(r0, 0x0) instead.
  __ Add(r3, r0, 0x0);
  __ Bind(&ok_5);

  __ B(&done);

  __ Bind(&wrong);
  // TODO: Use __ Mov(r0, 0x0) instead.
  __ Add(r3, r0, 0x0);

  __ Bind(&done);
  END();

  RUN();

  // TODO: Use r0.
  ASSERT_EQUAL_32(0x1, r3);
}


TEST(bfc_bfi) {
  SETUP();

  START();
  __ Mov(r0, 0xffffffff);
  __ Mov(r1, 0x01234567);
  __ Mov(r2, 0x0);

  __ Bfc(r0, 0, 3);
  __ Bfc(r0, 16, 5);

  __ Bfi(r2, r1, 0, 8);
  __ Bfi(r2, r1, 16, 16);
  END();

  RUN();

  ASSERT_EQUAL_32(0xffe0fff8, r0);
  ASSERT_EQUAL_32(0x45670067, r2);
}


TEST(bic) {
  SETUP();

  START();
  __ Mov(r0, 0xfff0);
  __ Mov(r1, 0xf00000ff);
  __ Mov(r2, 0xffffffff);

  __ Bic(r3, r0, r1);
  __ Bic(r4, r0, Operand(r1, LSL, 4));
  __ Bic(r5, r0, Operand(r1, LSR, 1));
  __ Bic(r6, r0, Operand(r1, ASR, 20));
  __ Bic(r7, r0, Operand(r1, ROR, 28));
  __ Bic(r8, r0, 0x1f);

  // Set the C flag.
  __ Adds(r9, r2, 1);
  __ Bic(r9, r1, Operand(r1, RRX));

  // Clear the C flag.
  __ Adds(r10, r0, 0);
  __ Bic(r10, r1, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_32(0x0000ff00, r3);
  ASSERT_EQUAL_32(0x0000f000, r4);
  ASSERT_EQUAL_32(0x0000ff80, r5);
  ASSERT_EQUAL_32(0x000000f0, r6);
  ASSERT_EQUAL_32(0x0000f000, r7);
  ASSERT_EQUAL_32(0x0000ffe0, r8);
  ASSERT_EQUAL_32(0x00000080, r9);
  ASSERT_EQUAL_32(0x80000080, r10);
}


TEST(bics) {
  SETUP();

  START();
  __ Mov(r0, 0);
  __ Mov(r1, 0xf00000ff);

  __ Bics(r0, r1, r1);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZFlag);
  ASSERT_EQUAL_32(0, r0);

  START();
  __ Mov(r0, 0x00fff000);
  __ Mov(r1, 0x0fffff00);

  __ Bics(r0, r0, Operand(r1, LSL, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZFlag);
  ASSERT_EQUAL_32(0x00000000, r0);

  START();
  __ Mov(r0, 0x0000fff0);
  __ Mov(r1, 0x0fffff00);

  __ Bics(r0, r0, Operand(r1, LSR, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZFlag);
  ASSERT_EQUAL_32(0x00000000, r0);

  START();
  __ Mov(r0, 0xf000fff0);
  __ Mov(r1, 0x0fffff00);

  __ Bics(r0, r0, Operand(r1, ASR, 4));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NFlag);
  ASSERT_EQUAL_32(0xf0000000, r0);

  START();
  __ Mov(r0, 0x80000000);
  __ Mov(r1, 0xfffffffe);

  __ Bics(r0, r0, Operand(r1, ROR, 1));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NFlag);
  ASSERT_EQUAL_32(0x80000000, r0);

  START();
  __ Mov(r0, 0x80000000);
  __ Mov(r1, 0x80000001);

  // Clear the C flag, forcing RRX to insert 0 in r1's most significant bit.
  __ Adds(r2, r0, 0);
  __ Bics(r2, r0, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NCFlag);
  ASSERT_EQUAL_32(0x80000000, r2);

  START();
  __ Mov(r0, 0x80000000);
  __ Mov(r1, 0x80000001);
  __ Mov(r2, 0xffffffff);

  // Set the C flag, forcing RRX to insert 1 in r1's most significant bit.
  __ Adds(r2, r2, 1);
  __ Bics(r2, r0, Operand(r1, RRX));
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZCFlag);
  ASSERT_EQUAL_32(0, r2);

  START();
  __ Mov(r0, 0xf000);

  __ Bics(r0, r0, 0xf000);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(ZFlag);
  ASSERT_EQUAL_32(0x00000000, r0);

  START();
  __ Mov(r0, 0xff000000);

  __ Bics(r0, r0, 0x7fffffff);
  END();

  RUN();

  ASSERT_EQUAL_NZCV(NFlag);
  ASSERT_EQUAL_32(0x80000000, r0);
}

// Make sure calling a macro-assembler instruction will generate literal pools
// if needed.
TEST_T32(veneer_pool_generated_by_macro_instruction) {
  SETUP();

  START();

  Label start, end;

  VIXL_CHECK(test.PoolIsEmpty());

  __ Mov(r0, 1);

  __ Bind(&start);
  __ Cbz(r0, &end);

  VIXL_CHECK(!test.PoolIsEmpty());

  // Generate enough code so that, after the loop, no instruction can be
  // generated before we need to generate the veneer pool.
  // Use `ExactAssemblyScope` and the assembler to generate the code.
  int32_t space = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  {
    ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
    while (space > 0) {
      __ nop();
      space -= k16BitT32InstructionSizeInBytes;
    }
  }

  // We should not have emitted the pool at this point.
  VIXL_CHECK(!test.PoolIsEmpty());
  VIXL_CHECK(test.GetPoolCheckpoint() == masm.GetCursorOffset());

  // Now the pool will need to be generated before we can emit anything.
  Label check;
  __ Bind(&check);
  __ Mov(r0, 0);
  // We should have generated 3 wide instructions:
  //     b.w past_veneer_pool
  //     b.w end ;; veneer from CBZ to "end".
  //   past_veneer_pool:
  //     mov r0, #0
  VIXL_CHECK(masm.GetSizeOfCodeGeneratedSince(&check) ==
             (3 * k32BitT32InstructionSizeInBytes));

  // Branch back to make sure the veneers work.
  __ B(&start);
  __ Bind(&end);

  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  ASSERT_EQUAL_32(0, r0);
}

// NOTE: This test has needed modifications for the new pool manager, as it
// was testing a corner case of the previous pool managers. We keep it as
// another testcase.
TEST(emit_reused_load_literal) {
  SETUP();

  START();

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  const int ldrd_range = masm.IsUsingA32() ? 255 : 1020;
  const int string_size = AlignUp(ldrd_range + kMaxInstructionSizeInBytes, 4);
  std::string test_string(string_size, 'x');
  StringLiteral big_literal(test_string.c_str());
  __ Adr(r4, &big_literal);

  // This load has a wider range than the Ldrd used below for the same
  // literal.
  Literal<uint64_t> l1(0xcafebeefdeadbaba);
  __ Ldr(r0, &l1);

  // With the old pool manager, this Ldrd used to force pool emission before
  // being generated. Now, 'l1' and 'big_literal' can be reordered in the pool,
  // and pool emission is not triggered anymore.
  __ Ldrd(r2, r3, &l1);

  __ Ldr(r4, MemOperand(r4));  // Load the first 4 characters in r4.
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0xdeadbaba, r0);
  ASSERT_EQUAL_32(0xdeadbaba, r2);
  ASSERT_EQUAL_32(0xcafebeef, r3);
  ASSERT_EQUAL_32(0x78787878, r4);
}

// NOTE: This test has needed modifications for the new pool manager, as it
// was testing a corner case of the previous pool managers. We keep it as
// another testcase.
TEST(emit_reused_load_literal_should_not_rewind) {
  // This test checks that we are not conservative when rewinding a load of a
  // literal that is already in the literal pool.
  SETUP();

  START();

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  // This load has a wider range than the Ldrd used below for the same
  // literal.
  Literal<uint64_t> l1(0xcafebeefdeadbaba);
  __ Ldr(r0, &l1);

  // Add a large string to the literal pool, but only *after* l1, so the
  // Ldrd below should not need to rewind.
  const int ldrd_range = masm.IsUsingA32() ? 255 : 1020;
  const int string_size = AlignUp(ldrd_range + kMaxInstructionSizeInBytes, 4);
  std::string test_string(string_size, 'x');
  StringLiteral big_literal(test_string.c_str());
  __ Adr(r4, &big_literal);
  __ Ldrd(r2, r3, &l1);

  // Here we used to check the pool size, which can now be zero as we emit the
  // literals in a different order.

  // Make sure the pool is emitted.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  __ Ldr(r4, MemOperand(r4));  // Load the first 4 characters in r4.
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0xdeadbaba, r0);
  ASSERT_EQUAL_32(0xdeadbaba, r2);
  ASSERT_EQUAL_32(0xcafebeef, r3);
  ASSERT_EQUAL_32(0x78787878, r4);
}


void EmitReusedLoadLiteralStressTest(InstructionSet isa, bool conditional) {
  // This test stresses loading a literal that is already in the literal pool,
  // for various positionings on the existing load from that literal. We try to
  // exercise cases where the two loads result in similar checkpoints for the
  // literal pool.
  SETUP();

  const int ldrd_range = masm.IsUsingA32() ? 255 : 1020;
  const int ldr_range = 4095;
  const int nop_size = masm.IsUsingA32() ? 4 : 2;
  const int nops = (ldr_range - ldrd_range) / nop_size;

  for (int n = nops - 10; n < nops + 10; ++n) {
    START();

    // Make sure the pool is empty.
    masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
    VIXL_CHECK(test.PoolIsEmpty());

    if (conditional) {
      __ Mov(r1, 0);
      __ Cmp(r1, 0);
    }

    // Add a large string to the pool, which will stress corner cases with the
    // Ldrd below (if the pool is not already emitted due to the Ldr).
    const int string_size = AlignUp(ldrd_range + kMaxInstructionSizeInBytes, 4);
    std::string test_string(string_size, 'x');
    StringLiteral big_literal(test_string.c_str());
    __ Ldr(r4, &big_literal);

    // This load has a wider range than the Ldrd used below for the same
    // literal.
    Literal<uint64_t> l1(0xcafebeefdeadbaba);
    __ Ldr(r0, &l1);

    // Generate nops, in order to bring the checkpoints of the Ldr and Ldrd
    // closer.
    {
      ExactAssemblyScope scope(&masm,
                               n * nop_size,
                               ExactAssemblyScope::kExactSize);
      for (int i = 0; i < n; ++i) {
        __ nop();
      }
    }

    if (conditional) {
      __ Ldrd(eq, r2, r3, &l1);
    } else {
      __ Ldrd(r2, r3, &l1);
    }

    // Here we used to check that the pool is empty. Since the new pool manager
    // allows reordering of literals in the pool, this will not always be the
    // case. 'l1' can now be emitted before 'big_literal', allowing the pool to
    // be emitted after the ldrd when the number of nops is small enough.

    END();

    RUN();

    // Check that the literals loaded correctly.
    ASSERT_EQUAL_32(0xdeadbaba, r0);
    ASSERT_EQUAL_32(0xdeadbaba, r2);
    ASSERT_EQUAL_32(0xcafebeef, r3);
    ASSERT_EQUAL_32(0x78787878, r4);
  }
}


TEST(emit_reused_load_literal_stress) {
  EmitReusedLoadLiteralStressTest(isa, false /*conditional*/);
}


TEST(emit_reused_conditional_load_literal_stress) {
  EmitReusedLoadLiteralStressTest(isa, true /*conditional*/);
}


TEST(test_many_loads_from_same_literal) {
  // This test generates multiple loads from the same literal in order to
  // test that the delegate recursion limit is appropriate for Ldrd with
  // large negative offsets.
  SETUP();

  START();

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  Literal<uint64_t> l0(0xcafebeefdeadbaba);
  __ Ldrd(r0, r1, &l0);
  for (int i = 0; i < 10000; ++i) {
    __ Add(r2, r2, i);
    __ Ldrd(r4, r5, &l0);
  }

  __ Ldrd(r2, r3, &l0);

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0xdeadbaba, r0);
  ASSERT_EQUAL_32(0xcafebeef, r1);
  ASSERT_EQUAL_32(0xdeadbaba, r2);
  ASSERT_EQUAL_32(0xcafebeef, r3);
  ASSERT_EQUAL_32(0xdeadbaba, r4);
  ASSERT_EQUAL_32(0xcafebeef, r5);
}


// Make sure calling a macro-assembler instruction will generate literal pools
// if needed.
TEST_T32(literal_pool_generated_by_macro_instruction) {
  SETUP();

  START();

  VIXL_CHECK(test.PoolIsEmpty());

  __ Ldrd(r0, r1, 0x1234567890abcdef);

  VIXL_CHECK(!test.PoolIsEmpty());

  // Generate enough code so that, after the loop, no instruction can be
  // generated before we need to generate the literal pool.
  // Use `ExactAssemblyScope` and the assembler to generate the code.
  int32_t space = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  {
    ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
    while (space > 0) {
      __ nop();
      space -= k16BitT32InstructionSizeInBytes;
    }
  }

  // We should not have emitted the literal pool at this point.
  VIXL_CHECK(!test.PoolIsEmpty());
  VIXL_CHECK(test.GetPoolCheckpoint() == masm.GetCursorOffset());

  // Now the pool will need to be generated before we emit anything.
  Label check;
  __ Bind(&check);
  __ Mov(r2, 0x12345678);
  // We should have generated 3 wide instructions and 8 bytes of data:
  //     b.w past_literal_pool
  //     .bytes 0x1234567890abcdef
  //   past_literal_pool:
  //     mov r2, #22136
  //     movt r2, #4660
  VIXL_CHECK(masm.GetSizeOfCodeGeneratedSince(&check) ==
             (3 * k32BitT32InstructionSizeInBytes + 8));

  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0x12345678, r2);
}

TEST(emit_single_literal) {
  SETUP();

  START();
  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);
  CHECK_POOL_SIZE(8);
  __ Vldr(s0, 1.0);
  __ Vldr(d1, 2.0);
  __ Vmov(d2, 4.1);
  __ Vmov(s8, 8.2);
  CHECK_POOL_SIZE(20);
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_FP32(1.0f, s0);
  ASSERT_EQUAL_FP64(2.0, d1);
  ASSERT_EQUAL_FP64(4.1, d2);
  ASSERT_EQUAL_FP32(8.2f, s8);
}


#undef __
#undef __TESTOBJ
#define __ masm->
#define __TESTOBJ test->


void EmitLdrdLiteralTest(MacroAssembler* masm, TestMacroAssembler* test) {
  const int ldrd_range = masm->IsUsingA32() ? 255 : 1020;
  // We want to emit code up to the maximum literal load range and ensure the
  // pool has not been emitted. Compute the limit (end).
  ptrdiff_t end = AlignDown(
      // Align down the PC to 4 bytes as the instruction does when it's
      // executed.
      // The PC will be the cursor offset plus the architecture state PC
      // offset.
      AlignDown(masm->GetBuffer()->GetCursorOffset() +
                    masm->GetArchitectureStatePCOffset(),
                4) +
          // Maximum range allowed to access the constant.
          ldrd_range -
          // Take into account the branch over the pool.
          kMaxInstructionSizeInBytes,
      // AlignDown to 4 byte as the literals will be 4 byte aligned.
      4);

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);
  CHECK_POOL_SIZE(8);

  int32_t margin = test->GetPoolCheckpoint() - masm->GetCursorOffset();
  VIXL_ASSERT(end == test->GetPoolCheckpoint());
  {
    ExactAssemblyScope scope(masm, margin, ExactAssemblyScope::kExactSize);
    // Opening the scope should not have triggered the emission of the literal
    // pool.
    VIXL_CHECK(!test->PoolIsEmpty());
    while (masm->GetCursorOffset() < end) {
      __ nop();
    }
    VIXL_CHECK(masm->GetCursorOffset() == end);
  }

  // Check that the pool has not been emited along the way.
  CHECK_POOL_SIZE(8);
  // This extra instruction should trigger an emit of the pool.
  __ Nop();
  // The pool should have been emitted.
  VIXL_CHECK(test->PoolIsEmpty());
}

#undef __
#undef __TESTOBJ
#define __ masm.
#define __TESTOBJ test.

// NOTE: This test has needed modifications for the new pool manager, as it
// was testing a corner case of the previous pool managers. We keep it as
// another testcase.
TEST(emit_literal_rewind) {
  SETUP();

  START();

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  EmitLdrdLiteralTest(&masm, &test);

  const int ldrd_range = masm.IsUsingA32() ? 255 : 1020;
  const int string_size = AlignUp(ldrd_range + kMaxInstructionSizeInBytes, 4);
  std::string test_string(string_size, 'x');
  StringLiteral big_literal(test_string.c_str());
  __ Adr(r4, &big_literal);
  __ Ldrd(r2, r3, 0xcafebeefdeadbaba);
  // With the old pool manager, the adr above would overflow the literal pool
  // and force a rewind and pool emission.
  // Here we used to check the pool size to confirm that 'big_literal' had
  // already been emitted. This does not have to be the case now, as we can
  // emit the literals in a different order.

  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());
  __ Ldr(r4, MemOperand(r4));  // Load the first 4 characters in r4.
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0xdeadbaba, r2);
  ASSERT_EQUAL_32(0xcafebeef, r3);
  ASSERT_EQUAL_32(0x78787878, r4);
}


// NOTE: This test has needed modifications for the new pool manager, as it
// was testing a corner case of the previous pool managers. We keep it as
// another testcase.
TEST(emit_literal_conditional_rewind) {
  SETUP();

  START();

  // This test is almost identical to the test above, but the Ldrd instruction
  // is conditional and there is a second conditional Ldrd instruction that will
  // not be executed.

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  const int ldrd_range = masm.IsUsingA32() ? 255 : 1020;
  const int string_size = AlignUp(ldrd_range + kMaxInstructionSizeInBytes, 4);
  std::string test_string(string_size, 'x');
  StringLiteral big_literal(test_string.c_str());
  __ Adr(r2, &big_literal);
  __ Mov(r0, 0);
  __ Mov(r1, 0);
  __ Mov(r3, 1);
  __ Cmp(r3, 1);
  __ Ldrd(eq, r0, r1, 0xcafebeefdeadbaba);
  __ Ldrd(ne, r0, r1, 0xdeadcafebeefbaba);
  // With the old pool manager, the adr above would overflow the literal pool
  // and force a rewind and pool emission.
  // Here we used to check the pool size to confirm that 'big_literal' had
  // already been emitted. This does not have to be the case now, as we can
  // emit the literals in a different order.

  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());
  __ Ldr(r2, MemOperand(r2));  // Load the first 4 characters in r2.
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0xdeadbaba, r0);
  ASSERT_EQUAL_32(0xcafebeef, r1);
  ASSERT_EQUAL_32(0x78787878, r2);
}

enum LiteralStressTestMode {
  kUnconditional,
  kConditionalTrue,
  kConditionalFalse,
  kConditionalBoth
};

// Test loading a literal when the size of the literal pool is close to the
// maximum range of the load, with varying PC values (and alignment, for T32).
// This test is similar to the tests above, with the difference that we allow
// an extra offset to the string size in order to make sure that various pool
// sizes close to the maximum supported offset will produce code that executes
// correctly. As the Ldrd might or might not be emitted before the pool, we do
// not assert on the size of the literal pool in this test.
void EmitLdrdLiteralStressTest(InstructionSet isa,
                               bool unaligned,
                               LiteralStressTestMode test_mode) {
  SETUP();

  for (int offset = -10; offset <= 10; ++offset) {
    START();

    if (unaligned) {
      __ Nop();
      VIXL_ASSERT((masm.GetBuffer()->GetCursorOffset() % 4) == 2);
    }

    // Make sure the pool is empty.
    masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
    VIXL_CHECK(test.PoolIsEmpty());

    const int ldrd_range = masm.IsUsingA32() ? 255 : 1020;
    const int string_size = ldrd_range + offset;
    std::string test_string(string_size - 1, 'x');
    StringLiteral big_literal(test_string.c_str());
    __ Adr(r2, &big_literal);
    __ Mov(r0, 0);
    __ Mov(r1, 0);
    switch (test_mode) {
      case kUnconditional:
        __ Ldrd(r0, r1, 0xcafebeefdeadbaba);
        break;
      case kConditionalTrue:
        __ Mov(r0, 0xffffffff);
        __ Mov(r1, r0);
        __ Mov(r3, 1);
        __ Cmp(r3, 1);
        __ Ldrd(eq, r0, r1, 0xcafebeefdeadbaba);
        break;
      case kConditionalFalse:
        __ Mov(r0, 0xdeadbaba);
        __ Mov(r1, 0xcafebeef);
        __ Mov(r3, 1);
        __ Cmp(r3, 1);
        __ Ldrd(ne, r0, r1, 0xdeadcafebeefbaba);
        break;
      case kConditionalBoth:
        __ Mov(r3, 1);
        __ Cmp(r3, 1);
        __ Ldrd(eq, r0, r1, 0xcafebeefdeadbaba);
        __ Ldrd(ne, r0, r1, 0xdeadcafebeefbaba);
        break;
    }

    masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
    VIXL_CHECK(test.PoolIsEmpty());
    __ Ldr(r2, MemOperand(r2));  // Load the first 4 characters in r2.
    END();

    RUN();

    // Check that the literals loaded correctly.
    ASSERT_EQUAL_32(0xdeadbaba, r0);
    ASSERT_EQUAL_32(0xcafebeef, r1);
    ASSERT_EQUAL_32(0x78787878, r2);
  }
}


TEST(emit_literal_stress) {
  EmitLdrdLiteralStressTest(isa, false /*unaligned*/, kUnconditional);
}


TEST_T32(emit_literal_stress_unaligned) {
  EmitLdrdLiteralStressTest(isa, true /*unaligned*/, kUnconditional);
}


TEST(emit_literal_conditional_stress) {
  EmitLdrdLiteralStressTest(isa, false /*unaligned*/, kConditionalTrue);
  EmitLdrdLiteralStressTest(isa, false /*unaligned*/, kConditionalFalse);
  EmitLdrdLiteralStressTest(isa, false /*unaligned*/, kConditionalBoth);
}


TEST_T32(emit_literal_conditional_stress_unaligned) {
  EmitLdrdLiteralStressTest(isa, true /*unaligned*/, kConditionalTrue);
  EmitLdrdLiteralStressTest(isa, true /*unaligned*/, kConditionalFalse);
  EmitLdrdLiteralStressTest(isa, true /*unaligned*/, kConditionalBoth);
}

TEST_T32(emit_literal_unaligned) {
  SETUP();

  START();

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  // Generate a nop to break the 4 bytes alignment.
  __ Nop();

  EmitLdrdLiteralTest(&masm, &test);

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}

TEST(literal_multiple_uses) {
  SETUP();

  START();
  Literal<int32_t> lit(42);
  __ Ldr(r0, &lit);
  CHECK_POOL_SIZE(4);

  // Multiple uses of the same literal object should not make the
  // pool grow.
  __ Ldrb(r1, &lit);
  __ Ldrsb(r2, &lit);
  __ Ldrh(r3, &lit);
  __ Ldrsh(r4, &lit);
  CHECK_POOL_SIZE(4);

  END();

  RUN();

  ASSERT_EQUAL_32(42, r0);
  ASSERT_EQUAL_32(42, r1);
  ASSERT_EQUAL_32(42, r2);
  ASSERT_EQUAL_32(42, r3);
  ASSERT_EQUAL_32(42, r4);
}


// A test with two loads literal which go out of range at the same time.
TEST_A32(ldr_literal_range_same_time) {
  SETUP();

  START();
  const int ldrd_range = 255;
  // We need to take into account the jump over the pool.
  const int ldrd_padding = ldrd_range - 2 * kA32InstructionSizeInBytes;
  const int ldr_range = 4095;
  // We need to take into account the ldrd padding and the ldrd instruction.
  const int ldr_padding =
      ldr_range - ldrd_padding - 2 * kA32InstructionSizeInBytes;

  __ Ldr(r1, 0x12121212);
  CHECK_POOL_SIZE(4);

  {
    int space = AlignDown(ldr_padding, kA32InstructionSizeInBytes);
    ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
    int32_t end = masm.GetCursorOffset() + space;
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
  }

  __ Ldrd(r2, r3, 0x1234567890abcdef);
  CHECK_POOL_SIZE(12);

  {
    int space = AlignDown(ldrd_padding, kA32InstructionSizeInBytes);
    ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
    for (int32_t end = masm.GetCursorOffset() + space;
         masm.GetCursorOffset() < end;) {
      __ nop();
    }
  }
  CHECK_POOL_SIZE(12);

  // This mov will put the two loads literal out of range and will force
  // the literal pool emission.
  __ Mov(r0, 0);
  VIXL_CHECK(test.PoolIsEmpty());
  END();

  RUN();

  ASSERT_EQUAL_32(0x12121212, r1);
  ASSERT_EQUAL_32(0x90abcdef, r2);
  ASSERT_EQUAL_32(0x12345678, r3);
}


TEST(ldr_literal_mix_types) {
  SETUP();

  START();
  Literal<uint64_t> l0(0x1234567890abcdef);
  Literal<int32_t> l1(0x12345678);
  Literal<uint16_t> l2(1234);
  Literal<int16_t> l3(-678);
  Literal<uint8_t> l4(42);
  Literal<int8_t> l5(-12);

  __ Ldrd(r0, r1, &l0);
  __ Ldr(r2, &l1);
  __ Ldrh(r3, &l2);
  __ Ldrsh(r4, &l3);
  __ Ldrb(r5, &l4);
  __ Ldrsb(r6, &l5);
  // The pool size does not include padding.
  CHECK_POOL_SIZE(18);

  END();

  RUN();

  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0x12345678, r2);
  ASSERT_EQUAL_32(1234, r3);
  ASSERT_EQUAL_32(-678, r4);
  ASSERT_EQUAL_32(42, r5);
  ASSERT_EQUAL_32(-12, r6);
}


TEST(ldr_literal_conditional) {
  SETUP();

  START();
  Literal<uint64_t> l0(0x1234567890abcdef);
  Literal<uint64_t> l0_not_taken(0x90abcdef12345678);
  Literal<int32_t> l1(0x12345678);
  Literal<int32_t> l1_not_taken(0x56781234);
  Literal<uint16_t> l2(1234);
  Literal<uint16_t> l2_not_taken(3412);
  Literal<int16_t> l3(-678);
  Literal<int16_t> l3_not_taken(678);
  Literal<uint8_t> l4(42);
  Literal<uint8_t> l4_not_taken(-42);
  Literal<int8_t> l5(-12);
  Literal<int8_t> l5_not_taken(12);
  Literal<float> l6(1.2345f);
  Literal<float> l6_not_taken(0.0f);
  Literal<double> l7(1.3333);
  Literal<double> l7_not_taken(0.0);

  // Check that conditionally loading literals of different types works
  // correctly for both A32 and T32.
  __ Mov(r7, 1);
  __ Cmp(r7, 1);
  __ Ldrd(eq, r0, r1, &l0);
  __ Ldrd(ne, r0, r1, &l0_not_taken);
  __ Cmp(r7, 0);
  __ Ldr(gt, r2, &l1);
  __ Ldr(le, r2, &l1_not_taken);
  __ Cmp(r7, 2);
  __ Ldrh(lt, r3, &l2);
  __ Ldrh(ge, r3, &l2_not_taken);
  __ Ldrsh(le, r4, &l3);
  __ Ldrsh(gt, r4, &l3_not_taken);
  __ Cmp(r7, 1);
  __ Ldrb(ge, r5, &l4);
  __ Ldrb(lt, r5, &l4_not_taken);
  __ Ldrsb(eq, r6, &l5);
  __ Ldrsb(ne, r6, &l5_not_taken);
  __ Vldr(Condition(eq), s0, &l6);
  __ Vldr(Condition(ne), s0, &l6_not_taken);
  __ Vldr(Condition(eq), d1, &l7);
  __ Vldr(Condition(ne), d1, &l7_not_taken);

  END();

  RUN();

  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0x12345678, r2);
  ASSERT_EQUAL_32(1234, r3);
  ASSERT_EQUAL_32(-678, r4);
  ASSERT_EQUAL_32(42, r5);
  ASSERT_EQUAL_32(-12, r6);
  ASSERT_EQUAL_FP32(1.2345f, s0);
  ASSERT_EQUAL_FP64(1.3333, d1);
}


struct LdrLiteralRangeTest {
  void (MacroAssembler::*instruction)(Register, RawLiteral*);
  Register result_reg;
  int a32_range;
  int t32_range;
  uint32_t literal_value;
  uint32_t test_value;
};


const LdrLiteralRangeTest kLdrLiteralRangeTestData[] =
    {{&MacroAssembler::Ldr, r1, 4095, 4095, 0x12345678, 0x12345678},
     {&MacroAssembler::Ldrh, r2, 255, 4095, 0xabcdefff, 0x0000efff},
     {&MacroAssembler::Ldrsh, r3, 255, 4095, 0x00008765, 0xffff8765},
     {&MacroAssembler::Ldrb, r4, 4095, 4095, 0x12345678, 0x00000078},
     {&MacroAssembler::Ldrsb, r5, 255, 4095, 0x00000087, 0xffffff87}};


void GenerateLdrLiteralTriggerPoolEmission(InstructionSet isa,
                                           bool unaligned_ldr) {
  SETUP();

  for (size_t i = 0; i < ARRAY_SIZE(kLdrLiteralRangeTestData); ++i) {
    const LdrLiteralRangeTest& test_case = kLdrLiteralRangeTestData[i];

    START();

    if (unaligned_ldr) {
      // Generate a nop to break the 4-byte alignment.
      __ Nop();
      VIXL_ASSERT((masm.GetBuffer()->GetCursorOffset() % 4) == 2);
    }

    __ Ldr(r6, 0x12345678);
    CHECK_POOL_SIZE(4);

    // TODO: The MacroAssembler currently checks for more space than required
    // when emitting macro instructions, triggering emission of the pool before
    // absolutely required. For now we keep a buffer. Fix this test when the
    // MacroAssembler becomes precise again.
    int masm_check_margin = 10 * kMaxInstructionSizeInBytes;
    int expected_pool_size = 4;
    while ((test.GetPoolCheckpoint() - masm.GetCursorOffset() -
            masm_check_margin) >=
           static_cast<int32_t>(kMaxInstructionSizeInBytes)) {
      __ Ldr(r7, 0x90abcdef);
      // Each ldr instruction will force a new literal value to be added
      // to the pool. Check that the literal pool grows accordingly.
      expected_pool_size += 4;
      CHECK_POOL_SIZE(expected_pool_size);
    }

    int space = test.GetPoolCheckpoint() - masm.GetCursorOffset();
    int end = masm.GetCursorOffset() + space;
    {
      // Generate nops precisely to fill the buffer.
      ExactAssemblyScope accurate_scope(&masm, space);  // This should not
                                                        // trigger emission of
                                                        // the pool.
      VIXL_CHECK(!test.PoolIsEmpty());
      while (masm.GetCursorOffset() < end) {
        __ nop();
      }
    }

    // This ldr will force the literal pool to be emitted before emitting
    // the load and will create a new pool for the new literal used by this ldr.
    VIXL_CHECK(!test.PoolIsEmpty());
    Literal<uint32_t> literal(test_case.literal_value);
    (masm.*test_case.instruction)(test_case.result_reg, &literal);
    CHECK_POOL_SIZE(4);

    END();

    RUN();

    ASSERT_EQUAL_32(0x12345678, r6);
    ASSERT_EQUAL_32(0x90abcdef, r7);
    ASSERT_EQUAL_32(test_case.test_value, test_case.result_reg);
  }
}


TEST(ldr_literal_trigger_pool_emission) {
  GenerateLdrLiteralTriggerPoolEmission(isa, false);
}


TEST_T32(ldr_literal_trigger_pool_emission_unaligned) {
  GenerateLdrLiteralTriggerPoolEmission(isa, true);
}

void GenerateLdrLiteralRangeTest(InstructionSet isa, bool unaligned_ldr) {
  SETUP();

  for (size_t i = 0; i < ARRAY_SIZE(kLdrLiteralRangeTestData); ++i) {
    const LdrLiteralRangeTest& test_case = kLdrLiteralRangeTestData[i];

    START();

    // Make sure the pool is empty.
    masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
    VIXL_CHECK(test.PoolIsEmpty());

    if (unaligned_ldr) {
      // Generate a nop to break the 4-byte alignment.
      __ Nop();
      VIXL_ASSERT((masm.GetBuffer()->GetCursorOffset() % 4) == 2);
    }

    Literal<uint32_t> literal(test_case.literal_value);
    (masm.*test_case.instruction)(test_case.result_reg, &literal);
    CHECK_POOL_SIZE(4);

    // Generate enough instruction so that we go out of range for the load
    // literal we just emitted.
    ptrdiff_t end =
        masm.GetBuffer()->GetCursorOffset() +
        ((masm.IsUsingA32()) ? test_case.a32_range : test_case.t32_range);
    while (masm.GetBuffer()->GetCursorOffset() < end) {
      __ Mov(r0, 0);
    }

    // The literal pool should have been emitted now.
    VIXL_CHECK(literal.IsBound());
    VIXL_CHECK(test.PoolIsEmpty());

    END();

    RUN();

    ASSERT_EQUAL_32(test_case.test_value, test_case.result_reg);
  }
}


TEST(ldr_literal_range) { GenerateLdrLiteralRangeTest(isa, false); }


TEST_T32(ldr_literal_range_unaligned) {
  GenerateLdrLiteralRangeTest(isa, true);
}


TEST(string_literal) {
  SETUP();

  START();
  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  StringLiteral hello_string("hello");

  __ Ldrb(r1, &hello_string);

  __ Adr(r0, &hello_string);
  __ Ldrb(r2, MemOperand(r0));
  END();

  RUN();

  ASSERT_EQUAL_32('h', r1);
  ASSERT_EQUAL_32('h', r2);
}


TEST(custom_literal_in_pool) {
  SETUP();

  START();
  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  Literal<uint32_t> l0(static_cast<uint32_t>(0x12345678));
  __ Ldr(r0, &l0);
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  __ Ldr(r1, &l0);
  VIXL_CHECK(test.PoolIsEmpty());

  Literal<uint64_t> cafebeefdeadbaba(0xcafebeefdeadbaba);
  __ Ldrd(r8, r9, &cafebeefdeadbaba);
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  __ Ldrd(r2, r3, &cafebeefdeadbaba);
  VIXL_CHECK(test.PoolIsEmpty());

  Literal<uint32_t> l1(0x09abcdef);
  __ Adr(r4, &l1);
  __ Ldr(r4, MemOperand(r4));
  masm.EmitLiteralPool();
  __ Adr(r5, &l1);
  __ Ldr(r5, MemOperand(r5));
  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x12345678, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0xdeadbaba, r2);
  ASSERT_EQUAL_32(0xcafebeef, r3);
  ASSERT_EQUAL_32(0xdeadbaba, r8);
  ASSERT_EQUAL_32(0xcafebeef, r9);
  ASSERT_EQUAL_32(0x09abcdef, r4);
  ASSERT_EQUAL_32(0x09abcdef, r5);
}


TEST(custom_literal_place) {
  SETUP();

  START();
  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  Literal<uint64_t> l0(0xcafebeefdeadbaba, RawLiteral::kManuallyPlaced);
  Literal<int32_t> l1(0x12345678, RawLiteral::kManuallyPlaced);
  Literal<uint16_t> l2(4567, RawLiteral::kManuallyPlaced);
  Literal<int16_t> l3(-4567, RawLiteral::kManuallyPlaced);
  Literal<uint8_t> l4(123, RawLiteral::kManuallyPlaced);
  Literal<int8_t> l5(-123, RawLiteral::kManuallyPlaced);

  __ Ldrd(r0, r1, &l0);
  __ Ldr(r2, &l1);
  __ Ldrh(r3, &l2);
  __ Ldrsh(r4, &l3);
  __ Ldrb(r5, &l4);
  __ Ldrsb(r6, &l5);

  VIXL_CHECK(test.PoolIsEmpty());

  // Manually generate a literal pool.
  Label after_pool;
  __ B(&after_pool);
  __ Place(&l0);
  __ Place(&l1);
  __ Place(&l2);
  __ Place(&l3);
  __ Place(&l4);
  __ Place(&l5);
  __ Bind(&after_pool);

  {
    UseScratchRegisterScope temps(&masm);
    Register temp = temps.Acquire();
    VIXL_CHECK(temp.Is(r12));

    __ Ldrd(r8, r9, &l0);
    __ Ldr(r7, &l1);
    __ Ldrh(r10, &l2);
    __ Ldrsh(r11, &l3);
    __ Ldrb(temp, &l4);
    // We don't use any function call so we can use lr as an extra register.
    __ Ldrsb(lr, &l5);
  }

  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0xdeadbaba, r0);
  ASSERT_EQUAL_32(0xcafebeef, r1);
  ASSERT_EQUAL_32(0x12345678, r2);
  ASSERT_EQUAL_32(4567, r3);
  ASSERT_EQUAL_32(-4567, r4);
  ASSERT_EQUAL_32(123, r5);
  ASSERT_EQUAL_32(-123, r6);

  ASSERT_EQUAL_32(0xdeadbaba, r8);
  ASSERT_EQUAL_32(0xcafebeef, r9);
  ASSERT_EQUAL_32(0x12345678, r7);
  ASSERT_EQUAL_32(4567, r10);
  ASSERT_EQUAL_32(-4567, r11);
  ASSERT_EQUAL_32(123, r12);
  ASSERT_EQUAL_32(-123, lr);
}


TEST(custom_literal_place_shared) {
  SETUP();

  for (size_t i = 0; i < ARRAY_SIZE(kLdrLiteralRangeTestData); ++i) {
    const LdrLiteralRangeTest& test_case = kLdrLiteralRangeTestData[i];

    START();

    // Make sure the pool is empty.
    masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
    VIXL_CHECK(test.PoolIsEmpty());

    Literal<uint32_t> before(test_case.literal_value,
                             RawLiteral::kManuallyPlaced);
    Literal<uint32_t> after(test_case.literal_value,
                            RawLiteral::kManuallyPlaced);

    VIXL_CHECK(!before.IsBound());
    VIXL_CHECK(!after.IsBound());

    // Manually generate a pool.
    Label end_of_pool_before;
    __ B(&end_of_pool_before);
    __ Place(&before);
    __ Bind(&end_of_pool_before);

    VIXL_CHECK(test.PoolIsEmpty());
    VIXL_CHECK(before.IsBound());
    VIXL_CHECK(!after.IsBound());

    // Load the entries several times to test that literals can be shared.
    for (int i = 0; i < 20; i++) {
      (masm.*test_case.instruction)(r0, &before);
      (masm.*test_case.instruction)(r1, &after);
    }

    VIXL_CHECK(test.PoolIsEmpty());
    VIXL_CHECK(before.IsBound());
    VIXL_CHECK(!after.IsBound());

    // Manually generate a pool.
    Label end_of_pool_after;
    __ B(&end_of_pool_after);
    __ Place(&after);
    __ Bind(&end_of_pool_after);

    VIXL_CHECK(test.PoolIsEmpty());
    VIXL_CHECK(before.IsBound());
    VIXL_CHECK(after.IsBound());

    END();

    RUN();

    ASSERT_EQUAL_32(test_case.test_value, r0);
    ASSERT_EQUAL_32(test_case.test_value, r1);
  }
}


TEST(custom_literal_place_range) {
  SETUP();

  for (size_t i = 0; i < ARRAY_SIZE(kLdrLiteralRangeTestData); ++i) {
    const LdrLiteralRangeTest& test_case = kLdrLiteralRangeTestData[i];
    const int nop_size = masm.IsUsingA32() ? kA32InstructionSizeInBytes
                                           : k16BitT32InstructionSizeInBytes;
    const int range =
        masm.IsUsingA32() ? test_case.a32_range : test_case.t32_range;
    // On T32 the PC will be 4-byte aligned to compute the range. The
    // MacroAssembler might also need to align the code buffer before emitting
    // the literal when placing it. We keep a margin to account for this.
    const int margin = masm.IsUsingT32() ? 4 : 0;

    // Take PC offset into account and make sure the literal is in the range.
    const int padding_before =
        range - masm.GetArchitectureStatePCOffset() - sizeof(uint32_t) - margin;

    // The margin computation below is correct because the ranges are not
    // 4-byte aligned. Otherwise this test would insert the exact number of
    // instructions to cover the range and the literal would end up being
    // placed outside the range.
    VIXL_ASSERT((range % 4) != 0);

    // The range is extended by the PC offset but we need to consider the ldr
    // instruction itself and the branch over the pool.
    const int padding_after = range + masm.GetArchitectureStatePCOffset() -
                              (2 * kMaxInstructionSizeInBytes) - margin;
    START();

    Literal<uint32_t> before(test_case.literal_value,
                             RawLiteral::kManuallyPlaced);
    Literal<uint32_t> after(test_case.literal_value,
                            RawLiteral::kManuallyPlaced);

    Label test_start;
    __ B(&test_start);
    __ Place(&before);

    {
      int space = AlignDown(padding_before, nop_size);
      ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
      for (int32_t end = masm.GetCursorOffset() + space;
           masm.GetCursorOffset() < end;) {
        __ nop();
      }
    }

    __ Bind(&test_start);
    (masm.*test_case.instruction)(r0, &before);
    (masm.*test_case.instruction)(r1, &after);

    {
      int space = AlignDown(padding_after, nop_size);
      ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
      for (int32_t end = masm.GetCursorOffset() + space;
           masm.GetCursorOffset() < end;) {
        __ nop();
      }
    }

    Label after_pool;
    __ B(&after_pool);
    __ Place(&after);
    __ Bind(&after_pool);

    END();

    RUN();

    ASSERT_EQUAL_32(test_case.test_value, r0);
    ASSERT_EQUAL_32(test_case.test_value, r1);
  }
}


TEST(emit_big_pool) {
  SETUP();

  START();
  // Make sure the pool is empty.
  VIXL_CHECK(test.PoolIsEmpty());

  Label start;
  __ Bind(&start);
  for (int i = 1000; i > 0; --i) {
    __ Ldr(r0, i);
  }

  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&start) == 4000);

  CHECK_POOL_SIZE(4000);
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(1, r0);
}


TEST_T32(too_far_cbz) {
  SETUP();

  START();
  Label start;
  Label end;
  Label exit;
  __ Mov(r0, 0);
  __ B(&start);
  __ Bind(&end);
  __ Mov(r0, 1);
  __ B(&exit);
  __ Bind(&start);
  // Cbz is only defined for forward jump. Check that it will work (substituted
  // by Cbnz/B).
  __ Cbz(r0, &end);
  __ Bind(&exit);
  END();

  RUN();

  ASSERT_EQUAL_32(1, r0);
}


TEST_T32(close_cbz) {
  SETUP();

  START();
  Label first;
  Label second;
  __ Mov(r0, 0);
  __ Mov(r1, 0);
  __ Mov(r2, 0);
  __ Cbz(r0, &first);
  __ Bind(&first);
  __ Mov(r1, 1);
  __ Cbnz(r0, &second);
  __ Bind(&second);
  __ Mov(r2, 2);
  END();

  RUN();

  ASSERT_EQUAL_32(0, r0);
  ASSERT_EQUAL_32(1, r1);
  ASSERT_EQUAL_32(2, r2);
}


TEST_T32(close_cbz2) {
  SETUP();

  START();
  Label first;
  Label second;
  __ Mov(r0, 0);
  __ Mov(r1, 0);
  __ Mov(r2, 0);
  __ Cmp(r0, 0);
  __ B(ne, &first);
  __ B(gt, &second);
  __ Cbz(r0, &first);
  __ Bind(&first);
  __ Mov(r1, 1);
  __ Cbnz(r0, &second);
  __ Bind(&second);
  __ Mov(r2, 2);
  END();

  RUN();

  ASSERT_EQUAL_32(0, r0);
  ASSERT_EQUAL_32(1, r1);
  ASSERT_EQUAL_32(2, r2);
}


TEST_T32(not_close_cbz) {
  SETUP();

  START();
  Label first;
  Label second;
  __ Cbz(r0, &first);
  __ B(ne, &first);
  __ Bind(&first);
  __ Cbnz(r0, &second);
  __ B(gt, &second);
  __ Bind(&second);
  END();

  RUN();
}


TEST_T32(veneers) {
  SETUP();

  START();
  Label zero;
  Label exit;
  __ Mov(r0, 0);
  // Create one literal pool entry.
  __ Ldr(r1, 0x12345678);
  CHECK_POOL_SIZE(4);
  __ Cbz(r0, &zero);
  __ Mov(r0, 1);
  __ B(&exit);
  for (int i = 32; i > 0; i--) {
    __ Mov(r1, 0);
  }
  // Assert that the pool contains only the two veneers.
  const int kVeneerSize = 4;
  CHECK_POOL_SIZE(2 * kVeneerSize);
  __ Bind(&zero);
  __ Mov(r0, 2);
  __ Bind(&exit);
  END();

  RUN();

  ASSERT_EQUAL_32(2, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}


// This test checks that veneers are sorted. If not, the test failed as the
// veneer for "exit" is emitted before the veneer for "zero" and the "zero"
// veneer is out of range for Cbz.
TEST_T32(veneers_labels_sort) {
  SETUP();

  START();
  Label start;
  Label zero;
  Label exit;
  __ Movs(r0, 0);
  __ B(ne, &exit);
  __ B(&start);
  for (int i = 1048400; i > 0; i -= 4) {
    __ Mov(r1, 0);
  }
  __ Bind(&start);
  __ Cbz(r0, &zero);
  __ Mov(r0, 1);
  __ B(&exit);
  for (int i = 32; i > 0; i--) {
    __ Mov(r1, 0);
  }
  __ Bind(&zero);
  __ Mov(r0, 2);
  __ Bind(&exit);
  END();

  RUN();

  ASSERT_EQUAL_32(2, r0);
}

// Check that a label bound within the assembler is effectively removed from
// the veneer pool.
TEST_T32(veneer_bind) {
  SETUP();
  START();

  Label target;
  __ Cbz(r0, &target);
  __ Nop();

  {
    // Bind the target label using the `Assembler`.
    ExactAssemblyScope scope(&masm,
                             kMaxInstructionSizeInBytes,
                             ExactAssemblyScope::kMaximumSize);
    __ bind(&target);
    __ nop();
  }

  VIXL_CHECK(target.IsBound());
  VIXL_CHECK(test.PoolIsEmpty());

  END();
}


// Check that the veneer pool is correctly emitted even if we do enough narrow
// branches before a cbz so that the cbz needs its veneer emitted first in the
// pool in order to work.
TEST_T32(b_narrow_and_cbz_sort) {
  SETUP();
  START();

  const int kLabelsCount = 40;
  const int kNops = 30;
  Label b_labels[kLabelsCount];
  Label cbz_label;

  __ Nop();

  __ Mov(r0, 0);
  __ Cmp(r0, 0);

  for (int i = 0; i < kLabelsCount; ++i) {
    __ B(ne, &b_labels[i], kNear);
  }

  {
    ExactAssemblyScope scope(&masm,
                             k16BitT32InstructionSizeInBytes * kNops,
                             ExactAssemblyScope::kExactSize);
    for (int i = 0; i < kNops; i++) {
      __ nop();
    }
  }

  // The pool should not be emitted here.
  __ Cbz(r0, &cbz_label);

  // Force pool emission. If the labels are not sorted, the cbz will be out
  // of range.
  int32_t end = test.GetPoolCheckpoint();
  int32_t margin = end - masm.GetCursorOffset();

  {
    ExactAssemblyScope scope(&masm, margin, ExactAssemblyScope::kExactSize);
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
  }

  __ Mov(r0, 1);

  for (int i = 0; i < kLabelsCount; ++i) {
    __ Bind(&b_labels[i]);
  }

  __ Bind(&cbz_label);

  END();

  RUN();

  ASSERT_EQUAL_32(0, r0);
}


TEST_T32(b_narrow_and_cbz_sort_2) {
  SETUP();
  START();

  const int kLabelsCount = 40;
  const int kNops = 30;
  Label b_labels[kLabelsCount];
  Label cbz_label;

  __ Mov(r0, 0);
  __ Cmp(r0, 0);

  for (int i = 0; i < kLabelsCount; ++i) {
    __ B(ne, &b_labels[i], kNear);
  }

  {
    ExactAssemblyScope scope(&masm,
                             k16BitT32InstructionSizeInBytes * kNops,
                             ExactAssemblyScope::kExactSize);
    for (int i = 0; i < kNops; i++) {
      __ nop();
    }
  }

  // The pool should not be emitted here.
  __ Cbz(r0, &cbz_label);

  // Force pool emission. If the labels are not sorted, the cbz will be out
  // of range.
  int32_t end = test.GetPoolCheckpoint();

  while (masm.GetCursorOffset() < end) __ Nop();

  __ Mov(r0, 1);

  for (int i = 0; i < kLabelsCount; ++i) {
    __ Bind(&b_labels[i]);
  }

  __ Bind(&cbz_label);

  END();

  RUN();

  ASSERT_EQUAL_32(0, r0);
}


TEST_T32(long_branch) {
  SETUP();
  START();

  for (int label_count = 128; label_count < 2048; label_count *= 2) {
    Label* l = new Label[label_count];

    for (int i = 0; i < label_count; i++) {
      __ B(&l[i]);
    }

    for (int i = 0; i < label_count; i++) {
      __ B(ne, &l[i]);
    }

    for (int i = 0; i < 261625; i++) {
      __ Clz(r0, r0);
    }

    for (int i = label_count - 1; i >= 0; i--) {
      __ Bind(&l[i]);
      __ Nop();
    }

    delete[] l;
  }

  masm.FinalizeCode();

  END();
  RUN();
}


TEST_T32(unaligned_branch_after_literal) {
  SETUP();

  START();

  // This test manually places a 32-bit literal after a 16-bit branch
  // which branches over the literal to an unaligned PC.
  Literal<int32_t> l0(0x01234567, RawLiteral::kManuallyPlaced);

  __ Ldr(r0, &l0);
  VIXL_CHECK(test.PoolIsEmpty());

  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  // Manually generate a literal pool.
  {
    Label after_pool;
    ExactAssemblyScope scope(&masm,
                             k16BitT32InstructionSizeInBytes + sizeof(int32_t),
                             CodeBufferCheckScope::kMaximumSize);
    __ b(Narrow, &after_pool);
    __ place(&l0);
    VIXL_ASSERT((masm.GetBuffer()->GetCursorOffset() % 4) == 2);
    __ bind(&after_pool);
  }

  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  // Check that the literal was loaded correctly.
  ASSERT_EQUAL_32(0x01234567, r0);
}


// This test check that we can update a Literal after usage.
TEST(literal_update) {
  SETUP();

  START();
  Label exit;
  Literal<uint32_t>* a32 =
      new Literal<uint32_t>(0xabcdef01, RawLiteral::kDeletedOnPoolDestruction);
  Literal<uint64_t>* a64 =
      new Literal<uint64_t>(UINT64_C(0xabcdef01abcdef01),
                            RawLiteral::kDeletedOnPoolDestruction);
  __ Ldr(r0, a32);
  __ Ldrd(r2, r3, a64);
  __ EmitLiteralPool();
  Literal<uint32_t>* b32 =
      new Literal<uint32_t>(0x10fedcba, RawLiteral::kDeletedOnPoolDestruction);
  Literal<uint64_t>* b64 =
      new Literal<uint64_t>(UINT64_C(0x10fedcba10fedcba),
                            RawLiteral::kDeletedOnPoolDestruction);
  __ Ldr(r1, b32);
  __ Ldrd(r4, r5, b64);
  // Update literals' values. "a32" and "a64" are already emitted. "b32" and
  // "b64" will only be emitted when "END()" will be called.
  a32->UpdateValue(0x12345678, masm.GetBuffer());
  a64->UpdateValue(UINT64_C(0x13579bdf02468ace), masm.GetBuffer());
  b32->UpdateValue(0x87654321, masm.GetBuffer());
  b64->UpdateValue(UINT64_C(0x1032547698badcfe), masm.GetBuffer());
  END();

  RUN();

  ASSERT_EQUAL_32(0x12345678, r0);
  ASSERT_EQUAL_32(0x87654321, r1);
  ASSERT_EQUAL_32(0x02468ace, r2);
  ASSERT_EQUAL_32(0x13579bdf, r3);
  ASSERT_EQUAL_32(0x98badcfe, r4);
  ASSERT_EQUAL_32(0x10325476, r5);
}


TEST(claim_peek_poke) {
  SETUP();

  START();

  Label start;
  __ Bind(&start);
  __ Claim(0);
  __ Drop(0);
  VIXL_CHECK((masm.GetCursorOffset() - start.GetLocation()) == 0);

  __ Claim(32);
  __ Ldr(r0, 0xcafe0000);
  __ Ldr(r1, 0xcafe0001);
  __ Ldr(r2, 0xcafe0002);
  __ Poke(r0, 0);
  __ Poke(r1, 4);
  __ Poke(r2, 8);
  __ Peek(r2, 0);
  __ Peek(r0, 4);
  __ Peek(r1, 8);
  __ Drop(32);

  END();

  RUN();

  ASSERT_EQUAL_32(0xcafe0001, r0);
  ASSERT_EQUAL_32(0xcafe0002, r1);
  ASSERT_EQUAL_32(0xcafe0000, r2);
}


TEST(msr_i) {
  SETUP();

  START();
  __ Mov(r0, 0xdead);
  __ Mov(r1, 0xdead);
  __ Mov(r2, 0xdead);
  __ Mov(r3, 0xb);
  __ Msr(APSR_nzcvqg, 0);
  __ Mrs(r0, APSR);
  __ Msr(APSR_nzcvqg, 0xffffffff);
  __ Mrs(r1, APSR);
  // Only modify nzcvq => keep previous g.
  __ Lsl(r4, r3, 28);
  __ Msr(APSR_nzcvq, r4);
  __ Mrs(r2, APSR);
  END();

  RUN();

  ASSERT_EQUAL_32(0x10, r0);
  ASSERT_EQUAL_32(0xf80f0010, r1);
  ASSERT_EQUAL_32(0xb00f0010, r2);
}


TEST(vcmp_s) {
  SETUP();

  START();

  __ Vmov(s0, 1.0);
  __ Vmov(s1, 2.0);
  __ Vmov(s2, 0.0);

  __ Vcmp(F32, s0, s1);
  __ Vmrs(RegisterOrAPSR_nzcv(r0.GetCode()), FPSCR);

  __ Vcmp(F32, s0, 0.0f);
  __ Vmrs(RegisterOrAPSR_nzcv(r1.GetCode()), FPSCR);

  __ Vcmp(F32, s2, 0.0f);
  __ Vmrs(RegisterOrAPSR_nzcv(r2.GetCode()), FPSCR);

  END();

  RUN();

  // N is for less than.
  ASSERT_EQUAL_32(NFlag, r0);
  // C is for greater than.
  ASSERT_EQUAL_32(CFlag, r1);
  // ZC is for equal.
  ASSERT_EQUAL_32(ZCFlag, r2);
}


TEST(vcmp_d) {
  SETUP();

  START();

  __ Vmov(d0, 1.0);
  __ Vmov(d1, 2.0);
  __ Vmov(d2, 0.0);

  __ Vcmp(F64, d0, d1);
  __ Vmrs(RegisterOrAPSR_nzcv(r0.GetCode()), FPSCR);

  __ Vcmp(F64, d0, 0.0);
  __ Vmrs(RegisterOrAPSR_nzcv(r1.GetCode()), FPSCR);

  __ Vcmp(F64, d2, 0.0);
  __ Vmrs(RegisterOrAPSR_nzcv(r2.GetCode()), FPSCR);

  END();

  RUN();

  // N is for less than.
  ASSERT_EQUAL_32(NFlag, r0);
  // C is for greater than.
  ASSERT_EQUAL_32(CFlag, r1);
  // ZC is for equal.
  ASSERT_EQUAL_32(ZCFlag, r2);
}


TEST(vcmpe_s) {
  SETUP();

  START();

  __ Vmov(s0, 1.0);
  __ Vmov(s1, 2.0);
  __ Vmov(s2, 0.0);

  __ Vcmpe(F32, s0, s1);
  __ Vmrs(RegisterOrAPSR_nzcv(r0.GetCode()), FPSCR);

  __ Vcmpe(F32, s0, 0.0f);
  __ Vmrs(RegisterOrAPSR_nzcv(r1.GetCode()), FPSCR);

  __ Vcmpe(F32, s2, 0.0f);
  __ Vmrs(RegisterOrAPSR_nzcv(r2.GetCode()), FPSCR);

  END();

  RUN();

  // N is for less than.
  ASSERT_EQUAL_32(NFlag, r0);
  // C is for greater than.
  ASSERT_EQUAL_32(CFlag, r1);
  // ZC is for equal.
  ASSERT_EQUAL_32(ZCFlag, r2);
}


TEST(vcmpe_d) {
  SETUP();

  START();

  __ Vmov(d0, 1.0);
  __ Vmov(d1, 2.0);
  __ Vmov(d2, 0.0);

  __ Vcmpe(F64, d0, d1);
  __ Vmrs(RegisterOrAPSR_nzcv(r0.GetCode()), FPSCR);

  __ Vcmpe(F64, d0, 0.0);
  __ Vmrs(RegisterOrAPSR_nzcv(r1.GetCode()), FPSCR);

  __ Vcmpe(F64, d2, 0.0);
  __ Vmrs(RegisterOrAPSR_nzcv(r2.GetCode()), FPSCR);

  END();

  RUN();

  // N is for less than.
  ASSERT_EQUAL_32(NFlag, r0);
  // C is for greater than.
  ASSERT_EQUAL_32(CFlag, r1);
  // ZC is for equal.
  ASSERT_EQUAL_32(ZCFlag, r2);
}


TEST(vmrs_vmsr) {
  SETUP();

  START();
  // Move some value to FPSCR and get them back to test vmsr/vmrs instructions.
  __ Mov(r0, 0x2a000000);
  __ Vmsr(FPSCR, r0);
  __ Vmrs(RegisterOrAPSR_nzcv(r1.GetCode()), FPSCR);

  __ Mov(r0, 0x5a000000);
  __ Vmsr(FPSCR, r0);
  __ Vmrs(RegisterOrAPSR_nzcv(r2.GetCode()), FPSCR);

  // Move to APSR_nzcv.
  __ Vmrs(RegisterOrAPSR_nzcv(pc.GetCode()), FPSCR);
  __ Mrs(r3, APSR);
  __ And(r3, r3, 0xf0000000);

  END();

  RUN();

  ASSERT_EQUAL_32(0x2a000000, r1);
  ASSERT_EQUAL_32(0x5a000000, r2);
  ASSERT_EQUAL_32(0x50000000, r3);
}


TEST(printf) {
  SETUP();

  START();
  __ Mov(r0, 0xb00e0000);
  __ Msr(APSR_nzcvqg, r0);
  __ Mov(r0, sp);
  __ Printf("sp=%x\n", r0);
  //  __ Printf("Hello world!\n");
  __ Mov(r0, 0x1234);
  __ Mov(r1, 0x5678);
  StringLiteral literal("extra string");
  __ Adr(r2, &literal);
  __ Mov(r3, 5);
  __ Mov(r4, 0xdead4444);
  __ Mov(r5, 0xdead5555);
  __ Mov(r6, 0xdead6666);
  __ Mov(r7, 0xdead7777);
  __ Mov(r8, 0xdead8888);
  __ Mov(r9, 0xdead9999);
  __ Mov(r10, 0xdeadaaaa);
  __ Mov(r11, 0xdeadbbbb);
  __ Vldr(d0, 1.2345);
  __ Vldr(d1, 2.9876);
  __ Vldr(s4, 1.3333);
  __ Vldr(s5, 3.21);
  __ Vldr(d3, 3.333);
  __ Vldr(d4, 4.444);
  __ Vldr(d5, 5.555);
  __ Vldr(d6, 6.666);
  __ Vldr(d7, 7.777);
  __ Vldr(d8, 8.888);
  __ Vldr(d9, 9.999);
  __ Vldr(d10, 10.000);
  __ Vldr(d11, 11.111);
  __ Vldr(d12, 12.222);
  __ Vldr(d13, 13.333);
  __ Vldr(d14, 14.444);
  __ Vldr(d15, 15.555);
  __ Vldr(d16, 16.666);
  __ Vldr(d17, 17.777);
  __ Vldr(d18, 18.888);
  __ Vldr(d19, 19.999);
  __ Vldr(d20, 20.000);
  __ Vldr(d21, 21.111);
  __ Vldr(d22, 22.222);
  __ Vldr(d23, 23.333);
  __ Vldr(d24, 24.444);
  __ Vldr(d25, 25.555);
  __ Vldr(d26, 26.666);
  __ Vldr(d27, 27.777);
  __ Vldr(d28, 28.888);
  __ Vldr(d29, 29.999);
  __ Vldr(d30, 30.000);
  __ Vldr(d31, 31.111);
  {
    UseScratchRegisterScope temps(&masm);
    // For effective use as an inspection tool, Printf must work without any
    // scratch registers.
    VIXL_CHECK(r12.Is(temps.Acquire()));
    __ Mov(r12, 0xdeadcccc);
    VIXL_CHECK(masm.GetScratchRegisterList()->IsEmpty());

    __ Printf("%% r0=%x r1=%x str=<%.*s>\n", r0, r1, r3, r2);
    __ Printf("r0=%d r1=%d str=<%s>\n", r0, r1, r2);
    __ Printf("d0=%g\n", d0);
    __ Printf("s4=%g\n", s4);
    __ Printf("d0=%g d1=%g s4=%g s5=%g\n", d0, d1, s4, s5);
    __ Printf("d0=%g r0=%x s4=%g r1=%x\n", d0, r0, s4, r1);
    __ Printf("r0=%x d0=%g r1=%x s4=%g\n", r0, d0, r1, s4);
    __ Mov(r0, sp);
    __ Printf("sp=%x\n", r0);
    __ Mrs(r0, APSR);
    // Only keep R/W fields.
    __ Mov(r2, 0xf80f0200);
    __ And(r0, r0, r2);
  }
  END();

  RUN();

  ASSERT_EQUAL_32(0xb00e0000, r0);
  ASSERT_EQUAL_32(0x5678, r1);
  ASSERT_EQUAL_32(5, r3);
  ASSERT_EQUAL_32(0xdead4444, r4);
  ASSERT_EQUAL_32(0xdead5555, r5);
  ASSERT_EQUAL_32(0xdead6666, r6);
  ASSERT_EQUAL_32(0xdead7777, r7);
  ASSERT_EQUAL_32(0xdead8888, r8);
  ASSERT_EQUAL_32(0xdead9999, r9);
  ASSERT_EQUAL_32(0xdeadaaaa, r10);
  ASSERT_EQUAL_32(0xdeadbbbb, r11);
  ASSERT_EQUAL_32(0xdeadcccc, r12);
  ASSERT_EQUAL_FP64(1.2345, d0);
  ASSERT_EQUAL_FP64(2.9876, d1);
  ASSERT_EQUAL_FP32(1.3333, s4);
  ASSERT_EQUAL_FP32(3.21, s5);
  ASSERT_EQUAL_FP64(4.444, d4);
  ASSERT_EQUAL_FP64(5.555, d5);
  ASSERT_EQUAL_FP64(6.666, d6);
  ASSERT_EQUAL_FP64(7.777, d7);
  ASSERT_EQUAL_FP64(8.888, d8);
  ASSERT_EQUAL_FP64(9.999, d9);
  ASSERT_EQUAL_FP64(10.000, d10);
  ASSERT_EQUAL_FP64(11.111, d11);
  ASSERT_EQUAL_FP64(12.222, d12);
  ASSERT_EQUAL_FP64(13.333, d13);
  ASSERT_EQUAL_FP64(14.444, d14);
  ASSERT_EQUAL_FP64(15.555, d15);
  ASSERT_EQUAL_FP64(16.666, d16);
  ASSERT_EQUAL_FP64(17.777, d17);
  ASSERT_EQUAL_FP64(18.888, d18);
  ASSERT_EQUAL_FP64(19.999, d19);
  ASSERT_EQUAL_FP64(20.000, d20);
  ASSERT_EQUAL_FP64(21.111, d21);
  ASSERT_EQUAL_FP64(22.222, d22);
  ASSERT_EQUAL_FP64(23.333, d23);
  ASSERT_EQUAL_FP64(24.444, d24);
  ASSERT_EQUAL_FP64(25.555, d25);
  ASSERT_EQUAL_FP64(26.666, d26);
  ASSERT_EQUAL_FP64(27.777, d27);
  ASSERT_EQUAL_FP64(28.888, d28);
  ASSERT_EQUAL_FP64(29.999, d29);
  ASSERT_EQUAL_FP64(30.000, d30);
  ASSERT_EQUAL_FP64(31.111, d31);
}

TEST(printf2) {
  SETUP();

  START();
  __ Mov(r0, 0x1234);
  __ Mov(r1, 0x5678);
  __ Vldr(d0, 1.2345);
  __ Vldr(s2, 2.9876);
  __ Printf("d0=%g d1=%g r0=%x r1=%x\n", d0, s2, r0, r1);
  END();

  RUN();
}


template <typename T>
void CheckInstructionSetA32(const T& assm) {
  VIXL_CHECK(assm.IsUsingA32());
  VIXL_CHECK(!assm.IsUsingT32());
  VIXL_CHECK(assm.GetInstructionSetInUse() == A32);
}


template <typename T>
void CheckInstructionSetT32(const T& assm) {
  VIXL_CHECK(assm.IsUsingT32());
  VIXL_CHECK(!assm.IsUsingA32());
  VIXL_CHECK(assm.GetInstructionSetInUse() == T32);
}


TEST_NOASM(set_isa_constructors) {
  byte buffer[1024];

#ifndef VIXL_INCLUDE_TARGET_T32_ONLY
  // A32 by default.
  CheckInstructionSetA32(Assembler());
  CheckInstructionSetA32(Assembler(1024));
  CheckInstructionSetA32(Assembler(buffer, sizeof(buffer)));

  CheckInstructionSetA32(MacroAssembler());
  CheckInstructionSetA32(MacroAssembler(1024));
  CheckInstructionSetA32(MacroAssembler(buffer, sizeof(buffer)));
#else
  // T32 by default.
  CheckInstructionSetT32(Assembler());
  CheckInstructionSetT32(Assembler(1024));
  CheckInstructionSetT32(Assembler(buffer, sizeof(buffer)));

  CheckInstructionSetT32(MacroAssembler());
  CheckInstructionSetT32(MacroAssembler(1024));
  CheckInstructionSetT32(MacroAssembler(buffer, sizeof(buffer)));
#endif

#ifdef VIXL_INCLUDE_TARGET_A32
  // Explicit A32.
  CheckInstructionSetA32(Assembler(A32));
  CheckInstructionSetA32(Assembler(1024, A32));
  CheckInstructionSetA32(Assembler(buffer, sizeof(buffer), A32));

  CheckInstructionSetA32(MacroAssembler(A32));
  CheckInstructionSetA32(MacroAssembler(1024, A32));
  CheckInstructionSetA32(MacroAssembler(buffer, sizeof(buffer), A32));
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
  // Explicit T32.
  CheckInstructionSetT32(Assembler(T32));
  CheckInstructionSetT32(Assembler(1024, T32));
  CheckInstructionSetT32(Assembler(buffer, sizeof(buffer), T32));

  CheckInstructionSetT32(MacroAssembler(T32));
  CheckInstructionSetT32(MacroAssembler(1024, T32));
  CheckInstructionSetT32(MacroAssembler(buffer, sizeof(buffer), T32));
#endif
}


TEST_NOASM(set_isa_empty) {
// It is possible to change the instruction set if no instructions have yet
// been generated. This test only makes sense when both A32 and T32 are
// supported.
#ifdef VIXL_INCLUDE_TARGET_AARCH32
  Assembler assm;
  CheckInstructionSetA32(assm);
  assm.UseT32();
  CheckInstructionSetT32(assm);
  assm.UseA32();
  CheckInstructionSetA32(assm);
  assm.UseInstructionSet(T32);
  CheckInstructionSetT32(assm);
  assm.UseInstructionSet(A32);
  CheckInstructionSetA32(assm);

  MacroAssembler masm;
  CheckInstructionSetA32(masm);
  masm.UseT32();
  CheckInstructionSetT32(masm);
  masm.UseA32();
  CheckInstructionSetA32(masm);
  masm.UseInstructionSet(T32);
  CheckInstructionSetT32(masm);
  masm.UseInstructionSet(A32);
  CheckInstructionSetA32(masm);
#endif
}


// clang-format off
TEST_NOASM(set_isa_noop) {
// It is possible to call a no-op UseA32/T32 or UseInstructionSet even if
// one or more instructions have been generated.
#ifdef VIXL_INCLUDE_TARGET_A32
  {
    Assembler assm(A32);
    CheckInstructionSetA32(assm);
    CodeBufferCheckScope scope(&assm, kMaxInstructionSizeInBytes);
    assm.bx(lr);
    VIXL_ASSERT(assm.GetCursorOffset() > 0);
    CheckInstructionSetA32(assm);
    assm.UseA32();
    CheckInstructionSetA32(assm);
    assm.UseInstructionSet(A32);
    CheckInstructionSetA32(assm);
    assm.FinalizeCode();
  }
  {
    MacroAssembler masm(A32);
    CheckInstructionSetA32(masm);
    masm.Bx(lr);
    VIXL_ASSERT(masm.GetCursorOffset() > 0);
    CheckInstructionSetA32(masm);
    masm.UseA32();
    CheckInstructionSetA32(masm);
    masm.UseInstructionSet(A32);
    CheckInstructionSetA32(masm);
    masm.FinalizeCode();
  }
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
  {
    Assembler assm(T32);
    CheckInstructionSetT32(assm);
    CodeBufferCheckScope scope(&assm, kMaxInstructionSizeInBytes);
    assm.bx(lr);
    VIXL_ASSERT(assm.GetCursorOffset() > 0);
    CheckInstructionSetT32(assm);
    assm.UseT32();
    CheckInstructionSetT32(assm);
    assm.UseInstructionSet(T32);
    CheckInstructionSetT32(assm);
    assm.FinalizeCode();
  }
  {
    MacroAssembler masm(T32);
    CheckInstructionSetT32(masm);
    masm.Bx(lr);
    VIXL_ASSERT(masm.GetCursorOffset() > 0);
    CheckInstructionSetT32(masm);
    masm.UseT32();
    CheckInstructionSetT32(masm);
    masm.UseInstructionSet(T32);
    CheckInstructionSetT32(masm);
    masm.FinalizeCode();
  }
#endif
}
// clang-format on


TEST(logical_arithmetic_identities) {
  SETUP();

  START();

  Label blob_1;
  __ Bind(&blob_1);
  __ Add(r0, r0, 0);
  __ And(r0, r0, 0xffffffff);
  __ Bic(r0, r0, 0);
  __ Eor(r0, r0, 0);
  __ Orn(r0, r0, 0xffffffff);
  __ Orr(r0, r0, 0);
  __ Sub(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_1) == 0);

  Label blob_2;
  __ Bind(&blob_2);
  __ Adds(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_2) != 0);

  Label blob_3;
  __ Bind(&blob_3);
  __ Ands(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_3) != 0);

  Label blob_4;
  __ Bind(&blob_4);
  __ Bics(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_4) != 0);

  Label blob_5;
  __ Bind(&blob_5);
  __ Eors(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_5) != 0);

  Label blob_6;
  __ Bind(&blob_6);
  __ Orns(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_6) != 0);

  Label blob_7;
  __ Bind(&blob_7);
  __ Orrs(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_7) != 0);

  Label blob_8;
  __ Bind(&blob_8);
  __ Subs(r0, r0, 0);
  VIXL_ASSERT(masm.GetSizeOfCodeGeneratedSince(&blob_8) != 0);

  __ Mov(r0, 0xbad);
  __ And(r1, r0, 0);
  __ Bic(r2, r0, 0xffffffff);
  __ Eor(r3, r0, 0xffffffff);
  __ Orn(r4, r0, 0);
  __ Orr(r5, r0, 0xffffffff);

  END();

  RUN();

  ASSERT_EQUAL_32(0xbad, r0);
  ASSERT_EQUAL_32(0, r1);
  ASSERT_EQUAL_32(0, r2);
  ASSERT_EQUAL_32(~0xbad, r3);
  ASSERT_EQUAL_32(0xffffffff, r4);
  ASSERT_EQUAL_32(0xffffffff, r5);
}


TEST(scratch_register_checks) {
  // It is unsafe for users to use registers that the MacroAssembler is also
  // using as scratch registers. This test checks the MacroAssembler's checking
  // mechanism itself.
  SETUP();
  START();
  {
    UseScratchRegisterScope temps(&masm);
    // 'ip' is a scratch register by default.
    VIXL_CHECK(masm.GetScratchRegisterList()->GetList() ==
               (1u << ip.GetCode()));
    VIXL_CHECK(temps.IsAvailable(ip));

    // Integer registers have no complicated aliasing so
    // masm.AliasesAvailableScratchRegister(reg) == temps.IsAvailable(reg).
    for (unsigned i = 0; i < kNumberOfRegisters; i++) {
      Register reg(i);
      VIXL_CHECK(masm.AliasesAvailableScratchRegister(reg) ==
                 temps.IsAvailable(reg));
    }
  }
  END();
}


TEST(scratch_register_checks_v) {
  // It is unsafe for users to use registers that the MacroAssembler is also
  // using as scratch registers. This test checks the MacroAssembler's checking
  // mechanism itself.
  SETUP();
  {
    UseScratchRegisterScope temps(&masm);
    // There is no default floating-point scratch register. Add temps of various
    // sizes to check handling of aliased registers.
    VIXL_CHECK(masm.GetScratchVRegisterList()->GetList() == 0);
    temps.Include(q15);
    temps.Include(d15);
    temps.Include(s15);
    temps.Include(d4);
    temps.Include(d5);
    temps.Include(s24);
    temps.Include(s25);
    temps.Include(s26);
    temps.Include(s27);
    temps.Include(q0);
    // See VRegisterList for details of the list encoding.
    VIXL_CHECK(masm.GetScratchVRegisterList()->GetList() ==
               UINT64_C(0xf0000000cf008f0f));
    //                    |       ||  || |
    //                   q15    d15|  || q0
    //                        s24-s27 |d4-d5
    //                               s15

    // Simple checks: Included registers are available.
    VIXL_CHECK(temps.IsAvailable(q15));
    VIXL_CHECK(temps.IsAvailable(d15));
    VIXL_CHECK(temps.IsAvailable(s15));
    VIXL_CHECK(temps.IsAvailable(d4));
    VIXL_CHECK(temps.IsAvailable(d5));
    VIXL_CHECK(temps.IsAvailable(s24));
    VIXL_CHECK(temps.IsAvailable(s25));
    VIXL_CHECK(temps.IsAvailable(s26));
    VIXL_CHECK(temps.IsAvailable(s27));
    VIXL_CHECK(temps.IsAvailable(q0));

    // Each available S register should mark the corresponding D and Q registers
    // as aliasing an available scratch register.
    for (unsigned s = 0; s < kNumberOfSRegisters; s++) {
      if (temps.IsAvailable(SRegister(s))) {
        VIXL_CHECK(masm.AliasesAvailableScratchRegister(SRegister(s)));
        VIXL_CHECK(masm.AliasesAvailableScratchRegister(DRegister(s / 2)));
        VIXL_CHECK(masm.AliasesAvailableScratchRegister(QRegister(s / 4)));
      } else {
        // AliasesAvailableScratchRegiters == IsAvailable for S registers.
        VIXL_CHECK(!masm.AliasesAvailableScratchRegister(SRegister(s)));
      }
    }

    // Similar checks for high D registers.
    unsigned first_high_d_register = kNumberOfSRegisters / 2;
    for (unsigned d = first_high_d_register; d < kMaxNumberOfDRegisters; d++) {
      if (temps.IsAvailable(DRegister(d))) {
        VIXL_CHECK(masm.AliasesAvailableScratchRegister(DRegister(d)));
        VIXL_CHECK(masm.AliasesAvailableScratchRegister(QRegister(d / 2)));
      } else {
        // AliasesAvailableScratchRegiters == IsAvailable for high D registers.
        VIXL_CHECK(!masm.AliasesAvailableScratchRegister(DRegister(d)));
      }
    }
  }
}


TEST(nop) {
  SETUP();

  Label start;
  __ Bind(&start);
  __ Nop();
  size_t nop_size = (isa == T32) ? k16BitT32InstructionSizeInBytes
                                 : kA32InstructionSizeInBytes;
  // `MacroAssembler::Nop` must generate at least one nop.
  VIXL_CHECK(masm.GetSizeOfCodeGeneratedSince(&start) >= nop_size);

  masm.FinalizeCode();
}

// Check that `GetPoolCheckpoint()` is precise.
TEST(literal_pool_margin) {
  SETUP();

  START();

  VIXL_CHECK(test.PoolIsEmpty());

  // Create a single literal.
  __ Ldrd(r0, r1, 0x1234567890abcdef);

  VIXL_CHECK(!test.PoolIsEmpty());

  // Generate code to fill all the margin we have before generating the literal
  // pool.
  int32_t margin = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  int32_t end = test.GetPoolCheckpoint();
  {
    ExactAssemblyScope scope(&masm, margin, ExactAssemblyScope::kExactSize);
    // Opening the scope should not have triggered the emission of the literal
    // pool.
    VIXL_CHECK(!test.PoolIsEmpty());
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
    VIXL_CHECK(masm.GetCursorOffset() == end);
  }

  // There should be no margin left to emit the literal pool.
  VIXL_CHECK(!test.PoolIsEmpty());
  VIXL_CHECK(test.GetPoolCheckpoint() == masm.GetCursorOffset());

  // So emitting a single instruction should force emission of the pool.
  __ Nop();
  VIXL_CHECK(test.PoolIsEmpty());
  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}


// Check that `GetPoolCheckpoint()` is precise.
TEST(veneer_pool_margin) {
  SETUP();

  START();

  VIXL_CHECK(test.PoolIsEmpty());

  // Create a single veneer.
  Label target;
  __ B(eq, &target);

  VIXL_CHECK(!test.PoolIsEmpty());

  // Generate code to fill all the margin we have before generating the veneer
  // pool.
  int32_t margin = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  int32_t end = test.GetPoolCheckpoint();
  {
    ExactAssemblyScope scope(&masm, margin, ExactAssemblyScope::kExactSize);
    // Opening the scope should not have triggered the emission of the veneer
    // pool.
    VIXL_CHECK(!test.PoolIsEmpty());
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
    VIXL_CHECK(masm.GetCursorOffset() == end);
  }
  // There should be no margin left to emit the veneer pool.
  VIXL_CHECK(test.GetPoolCheckpoint() == masm.GetCursorOffset());

  // So emitting a single instruction should force emission of the pool.
  // We cannot simply check that the veneer pool is empty, because the veneer
  // emitted for the CBZ instruction above is itself tracked by the veneer
  // mechanisms. Instead, check that some 'unexpected' code is generated.
  Label check;
  __ Bind(&check);
  {
    ExactAssemblyScope scope(&masm, 2, ExactAssemblyScope::kMaximumSize);
    // Do not actually generate any code.
  }
  VIXL_CHECK(masm.GetSizeOfCodeGeneratedSince(&check) > 0);
  __ Bind(&target);
  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();
}

TEST_T32(near_branch_fuzz) {
  SETUP();
  START();

  uint16_t seed[3] = {1, 2, 3};
  seed48(seed);

  const int label_count = 31;
  Label* l;

  // Use multiple iterations, as each produces a different predictably random
  // sequence.
  const int iterations = 64;

  int loop_count = 0;
  __ Mov(r1, 0);

  // Initialise the status flags to Z set.
  __ Cmp(r1, r1);

  // Gradually increasing the number of cases effectively increases the
  // probability of nops being emitted in the sequence. The branch-to-bind
  // ratio in the sequence is fixed at 4:1 by the ratio of cases.
  for (int case_count = 6; case_count < 37; case_count++) {
    for (int iter = 0; iter < iterations; iter++) {
      l = new Label[label_count];

      // Set r0 != 0 to force no branches to be taken. Also acts as a marker
      // between each iteration in the disassembly.
      __ Mov(r0, 1);

      for (;;) {
        uint32_t inst_case = static_cast<uint32_t>(mrand48()) % case_count;
        uint32_t label_index = static_cast<uint32_t>(mrand48()) % label_count;

        switch (inst_case) {
          case 0:  // Bind.
            if (!l[label_index].IsBound()) {
              __ Bind(&l[label_index]);

              // We should hit each label exactly once (because the branches are
              // never taken). Keep a counter to verify this.
              loop_count++;
              __ Add(r1, r1, 1);
            }
            break;
          case 1:  // Compare and branch if zero (untaken as r0 == 1).
            __ Cbz(r0, &l[label_index]);
            break;
          case 2: {  // Compare and branch if not zero.
            Label past_branch;
            __ B(eq, &past_branch, kNear);
            __ Cbnz(r0, &l[label_index]);
            __ Bind(&past_branch);
            break;
          }
          case 3: {  // Unconditional branch preferred near.
            Label past_branch;
            __ B(eq, &past_branch, kNear);
            __ B(&l[label_index], kNear);
            __ Bind(&past_branch);
            break;
          }
          case 4:  // Conditional branch (untaken as Z set) preferred near.
            __ B(ne, &l[label_index], kNear);
            break;
          default:  // Nop.
            __ Nop();
            break;
        }

        // If all labels have been bound, exit the inner loop and finalise the
        // code.
        bool all_bound = true;
        for (int i = 0; i < label_count; i++) {
          all_bound = all_bound && l[i].IsBound();
        }
        if (all_bound) break;
      }

      // Ensure that the veneer pools are emitted, to keep each branch/bind test
      // independent. We will generate more code following this.
      masm.FinalizeCode(MacroAssembler::kFallThrough);
      delete[] l;
    }
  }

  END();
  RUN();

  ASSERT_EQUAL_32(loop_count, r1);
}


static void NearBranchAndLiteralFuzzHelper(InstructionSet isa,
                                           int shard_count,
                                           int shard_offset) {
  SETUP();
  START();

  uint16_t seed[3] = {1, 2, 3};
  seed48(seed);

  const int label_count = 15;
  const int literal_count = 31;
  Label* labels;
  uint64_t* literal_values;
  Literal<uint64_t>* literals[literal_count];

  // Use multiple iterations, as each produces a different predictably random
  // sequence.
  const int iterations = 128;
  const int n_cases = 20;
  VIXL_CHECK((iterations % shard_count) == 0);

  int loop_count = 0;
  __ Mov(r1, 0);

  // If the value of r4 changes then the test fails.
  __ Mov(r4, 42);

  // This test generates a mix of 20 different code sequences (see switch case
  // below). The cases are split in 4 groups:
  //
  //   - 0..3: Generate various amount of nops.
  //   - 4..7: Generate various load intstructions with literals.
  //   - 8..14: Generate various branch instructions.
  //   - 15..19: Generate various amount of nops.
  //
  // The idea behind this is that we can have a window of size N which we can
  // slide across these cases. And as a result, randomly generate sequences with
  // a different ratio of:
  //   - "nops vs literals"
  //   - "literal vs veneers"
  //   - "veneers vs nops"
  //
  // In this test, we grow a window from 5 to 14, and then slide this window
  // across all cases each time. We call this sliding a "ratio", which is in
  // fact an offset from the first case of the switch.

  for (uint32_t window = 5; window < 14; window++) {
    for (uint32_t ratio = 0; ratio < static_cast<uint32_t>(n_cases - window);
         ratio++) {
      for (int iter = shard_offset; iter < iterations; iter += shard_count) {
        Label fail;
        Label end;

        // Reset local state.
        labels = new Label[label_count];

        // Create new literal values.
        literal_values = new uint64_t[literal_count];
        for (int lit = 0; lit < literal_count; lit++) {
          // TODO: Generate pseudo-random data for literals. At the moment, the
          // disassembler breaks if we do this.
          literal_values[lit] = lit;
          literals[lit] = new Literal<uint64_t>(literal_values[lit]);
        }

        for (;;) {
          uint32_t inst_case =
              (static_cast<uint32_t>(mrand48()) % window) + ratio;
          uint32_t label_index = static_cast<uint32_t>(mrand48()) % label_count;
          uint32_t literal_index =
              static_cast<uint32_t>(mrand48()) % literal_count;

          if (inst_case == ratio) {
            if (!labels[label_index].IsBound()) {
              __ Bind(&labels[label_index]);

              // We should hit each label exactly once (because the branches are
              // never taken). Keep a counter to verify this.
              loop_count++;
              __ Add(r1, r1, 1);
              continue;
            }
          }

          switch (inst_case) {
            case 0:
              __ Nop();
              break;
            case 1:
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 2:
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 3:
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 4:
              __ Ldr(r2, literals[literal_index]);
              __ Cmp(r2, static_cast<uint32_t>(literal_values[literal_index]));
              __ B(ne, &fail);
              __ Mov(r2, 0);
              break;
            case 5:
              __ Ldrb(r2, literals[literal_index]);
              __ Cmp(r2,
                     static_cast<uint32_t>(literal_values[literal_index]) &
                         0xff);
              __ B(ne, &fail);
              __ Mov(r2, 0);
              break;
            case 6:
              __ Ldrd(r2, r3, literals[literal_index]);
              __ Cmp(r2, static_cast<uint32_t>(literal_values[literal_index]));
              __ B(ne, &fail);
              __ Mov(r2, 0);
              __ Cmp(r3,
                     static_cast<uint32_t>(literal_values[literal_index] >>
                                           32));
              __ B(ne, &fail);
              __ Mov(r3, 0);
              break;
            case 7:
              __ Vldr(s0, literals[literal_index]);
              __ Vmov(s1, static_cast<uint32_t>(literal_values[literal_index]));
              __ Vcmp(s0, s1);
              __ B(ne, &fail);
              __ Vmov(s0, 0);
              break;
            case 8: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ Cbz(r0, &labels[label_index]);
              __ Bind(&past_branch);
              break;
            }
            case 9: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ Cbnz(r0, &labels[label_index]);
              __ Bind(&past_branch);
              break;
            }
            case 10: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ B(ne, &labels[label_index], kNear);
              __ Bind(&past_branch);
              break;
            }
            case 11: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ B(&labels[label_index], kNear);
              __ Bind(&past_branch);
              break;
            }
            case 12: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ B(ne, &labels[label_index]);
              __ Bind(&past_branch);
              break;
            }
            case 13: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ B(&labels[label_index]);
              __ Bind(&past_branch);
              break;
            }
            case 14: {
              Label past_branch;
              __ B(&past_branch, kNear);
              __ Bl(&labels[label_index]);
              __ Bind(&past_branch);
              break;
            }
            case 15:
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 16:
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 17:
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 18:
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              __ Nop();
              break;
            case 19:
              __ Nop();
              __ Nop();
              break;
            default:
              VIXL_UNREACHABLE();
              break;
          }

          // If all labels have been bound, exit the inner loop and finalise the
          // code.
          bool all_bound = true;
          for (int i = 0; i < label_count; i++) {
            all_bound = all_bound && labels[i].IsBound();
          }
          if (all_bound) break;
        }

        __ B(&end);
        __ Bind(&fail);
        __ Mov(r4, 0);
        __ Bind(&end);

        // Ensure that the veneer pools are emitted, to keep each branch/bind
        // test
        // independent.
        masm.FinalizeCode(MacroAssembler::kFallThrough);
        delete[] labels;
        for (int lit = 0; lit < literal_count; lit++) {
          delete literals[lit];
        }
      }
    }
  }

  END();
  RUN();

  ASSERT_EQUAL_32(loop_count, r1);
  ASSERT_EQUAL_32(42, r4);
}

TEST_T32(near_branch_and_literal_fuzz_0) {
  NearBranchAndLiteralFuzzHelper(isa, 4, 0);
}

TEST_T32(near_branch_and_literal_fuzz_1) {
  NearBranchAndLiteralFuzzHelper(isa, 4, 1);
}

TEST_T32(near_branch_and_literal_fuzz_2) {
  NearBranchAndLiteralFuzzHelper(isa, 4, 2);
}

TEST_T32(near_branch_and_literal_fuzz_3) {
  NearBranchAndLiteralFuzzHelper(isa, 4, 3);
}

#ifdef VIXL_INCLUDE_TARGET_T32
TEST_NOASM(code_buffer_precise_growth) {
  static const int kBaseBufferSize = 16;
  MacroAssembler masm(kBaseBufferSize, T32);

  VIXL_CHECK(masm.GetBuffer()->GetCapacity() == kBaseBufferSize);

  {
    // Fill the buffer with nops.
    ExactAssemblyScope scope(&masm,
                             kBaseBufferSize,
                             ExactAssemblyScope::kExactSize);
    for (int i = 0; i < kBaseBufferSize; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

  // The buffer should not have grown yet.
  VIXL_CHECK(masm.GetBuffer()->GetCapacity() == kBaseBufferSize);

  // Generating a single instruction should force the buffer to grow.
  __ Nop();

  VIXL_CHECK(masm.GetBuffer()->GetCapacity() > kBaseBufferSize);

  masm.FinalizeCode();
}
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
TEST_NOASM(out_of_space_immediately_before_EnsureEmitFor) {
  static const int kBaseBufferSize = 64;
  MacroAssembler masm(kBaseBufferSize, T32);
  TestMacroAssembler test(&masm);

  VIXL_CHECK(masm.GetBuffer()->GetCapacity() == kBaseBufferSize);

  VIXL_CHECK(test.PoolIsEmpty());

  // Create a veneer.
  Label target;
  __ Cbz(r0, &target);

  VIXL_CHECK(!test.PoolIsEmpty());

  VIXL_CHECK(IsUint32(masm.GetBuffer()->GetRemainingBytes()));
  uint32_t space = static_cast<uint32_t>(masm.GetBuffer()->GetRemainingBytes());
  {
    // Fill the buffer with nops.
    ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
    for (uint32_t i = 0; i < space; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

  VIXL_CHECK(!test.PoolIsEmpty());

  // The buffer should not have grown yet, and there should be no space left.
  VIXL_CHECK(masm.GetBuffer()->GetCapacity() == kBaseBufferSize);
  VIXL_CHECK(masm.GetBuffer()->GetRemainingBytes() == 0);

  // Force emission of the veneer, at a point where there is no space available
  // in the buffer.
  int32_t past_cbz_range =
      test.GetPoolCheckpoint() - masm.GetCursorOffset() + 1;
  masm.EnsureEmitFor(past_cbz_range);

  __ Bind(&target);

  VIXL_CHECK(test.PoolIsEmpty());

  masm.FinalizeCode();
}
#endif


TEST_NOASM(EnsureEmitFor) {
  static const int kBaseBufferSize = 32;
  MacroAssembler masm(kBaseBufferSize);

  VIXL_CHECK(masm.GetBuffer()->GetCapacity() == kBaseBufferSize);

  VIXL_CHECK(IsUint32(masm.GetBuffer()->GetRemainingBytes()));
  int32_t space = static_cast<int32_t>(masm.GetBuffer()->GetRemainingBytes());
  int32_t end = __ GetCursorOffset() + space;
  {
    // Fill the buffer with nops.
    ExactAssemblyScope scope(&masm, space, ExactAssemblyScope::kExactSize);
    while (__ GetCursorOffset() != end) {
      __ nop();
    }
  }

  // Test that EnsureEmitFor works.
  VIXL_CHECK(!masm.GetBuffer()->HasSpaceFor(4));
  masm.EnsureEmitFor(4);
  VIXL_CHECK(masm.GetBuffer()->HasSpaceFor(4));
  __ Nop();

  masm.FinalizeCode();
}

TEST_T32(distant_literal_references) {
  SETUP();
  START();

  Literal<uint64_t>* literal =
      new Literal<uint64_t>(UINT64_C(0x0123456789abcdef),
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);
  // Refer to the literal so that it is emitted early.
  __ Ldr(r0, literal);

  // Add enough nops to exceed the range of all loads.
  int space = 5000;
  {
    ExactAssemblyScope scope(&masm, space, CodeBufferCheckScope::kExactSize);
    VIXL_ASSERT(masm.IsUsingT32());
    for (int i = 0; i < space; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

#define ENSURE_ALIGNED()                                                      \
  do {                                                                        \
    if (!IsMultiple<k32BitT32InstructionSizeInBytes>(                         \
            masm.GetCursorOffset())) {                                        \
      ExactAssemblyScope scope(&masm,                                         \
                               k16BitT32InstructionSizeInBytes,               \
                               ExactAssemblyScope::kExactSize);               \
      __ nop();                                                               \
    }                                                                         \
    VIXL_ASSERT(                                                              \
        IsMultiple<k32BitT32InstructionSizeInBytes>(masm.GetCursorOffset())); \
  } while (0)

  // The literal has already been emitted, and is out of range of all of these
  // instructions. The delegates must generate fix-up code.
  ENSURE_ALIGNED();
  __ Ldr(r1, literal);
  ENSURE_ALIGNED();
  __ Ldrb(r2, literal);
  ENSURE_ALIGNED();
  __ Ldrsb(r3, literal);
  ENSURE_ALIGNED();
  __ Ldrh(r4, literal);
  ENSURE_ALIGNED();
  __ Ldrsh(r5, literal);
  ENSURE_ALIGNED();
  __ Ldrd(r6, r7, literal);
  ENSURE_ALIGNED();
  __ Vldr(d0, literal);
  ENSURE_ALIGNED();
  __ Vldr(s3, literal);

#undef ENSURE_ALIGNED

  END();
  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x89abcdef, r0);
  ASSERT_EQUAL_32(0x89abcdef, r1);
  ASSERT_EQUAL_32(0xef, r2);
  ASSERT_EQUAL_32(0xffffffef, r3);
  ASSERT_EQUAL_32(0xcdef, r4);
  ASSERT_EQUAL_32(0xffffcdef, r5);
  ASSERT_EQUAL_32(0x89abcdef, r6);
  ASSERT_EQUAL_32(0x01234567, r7);
  ASSERT_EQUAL_FP64(RawbitsToDouble(0x0123456789abcdef), d0);
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s3);
}


TEST_T32(distant_literal_references_unaligned_pc) {
  SETUP();
  START();

  Literal<uint64_t>* literal =
      new Literal<uint64_t>(UINT64_C(0x0123456789abcdef),
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);
  // Refer to the literal so that it is emitted early.
  __ Ldr(r0, literal);

  // Add enough nops to exceed the range of all loads, leaving the PC aligned
  // to only a two-byte boundary.
  int space = 5002;
  {
    ExactAssemblyScope scope(&masm, space, CodeBufferCheckScope::kExactSize);
    VIXL_ASSERT(masm.IsUsingT32());
    for (int i = 0; i < space; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

#define ENSURE_NOT_ALIGNED()                                                   \
  do {                                                                         \
    if (IsMultiple<k32BitT32InstructionSizeInBytes>(masm.GetCursorOffset())) { \
      ExactAssemblyScope scope(&masm,                                          \
                               k16BitT32InstructionSizeInBytes,                \
                               ExactAssemblyScope::kExactSize);                \
      __ nop();                                                                \
    }                                                                          \
    VIXL_ASSERT(                                                               \
        !IsMultiple<k32BitT32InstructionSizeInBytes>(masm.GetCursorOffset())); \
  } while (0)

  // The literal has already been emitted, and is out of range of all of these
  // instructions. The delegates must generate fix-up code.
  ENSURE_NOT_ALIGNED();
  __ Ldr(r1, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrb(r2, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrsb(r3, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrh(r4, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrsh(r5, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrd(r6, r7, literal);
  {
    // TODO: We currently require an extra scratch register for these cases
    // because MemOperandComputationHelper isn't able to fit add_sub_offset into
    // a single 'sub' instruction, so 'pc' gets preserved first. The same
    // problem technically exists for the other loads, but vldr is particularly
    // badly affected because vldr cannot set the low bits in its offset mask,
    // so the add/sub operand is likely to be difficult to encode.
    //
    // At the moment, we get this:
    //     mov r8, pc
    //     mov ip, #5118
    //     sub r8, pc
    //     vldr d0, [r8, #48]
    //
    // We should be able to generate something like this:
    //     sub ip, pc, #0x1300    // 5118 & 0xff00
    //     sub ip, #0xfe          // 5118 & 0x00ff
    //     vldr d0, [ip, #48]
    UseScratchRegisterScope temps(&masm);
    temps.Include(r8);
    ENSURE_NOT_ALIGNED();
    __ Vldr(d0, literal);
    ENSURE_NOT_ALIGNED();
    __ Vldr(s3, literal);
  }

#undef ENSURE_NOT_ALIGNED

  END();
  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x89abcdef, r0);
  ASSERT_EQUAL_32(0x89abcdef, r1);
  ASSERT_EQUAL_32(0xef, r2);
  ASSERT_EQUAL_32(0xffffffef, r3);
  ASSERT_EQUAL_32(0xcdef, r4);
  ASSERT_EQUAL_32(0xffffcdef, r5);
  ASSERT_EQUAL_32(0x89abcdef, r6);
  ASSERT_EQUAL_32(0x01234567, r7);
  ASSERT_EQUAL_FP64(RawbitsToDouble(0x0123456789abcdef), d0);
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s3);
}


TEST_T32(distant_literal_references_short_range) {
  SETUP();
  START();

  Literal<uint64_t>* literal =
      new Literal<uint64_t>(UINT64_C(0x0123456789abcdef),
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);
  // Refer to the literal so that it is emitted early.
  __ Vldr(s4, literal);

  // Add enough nops to exceed the range of the loads, but not the adr that will
  // be generated to read the PC.
  int space = 4000;
  {
    ExactAssemblyScope scope(&masm, space, CodeBufferCheckScope::kExactSize);
    VIXL_ASSERT(masm.IsUsingT32());
    for (int i = 0; i < space; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

#define ENSURE_ALIGNED()                                                      \
  do {                                                                        \
    if (!IsMultiple<k32BitT32InstructionSizeInBytes>(                         \
            masm.GetCursorOffset())) {                                        \
      ExactAssemblyScope scope(&masm,                                         \
                               k16BitT32InstructionSizeInBytes,               \
                               ExactAssemblyScope::kExactSize);               \
      __ nop();                                                               \
    }                                                                         \
    VIXL_ASSERT(                                                              \
        IsMultiple<k32BitT32InstructionSizeInBytes>(masm.GetCursorOffset())); \
  } while (0)

  // The literal has already been emitted, and is out of range of all of these
  // instructions. The delegates must generate fix-up code.
  ENSURE_ALIGNED();
  __ Ldr(r1, literal);
  ENSURE_ALIGNED();
  __ Ldrb(r2, literal);
  ENSURE_ALIGNED();
  __ Ldrsb(r3, literal);
  ENSURE_ALIGNED();
  __ Ldrh(r4, literal);
  ENSURE_ALIGNED();
  __ Ldrsh(r5, literal);
  ENSURE_ALIGNED();
  __ Ldrd(r6, r7, literal);
  ENSURE_ALIGNED();
  __ Vldr(d0, literal);
  ENSURE_ALIGNED();
  __ Vldr(s3, literal);

#undef ENSURE_ALIGNED

  END();
  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s4);
  ASSERT_EQUAL_32(0x89abcdef, r1);
  ASSERT_EQUAL_32(0xef, r2);
  ASSERT_EQUAL_32(0xffffffef, r3);
  ASSERT_EQUAL_32(0xcdef, r4);
  ASSERT_EQUAL_32(0xffffcdef, r5);
  ASSERT_EQUAL_32(0x89abcdef, r6);
  ASSERT_EQUAL_32(0x01234567, r7);
  ASSERT_EQUAL_FP64(RawbitsToDouble(0x0123456789abcdef), d0);
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s3);
}


TEST_T32(distant_literal_references_short_range_unaligned_pc) {
  SETUP();
  START();

  Literal<uint64_t>* literal =
      new Literal<uint64_t>(UINT64_C(0x0123456789abcdef),
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);
  // Refer to the literal so that it is emitted early.
  __ Vldr(s4, literal);

  // Add enough nops to exceed the range of the loads, but not the adr that will
  // be generated to read the PC.
  int space = 4000;
  {
    ExactAssemblyScope scope(&masm, space, CodeBufferCheckScope::kExactSize);
    VIXL_ASSERT(masm.IsUsingT32());
    for (int i = 0; i < space; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

#define ENSURE_NOT_ALIGNED()                                                   \
  do {                                                                         \
    if (IsMultiple<k32BitT32InstructionSizeInBytes>(masm.GetCursorOffset())) { \
      ExactAssemblyScope scope(&masm,                                          \
                               k16BitT32InstructionSizeInBytes,                \
                               ExactAssemblyScope::kExactSize);                \
      __ nop();                                                                \
    }                                                                          \
    VIXL_ASSERT(                                                               \
        !IsMultiple<k32BitT32InstructionSizeInBytes>(masm.GetCursorOffset())); \
  } while (0)

  // The literal has already been emitted, and is out of range of all of these
  // instructions. The delegates must generate fix-up code.
  ENSURE_NOT_ALIGNED();
  __ Ldr(r1, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrb(r2, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrsb(r3, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrh(r4, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrsh(r5, literal);
  ENSURE_NOT_ALIGNED();
  __ Ldrd(r6, r7, literal);
  ENSURE_NOT_ALIGNED();
  __ Vldr(d0, literal);
  ENSURE_NOT_ALIGNED();
  __ Vldr(s3, literal);

#undef ENSURE_NOT_ALIGNED

  END();
  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s4);
  ASSERT_EQUAL_32(0x89abcdef, r1);
  ASSERT_EQUAL_32(0xef, r2);
  ASSERT_EQUAL_32(0xffffffef, r3);
  ASSERT_EQUAL_32(0xcdef, r4);
  ASSERT_EQUAL_32(0xffffcdef, r5);
  ASSERT_EQUAL_32(0x89abcdef, r6);
  ASSERT_EQUAL_32(0x01234567, r7);
  ASSERT_EQUAL_FP64(RawbitsToDouble(0x0123456789abcdef), d0);
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s3);
}


TEST_T32(distant_literal_references_long_range) {
  SETUP();
  START();

  Literal<uint64_t>* literal =
      new Literal<uint64_t>(UINT64_C(0x0123456789abcdef),
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);
  // Refer to the literal so that it is emitted early.
  __ Ldr(r0, literal);

#define PAD_WITH_NOPS(space)                                             \
  do {                                                                   \
    {                                                                    \
      ExactAssemblyScope scope(&masm,                                    \
                               space,                                    \
                               CodeBufferCheckScope::kExactSize);        \
      VIXL_ASSERT(masm.IsUsingT32());                                    \
      for (int i = 0; i < space; i += k16BitT32InstructionSizeInBytes) { \
        __ nop();                                                        \
      }                                                                  \
    }                                                                    \
  } while (0)

  // Add enough nops to exceed the range of all loads.
  PAD_WITH_NOPS(5000);

  // The literal has already been emitted, and is out of range of all of these
  // instructions. The delegates must generate fix-up code.
  __ Ldr(r1, literal);
  __ Ldrb(r2, literal);
  __ Ldrsb(r3, literal);
  __ Ldrh(r4, literal);
  __ Ldrsh(r5, literal);
  __ Ldrd(r6, r7, literal);
  __ Vldr(d0, literal);
  __ Vldr(s3, literal);

  // Add enough nops to exceed the range of the adr+sub sequence.
  PAD_WITH_NOPS(0x421000);

  __ Ldr(r1, literal);
  __ Ldrb(r2, literal);
  __ Ldrsb(r3, literal);
  __ Ldrh(r4, literal);
  __ Ldrsh(r5, literal);
  __ Ldrd(r6, r7, literal);
  {
    // TODO: We currently require an extra scratch register for these cases. We
    // should be able to optimise the code generation to avoid this requirement
    // (and in many cases avoid a 32-bit instruction).
    UseScratchRegisterScope temps(&masm);
    temps.Include(r8);
    __ Vldr(d0, literal);
    __ Vldr(s3, literal);
  }

#undef PAD_WITH_NOPS

  END();
  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x89abcdef, r0);
  ASSERT_EQUAL_32(0x89abcdef, r1);
  ASSERT_EQUAL_32(0xef, r2);
  ASSERT_EQUAL_32(0xffffffef, r3);
  ASSERT_EQUAL_32(0xcdef, r4);
  ASSERT_EQUAL_32(0xffffcdef, r5);
  ASSERT_EQUAL_32(0x89abcdef, r6);
  ASSERT_EQUAL_32(0x01234567, r7);
  ASSERT_EQUAL_FP64(RawbitsToDouble(0x0123456789abcdef), d0);
  ASSERT_EQUAL_FP32(RawbitsToFloat(0x89abcdef), s3);
}


TEST(barriers) {
  // Generate all supported barriers, this is just a smoke test
  SETUP();

  START();

  // DMB
  __ Dmb(SY);
  __ Dmb(ST);
  __ Dmb(ISH);
  __ Dmb(ISHST);
  __ Dmb(NSH);
  __ Dmb(NSHST);
  __ Dmb(OSH);
  __ Dmb(OSHST);

  // DSB
  __ Dsb(SY);
  __ Dsb(ST);
  __ Dsb(ISH);
  __ Dsb(ISHST);
  __ Dsb(NSH);
  __ Dsb(NSHST);
  __ Dsb(OSH);
  __ Dsb(OSHST);

  // ISB
  __ Isb(SY);

  END();
}


TEST(preloads) {
  // Smoke test for various pld/pli forms.
  SETUP();

  START();

  // PLD immediate
  __ Pld(MemOperand(sp, 0));
  __ Pld(MemOperand(r0, 0));
  __ Pld(MemOperand(r1, 123));
  __ Pld(MemOperand(r2, 1234));
  __ Pld(MemOperand(r3, 4095));
  __ Pld(MemOperand(r4, -123));
  __ Pld(MemOperand(r5, -255));

  if (masm.IsUsingA32()) {
    __ Pld(MemOperand(r6, -1234));
    __ Pld(MemOperand(r7, -4095));
  }


  // PLDW immediate
  __ Pldw(MemOperand(sp, 0));
  __ Pldw(MemOperand(r0, 0));
  __ Pldw(MemOperand(r1, 123));
  __ Pldw(MemOperand(r2, 1234));
  __ Pldw(MemOperand(r3, 4095));
  __ Pldw(MemOperand(r4, -123));
  __ Pldw(MemOperand(r5, -255));

  if (masm.IsUsingA32()) {
    __ Pldw(MemOperand(r6, -1234));
    __ Pldw(MemOperand(r7, -4095));
  }

  // PLD register
  __ Pld(MemOperand(r0, r1));
  __ Pld(MemOperand(r0, r1, LSL, 1));
  __ Pld(MemOperand(r0, r1, LSL, 2));
  __ Pld(MemOperand(r0, r1, LSL, 3));

  if (masm.IsUsingA32()) {
    __ Pld(MemOperand(r0, r1, LSL, 4));
    __ Pld(MemOperand(r0, r1, LSL, 20));
  }

  // PLDW register
  __ Pldw(MemOperand(r0, r1));
  __ Pldw(MemOperand(r0, r1, LSL, 1));
  __ Pldw(MemOperand(r0, r1, LSL, 2));
  __ Pldw(MemOperand(r0, r1, LSL, 3));

  if (masm.IsUsingA32()) {
    __ Pldw(MemOperand(r0, r1, LSL, 4));
    __ Pldw(MemOperand(r0, r1, LSL, 20));
  }

  // PLI immediate
  __ Pli(MemOperand(sp, 0));
  __ Pli(MemOperand(r0, 0));
  __ Pli(MemOperand(r1, 123));
  __ Pli(MemOperand(r2, 1234));
  __ Pli(MemOperand(r3, 4095));
  __ Pli(MemOperand(r4, -123));
  __ Pli(MemOperand(r5, -255));

  if (masm.IsUsingA32()) {
    __ Pli(MemOperand(r6, -1234));
    __ Pli(MemOperand(r7, -4095));
  }

  // PLI register
  __ Pli(MemOperand(r0, r1));
  __ Pli(MemOperand(r0, r1, LSL, 1));
  __ Pli(MemOperand(r0, r1, LSL, 2));
  __ Pli(MemOperand(r0, r1, LSL, 3));

  if (masm.IsUsingA32()) {
    __ Pli(MemOperand(r0, r1, LSL, 4));
    __ Pli(MemOperand(r0, r1, LSL, 20));
  }

  END();
}


TEST_T32(veneer_mirrored_branches) {
  SETUP();

  START();

  const int kMaxBranchCount = 256;

  for (int branch_count = 1; branch_count < kMaxBranchCount; branch_count++) {
    Label* targets = new Label[branch_count];

    for (int i = 0; i < branch_count; i++) {
      __ Cbz(r0, &targets[i]);
    }

    for (int i = 0; i < branch_count; i++) {
      __ Bind(&targets[branch_count - i - 1]);
      __ Orr(r0, r0, r0);
    }

    delete[] targets;
  }

  END();
}


TEST_T32(branch_fuzz_example) {
  SETUP();

  START();

  Label l[64];
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[30]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[22]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[15]);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[6]);
  __ Bind(&l[26]);
  __ Cbz(r0, &l[29]);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[22]);
  __ Bind(&l[12]);
  __ Bind(&l[22]);
  __ Cbz(r0, &l[10]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[30]);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[27]);
  __ Cbz(r0, &l[11]);
  __ Bind(&l[7]);
  __ Cbz(r0, &l[18]);
  __ Bind(&l[14]);
  __ Cbz(r0, &l[1]);
  __ Bind(&l[18]);
  __ Cbz(r0, &l[11]);
  __ Cbz(r0, &l[6]);
  __ Bind(&l[21]);
  __ Cbz(r0, &l[28]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[22]);
  __ Bind(&l[23]);
  __ Cbz(r0, &l[21]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[9]);
  __ Bind(&l[9]);
  __ Cbz(r0, &l[4]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[10]);
  __ And(r0, r0, r0);
  __ Bind(&l[8]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[10]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[17]);
  __ Bind(&l[10]);
  __ Cbz(r0, &l[8]);
  __ Cbz(r0, &l[25]);
  __ Cbz(r0, &l[4]);
  __ Bind(&l[28]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[16]);
  __ Bind(&l[19]);
  __ Cbz(r0, &l[14]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[26]);
  __ Cbz(r0, &l[21]);
  __ And(r0, r0, r0);
  __ Bind(&l[24]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[19]);
  __ Cbz(r0, &l[26]);
  __ Cbz(r0, &l[4]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[27]);
  __ Cbz(r0, &l[14]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[18]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[6]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[15]);
  __ Cbz(r0, &l[0]);
  __ Cbz(r0, &l[10]);
  __ Cbz(r0, &l[16]);
  __ Cbz(r0, &l[30]);
  __ Cbz(r0, &l[8]);
  __ Cbz(r0, &l[16]);
  __ Cbz(r0, &l[22]);
  __ Cbz(r0, &l[27]);
  __ Cbz(r0, &l[12]);
  __ Cbz(r0, &l[0]);
  __ Cbz(r0, &l[23]);
  __ Cbz(r0, &l[27]);
  __ Cbz(r0, &l[16]);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[4]);
  __ Cbz(r0, &l[11]);
  __ Cbz(r0, &l[6]);
  __ Cbz(r0, &l[23]);
  __ Bind(&l[16]);
  __ Cbz(r0, &l[10]);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[12]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[11]);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[3]);
  __ Cbz(r0, &l[18]);
  __ Bind(&l[4]);
  __ Cbz(r0, &l[31]);
  __ Cbz(r0, &l[25]);
  __ Cbz(r0, &l[22]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[19]);
  __ Cbz(r0, &l[16]);
  __ Cbz(r0, &l[21]);
  __ Cbz(r0, &l[27]);
  __ Bind(&l[1]);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[13]);
  __ Cbz(r0, &l[10]);
  __ Cbz(r0, &l[6]);
  __ Cbz(r0, &l[30]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[7]);
  __ Cbz(r0, &l[17]);
  __ Bind(&l[0]);
  __ Cbz(r0, &l[13]);
  __ Cbz(r0, &l[11]);
  __ Cbz(r0, &l[19]);
  __ Cbz(r0, &l[22]);
  __ Cbz(r0, &l[9]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[15]);
  __ Cbz(r0, &l[31]);
  __ Cbz(r0, &l[2]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[6]);
  __ Bind(&l[27]);
  __ Bind(&l[13]);
  __ Cbz(r0, &l[23]);
  __ Cbz(r0, &l[7]);
  __ Bind(&l[2]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[1]);
  __ Bind(&l[15]);
  __ Cbz(r0, &l[13]);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[8]);
  __ Cbz(r0, &l[30]);
  __ Cbz(r0, &l[8]);
  __ Cbz(r0, &l[27]);
  __ Cbz(r0, &l[2]);
  __ Cbz(r0, &l[31]);
  __ Cbz(r0, &l[4]);
  __ Cbz(r0, &l[11]);
  __ Bind(&l[29]);
  __ Cbz(r0, &l[7]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[11]);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[3]);
  __ Cbz(r0, &l[3]);
  __ Cbz(r0, &l[22]);
  __ Cbz(r0, &l[19]);
  __ Cbz(r0, &l[4]);
  __ Bind(&l[6]);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[3]);
  __ Cbz(r0, &l[23]);
  __ Cbz(r0, &l[12]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[22]);
  __ Cbz(r0, &l[24]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[16]);
  __ Cbz(r0, &l[19]);
  __ Cbz(r0, &l[20]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[4]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[25]);
  __ Cbz(r0, &l[21]);
  __ Cbz(r0, &l[20]);
  __ Cbz(r0, &l[29]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[10]);
  __ Cbz(r0, &l[5]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[25]);
  __ Cbz(r0, &l[26]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[19]);
  __ And(r0, r0, r0);
  __ Bind(&l[17]);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[6]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[26]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[24]);
  __ Bind(&l[20]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[10]);
  __ Cbz(r0, &l[19]);
  __ Cbz(r0, &l[6]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[13]);
  __ Cbz(r0, &l[15]);
  __ Cbz(r0, &l[22]);
  __ Cbz(r0, &l[8]);
  __ Cbz(r0, &l[6]);
  __ Cbz(r0, &l[23]);
  __ Cbz(r0, &l[6]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[13]);
  __ Bind(&l[31]);
  __ Cbz(r0, &l[14]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[27]);
  __ Cbz(r0, &l[10]);
  __ Cbz(r0, &l[30]);
  __ Cbz(r0, &l[14]);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[26]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[2]);
  __ Cbz(r0, &l[21]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[24]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[17]);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[24]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[17]);
  __ Cbz(r0, &l[12]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[31]);
  __ Cbz(r0, &l[25]);
  __ And(r0, r0, r0);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[13]);
  __ Cbz(r0, &l[14]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[5]);
  __ Cbz(r0, &l[12]);
  __ Cbz(r0, &l[3]);
  __ Cbz(r0, &l[25]);
  __ Bind(&l[11]);
  __ Cbz(r0, &l[15]);
  __ Cbz(r0, &l[20]);
  __ Cbz(r0, &l[22]);
  __ Cbz(r0, &l[19]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[19]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[21]);
  __ Cbz(r0, &l[0]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[16]);
  __ Cbz(r0, &l[28]);
  __ Cbz(r0, &l[18]);
  __ Cbz(r0, &l[3]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[15]);
  __ Cbz(r0, &l[8]);
  __ Cbz(r0, &l[25]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[21]);
  __ Cbz(r0, &l[1]);
  __ Cbz(r0, &l[29]);
  __ Cbz(r0, &l[15]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[24]);
  __ Cbz(r0, &l[3]);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[9]);
  __ Cbz(r0, &l[24]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[19]);
  __ And(r0, r0, r0);
  __ Cbz(r0, &l[30]);
  __ Bind(&l[25]);
  __ Bind(&l[3]);
  __ Bind(&l[30]);
  __ Bind(&l[5]);

  END();
}


// Generate a "B" and a "Cbz" which have the same checkpoint. Without proper
// management (i.e. if the veneers were only generated at the shared
// checkpoint), one one of the branches would be out of range.
TEST_T32(veneer_simultaneous) {
  SETUP();

  START();

  // `2046` max range - the size of the B.EQ itself.
  static const int kMaxBCondRange = 1048574;

  Label target_1;
  Label target_2;

  __ B(eq, &target_1);

  int target_1_size_1 =
      kMaxBCondRange - kCbzCbnzRange - k32BitT32InstructionSizeInBytes;
  int end_1 = masm.GetCursorOffset() + target_1_size_1;
  while (masm.GetCursorOffset() < end_1) {
    __ Nop();
  }

  __ Cbz(r0, &target_2);

  int target_1_size_2 = kCbzCbnzRange - k16BitT32InstructionSizeInBytes;
  int end_2 = masm.GetCursorOffset() + target_1_size_2;
  while (masm.GetCursorOffset() < end_2) {
    __ Nop();
  }

  __ Nop();

  __ Bind(&target_1);
  __ Bind(&target_2);

  END();
}


// Generate a "B" and a "Cbz" which have the same checkpoint and the same label.
TEST_T32(veneer_simultaneous_one_label) {
  SETUP();

  START();

  // `2046` max range - the size of the B.EQ itself.
  static const int kMaxBCondRange = 1048574;

  Label target;

  __ B(eq, &target);

  int target_1_size_1 =
      kMaxBCondRange - kCbzCbnzRange - k32BitT32InstructionSizeInBytes;
  int end_1 = masm.GetCursorOffset() + target_1_size_1;
  while (masm.GetCursorOffset() < end_1) {
    __ Nop();
  }

  __ Cbz(r0, &target);

  int target_1_size_2 = kCbzCbnzRange - k16BitT32InstructionSizeInBytes;
  int end_2 = masm.GetCursorOffset() + target_1_size_2;
  while (masm.GetCursorOffset() < end_2) {
    __ Nop();
  }

  __ Nop();

  __ Bind(&target);

  END();
}

// NOTE: This test has needed modifications for the new pool manager, as it
// was testing a corner case of the previous pool managers. We keep it as
// another testcase.
TEST_T32(veneer_and_literal) {
  SETUP();

  START();

  VIXL_CHECK(test.PoolIsEmpty());

  const uint32_t ldrd_range = 1020;
  const uint32_t cbz_range = 126;
  const uint32_t kLabelsCount = 20;
  Label labels[kLabelsCount];

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);

  // Generate some nops.
  uint32_t i = 0;
  for (; i < ldrd_range - cbz_range - 40;
       i += k16BitT32InstructionSizeInBytes) {
    __ Nop();
  }

  // At this point, it remains cbz_range + 40 => 166 bytes before ldrd becomes
  // out of range.
  // We generate kLabelsCount * 4 => 80 bytes. We shouldn't generate the
  // literal pool.
  for (uint32_t j = 0; j < kLabelsCount; j++) {
    __ Cbz(r0, &labels[j]);
    __ Nop();
    i += 2 * k16BitT32InstructionSizeInBytes;
  }

  // We generate a few more instructions.
  for (; i < ldrd_range - 4 * kA32InstructionSizeInBytes;
       i += k16BitT32InstructionSizeInBytes) {
    __ Nop();
  }

  // Bind all the used labels.
  for (uint32_t j = 0; j < kLabelsCount; j++) {
    __ Bind(&labels[j]);
    __ Nop();
  }

  // Now that all the labels have been bound, we have no more veneers.
  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}

// NOTE: This test has needed modifications for the new pool manager, as it
// was testing a corner case of the previous pool managers. We keep it as
// another testcase.
TEST_T32(veneer_and_literal2) {
  SETUP();

  START();

  VIXL_CHECK(test.PoolIsEmpty());

  const uint32_t ldrd_range = 1020;
  const uint32_t cbz_range = 126;
  const uint32_t kLabelsCount = 20;
  const int32_t kTypicalMacroInstructionMaxSize =
      8 * kMaxInstructionSizeInBytes;
  Label labels[kLabelsCount];

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);

  for (uint32_t i = 0; i < ldrd_range - cbz_range - 4 * kLabelsCount;
       i += k16BitT32InstructionSizeInBytes) {
    __ Nop();
  }

  // Add entries to the veneer pool.
  for (uint32_t i = 0; i < kLabelsCount; i++) {
    __ Cbz(r0, &labels[i]);
    __ Nop();
  }

  // Generate nops up to the literal pool limit.
  while (test.GetPoolCheckpoint() - masm.GetCursorOffset() >=
         kTypicalMacroInstructionMaxSize) {
    __ Nop();
  }

  // At this point, no literals and no veneers have been generated.
  VIXL_ASSERT(!test.PoolIsEmpty());
  // The literal pool needs to be generated.
  VIXL_ASSERT(test.GetPoolCheckpoint() - masm.GetCursorOffset() <
              kTypicalMacroInstructionMaxSize);

  // This extra Nop will generate the pools.
  __ Nop();

  // Bind all the used labels.
  for (uint32_t j = 0; j < kLabelsCount; j++) {
    __ Bind(&labels[j]);
    __ Nop();
  }

  // Now that all the labels have been bound, we have no more veneers.
  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}


// Use a literal when we already have a veneer pool potential size greater than
// the literal range => generate the literal immediately (not optimum but it
// works).
TEST_T32(veneer_and_literal3) {
  SETUP();

  START();

  static const int kLabelsCount = 1000;

  Label labels[kLabelsCount];

  // Set the Z flag so that the following branches are not taken.
  __ Movs(r0, 0);

  for (int i = 0; i < kLabelsCount; i++) {
    __ B(ne, &labels[i]);
  }

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);

  for (int i = 0; i < 10; i++) {
    __ Nop();
  }

  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&labels[i]);
  }

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}


// Literal has to be generated sooner than veneers. However, as the literal
// pool generation would make the veneers out of range, generate the veneers
// first.
TEST_T32(veneer_and_literal4) {
  SETUP();

  START();

  Label end;

  // Set the Z flag so that the following branch is not taken.
  __ Movs(r0, 0);
  __ B(ne, &end);

  uint32_t value = 0x1234567;
  Literal<uint32_t>* literal =
      new Literal<uint32_t>(value,
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);

  __ Ldr(r11, literal);

  // The range for ldr is 4095, the range for cbz is 127. Generate nops
  // to have the ldr becomming out of range just before the cbz.
  const int NUM_NOPS = 2044;
  const int NUM_RANGE = 58;

  const int NUM1 = NUM_NOPS - NUM_RANGE;
  const int NUM2 = NUM_RANGE;

  {
    ExactAssemblyScope aas(&masm, 2 * NUM1, CodeBufferCheckScope::kMaximumSize);
    for (int i = 0; i < NUM1; i++) {
      __ nop();
    }
  }

  __ Cbz(r1, &end);

  {
    ExactAssemblyScope aas(&masm, 2 * NUM2, CodeBufferCheckScope::kMaximumSize);
    for (int i = 0; i < NUM2; i++) {
      __ nop();
    }
  }

  {
    ExactAssemblyScope aas(&masm, 4, CodeBufferCheckScope::kMaximumSize);
    __ add(r1, r1, 3);
  }
  __ Bind(&end);

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x1234567, r11);
}


// Literal has to be generated sooner than veneers. However, as the literal
// pool generation would make the veneers out of range, generate the veneers
// first.
TEST_T32(veneer_and_literal5) {
  SETUP();

  START();

  static const int kTestCount = 100;
  Label labels[kTestCount];

  int first_test = 2000;
  // Test on both sizes of the Adr range which is 4095.
  for (int test = 0; test < kTestCount; test++) {
    const int string_size = 1000;  // A lot more than the cbz range.
    std::string test_string(string_size, 'x');
    StringLiteral big_literal(test_string.c_str());

    __ Adr(r11, &big_literal);

    {
      int num_nops = first_test + test;
      ExactAssemblyScope aas(&masm,
                             2 * num_nops,
                             CodeBufferCheckScope::kMaximumSize);
      for (int i = 0; i < num_nops; i++) {
        __ nop();
      }
    }

    __ Cbz(r1, &labels[test]);

    {
      ExactAssemblyScope aas(&masm, 4, CodeBufferCheckScope::kMaximumSize);
      __ add(r1, r1, 3);
    }
    __ Bind(&labels[test]);
    // Emit the literal pool if it has not beeen emitted (it's the case for
    // the lower values of test).
    __ EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  }

  END();
}

// Check that veneer and literals are well generated when they are out of
// range at the same time.
TEST_T32(veneer_and_literal6) {
  SETUP();

  START();

  Label t1, t2, t3, t4, t5;
  static const int kLdrdRange = 1020;
  static const int kSizeForCbz = k16BitT32InstructionSizeInBytes;

  __ Ldrd(r0, r1, 0x1111111111111111);
  __ Ldrd(r2, r3, 0x2222222222222222);
  __ Ldrd(r4, r5, 0x3333333333333333);
  __ Ldrd(r6, r7, 0x4444444444444444);
  __ Ldrd(r8, r9, 0x5555555555555555);
  __ Ldrd(r10, r11, 0x6666666666666666);
  __ Ldrd(r10, r11, 0x1234567890abcdef);

  // Ldrd has a bigger range that cbz. Generate some nops before the cbzs in
  // order to reach the maximum range of ldrd and cbz at the same time.
  {
    int nop_size = kLdrdRange - kCbzCbnzRange - 5 * kSizeForCbz;
    ExactAssemblyScope scope(&masm, nop_size, CodeBufferCheckScope::kExactSize);
    for (int i = 0; i < nop_size; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

  __ Cbz(r2, &t1);
  __ Cbz(r2, &t2);
  __ Cbz(r2, &t3);
  __ Cbz(r2, &t4);
  __ Cbz(r2, &t5);

  // At this point, the ldrds are not out of range. It remains a kCbzCbnzRange
  // margin (minus the size of the veneers).

  // At this point, the literal and the veneer pools are not emitted.
  const int kLdrdLiteralSize = 8;
  const int kVeneerSize = 4;
  CHECK_POOL_SIZE(7 * kLdrdLiteralSize + 5 * kVeneerSize);
  VIXL_CHECK(test.GetPoolCheckpoint() - masm.GetCursorOffset() < kCbzCbnzRange);

  // This scope will generate both veneers (they are both out of range).
  {
    int nop_size = kCbzCbnzRange;
    ExactAssemblyScope scope(&masm, nop_size, CodeBufferCheckScope::kExactSize);
    for (int i = 0; i < nop_size; i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

  // Check that both literals and veneers have been emitted.
  CHECK_POOL_SIZE(5 * kVeneerSize);
  VIXL_CHECK(test.GetPoolCheckpoint() - masm.GetCursorOffset() > kCbzCbnzRange);

  __ Bind(&t1);
  __ Bind(&t2);
  __ Bind(&t3);
  __ Bind(&t4);
  __ Bind(&t5);

  CHECK_POOL_SIZE(0);

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x11111111, r0);
  ASSERT_EQUAL_32(0x11111111, r1);
  ASSERT_EQUAL_32(0x22222222, r2);
  ASSERT_EQUAL_32(0x22222222, r3);
  ASSERT_EQUAL_32(0x33333333, r4);
  ASSERT_EQUAL_32(0x33333333, r5);
  ASSERT_EQUAL_32(0x44444444, r6);
  ASSERT_EQUAL_32(0x44444444, r7);
  ASSERT_EQUAL_32(0x55555555, r8);
  ASSERT_EQUAL_32(0x55555555, r9);
  ASSERT_EQUAL_32(0x90abcdef, r10);
  ASSERT_EQUAL_32(0x12345678, r11);
}

// Check that a label which is just bound during the MacroEmissionCheckScope
// can be used.
TEST(ldr_label_bound_during_scope) {
  SETUP();
  START();

  Literal<uint64_t>* literal =
      new Literal<uint64_t>(UINT64_C(0x1234567890abcdef),
                            RawLiteral::kPlacedWhenUsed,
                            RawLiteral::kDeletedOnPoolDestruction);
  __ Ldrd(r0, r1, literal);

  const int nop_size = masm.IsUsingA32() ? 4 : 2;
  while (test.GetPoolCheckpoint() >=
         (masm.GetCursorOffset() +
          static_cast<int32_t>(kMaxInstructionSizeInBytes))) {
    ExactAssemblyScope scope(&masm, nop_size, ExactAssemblyScope::kExactSize);
    __ nop();
  }

  VIXL_ASSERT(!test.PoolIsEmpty());

  // This Ldrd will first generate the pool and then use literal which has just
  // been bound.
  __ Ldrd(r2, r3, literal);

  VIXL_ASSERT(test.PoolIsEmpty());

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0x90abcdef, r2);
  ASSERT_EQUAL_32(0x12345678, r3);
}


TEST_T32(test_it_scope_and_literal_pool) {
  // This test stresses the ITScope to make sure the number of bytes it tries
  // to emit is in sync with the MacroEmissionCheckScope that is around it.
  SETUP();

  START();

  // Make sure the pool is empty.
  masm.EmitLiteralPool(PoolManager<int32_t>::kBranchRequired);
  VIXL_CHECK(test.PoolIsEmpty());

  Literal<uint64_t> l0(0xcafebeefdeadbaba);
  __ Ldrd(r0, r1, &l0);
  // Leave exactly as many bytes between cursor and pool emission checkpoint as
  // the typical macro instruction needs (and MacroEmissionCheckScope allows
  // for).
  const int32_t kTypicalMacroInstructionMaxSize =
      8 * kMaxInstructionSizeInBytes;
  int32_t margin = test.GetPoolCheckpoint() - masm.GetCursorOffset() -
                   kTypicalMacroInstructionMaxSize;
  int32_t end = masm.GetCursorOffset() + margin;

  {
    ExactAssemblyScope scope(&masm, margin, ExactAssemblyScope::kExactSize);
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
  }
  VIXL_CHECK((test.GetPoolCheckpoint() - masm.GetCursorOffset()) ==
             kTypicalMacroInstructionMaxSize);

  // We cannot use an IT block for this instruction, hence ITScope will
  // generate a branch over it.
  __ Add(ne, r8, r9, 256);

  END();

  RUN();

  // Check that the literals loaded correctly.
  ASSERT_EQUAL_32(0xdeadbaba, r0);
  ASSERT_EQUAL_32(0xcafebeef, r1);
}


// TODO: Remove this limitation by having a sandboxing mechanism.
#if defined(VIXL_HOST_POINTER_32)
TEST(ldm_stm_no_writeback) {
  SETUP();

  START();

  const uint32_t src[4] = {0x12345678, 0x09abcdef, 0xc001c0de, 0xdeadbeef};
  uint32_t dst1[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
  uint32_t dst2[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};

  __ Mov(r0, reinterpret_cast<uintptr_t>(src));
  __ Ldm(r0, NO_WRITE_BACK, RegisterList(r1, r2, r3, r4));
  __ Ldm(r0, NO_WRITE_BACK, RegisterList(r5, r6, r9, r11));

  __ Mov(r0, reinterpret_cast<uintptr_t>(dst1));
  __ Stm(r0, NO_WRITE_BACK, RegisterList(r1, r2, r3, r4));

  __ Mov(r0, reinterpret_cast<uintptr_t>(dst2));
  __ Stm(r0, NO_WRITE_BACK, RegisterList(r5, r6, r9, r11));

  END();

  RUN();

  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0x09abcdef, r2);
  ASSERT_EQUAL_32(0xc001c0de, r3);
  ASSERT_EQUAL_32(0xdeadbeef, r4);

  ASSERT_EQUAL_32(0x12345678, r5);
  ASSERT_EQUAL_32(0x09abcdef, r6);
  ASSERT_EQUAL_32(0xc001c0de, r9);
  ASSERT_EQUAL_32(0xdeadbeef, r11);

  ASSERT_EQUAL_32(0x12345678, dst1[0]);
  ASSERT_EQUAL_32(0x09abcdef, dst1[1]);
  ASSERT_EQUAL_32(0xc001c0de, dst1[2]);
  ASSERT_EQUAL_32(0xdeadbeef, dst1[3]);

  ASSERT_EQUAL_32(0x12345678, dst2[0]);
  ASSERT_EQUAL_32(0x09abcdef, dst2[1]);
  ASSERT_EQUAL_32(0xc001c0de, dst2[2]);
  ASSERT_EQUAL_32(0xdeadbeef, dst2[3]);
}


TEST(ldm_stm_writeback) {
  SETUP();

  START();

  const uint32_t src[4] = {0x12345678, 0x09abcdef, 0xc001c0de, 0xdeadbeef};
  uint32_t dst[8] = {0x00000000,
                     0x00000000,
                     0x00000000,
                     0x00000000,
                     0x00000000,
                     0x00000000,
                     0x00000000,
                     0x00000000};

  __ Mov(r0, reinterpret_cast<uintptr_t>(src));
  __ Ldm(r0, WRITE_BACK, RegisterList(r2, r3));
  __ Ldm(r0, WRITE_BACK, RegisterList(r4, r5));

  __ Mov(r1, reinterpret_cast<uintptr_t>(dst));
  __ Stm(r1, WRITE_BACK, RegisterList(r2, r3, r4, r5));
  __ Stm(r1, WRITE_BACK, RegisterList(r2, r3, r4, r5));

  END();

  RUN();

  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(src + 4), r0);
  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(dst + 8), r1);

  ASSERT_EQUAL_32(0x12345678, r2);
  ASSERT_EQUAL_32(0x09abcdef, r3);
  ASSERT_EQUAL_32(0xc001c0de, r4);
  ASSERT_EQUAL_32(0xdeadbeef, r5);

  ASSERT_EQUAL_32(0x12345678, dst[0]);
  ASSERT_EQUAL_32(0x09abcdef, dst[1]);
  ASSERT_EQUAL_32(0xc001c0de, dst[2]);
  ASSERT_EQUAL_32(0xdeadbeef, dst[3]);
  ASSERT_EQUAL_32(0x12345678, dst[4]);
  ASSERT_EQUAL_32(0x09abcdef, dst[5]);
  ASSERT_EQUAL_32(0xc001c0de, dst[6]);
  ASSERT_EQUAL_32(0xdeadbeef, dst[7]);
}


TEST_A32(ldm_stm_da_ib) {
  SETUP();

  START();

  const uint32_t src1[4] = {0x33333333, 0x44444444, 0x11111111, 0x22222222};
  const uint32_t src2[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};

  uint32_t dst1[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
  uint32_t dst2[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};

  __ Mov(r11, reinterpret_cast<uintptr_t>(src1 + 3));
  __ Ldmda(r11, WRITE_BACK, RegisterList(r0, r1));
  __ Ldmda(r11, NO_WRITE_BACK, RegisterList(r2, r3));

  __ Mov(r10, reinterpret_cast<uintptr_t>(src2) - sizeof(src2[0]));
  __ Ldmib(r10, WRITE_BACK, RegisterList(r4, r5));
  __ Ldmib(r10, NO_WRITE_BACK, RegisterList(r6, r7));

  __ Mov(r9, reinterpret_cast<uintptr_t>(dst1 + 3));
  __ Stmda(r9, WRITE_BACK, RegisterList(r0, r1));
  __ Stmda(r9, NO_WRITE_BACK, RegisterList(r2, r3));

  __ Mov(r8, reinterpret_cast<uintptr_t>(dst2) - sizeof(dst2[0]));
  __ Stmib(r8, WRITE_BACK, RegisterList(r4, r5));
  __ Stmib(r8, NO_WRITE_BACK, RegisterList(r6, r7));


  END();

  RUN();

  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(src1 + 1), r11);
  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(src2 + 1), r10);
  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(dst1 + 1), r9);
  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(dst2 + 1), r8);

  ASSERT_EQUAL_32(0x11111111, r0);
  ASSERT_EQUAL_32(0x22222222, r1);
  ASSERT_EQUAL_32(0x33333333, r2);
  ASSERT_EQUAL_32(0x44444444, r3);

  ASSERT_EQUAL_32(0x11111111, r4);
  ASSERT_EQUAL_32(0x22222222, r5);
  ASSERT_EQUAL_32(0x33333333, r6);
  ASSERT_EQUAL_32(0x44444444, r7);

  ASSERT_EQUAL_32(0x33333333, dst1[0]);
  ASSERT_EQUAL_32(0x44444444, dst1[1]);
  ASSERT_EQUAL_32(0x11111111, dst1[2]);
  ASSERT_EQUAL_32(0x22222222, dst1[3]);

  ASSERT_EQUAL_32(0x11111111, dst2[0]);
  ASSERT_EQUAL_32(0x22222222, dst2[1]);
  ASSERT_EQUAL_32(0x33333333, dst2[2]);
  ASSERT_EQUAL_32(0x44444444, dst2[3]);
}


TEST(ldmdb_stmdb) {
  SETUP();

  START();

  const uint32_t src[6] =
      {0x55555555, 0x66666666, 0x33333333, 0x44444444, 0x11111111, 0x22222222};

  uint32_t dst[6] =
      {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

  __ Mov(r11, reinterpret_cast<uintptr_t>(src + 6));
  __ Ldmdb(r11, WRITE_BACK, RegisterList(r1, r2));
  __ Ldmdb(r11, WRITE_BACK, RegisterList(r3, r4));
  __ Ldmdb(r11, NO_WRITE_BACK, RegisterList(r5, r6));

  __ Mov(r10, reinterpret_cast<uintptr_t>(dst + 6));
  __ Stmdb(r10, WRITE_BACK, RegisterList(r5, r6));
  __ Stmdb(r10, WRITE_BACK, RegisterList(r3, r4));
  __ Stmdb(r10, NO_WRITE_BACK, RegisterList(r1, r2));

  END();

  RUN();

  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(src + 2), r11);
  ASSERT_EQUAL_32(reinterpret_cast<uintptr_t>(dst + 2), r10);

  ASSERT_EQUAL_32(0x11111111, r1);
  ASSERT_EQUAL_32(0x22222222, r2);
  ASSERT_EQUAL_32(0x33333333, r3);
  ASSERT_EQUAL_32(0x44444444, r4);
  ASSERT_EQUAL_32(0x55555555, r5);
  ASSERT_EQUAL_32(0x66666666, r6);

  ASSERT_EQUAL_32(0x11111111, dst[0]);
  ASSERT_EQUAL_32(0x22222222, dst[1]);
  ASSERT_EQUAL_32(0x33333333, dst[2]);
  ASSERT_EQUAL_32(0x44444444, dst[3]);
  ASSERT_EQUAL_32(0x55555555, dst[4]);
  ASSERT_EQUAL_32(0x66666666, dst[5]);
}
#endif


TEST(blx) {
  SETUP();

  START();

  // TODO(all): Ideally this test should jump back and forth between ARM and
  // Thumb mode and should also cover BLX immediate. Update this test if we
  // allow VIXL assembler to change ISA anywhere in the code buffer.

  Label test_start;
  Label func1;
  Label func2;

  __ B(&test_start);

  __ Bind(&func1);
  __ Mov(r0, 0x11111111);
  __ Push(lr);
  {
    size_t size_of_generated_code;
    if (masm.IsUsingA32()) {
      size_of_generated_code = 7 * kA32InstructionSizeInBytes;
    } else {
      size_of_generated_code = 5 * k32BitT32InstructionSizeInBytes +
                               3 * k16BitT32InstructionSizeInBytes;
    }
    ExactAssemblyScope scope(&masm,
                             size_of_generated_code,
                             ExactAssemblyScope::kExactSize);
    __ adr(r11, &func2);
    if (masm.IsUsingT32()) {
      // The jump target needs to have its least significant bit set to indicate
      // that we are jumping into thumb mode.
      __ orr(r11, r11, 1);
    }
    __ blx(r11);
    __ pop(lr);
    __ bx(lr);

    __ bind(&func2);
    __ movw(r1, 0x2222);
    __ movt(r1, 0x2222);
    __ bx(lr);
  }

  __ Bind(&test_start);
  __ Mov(r0, 0xdeadc0de);
  __ Mov(r1, 0xdeadc0de);
  __ Bl(&func1);

  END();

  RUN();

  // Really basic test to check that we reached the different parts of the test.
  ASSERT_EQUAL_32(0x11111111, r0);
  ASSERT_EQUAL_32(0x22222222, r1);
}

// Check that B with a near hint use a narrow branch when it can.
TEST_T32(b_near_hint) {
  SETUP();
  START();

  Label start;
  Label end;

  __ Bind(&start);
  __ Nop();

  {
    // Generate a branch which should be narrow.
    EmissionCheckScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(&start, kNear);
  }
  {
    ExactAssemblyScope scope(&masm,
                             kBNarrowRange,
                             ExactAssemblyScope::kExactSize);
    for (int32_t i = 0; i < kBNarrowRange;
         i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }
  {
    // Generate a branch which should be wide.
    EmissionCheckScope scope(&masm,
                             k32BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(&start, kNear);
  }
  {
    // Generate a forward branch which should be narrow.
    EmissionCheckScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(&end, kNear);
  }

  int32_t margin = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  VIXL_CHECK(margin < kBNarrowRange);

  {
    ExactAssemblyScope scope(&masm,
                             kBNarrowRange,
                             ExactAssemblyScope::kExactSize);
    for (int32_t i = 0; i < kBNarrowRange;
         i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

  // A veneer should have been generated.
  margin = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  VIXL_CHECK(margin > kBNarrowRange);

  __ Bind(&end);

  END();

  DISASSEMBLE();
}

// Check that B with a far hint use a narrow branch only for a near backward
// branch.
TEST_T32(b_far_hint) {
  SETUP();
  START();

  Label start;
  Label end;

  __ Bind(&start);
  __ Nop();

  {
    // Generate a branch which should be narrow.
    EmissionCheckScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(&start, kFar);
  }
  {
    ExactAssemblyScope scope(&masm,
                             kBNarrowRange,
                             ExactAssemblyScope::kExactSize);
    for (int32_t i = 0; i < kBNarrowRange;
         i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }
  {
    // Generate a branch which should be wide.
    EmissionCheckScope scope(&masm,
                             k32BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(&start, kFar);
  }
  {
    // Generate a forward branch which should be wide.
    EmissionCheckScope scope(&masm,
                             k32BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(&end, kFar);
  }

  __ Bind(&end);

  END();

  DISASSEMBLE();
}

// Check that conditional B with a near hint use a narrow branch when it can.
TEST_T32(b_conditional_near_hint) {
  SETUP();
  START();

  Label start;
  Label end;

  __ Bind(&start);
  __ Nop();
  {
    // Generate a branch which should be narrow.
    EmissionCheckScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(eq, &start, kNear);
  }
  {
    ExactAssemblyScope scope(&masm,
                             kBConditionalNarrowRange,
                             ExactAssemblyScope::kExactSize);
    for (int32_t i = 0; i < kBConditionalNarrowRange;
         i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }
  {
    // Generate a branch which should be wide.
    EmissionCheckScope scope(&masm,
                             k32BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(eq, &start, kNear);
  }
  {
    // Generate a forward branch which should be narrow.
    EmissionCheckScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(eq, &end, kNear);
  }

  int32_t margin = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  VIXL_CHECK(margin < kBConditionalNarrowRange);

  {
    ExactAssemblyScope scope(&masm,
                             kBConditionalNarrowRange,
                             ExactAssemblyScope::kExactSize);
    for (int32_t i = 0; i < kBConditionalNarrowRange;
         i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }

  // A veneer should have been generated.
  margin = test.GetPoolCheckpoint() - masm.GetCursorOffset();
  VIXL_CHECK(margin > kBConditionalNarrowRange);

  __ Bind(&end);

  END();

  DISASSEMBLE();
}

// Check that conditional B with a far hint use a narrow branch only for a
// near backward branch.
TEST_T32(b_conditional_far_hint) {
  SETUP();
  START();

  Label start;
  Label end;

  __ Bind(&start);
  __ Nop();

  {
    // Generate a branch which should be narrow.
    EmissionCheckScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(eq, &start, kFar);
  }
  {
    ExactAssemblyScope scope(&masm,
                             kBConditionalNarrowRange,
                             ExactAssemblyScope::kExactSize);
    for (int32_t i = 0; i < kBConditionalNarrowRange;
         i += k16BitT32InstructionSizeInBytes) {
      __ nop();
    }
  }
  {
    // Generate a branch which should be wide.
    EmissionCheckScope scope(&masm,
                             k32BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(eq, &start, kFar);
  }
  {
    // Generate a forward branch which should be wide.
    EmissionCheckScope scope(&masm,
                             k32BitT32InstructionSizeInBytes,
                             EmissionCheckScope::kExactSize);
    __ B(eq, &end, kFar);
  }

  __ Bind(&end);

  END();

  DISASSEMBLE();
}


// Check that the veneer pool is correctly emitted even if we do a lot of narrow
// branches.
TEST_T32(b_narrow_many) {
  SETUP();
  START();

  static const int kLabelsCount = kBNarrowRange / 2;

  Label labels[kLabelsCount];

  __ Mov(r0, 0);

  for (int i = 0; i < kLabelsCount; i++) {
    __ B(&labels[i], kNear);
  }

  __ Mov(r0, 1);
  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&labels[i]);
  }
  __ Nop();

  END();

  RUN();

  ASSERT_EQUAL_32(0, r0);
}


// Check that the veneer pool is correctly emitted even if we do a lot of narrow
// branches and cbz.
TEST_T32(b_narrow_and_cbz) {
  SETUP();
  START();

  static const int kLabelsCount = kBNarrowRange / 4;

  Label b_labels[kLabelsCount];
  Label cbz_labels[kLabelsCount];

  __ Mov(r0, 0);

  for (int i = 0; i < kLabelsCount; i++) {
    __ B(&b_labels[i], kNear);
    __ Cbz(r0, &cbz_labels[i]);
  }

  __ Mov(r0, 1);
  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&b_labels[i]);
  }

  __ Mov(r0, 2);
  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&cbz_labels[i]);
  }

  __ Nop();

  END();

  RUN();

  ASSERT_EQUAL_32(2, r0);
}


#define CHECK_SIZE_MATCH(ASM1, ASM2)                                 \
  {                                                                  \
    MacroAssembler masm1(BUF_SIZE);                                  \
    masm1.UseInstructionSet(isa);                                    \
    VIXL_ASSERT(masm1.GetCursorOffset() == 0);                       \
    masm1.ASM1;                                                      \
    masm1.FinalizeCode();                                            \
    int size1 = masm1.GetCursorOffset();                             \
                                                                     \
    MacroAssembler masm2(BUF_SIZE);                                  \
    masm2.UseInstructionSet(isa);                                    \
    VIXL_ASSERT(masm2.GetCursorOffset() == 0);                       \
    masm2.ASM2;                                                      \
    masm2.FinalizeCode();                                            \
    int size2 = masm2.GetCursorOffset();                             \
                                                                     \
    bool disassemble = Test::disassemble();                          \
    if (size1 != size2) {                                            \
      printf("Sizes did not match:\n");                              \
      disassemble = true;                                            \
    }                                                                \
    if (disassemble) {                                               \
      PrintDisassembler dis(std::cout, 0);                           \
      printf("// " #ASM1 "\n");                                      \
      if (masm1.IsUsingT32()) {                                      \
        dis.DisassembleT32Buffer(masm1.GetBuffer()                   \
                                     ->GetStartAddress<uint16_t*>(), \
                                 size1);                             \
      } else {                                                       \
        dis.DisassembleA32Buffer(masm1.GetBuffer()                   \
                                     ->GetStartAddress<uint32_t*>(), \
                                 size1);                             \
      }                                                              \
      printf("\n");                                                  \
                                                                     \
      dis.SetCodeAddress(0);                                         \
      printf("// " #ASM2 "\n");                                      \
      if (masm2.IsUsingT32()) {                                      \
        dis.DisassembleT32Buffer(masm2.GetBuffer()                   \
                                     ->GetStartAddress<uint16_t*>(), \
                                 size2);                             \
      } else {                                                       \
        dis.DisassembleA32Buffer(masm2.GetBuffer()                   \
                                     ->GetStartAddress<uint32_t*>(), \
                                 size2);                             \
      }                                                              \
      printf("\n");                                                  \
    }                                                                \
    VIXL_CHECK(size1 == size2);                                      \
  }


TEST_T32(macro_assembler_commute) {
  // Test that the MacroAssembler will commute operands if it means it can use a
  // 16-bit instruction with the same effect.

  // TODO: The commented-out tests should pass, but don't. When they are fixed,
  // we should update this test.

  // CHECK_SIZE_MATCH(Adc(DontCare, r7, r6, r7),
  //                  Adc(DontCare, r7, r7, r6));

  // CHECK_SIZE_MATCH(Adc(DontCare, eq, r7, r6, r7),
  //                  Adc(DontCare, eq, r7, r7, r6));

  CHECK_SIZE_MATCH(Add(DontCare, r1, r2, r7), Add(DontCare, r1, r7, r2));

  CHECK_SIZE_MATCH(Add(DontCare, lt, r1, r2, r7),
                   Add(DontCare, lt, r1, r7, r2));

  // CHECK_SIZE_MATCH(Add(DontCare, r4, r4, r10),
  //                  Add(DontCare, r4, r10, r4));

  // CHECK_SIZE_MATCH(Add(DontCare, eq, r4, r4, r10),
  //                  Add(DontCare, eq, r4, r10, r4));

  // CHECK_SIZE_MATCH(Add(DontCare, r7, sp, r7),
  //                  Add(DontCare, r7, r7, sp));

  // CHECK_SIZE_MATCH(Add(DontCare, eq, r7, sp, r7),
  //                  Add(DontCare, eq, r7, r7, sp));

  // CHECK_SIZE_MATCH(Add(DontCare, sp, sp, r10),
  //                  Add(DontCare, sp, r10, sp));

  // CHECK_SIZE_MATCH(Add(DontCare, eq, sp, sp, r10),
  //                  Add(DontCare, eq, sp, r10, sp));

  // CHECK_SIZE_MATCH(And(DontCare, r7, r7, r6),
  //                  And(DontCare, r7, r6, r7));

  // CHECK_SIZE_MATCH(And(DontCare, eq, r7, r7, r6),
  //                  And(DontCare, eq, r7, r6, r7));

  // CHECK_SIZE_MATCH(Eor(DontCare, r7, r7, r6),
  //                  Eor(DontCare, r7, r6, r7));

  // CHECK_SIZE_MATCH(Eor(DontCare, eq, r7, r7, r6),
  //                  Eor(DontCare, eq, r7, r6, r7));

  // CHECK_SIZE_MATCH(Mul(DontCare, r0, r1, r0),
  //                  Mul(DontCare, r0, r0, r1));

  // CHECK_SIZE_MATCH(Mul(DontCare, eq, r0, r1, r0),
  //                  Mul(DontCare, eq, r0, r0, r1));

  // CHECK_SIZE_MATCH(Orr(DontCare, r7, r7, r6),
  //                  Orr(DontCare, r7, r6, r7));

  // CHECK_SIZE_MATCH(Orr(DontCare, eq, r7, r7, r6),
  //                  Orr(DontCare, eq, r7, r6, r7));


  CHECK_SIZE_MATCH(Adc(r7, r6, r7), Adc(r7, r7, r6));

  // CHECK_SIZE_MATCH(Adc(eq, r7, r6, r7),
  //                  Adc(eq, r7, r7, r6));

  CHECK_SIZE_MATCH(Add(r1, r2, r7), Add(r1, r7, r2));

  CHECK_SIZE_MATCH(Add(lt, r1, r2, r7), Add(lt, r1, r7, r2));

  // CHECK_SIZE_MATCH(Add(r4, r4, r10),
  //                  Add(r4, r10, r4));

  // CHECK_SIZE_MATCH(Add(eq, r4, r4, r10),
  //                  Add(eq, r4, r10, r4));

  // CHECK_SIZE_MATCH(Add(r7, sp, r7),
  //                  Add(r7, r7, sp));

  // CHECK_SIZE_MATCH(Add(eq, r7, sp, r7),
  //                  Add(eq, r7, r7, sp));

  // CHECK_SIZE_MATCH(Add(sp, sp, r10),
  //                  Add(sp, r10, sp));

  // CHECK_SIZE_MATCH(Add(eq, sp, sp, r10),
  //                  Add(eq, sp, r10, sp));

  CHECK_SIZE_MATCH(And(r7, r7, r6), And(r7, r6, r7));

  // CHECK_SIZE_MATCH(And(eq, r7, r7, r6),
  //                  And(eq, r7, r6, r7));

  CHECK_SIZE_MATCH(Eor(r7, r7, r6), Eor(r7, r6, r7));

  // CHECK_SIZE_MATCH(Eor(eq, r7, r7, r6),
  //                  Eor(eq, r7, r6, r7));

  CHECK_SIZE_MATCH(Mul(r0, r1, r0), Mul(r0, r0, r1));

  // CHECK_SIZE_MATCH(Mul(eq, r0, r1, r0),
  //                  Mul(eq, r0, r0, r1));

  CHECK_SIZE_MATCH(Orr(r7, r7, r6), Orr(r7, r6, r7));

  // CHECK_SIZE_MATCH(Orr(eq, r7, r7, r6),
  //                  Orr(eq, r7, r6, r7));


  // CHECK_SIZE_MATCH(Adcs(r7, r6, r7),
  //                  Adcs(r7, r7, r6));

  // CHECK_SIZE_MATCH(Adcs(eq, r7, r6, r7),
  //                  Adcs(eq, r7, r7, r6));

  CHECK_SIZE_MATCH(Adds(r1, r2, r7), Adds(r1, r7, r2));

  CHECK_SIZE_MATCH(Adds(lt, r1, r2, r7), Adds(lt, r1, r7, r2));

  CHECK_SIZE_MATCH(Adds(r4, r4, r10), Adds(r4, r10, r4));

  CHECK_SIZE_MATCH(Adds(eq, r4, r4, r10), Adds(eq, r4, r10, r4));

  CHECK_SIZE_MATCH(Adds(r7, sp, r7), Adds(r7, r7, sp));

  CHECK_SIZE_MATCH(Adds(eq, r7, sp, r7), Adds(eq, r7, r7, sp));

  CHECK_SIZE_MATCH(Adds(sp, sp, r10), Adds(sp, r10, sp));

  CHECK_SIZE_MATCH(Adds(eq, sp, sp, r10), Adds(eq, sp, r10, sp));

  // CHECK_SIZE_MATCH(Ands(r7, r7, r6),
  //                  Ands(r7, r6, r7));

  // CHECK_SIZE_MATCH(Ands(eq, r7, r7, r6),
  //                  Ands(eq, r7, r6, r7));

  // CHECK_SIZE_MATCH(Eors(r7, r7, r6),
  //                  Eors(r7, r6, r7));

  // CHECK_SIZE_MATCH(Eors(eq, r7, r7, r6),
  //                  Eors(eq, r7, r6, r7));

  // CHECK_SIZE_MATCH(Muls(r0, r1, r0),
  //                  Muls(r0, r0, r1));

  // CHECK_SIZE_MATCH(Muls(eq, r0, r1, r0),
  //                  Muls(eq, r0, r0, r1));

  // CHECK_SIZE_MATCH(Orrs(r7, r7, r6),
  //                  Orrs(r7, r6, r7));

  // CHECK_SIZE_MATCH(Orrs(eq, r7, r7, r6),
  //                  Orrs(eq, r7, r6, r7));
}

TEST(emit_pool_when_manually_placing_literal) {
  SETUP();
  START();

  // Literal that will be manually placed.
  Literal<uint64_t> l0(0xcafebeefdeadbaba, RawLiteral::kManuallyPlaced);

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);

  // Branch using the assembler, to avoid introducing a veneer.
  Label over_literal;
  const int kBranchSize = 4;
  {
    ExactAssemblyScope scope(&masm,
                             kBranchSize,
                             ExactAssemblyScope::kExactSize);
    __ b(&over_literal);
  }

  // Almost reach the pool checkpoint.
  int32_t margin =
      test.GetPoolCheckpoint() - masm.GetCursorOffset() - l0.GetSize() / 2;
  int32_t end = masm.GetCursorOffset() + margin;
  {
    ExactAssemblyScope scope(&masm, margin, ExactAssemblyScope::kExactSize);
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
  }

  VIXL_CHECK(!test.PoolIsEmpty());
  __ Place(&l0);
  // The pool must now have been emitted.
  VIXL_CHECK(test.PoolIsEmpty());

  __ Bind(&over_literal);

  __ Ldrd(r2, r3, &l0);

  END();

  RUN();

  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
  ASSERT_EQUAL_32(0xdeadbaba, r2);
  ASSERT_EQUAL_32(0xcafebeef, r3);
}


// The addition of padding only happens for T32.
TEST_T32(emit_pool_when_adding_padding_due_to_bind) {
  SETUP();
  START();

  // Make sure we start with a 4-byte aligned address, in order for the
  // location where we will call Bind() to be 4-byte aligned.
  {
    ExactAssemblyScope scope(&masm,
                             k16BitT32InstructionSizeInBytes,
                             ExactAssemblyScope::kMaximumSize);
    while (masm.GetCursorOffset() % 4 != 0) {
      __ nop();
    }
  }

  // Create one literal pool entry.
  __ Ldrd(r0, r1, 0x1234567890abcdef);

  // Almost reach the pool checkpoint.
  const int kPaddingBytes = 2;
  int32_t margin =
      test.GetPoolCheckpoint() - masm.GetCursorOffset() - kPaddingBytes;
  int32_t end = masm.GetCursorOffset() + margin;
  {
    ExactAssemblyScope scope(&masm, margin, ExactAssemblyScope::kExactSize);
    while (masm.GetCursorOffset() < end) {
      __ nop();
    }
  }

  Label label;
  __ Cbz(r0, &label);

  VIXL_CHECK(!test.PoolIsEmpty());
  // In order to hit the case where binding the label needs to add padding,
  // we need this to be a 4-byte aligned address.
  VIXL_ASSERT((masm.GetBuffer()->GetCursorOffset() % 4) == 0);

  __ Bind(&label);
  // The pool must now have been emitted.
  VIXL_CHECK(test.PoolIsEmpty());

  END();

  RUN();

  ASSERT_EQUAL_32(0x90abcdef, r0);
  ASSERT_EQUAL_32(0x12345678, r1);
}

static void AddBranchesAndGetCloseToCheckpoint(MacroAssembler* masm,
                                               TestMacroAssembler* test,
                                               const int kLabelsCount,
                                               Label b_labels[],
                                               int32_t margin) {
  // Add many veneers to the pool.
  for (int i = 0; i < kLabelsCount; i++) {
    masm->B(&b_labels[i]);
  }

  // Get close to the veneer emission margin (considering the heuristic).
  // Use add instead of nop to make viewing the disassembled code easier.
  const int kAddSize = masm->IsUsingT32() ? k16BitT32InstructionSizeInBytes
                                          : kA32InstructionSizeInBytes;
  int32_t end = test->GetPoolCheckpoint();
  int32_t space = end - masm->GetCursorOffset() - margin;
  {
    ExactAssemblyScope scope(masm, space, ExactAssemblyScope::kExactSize);
    while (space > 0) {
      masm->add(r0, r0, r0);
      space -= kAddSize;
    }
  }

  // Make sure the veneers have not yet been emitted.
  const int kVeneerSize = 4;
  VIXL_CHECK(test->GetPoolSize() == kLabelsCount * kVeneerSize);
}

static void EmitIndividualNops(MacroAssembler* masm, const int kNops) {
  for (int i = 0; i < kNops; ++i) {
    masm->Nop();
  }
}

static void EmitNopsInExactAssemblyScope(MacroAssembler* masm,
                                         const int kNops) {
  const int kNopSize = masm->IsUsingT32() ? k16BitT32InstructionSizeInBytes
                                          : kA32InstructionSizeInBytes;
  {
    ExactAssemblyScope scope(masm,
                             kNops * kNopSize,
                             ExactAssemblyScope::kExactSize);
    for (int i = 0; i < kNops; i++) {
      masm->nop();
    }
  }
}

TEST_A32(literal_and_veneer_interaction_1) {
  SETUP();
  START();

  static const int kLabelsCount = 100;

  Label b_labels[kLabelsCount];

  AddBranchesAndGetCloseToCheckpoint(&masm,
                                     &test,
                                     kLabelsCount,
                                     b_labels,
                                     1 * KBytes);

  // Emit a load of a large string. In the past, we have attempted to emit
  // the literal load without emitting the veneers, which meant that we were
  // left with an impossible scheduling problem for the pool objects (due to
  // the short range of the ldrd).
  std::string test_string(2 * KBytes, 'x');
  StringLiteral big_literal(test_string.c_str());
  __ Ldrd(r0, r1, &big_literal);

  EmitIndividualNops(&masm, 1000);

  // We can now safely bind the labels.
  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&b_labels[i]);
  }

  END();

  RUN();
}


TEST_A32(literal_and_veneer_interaction_2) {
  SETUP();
  START();

  static const int kLabelsCount = 100;

  Label b_labels[kLabelsCount];

  AddBranchesAndGetCloseToCheckpoint(&masm,
                                     &test,
                                     kLabelsCount,
                                     b_labels,
                                     1 * KBytes);

  // This is similar to the test above. The Ldr instruction can be emitted with
  // no problems. The Ldrd used to force emission of the literal pool, pushing
  // the veneers out of range - we make sure this does not happen anymore.
  std::string test_string(2 * KBytes, 'z');
  StringLiteral big_literal(test_string.c_str());
  __ Ldr(r2, &big_literal);

  const int kVeneerSize = 4;
  CHECK_POOL_SIZE(kLabelsCount * kVeneerSize + big_literal.GetSize());

  std::string test_string2(2 * KBytes, 'x');
  StringLiteral big_literal2(test_string.c_str());
  __ Ldrd(r0, r1, &big_literal2);

  EmitIndividualNops(&masm, 1000);

  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&b_labels[i]);
  }

  END();

  RUN();
}


TEST_A32(literal_and_veneer_interaction_3) {
  SETUP();
  START();

  static const int kLabelsCount = 100;
  Label b_labels[kLabelsCount];

  AddBranchesAndGetCloseToCheckpoint(&masm,
                                     &test,
                                     kLabelsCount,
                                     b_labels,
                                     1 * KBytes);

  // Here, we used to emit the Ldrd instruction and then emit the veneers
  // before the literal is emitted, hence pushing the Ldrd out of range.
  // Make sure this does not happen anymore.
  __ Ldrd(r2, r3, 0x12345678);

  // The issue would only appear when emitting the nops in a single scope.
  EmitNopsInExactAssemblyScope(&masm, 4096);

  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&b_labels[i]);
  }

  END();

  RUN();
}


// Equivalent to literal_and_veneer_interaction_1, but for T32.
TEST_T32(literal_and_veneer_interaction_4) {
  SETUP();
  START();

  static const int kLabelsCount = 550;

  Label b_labels[kLabelsCount];

  AddBranchesAndGetCloseToCheckpoint(&masm,
                                     &test,
                                     kLabelsCount,
                                     b_labels,
                                     KBytes / 2);

  std::string test_string(3 * KBytes, 'x');
  StringLiteral big_literal(test_string.c_str());
  __ Ldrd(r0, r1, &big_literal);

  EmitIndividualNops(&masm, 2000);

  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&b_labels[i]);
  }

  END();

  RUN();
}

// Equivalent to literal_and_veneer_interaction_3, but for T32.
TEST_T32(literal_and_veneer_interaction_5) {
  SETUP();
  START();

  static const int kLabelsCount = 550;
  Label b_labels[kLabelsCount];

  AddBranchesAndGetCloseToCheckpoint(&masm,
                                     &test,
                                     kLabelsCount,
                                     b_labels,
                                     1 * KBytes);

  __ Ldrd(r2, r3, 0x12345678);

  EmitNopsInExactAssemblyScope(&masm, 4096);

  for (int i = 0; i < kLabelsCount; i++) {
    __ Bind(&b_labels[i]);
  }

  END();

  RUN();
}

TEST_T32(assembler_bind_label) {
  SETUP();
  START();

  Label label;
  __ B(eq, &label, kNear);

  // At this point we keep track of the veneer in the pool.
  VIXL_CHECK(!test.PoolIsEmpty());

  {
    // Bind the label with the assembler.
    ExactAssemblyScope scope(&masm, 2, ExactAssemblyScope::kMaximumSize);
    __ bind(&label);
  }

  // Make sure the pool is now empty.
  VIXL_CHECK(test.PoolIsEmpty());

  EmitNopsInExactAssemblyScope(&masm, 4096);

  END();

  RUN();
}

#ifdef VIXL_DEBUG
#define TEST_FORWARD_REFERENCE_INFO(INST, INFO, ASM)    \
  POSITIVE_TEST_FORWARD_REFERENCE_INFO(INST, INFO, ASM) \
  NEGATIVE_TEST_FORWARD_REFERENCE_INFO(INST, ASM)
#else
// Skip the negative tests for release builds, as they require debug-only checks
// in ExactAssemblyScope.
#define TEST_FORWARD_REFERENCE_INFO(INST, INFO, ASM) \
  POSITIVE_TEST_FORWARD_REFERENCE_INFO(INST, INFO, ASM)
#endif

#define POSITIVE_TEST_FORWARD_REFERENCE_INFO(INST, INFO, ASM)                \
  can_encode = masm.INFO;                                                    \
  VIXL_CHECK(can_encode);                                                    \
  {                                                                          \
    ExactAssemblyScope scope(&masm,                                          \
                             info->size,                                     \
                             ExactAssemblyScope::kExactSize);                \
    int32_t pc = masm.GetCursorOffset() + __ GetArchitectureStatePCOffset(); \
    if (info->pc_needs_aligning == ReferenceInfo::kAlignPc) {                \
      pc = AlignDown(pc, 4);                                                 \
    }                                                                        \
    Label label(pc + info->min_offset);                                      \
    masm.ASM;                                                                \
  }                                                                          \
  {                                                                          \
    ExactAssemblyScope scope(&masm,                                          \
                             info->size,                                     \
                             ExactAssemblyScope::kExactSize);                \
    int32_t pc = masm.GetCursorOffset() + __ GetArchitectureStatePCOffset(); \
    if (info->pc_needs_aligning == ReferenceInfo::kAlignPc) {                \
      pc = AlignDown(pc, 4);                                                 \
    }                                                                        \
    Label label(pc + info->max_offset);                                      \
    masm.ASM;                                                                \
  }

#ifdef VIXL_NEGATIVE_TESTING
#define NEGATIVE_TEST_FORWARD_REFERENCE_INFO(INST, ASM)                      \
  try {                                                                      \
    ExactAssemblyScope scope(&masm,                                          \
                             info->size,                                     \
                             ExactAssemblyScope::kMaximumSize);              \
    int32_t pc = masm.GetCursorOffset() + __ GetArchitectureStatePCOffset(); \
    if (info->pc_needs_aligning == ReferenceInfo::kAlignPc) {                \
      pc = AlignDown(pc, 4);                                                 \
    }                                                                        \
    Label label(pc + info->max_offset + info->alignment);                    \
    masm.ASM;                                                                \
    printf("Negative test for forward reference failed for %s.\n", INST);    \
    abort();                                                                 \
  } catch (const std::runtime_error&) {                                      \
  }                                                                          \
  try {                                                                      \
    ExactAssemblyScope scope(&masm,                                          \
                             info->size,                                     \
                             ExactAssemblyScope::kMaximumSize);              \
    int32_t pc = masm.GetCursorOffset() + __ GetArchitectureStatePCOffset(); \
    if (info->pc_needs_aligning == ReferenceInfo::kAlignPc) {                \
      pc = AlignDown(pc, 4);                                                 \
    }                                                                        \
    Label label(pc + info->min_offset - info->alignment);                    \
    masm.ASM;                                                                \
    printf("Negative test for forward reference failed for %s.\n", INST);    \
    abort();                                                                 \
  } catch (const std::runtime_error&) {                                      \
  }
#else
#define NEGATIVE_TEST_FORWARD_REFERENCE_INFO(INST, ASM)
#endif

TEST_T32(forward_reference_info_T32) {
  MacroAssembler masm(BUF_SIZE, T32);

  Label unbound;
  const ReferenceInfo* info;
  bool can_encode;

  // clang-format off

  TEST_FORWARD_REFERENCE_INFO(
    "adr",
    adr_info(al, Narrow, r0, &unbound, &info),
    adr(al, Narrow, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "adr",
    adr_info(al, Wide, r0, &unbound, &info),
    adr(al, Wide, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "adr",
    adr_info(al, Best, r0, &unbound, &info),
    adr(al, Best, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(al, Narrow, &unbound, &info),
    b(al, Narrow, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(al, Wide, &unbound, &info),
    b(al, Wide, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(al, Best, &unbound, &info),
    b(al, Best, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(gt, Narrow, &unbound, &info),
    b(gt, Narrow, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(gt, Wide, &unbound, &info),
    b(gt, Wide, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(gt, Best, &unbound, &info),
    b(gt, Best, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "bl",
    bl_info(al, &unbound, &info),
    bl(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "blx",
    blx_info(al, &unbound, &info),
    blx(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "cbnz",
    cbnz_info(r0, &unbound, &info),
    cbnz(r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "cbz",
    cbz_info(r0, &unbound, &info),
    cbz(r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldr",
    ldr_info(al, Narrow, r0, &unbound, &info),
    ldr(al, Narrow, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldr",
    ldr_info(al, Wide, r0, &unbound, &info),
    ldr(al, Wide, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldr",
    ldr_info(al, Best, r0, &unbound, &info),
    ldr(al, Best, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrb",
    ldrb_info(al, r0, &unbound, &info),
    ldrb(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrd",
    ldrd_info(al, r0, r1, &unbound, &info),
    ldrd(al, r0, r1, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrh",
    ldrh_info(al, r0, &unbound, &info),
    ldrh(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrsb",
    ldrsb_info(al, r0, &unbound, &info),
    ldrsb(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrsh",
    ldrsh_info(al, r0, &unbound, &info),
    ldrsh(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "pld",
    pld_info(al, &unbound, &info),
    pld(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "pli",
    pli_info(al, &unbound, &info),
    pli(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "vldr",
    vldr_info(al, Untyped64, d0, &unbound, &info),
    vldr(al, Untyped64, d0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "vldr",
    vldr_info(al, Untyped32, s0, &unbound, &info),
    vldr(al, Untyped32, s0, &label));

  // clang-format on

  masm.FinalizeCode();
}

TEST_A32(forward_reference_info_A32) {
  MacroAssembler masm(BUF_SIZE, A32);
  Label unbound;
  const ReferenceInfo* info;
  bool can_encode;

  // clang-format off

  TEST_FORWARD_REFERENCE_INFO(
    "adr",
    adr_info(al, Best, r0, &unbound, &info),
    adr(al, Best, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(al, Best, &unbound, &info),
    b(al, Best, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "b",
    b_info(gt, Best, &unbound, &info),
    b(gt, Best, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "bl",
    bl_info(al, &unbound, &info),
    bl(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "blx",
    blx_info(al, &unbound, &info),
    blx(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldr",
    ldr_info(al, Best, r0, &unbound, &info),
    ldr(al, Best, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrb",
    ldrb_info(al, r0, &unbound, &info),
    ldrb(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrd",
    ldrd_info(al, r0, r1, &unbound, &info),
    ldrd(al, r0, r1, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrh",
    ldrh_info(al, r0, &unbound, &info),
    ldrh(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrsb",
    ldrsb_info(al, r0, &unbound, &info),
    ldrsb(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "ldrsh",
    ldrsh_info(al, r0, &unbound, &info),
    ldrsh(al, r0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "pld",
    pld_info(al, &unbound, &info),
    pld(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "pli",
    pli_info(al, &unbound, &info),
    pli(al, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "vldr",
    vldr_info(al, Untyped64, d0, &unbound, &info),
    vldr(al, Untyped64, d0, &label));

  TEST_FORWARD_REFERENCE_INFO(
    "vldr",
    vldr_info(al, Untyped32, s0, &unbound, &info),
    vldr(al, Untyped32, s0, &label));

  // clang-format on

  masm.FinalizeCode();
}

}  // namespace aarch32
}  // namespace vixl
