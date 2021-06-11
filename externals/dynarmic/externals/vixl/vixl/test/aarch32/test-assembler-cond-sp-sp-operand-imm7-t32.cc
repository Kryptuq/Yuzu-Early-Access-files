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


// -----------------------------------------------------------------------------
// This file is auto generated from the
// test/aarch32/config/template-assembler-aarch32.cc.in template file using
// tools/generate_tests.py.
//
// PLEASE DO NOT EDIT.
// -----------------------------------------------------------------------------


#include "test-runner.h"

#include "test-utils.h"
#include "test-utils-aarch32.h"

#include "aarch32/assembler-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#define BUF_SIZE (4096)

namespace vixl {
namespace aarch32 {

// List of instruction mnemonics.
#define FOREACH_INSTRUCTION(M) \
  M(add)                       \
  M(sub)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` has
// various layouts across generated tests so it absolutely cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_T32
namespace {

// Values to be passed to the assembler to produce the instruction under test.
struct Operands {
  Condition cond;
  Register rd;
  Register rn;
  uint32_t immediate;
};

// This structure contains all data needed to test one specific
// instruction.
struct TestData {
  // The `operands` field represents what to pass to the assembler to
  // produce the instruction.
  Operands operands;
  // True if we need to generate an IT instruction for this test to be valid.
  bool in_it_block;
  // The condition to give the IT instruction, this will be set to "al" by
  // default.
  Condition it_condition;
  // Description of the operands, used for error reporting.
  const char* operands_description;
  // Unique identifier, used for generating traces.
  const char* identifier;
};

struct TestResult {
  size_t size;
  const byte* encoding;
};

// Each element of this array produce one instruction encoding.
const TestData kTests[] =
    {{{al, sp, sp, 0x0}, false, al, "al sp sp 0x0", "al_sp_sp_0x0"},
     {{al, sp, sp, 0x4}, false, al, "al sp sp 0x4", "al_sp_sp_0x4"},
     {{al, sp, sp, 0x8}, false, al, "al sp sp 0x8", "al_sp_sp_0x8"},
     {{al, sp, sp, 0xc}, false, al, "al sp sp 0xc", "al_sp_sp_0xc"},
     {{al, sp, sp, 0x10}, false, al, "al sp sp 0x10", "al_sp_sp_0x10"},
     {{al, sp, sp, 0x14}, false, al, "al sp sp 0x14", "al_sp_sp_0x14"},
     {{al, sp, sp, 0x18}, false, al, "al sp sp 0x18", "al_sp_sp_0x18"},
     {{al, sp, sp, 0x1c}, false, al, "al sp sp 0x1c", "al_sp_sp_0x1c"},
     {{al, sp, sp, 0x20}, false, al, "al sp sp 0x20", "al_sp_sp_0x20"},
     {{al, sp, sp, 0x24}, false, al, "al sp sp 0x24", "al_sp_sp_0x24"},
     {{al, sp, sp, 0x28}, false, al, "al sp sp 0x28", "al_sp_sp_0x28"},
     {{al, sp, sp, 0x2c}, false, al, "al sp sp 0x2c", "al_sp_sp_0x2c"},
     {{al, sp, sp, 0x30}, false, al, "al sp sp 0x30", "al_sp_sp_0x30"},
     {{al, sp, sp, 0x34}, false, al, "al sp sp 0x34", "al_sp_sp_0x34"},
     {{al, sp, sp, 0x38}, false, al, "al sp sp 0x38", "al_sp_sp_0x38"},
     {{al, sp, sp, 0x3c}, false, al, "al sp sp 0x3c", "al_sp_sp_0x3c"},
     {{al, sp, sp, 0x40}, false, al, "al sp sp 0x40", "al_sp_sp_0x40"},
     {{al, sp, sp, 0x44}, false, al, "al sp sp 0x44", "al_sp_sp_0x44"},
     {{al, sp, sp, 0x48}, false, al, "al sp sp 0x48", "al_sp_sp_0x48"},
     {{al, sp, sp, 0x4c}, false, al, "al sp sp 0x4c", "al_sp_sp_0x4c"},
     {{al, sp, sp, 0x50}, false, al, "al sp sp 0x50", "al_sp_sp_0x50"},
     {{al, sp, sp, 0x54}, false, al, "al sp sp 0x54", "al_sp_sp_0x54"},
     {{al, sp, sp, 0x58}, false, al, "al sp sp 0x58", "al_sp_sp_0x58"},
     {{al, sp, sp, 0x5c}, false, al, "al sp sp 0x5c", "al_sp_sp_0x5c"},
     {{al, sp, sp, 0x60}, false, al, "al sp sp 0x60", "al_sp_sp_0x60"},
     {{al, sp, sp, 0x64}, false, al, "al sp sp 0x64", "al_sp_sp_0x64"},
     {{al, sp, sp, 0x68}, false, al, "al sp sp 0x68", "al_sp_sp_0x68"},
     {{al, sp, sp, 0x6c}, false, al, "al sp sp 0x6c", "al_sp_sp_0x6c"},
     {{al, sp, sp, 0x70}, false, al, "al sp sp 0x70", "al_sp_sp_0x70"},
     {{al, sp, sp, 0x74}, false, al, "al sp sp 0x74", "al_sp_sp_0x74"},
     {{al, sp, sp, 0x78}, false, al, "al sp sp 0x78", "al_sp_sp_0x78"},
     {{al, sp, sp, 0x7c}, false, al, "al sp sp 0x7c", "al_sp_sp_0x7c"},
     {{al, sp, sp, 0x80}, false, al, "al sp sp 0x80", "al_sp_sp_0x80"},
     {{al, sp, sp, 0x84}, false, al, "al sp sp 0x84", "al_sp_sp_0x84"},
     {{al, sp, sp, 0x88}, false, al, "al sp sp 0x88", "al_sp_sp_0x88"},
     {{al, sp, sp, 0x8c}, false, al, "al sp sp 0x8c", "al_sp_sp_0x8c"},
     {{al, sp, sp, 0x90}, false, al, "al sp sp 0x90", "al_sp_sp_0x90"},
     {{al, sp, sp, 0x94}, false, al, "al sp sp 0x94", "al_sp_sp_0x94"},
     {{al, sp, sp, 0x98}, false, al, "al sp sp 0x98", "al_sp_sp_0x98"},
     {{al, sp, sp, 0x9c}, false, al, "al sp sp 0x9c", "al_sp_sp_0x9c"},
     {{al, sp, sp, 0xa0}, false, al, "al sp sp 0xa0", "al_sp_sp_0xa0"},
     {{al, sp, sp, 0xa4}, false, al, "al sp sp 0xa4", "al_sp_sp_0xa4"},
     {{al, sp, sp, 0xa8}, false, al, "al sp sp 0xa8", "al_sp_sp_0xa8"},
     {{al, sp, sp, 0xac}, false, al, "al sp sp 0xac", "al_sp_sp_0xac"},
     {{al, sp, sp, 0xb0}, false, al, "al sp sp 0xb0", "al_sp_sp_0xb0"},
     {{al, sp, sp, 0xb4}, false, al, "al sp sp 0xb4", "al_sp_sp_0xb4"},
     {{al, sp, sp, 0xb8}, false, al, "al sp sp 0xb8", "al_sp_sp_0xb8"},
     {{al, sp, sp, 0xbc}, false, al, "al sp sp 0xbc", "al_sp_sp_0xbc"},
     {{al, sp, sp, 0xc0}, false, al, "al sp sp 0xc0", "al_sp_sp_0xc0"},
     {{al, sp, sp, 0xc4}, false, al, "al sp sp 0xc4", "al_sp_sp_0xc4"},
     {{al, sp, sp, 0xc8}, false, al, "al sp sp 0xc8", "al_sp_sp_0xc8"},
     {{al, sp, sp, 0xcc}, false, al, "al sp sp 0xcc", "al_sp_sp_0xcc"},
     {{al, sp, sp, 0xd0}, false, al, "al sp sp 0xd0", "al_sp_sp_0xd0"},
     {{al, sp, sp, 0xd4}, false, al, "al sp sp 0xd4", "al_sp_sp_0xd4"},
     {{al, sp, sp, 0xd8}, false, al, "al sp sp 0xd8", "al_sp_sp_0xd8"},
     {{al, sp, sp, 0xdc}, false, al, "al sp sp 0xdc", "al_sp_sp_0xdc"},
     {{al, sp, sp, 0xe0}, false, al, "al sp sp 0xe0", "al_sp_sp_0xe0"},
     {{al, sp, sp, 0xe4}, false, al, "al sp sp 0xe4", "al_sp_sp_0xe4"},
     {{al, sp, sp, 0xe8}, false, al, "al sp sp 0xe8", "al_sp_sp_0xe8"},
     {{al, sp, sp, 0xec}, false, al, "al sp sp 0xec", "al_sp_sp_0xec"},
     {{al, sp, sp, 0xf0}, false, al, "al sp sp 0xf0", "al_sp_sp_0xf0"},
     {{al, sp, sp, 0xf4}, false, al, "al sp sp 0xf4", "al_sp_sp_0xf4"},
     {{al, sp, sp, 0xf8}, false, al, "al sp sp 0xf8", "al_sp_sp_0xf8"},
     {{al, sp, sp, 0xfc}, false, al, "al sp sp 0xfc", "al_sp_sp_0xfc"},
     {{al, sp, sp, 0x100}, false, al, "al sp sp 0x100", "al_sp_sp_0x100"},
     {{al, sp, sp, 0x104}, false, al, "al sp sp 0x104", "al_sp_sp_0x104"},
     {{al, sp, sp, 0x108}, false, al, "al sp sp 0x108", "al_sp_sp_0x108"},
     {{al, sp, sp, 0x10c}, false, al, "al sp sp 0x10c", "al_sp_sp_0x10c"},
     {{al, sp, sp, 0x110}, false, al, "al sp sp 0x110", "al_sp_sp_0x110"},
     {{al, sp, sp, 0x114}, false, al, "al sp sp 0x114", "al_sp_sp_0x114"},
     {{al, sp, sp, 0x118}, false, al, "al sp sp 0x118", "al_sp_sp_0x118"},
     {{al, sp, sp, 0x11c}, false, al, "al sp sp 0x11c", "al_sp_sp_0x11c"},
     {{al, sp, sp, 0x120}, false, al, "al sp sp 0x120", "al_sp_sp_0x120"},
     {{al, sp, sp, 0x124}, false, al, "al sp sp 0x124", "al_sp_sp_0x124"},
     {{al, sp, sp, 0x128}, false, al, "al sp sp 0x128", "al_sp_sp_0x128"},
     {{al, sp, sp, 0x12c}, false, al, "al sp sp 0x12c", "al_sp_sp_0x12c"},
     {{al, sp, sp, 0x130}, false, al, "al sp sp 0x130", "al_sp_sp_0x130"},
     {{al, sp, sp, 0x134}, false, al, "al sp sp 0x134", "al_sp_sp_0x134"},
     {{al, sp, sp, 0x138}, false, al, "al sp sp 0x138", "al_sp_sp_0x138"},
     {{al, sp, sp, 0x13c}, false, al, "al sp sp 0x13c", "al_sp_sp_0x13c"},
     {{al, sp, sp, 0x140}, false, al, "al sp sp 0x140", "al_sp_sp_0x140"},
     {{al, sp, sp, 0x144}, false, al, "al sp sp 0x144", "al_sp_sp_0x144"},
     {{al, sp, sp, 0x148}, false, al, "al sp sp 0x148", "al_sp_sp_0x148"},
     {{al, sp, sp, 0x14c}, false, al, "al sp sp 0x14c", "al_sp_sp_0x14c"},
     {{al, sp, sp, 0x150}, false, al, "al sp sp 0x150", "al_sp_sp_0x150"},
     {{al, sp, sp, 0x154}, false, al, "al sp sp 0x154", "al_sp_sp_0x154"},
     {{al, sp, sp, 0x158}, false, al, "al sp sp 0x158", "al_sp_sp_0x158"},
     {{al, sp, sp, 0x15c}, false, al, "al sp sp 0x15c", "al_sp_sp_0x15c"},
     {{al, sp, sp, 0x160}, false, al, "al sp sp 0x160", "al_sp_sp_0x160"},
     {{al, sp, sp, 0x164}, false, al, "al sp sp 0x164", "al_sp_sp_0x164"},
     {{al, sp, sp, 0x168}, false, al, "al sp sp 0x168", "al_sp_sp_0x168"},
     {{al, sp, sp, 0x16c}, false, al, "al sp sp 0x16c", "al_sp_sp_0x16c"},
     {{al, sp, sp, 0x170}, false, al, "al sp sp 0x170", "al_sp_sp_0x170"},
     {{al, sp, sp, 0x174}, false, al, "al sp sp 0x174", "al_sp_sp_0x174"},
     {{al, sp, sp, 0x178}, false, al, "al sp sp 0x178", "al_sp_sp_0x178"},
     {{al, sp, sp, 0x17c}, false, al, "al sp sp 0x17c", "al_sp_sp_0x17c"},
     {{al, sp, sp, 0x180}, false, al, "al sp sp 0x180", "al_sp_sp_0x180"},
     {{al, sp, sp, 0x184}, false, al, "al sp sp 0x184", "al_sp_sp_0x184"},
     {{al, sp, sp, 0x188}, false, al, "al sp sp 0x188", "al_sp_sp_0x188"},
     {{al, sp, sp, 0x18c}, false, al, "al sp sp 0x18c", "al_sp_sp_0x18c"},
     {{al, sp, sp, 0x190}, false, al, "al sp sp 0x190", "al_sp_sp_0x190"},
     {{al, sp, sp, 0x194}, false, al, "al sp sp 0x194", "al_sp_sp_0x194"},
     {{al, sp, sp, 0x198}, false, al, "al sp sp 0x198", "al_sp_sp_0x198"},
     {{al, sp, sp, 0x19c}, false, al, "al sp sp 0x19c", "al_sp_sp_0x19c"},
     {{al, sp, sp, 0x1a0}, false, al, "al sp sp 0x1a0", "al_sp_sp_0x1a0"},
     {{al, sp, sp, 0x1a4}, false, al, "al sp sp 0x1a4", "al_sp_sp_0x1a4"},
     {{al, sp, sp, 0x1a8}, false, al, "al sp sp 0x1a8", "al_sp_sp_0x1a8"},
     {{al, sp, sp, 0x1ac}, false, al, "al sp sp 0x1ac", "al_sp_sp_0x1ac"},
     {{al, sp, sp, 0x1b0}, false, al, "al sp sp 0x1b0", "al_sp_sp_0x1b0"},
     {{al, sp, sp, 0x1b4}, false, al, "al sp sp 0x1b4", "al_sp_sp_0x1b4"},
     {{al, sp, sp, 0x1b8}, false, al, "al sp sp 0x1b8", "al_sp_sp_0x1b8"},
     {{al, sp, sp, 0x1bc}, false, al, "al sp sp 0x1bc", "al_sp_sp_0x1bc"},
     {{al, sp, sp, 0x1c0}, false, al, "al sp sp 0x1c0", "al_sp_sp_0x1c0"},
     {{al, sp, sp, 0x1c4}, false, al, "al sp sp 0x1c4", "al_sp_sp_0x1c4"},
     {{al, sp, sp, 0x1c8}, false, al, "al sp sp 0x1c8", "al_sp_sp_0x1c8"},
     {{al, sp, sp, 0x1cc}, false, al, "al sp sp 0x1cc", "al_sp_sp_0x1cc"},
     {{al, sp, sp, 0x1d0}, false, al, "al sp sp 0x1d0", "al_sp_sp_0x1d0"},
     {{al, sp, sp, 0x1d4}, false, al, "al sp sp 0x1d4", "al_sp_sp_0x1d4"},
     {{al, sp, sp, 0x1d8}, false, al, "al sp sp 0x1d8", "al_sp_sp_0x1d8"},
     {{al, sp, sp, 0x1dc}, false, al, "al sp sp 0x1dc", "al_sp_sp_0x1dc"},
     {{al, sp, sp, 0x1e0}, false, al, "al sp sp 0x1e0", "al_sp_sp_0x1e0"},
     {{al, sp, sp, 0x1e4}, false, al, "al sp sp 0x1e4", "al_sp_sp_0x1e4"},
     {{al, sp, sp, 0x1e8}, false, al, "al sp sp 0x1e8", "al_sp_sp_0x1e8"},
     {{al, sp, sp, 0x1ec}, false, al, "al sp sp 0x1ec", "al_sp_sp_0x1ec"},
     {{al, sp, sp, 0x1f0}, false, al, "al sp sp 0x1f0", "al_sp_sp_0x1f0"},
     {{al, sp, sp, 0x1f4}, false, al, "al sp sp 0x1f4", "al_sp_sp_0x1f4"},
     {{al, sp, sp, 0x1f8}, false, al, "al sp sp 0x1f8", "al_sp_sp_0x1f8"},
     {{al, sp, sp, 0x1fc}, false, al, "al sp sp 0x1fc", "al_sp_sp_0x1fc"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-sp-sp-operand-imm7-add-t32.h"
#include "aarch32/traces/assembler-cond-sp-sp-operand-imm7-sub-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
                                   Register rn,
                                   const Operand& op);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  unsigned total_error_count = 0;
  MacroAssembler masm(BUF_SIZE);

  masm.UseT32();

  for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
    // Values to pass to the macro-assembler.
    Condition cond = kTests[i].operands.cond;
    Register rd = kTests[i].operands.rd;
    Register rn = kTests[i].operands.rn;
    uint32_t immediate = kTests[i].operands.immediate;
    Operand op(immediate);

    int32_t start = masm.GetCursorOffset();
    {
      // We never generate more that 4 bytes, as IT instructions are only
      // allowed for narrow encodings.
      ExactAssemblyScope scope(&masm, 4, ExactAssemblyScope::kMaximumSize);
      if (kTests[i].in_it_block) {
        masm.it(kTests[i].it_condition);
      }
      (masm.*instruction)(cond, rd, rn, op);
    }
    int32_t end = masm.GetCursorOffset();

    const byte* result_ptr =
        masm.GetBuffer()->GetOffsetAddress<const byte*>(start);
    VIXL_ASSERT(start < end);
    uint32_t result_size = end - start;

    if (Test::generate_test_trace()) {
      // Print the result bytes.
      printf("const byte kInstruction_%s_%s[] = {\n",
             mnemonic,
             kTests[i].identifier);
      for (uint32_t j = 0; j < result_size; j++) {
        if (j == 0) {
          printf("  0x%02" PRIx8, result_ptr[j]);
        } else {
          printf(", 0x%02" PRIx8, result_ptr[j]);
        }
      }
      // This comment is meant to be used by external tools to validate
      // the encoding. We can parse the comment to figure out what
      // instruction this corresponds to.
      if (kTests[i].in_it_block) {
        printf(" // It %s; %s %s\n};\n",
               kTests[i].it_condition.GetName(),
               mnemonic,
               kTests[i].operands_description);
      } else {
        printf(" // %s %s\n};\n", mnemonic, kTests[i].operands_description);
      }
    } else {
      // Check we've emitted the exact same encoding as present in the
      // trace file. Only print up to `kErrorReportLimit` errors.
      if (((result_size != reference[i].size) ||
           (memcmp(result_ptr, reference[i].encoding, reference[i].size) !=
            0)) &&
          (++total_error_count <= kErrorReportLimit)) {
        printf("Error when testing \"%s\" with operands \"%s\":\n",
               mnemonic,
               kTests[i].operands_description);
        printf("  Expected: ");
        for (uint32_t j = 0; j < reference[i].size; j++) {
          if (j == 0) {
            printf("0x%02" PRIx8, reference[i].encoding[j]);
          } else {
            printf(", 0x%02" PRIx8, reference[i].encoding[j]);
          }
        }
        printf("\n");
        printf("  Found:    ");
        for (uint32_t j = 0; j < result_size; j++) {
          if (j == 0) {
            printf("0x%02" PRIx8, result_ptr[j]);
          } else {
            printf(", 0x%02" PRIx8, result_ptr[j]);
          }
        }
        printf("\n");
      }
    }
  }

  masm.FinalizeCode();

  if (Test::generate_test_trace()) {
    // Finalize the trace file by writing the final `TestResult` array
    // which links all generated instruction encodings.
    printf("const TestResult kReference%s[] = {\n", mnemonic);
    for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
      printf("  {\n");
      printf("    ARRAY_SIZE(kInstruction_%s_%s),\n",
             mnemonic,
             kTests[i].identifier);
      printf("    kInstruction_%s_%s,\n", mnemonic, kTests[i].identifier);
      printf("  },\n");
    }
    printf("};\n");
  } else {
    if (total_error_count > kErrorReportLimit) {
      printf("%u other errors follow.\n",
             total_error_count - kErrorReportLimit);
    }
    // Crash if the test failed.
    VIXL_CHECK(total_error_count == 0);
  }
}

// Instantiate tests for each instruction in the list.
#define TEST(mnemonic)                                                        \
  void Test_##mnemonic() {                                                    \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic);   \
  }                                                                           \
  Test test_##mnemonic("AARCH32_ASSEMBLER_COND_SP_SP_OPERAND_IMM7_" #mnemonic \
                       "_T32",                                                \
                       &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
