// Copyright 2016, VIXL authors
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

#include <list>
#include <sstream>
#include <string>

#include "test-runner.h"
#include "test-utils.h"

#include "aarch32/disasm-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#ifdef VIXL_NEGATIVE_TESTING
#include <stdexcept>
#endif

namespace vixl {
namespace aarch32 {

#define __ masm.
#define TEST(name) TEST_(AARCH32_DISASM_##name)

#ifdef VIXL_INCLUDE_TARGET_T32
#define TEST_T32(name) TEST_(AARCH32_DISASM_##name)
#else
#define TEST_T32(name) void Test##name()
#endif

#ifdef VIXL_INCLUDE_TARGET_A32
#define TEST_A32(name) TEST_(AARCH32_DISASM_##name)
#else
#define TEST_A32(name) void Test##name()
#endif

#define BUF_SIZE (4096)

#define SETUP() MacroAssembler masm(BUF_SIZE);

#define CLEANUP()

#ifdef VIXL_NEGATIVE_TESTING
#define START_COMPARE() \
  {                     \
    try {               \
      int32_t start = masm.GetCursorOffset();

#define END_COMPARE_CHECK_SIZE(EXP, SIZE)                       \
  int32_t end = masm.GetCursorOffset();                         \
  masm.FinalizeCode();                                          \
  std::ostringstream ss;                                        \
  TestDisassembler disassembler(ss, 0);                         \
  if (masm.IsUsingT32()) {                                      \
    disassembler.DisassembleT32(*masm.GetBuffer(), start, end); \
  } else {                                                      \
    disassembler.DisassembleA32(*masm.GetBuffer(), start, end); \
  }                                                             \
  masm.GetBuffer()->Reset();                                    \
  if (Test::disassemble()) {                                    \
    printf("----\n");                                           \
    printf("%s", ss.str().c_str());                             \
  }                                                             \
  if (std::string(EXP) != ss.str()) {                           \
    printf("\n%s:%d:%s\nFound:\n%sExpected:\n%s",               \
           __FILE__,                                            \
           __LINE__,                                            \
           masm.IsUsingT32() ? "T32" : "A32",                   \
           ss.str().c_str(),                                    \
           EXP);                                                \
    abort();                                                    \
  }                                                             \
  if ((SIZE) != -1 && ((end - start) != (SIZE))) {              \
    printf("\nExpected %d bits, found %d bits\n",               \
           8 * (SIZE),                                          \
           8 * (end - start));                                  \
    abort();                                                    \
  }                                                             \
  }                                                             \
  catch (const std::runtime_error& e) {                         \
    const char* msg = e.what();                                 \
    printf("\n%s:%d:%s\nFound:\n%sExpected:\n%s",               \
           __FILE__,                                            \
           __LINE__,                                            \
           masm.IsUsingT32() ? "T32" : "A32",                   \
           msg,                                                 \
           EXP);                                                \
    abort();                                                    \
  }                                                             \
  }
#else
#define START_COMPARE() \
  {                     \
    int32_t start = masm.GetCursorOffset();

#define END_COMPARE_CHECK_SIZE(EXP, SIZE)                       \
  int32_t end = masm.GetCursorOffset();                         \
  masm.FinalizeCode();                                          \
  std::ostringstream ss;                                        \
  TestDisassembler disassembler(ss, 0);                         \
  if (masm.IsUsingT32()) {                                      \
    disassembler.DisassembleT32(*masm.GetBuffer(), start, end); \
  } else {                                                      \
    disassembler.DisassembleA32(*masm.GetBuffer(), start, end); \
  }                                                             \
  masm.GetBuffer()->Reset();                                    \
  if (Test::disassemble()) {                                    \
    printf("----\n");                                           \
    printf("%s", ss.str().c_str());                             \
  }                                                             \
  if (std::string(EXP) != ss.str()) {                           \
    printf("\n%s:%d:%s\nFound:\n%sExpected:\n%s",               \
           __FILE__,                                            \
           __LINE__,                                            \
           masm.IsUsingT32() ? "T32" : "A32",                   \
           ss.str().c_str(),                                    \
           EXP);                                                \
    abort();                                                    \
  }                                                             \
  if ((SIZE) != -1 && ((end - start) != (SIZE))) {              \
    printf("\nExpected %d bits, found %d bits\n",               \
           8 * (SIZE),                                          \
           8 * (end - start));                                  \
    abort();                                                    \
  }                                                             \
  }
#endif

#define END_COMPARE(EXP) END_COMPARE_CHECK_SIZE(EXP, -1)

#ifdef VIXL_INCLUDE_TARGET_A32
#define COMPARE_A32(ASM, EXP) \
  masm.UseA32();              \
  START_COMPARE()             \
  masm.ASM;                   \
  END_COMPARE(EXP)
#else
#define COMPARE_A32(ASM, EXP)
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
#define COMPARE_T32(ASM, EXP) \
  masm.UseT32();              \
  START_COMPARE()             \
  masm.ASM;                   \
  END_COMPARE(EXP)
#else
#define COMPARE_T32(ASM, EXP)
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
#define COMPARE_T32_CHECK_SIZE(ASM, EXP, SIZE) \
  masm.UseT32();                               \
  START_COMPARE()                              \
  masm.ASM;                                    \
  END_COMPARE_CHECK_SIZE(EXP, SIZE)
#else
#define COMPARE_T32_CHECK_SIZE(ASM, EXP, SIZE)
#endif

#define COMPARE_BOTH(ASM, EXP) \
  COMPARE_A32(ASM, EXP)        \
  COMPARE_T32(ASM, EXP)

#ifdef VIXL_NEGATIVE_TESTING
#define NEGATIVE_TEST(ASM, EXP, TEMPORARILY_ACCEPTED)                          \
  {                                                                            \
    try {                                                                      \
      int32_t start = masm.GetCursorOffset();                                  \
      ASM int32_t end = masm.GetCursorOffset();                                \
      masm.FinalizeCode();                                                     \
      if (!TEMPORARILY_ACCEPTED) {                                             \
        std::ostringstream ss;                                                 \
        PrintDisassembler disassembler(ss, 0);                                 \
        if (masm.IsUsingT32()) {                                               \
          disassembler.DisassembleT32Buffer(masm.GetBuffer()                   \
                                                ->GetOffsetAddress<uint16_t*>( \
                                                    start),                    \
                                            end);                              \
        } else {                                                               \
          disassembler.DisassembleA32Buffer(masm.GetBuffer()                   \
                                                ->GetOffsetAddress<uint32_t*>( \
                                                    start),                    \
                                            end);                              \
        }                                                                      \
        printf("\n%s:%d:%s\nNo exception raised.\n",                           \
               __FILE__,                                                       \
               __LINE__,                                                       \
               masm.IsUsingT32() ? "T32" : "A32");                             \
        printf("Found:\n%sExpected:\n%s", ss.str().c_str(), EXP);              \
        abort();                                                               \
      }                                                                        \
    } catch (const std::runtime_error& e) {                                    \
      const char* msg = e.what();                                              \
      size_t exp_len = strlen(EXP);                                            \
      if (TEMPORARILY_ACCEPTED) {                                              \
        printf(                                                                \
            "\nNegative MacroAssembler test that was temporarily "             \
            "assembling a deprecated or unpredictable instruction is now "     \
            "correctly raising an exception. Please update the "               \
            "test to reflect this.\n");                                        \
        printf("at: %s:%d:%s\n",                                               \
               __FILE__,                                                       \
               __LINE__,                                                       \
               masm.IsUsingT32() ? "T32" : "A32");                             \
        abort();                                                               \
      } else if (std::strncmp(EXP, msg, exp_len) != 0) {                       \
        printf("\n%s:%d:%s\nFound:\n%sExpected:\n%s...",                       \
               __FILE__,                                                       \
               __LINE__,                                                       \
               masm.IsUsingT32() ? "T32" : "A32",                              \
               msg,                                                            \
               EXP);                                                           \
        abort();                                                               \
      }                                                                        \
    }                                                                          \
  }

#ifdef VIXL_INCLUDE_TARGET_A32
#define MUST_FAIL_TEST_A32(ASM, EXP)       \
  masm.UseA32();                           \
  NEGATIVE_TEST({ masm.ASM; }, EXP, false) \
  masm.GetBuffer()->Reset();
#else
#define MUST_FAIL_TEST_A32(ASM, EXP)
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
#define MUST_FAIL_TEST_T32(ASM, EXP)       \
  masm.UseT32();                           \
  NEGATIVE_TEST({ masm.ASM; }, EXP, false) \
  masm.GetBuffer()->Reset();
#else
#define MUST_FAIL_TEST_T32(ASM, EXP)
#endif

#define MUST_FAIL_TEST_BOTH(ASM, EXP) \
  MUST_FAIL_TEST_A32(ASM, EXP)        \
  MUST_FAIL_TEST_T32(ASM, EXP)

#ifdef VIXL_INCLUDE_TARGET_A32
#define MUST_FAIL_TEST_A32_BLOCK(ASM, EXP) \
  masm.UseA32();                           \
  NEGATIVE_TEST(ASM, EXP, false)           \
  masm.GetBuffer()->Reset();
#else
#define MUST_FAIL_TEST_A32_BLOCK(ASM, EXP)
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
#define MUST_FAIL_TEST_T32_BLOCK(ASM, EXP) \
  masm.UseT32();                           \
  NEGATIVE_TEST(ASM, EXP, false)           \
  masm.GetBuffer()->Reset();
#else
#define MUST_FAIL_TEST_T32_BLOCK(ASM, EXP)
#endif

#define MUST_FAIL_TEST_BOTH_BLOCK(ASM, EXP) \
  MUST_FAIL_TEST_A32_BLOCK(ASM, EXP)        \
  MUST_FAIL_TEST_T32_BLOCK(ASM, EXP)
#else
// Skip negative tests.
#define MUST_FAIL_TEST_A32(ASM, EXP)                         \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define MUST_FAIL_TEST_T32(ASM, EXP)                         \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define MUST_FAIL_TEST_BOTH(ASM, EXP)                        \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define MUST_FAIL_TEST_A32_BLOCK(ASM, EXP)                   \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define MUST_FAIL_TEST_T32_BLOCK(ASM, EXP)                   \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define MUST_FAIL_TEST_BOTH_BLOCK(ASM, EXP)                  \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#endif

#ifdef VIXL_NEGATIVE_TESTING
#ifdef VIXL_INCLUDE_TARGET_A32
#define SHOULD_FAIL_TEST_A32(ASM)        \
  masm.UseA32();                         \
  NEGATIVE_TEST({ masm.ASM; }, "", true) \
  masm.GetBuffer()->Reset();
#else
#define SHOULD_FAIL_TEST_A32(ASM)
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
#define SHOULD_FAIL_TEST_T32(ASM)        \
  masm.UseT32();                         \
  NEGATIVE_TEST({ masm.ASM; }, "", true) \
  masm.GetBuffer()->Reset();
#else
#define SHOULD_FAIL_TEST_T32(ASM)
#endif

#define SHOULD_FAIL_TEST_BOTH(ASM) \
  SHOULD_FAIL_TEST_A32(ASM)        \
  SHOULD_FAIL_TEST_T32(ASM)
#else
#define SHOULD_FAIL_TEST_A32(ASM)                            \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define SHOULD_FAIL_TEST_T32(ASM)                            \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#define SHOULD_FAIL_TEST_BOTH(ASM)                           \
  printf(                                                    \
      "Skipping negative tests. To enable them, build with " \
      "'negative_testing=on'.\n");
#endif

class TestDisassembler : public PrintDisassembler {
 public:
  TestDisassembler(std::ostream& os, uint32_t pc)  // NOLINT(runtime/references)
      : PrintDisassembler(os, pc) {}

  virtual void PrintCodeAddress(uint32_t code_address) VIXL_OVERRIDE {
    USE(code_address);
  }

  virtual void PrintOpcode16(uint32_t opcode) VIXL_OVERRIDE { USE(opcode); }

  virtual void PrintOpcode32(uint32_t opcode) VIXL_OVERRIDE { USE(opcode); }

  void DisassembleA32(const CodeBuffer& buffer,
                      ptrdiff_t start,
                      ptrdiff_t end) {
    DisassembleA32Buffer(buffer.GetOffsetAddress<const uint32_t*>(start),
                         end - start);
  }

  void DisassembleT32(const CodeBuffer& buffer,
                      ptrdiff_t start,
                      ptrdiff_t end) {
    DisassembleT32Buffer(buffer.GetOffsetAddress<const uint16_t*>(start),
                         end - start);
  }
};


TEST_T32(t32_disassembler_limit1) {
  SETUP();

  masm.UseT32();
  START_COMPARE()
  masm.Add(r9, r10, r11);
  masm.GetBuffer()->Emit16(kLowestT32_32Opcode >> 16);
  END_COMPARE(
      "add r9, r10, r11\n"
      "?\n");

  CLEANUP();
}


TEST_T32(t32_disassembler_limit2) {
  SETUP();

  masm.UseT32();
  START_COMPARE()
  masm.Add(r9, r10, r11);
  masm.Add(r0, r0, r1);
  END_COMPARE(
      "add r9, r10, r11\n"
      "add r0, r1\n");

  CLEANUP();
}


TEST(macro_assembler_orn) {
  SETUP();

  // - Identities.

  COMPARE_BOTH(Orn(r0, r1, 0), "mvn r0, #0\n");
  COMPARE_BOTH(Orn(r0, r0, 0xffffffff), "");

  // - Immediate form. This form does not need macro-assembler support
  //   for T32.

  // Use r0 as the temporary register.
  COMPARE_A32(Orn(r0, r1, 1),
              "mvn r0, #1\n"
              "orr r0, r1, r0\n");
  // Use ip as the temporary register.
  COMPARE_A32(Orns(r0, r0, 1),
              "mvn ip, #1\n"
              "orrs r0, ip\n");

  //  - Too large immediate form.
  COMPARE_BOTH(Orn(r0, r1, 0x00ffffff), "orr r0, r1, #0xff000000\n");
  COMPARE_BOTH(Orn(r0, r1, 0xff00ffff), "orr r0, r1, #0xff0000\n");
  COMPARE_BOTH(Orns(r0, r1, 0x00ffffff), "orrs r0, r1, #0xff000000\n");

  COMPARE_A32(Orns(r0, r1, 0xabcd2345),
              "mov ip, #9029\n"
              "movt ip, #43981\n"
              "mvn r0, ip\n"
              "orrs r0, r1, r0\n");
  COMPARE_T32(Orn(r0, r1, 0xabcd2345),
              "mov r0, #9029\n"
              "movt r0, #43981\n"
              "orn r0, r1, r0\n");

  // - Plain register form. This form does not need macro-assembler
  //   support for T32.

  // Use r0 as the temporary register.
  COMPARE_A32(Orn(r0, r1, r2),
              "mvn r0, r2\n"
              "orr r0, r1, r0\n");
  // Use ip as the temporary register.
  COMPARE_A32(Orn(r0, r0, r1),
              "mvn ip, r1\n"
              "orr r0, ip\n");
  // Use r0 as the temporary register.
  COMPARE_A32(Orn(r0, r1, r0),
              "mvn r0, r0\n"
              "orr r0, r1, r0\n");
  // Use ip as the temporary register.
  COMPARE_A32(Orn(r0, r0, r0),
              "mvn ip, r0\n"
              "orr r0, ip\n");

  // - Shifted register form. This form does not need macro-assembler
  //   support for T32.

  // Use r0 as the temporary register.
  COMPARE_A32(Orn(r0, r1, Operand(r2, LSL, 1)),
              "mvn r0, r2, lsl #1\n"
              "orr r0, r1, r0\n");
  // Use ip as the temporary register.
  COMPARE_A32(Orns(r0, r0, Operand(r2, LSR, 2)),
              "mvn ip, r2, lsr #2\n"
              "orrs r0, ip\n");

  // - Register shifted register form.

  // Use r0 as the temporary register.
  COMPARE_A32(Orn(r0, r1, Operand(r2, LSL, r3)),
              "mvn r0, r2, lsl r3\n"
              "orr r0, r1, r0\n");
  COMPARE_T32(Orn(r0, r1, Operand(r2, LSL, r3)),
              "lsl r0, r2, r3\n"
              "orn r0, r1, r0\n");
  // Use ip as the temporary register.
  COMPARE_A32(Orns(r0, r0, Operand(r2, LSR, r3)),
              "mvn ip, r2, lsr r3\n"
              "orrs r0, ip\n");
  COMPARE_T32(Orns(r0, r0, Operand(r2, LSR, r3)),
              "lsr ip, r2, r3\n"
              "orns r0, ip\n");
  // Use ip as the temporary register.
  COMPARE_A32(Orn(r0, r0, Operand(r0, ASR, r3)),
              "mvn ip, r0, asr r3\n"
              "orr r0, ip\n");
  COMPARE_T32(Orn(r0, r0, Operand(r0, ASR, r3)),
              "asr ip, r0, r3\n"
              "orn r0, ip\n");
  CLEANUP();
}


TEST(macro_assembler_t32_rsc) {
  SETUP();

  // - Immediate form. We can always re-use `rn`.

  // No need for temporay registers.
  COMPARE_T32(Rsc(r0, r1, 1),
              "mvn r0, r1\n"
              "adc r0, #1\n");
  // No need for temporay registers.
  COMPARE_T32(Rscs(r0, r0, 2),
              "mvn r0, r0\n"
              "adcs r0, #2\n");

  //  - Too large immediate form.

  // TODO: optimize this.
  COMPARE_A32(Rsc(r0, r1, 0x00ffffff),
              "mvn r0, #4278190080\n"
              "rsc r0, r1, r0\n");
  COMPARE_T32(Rscs(r0, r1, 0x00ffffff),
              "mvn r0, r1\n"
              "mvn ip, #4278190080\n"
              "adcs r0, ip\n");
  COMPARE_A32(Rsc(r0, r0, 0x00ffffff),
              "mvn ip, #4278190080\n"
              "rsc r0, ip\n");
  COMPARE_T32(Rscs(r0, r0, 0x00ffffff),
              "mvn r0, r0\n"
              "mvn ip, #4278190080\n"
              "adcs r0, ip\n");

  COMPARE_A32(Rsc(r0, r1, 0xabcd2345),
              "mov r0, #9029\n"
              "movt r0, #43981\n"
              "rsc r0, r1, r0\n");
  COMPARE_T32(Rscs(r0, r1, 0xabcd2345),
              "mvn r0, r1\n"
              "mov ip, #56506\n"
              "movt ip, #21554\n"
              "sbcs r0, ip\n");
  COMPARE_A32(Rsc(r0, r0, 0xabcd2345),
              "mov ip, #9029\n"
              "movt ip, #43981\n"
              "rsc r0, ip\n");
  COMPARE_T32(Rscs(r0, r0, 0xabcd2345),
              "mvn r0, r0\n"
              "mov ip, #56506\n"
              "movt ip, #21554\n"
              "sbcs r0, ip\n");

  // - Plain register form.

  // No need for temporary registers.
  COMPARE_T32(Rscs(r0, r1, r2),
              "mvn r0, r1\n"
              "adcs r0, r2\n");
  // Use r0 as the temporary register.
  COMPARE_T32(Rscs(r0, r1, r1),
              "mvn r0, r1\n"
              "adcs r0, r1\n");
  // Use ip as the temporary register.
  COMPARE_T32(Rscs(r0, r0, r0),
              "mvn ip, r0\n"
              "adcs r0, ip, r0\n");

  // - Shifted register form.

  // No need for temporay registers.
  COMPARE_T32(Rsc(r0, r1, Operand(r2, LSL, 1)),
              "mvn r0, r1\n"
              "adc r0, r2, lsl #1\n");
  // Use ip as the temporary register.
  COMPARE_T32(Rscs(r0, r1, Operand(r0, LSR, 2)),
              "mvn ip, r1\n"
              "adcs r0, ip, r0, lsr #2\n");
  // Use r0 as the temporary register.
  COMPARE_T32(Rsc(r0, r1, Operand(r1, ASR, 3)),
              "mvn r0, r1\n"
              "adc r0, r1, asr #3\n");
  // Use ip as the temporary register.
  COMPARE_T32(Rscs(r0, r0, Operand(r0, ROR, 4)),
              "mvn ip, r0\n"
              "adcs r0, ip, r0, ror #4\n");

  // - Register shifted register form. The macro-assembler handles this form in
  //   two steps. First, a shift instruction is generated from the operand. And
  //   finally the operation is reduced to its plain register form.

  COMPARE_T32(Rsc(r0, r1, Operand(r2, LSL, r3)),
              "lsl r0, r2, r3\n"
              "mvn ip, r1\n"
              "adc r0, ip, r0\n");
  // Use r0 and ip as the temporary register.
  COMPARE_T32(Rscs(r0, r1, Operand(r1, LSR, r3)),
              "lsr r0, r1, r3\n"
              "mvn ip, r1\n"
              "adcs r0, ip, r0\n");
  // Use ip and r0 as the temporary register.
  COMPARE_T32(Rsc(r0, r0, Operand(r2, ASR, r3)),
              "asr ip, r2, r3\n"
              "mvn r0, r0\n"
              "adc r0, ip\n");
  // Use ip and r0 as the temporary register.
  COMPARE_T32(Rscs(r0, r0, Operand(r0, ROR, r3)),
              "ror ip, r0, r3\n"
              "mvn r0, r0\n"
              "adcs r0, ip\n");
  // Use ip and r0 as the temporary register.
  COMPARE_T32(Rsc(r0, r0, Operand(r0, LSL, r0)),
              "lsl ip, r0, r0\n"
              "mvn r0, r0\n"
              "adc r0, ip\n");

  CLEANUP();
}


TEST(macro_assembler_t32_register_shift_register) {
  SETUP();

  COMPARE_T32(Adc(r0, r1, Operand(r2, LSL, r3)),
              "lsl r0, r2, r3\n"
              "adc r0, r1, r0\n");
  COMPARE_T32(Adcs(r0, r0, Operand(r2, LSR, r3)),
              "lsr ip, r2, r3\n"
              "adcs r0, ip\n");
  COMPARE_T32(Add(r0, r0, Operand(r0, ASR, r3)),
              "asr ip, r0, r3\n"
              "add r0, ip\n");
  COMPARE_T32(Adds(r0, r0, Operand(r0, ROR, r0)),
              "ror ip, r0, r0\n"
              "adds r0, ip\n");

  CLEANUP();
}


TEST(macro_assembler_big_offset) {
  SETUP();

  COMPARE_BOTH(Ldr(r0, MemOperand(r1, 0xfff123)),
               "add r0, r1, #1044480\n"  // #0xff000
               "add r0, #15728640\n"     // #0x00f00000
               "ldr r0, [r0, #291]\n");  // #0x123
  COMPARE_BOTH(Ldr(r0, MemOperand(r1, 0xff123)),
               "add r0, r1, #1044480\n"  // #0xff000
               "ldr r0, [r0, #291]\n");  // #0x123
  COMPARE_BOTH(Ldr(r0, MemOperand(r1, -0xff123)),
               "sub r0, r1, #1048576\n"   // #0x100000
               "ldr r0, [r0, #3805]\n");  // #0xedd

  COMPARE_A32(Ldr(r0, MemOperand(r1, 0xfff123, PreIndex)),
              "add r1, #1044480\n"       // #0xff000
              "add r1, #15728640\n"      // #0x00f00000
              "ldr r0, [r1, #291]!\n");  // #0x123
  COMPARE_A32(Ldr(r0, MemOperand(r1, 0xff123, PreIndex)),
              "add r1, #1044480\n"       // #0xff000
              "ldr r0, [r1, #291]!\n");  // #0x123
  COMPARE_A32(Ldr(r0, MemOperand(r1, -0xff123, PreIndex)),
              "sub r1, #1048576\n"        // #0x100000
              "ldr r0, [r1, #3805]!\n");  // #0xedd

  COMPARE_T32(Ldr(r0, MemOperand(r1, 0xfff12, PreIndex)),
              "add r1, #65280\n"        // #0xff00
              "add r1, #983040\n"       // #0x000f0000
              "ldr r0, [r1, #18]!\n");  // #0x12
  COMPARE_T32(Ldr(r0, MemOperand(r1, 0xff12, PreIndex)),
              "add r1, #65280\n"        // #0xff00
              "ldr r0, [r1, #18]!\n");  // #0x12
  COMPARE_T32(Ldr(r0, MemOperand(r1, -0xff12, PreIndex)),
              "sub r1, #65536\n"         // #0x10000
              "ldr r0, [r1, #238]!\n");  // #0xee

  COMPARE_A32(Ldr(r0, MemOperand(r1, 0xfff123, PostIndex)),
              "ldr r0, [r1], #291\n"   // #0x123
              "add r1, #1044480\n"     // #0xff000
              "add r1, #15728640\n");  // #0x00f00000
  COMPARE_A32(Ldr(r0, MemOperand(r1, 0xff123, PostIndex)),
              "ldr r0, [r1], #291\n"  // #0x123
              "add r1, #1044480\n");  // #0xff000
  COMPARE_A32(Ldr(r0, MemOperand(r1, -0xff123, PostIndex)),
              "ldr r0, [r1], #3805\n"  // #0xedd
              "sub r1, #1048576\n");   // #0x100000

  COMPARE_T32(Ldr(r0, MemOperand(r1, 0xfff12, PostIndex)),
              "ldr r0, [r1], #18\n"  // #0x12
              "add r1, #65280\n"     // #0xff00
              "add r1, #983040\n");  // #0x000f0000
  COMPARE_T32(Ldr(r0, MemOperand(r1, 0xff12, PostIndex)),
              "ldr r0, [r1], #18\n"  // #0x12
              "add r1, #65280\n");   // #0xff00
  COMPARE_T32(Ldr(r0, MemOperand(r1, -0xff12, PostIndex)),
              "ldr r0, [r1], #238\n"  // #0xee
              "sub r1, #65536\n");    // #0x10000

  COMPARE_A32(Ldrh(r0, MemOperand(r1, 0xfff123)),
              "add r0, r1, #61696\n"    // #0xf100
              "add r0, #16711680\n"     // #0x00ff0000
              "ldrh r0, [r0, #35]\n");  // #0x23
  COMPARE_T32(Ldrh(r0, MemOperand(r1, 0xfff123)),
              "add r0, r1, #1044480\n"   // #0xff000
              "add r0, #15728640\n"      // #0x00f00000
              "ldrh r0, [r0, #291]\n");  // #0x123

  COMPARE_A32(Ldrh(r0, MemOperand(r1, 0xff123)),
              "add r0, r1, #61696\n"    // #0xf100
              "add r0, #983040\n"       // #0x000f0000
              "ldrh r0, [r0, #35]\n");  // #0x23
  COMPARE_T32(Ldrh(r0, MemOperand(r1, 0xff123)),
              "add r0, r1, #1044480\n"   // #0xff000
              "ldrh r0, [r0, #291]\n");  // #0x123
  COMPARE_A32(Ldrh(r0, MemOperand(r1, -0xff123)),
              "sub r0, r1, #61952\n"     // #0xf200
              "sub r0, #983040\n"        // #0x000f0000
              "ldrh r0, [r0, #221]\n");  // #0xdd
  COMPARE_T32(Ldrh(r0, MemOperand(r1, -0xff123)),
              "sub r0, r1, #1048576\n"    // #0x100000
              "ldrh r0, [r0, #3805]\n");  // #0xedd

  MUST_FAIL_TEST_BOTH(Ldr(r0, MemOperand(r0, 0xfff12, PreIndex)),
                      "Ill-formed 'ldr' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldr(r0, MemOperand(r0, 0xfff12, PostIndex)),
                      "Ill-formed 'ldr' instruction.\n");
  CLEANUP();
}


TEST(macro_assembler_load) {
  SETUP();

  // Register base and offset that we can encode in both A1 and T1.
  COMPARE_BOTH(Ldr(r0, MemOperand(r1, r8, Offset)), "ldr r0, [r1, r8]\n");

  // Negative register offset. Use the destination as a scratch register,
  // regardless of the values of the base and offset register.
  COMPARE_T32(Ldr(r0, MemOperand(r0, minus, r0, Offset)),
              "sub r0, r0\n"
              "ldr r0, [r0]\n");

  COMPARE_T32(Ldr(r0, MemOperand(r0, minus, r1, Offset)),
              "sub r0, r1\n"
              "ldr r0, [r0]\n");

  COMPARE_T32(Ldr(r0, MemOperand(r1, minus, r0, Offset)),
              "sub r0, r1, r0\n"
              "ldr r0, [r0]\n");

  COMPARE_T32(Ldr(r0, MemOperand(r1, minus, r2, Offset)),
              "sub r0, r1, r2\n"
              "ldr r0, [r0]\n");

  // Pre-index negative offset.
  COMPARE_T32(Ldr(r0, MemOperand(r1, minus, r2, PreIndex)),
              "sub r1, r2\n"
              "ldr r0, [r1]\n");

  // Post-index negative offset.
  COMPARE_T32(Ldr(r0, MemOperand(r1, minus, r2, PostIndex)),
              "ldr r0, [r1]\n"
              "sub r1, r2\n");

  // SP is allowed as base, offset and destination.
  COMPARE_BOTH(Ldr(sp, MemOperand(sp, sp, Offset)), "ldr sp, [sp, sp]\n");

  // PC is allowed as destination - make sure it is not used as a temporary
  // register.
  COMPARE_BOTH(Ldr(pc, MemOperand(r0, r0, Offset)), "ldr pc, [r0, r0]\n");
  COMPARE_A32(Ldr(pc, MemOperand(r0, r0, PreIndex)), "ldr pc, [r0, r0]!\n");
  COMPARE_T32(Ldr(pc, MemOperand(r0, r0, PreIndex)),
              "add r0, r0\n"
              "ldr pc, [r0]\n");
  COMPARE_A32(Ldr(pc, MemOperand(r0, r0, PostIndex)), "ldr pc, [r0], r0\n");
  COMPARE_T32(Ldr(pc, MemOperand(r0, r0, PostIndex)),
              "ldr pc, [r0]\n"
              "add r0, r0\n");

  // PC is allowed as register base in the offset variant only for A32.
  COMPARE_A32(Ldr(r0, MemOperand(pc, r0, Offset)), "ldr r0, [pc, r0]\n");
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(pc, r0, Offset)),
                     "The MacroAssembler does not convert loads and stores with"
                     " a PC base register for T32.\n");

  // PC is not allowed as register base in the pre-index and post-index
  // variants.
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(pc, r0, PreIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC base register in pre-index or post-index "
                     "mode.\n");
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(pc, r0, PostIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC base register in pre-index or post-index "
                     "mode.\n");

  // We don't convert loads with PC as the register offset.
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(r0, minus, pc, Offset)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC offset register.\n");
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(r0, pc, PreIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC offset register.\n");
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(r0, pc, PostIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC offset register.\n");

  MUST_FAIL_TEST_BOTH(Ldr(r0, MemOperand(r0, Sign(plus), pc, Offset)),
                      "Unpredictable instruction.\n");

  // TODO: PC should not be allowed as register base in A32 with pre-index
  //       and post-index (unpredictable).
  SHOULD_FAIL_TEST_A32(Ldr(r0, MemOperand(pc, r0, PreIndex)));
  SHOULD_FAIL_TEST_A32(Ldr(r0, MemOperand(pc, r0, PostIndex)));

  // TODO: load with the same register used as base and as destination
  //       should fail to assemble (unpredictable).
  SHOULD_FAIL_TEST_A32(Ldr(r0, MemOperand(r0, r1, PreIndex)));
  SHOULD_FAIL_TEST_A32(Ldr(r0, MemOperand(r0, r1, PostIndex)));
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(r0, r1, PreIndex)),
                     "Ill-formed 'ldr' instruction.\n");
  MUST_FAIL_TEST_T32(Ldr(r0, MemOperand(r0, r1, PostIndex)),
                     "Ill-formed 'ldr' instruction.\n");

  CLEANUP();
}


TEST(macro_assembler_store) {
  SETUP();

  // Register base and offset that we can encode in both A1 and T1.
  COMPARE_BOTH(Str(r0, MemOperand(r1, r8, Offset)), "str r0, [r1, r8]\n");

  // Negative register offset.
  COMPARE_T32(Str(r0, MemOperand(r0, minus, r0, Offset)),
              "sub ip, r0, r0\n"
              "str r0, [ip]\n");

  COMPARE_T32(Str(r0, MemOperand(r0, minus, r1, Offset)),
              "sub ip, r0, r1\n"
              "str r0, [ip]\n");

  COMPARE_T32(Str(r0, MemOperand(r1, minus, r0, Offset)),
              "sub ip, r1, r0\n"
              "str r0, [ip]\n");

  COMPARE_T32(Str(r0, MemOperand(r1, minus, r2, Offset)),
              "sub ip, r1, r2\n"
              "str r0, [ip]\n");

  // Pre-index negative offset.
  COMPARE_T32(Str(r0, MemOperand(r1, minus, r2, PreIndex)),
              "sub r1, r2\n"
              "str r0, [r1]\n");

  // Post-index negative offset.
  COMPARE_T32(Str(r0, MemOperand(r1, minus, r2, PostIndex)),
              "str r0, [r1]\n"
              "sub r1, r2\n");

  // SP is allowed as base, offset and source.
  COMPARE_BOTH(Str(sp, MemOperand(sp, sp, Offset)), "str sp, [sp, sp]\n");

  COMPARE_A32(Str(pc, MemOperand(r0, r0, Offset)), "str pc, [r0, r0]\n");
  COMPARE_A32(Str(pc, MemOperand(r0, r0, PreIndex)), "str pc, [r0, r0]!\n");
  COMPARE_A32(Str(pc, MemOperand(r0, r0, PostIndex)), "str pc, [r0], r0\n");
  MUST_FAIL_TEST_T32(Str(pc, MemOperand(r0, r0, Offset)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Str(pc, MemOperand(r0, r0, PreIndex)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Str(pc, MemOperand(r0, r0, PostIndex)),
                     "Unpredictable instruction.\n");

  // PC is allowed as register base in the offset variant only for A32.
  COMPARE_A32(Str(r0, MemOperand(pc, r0, Offset)), "str r0, [pc, r0]\n");
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(pc, r0, Offset)),
                     "The MacroAssembler does not convert loads and stores with"
                     " a PC base register for T32.\n");

  // PC is not allowed as register base in the pre-index and post-index
  // variants.
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(pc, r0, PreIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC base register in pre-index or post-index "
                     "mode.\n");
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(pc, r0, PostIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC base register in pre-index or post-index "
                     "mode.\n");

  // We don't convert loads with PC as the register offset.
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(r0, minus, pc, Offset)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC offset register.\n");
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(r0, pc, PreIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC offset register.\n");
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(r0, pc, PostIndex)),
                     "The MacroAssembler does not convert loads and stores "
                     "with a PC offset register.\n");

  MUST_FAIL_TEST_BOTH(Str(r0, MemOperand(r0, Sign(plus), pc, Offset)),
                      "Unpredictable instruction.\n");

  // TODO: PC should not be allowed as register base in A32 with pre-index
  //       and post-index (unpredictable).
  SHOULD_FAIL_TEST_A32(Str(r0, MemOperand(pc, r0, PreIndex)));
  SHOULD_FAIL_TEST_A32(Str(r0, MemOperand(pc, r0, PostIndex)));

  // TODO: store with the same register used as base and as source
  //       should fail to assemble (unpredictable).
  SHOULD_FAIL_TEST_A32(Str(r0, MemOperand(r0, r1, PreIndex)));
  SHOULD_FAIL_TEST_A32(Str(r0, MemOperand(r0, r1, PostIndex)));
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(r0, r1, PreIndex)),
                     "Ill-formed 'str' instruction.\n");
  MUST_FAIL_TEST_T32(Str(r0, MemOperand(r0, r1, PostIndex)),
                     "Ill-formed 'str' instruction.\n");

  CLEANUP();
}


TEST(macro_assembler_ldrd) {
  SETUP();

  // - Tests with no offset.

  COMPARE_BOTH(Ldrd(r0, r1, MemOperand(r3)), "ldrd r0, r1, [r3]\n");
  // Destination registers need to start with a even numbered register on A32.
  MUST_FAIL_TEST_A32(Ldrd(r1, r2, MemOperand(r3)),
                     "Unpredictable instruction.\n");
  COMPARE_T32(Ldrd(r1, r2, MemOperand(r3)), "ldrd r1, r2, [r3]\n");
  // Registers need to be adjacent on A32.
  MUST_FAIL_TEST_A32(Ldrd(r0, r2, MemOperand(r1)),
                     "Ill-formed 'ldrd' instruction.\n");
  COMPARE_T32(Ldrd(r0, r2, MemOperand(r1)), "ldrd r0, r2, [r1]\n");

  COMPARE_BOTH(Ldrd(r0, r1, MemOperand(r2)), "ldrd r0, r1, [r2]\n");

  // - Tests with immediate offsets.

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 1020)),
              "add r0, r2, #1020\n"
              "ldrd r0, r1, [r0]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 1020)), "ldrd r0, r1, [r2, #1020]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -1020)),
              "sub r0, r2, #1020\n"
              "ldrd r0, r1, [r0]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -1020)),
              "ldrd r0, r1, [r2, #-1020]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 0xabcc)),
              "add r0, r2, #43776\n"
              "ldrd r0, r1, [r0, #204]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 0xabcc)),
              "add r0, r2, #43008\n"
              "ldrd r0, r1, [r0, #972]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -0xabcc)),
              "sub r0, r2, #44032\n"
              "ldrd r0, r1, [r0, #52]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -0xabcc)),
              "sub r0, r2, #44032\n"
              "ldrd r0, r1, [r0, #52]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 0xabcdec)),
              "add r0, r2, #52480\n"
              "add r0, #11206656\n"
              "ldrd r0, r1, [r0, #236]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 0xabcdec)),
              "add r0, r2, #248832\n"
              "add r0, #11010048\n"
              "ldrd r0, r1, [r0, #492]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -0xabcdec)),
              "sub r0, r2, #52736\n"
              "sub r0, #11206656\n"
              "ldrd r0, r1, [r0, #20]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -0xabcdec)),
              "sub r0, r2, #774144\n"
              "sub r0, #10485760\n"
              "ldrd r0, r1, [r0, #532]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r0, 0xabcc)),
              "add r1, r0, #43776\n"
              "ldrd r0, r1, [r1, #204]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r0, 0xabcc)),
              "add r1, r0, #43008\n"
              "ldrd r0, r1, [r1, #972]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r0, -0xabcc)),
              "sub r1, r0, #44032\n"
              "ldrd r0, r1, [r1, #52]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r0, -0xabcc)),
              "sub r1, r0, #44032\n"
              "ldrd r0, r1, [r1, #52]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r0, 0xabcdec)),
              "add r1, r0, #52480\n"
              "add r1, #11206656\n"
              "ldrd r0, r1, [r1, #236]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r0, 0xabcdec)),
              "add r1, r0, #248832\n"
              "add r1, #11010048\n"
              "ldrd r0, r1, [r1, #492]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r0, -0xabcdec)),
              "sub r1, r0, #52736\n"
              "sub r1, #11206656\n"
              "ldrd r0, r1, [r1, #20]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r0, -0xabcdec)),
              "sub r1, r0, #774144\n"
              "sub r1, #10485760\n"
              "ldrd r0, r1, [r1, #532]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r1, 0xabcc)),
              "add r0, r1, #43776\n"
              "ldrd r0, r1, [r0, #204]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r1, 0xabcc)),
              "add r0, r1, #43008\n"
              "ldrd r0, r1, [r0, #972]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r1, -0xabcc)),
              "sub r0, r1, #44032\n"
              "ldrd r0, r1, [r0, #52]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r1, -0xabcc)),
              "sub r0, r1, #44032\n"
              "ldrd r0, r1, [r0, #52]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r1, 0xabcdec)),
              "add r0, r1, #52480\n"
              "add r0, #11206656\n"
              "ldrd r0, r1, [r0, #236]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r1, 0xabcdec)),
              "add r0, r1, #248832\n"
              "add r0, #11010048\n"
              "ldrd r0, r1, [r0, #492]\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r1, -0xabcdec)),
              "sub r0, r1, #52736\n"
              "sub r0, #11206656\n"
              "ldrd r0, r1, [r0, #20]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r1, -0xabcdec)),
              "sub r0, r1, #774144\n"
              "sub r0, #10485760\n"
              "ldrd r0, r1, [r0, #532]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 0xabcc, PostIndex)),
              "ldrd r0, r1, [r2], #204\n"
              "add r2, #43776\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 0xabcc, PostIndex)),
              "ldrd r0, r1, [r2], #972\n"
              "add r2, #43008\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -0xabcc, PostIndex)),
              "ldrd r0, r1, [r2], #52\n"
              "sub r2, #44032\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -0xabcc, PostIndex)),
              "ldrd r0, r1, [r2], #52\n"
              "sub r2, #44032\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 0xabcdec, PostIndex)),
              "ldrd r0, r1, [r2], #236\n"
              "add r2, #52480\n"
              "add r2, #11206656\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 0xabcdec, PostIndex)),
              "ldrd r0, r1, [r2], #492\n"
              "add r2, #248832\n"
              "add r2, #11010048\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -0xabcdec, PostIndex)),
              "ldrd r0, r1, [r2], #20\n"
              "sub r2, #52736\n"
              "sub r2, #11206656\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -0xabcdec, PostIndex)),
              "ldrd r0, r1, [r2], #532\n"
              "sub r2, #774144\n"
              "sub r2, #10485760\n");

  // PostIndex with the same register as base and destination is invalid.
  MUST_FAIL_TEST_BOTH(Ldrd(r0, r1, MemOperand(r0, 0xabcd, PostIndex)),
                      "Ill-formed 'ldrd' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldrd(r0, r1, MemOperand(r1, 0xabcdef, PostIndex)),
                      "Ill-formed 'ldrd' instruction.\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 0xabcc, PreIndex)),
              "add r2, #43776\n"
              "ldrd r0, r1, [r2, #204]!\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 0xabcc, PreIndex)),
              "add r2, #43008\n"
              "ldrd r0, r1, [r2, #972]!\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -0xabcc, PreIndex)),
              "sub r2, #44032\n"
              "ldrd r0, r1, [r2, #52]!\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -0xabcc, PreIndex)),
              "sub r2, #44032\n"
              "ldrd r0, r1, [r2, #52]!\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, 0xabcdec, PreIndex)),
              "add r2, #52480\n"
              "add r2, #11206656\n"
              "ldrd r0, r1, [r2, #236]!\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, 0xabcdec, PreIndex)),
              "add r2, #248832\n"
              "add r2, #11010048\n"
              "ldrd r0, r1, [r2, #492]!\n");
  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, -0xabcdec, PreIndex)),
              "sub r2, #52736\n"
              "sub r2, #11206656\n"
              "ldrd r0, r1, [r2, #20]!\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, -0xabcdec, PreIndex)),
              "sub r2, #774144\n"
              "sub r2, #10485760\n"
              "ldrd r0, r1, [r2, #532]!\n");

  // - Tests with register offsets.

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, r3)), "ldrd r0, r1, [r2, r3]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, r3)),
              "add r0, r2, r3\n"
              "ldrd r0, r1, [r0]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, minus, r3)),
              "ldrd r0, r1, [r2, -r3]\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, minus, r3)),
              "sub r0, r2, r3\n"
              "ldrd r0, r1, [r0]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, r3, PostIndex)),
              "ldrd r0, r1, [r2], r3\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, r3, PostIndex)),
              "ldrd r0, r1, [r2]\n"
              "add r2, r3\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, minus, r3, PostIndex)),
              "ldrd r0, r1, [r2], -r3\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, minus, r3, PostIndex)),
              "ldrd r0, r1, [r2]\n"
              "sub r2, r3\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, r3, PreIndex)),
              "ldrd r0, r1, [r2, r3]!\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, r3, PreIndex)),
              "add r2, r3\n"
              "ldrd r0, r1, [r2]\n");

  COMPARE_A32(Ldrd(r0, r1, MemOperand(r2, minus, r3, PreIndex)),
              "ldrd r0, r1, [r2, -r3]!\n");
  COMPARE_T32(Ldrd(r0, r1, MemOperand(r2, minus, r3, PreIndex)),
              "sub r2, r3\n"
              "ldrd r0, r1, [r2]\n");

  // - We do not support register shifted base register operands with LDRD.

  MUST_FAIL_TEST_BOTH(Ldrd(r0, r1, MemOperand(r2, r3, LSL, 4)),
                      "Ill-formed 'ldrd' instruction.\n");

  // First register is odd - rejected by the Assembler.
  MUST_FAIL_TEST_A32(Ldrd(r1, r2, MemOperand(r0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Ldrd(r1, r2, MemOperand(r0, r0, PreIndex)),
                     "Unpredictable instruction.\n");
  // First register is odd - rejected by the MacroAssembler.
  MUST_FAIL_TEST_A32(Ldrd(r1, r2, MemOperand(r0, 0xabcd, PreIndex)),
                     "Ill-formed 'ldrd' instruction.\n");

  // First register is lr - rejected by the Assembler.
  MUST_FAIL_TEST_A32(Ldrd(lr, pc, MemOperand(r0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Ldrd(lr, pc, MemOperand(r0, r0, PreIndex)),
                     "Unpredictable instruction.\n");
  // First register is lr - rejected by the MacroAssembler.
  MUST_FAIL_TEST_A32(Ldrd(lr, pc, MemOperand(r0, 0xabcd, PreIndex)),
                     "Ill-formed 'ldrd' instruction.\n");

  // Non-adjacent registers.
  MUST_FAIL_TEST_A32(Ldrd(r0, r2, MemOperand(r0)),
                     "Ill-formed 'ldrd' instruction.\n");

  CLEANUP();
}

TEST(macro_assembler_strd) {
  SETUP();

  // - Tests with no offset.

  COMPARE_BOTH(Strd(r0, r1, MemOperand(r3)), "strd r0, r1, [r3]\n");
  // Destination registers need to start with a even numbered register on A32.
  MUST_FAIL_TEST_A32(Strd(r1, r2, MemOperand(r3)),
                     "Unpredictable instruction.\n");
  COMPARE_T32(Strd(r1, r2, MemOperand(r3)), "strd r1, r2, [r3]\n");
  // Registers need to be adjacent on A32.
  MUST_FAIL_TEST_A32(Strd(r0, r2, MemOperand(r1)),
                     "Ill-formed 'strd' instruction.\n");
  COMPARE_T32(Strd(r0, r2, MemOperand(r1)), "strd r0, r2, [r1]\n");

  COMPARE_BOTH(Strd(r0, r1, MemOperand(r2)), "strd r0, r1, [r2]\n");

  // - Tests with immediate offsets.

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 1020)),
              "add ip, r2, #1020\n"
              "strd r0, r1, [ip]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 1020)), "strd r0, r1, [r2, #1020]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -1020)),
              "sub ip, r2, #1020\n"
              "strd r0, r1, [ip]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -1020)),
              "strd r0, r1, [r2, #-1020]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 0xabcc)),
              "add ip, r2, #43776\n"
              "strd r0, r1, [ip, #204]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 0xabcc)),
              "add ip, r2, #43008\n"
              "strd r0, r1, [ip, #972]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -0xabcc)),
              "sub ip, r2, #44032\n"
              "strd r0, r1, [ip, #52]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -0xabcc)),
              "sub ip, r2, #44032\n"
              "strd r0, r1, [ip, #52]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 0xabcdec)),
              "add ip, r2, #52480\n"
              "add ip, #11206656\n"
              "strd r0, r1, [ip, #236]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 0xabcdec)),
              "add ip, r2, #248832\n"
              "add ip, #11010048\n"
              "strd r0, r1, [ip, #492]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -0xabcdec)),
              "sub ip, r2, #52736\n"
              "sub ip, #11206656\n"
              "strd r0, r1, [ip, #20]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -0xabcdec)),
              "sub ip, r2, #774144\n"
              "sub ip, #10485760\n"
              "strd r0, r1, [ip, #532]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r0, 0xabcc)),
              "add ip, r0, #43776\n"
              "strd r0, r1, [ip, #204]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r0, 0xabcc)),
              "add ip, r0, #43008\n"
              "strd r0, r1, [ip, #972]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r0, -0xabcc)),
              "sub ip, r0, #44032\n"
              "strd r0, r1, [ip, #52]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r0, -0xabcc)),
              "sub ip, r0, #44032\n"
              "strd r0, r1, [ip, #52]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r0, 0xabcdec)),
              "add ip, r0, #52480\n"
              "add ip, #11206656\n"
              "strd r0, r1, [ip, #236]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r0, 0xabcdec)),
              "add ip, r0, #248832\n"
              "add ip, #11010048\n"
              "strd r0, r1, [ip, #492]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r0, -0xabcdec)),
              "sub ip, r0, #52736\n"
              "sub ip, #11206656\n"
              "strd r0, r1, [ip, #20]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r0, -0xabcdec)),
              "sub ip, r0, #774144\n"
              "sub ip, #10485760\n"
              "strd r0, r1, [ip, #532]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r1, 0xabcc)),
              "add ip, r1, #43776\n"
              "strd r0, r1, [ip, #204]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r1, 0xabcc)),
              "add ip, r1, #43008\n"
              "strd r0, r1, [ip, #972]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r1, -0xabcc)),
              "sub ip, r1, #44032\n"
              "strd r0, r1, [ip, #52]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r1, -0xabcc)),
              "sub ip, r1, #44032\n"
              "strd r0, r1, [ip, #52]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r1, 0xabcdec)),
              "add ip, r1, #52480\n"
              "add ip, #11206656\n"
              "strd r0, r1, [ip, #236]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r1, 0xabcdec)),
              "add ip, r1, #248832\n"
              "add ip, #11010048\n"
              "strd r0, r1, [ip, #492]\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r1, -0xabcdec)),
              "sub ip, r1, #52736\n"
              "sub ip, #11206656\n"
              "strd r0, r1, [ip, #20]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r1, -0xabcdec)),
              "sub ip, r1, #774144\n"
              "sub ip, #10485760\n"
              "strd r0, r1, [ip, #532]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 0xabcc, PostIndex)),
              "strd r0, r1, [r2], #204\n"
              "add r2, #43776\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 0xabcc, PostIndex)),
              "strd r0, r1, [r2], #972\n"
              "add r2, #43008\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -0xabcc, PostIndex)),
              "strd r0, r1, [r2], #52\n"
              "sub r2, #44032\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -0xabcc, PostIndex)),
              "strd r0, r1, [r2], #52\n"
              "sub r2, #44032\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 0xabcdec, PostIndex)),
              "strd r0, r1, [r2], #236\n"
              "add r2, #52480\n"
              "add r2, #11206656\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 0xabcdec, PostIndex)),
              "strd r0, r1, [r2], #492\n"
              "add r2, #248832\n"
              "add r2, #11010048\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -0xabcdec, PostIndex)),
              "strd r0, r1, [r2], #20\n"
              "sub r2, #52736\n"
              "sub r2, #11206656\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -0xabcdec, PostIndex)),
              "strd r0, r1, [r2], #532\n"
              "sub r2, #774144\n"
              "sub r2, #10485760\n");

  // PostIndex with the same register as base and source is invalid.
  MUST_FAIL_TEST_BOTH(Strd(r0, r1, MemOperand(r0, 0xabcd, PostIndex)),
                      "Ill-formed 'strd' instruction.\n");
  MUST_FAIL_TEST_BOTH(Strd(r0, r1, MemOperand(r1, 0xabcdef, PostIndex)),
                      "Ill-formed 'strd' instruction.\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 0xabcc, PreIndex)),
              "add r2, #43776\n"
              "strd r0, r1, [r2, #204]!\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 0xabcc, PreIndex)),
              "add r2, #43008\n"
              "strd r0, r1, [r2, #972]!\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -0xabcc, PreIndex)),
              "sub r2, #44032\n"
              "strd r0, r1, [r2, #52]!\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -0xabcc, PreIndex)),
              "sub r2, #44032\n"
              "strd r0, r1, [r2, #52]!\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, 0xabcdec, PreIndex)),
              "add r2, #52480\n"
              "add r2, #11206656\n"
              "strd r0, r1, [r2, #236]!\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, 0xabcdec, PreIndex)),
              "add r2, #248832\n"
              "add r2, #11010048\n"
              "strd r0, r1, [r2, #492]!\n");
  COMPARE_A32(Strd(r0, r1, MemOperand(r2, -0xabcdec, PreIndex)),
              "sub r2, #52736\n"
              "sub r2, #11206656\n"
              "strd r0, r1, [r2, #20]!\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, -0xabcdec, PreIndex)),
              "sub r2, #774144\n"
              "sub r2, #10485760\n"
              "strd r0, r1, [r2, #532]!\n");

  // - Tests with register offsets.

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, r3)), "strd r0, r1, [r2, r3]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, r3)),
              "add ip, r2, r3\n"
              "strd r0, r1, [ip]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, minus, r3)),
              "strd r0, r1, [r2, -r3]\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, minus, r3)),
              "sub ip, r2, r3\n"
              "strd r0, r1, [ip]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, r3, PostIndex)),
              "strd r0, r1, [r2], r3\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, r3, PostIndex)),
              "strd r0, r1, [r2]\n"
              "add r2, r3\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, minus, r3, PostIndex)),
              "strd r0, r1, [r2], -r3\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, minus, r3, PostIndex)),
              "strd r0, r1, [r2]\n"
              "sub r2, r3\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, r3, PreIndex)),
              "strd r0, r1, [r2, r3]!\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, r3, PreIndex)),
              "add r2, r3\n"
              "strd r0, r1, [r2]\n");

  COMPARE_A32(Strd(r0, r1, MemOperand(r2, minus, r3, PreIndex)),
              "strd r0, r1, [r2, -r3]!\n");
  COMPARE_T32(Strd(r0, r1, MemOperand(r2, minus, r3, PreIndex)),
              "sub r2, r3\n"
              "strd r0, r1, [r2]\n");

  // - We do not support register shifted base register operands with LDRD.

  MUST_FAIL_TEST_BOTH(Strd(r0, r1, MemOperand(r2, r3, LSL, 4)),
                      "Ill-formed 'strd' instruction.\n");

  // First register is odd - rejected by the Assembler.
  MUST_FAIL_TEST_A32(Strd(r1, r2, MemOperand(r0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Strd(r1, r2, MemOperand(r0, r0, PreIndex)),
                     "Unpredictable instruction.\n");
  // First register is odd - rejected by the MacroAssembler.
  MUST_FAIL_TEST_A32(Strd(r1, r2, MemOperand(r0, 0xabcd, PreIndex)),
                     "Ill-formed 'strd' instruction.\n");

  // First register is lr - rejected by the Assembler.
  MUST_FAIL_TEST_A32(Strd(lr, pc, MemOperand(r0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Strd(lr, pc, MemOperand(r0, r0, PreIndex)),
                     "Unpredictable instruction.\n");
  // First register is lr - rejected by the MacroAssembler.
  MUST_FAIL_TEST_A32(Strd(lr, pc, MemOperand(r0, 0xabcd, PreIndex)),
                     "Ill-formed 'strd' instruction.\n");

  // Non-adjacent registers.
  MUST_FAIL_TEST_A32(Strd(r0, r2, MemOperand(r0)),
                     "Ill-formed 'strd' instruction.\n");

  CLEANUP();
}


TEST(macro_assembler_wide_immediate) {
  SETUP();

  COMPARE_BOTH(Adc(r0, r1, 0xbadbeef),
               "mov r0, #48879\n"
               "movt r0, #2989\n"
               "adc r0, r1, r0\n");

  COMPARE_BOTH(Add(r0, r0, 0xbadbeef),
               "mov ip, #48879\n"
               "movt ip, #2989\n"
               "add r0, ip\n");

  COMPARE_BOTH(Mov(r0, 0xbadbeef),
               "mov r0, #48879\n"
               "movt r0, #2989\n");
  COMPARE_A32(Mov(eq, r0, 0xbadbeef),
              "moveq r0, #48879\n"
              "movteq r0, #2989\n");
  COMPARE_T32(Mov(eq, r0, 0xbadbeef),
              "bne 0x0000000a\n"
              "mov r0, #48879\n"
              "movt r0, #2989\n");

  COMPARE_BOTH(Movs(r0, 0xbadbeef),
               "mov r0, #48879\n"
               "movt r0, #2989\n"
               "tst r0, r0\n");
  COMPARE_A32(Movs(eq, r0, 0xbadbeef),
              "moveq r0, #48879\n"
              "movteq r0, #2989\n"
              "tsteq r0, r0\n");
  COMPARE_T32(Movs(eq, r0, 0xbadbeef),
              "bne 0x0000000c\n"
              "mov r0, #48879\n"
              "movt r0, #2989\n"
              "tst r0, r0\n");
  COMPARE_A32(Movs(pc, 0x1), "movs pc, #1\n");
  MUST_FAIL_TEST_T32(Movs(pc, 0x1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(pc, 0xbadbeed), "Ill-formed 'movs' instruction.\n");

  COMPARE_BOTH(Mov(pc, 0xbadbeef),
               "mov ip, #48879\n"
               "movt ip, #2989\n"
               "bx ip\n");
  COMPARE_A32(Mov(eq, pc, 0xbadbeef),
              "mov ip, #48879\n"
              "movt ip, #2989\n"
              "bxeq ip\n");
  COMPARE_T32(Mov(eq, pc, 0xbadbeef),
              "bne 0x0000000c\n"
              "mov ip, #48879\n"
              "movt ip, #2989\n"
              "bx ip\n");

  CLEANUP();
}


TEST(macro_assembler_And) {
  SETUP();

  // Identities.
  COMPARE_BOTH(And(r0, r1, 0), "mov r0, #0\n");
  COMPARE_BOTH(And(r0, r0, 0xffffffff), "");
  CLEANUP();
}


TEST(macro_assembler_Bic) {
  SETUP();

  // Identities.
  COMPARE_BOTH(Bic(r0, r1, 0xffffffff), "mov r0, #0\n");
  COMPARE_BOTH(Bic(r0, r0, 0), "");
  CLEANUP();
}


TEST(macro_assembler_Orr) {
  SETUP();

  // Identities.
  COMPARE_BOTH(Orr(r0, r1, 0xffffffff), "mvn r0, #0\n");
  COMPARE_BOTH(Orr(r0, r0, 0), "");
  CLEANUP();
}


TEST(macro_assembler_InstructionCondSizeRROp) {
  SETUP();

  // Special case for Orr <-> Orn correspondance.

  COMPARE_T32(Orr(r0, r1, 0x00ffffff), "orn r0, r1, #0xff000000\n");
  COMPARE_T32(Orrs(r0, r1, 0x00ffffff), "orns r0, r1, #0xff000000\n");

  // Encodable immediates.

  COMPARE_A32(Add(r0, r1, -1), "sub r0, r1, #1\n");
  COMPARE_A32(Adds(r0, r1, -1), "subs r0, r1, #1\n");
  // 0xffffffff is encodable in a T32 ADD.
  COMPARE_T32(Add(r0, r1, -1), "add r0, r1, #4294967295\n");
  COMPARE_T32(Adds(r0, r1, -1), "adds r0, r1, #4294967295\n");

  COMPARE_BOTH(Add(r0, r1, -4), "sub r0, r1, #4\n");
  COMPARE_BOTH(Adds(r0, r1, -4), "subs r0, r1, #4\n");

  COMPARE_BOTH(Adc(r0, r1, -2), "sbc r0, r1, #1\n");
  COMPARE_BOTH(Adcs(r0, r1, -2), "sbcs r0, r1, #1\n");

  COMPARE_A32(Sub(r0, r1, -1), "add r0, r1, #1\n");
  COMPARE_A32(Subs(r0, r1, -1), "adds r0, r1, #1\n");
  // 0xffffffff is encodable in a T32 SUB.
  COMPARE_T32(Sub(r0, r1, -1), "sub r0, r1, #4294967295\n");
  COMPARE_T32(Subs(r0, r1, -1), "subs r0, r1, #4294967295\n");

  COMPARE_BOTH(Sub(r0, r1, -4), "add r0, r1, #4\n");
  COMPARE_BOTH(Subs(r0, r1, -4), "adds r0, r1, #4\n");

  COMPARE_BOTH(Sbc(r0, r1, -5), "adc r0, r1, #4\n");
  COMPARE_BOTH(Sbcs(r0, r1, -5), "adcs r0, r1, #4\n");

  // Non-encodable immediates

  COMPARE_BOTH(Adc(r0, r1, 0xabcd),
               "mov r0, #43981\n"
               "adc r0, r1, r0\n");

  COMPARE_BOTH(Adc(r0, r1, -0xabcd),
               "mov r0, #43980\n"  // This represents #0xabcd - 1.
               "sbc r0, r1, r0\n");

  COMPARE_BOTH(Adc(r0, r1, 0x1234abcd),
               "mov r0, #43981\n"
               "movt r0, #4660\n"
               "adc r0, r1, r0\n");

  COMPARE_BOTH(Adc(r0, r1, -0x1234abcd),
               "mov r0, #43980\n"  // This represents #0x1234abcd - 1.
               "movt r0, #4660\n"
               "sbc r0, r1, r0\n");

  // Non-encodable immediates with the same source and destination registers.

  COMPARE_BOTH(Sbc(r0, r0, 0xabcd),
               "mov ip, #43981\n"
               "sbc r0, ip\n");

  COMPARE_BOTH(Sbc(r0, r0, -0xabcd),
               "mov ip, #43980\n"  // This represents #0xabcd - 1.
               "adc r0, ip\n");

  COMPARE_BOTH(Sbc(r0, r0, 0x1234abcd),
               "mov ip, #43981\n"
               "movt ip, #4660\n"
               "sbc r0, ip\n");

  COMPARE_BOTH(Sbc(r0, r0, -0x1234abcd),
               "mov ip, #43980\n"  // This represents #0x1234abcd - 1.
               "movt ip, #4660\n"
               "adc r0, ip\n");


  // Test that we can pass a register shifted register operand in T32.

  COMPARE_T32(Adc(r0, r1, Operand(r2, LSL, r3)),
              "lsl r0, r2, r3\n"
              "adc r0, r1, r0\n");

  COMPARE_T32(Add(r3, r2, Operand(r2, ASR, r3)),
              "asr r3, r2, r3\n"
              "add r3, r2, r3\n");

  COMPARE_T32(Ands(r3, r2, Operand(r2, LSR, r2)),
              "lsr r3, r2, r2\n"
              "ands r3, r2, r3\n");

  COMPARE_T32(Asr(r2, r2, Operand(r2, ROR, r2)),
              "ror ip, r2, r2\n"
              "asr r2, ip\n");

  COMPARE_T32(Asr(r2, r2, Operand(r2, ROR, r2)),
              "ror ip, r2, r2\n"
              "asr r2, ip\n");


  CLEANUP();
}


TEST(macro_assembler_InstructionCondRO) {
  SETUP();

  COMPARE_BOTH(Teq(r0, 0xbad),
               "mov ip, #2989\n"
               "teq r0, ip\n");
  COMPARE_BOTH(Teq(r0, 0xbadbeef),
               "mov ip, #48879\n"
               "movt ip, #2989\n"
               "teq r0, ip\n");
  MUST_FAIL_TEST_T32(Teq(r0, Operand(r1, LSL, r2)),
                     "Ill-formed 'teq' instruction.\n");

  CLEANUP();
}


TEST(macro_assembler_too_large_immediate) {
  SETUP();

  // Attempting to use a 17-bit immediate with movt.
  MUST_FAIL_TEST_BOTH(Movt(r0, 0x10000), "`Movt` expects a 16-bit immediate.");

  CLEANUP();
}


TEST(macro_assembler_Cbz) {
  SETUP();

#ifdef VIXL_INCLUDE_TARGET_A32
  // Cbz/Cbnz are not available in A32 mode.
  // Make sure GetArchitectureStatePCOffset() returns the correct value.
  __ UseA32();
  Label label_64(__ GetCursorOffset() + __ GetArchitectureStatePCOffset() + 64);
  MUST_FAIL_TEST_A32(Cbz(r0, &label_64), "Cbz is only available for T32.\n");
  MUST_FAIL_TEST_A32(Cbnz(r0, &label_64), "Cbnz is only available for T32.\n");
#endif

#ifdef VIXL_INCLUDE_TARGET_T32
  // Make sure GetArchitectureStatePCOffset() returns the correct value.
  __ UseT32();
  // Largest encodable offset.
  Label label_126(__ GetCursorOffset() + __ GetArchitectureStatePCOffset() +
                  126);
  COMPARE_T32(Cbz(r0, &label_126), "cbz r0, 0x00000082\n");
  COMPARE_T32(Cbnz(r0, &label_126), "cbnz r0, 0x00000082\n");

  // Offset cannot be encoded.
  Label label_128(__ GetCursorOffset() + __ GetArchitectureStatePCOffset() +
                  128);
  COMPARE_T32(Cbz(r0, &label_128),
              "cbnz r0, 0x00000004\n"
              "b 0x00000084\n");
  COMPARE_T32(Cbnz(r0, &label_128),
              "cbz r0, 0x00000004\n"
              "b 0x00000084\n");

  // Offset that cannot be encoded and needs 32-bit branch instruction.
  Label label_8192(__ GetCursorOffset() + __ GetArchitectureStatePCOffset() +
                   8192);
  COMPARE_T32(Cbz(r0, &label_8192),
              "cbnz r0, 0x00000006\n"
              "b 0x00002004\n");
  COMPARE_T32(Cbnz(r0, &label_8192),
              "cbz r0, 0x00000006\n"
              "b 0x00002004\n");

  // Negative offset.
  Label label_neg(__ GetCursorOffset() + __ GetArchitectureStatePCOffset() +
                  -8);
  COMPARE_T32(Cbz(r0, &label_neg),
              "cbnz r0, 0x00000004\n"
              "b 0xfffffffc\n");
  COMPARE_T32(Cbnz(r0, &label_neg),
              "cbz r0, 0x00000004\n"
              "b 0xfffffffc\n");

  // Large negative offset.
  Label label_neg128(__ GetCursorOffset() + __ GetArchitectureStatePCOffset() +
                     -128);
  COMPARE_T32(Cbz(r0, &label_neg128),
              "cbnz r0, 0x00000004\n"
              "b 0xffffff84\n");
  COMPARE_T32(Cbnz(r0, &label_neg128),
              "cbz r0, 0x00000004\n"
              "b 0xffffff84\n");
#endif

  CLEANUP();
}


#ifdef VIXL_NEGATIVE_TESTING
TEST(assembler_crc_negative) {
  SETUP();

  ExactAssemblyScope scope(&masm, 2, CodeBufferCheckScope::kMaximumSize);

  masm.it(eq);

  MUST_FAIL_TEST_T32(crc32b(eq, r0, r1, r2), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(crc32cb(eq, r0, r1, r2), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(crc32ch(eq, r0, r1, r2), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(crc32cw(eq, r0, r1, r2), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(crc32h(eq, r0, r1, r2), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(crc32w(eq, r0, r1, r2), "Unpredictable instruction.\n");

  CLEANUP();
}
#endif


#ifdef VIXL_NEGATIVE_TESTING
TEST(assembler_hvc_negative) {
  SETUP();

  ExactAssemblyScope scope(&masm, 2, CodeBufferCheckScope::kMaximumSize);

  masm.it(eq);

  MUST_FAIL_TEST_T32(hvc(eq, 0), "Unpredictable instruction.\n");

  CLEANUP();
}
#endif


TEST(macro_assembler_vld) {
  SETUP();

  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld1.8 {d0}, [r1]\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld1.8 {d0}, [r1]!\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, kMultipleLanes),
                    AlignedMemOperand(r8, kNoAlignment, r2, PostIndex)),
               "vld1.8 {d0}, [r8], r2\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld1.8 {d0[]}, [r1]\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, kAllLanes),
                    AlignedMemOperand(r9, kNoAlignment, PostIndex)),
               "vld1.8 {d0[]}, [r9]!\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld1.8 {d0[]}, [r1], r2\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, 0),
                    AlignedMemOperand(r10, kNoAlignment)),
               "vld1.8 {d0[0]}, [r10]\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, 1),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld1.8 {d0[1]}, [r1]!\n");
  COMPARE_BOTH(Vld1(Untyped8,
                    NeonRegisterList(d0, 2),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld1.8 {d0[2]}, [r1], r2\n");

  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld2.8 {d0,d1}, [r1]\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld2.8 {d0,d1}, [r1]!\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, r8, PostIndex)),
               "vld2.8 {d0,d1}, [r1], r8\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld2.8 {d0[],d1[]}, [r1]\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kAllLanes),
                    AlignedMemOperand(r9, kNoAlignment, PostIndex)),
               "vld2.8 {d0[],d1[]}, [r9]!\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld2.8 {d0[],d1[]}, [r1], r2\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, 0),
                    AlignedMemOperand(r10, kNoAlignment)),
               "vld2.8 {d0[0],d1[0]}, [r10]\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, 1),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld2.8 {d0[1],d1[1]}, [r1]!\n");
  COMPARE_BOTH(Vld2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, 2),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld2.8 {d0[2],d1[2]}, [r1], r2\n");

  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld3.8 {d0,d1,d2}, [r1]\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld3.8 {d0,d1,d2}, [r1]!\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kMultipleLanes),
                    AlignedMemOperand(r11, kNoAlignment, r2, PostIndex)),
               "vld3.8 {d0,d1,d2}, [r11], r2\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kAllLanes),
                    MemOperand(r1)),
               "vld3.8 {d0[],d1[],d2[]}, [r1]\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kAllLanes),
                    MemOperand(r11, PostIndex)),
               "vld3.8 {d0[],d1[],d2[]}, [r11]!\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kAllLanes),
                    MemOperand(r1, r2, PostIndex)),
               "vld3.8 {d0[],d1[],d2[]}, [r1], r2\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, 0),
                    MemOperand(sp)),
               "vld3.8 {d0[0],d1[0],d2[0]}, [sp]\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, 1),
                    MemOperand(r1, PostIndex)),
               "vld3.8 {d0[1],d1[1],d2[1]}, [r1]!\n");
  COMPARE_BOTH(Vld3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, 2),
                    MemOperand(r1, r2, PostIndex)),
               "vld3.8 {d0[2],d1[2],d2[2]}, [r1], r2\n");

  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d6, kDouble, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld4.8 {d0,d2,d4,d6}, [r1]\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld4.8 {d0,d1,d2,d3}, [r1]!\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld4.8 {d0,d1,d2,d3}, [r1], r2\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld4.8 {d0[],d1[],d2[],d3[]}, [r1]\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d6, kDouble, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld4.8 {d0[],d2[],d4[],d6[]}, [r1]!\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, kAllLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld4.8 {d0[],d1[],d2[],d3[]}, [r1], r2\n");
  COMPARE_BOTH(Vld4(Untyped16,
                    NeonRegisterList(d0, d6, kDouble, 3),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vld4.16 {d0[3],d2[3],d4[3],d6[3]}, [r1]\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, 6),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vld4.8 {d0[6],d1[6],d2[6],d3[6]}, [r1]!\n");
  COMPARE_BOTH(Vld4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, 7),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vld4.8 {d0[7],d1[7],d2[7],d3[7]}, [r1], r2\n");

  CLEANUP();
}


TEST(macro_assembler_vst) {
  SETUP();

  COMPARE_BOTH(Vst1(Untyped8,
                    NeonRegisterList(d0, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst1.8 {d0}, [r1]\n");
  COMPARE_BOTH(Vst1(Untyped8,
                    NeonRegisterList(d0, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst1.8 {d0}, [r1]!\n");
  COMPARE_BOTH(Vst1(Untyped8,
                    NeonRegisterList(d0, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vst1.8 {d0}, [r1], r2\n");
  COMPARE_BOTH(Vst1(Untyped8,
                    NeonRegisterList(d0, 0),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst1.8 {d0[0]}, [r1]\n");
  COMPARE_BOTH(Vst1(Untyped8,
                    NeonRegisterList(d0, 1),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst1.8 {d0[1]}, [r1]!\n");
  COMPARE_BOTH(Vst1(Untyped8,
                    NeonRegisterList(d0, 2),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vst1.8 {d0[2]}, [r1], r2\n");

  COMPARE_BOTH(Vst2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst2.8 {d0,d1}, [r1]\n");
  COMPARE_BOTH(Vst2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst2.8 {d0,d1}, [r1]!\n");
  COMPARE_BOTH(Vst2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vst2.8 {d0,d1}, [r1], r2\n");
  COMPARE_BOTH(Vst2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, 3),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst2.8 {d0[3],d1[3]}, [r1]\n");
  COMPARE_BOTH(Vst2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, 4),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst2.8 {d0[4],d1[4]}, [r1]!\n");
  COMPARE_BOTH(Vst2(Untyped8,
                    NeonRegisterList(d0, d1, kSingle, 5),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vst2.8 {d0[5],d1[5]}, [r1], r2\n");

  COMPARE_BOTH(Vst3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst3.8 {d0,d1,d2}, [r1]\n");
  COMPARE_BOTH(Vst3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst3.8 {d0,d1,d2}, [r1]!\n");
  COMPARE_BOTH(Vst3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vst3.8 {d0,d1,d2}, [r1], r2\n");
  COMPARE_BOTH(Vst3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, 0),
                    MemOperand(r1)),
               "vst3.8 {d0[0],d1[0],d2[0]}, [r1]\n");
  COMPARE_BOTH(Vst3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, 6),
                    MemOperand(r1, PostIndex)),
               "vst3.8 {d0[6],d1[6],d2[6]}, [r1]!\n");
  COMPARE_BOTH(Vst3(Untyped8,
                    NeonRegisterList(d0, d2, kSingle, 7),
                    MemOperand(r1, r2, PostIndex)),
               "vst3.8 {d0[7],d1[7],d2[7]}, [r1], r2\n");

  COMPARE_BOTH(Vst4(Untyped8,
                    NeonRegisterList(d10, d13, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst4.8 {d10,d11,d12,d13}, [r1]\n");
  COMPARE_BOTH(Vst4(Untyped8,
                    NeonRegisterList(d10, d13, kSingle, kMultipleLanes),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst4.8 {d10,d11,d12,d13}, [r1]!\n");
  COMPARE_BOTH(Vst4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, kMultipleLanes),
                    AlignedMemOperand(r8, kNoAlignment, r9, PostIndex)),
               "vst4.8 {d0,d1,d2,d3}, [r8], r9\n");
  COMPARE_BOTH(Vst4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, 0),
                    AlignedMemOperand(r1, kNoAlignment)),
               "vst4.8 {d0[0],d1[0],d2[0],d3[0]}, [r1]\n");
  COMPARE_BOTH(Vst4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, 0),
                    AlignedMemOperand(r1, kNoAlignment, PostIndex)),
               "vst4.8 {d0[0],d1[0],d2[0],d3[0]}, [r1]!\n");
  COMPARE_BOTH(Vst4(Untyped8,
                    NeonRegisterList(d0, d3, kSingle, 0),
                    AlignedMemOperand(r1, kNoAlignment, r2, PostIndex)),
               "vst4.8 {d0[0],d1[0],d2[0],d3[0]}, [r1], r2\n");

  CLEANUP();
}


TEST(assembler_vldm_vstm_negative) {
  SETUP();

  ExactAssemblyScope scope(&masm, 0, CodeBufferCheckScope::kMaximumSize);

  MUST_FAIL_TEST_BOTH(fldmdbx(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(fldmiax(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(fldmiax(pc, NO_WRITE_BACK, DRegisterList(d0)),
                     "Unpredictable instruction.\n");

  MUST_FAIL_TEST_BOTH(fstmdbx(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(fstmiax(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(fstmiax(pc, NO_WRITE_BACK, DRegisterList(d0)),
                     "Unpredictable instruction.\n");

  MUST_FAIL_TEST_BOTH(vldmdb(pc, WRITE_BACK, SRegisterList(s0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vldm(pc, WRITE_BACK, SRegisterList(s0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vldm(pc, NO_WRITE_BACK, SRegisterList(s0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vldmia(pc, WRITE_BACK, SRegisterList(s0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vldmia(pc, NO_WRITE_BACK, SRegisterList(s0)),
                     "Unpredictable instruction.\n");

  MUST_FAIL_TEST_BOTH(vldmdb(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vldm(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vldm(pc, NO_WRITE_BACK, DRegisterList(d0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vldmia(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vldmia(pc, NO_WRITE_BACK, DRegisterList(d0)),
                     "Unpredictable instruction.\n");

  MUST_FAIL_TEST_BOTH(vstmdb(pc, WRITE_BACK, SRegisterList(s0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vstm(pc, WRITE_BACK, SRegisterList(s0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vstm(pc, NO_WRITE_BACK, SRegisterList(s0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vstmia(pc, WRITE_BACK, SRegisterList(s0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vstmia(pc, NO_WRITE_BACK, SRegisterList(s0)),
                     "Unpredictable instruction.\n");

  MUST_FAIL_TEST_BOTH(vstmdb(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vstm(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vstm(pc, NO_WRITE_BACK, DRegisterList(d0)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(vstmia(pc, WRITE_BACK, DRegisterList(d0)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(vstmia(pc, NO_WRITE_BACK, DRegisterList(d0)),
                     "Unpredictable instruction.\n");

  CLEANUP();
}


#define TEST_VMEMOP(MACRO_OP, STRING_OP, DST_REG)                    \
  SETUP();                                                           \
                                                                     \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, 1024)),               \
              "add ip, r8, #1024\n" STRING_OP #DST_REG ", [ip]\n");  \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, 1371)),               \
              "add ip, r8, #1371\n" STRING_OP #DST_REG ", [ip]\n");  \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, 4113)),               \
              "add ip, r8, #17\n"                                    \
              "add ip, #4096\n" STRING_OP #DST_REG ", [ip]\n");      \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, 65808)),              \
              "add ip, r8, #272\n"                                   \
              "add ip, #65536\n" STRING_OP #DST_REG ", [ip]\n");     \
                                                                     \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, -1024)),              \
              "sub ip, r8, #1024\n" STRING_OP #DST_REG ", [ip]\n");  \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, -1371)),              \
              "sub ip, r8, #1371\n" STRING_OP #DST_REG ", [ip]\n");  \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, -4113)),              \
              "sub ip, r8, #17\n"                                    \
              "sub ip, #4096\n" STRING_OP #DST_REG ", [ip]\n");      \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r8, -65808)),             \
              "sub ip, r8, #272\n"                                   \
              "sub ip, #65536\n" STRING_OP #DST_REG ", [ip]\n");     \
                                                                     \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, 0, PreIndex)),        \
              STRING_OP #DST_REG ", [r9]\n");                        \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, 137, PreIndex)),      \
              "add r9, #137\n" STRING_OP #DST_REG ", [r9]\n");       \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, 4110, PreIndex)),     \
              "add r9, #14\n"                                        \
              "add r9, #4096\n" STRING_OP #DST_REG ", [r9]\n");      \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, 65623, PreIndex)),    \
              "add r9, #87\n"                                        \
              "add r9, #65536\n" STRING_OP #DST_REG ", [r9]\n");     \
                                                                     \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, -137, PreIndex)),     \
              "sub r9, #137\n" STRING_OP #DST_REG ", [r9]\n");       \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, -4110, PreIndex)),    \
              "sub r9, #14\n"                                        \
              "sub r9, #4096\n" STRING_OP #DST_REG ", [r9]\n");      \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r9, -65623, PreIndex)),   \
              "sub r9, #87\n"                                        \
              "sub r9, #65536\n" STRING_OP #DST_REG ", [r9]\n");     \
                                                                     \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, 0, PostIndex)),      \
              STRING_OP #DST_REG ", [r10]\n");                       \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, 137, PostIndex)),    \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "add r10, #137\n");                                    \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, 4110, PostIndex)),   \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "add r10, #14\n"                                       \
              "add r10, #4096\n");                                   \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, 65623, PostIndex)),  \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "add r10, #87\n"                                       \
              "add r10, #65536\n");                                  \
                                                                     \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, -137, PostIndex)),   \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "sub r10, #137\n");                                    \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, -4110, PostIndex)),  \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "sub r10, #14\n"                                       \
              "sub r10, #4096\n");                                   \
  COMPARE_T32(MACRO_OP(DST_REG, MemOperand(r10, -65623, PostIndex)), \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "sub r10, #87\n"                                       \
              "sub r10, #65536\n");                                  \
  CLEANUP();

TEST(macro_assembler_T32_Vldr_d) { TEST_VMEMOP(Vldr, "vldr ", d0); }

TEST(macro_assembler_T32_Vstr_d) { TEST_VMEMOP(Vstr, "vstr ", d1); }

TEST(macro_assembler_T32_Vldr_s) { TEST_VMEMOP(Vldr, "vldr ", s2); }

TEST(macro_assembler_T32_Vstr_s) { TEST_VMEMOP(Vstr, "vstr ", s3); }

#undef TEST_VMEMOP

#define TEST_VMEMOP(MACRO_OP, STRING_OP, DST_REG)                    \
  SETUP();                                                           \
                                                                     \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r8, 137)),                \
              "add ip, r8, #137\n" STRING_OP #DST_REG ", [ip]\n");   \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r8, 274)),                \
              "add ip, r8, #18\n"                                    \
              "add ip, #256\n" STRING_OP #DST_REG ", [ip]\n");       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r8, 65623)),              \
              "add ip, r8, #87\n"                                    \
              "add ip, #65536\n" STRING_OP #DST_REG ", [ip]\n");     \
                                                                     \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r8, -137)),               \
              "sub ip, r8, #137\n" STRING_OP #DST_REG ", [ip]\n");   \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r8, -274)),               \
              "sub ip, r8, #18\n"                                    \
              "sub ip, #256\n" STRING_OP #DST_REG ", [ip]\n");       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r8, -65623)),             \
              "sub ip, r8, #87\n"                                    \
              "sub ip, #65536\n" STRING_OP #DST_REG ", [ip]\n");     \
                                                                     \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, 0, PreIndex)),        \
              STRING_OP #DST_REG ", [r9]\n");                        \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, 137, PreIndex)),      \
              "add r9, #137\n" STRING_OP #DST_REG ", [r9]\n");       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, 274, PreIndex)),      \
              "add r9, #18\n"                                        \
              "add r9, #256\n" STRING_OP #DST_REG ", [r9]\n");       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, 65623, PreIndex)),    \
              "add r9, #87\n"                                        \
              "add r9, #65536\n" STRING_OP #DST_REG ", [r9]\n");     \
                                                                     \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, -137, PreIndex)),     \
              "sub r9, #137\n" STRING_OP #DST_REG ", [r9]\n");       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, -274, PreIndex)),     \
              "sub r9, #18\n"                                        \
              "sub r9, #256\n" STRING_OP #DST_REG ", [r9]\n");       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r9, -65623, PreIndex)),   \
              "sub r9, #87\n"                                        \
              "sub r9, #65536\n" STRING_OP #DST_REG ", [r9]\n");     \
                                                                     \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, 0, PostIndex)),      \
              STRING_OP #DST_REG ", [r10]\n");                       \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, 137, PostIndex)),    \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "add r10, #137\n");                                    \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, 274, PostIndex)),    \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "add r10, #18\n"                                       \
              "add r10, #256\n");                                    \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, 65623, PostIndex)),  \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "add r10, #87\n"                                       \
              "add r10, #65536\n");                                  \
                                                                     \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, -137, PostIndex)),   \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "sub r10, #137\n");                                    \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, -274, PostIndex)),   \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "sub r10, #18\n"                                       \
              "sub r10, #256\n");                                    \
  COMPARE_A32(MACRO_OP(DST_REG, MemOperand(r10, -65623, PostIndex)), \
              STRING_OP #DST_REG                                     \
              ", [r10]\n"                                            \
              "sub r10, #87\n"                                       \
              "sub r10, #65536\n");                                  \
  CLEANUP();


TEST(macro_assembler_A32_Vldr_d) { TEST_VMEMOP(Vldr, "vldr ", d0); }

TEST(macro_assembler_A32_Vstr_d) { TEST_VMEMOP(Vstr, "vstr ", d1); }

TEST(macro_assembler_A32_Vldr_s) { TEST_VMEMOP(Vldr, "vldr ", s2); }

TEST(macro_assembler_A32_Vstr_s) { TEST_VMEMOP(Vstr, "vstr ", s3); }

#undef TEST_VMEMOP

TEST(assembler_vstr_negative) {
  SETUP();

  ExactAssemblyScope scope(&masm, 8, CodeBufferCheckScope::kMaximumSize);

  MUST_FAIL_TEST_T32(vstr(s0, MemOperand(pc, 0)),
                     "Unpredictable instruction.\n");

  MUST_FAIL_TEST_T32(vstr(d0, MemOperand(pc, 0)),
                     "Unpredictable instruction.\n");

  CLEANUP();
}

TEST(macro_assembler_Vldr_Vstr_negative) {
  SETUP();

  MUST_FAIL_TEST_BOTH(Vldr(s0, MemOperand(pc, 1, PreIndex)),
                      "The MacroAssembler does not convert vldr or vstr"
                      " with a PC base register.\n");

  MUST_FAIL_TEST_BOTH(Vldr(s0, MemOperand(pc, r0, PreIndex)),
                      "Ill-formed 'vldr' instruction.\n");

  MUST_FAIL_TEST_BOTH(Vstr(s0, MemOperand(pc, 1, PreIndex)),
                      "The MacroAssembler does not convert vldr or vstr"
                      " with a PC base register.\n");

  MUST_FAIL_TEST_BOTH(Vstr(s0, MemOperand(pc, r0, PreIndex)),
                      "Ill-formed 'vstr' instruction.\n");

  MUST_FAIL_TEST_BOTH(Vldr(d0, MemOperand(pc, 1, PreIndex)),
                      "The MacroAssembler does not convert vldr or vstr"
                      " with a PC base register.\n");

  MUST_FAIL_TEST_BOTH(Vldr(d0, MemOperand(pc, r0, PreIndex)),
                      "Ill-formed 'vldr' instruction.\n");

  MUST_FAIL_TEST_BOTH(Vstr(d0, MemOperand(pc, 1, PreIndex)),
                      "The MacroAssembler does not convert vldr or vstr"
                      " with a PC base register.\n");

  MUST_FAIL_TEST_BOTH(Vstr(d0, MemOperand(pc, r0, PreIndex)),
                      "Ill-formed 'vstr' instruction.\n");
  CLEANUP();
}

#define TEST_SHIFT_T32(Inst, name, offset)          \
  COMPARE_T32(Inst(r0, Operand(r1, LSL, r2)),       \
              "lsl ip, r1, r2\n" name " r0, ip\n"); \
  COMPARE_T32(Inst(r0, Operand(r1, LSR, r2)),       \
              "lsr ip, r1, r2\n" name " r0, ip\n"); \
  COMPARE_T32(Inst(r0, Operand(r1, ASR, r2)),       \
              "asr ip, r1, r2\n" name " r0, ip\n"); \
  COMPARE_T32(Inst(r0, Operand(r1, ROR, r2)),       \
              "ror ip, r1, r2\n" name " r0, ip\n"); \
  COMPARE_T32(Inst(eq, r0, Operand(r1, LSL, r2)),   \
              "bne " #offset                        \
              "\n"                                  \
              "lsl ip, r1, r2\n" name " r0, ip\n"); \
  COMPARE_T32(Inst(le, r0, Operand(r1, LSL, r2)),   \
              "bgt " #offset                        \
              "\n"                                  \
              "lsl ip, r1, r2\n" name " r0, ip\n");

#define TEST_MOV_SHIFT_T32(Inst, s, offset)                             \
  COMPARE_T32(Inst(r0, Operand(r1, LSL, r2)), "lsl" s " r0, r1, r2\n"); \
  COMPARE_T32(Inst(r0, Operand(r1, LSR, r2)), "lsr" s " r0, r1, r2\n"); \
  COMPARE_T32(Inst(r0, Operand(r1, ASR, r2)), "asr" s " r0, r1, r2\n"); \
  COMPARE_T32(Inst(r0, Operand(r1, ROR, r2)), "ror" s " r0, r1, r2\n"); \
  COMPARE_T32(Inst(eq, r0, Operand(r1, LSL, r2)),                       \
              "bne " #offset                                            \
              "\n"                                                      \
              "lsl" s " r0, r1, r2\n");                                 \
  COMPARE_T32(Inst(le, r0, Operand(r1, LSL, r2)),                       \
              "bgt " #offset                                            \
              "\n"                                                      \
              "lsl" s " r0, r1, r2\n");

#define TEST_WIDE_IMMEDIATE(Inst, name, offset)         \
  COMPARE_BOTH(Inst(r0, 0xbadbeef),                     \
               "mov ip, #48879\n"                       \
               "movt ip, #2989\n" name " r0, ip\n");    \
  COMPARE_A32(Inst(eq, r0, 0xbadbeef),                  \
              "moveq ip, #48879\n"                      \
              "movteq ip, #2989\n" name "eq r0, ip\n"); \
  COMPARE_T32(Inst(eq, r0, 0xbadbeef),                  \
              "bne " #offset                            \
              "\n"                                      \
              "mov ip, #48879\n"                        \
              "movt ip, #2989\n" name " r0, ip\n");

#define TEST_WIDE_IMMEDIATE_PC(Inst, name, offset)            \
  COMPARE_A32(Inst(pc, 0xbadbeef),                            \
              "mov ip, #48879\n"                              \
              "movt ip, #2989\n" name " pc, ip\n");           \
  COMPARE_A32(Inst(eq, pc, 0xbadbeef),                        \
              "moveq ip, #48879\n"                            \
              "movteq ip, #2989\n" name "eq pc, ip\n");       \
  MUST_FAIL_TEST_T32(Inst(pc, 0xbadbeef),                     \
                     "Ill-formed '" name "' instruction.\n"); \
  MUST_FAIL_TEST_T32(Inst(eq, pc, 0xbadbeef),                 \
                     "Ill-formed '" name "' instruction.\n");

TEST(macro_assembler_InstructionCondSizeROp) {
  SETUP();

  // T32 register shifted register.
  TEST_SHIFT_T32(Cmn, "cmn", 0x0000000a)
  TEST_SHIFT_T32(Cmp, "cmp", 0x00000008)
  TEST_SHIFT_T32(Mvn, "mvn", 0x0000000a)
  TEST_SHIFT_T32(Mvns, "mvns", 0x0000000a)
  TEST_SHIFT_T32(Sxtb, "sxtb", 0x0000000a)
  TEST_SHIFT_T32(Sxth, "sxth", 0x0000000a)
  TEST_SHIFT_T32(Tst, "tst", 0x0000000a)
  TEST_SHIFT_T32(Uxtb, "uxtb", 0x0000000a)
  TEST_SHIFT_T32(Uxth, "uxth", 0x0000000a)

  TEST_MOV_SHIFT_T32(Mov, "", 0x00000006)
  TEST_MOV_SHIFT_T32(Movs, "s", 0x00000006)

  MUST_FAIL_TEST_BOTH(Movs(pc, r0), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(pc, Operand(r0, LSL, 0x4)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(pc, Operand(r0, ASR, r2)),
                      "Unpredictable instruction.\n");

  // Wide immediates (Mov and Movs are tested in
  // "macro_assembler_wide_immediate").
  TEST_WIDE_IMMEDIATE(Cmp, "cmp", 0x0000000c);
  TEST_WIDE_IMMEDIATE(Cmn, "cmn", 0x0000000e);
  TEST_WIDE_IMMEDIATE(Tst, "tst", 0x0000000e);
  TEST_WIDE_IMMEDIATE_PC(Cmp, "cmp", 0x0000000c);
  TEST_WIDE_IMMEDIATE_PC(Cmn, "cmn", 0x0000000e);
  TEST_WIDE_IMMEDIATE_PC(Tst, "tst", 0x0000000e);

  // For Mvn and Mvns, we don't allow PC as a destination.
  TEST_WIDE_IMMEDIATE(Mvn, "mvn", 0x0000000e);
  TEST_WIDE_IMMEDIATE(Mvns, "mvns", 0x0000000e);
  MUST_FAIL_TEST_BOTH(Mvn(pc, 0xbadbeef), "Ill-formed 'mvn' instruction.\n");
  MUST_FAIL_TEST_BOTH(Mvn(eq, pc, 0xbadbeef),
                      "Ill-formed 'mvn' instruction.\n");
  MUST_FAIL_TEST_BOTH(Mvns(pc, 0xbadbeef), "Ill-formed 'mvns' instruction.\n");
  MUST_FAIL_TEST_BOTH(Mvns(eq, pc, 0xbadbeef),
                      "Ill-formed 'mvns' instruction.\n");

  MUST_FAIL_TEST_BOTH(Sxtb(r0, 0x1), "Ill-formed 'sxtb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Sxth(r0, 0x1), "Ill-formed 'sxth' instruction.\n");
  MUST_FAIL_TEST_BOTH(Uxtb(r0, 0x1), "Ill-formed 'uxtb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Uxth(r0, 0x1), "Ill-formed 'uxth' instruction.\n");

  CLEANUP();
}

#undef TEST_SHIFT_T32
#undef TEST_MOV_SHIFT_T32
#undef TEST_WIDE_IMMEDIATE
#undef TEST_WIDE_IMMEDIATE_PC

TEST(macro_assembler_Msr) {
  SETUP();

  // Msr with immediate for T32.
  COMPARE_T32(Msr(APSR_nzcvq, 0x0),
              "mov ip, #0\n"
              "msr APSR_nzcvq, ip\n");

  // Wide immediate.
  COMPARE_BOTH(Msr(APSR_nzcvq, 0xbadbeef),
               "mov ip, #48879\n"
               "movt ip, #2989\n"
               "msr APSR_nzcvq, ip\n");

  // Other types of operands are not handled.
  MUST_FAIL_TEST_BOTH(Msr(APSR_nzcvq, Operand(r0, LSR, r1)),
                      "Ill-formed 'msr' instruction.\n");
  CLEANUP();
}


TEST(macro_assembler_Vmov_imm) {
  SETUP();

  COMPARE_BOTH(Vmov(s0, 0.0f),
               "mov ip, #0\n"
               "vmov s0, ip\n");
  COMPARE_BOTH(Vmov(s1, 1.0f), "vmov.f32 s1, #1\n");
  COMPARE_BOTH(Vmov(s2, RawbitsToFloat(0x0000db6c)),
               "mov ip, #56172\n"
               "vmov s2, ip\n");
  COMPARE_BOTH(Vmov(s3, RawbitsToFloat(0x327b23c6)),
               "mov ip, #9158\n"
               "movt ip, #12923\n"
               "vmov s3, ip\n");
  COMPARE_BOTH(Vmov(s4, RawbitsToFloat(0xffcc7fff)),
               "mvn ip, #3375104\n"
               "vmov s4, ip\n");
  COMPARE_BOTH(Vmov(s5, RawbitsToFloat(0xb72df575)),
               "mov ip, #62837\n"
               "movt ip, #46893\n"
               "vmov s5, ip\n");

  COMPARE_BOTH(Vmov(d6, 0.0), "vmov.i64 d6, #0x0000000000000000\n");
  COMPARE_BOTH(Vmov(d7, 1.0), "vmov.f64 d7, #1\n");
  COMPARE_BOTH(Vmov(d8, RawbitsToDouble(0x000000000000af8e)),
               "mov ip, #44942\n"
               "vdup.32 d8, ip\n"
               "mov ip, #0\n"
               "vmov.32 d8[1], ip\n");
  COMPARE_BOTH(Vmov(d9, RawbitsToDouble(0x000070210000af8e)),
               "mov ip, #44942\n"
               "vdup.32 d9, ip\n"
               "mov ip, #28705\n"
               "vmov.32 d9[1], ip\n");
  COMPARE_BOTH(Vmov(d10, RawbitsToDouble(0x7021000000000000)),
               "mov ip, #0\n"
               "vdup.32 d10, ip\n"
               "mov ip, #0\n"
               "movt ip, #28705\n"
               "vmov.32 d10[1], ip\n");
  COMPARE_BOTH(Vmov(d11, RawbitsToDouble(0x7021da4b0000af8e)),
               "mov ip, #44942\n"
               "vdup.32 d11, ip\n"
               "mov ip, #55883\n"
               "movt ip, #28705\n"
               "vmov.32 d11[1], ip\n");
  COMPARE_BOTH(Vmov(d12, RawbitsToDouble(0x0cff553204ec4a3f)),
               "mov ip, #19007\n"
               "movt ip, #1260\n"
               "vdup.32 d12, ip\n"
               "mov ip, #21810\n"
               "movt ip, #3327\n"
               "vmov.32 d12[1], ip\n");
  COMPARE_BOTH(Vmov(d13, RawbitsToDouble(0xa2037ad20000f592)),
               "mov ip, #62866\n"
               "vdup.32 d13, ip\n"
               "mov ip, #31442\n"
               "movt ip, #41475\n"
               "vmov.32 d13[1], ip\n");
  COMPARE_BOTH(Vmov(d14, RawbitsToDouble(0xe62556c325a59470)),
               "mov ip, #38000\n"
               "movt ip, #9637\n"
               "vdup.32 d14, ip\n"
               "mov ip, #22211\n"
               "movt ip, #58917\n"
               "vmov.32 d14[1], ip\n");
  CLEANUP();
}

TEST(macro_assembler_PushRegisterList) {
  SETUP();

  // Allow the test to use all registers.
  UseScratchRegisterScope temps(&masm);
  temps.ExcludeAll();

  COMPARE_BOTH(Push(RegisterList(0x1111)), "push {r0,r4,r8,ip}\n");

  COMPARE_BOTH(Push(RegisterList(0x1fff)),
               "push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip}\n");

  COMPARE_BOTH(Push(RegisterList(0x5fff)),
               "push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip,lr}\n");

  COMPARE_A32(Push(ne, RegisterList(0x1fff)),
              "pushne {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip}\n");

  COMPARE_T32(Push(ne, RegisterList(0x1fff)),
              "beq 0x00000006\n"
              "push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip}\n");

  COMPARE_A32(Push(RegisterList(sp)), "stmdb sp!, {sp}\n");

  // TODO: Clarify behaviour of MacroAssembler vs Assembler with respect to
  //       deprecated and unpredictable instructions. The tests reflect the
  //       current behaviour and will need to be updated.

  // Deprecated, but accepted:
  SHOULD_FAIL_TEST_A32(Push(RegisterList(pc)));
  // Whereas we don't accept the single-register version:
  MUST_FAIL_TEST_BOTH(Push(pc), "Unpredictable instruction.\n");

  // Accepted, but stores UNKNOWN value for the SP:
  SHOULD_FAIL_TEST_A32(Push(RegisterList(r0, sp)));

  // The following use the T1 and A1 encodings for T32 and A32 respectively, and
  // hence have different preferred disassembly.
  COMPARE_T32(Push(RegisterList(r0)), "push {r0}\n");
  COMPARE_A32(Push(RegisterList(r0)), "stmdb sp!, {r0}\n");
  COMPARE_T32(Push(RegisterList(r7)), "push {r7}\n");
  COMPARE_A32(Push(RegisterList(r7)), "stmdb sp!, {r7}\n");
  COMPARE_T32(Push(RegisterList(lr)), "push {lr}\n");
  COMPARE_A32(Push(RegisterList(lr)), "stmdb sp!, {lr}\n");

  // T2 and A1 encodings, with the same preferred disassembly:
  COMPARE_BOTH(Push(RegisterList(r8)), "stmdb sp!, {r8}\n");

  // Cannot push the sp and pc in T32 when using a register list.
  MUST_FAIL_TEST_T32(Push(RegisterList(sp)),
                     "Ill-formed 'push' instruction.\n");
  MUST_FAIL_TEST_T32(Push(RegisterList(pc)),
                     "Ill-formed 'push' instruction.\n");

  CLEANUP();
}

TEST(macro_assembler_PopRegisterList) {
  SETUP();

  // Allow the test to use all registers.
  UseScratchRegisterScope temps(&masm);
  temps.ExcludeAll();

  COMPARE_BOTH(Pop(RegisterList(0x1111)), "pop {r0,r4,r8,ip}\n");

  COMPARE_BOTH(Pop(RegisterList(0x1fff)),
               "pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip}\n");

  COMPARE_BOTH(Pop(RegisterList(0x5fff)),
               "pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip,lr}\n");

  COMPARE_A32(Pop(ne, RegisterList(0x1fff)),
              "popne {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip}\n");

  COMPARE_T32(Pop(ne, RegisterList(0x1fff)),
              "beq 0x00000006\n"
              "pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,ip}\n");

  // TODO: Accepted, but value of SP after the instruction is UNKNOWN:
  SHOULD_FAIL_TEST_A32(Pop(RegisterList(sp)));

  // Cannot pop the sp in T32 when using a register list.
  MUST_FAIL_TEST_T32(Pop(RegisterList(sp)), "Ill-formed 'pop' instruction.\n");

  // The following use the T1 and A1 encodings for T32 and A32 respectively, and
  // hence have different preferred disassembly.
  COMPARE_T32(Pop(RegisterList(pc)), "pop {pc}\n");
  COMPARE_A32(Pop(RegisterList(pc)), "ldm sp!, {pc}\n");
  COMPARE_T32(Pop(RegisterList(r0)), "pop {r0}\n");
  COMPARE_A32(Pop(RegisterList(r0)), "ldm sp!, {r0}\n");
  COMPARE_T32(Pop(RegisterList(r7)), "pop {r7}\n");
  COMPARE_A32(Pop(RegisterList(r7)), "ldm sp!, {r7}\n");

  // T2 and A1 encodings, with the same preferred disassembly:
  COMPARE_BOTH(Pop(RegisterList(r8)), "ldm sp!, {r8}\n");
  COMPARE_BOTH(Pop(RegisterList(lr)), "ldm sp!, {lr}\n");

  // TODO: Pushing both the lr and pc should not be allowed by the
  //       MacroAssembler (deprecated for A32, for T32 they shouldn't both
  //       be in the list).
  SHOULD_FAIL_TEST_BOTH(Pop(RegisterList(lr, pc)));

  CLEANUP();
}


TEST(macro_assembler_unpredictable) {
  SETUP();

  // ADC, ADCS (immediate).
  COMPARE_A32(Adc(pc, r0, 1), "adc pc, r0, #1\n");
  COMPARE_A32(Adc(r0, pc, 1), "adc r0, pc, #1\n");
  MUST_FAIL_TEST_T32(Adc(pc, r0, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adc(r0, pc, 1), "Unpredictable instruction.\n");
  COMPARE_A32(Adcs(pc, r0, 1), "adcs pc, r0, #1\n");
  COMPARE_A32(Adcs(r0, pc, 1), "adcs r0, pc, #1\n");
  MUST_FAIL_TEST_T32(Adcs(pc, r0, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adcs(r0, pc, 1), "Unpredictable instruction.\n");

  // ADC, ADCS (register).
  COMPARE_A32(Adc(pc, r0, r1), "adc pc, r0, r1\n");
  COMPARE_A32(Adc(r0, pc, r1), "adc r0, pc, r1\n");
  COMPARE_A32(Adc(r0, r1, pc), "adc r0, r1, pc\n");
  MUST_FAIL_TEST_T32(Adc(pc, r0, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adc(r0, pc, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adc(r0, r1, pc), "Unpredictable instruction.\n");
  COMPARE_A32(Adcs(pc, r0, r1), "adcs pc, r0, r1\n");
  COMPARE_A32(Adcs(r0, pc, r1), "adcs r0, pc, r1\n");
  COMPARE_A32(Adcs(r0, r1, pc), "adcs r0, r1, pc\n");
  MUST_FAIL_TEST_T32(Adcs(pc, r0, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adcs(r0, pc, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adcs(r0, r1, pc), "Unpredictable instruction.\n");

  // ADC, ADCS (register-shifted register).
  MUST_FAIL_TEST_A32(Adc(pc, r0, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adc(r0, pc, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adc(r0, r1, Operand(pc, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adc(r0, r1, Operand(r2, LSL, pc)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adcs(pc, r0, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adcs(r0, pc, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adcs(r0, r1, Operand(pc, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adcs(r0, r1, Operand(r2, LSL, pc)),
                     "Unpredictable instruction.\n");

  // ADD (immediate, to PC).
  COMPARE_A32(Add(r0, pc, 1), "adr r0, 0x00000009\n");
  COMPARE_T32(Add(r0, pc, 1), "adr r0, 0x00000005\n");
  COMPARE_A32(Add(pc, pc, 1), "adr pc, 0x00000009\n");
  MUST_FAIL_TEST_T32(Add(pc, pc, 1), "Unpredictable instruction.\n");

  // ADD, ADDS (immediate).
  COMPARE_A32(Add(pc, r0, 1), "add pc, r0, #1\n");
  MUST_FAIL_TEST_T32(Add(pc, r0, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Add(pc, r0, 0x123), "Unpredictable instruction.\n");
  COMPARE_A32(Adds(pc, r0, 1), "adds pc, r0, #1\n");
  COMPARE_A32(Adds(r0, pc, 1), "adds r0, pc, #1\n");
  // TODO: Try to make these error messages more consistent.
  MUST_FAIL_TEST_T32(Adds(r0, pc, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adds(r0, pc, 0x123), "Ill-formed 'adds' instruction.\n");

  // ADD, ADDS (register).
  COMPARE_A32(Add(pc, r0, r1), "add pc, r0, r1\n");
  COMPARE_A32(Add(r0, pc, r1), "add r0, pc, r1\n");
  COMPARE_A32(Add(r0, r1, pc), "add r0, r1, pc\n");
  COMPARE_T32(Add(r0, r0, pc), "add r0, pc\n");
  COMPARE_T32(Add(pc, pc, r0), "add pc, r0\n");
  MUST_FAIL_TEST_T32(Add(pc, pc, pc), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Add(pc, r0, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Add(r0, pc, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Add(r0, r1, pc), "Unpredictable instruction.\n");
  COMPARE_A32(Adds(pc, r0, r1), "adds pc, r0, r1\n");
  COMPARE_A32(Adds(r0, pc, r1), "adds r0, pc, r1\n");
  COMPARE_A32(Adds(r0, r1, pc), "adds r0, r1, pc\n");
  MUST_FAIL_TEST_T32(Adds(r0, pc, r1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Adds(r0, r1, pc), "Unpredictable instruction.\n");

  // ADD, ADDS (register-shifted register)
  MUST_FAIL_TEST_A32(Add(pc, r0, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, pc, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, r1, Operand(pc, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, r1, Operand(r2, LSL, pc)),
                     "Unpredictable instruction.\n");
  COMPARE_A32(Add(pc, sp, 1), "add pc, sp, #1\n");
  MUST_FAIL_TEST_T32(Add(pc, sp, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adds(pc, r0, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adds(r0, pc, Operand(r1, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adds(r0, r1, Operand(pc, LSL, r2)),
                     "Unpredictable instruction.\n");
  MUST_FAIL_TEST_A32(Adds(r0, r1, Operand(r2, LSL, pc)),
                     "Unpredictable instruction.\n");

  // ADD, ADDS (SP plus immediate).
  COMPARE_A32(Add(pc, sp, 1), "add pc, sp, #1\n");
  MUST_FAIL_TEST_T32(Add(pc, sp, 1), "Unpredictable instruction.\n");
  COMPARE_A32(Adds(pc, sp, 1), "adds pc, sp, #1\n");
  MUST_FAIL_TEST_T32(Adds(pc, sp, 1), "Ill-formed 'adds' instruction.\n");

  // ADD, ADDS (SP plus register).
  COMPARE_A32(Add(pc, sp, r0), "add pc, sp, r0\n");
  MUST_FAIL_TEST_T32(Add(pc, sp, r0), "Unpredictable instruction.\n");
  COMPARE_A32(Add(r0, sp, pc), "add r0, sp, pc\n");
  MUST_FAIL_TEST_T32(Add(r0, sp, pc), "Unpredictable instruction.\n");
  COMPARE_BOTH(Add(pc, sp, pc), "add pc, sp, pc\n");
  COMPARE_BOTH(Add(sp, sp, pc), "add sp, pc\n");
  COMPARE_A32(Adds(pc, sp, r0), "adds pc, sp, r0\n");
  MUST_FAIL_TEST_T32(Adds(pc, sp, r0), "Ill-formed 'adds' instruction.\n");
  COMPARE_A32(Adds(r0, sp, pc), "adds r0, sp, pc\n");
  MUST_FAIL_TEST_T32(Adds(r0, sp, pc), "Unpredictable instruction.\n");
  COMPARE_A32(Adds(pc, sp, pc), "adds pc, sp, pc\n");
  MUST_FAIL_TEST_T32(Adds(pc, sp, pc), "Ill-formed 'adds' instruction.\n");
  COMPARE_A32(Adds(sp, sp, pc), "adds sp, pc\n");
  MUST_FAIL_TEST_T32(Adds(sp, sp, pc), "Unpredictable instruction.\n");

  // ADR.
  {
    Literal<uint32_t> literal(0x12345678);
    // The address is 0x4 and not 0x0 because of the branch over the literal.
    // TODO: Consider disallowing this instruction.
    COMPARE_A32(Adr(pc, &literal), "adr pc, 0x00000004\n");
    MUST_FAIL_TEST_T32(Adr(pc, &literal), "Unpredictable instruction.\n");
  }

  // CLZ.
  MUST_FAIL_TEST_BOTH(Clz(pc, r0), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Clz(r0, pc), "Unpredictable instruction.\n");

  // MOV, MOVS (immediate).
  COMPARE_A32(Mov(pc, 1), "mov pc, #1\n");
  MUST_FAIL_TEST_T32(Mov(pc, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_T32(Mov(pc, 0xfff), "Unpredictable instruction.\n");
  COMPARE_A32(Mov(pc, 0xf000), "mov pc, #61440\n");
  MUST_FAIL_TEST_T32(Mov(pc, 0xf000), "Unpredictable instruction.\n");
  COMPARE_A32(Movs(pc, 1), "movs pc, #1\n");
  MUST_FAIL_TEST_T32(Movs(pc, 1), "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(pc, 0xfff), "Ill-formed 'movs' instruction.\n");
  COMPARE_A32(Movs(pc, 0xf000), "movs pc, #61440\n");
  MUST_FAIL_TEST_T32(Movs(pc, 0xf000), "Unpredictable instruction.\n");

  // MOV, MOVS (register).
  COMPARE_BOTH(Mov(pc, r0), "mov pc, r0\n");
  COMPARE_BOTH(Mov(r0, pc), "mov r0, pc\n");
  MUST_FAIL_TEST_BOTH(Movs(pc, r0), "Unpredictable instruction.\n");
  COMPARE_A32(Movs(r0, pc), "movs r0, pc\n");
  MUST_FAIL_TEST_T32(Movs(r0, pc), "Unpredictable instruction.\n");

  // MOV, MOVS (register-shifted register).
  MUST_FAIL_TEST_BOTH(Mov(pc, Operand(r0, ASR, r1)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Mov(r0, Operand(pc, ASR, r1)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Mov(r0, Operand(r1, ASR, pc)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(pc, Operand(r0, ASR, r1)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(r0, Operand(pc, ASR, r1)),
                      "Unpredictable instruction.\n");
  MUST_FAIL_TEST_BOTH(Movs(r0, Operand(r1, ASR, pc)),
                      "Unpredictable instruction.\n");

  CLEANUP();
}


TEST(macro_assembler_pc_rel_A32) {
  SETUP();
  // Simple cases alias adr.
  COMPARE_A32(Add(r0, pc, -8), "adr r0, 0x00000000\n");
  COMPARE_A32(Add(r0, pc, 255), "adr r0, 0x00000107\n");
  COMPARE_A32(Add(r0, pc, 256), "adr r0, 0x00000108\n");
  COMPARE_A32(Add(r0, pc, 1024), "adr r0, 0x00000408\n");
  COMPARE_A32(Add(r0, pc, -9), "adr r0, 0xffffffff\n");
  COMPARE_A32(Add(r0, pc, -1024), "adr r0, 0xfffffc08\n");
  COMPARE_A32(Add(r0, pc, UINT32_C(0x80000000)), "adr r0, 0x80000008\n");
  COMPARE_A32(Add(r0, pc, -0x7fffffff), "adr r0, 0x80000009\n");

  COMPARE_A32(Sub(r0, pc, 8), "adr r0, 0x00000000\n");
  COMPARE_A32(Sub(r0, pc, -255), "adr r0, 0x00000107\n");
  COMPARE_A32(Sub(r0, pc, -256), "adr r0, 0x00000108\n");
  COMPARE_A32(Sub(r0, pc, -1024), "adr r0, 0x00000408\n");
  COMPARE_A32(Sub(r0, pc, 9), "adr r0, 0xffffffff\n");
  COMPARE_A32(Sub(r0, pc, 1024), "adr r0, 0xfffffc08\n");
  COMPARE_A32(Sub(r0, pc, UINT32_C(0x80000000)), "adr r0, 0x80000008\n");
  COMPARE_A32(Sub(r0, pc, -0x7fffffff), "adr r0, 0x80000007\n");

  // Cases out of range.
  // Only negative offsets are supported, because the proper behaviour for
  // positive offsets is not clear.
  MUST_FAIL_TEST_A32(Add(r0, pc, 1025), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, pc, 0xffff), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, pc, 0x10001), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, pc, 0x12345678),
                     "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Add(r0, pc, 0x7fffffff),
                     "Ill-formed 'add' instruction.\n");
  COMPARE_A32(Add(r0, pc, -1025),
              "adr r0, 0x00000007\n"
              "sub r0, #1024\n");
  COMPARE_A32(Add(r0, pc, -0xffff),
              "adr r0, 0xffffff09\n"
              "sub r0, #65280\n");
  COMPARE_A32(Add(r0, pc, -0x10001),
              "adr r0, 0x00000007\n"
              "sub r0, #65536\n");
  COMPARE_A32(Add(r0, pc, -0x2345678),
              "adr r0, 0xfffffd90\n"
              "sub r0, #21504\n"
              "sub r0, #36962304\n");
  COMPARE_A32(Add(r0, pc, -0x12345678),
              "adr r0, 0xfffffd90\n"
              "mov ip, #21504\n"
              "movt ip, #4660\n"
              "sub r0, ip\n");

  MUST_FAIL_TEST_A32(Sub(r0, pc, -1025), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Sub(r0, pc, -0xffff), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Sub(r0, pc, -0x10001), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_A32(Sub(r0, pc, -0x12345678),
                     "Ill-formed 'add' instruction.\n");
  COMPARE_A32(Sub(r0, pc, 1025),
              "adr r0, 0x00000007\n"
              "sub r0, #1024\n");
  COMPARE_A32(Sub(r0, pc, 0xffff),
              "adr r0, 0xffffff09\n"
              "sub r0, #65280\n");
  COMPARE_A32(Sub(r0, pc, 0x10001),
              "adr r0, 0x00000007\n"
              "sub r0, #65536\n");
  COMPARE_A32(Sub(r0, pc, 0x2345678),
              "adr r0, 0xfffffd90\n"
              "sub r0, #21504\n"
              "sub r0, #36962304\n");
  COMPARE_A32(Sub(r0, pc, 0x12345678),
              "adr r0, 0xfffffd90\n"
              "mov ip, #21504\n"
              "movt ip, #4660\n"
              "sub r0, ip\n");
  COMPARE_A32(Sub(r0, pc, 0x7fffffff),
              "adr r0, 0xffffff09\n"
              "add r0, #256\n"
              "add r0, #2147483648\n");

  CLEANUP();
}


TEST(macro_assembler_pc_rel_T32) {
  SETUP();
  // Simple cases alias adr.
  COMPARE_T32(Add(r0, pc, -4), "adr r0, 0x00000000\n");     // T1
  COMPARE_T32(Add(r0, pc, 1020), "adr r0, 0x00000400\n");   // T1
  COMPARE_T32(Add(r0, pc, -5), "adr r0, 0xffffffff\n");     // T2
  COMPARE_T32(Add(r0, pc, -4095), "adr r0, 0xfffff005\n");  // T2
  COMPARE_T32(Add(r0, pc, -3), "adr r0, 0x00000001\n");     // T3
  COMPARE_T32(Add(r0, pc, 1021), "adr r0, 0x00000401\n");   // T3
  COMPARE_T32(Add(r0, pc, 1019), "adr r0, 0x000003ff\n");   // T3
  COMPARE_T32(Add(r0, pc, 4095), "adr r0, 0x00001003\n");   // T3

  COMPARE_T32(Sub(r0, pc, 4), "adr r0, 0x00000000\n");      // T1
  COMPARE_T32(Sub(r0, pc, -1020), "adr r0, 0x00000400\n");  // T1
  COMPARE_T32(Sub(r0, pc, 5), "adr r0, 0xffffffff\n");      // T2
  COMPARE_T32(Sub(r0, pc, 4095), "adr r0, 0xfffff005\n");   // T2
  COMPARE_T32(Sub(r0, pc, 3), "adr r0, 0x00000001\n");      // T3
  COMPARE_T32(Sub(r0, pc, -1021), "adr r0, 0x00000401\n");  // T3
  COMPARE_T32(Sub(r0, pc, -1019), "adr r0, 0x000003ff\n");  // T3
  COMPARE_T32(Sub(r0, pc, -4095), "adr r0, 0x00001003\n");  // T3

  // Cases out of range.
  // Only negative offsets are supported, because the proper behaviour for
  // positive offsets is not clear.

  MUST_FAIL_TEST_T32(Add(r0, pc, 4096), "Unpredictable instruction.\n");

  MUST_FAIL_TEST_T32(Add(r0, pc, -4096), "Unpredictable instruction.\n");

  MUST_FAIL_TEST_T32(Add(r0, pc, 0xffff), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_T32(Add(r0, pc, 0x10002), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_T32(Add(r0, pc, 0x12345678),
                     "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_T32(Add(r0, pc, 0x7fffffff),
                     "Ill-formed 'add' instruction.\n");
  COMPARE_T32(Add(r0, pc, -0x12345678),
              "mov r0, pc\n"
              "mov ip, #22136\n"
              "movt ip, #4660\n"
              "sub r0, ip\n");
  COMPARE_T32(Add(r0, pc, -0x7fffffff),
              "mov r0, pc\n"
              "add r0, #1\n"
              "add r0, #2147483648\n");

  // TODO: This test aborts in the Assembler (with unpredictable instruction
  // errors) before the MacroAssembler gets a chance to do something
  // predictable.
  // COMPARE_T32(Sub(r0, pc, -4096), "mov r0, pc\n"
  //                                 "add r0, #4096\n");

  MUST_FAIL_TEST_T32(Sub(r0, pc, 4096), "Unpredictable instruction.\n");

  MUST_FAIL_TEST_T32(Sub(r0, pc, -0xffff), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_T32(Sub(r0, pc, -0x10002), "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_T32(Sub(r0, pc, -0x12345678),
                     "Ill-formed 'add' instruction.\n");
  MUST_FAIL_TEST_T32(Sub(r0, pc, -0x7fffffff),
                     "Ill-formed 'add' instruction.\n");
  COMPARE_T32(Sub(r0, pc, 0x12345678),
              "mov r0, pc\n"
              "mov ip, #22136\n"
              "movt ip, #4660\n"
              "sub r0, ip\n");
  COMPARE_T32(Sub(r0, pc, 0x7fffffff),
              "mov r0, pc\n"
              "add r0, #1\n"
              "add r0, #2147483648\n");
  CLEANUP();
}


TEST(macro_assembler_unsupported) {
  SETUP();

  MUST_FAIL_TEST_BOTH(Sxtab(r0, r1, Operand(r2, ROR, 1)),
                      "Ill-formed 'sxtab' instruction.\n");
  MUST_FAIL_TEST_BOTH(Sxtab16(r0, r1, Operand(r0, ASR, 2)),
                      "Ill-formed 'sxtab16' instruction.\n");
  MUST_FAIL_TEST_BOTH(Sxtah(r0, r1, Operand(r0, LSL, r1)),
                      "Ill-formed 'sxtah' instruction.\n");
  MUST_FAIL_TEST_BOTH(Uxtab(r0, r1, Operand(r0, LSR, r2)),
                      "Ill-formed 'uxtab' instruction.\n");
  MUST_FAIL_TEST_BOTH(Uxtab16(r0, r1, Operand(r0, ROR, 1)),
                      "Ill-formed 'uxtab16' instruction.\n");
  MUST_FAIL_TEST_BOTH(Uxtah(r0, r1, Operand(r0, ASR, 2)),
                      "Ill-formed 'uxtah' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pkhbt(r0, r1, Operand(r0, LSL, r1)),
                      "Ill-formed 'pkhbt' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pkhtb(r0, r1, Operand(r0, LSR, r2)),
                      "Ill-formed 'pkhtb' instruction.\n");

  MUST_FAIL_TEST_BOTH(Pld(MemOperand(r0, 1, PreIndex)),
                      "Ill-formed 'pld' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pldw(MemOperand(r0, 1, PostIndex)),
                      "Ill-formed 'pldw' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pli(MemOperand(r0, 1, PreIndex)),
                      "Ill-formed 'pli' instruction.\n");

  MUST_FAIL_TEST_BOTH(Pld(MemOperand(r0, r0, PreIndex)),
                      "Ill-formed 'pld' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pldw(MemOperand(r0, r1, PostIndex)),
                      "Ill-formed 'pldw' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pli(MemOperand(r0, r2, PreIndex)),
                      "Ill-formed 'pli' instruction.\n");

  MUST_FAIL_TEST_BOTH(Pld(MemOperand(r0, r0, LSL, 1, PreIndex)),
                      "Ill-formed 'pld' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pldw(MemOperand(r0, r1, LSR, 2, PostIndex)),
                      "Ill-formed 'pldw' instruction.\n");
  MUST_FAIL_TEST_BOTH(Pli(MemOperand(r0, r2, ASR, 3, PreIndex)),
                      "Ill-formed 'pli' instruction.\n");

  MUST_FAIL_TEST_BOTH(Lda(r0, MemOperand(r0, 1)),
                      "Ill-formed 'lda' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldab(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldab' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldaex(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldaex' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldaexb(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldaexb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldaexh(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldaexh' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldah(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldah' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldrex(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldrex' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldrexb(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldrexb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldrexh(r0, MemOperand(r0, 1)),
                      "Ill-formed 'ldrexh' instruction.\n");
  MUST_FAIL_TEST_BOTH(Stl(r0, MemOperand(r0, 1)),
                      "Ill-formed 'stl' instruction.\n");
  MUST_FAIL_TEST_BOTH(Stlb(r0, MemOperand(r0, 1)),
                      "Ill-formed 'stlb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Stlh(r0, MemOperand(r0, 1)),
                      "Ill-formed 'stlh' instruction.\n");

  MUST_FAIL_TEST_BOTH(Ldaexd(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'ldaexd' instruction.\n");
  MUST_FAIL_TEST_BOTH(Ldrexd(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'ldrexd' instruction.\n");
  MUST_FAIL_TEST_BOTH(Stlex(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'stlex' instruction.\n");
  MUST_FAIL_TEST_BOTH(Stlexb(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'stlexb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Stlexh(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'stlexh' instruction.\n");
  MUST_FAIL_TEST_BOTH(Strex(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'strex' instruction.\n");
  MUST_FAIL_TEST_BOTH(Strexb(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'strexb' instruction.\n");
  MUST_FAIL_TEST_BOTH(Strexh(r0, r1, MemOperand(r0, 1)),
                      "Ill-formed 'strexh' instruction.\n");

  MUST_FAIL_TEST_BOTH(Stlexd(r0, r1, r2, MemOperand(r0, 1)),
                      "Ill-formed 'stlexd' instruction.\n");
  MUST_FAIL_TEST_BOTH(Strexd(r0, r1, r2, MemOperand(r0, 1)),
                      "Ill-formed 'strexd' instruction.\n");

  CLEANUP();
}

TEST(macro_assembler_Vmov_neon_immediate) {
  SETUP();

  // Move 8, 16 and 32-bit immediates into D registers, duplicated across the
  // destination.
  COMPARE_BOTH(Vmov(I8, d0, 0xac), "vmov.i8 d0, #172\n");

  COMPARE_BOTH(Vmov(I16, d0, 0xa4), "vmov.i16 d0, #164\n");
  COMPARE_BOTH(Vmov(I16, d0, 0x9797), "vmov.i8 d0, #151\n");
  COMPARE_BOTH(Vmov(I16, d0, 0x9ef6),
               "mov ip, #40694\n"
               "vdup.16 d0, ip\n");

  COMPARE_BOTH(Vmov(I32, d0, 0x6d0000), "vmov.i32 d0, #7143424\n");
  COMPARE_BOTH(Vmov(I32, d0, 0x15ffffff), "vmvn.i32 d0, #3925868544\n");
  COMPARE_BOTH(Vmov(I32, d0, 0x74747474), "vmov.i8 d0, #116\n");
  COMPARE_BOTH(Vmov(I32, d0, 0xff0000ff), "vmov.i64 d0, #0xff0000ffff0000ff\n");
  COMPARE_BOTH(Vmov(I32, d0, 0x1ecb9ef6),
               "mov ip, #40694\n"
               "movt ip, #7883\n"
               "vdup.32 d0, ip\n");
  COMPARE_BOTH(Vmov(I32, d0, 0x006d0000), "vmov.i32 d0, #7143424\n");
  COMPARE_BOTH(Vmov(I32, d0, 0x00004da6),
               "mov ip, #19878\n"
               "vdup.32 d0, ip\n");
  COMPARE_BOTH(Vmov(I32, d0, 0xffffff55), "vmvn.i32 d0, #170\n");
  COMPARE_BOTH(Vmov(I32, d0, 0xffff55ff), "vmvn.i32 d0, #43520\n");
  COMPARE_BOTH(Vmov(I32, d0, 0xff55ffff), "vmvn.i32 d0, #11141120\n");

  COMPARE_BOTH(Vmov(I64, d0, UINT64_C(0xa5a5a5a5a5a5a5a5)),
               "vmov.i8 d0, #165\n");
  COMPARE_BOTH(Vmov(I64, d0, UINT64_C(0x0a01248315ffffff)),
               "mvn ip, #3925868544\n"
               "vdup.32 d0, ip\n"
               "mov ip, #9347\n"
               "movt ip, #2561\n"
               "vmov.32 d0[1], ip\n");
  COMPARE_BOTH(Vmov(I64, d0, UINT64_C(0x6fe1a7a779e33af2)),
               "mov ip, #15090\n"
               "movt ip, #31203\n"
               "vdup.32 d0, ip\n"
               "mov ip, #42919\n"
               "movt ip, #28641\n"
               "vmov.32 d0[1], ip\n");
  COMPARE_BOTH(Vmov(I64, d0, UINT64_C(0x2efa8b440000c1da)),
               "mov ip, #49626\n"
               "vdup.32 d0, ip\n"
               "mov ip, #35652\n"
               "movt ip, #12026\n"
               "vmov.32 d0[1], ip\n");
  COMPARE_BOTH(Vmov(I64, d0, UINT64_C(0x00008bb75c3036fd)),
               "mov ip, #14077\n"
               "movt ip, #23600\n"
               "vdup.32 d0, ip\n"
               "mov ip, #35767\n"
               "vmov.32 d0[1], ip\n");

  COMPARE_BOTH(Vmov(F32, d0, 0.5), "vmov.f32 d0, #0.5\n");
  COMPARE_BOTH(Vmov(F32, d0, 1.1),
               "mov ip, #52429\n"
               "movt ip, #16268\n"
               "vdup.32 d0, ip\n");

  COMPARE_T32(Vmov(I64, d0, UINT64_C(0x2fff2fff3e2869e7)),
              "mov ip, #27111\n"
              "movt ip, #15912\n"
              "vdup.32 d0, ip\n"
              "mvn ip, #3489714176\n"
              "vmov.32 d0[1], ip\n");

  COMPARE_A32(Vmov(I32, d0, 0x0ffffffa),
              "mvn ip, #4026531845\n"
              "vdup.32 d0, ip\n");
  COMPARE_A32(Vmov(I64, d0, UINT64_C(0x65ffffff16a0ef46)),
              "mov ip, #61254\n"
              "movt ip, #5792\n"
              "vdup.32 d0, ip\n"
              "mvn ip, #2583691264\n"
              "vmov.32 d0[1], ip\n");

  // Move 8, 16 and 32-bit immediates into Q registers, duplicated across the
  // destination.
  COMPARE_BOTH(Vmov(I8, q0, 0xac), "vmov.i8 q0, #172\n");

  COMPARE_BOTH(Vmov(I16, q0, 0xa4), "vmov.i16 q0, #164\n");
  COMPARE_BOTH(Vmov(I16, q0, 0x9797), "vmov.i8 q0, #151\n");
  COMPARE_BOTH(Vmov(I16, q0, 0x9ef6),
               "mov ip, #40694\n"
               "vdup.16 q0, ip\n");

  COMPARE_BOTH(Vmov(I32, q0, 0x6d0000), "vmov.i32 q0, #7143424\n");
  COMPARE_BOTH(Vmov(I32, q0, 0x15ffffff), "vmvn.i32 q0, #3925868544\n");
  COMPARE_BOTH(Vmov(I32, q0, 0x74747474), "vmov.i8 q0, #116\n");
  COMPARE_BOTH(Vmov(I32, q0, 0xff0000ff), "vmov.i64 q0, #0xff0000ffff0000ff\n");
  COMPARE_BOTH(Vmov(I32, q0, 0x1ecb9ef6),
               "mov ip, #40694\n"
               "movt ip, #7883\n"
               "vdup.32 q0, ip\n");
  COMPARE_BOTH(Vmov(I32, q0, 0x006d0000), "vmov.i32 q0, #7143424\n");
  COMPARE_BOTH(Vmov(I32, q0, 0x00004da6),
               "mov ip, #19878\n"
               "vdup.32 q0, ip\n");

  COMPARE_BOTH(Vmov(I64, q0, UINT64_C(0xa5a5a5a5a5a5a5a5)),
               "vmov.i8 q0, #165\n");
  COMPARE_BOTH(Vmov(I64, q0, UINT64_C(0x0a01248315ffffff)),
               "mvn ip, #3925868544\n"
               "vdup.32 q0, ip\n"
               "mov ip, #9347\n"
               "movt ip, #2561\n"
               "vmov.32 d0[1], ip\n"
               "vmov.f64 d1, d0\n");
  COMPARE_BOTH(Vmov(I64, q0, UINT64_C(0x6fe1a7a779e33af2)),
               "mov ip, #15090\n"
               "movt ip, #31203\n"
               "vdup.32 q0, ip\n"
               "mov ip, #42919\n"
               "movt ip, #28641\n"
               "vmov.32 d0[1], ip\n"
               "vmov.f64 d1, d0\n");
  COMPARE_BOTH(Vmov(I64, q0, UINT64_C(0x2efa8b440000c1da)),
               "mov ip, #49626\n"
               "vdup.32 q0, ip\n"
               "mov ip, #35652\n"
               "movt ip, #12026\n"
               "vmov.32 d0[1], ip\n"
               "vmov.f64 d1, d0\n");
  COMPARE_BOTH(Vmov(I64, q0, UINT64_C(0x00008bb75c3036fd)),
               "mov ip, #14077\n"
               "movt ip, #23600\n"
               "vdup.32 q0, ip\n"
               "mov ip, #35767\n"
               "vmov.32 d0[1], ip\n"
               "vmov.f64 d1, d0\n");

  COMPARE_BOTH(Vmov(F32, q0, 0.5), "vmov.f32 q0, #0.5\n");
  COMPARE_BOTH(Vmov(F32, q0, 1.1),
               "mov ip, #52429\n"
               "movt ip, #16268\n"
               "vdup.32 q0, ip\n");
  COMPARE_BOTH(Vmov(F64, q0, 0.5),
               "vmov.f64 d0, #0.5\n"
               "vmov.f64 d1, d0\n");
  COMPARE_BOTH(Vmov(F64, q0, 1.1),
               "mov ip, #39322\n"
               "movt ip, #39321\n"
               "vdup.32 d0, ip\n"
               "mov ip, #39321\n"
               "movt ip, #16369\n"
               "vmov.32 d0[1], ip\n"
               "vmov.f64 d1, d0\n");

  COMPARE_T32(Vmov(I64, q0, UINT64_C(0x2fff2fff3e2869e7)),
              "mov ip, #27111\n"
              "movt ip, #15912\n"
              "vdup.32 q0, ip\n"
              "mvn ip, #3489714176\n"
              "vmov.32 d0[1], ip\n"
              "vmov.f64 d1, d0\n");

  COMPARE_A32(Vmov(I32, q0, 0x0ffffffa),
              "mvn ip, #4026531845\n"
              "vdup.32 q0, ip\n");
  COMPARE_A32(Vmov(I64, q0, UINT64_C(0x65ffffff16a0ef46)),
              "mov ip, #61254\n"
              "movt ip, #5792\n"
              "vdup.32 q0, ip\n"
              "mvn ip, #2583691264\n"
              "vmov.32 d0[1], ip\n"
              "vmov.f64 d1, d0\n");
  CLEANUP();
}

TEST(macro_assembler_T32_IT) {
  SETUP();

  // ADC (register) T1
  COMPARE_T32(Adc(eq, r0, r0, r1),
              "it eq\n"
              "adceq r0, r1\n");

  COMPARE_T32(Adc(eq, r0, r1, r2),
              "bne 0x00000006\n"
              "adc r0, r1, r2\n");

  // ADD (immediate) T1
  COMPARE_T32(Add(eq, r0, r1, 0x1),
              "it eq\n"
              "addeq r0, r1, #1\n");

  COMPARE_T32(Add(eq, r0, r1, 0x8),
              "bne 0x00000006\n"
              "add r0, r1, #8\n");

  // ADD (immediate) T2
  COMPARE_T32(Add(eq, r0, r0, 0xff),
              "it eq\n"
              "addeq r0, #255\n");

  // ADD (register) T1
  COMPARE_T32(Add(eq, r0, r1, r7),
              "it eq\n"
              "addeq r0, r1, r7\n");

  // ADD (register) T2
  COMPARE_T32(Add(eq, r5, r5, r8),
              "it eq\n"
              "addeq r5, r8\n");

  // ADD (SP plus immediate) T1
  COMPARE_T32(Add(eq, r7, sp, 1020),
              "it eq\n"
              "addeq r7, sp, #1020\n");

  COMPARE_T32(Add(eq, r7, sp, 1),
              "bne 0x00000006\n"
              "add r7, sp, #1\n");

  COMPARE_T32(Add(eq, r7, sp, 1024),
              "bne 0x00000006\n"
              "add r7, sp, #1024\n");

  COMPARE_T32(Add(eq, sp, sp, 32),
              "bne 0x00000004\n"
              "add sp, #32\n");

  // ADD (SP plus register) T1
  COMPARE_T32(Add(eq, r7, sp, r7),
              "it eq\n"
              "addeq r7, sp, r7\n");

  // ADD (SP plus register) T2
  COMPARE_T32(Add(eq, sp, sp, r10),
              "it eq\n"
              "addeq sp, r10\n");

  COMPARE_T32(Add(eq, r5, r5, sp),
              "bne 0x00000006\n"
              "add.w r5, sp\n");

  // AND (register) T1
  COMPARE_T32(And(eq, r7, r7, r0),
              "it eq\n"
              "andeq r7, r0\n");

  COMPARE_T32(And(eq, r8, r8, r0),
              "bne 0x00000006\n"
              "and r8, r0\n");

  // ASR (immediate) T2
  COMPARE_T32(Asr(eq, r0, r1, 16),
              "it eq\n"
              "asreq r0, r1, #16\n");

  COMPARE_T32(Asr(eq, r0, r1, 32),
              "it eq\n"
              "asreq r0, r1, #32\n");

  COMPARE_T32(Asr(eq, r0, r1, 0),
              "bne 0x0000000a\n"
              "mov r0, #0\n"
              "asr r0, r1, r0\n");

  // ASR (register) T1
  COMPARE_T32(Asr(eq, r7, r7, r3),
              "it eq\n"
              "asreq r7, r3\n");

  COMPARE_T32(Asr(eq, r8, r8, r3),
              "bne 0x00000006\n"
              "asr r8, r3\n");

  // BIC (register) T1
  COMPARE_T32(Bic(eq, r7, r7, r6),
              "it eq\n"
              "biceq r7, r6\n");

  COMPARE_T32(Bic(eq, r8, r8, r6),
              "bne 0x00000006\n"
              "bic r8, r6\n");

  Label l;
  __ Bind(&l);

  // BLX (register) T1
  COMPARE_T32(Blx(eq, lr),
              "it eq\n"
              "blxeq lr\n");
  COMPARE_T32(Blx(eq, &l),
              "bne 0x00000006\n"
              "blx 0x00000000\n");

  // BX (register) T1
  COMPARE_T32(Bx(eq, lr),
              "it eq\n"
              "bxeq lr\n");

  // CMN (register) T1
  COMPARE_T32(Cmn(eq, r0, r1),
              "it eq\n"
              "cmneq r0, r1\n");

  COMPARE_T32(Cmn(eq, r0, r8),
              "bne 0x00000006\n"
              "cmn r0, r8\n");

  // CMP (immediate) T1
  COMPARE_T32(Cmp(eq, r7, 0xff),
              "it eq\n"
              "cmpeq r7, #255\n");

  // CMP (register) T1
  COMPARE_T32(Cmp(eq, r6, r7),
              "it eq\n"
              "cmpeq r6, r7\n");

  // CMP (register) T2
  COMPARE_T32(Cmp(eq, r9, r10),
              "it eq\n"
              "cmpeq r9, r10\n");

  COMPARE_T32(Cmp(eq, r0, 0x100),
              "bne 0x00000006\n"
              "cmp r0, #256\n");

  // EOR (register) T1
  COMPARE_T32(Eor(eq, r0, r0, r7),
              "it eq\n"
              "eoreq r0, r7\n");

  COMPARE_T32(Eor(eq, r0, r0, 0x1),
              "bne 0x00000006\n"
              "eor r0, #0x1\n");

  // LDR (immediate) T1
  COMPARE_T32(Ldr(eq, r4, MemOperand(r7, 124)),
              "it eq\n"
              "ldreq r4, [r7, #124]\n");

  COMPARE_T32(Ldr(eq, r4, MemOperand(r7, 1)),
              "bne 0x00000006\n"
              "ldr r4, [r7, #1]\n");

  COMPARE_T32(Ldr(eq, r4, MemOperand(r7, 128)),
              "bne 0x00000006\n"
              "ldr r4, [r7, #128]\n");

  // LDR (immediate) T2
  COMPARE_T32(Ldr(eq, r4, MemOperand(sp, 1020)),
              "it eq\n"
              "ldreq r4, [sp, #1020]\n");

  COMPARE_T32(Ldr(eq, r4, MemOperand(sp, 1)),
              "bne 0x00000006\n"
              "ldr r4, [sp, #1]\n");

  COMPARE_T32(Ldr(eq, r4, MemOperand(sp, 1024)),
              "bne 0x00000006\n"
              "ldr r4, [sp, #1024]\n");

  // LDR (register) T1
  COMPARE_T32(Ldr(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "ldreq r5, [r6, r7]\n");

  COMPARE_T32(Ldr(eq, r5, MemOperand(r6, r8)),
              "bne 0x00000006\n"
              "ldr r5, [r6, r8]\n");

  // LDRB (immediate) T1
  COMPARE_T32(Ldrb(eq, r6, MemOperand(r7, 31)),
              "it eq\n"
              "ldrbeq r6, [r7, #31]\n");

  COMPARE_T32(Ldrb(eq, r6, MemOperand(r7, 32)),
              "bne 0x00000006\n"
              "ldrb r6, [r7, #32]\n");

  // LDRB (register) T1
  COMPARE_T32(Ldrb(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "ldrbeq r5, [r6, r7]\n");

  COMPARE_T32(Ldrb(eq, r6, MemOperand(r9)),
              "bne 0x00000006\n"
              "ldrb r6, [r9]\n");

  // LDRH (immediate) T1
  COMPARE_T32(Ldrh(eq, r6, MemOperand(r7, 62)),
              "it eq\n"
              "ldrheq r6, [r7, #62]\n");

  COMPARE_T32(Ldrh(eq, r6, MemOperand(r7, 64)),
              "bne 0x00000006\n"
              "ldrh r6, [r7, #64]\n");

  COMPARE_T32(Ldrh(eq, r6, MemOperand(r7, 1)),
              "bne 0x00000006\n"
              "ldrh r6, [r7, #1]\n");

  // LDRH (register) T1
  COMPARE_T32(Ldrh(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "ldrheq r5, [r6, r7]\n");

  COMPARE_T32(Ldrh(eq, r6, MemOperand(r9)),
              "bne 0x00000006\n"
              "ldrh r6, [r9]\n");

  // LDRSB (register) T1
  COMPARE_T32(Ldrsb(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "ldrsbeq r5, [r6, r7]\n");

  COMPARE_T32(Ldrsb(eq, r6, MemOperand(r9)),
              "bne 0x00000006\n"
              "ldrsb r6, [r9]\n");

  // LDRSH (register) T1
  COMPARE_T32(Ldrsh(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "ldrsheq r5, [r6, r7]\n");

  COMPARE_T32(Ldrsh(eq, r6, MemOperand(r9)),
              "bne 0x00000006\n"
              "ldrsh r6, [r9]\n");

  // LSL (immediate) T2
  COMPARE_T32(Lsl(eq, r0, r1, 16),
              "it eq\n"
              "lsleq r0, r1, #16\n");

  COMPARE_T32(Lsl(eq, r0, r1, 0),
              "bne 0x0000000a\n"
              "mov r0, #0\n"
              "lsl r0, r1, r0\n");

  COMPARE_T32(Lsl(eq, r0, r1, 32),
              "bne 0x0000000a\n"
              "mov r0, #32\n"
              "lsl r0, r1, r0\n");

  // LSL (register) T1
  COMPARE_T32(Lsl(eq, r7, r7, r3),
              "it eq\n"
              "lsleq r7, r3\n");

  COMPARE_T32(Lsl(eq, r8, r8, r3),
              "bne 0x00000006\n"
              "lsl r8, r3\n");

  // LSR (immediate) T2
  COMPARE_T32(Lsr(eq, r0, r1, 16),
              "it eq\n"
              "lsreq r0, r1, #16\n");

  COMPARE_T32(Lsr(eq, r0, r1, 32),
              "it eq\n"
              "lsreq r0, r1, #32\n");

  COMPARE_T32(Lsr(eq, r0, r1, 0),
              "bne 0x0000000a\n"
              "mov r0, #0\n"
              "lsr r0, r1, r0\n");

  // LSR (register) T1
  COMPARE_T32(Lsr(eq, r7, r7, r3),
              "it eq\n"
              "lsreq r7, r3\n");

  COMPARE_T32(Lsr(eq, r8, r8, r3),
              "bne 0x00000006\n"
              "lsr r8, r3\n");

  // MOV (immediate) T1
  COMPARE_T32(Mov(eq, r7, 0xff),
              "it eq\n"
              "moveq r7, #255\n");

  // MOV (register) T1
  COMPARE_T32(Mov(eq, r9, r8),
              "it eq\n"
              "moveq r9, r8\n");

  // MOV (register) T2
  COMPARE_T32(Mov(eq, r0, Operand(r1, LSR, 16)),
              "it eq\n"
              "lsreq r0, r1, #16\n");

  COMPARE_T32(Mov(eq, r0, Operand(r1, ROR, 16)),
              "bne 0x00000006\n"
              "ror r0, r1, #16\n");

  // MOV (register-shifted register) T1
  COMPARE_T32(Mov(eq, r0, Operand(r0, LSR, r1)),
              "it eq\n"
              "lsreq r0, r1\n");

  COMPARE_T32(Mov(eq, r0, Operand(r1, LSR, r2)),
              "bne 0x00000006\n"
              "lsr r0, r1, r2\n");

  // MUL (T1)
  COMPARE_T32(Mul(eq, r0, r1, r0),
              "it eq\n"
              "muleq r0, r1, r0\n");

  COMPARE_T32(Mul(eq, r0, r1, r2),
              "bne 0x00000006\n"
              "mul r0, r1, r2\n");

  // MVN (register) T1
  COMPARE_T32(Mvn(eq, r4, r6),
              "it eq\n"
              "mvneq r4, r6\n");

  COMPARE_T32(Mvn(eq, r8, r6),
              "bne 0x00000006\n"
              "mvn r8, r6\n");

  // ORR (register) T1
  COMPARE_T32(Orr(eq, r0, r0, r1),
              "it eq\n"
              "orreq r0, r1\n");

  COMPARE_T32(Orr(eq, r0, r1, r2),
              "bne 0x00000006\n"
              "orr r0, r1, r2\n");

  // ROR (register) T1
  COMPARE_T32(Ror(eq, r7, r7, r3),
              "it eq\n"
              "roreq r7, r3\n");

  COMPARE_T32(Ror(eq, r8, r8, r3),
              "bne 0x00000006\n"
              "ror r8, r3\n");

  COMPARE_T32(Ror(eq, r0, r1, 16),
              "bne 0x00000006\n"
              "ror r0, r1, #16\n");

  // RSB (immediate) T1
  COMPARE_T32(Rsb(eq, r0, r1, 0),
              "it eq\n"
              "rsbeq r0, r1, #0\n");

  COMPARE_T32(Rsb(eq, r0, r1, 1),
              "bne 0x00000006\n"
              "rsb r0, r1, #1\n");

  // SBC (register) T1
  COMPARE_T32(Sbc(eq, r0, r0, r1),
              "it eq\n"
              "sbceq r0, r1\n");

  COMPARE_T32(Sbc(eq, r0, r1, r2),
              "bne 0x00000006\n"
              "sbc r0, r1, r2\n");

  // STR (immediate) T1
  COMPARE_T32(Str(eq, r4, MemOperand(r7, 124)),
              "it eq\n"
              "streq r4, [r7, #124]\n");

  COMPARE_T32(Str(eq, r4, MemOperand(r7, 1)),
              "bne 0x00000006\n"
              "str r4, [r7, #1]\n");

  COMPARE_T32(Str(eq, r4, MemOperand(r7, 128)),
              "bne 0x00000006\n"
              "str r4, [r7, #128]\n");

  // STR (immediate) T2
  COMPARE_T32(Str(eq, r4, MemOperand(sp, 1020)),
              "it eq\n"
              "streq r4, [sp, #1020]\n");

  COMPARE_T32(Str(eq, r4, MemOperand(sp, 1)),
              "bne 0x00000006\n"
              "str r4, [sp, #1]\n");

  COMPARE_T32(Str(eq, r4, MemOperand(sp, 1024)),
              "bne 0x00000006\n"
              "str r4, [sp, #1024]\n");

  // STR (register) T1
  COMPARE_T32(Str(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "streq r5, [r6, r7]\n");

  COMPARE_T32(Str(eq, r5, MemOperand(r6, r8)),
              "bne 0x00000006\n"
              "str r5, [r6, r8]\n");

  // STRB (immediate) T1
  COMPARE_T32(Strb(eq, r6, MemOperand(r7, 31)),
              "it eq\n"
              "strbeq r6, [r7, #31]\n");

  COMPARE_T32(Strb(eq, r6, MemOperand(r7, 32)),
              "bne 0x00000006\n"
              "strb r6, [r7, #32]\n");

  // STRB (register) T1
  COMPARE_T32(Strb(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "strbeq r5, [r6, r7]\n");

  COMPARE_T32(Strb(eq, r6, MemOperand(r9)),
              "bne 0x00000006\n"
              "strb r6, [r9]\n");

  // STRH (immediate) T1
  COMPARE_T32(Strh(eq, r6, MemOperand(r7, 62)),
              "it eq\n"
              "strheq r6, [r7, #62]\n");

  COMPARE_T32(Strh(eq, r6, MemOperand(r7, 64)),
              "bne 0x00000006\n"
              "strh r6, [r7, #64]\n");

  COMPARE_T32(Strh(eq, r6, MemOperand(r7, 1)),
              "bne 0x00000006\n"
              "strh r6, [r7, #1]\n");

  // STRH (register) T1
  COMPARE_T32(Strh(eq, r5, MemOperand(r6, r7)),
              "it eq\n"
              "strheq r5, [r6, r7]\n");

  COMPARE_T32(Strh(eq, r6, MemOperand(r9)),
              "bne 0x00000006\n"
              "strh r6, [r9]\n");

  // SUB (immediate) T1
  COMPARE_T32(Sub(eq, r0, r1, 0x1),
              "it eq\n"
              "subeq r0, r1, #1\n");

  COMPARE_T32(Sub(eq, r0, r1, 0x8),
              "bne 0x00000006\n"
              "sub r0, r1, #8\n");

  // SUB (immediate) T2
  COMPARE_T32(Sub(eq, r0, r0, 0xff),
              "it eq\n"
              "subeq r0, #255\n");

  // SUB (register) T1
  COMPARE_T32(Sub(eq, r0, r1, r7),
              "it eq\n"
              "subeq r0, r1, r7\n");

  COMPARE_T32(Sub(eq, r5, r5, r8),
              "bne 0x00000006\n"
              "sub r5, r8\n");

  COMPARE_T32(Sub(eq, r7, sp, 1),
              "bne 0x00000006\n"
              "sub r7, sp, #1\n");

  MUST_FAIL_TEST_T32(Sub(eq, pc, pc, 0), "Unpredictable instruction.\n");

  // TST (register) T1
  COMPARE_T32(Tst(eq, r0, r1),
              "it eq\n"
              "tsteq r0, r1\n");

  COMPARE_T32(Tst(eq, r8, r9),
              "bne 0x00000006\n"
              "tst r8, r9\n");

  CLEANUP();
}


TEST(unbound_label) {
  SETUP();

#ifdef VIXL_DEBUG
  MUST_FAIL_TEST_BOTH_BLOCK(
      {
        Label label;
        masm.B(&label);
      },
      "Location, label or literal used but not bound.\n")

  MUST_FAIL_TEST_BOTH_BLOCK(
      {
        Label label;
        masm.B(eq, &label);
      },
      "Location, label or literal used but not bound.\n")

  MUST_FAIL_TEST_T32_BLOCK(
      {
        Label label;
        masm.Cbz(r0, &label);
      },
      "Location, label or literal used but not bound.\n")

  MUST_FAIL_TEST_T32_BLOCK(
      {
        Label label;
        masm.Cbnz(r1, &label);
      },
      "Location, label or literal used but not bound.\n")
#endif

  CLEANUP();
}


TEST(macro_assembler_AddressComputationHelper) {
  SETUP();

  // Simple cases: the address fits in the mask.
  COMPARE_A32(Ldr(r0, masm.MemOperandComputationHelper(r1, r1, 0xfff, 0xfff)),
              "ldr r0, [r1, #4095]\n");
  COMPARE_A32(Ldr(r0, masm.MemOperandComputationHelper(r1, r1, 1, 0xfff)),
              "ldr r0, [r1, #1]\n");
  COMPARE_A32(Ldr(r0, masm.MemOperandComputationHelper(r1, r1, 0, 0xfff)),
              "ldr r0, [r1]\n");

  // Similar, but the base register must be preserved. (This has no effect for
  // encodable cases.)
  COMPARE_A32(Ldr(r0, masm.MemOperandComputationHelper(r2, r1, 0xfff, 0xfff)),
              "ldr r0, [r1, #4095]\n");

  // Cases where the extra offset has to be aligned.
  COMPARE_A32(Vldr(d0, masm.MemOperandComputationHelper(r1, r1, 0x3fc, 0x3fc)),
              "vldr d0, [r1, #1020]\n");

  // Out-of-range offsets.
  COMPARE_A32(Ldr(r0, masm.MemOperandComputationHelper(r1, r1, 0x1000, 0xfff)),
              "add r1, #4096\n"
              "ldr r0, [r1]\n");
  COMPARE_A32(Ldr(r0, masm.MemOperandComputationHelper(r2, r1, 0x1000, 0xfff)),
              "add r2, r1, #4096\n"
              "ldr r0, [r2]\n");
  COMPARE_A32(Ldr(r0,
                  masm.MemOperandComputationHelper(r2, r1, 0xffffffff, 0xfff)),
              "sub r2, r1, #1\n"
              "ldr r0, [r2]\n");

  // TODO: Improve the code generation for these cases.

  COMPARE_A32(Ldr(r0,
                  masm.MemOperandComputationHelper(r2, r1, 0x12345678, 0xfff)),
              "mov r2, #20480\n"
              "movt r2, #4660\n"
              "add r2, r1, r2\n"
              "ldr r0, [r2, #1656]\n");
  COMPARE_A32(Ldr(r0,
                  masm.MemOperandComputationHelper(r2, r1, 0x7fffffff, 0xfff)),
              "sub r2, r1, #1\n"
              "sub r2, #2147483648\n"
              "ldr r0, [r2]\n");
  COMPARE_A32(Ldr(r0,
                  masm.MemOperandComputationHelper(r2, r1, 0xffcba000, 0xfff)),
              "sub r2, r1, #286720\n"
              "sub r2, #3145728\n"
              "ldr r0, [r2]\n");

  CLEANUP();
}


TEST(barriers) {
  SETUP();

  // DMB
  COMPARE_BOTH(Dmb(SY), "dmb sy\n");
  COMPARE_BOTH(Dmb(ST), "dmb st\n");
  COMPARE_BOTH(Dmb(ISH), "dmb ish\n");
  COMPARE_BOTH(Dmb(ISHST), "dmb ishst\n");
  COMPARE_BOTH(Dmb(NSH), "dmb nsh\n");
  COMPARE_BOTH(Dmb(NSHST), "dmb nshst\n");
  COMPARE_BOTH(Dmb(OSH), "dmb osh\n");
  COMPARE_BOTH(Dmb(OSHST), "dmb oshst\n");

  // DSB
  COMPARE_BOTH(Dsb(SY), "dsb sy\n");
  COMPARE_BOTH(Dsb(ST), "dsb st\n");
  COMPARE_BOTH(Dsb(ISH), "dsb ish\n");
  COMPARE_BOTH(Dsb(ISHST), "dsb ishst\n");
  COMPARE_BOTH(Dsb(NSH), "dsb nsh\n");
  COMPARE_BOTH(Dsb(NSHST), "dsb nshst\n");
  COMPARE_BOTH(Dsb(OSH), "dsb osh\n");
  COMPARE_BOTH(Dsb(OSHST), "dsb oshst\n");

  // ISB
  COMPARE_BOTH(Isb(SY), "isb sy\n");

  CLEANUP();
}


TEST(preloads) {
  // Smoke test for various pld/pli forms.
  SETUP();

  // PLD immediate
  COMPARE_BOTH(Pld(MemOperand(r0, 0)), "pld [r0]\n");
  COMPARE_BOTH(Pld(MemOperand(r1, 123)), "pld [r1, #123]\n");
  COMPARE_BOTH(Pld(MemOperand(r4, -123)), "pld [r4, #-123]\n");

  COMPARE_A32(Pld(MemOperand(r7, -4095)), "pld [r7, #-4095]\n");

  // PLDW immediate
  COMPARE_BOTH(Pldw(MemOperand(r0, 0)), "pldw [r0]\n");
  COMPARE_BOTH(Pldw(MemOperand(r1, 123)), "pldw [r1, #123]\n");
  COMPARE_BOTH(Pldw(MemOperand(r4, -123)), "pldw [r4, #-123]\n");

  COMPARE_A32(Pldw(MemOperand(r7, -4095)), "pldw [r7, #-4095]\n");

  // PLD register
  COMPARE_BOTH(Pld(MemOperand(r0, r1)), "pld [r0, r1]\n");
  COMPARE_BOTH(Pld(MemOperand(r0, r1, LSL, 1)), "pld [r0, r1, lsl #1]\n");

  COMPARE_A32(Pld(MemOperand(r0, r1, LSL, 20)), "pld [r0, r1, lsl #20]\n");

  // PLDW register
  COMPARE_BOTH(Pldw(MemOperand(r0, r1)), "pldw [r0, r1]\n");
  COMPARE_BOTH(Pldw(MemOperand(r0, r1, LSL, 1)), "pldw [r0, r1, lsl #1]\n");

  COMPARE_A32(Pldw(MemOperand(r0, r1, LSL, 20)), "pldw [r0, r1, lsl #20]\n");

  // PLD literal
  COMPARE_BOTH(Pld(MemOperand(pc, minus, 0)), "pld [pc, #-0]\n");

  // PLI immediate
  COMPARE_BOTH(Pli(MemOperand(r0, 0)), "pli [r0]\n");
  COMPARE_BOTH(Pli(MemOperand(r1, 123)), "pli [r1, #123]\n");
  COMPARE_BOTH(Pli(MemOperand(r4, -123)), "pli [r4, #-123]\n");

  COMPARE_A32(Pli(MemOperand(r7, -4095)), "pli [r7, #-4095]\n");

  // PLI register
  COMPARE_BOTH(Pli(MemOperand(r0, r1)), "pli [r0, r1]\n");
  COMPARE_BOTH(Pli(MemOperand(r0, r1, LSL, 1)), "pli [r0, r1, lsl #1]\n");

  COMPARE_A32(Pli(MemOperand(r0, r1, LSL, 20)), "pli [r0, r1, lsl #20]\n");

  // PLI literal
  COMPARE_BOTH(Pli(MemOperand(pc, minus, 0)), "pli [pc, #-0]\n");

  CLEANUP();
}


TEST(vcmp_vcmpe) {
  SETUP();

  COMPARE_BOTH(Vcmp(F32, s0, s1), "vcmp.f32 s0, s1\n");
  COMPARE_BOTH(Vcmp(F64, d0, d1), "vcmp.f64 d0, d1\n");
  COMPARE_BOTH(Vcmp(F32, s0, 0.0f), "vcmp.f32 s0, #0.0\n");
  COMPARE_BOTH(Vcmp(F64, d0, 0.0), "vcmp.f64 d0, #0.0\n");

  COMPARE_BOTH(Vcmpe(F32, s0, s1), "vcmpe.f32 s0, s1\n");
  COMPARE_BOTH(Vcmpe(F64, d0, d1), "vcmpe.f64 d0, d1\n");
  COMPARE_BOTH(Vcmpe(F32, s0, 0.0f), "vcmpe.f32 s0, #0.0\n");
  COMPARE_BOTH(Vcmpe(F64, d0, 0.0), "vcmpe.f64 d0, #0.0\n");

  CLEANUP();
}


TEST(vmrs_vmsr) {
  SETUP();

  COMPARE_BOTH(Vmsr(FPSCR, r0), "vmsr FPSCR, r0\n");

  COMPARE_BOTH(Vmrs(RegisterOrAPSR_nzcv(r1.GetCode()), FPSCR),
               "vmrs r1, FPSCR\n");

  COMPARE_BOTH(Vmrs(RegisterOrAPSR_nzcv(pc.GetCode()), FPSCR),
               "vmrs APSR_nzcv, FPSCR\n");

  CLEANUP();
}


TEST(ldm_stm) {
  SETUP();

  // ldm/stm
  COMPARE_BOTH(Ldm(r0, NO_WRITE_BACK, RegisterList(r1)), "ldm r0, {r1}\n");

  COMPARE_BOTH(Ldm(r1, NO_WRITE_BACK, RegisterList(r2, r5, r9, r10)),
               "ldm r1, {r2,r5,r9,r10}\n");

  COMPARE_BOTH(Ldm(r0, WRITE_BACK, RegisterList(r1, r2)), "ldm r0!, {r1,r2}\n");

  COMPARE_BOTH(Stm(r1, NO_WRITE_BACK, RegisterList(r2, r5, r9, r10)),
               "stm r1, {r2,r5,r9,r10}\n");

  COMPARE_BOTH(Stm(r0, WRITE_BACK, RegisterList(r1, r2)), "stm r0!, {r1,r2}\n");

  // ldmda/stmda
  COMPARE_A32(Ldmda(r11, WRITE_BACK, RegisterList(r0, r1)),
              "ldmda r11!, {r0,r1}\n");

  COMPARE_A32(Ldmda(r11, NO_WRITE_BACK, RegisterList(r2, r3)),
              "ldmda r11, {r2,r3}\n");

  COMPARE_A32(Stmda(r11, WRITE_BACK, RegisterList(r0, r1)),
              "stmda r11!, {r0,r1}\n");

  COMPARE_A32(Stmda(r11, NO_WRITE_BACK, RegisterList(r2, r3)),
              "stmda r11, {r2,r3}\n");

  // ldmib/stmib
  COMPARE_A32(Ldmib(r11, WRITE_BACK, RegisterList(r0, r1)),
              "ldmib r11!, {r0,r1}\n");

  COMPARE_A32(Ldmib(r11, NO_WRITE_BACK, RegisterList(r2, r3)),
              "ldmib r11, {r2,r3}\n");

  COMPARE_A32(Stmib(r11, WRITE_BACK, RegisterList(r0, r1)),
              "stmib r11!, {r0,r1}\n");

  COMPARE_A32(Stmib(r11, NO_WRITE_BACK, RegisterList(r2, r3)),
              "stmib r11, {r2,r3}\n");

  // ldmdb/stmdb
  COMPARE_BOTH(Ldmdb(r11, WRITE_BACK, RegisterList(r0, r1)),
               "ldmdb r11!, {r0,r1}\n");

  COMPARE_BOTH(Ldmdb(r11, NO_WRITE_BACK, RegisterList(r2, r3)),
               "ldmdb r11, {r2,r3}\n");

  COMPARE_BOTH(Stmdb(r11, WRITE_BACK, RegisterList(r0, r1)),
               "stmdb r11!, {r0,r1}\n");

  COMPARE_BOTH(Stmdb(r11, NO_WRITE_BACK, RegisterList(r2, r3)),
               "stmdb r11, {r2,r3}\n");

  CLEANUP();
}


#define CHECK_T32_16(ASM, EXP) COMPARE_T32_CHECK_SIZE(ASM, EXP, 2)
// For instructions inside an IT block, we need to account for the IT
// instruction as well (another 16 bits).
#define CHECK_T32_16_IT_BLOCK(ASM, EXP) COMPARE_T32_CHECK_SIZE(ASM, EXP, 4)

TEST(macro_assembler_T32_16bit) {
  SETUP();

  // Allow the test to use all registers.
  UseScratchRegisterScope temps(&masm);
  temps.ExcludeAll();

  CHECK_T32_16(Adc(DontCare, r7, r7, r6), "adcs r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Adc(DontCare, eq, r7, r7, r6),
                        "it eq\n"
                        "adceq r7, r6\n");

  CHECK_T32_16(Add(DontCare, r6, r7, 7), "adds r6, r7, #7\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, lt, r6, r7, 7),
                        "it lt\n"
                        "addlt r6, r7, #7\n");

  CHECK_T32_16(Add(DontCare, r5, r5, 255), "adds r5, #255\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, lt, r5, r5, 255),
                        "it lt\n"
                        "addlt r5, #255\n");

  // Make sure we select the non flag-setting version here, since
  // this can have two potential encodings.
  CHECK_T32_16(Add(DontCare, r1, r1, r2), "add r1, r2\n");

  CHECK_T32_16(Add(DontCare, r1, r2, r7), "adds r1, r2, r7\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, lt, r1, r2, r7),
                        "it lt\n"
                        "addlt r1, r2, r7\n");

  CHECK_T32_16(Add(DontCare, r4, r4, r12), "add r4, ip\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, eq, r4, r4, r12),
                        "it eq\n"
                        "addeq r4, ip\n");

  CHECK_T32_16(Add(DontCare, r0, sp, 1020), "add r0, sp, #1020\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, ge, r0, sp, 1020),
                        "it ge\n"
                        "addge r0, sp, #1020\n");

  // The equivalent inside an IT block is deprecated.
  CHECK_T32_16(Add(DontCare, sp, sp, 508), "add sp, #508\n");

  CHECK_T32_16(Add(DontCare, r7, sp, r7), "add r7, sp, r7\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, eq, r7, sp, r7),
                        "it eq\n"
                        "addeq r7, sp, r7\n");

  CHECK_T32_16(Add(DontCare, sp, sp, r10), "add sp, r10\n");

  CHECK_T32_16_IT_BLOCK(Add(DontCare, eq, sp, sp, r10),
                        "it eq\n"
                        "addeq sp, r10\n");

  CHECK_T32_16(And(DontCare, r7, r7, r6), "ands r7, r6\n");

  CHECK_T32_16_IT_BLOCK(And(DontCare, eq, r7, r7, r6),
                        "it eq\n"
                        "andeq r7, r6\n");

  CHECK_T32_16(Asr(DontCare, r0, r1, 32), "asrs r0, r1, #32\n");

  CHECK_T32_16_IT_BLOCK(Asr(DontCare, eq, r0, r1, 32),
                        "it eq\n"
                        "asreq r0, r1, #32\n");

  CHECK_T32_16(Asr(DontCare, r0, r0, r1), "asrs r0, r1\n");

  CHECK_T32_16_IT_BLOCK(Asr(DontCare, eq, r0, r0, r1),
                        "it eq\n"
                        "asreq r0, r1\n");

  CHECK_T32_16(Bic(DontCare, r7, r7, r6), "bics r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Bic(DontCare, eq, r7, r7, r6),
                        "it eq\n"
                        "biceq r7, r6\n");

  CHECK_T32_16(Eor(DontCare, r7, r7, r6), "eors r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Eor(DontCare, eq, r7, r7, r6),
                        "it eq\n"
                        "eoreq r7, r6\n");

  CHECK_T32_16(Lsl(DontCare, r0, r1, 31), "lsls r0, r1, #31\n");

  CHECK_T32_16_IT_BLOCK(Lsl(DontCare, eq, r0, r1, 31),
                        "it eq\n"
                        "lsleq r0, r1, #31\n");

  CHECK_T32_16(Lsl(DontCare, r0, r0, r1), "lsls r0, r1\n");

  CHECK_T32_16_IT_BLOCK(Lsl(DontCare, eq, r0, r0, r1),
                        "it eq\n"
                        "lsleq r0, r1\n");

  CHECK_T32_16(Lsr(DontCare, r0, r1, 32), "lsrs r0, r1, #32\n");

  CHECK_T32_16_IT_BLOCK(Lsr(DontCare, eq, r0, r1, 32),
                        "it eq\n"
                        "lsreq r0, r1, #32\n");

  CHECK_T32_16(Lsr(DontCare, r0, r0, r1), "lsrs r0, r1\n");

  CHECK_T32_16_IT_BLOCK(Lsr(DontCare, eq, r0, r0, r1),
                        "it eq\n"
                        "lsreq r0, r1\n");

  CHECK_T32_16(Mov(DontCare, r7, 255), "movs r7, #255\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r7, 255),
                        "it eq\n"
                        "moveq r7, #255\n");

  CHECK_T32_16(Mov(DontCare, r9, r8), "mov r9, r8\n");

  // Check that we don't try to pick the MOVS register-shifted register variant.
  CHECK_T32_16(Mov(DontCare, r5, r6), "mov r5, r6\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r9, r8),
                        "it eq\n"
                        "moveq r9, r8\n");

  CHECK_T32_16(Mov(DontCare, r5, Operand(r6, ASR, 1)), "asrs r5, r6, #1\n");

  CHECK_T32_16(Mov(DontCare, r5, Operand(r6, ASR, 32)), "asrs r5, r6, #32\n");

  CHECK_T32_16(Mov(DontCare, r5, Operand(r6, LSR, 1)), "lsrs r5, r6, #1\n");

  CHECK_T32_16(Mov(DontCare, r5, Operand(r6, LSR, 32)), "lsrs r5, r6, #32\n");

  CHECK_T32_16(Mov(DontCare, r5, Operand(r6, LSL, 1)), "lsls r5, r6, #1\n");

  CHECK_T32_16(Mov(DontCare, r5, Operand(r6, LSL, 31)), "lsls r5, r6, #31\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r5, Operand(r6, ASR, 1)),
                        "it eq\n"
                        "asreq r5, r6, #1\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r5, Operand(r6, ASR, 32)),
                        "it eq\n"
                        "asreq r5, r6, #32\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r5, Operand(r6, LSR, 1)),
                        "it eq\n"
                        "lsreq r5, r6, #1\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r5, Operand(r6, LSR, 32)),
                        "it eq\n"
                        "lsreq r5, r6, #32\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r5, Operand(r6, LSL, 1)),
                        "it eq\n"
                        "lsleq r5, r6, #1\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r5, Operand(r6, LSL, 31)),
                        "it eq\n"
                        "lsleq r5, r6, #31\n");

  CHECK_T32_16(Mov(DontCare, r7, Operand(r7, ASR, r6)), "asrs r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r7, Operand(r7, ASR, r6)),
                        "it eq\n"
                        "asreq r7, r6\n");

  CHECK_T32_16(Mov(DontCare, r7, Operand(r7, LSR, r6)), "lsrs r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r7, Operand(r7, LSR, r6)),
                        "it eq\n"
                        "lsreq r7, r6\n");

  CHECK_T32_16(Mov(DontCare, r7, Operand(r7, LSL, r6)), "lsls r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r7, Operand(r7, LSL, r6)),
                        "it eq\n"
                        "lsleq r7, r6\n");

  CHECK_T32_16(Mov(DontCare, r7, Operand(r7, ROR, r6)), "rors r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Mov(DontCare, eq, r7, Operand(r7, ROR, r6)),
                        "it eq\n"
                        "roreq r7, r6\n");

  CHECK_T32_16(Mul(DontCare, r0, r1, r0), "muls r0, r1, r0\n");

  CHECK_T32_16_IT_BLOCK(Mul(DontCare, eq, r0, r1, r0),
                        "it eq\n"
                        "muleq r0, r1, r0\n");

  CHECK_T32_16(Mvn(DontCare, r6, r7), "mvns r6, r7\n");

  CHECK_T32_16_IT_BLOCK(Mvn(DontCare, eq, r6, r7),
                        "it eq\n"
                        "mvneq r6, r7\n");

  CHECK_T32_16(Orr(DontCare, r7, r7, r6), "orrs r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Orr(DontCare, eq, r7, r7, r6),
                        "it eq\n"
                        "orreq r7, r6\n");

  CHECK_T32_16(Ror(DontCare, r0, r0, r1), "rors r0, r1\n");

  CHECK_T32_16_IT_BLOCK(Ror(DontCare, eq, r0, r0, r1),
                        "it eq\n"
                        "roreq r0, r1\n");

  CHECK_T32_16(Rsb(DontCare, r7, r6, 0), "rsbs r7, r6, #0\n");

  CHECK_T32_16_IT_BLOCK(Rsb(DontCare, eq, r7, r6, 0),
                        "it eq\n"
                        "rsbeq r7, r6, #0\n");

  CHECK_T32_16(Sbc(DontCare, r7, r7, r6), "sbcs r7, r6\n");

  CHECK_T32_16_IT_BLOCK(Sbc(DontCare, eq, r7, r7, r6),
                        "it eq\n"
                        "sbceq r7, r6\n");

  CHECK_T32_16(Sub(DontCare, r6, r7, 7), "subs r6, r7, #7\n");

  CHECK_T32_16_IT_BLOCK(Sub(DontCare, lt, r6, r7, 7),
                        "it lt\n"
                        "sublt r6, r7, #7\n");

  CHECK_T32_16(Sub(DontCare, r5, r5, 255), "subs r5, #255\n");

  CHECK_T32_16_IT_BLOCK(Sub(DontCare, lt, r5, r5, 255),
                        "it lt\n"
                        "sublt r5, #255\n");

  CHECK_T32_16(Sub(DontCare, r1, r2, r7), "subs r1, r2, r7\n");

  CHECK_T32_16_IT_BLOCK(Sub(DontCare, lt, r1, r2, r7),
                        "it lt\n"
                        "sublt r1, r2, r7\n");

  // The equivalent inside an IT block is deprecated.
  CHECK_T32_16(Sub(DontCare, sp, sp, 508), "sub sp, #508\n");

  // Generate SUBS for ADD.
  CHECK_T32_16(Add(DontCare, r0, r1, -1), "subs r0, r1, #1\n");

  CHECK_T32_16(Add(DontCare, r0, r1, -7), "subs r0, r1, #7\n");

  CHECK_T32_16(Add(DontCare, r6, r6, -1), "subs r6, #1\n");

  CHECK_T32_16(Add(DontCare, r6, r6, -255), "subs r6, #255\n");

  // Generate ADDS for SUB.
  CHECK_T32_16(Sub(DontCare, r0, r1, -1), "adds r0, r1, #1\n");

  CHECK_T32_16(Sub(DontCare, r0, r1, -7), "adds r0, r1, #7\n");

  CHECK_T32_16(Sub(DontCare, r6, r6, -1), "adds r6, #1\n");

  CHECK_T32_16(Sub(DontCare, r6, r6, -255), "adds r6, #255\n");

  // Check that we don't change the opcode for INT_MIN.
  COMPARE_T32(Add(DontCare, r6, r6, 0x80000000), "add r6, #2147483648\n");

  COMPARE_T32(Sub(DontCare, r6, r6, 0x80000000), "sub r6, #2147483648\n");

  CLEANUP();
}
#undef CHECK_T32_16
#undef CHECK_T32_16_IT_BLOCK

TEST(nop_code) {
  SETUP();

  COMPARE_BOTH(Nop(), "nop\n");

  COMPARE_BOTH(And(r0, r0, r0), "");
  COMPARE_BOTH(And(DontCare, r0, r0, r0), "");

  COMPARE_BOTH(Mov(r0, r0), "");
  COMPARE_BOTH(Mov(DontCare, r0, r0), "");

  COMPARE_BOTH(Orr(r0, r0, r0), "");
  COMPARE_BOTH(Orr(DontCare, r0, r0, r0), "");

  CLEANUP();
}


TEST(minus_zero_offsets) {
  SETUP();

  COMPARE_A32(Ldr(r0, MemOperand(pc, minus, 0)), "ldr r0, [pc, #-0]\n");
  COMPARE_T32(Ldr(r0, MemOperand(pc, minus, 0)), "ldr.w r0, [pc, #-0]\n");
  COMPARE_BOTH(Ldrb(r0, MemOperand(pc, minus, 0)), "ldrb r0, [pc, #-0]\n");
  COMPARE_BOTH(Ldrh(r0, MemOperand(pc, minus, 0)), "ldrh r0, [pc, #-0]\n");
  COMPARE_BOTH(Ldrd(r0, r1, MemOperand(pc, minus, 0)),
               "ldrd r0, r1, [pc, #-0]\n");
  COMPARE_BOTH(Ldrsb(r0, MemOperand(pc, minus, 0)), "ldrsb r0, [pc, #-0]\n");
  COMPARE_BOTH(Ldrsh(r0, MemOperand(pc, minus, 0)), "ldrsh r0, [pc, #-0]\n");
  COMPARE_BOTH(Pld(MemOperand(pc, minus, 0)), "pld [pc, #-0]\n");
  COMPARE_BOTH(Pli(MemOperand(pc, minus, 0)), "pli [pc, #-0]\n");
  COMPARE_BOTH(Vldr(s0, MemOperand(pc, minus, 0)), "vldr s0, [pc, #-0]\n");
  COMPARE_BOTH(Vldr(d0, MemOperand(pc, minus, 0)), "vldr d0, [pc, #-0]\n");

  // This is an alias of ADR with a minus zero offset.
  COMPARE_BOTH(Sub(r0, pc, 0), "sub r0, pc, #0\n");

  CLEANUP();
}


TEST(big_add_sub) {
  SETUP();

  COMPARE_A32(Add(r0, r1, 0x4321),
              "add r0, r1, #33\n"
              "add r0, #17152\n");
  COMPARE_T32(Add(r0, r1, 0x4321),
              "add r0, r1, #801\n"
              "add r0, #16384\n");
  COMPARE_BOTH(Add(r0, r1, 0x432100),
               "add r0, r1, #8448\n"
               "add r0, #4390912\n");
  COMPARE_BOTH(Add(r0, r1, 0x43000210),
               "add r0, r1, #528\n"
               "add r0, #1124073472\n");
  COMPARE_BOTH(Add(r0, r1, 0x30c00210),
               "add r0, r1, #528\n"
               "add r0, #817889280\n");
  COMPARE_BOTH(Add(r0, r1, 0x43000021),
               "add r0, r1, #33\n"
               "add r0, #1124073472\n");
  COMPARE_T32(Add(r0, r1, 0x54321),
              "add r0, r1, #801\n"
              "add r0, #344064\n");
  COMPARE_T32(Add(r0, r1, 0x54000321),
              "add r0, r1, #801\n"
              "add r0, #1409286144\n");

  COMPARE_A32(Sub(r0, r1, 0x4321),
              "sub r0, r1, #33\n"
              "sub r0, #17152\n");
  COMPARE_T32(Sub(r0, r1, 0x4321),
              "sub r0, r1, #801\n"
              "sub r0, #16384\n");
  COMPARE_BOTH(Sub(r0, r1, 0x432100),
               "sub r0, r1, #8448\n"
               "sub r0, #4390912\n");
  COMPARE_BOTH(Sub(r0, r1, 0x43000210),
               "sub r0, r1, #528\n"
               "sub r0, #1124073472\n");
  COMPARE_BOTH(Sub(r0, r1, 0x30c00210),
               "sub r0, r1, #528\n"
               "sub r0, #817889280\n");
  COMPARE_BOTH(Sub(r0, r1, 0x43000021),
               "sub r0, r1, #33\n"
               "sub r0, #1124073472\n");
  COMPARE_T32(Sub(r0, r1, 0x54321),
              "sub r0, r1, #801\n"
              "sub r0, #344064\n");
  COMPARE_T32(Sub(r0, r1, 0x54000321),
              "sub r0, r1, #801\n"
              "sub r0, #1409286144\n");

  CLEANUP();
}

}  // namespace aarch32
}  // namespace vixl
