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
  M(vadd)                      \
  M(vsub)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` has
// various layouts across generated tests so it absolutely cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_T32
namespace {

// Values to be passed to the assembler to produce the instruction under test.
struct Operands {
  DataType dt;
  DRegister rd;
  DRegister rn;
  DRegister rm;
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
    {{{F64, d16, d17, d14}, false, al, "F64 d16 d17 d14", "F64_d16_d17_d14"},
     {{F64, d21, d29, d16}, false, al, "F64 d21 d29 d16", "F64_d21_d29_d16"},
     {{F64, d19, d28, d12}, false, al, "F64 d19 d28 d12", "F64_d19_d28_d12"},
     {{F32, d5, d16, d12}, false, al, "F32 d5 d16 d12", "F32_d5_d16_d12"},
     {{F64, d31, d7, d1}, false, al, "F64 d31 d7 d1", "F64_d31_d7_d1"},
     {{F64, d22, d6, d1}, false, al, "F64 d22 d6 d1", "F64_d22_d6_d1"},
     {{F32, d22, d28, d12}, false, al, "F32 d22 d28 d12", "F32_d22_d28_d12"},
     {{F32, d15, d1, d6}, false, al, "F32 d15 d1 d6", "F32_d15_d1_d6"},
     {{F32, d18, d17, d23}, false, al, "F32 d18 d17 d23", "F32_d18_d17_d23"},
     {{F64, d29, d21, d18}, false, al, "F64 d29 d21 d18", "F64_d29_d21_d18"},
     {{F64, d28, d29, d29}, false, al, "F64 d28 d29 d29", "F64_d28_d29_d29"},
     {{F64, d26, d28, d26}, false, al, "F64 d26 d28 d26", "F64_d26_d28_d26"},
     {{F64, d2, d18, d10}, false, al, "F64 d2 d18 d10", "F64_d2_d18_d10"},
     {{F64, d4, d30, d6}, false, al, "F64 d4 d30 d6", "F64_d4_d30_d6"},
     {{F64, d11, d25, d14}, false, al, "F64 d11 d25 d14", "F64_d11_d25_d14"},
     {{F32, d4, d26, d15}, false, al, "F32 d4 d26 d15", "F32_d4_d26_d15"},
     {{F64, d9, d20, d30}, false, al, "F64 d9 d20 d30", "F64_d9_d20_d30"},
     {{F64, d18, d12, d20}, false, al, "F64 d18 d12 d20", "F64_d18_d12_d20"},
     {{F64, d1, d27, d15}, false, al, "F64 d1 d27 d15", "F64_d1_d27_d15"},
     {{F32, d22, d22, d3}, false, al, "F32 d22 d22 d3", "F32_d22_d22_d3"},
     {{F32, d29, d19, d6}, false, al, "F32 d29 d19 d6", "F32_d29_d19_d6"},
     {{F64, d9, d23, d8}, false, al, "F64 d9 d23 d8", "F64_d9_d23_d8"},
     {{F32, d12, d18, d29}, false, al, "F32 d12 d18 d29", "F32_d12_d18_d29"},
     {{F64, d0, d12, d13}, false, al, "F64 d0 d12 d13", "F64_d0_d12_d13"},
     {{F64, d9, d17, d17}, false, al, "F64 d9 d17 d17", "F64_d9_d17_d17"},
     {{F64, d11, d0, d29}, false, al, "F64 d11 d0 d29", "F64_d11_d0_d29"},
     {{F64, d24, d16, d28}, false, al, "F64 d24 d16 d28", "F64_d24_d16_d28"},
     {{F64, d22, d14, d26}, false, al, "F64 d22 d14 d26", "F64_d22_d14_d26"},
     {{F64, d11, d14, d22}, false, al, "F64 d11 d14 d22", "F64_d11_d14_d22"},
     {{F64, d19, d12, d6}, false, al, "F64 d19 d12 d6", "F64_d19_d12_d6"},
     {{F64, d29, d3, d28}, false, al, "F64 d29 d3 d28", "F64_d29_d3_d28"},
     {{F64, d21, d8, d3}, false, al, "F64 d21 d8 d3", "F64_d21_d8_d3"},
     {{F32, d1, d15, d25}, false, al, "F32 d1 d15 d25", "F32_d1_d15_d25"},
     {{F64, d22, d8, d8}, false, al, "F64 d22 d8 d8", "F64_d22_d8_d8"},
     {{F64, d29, d23, d10}, false, al, "F64 d29 d23 d10", "F64_d29_d23_d10"},
     {{F32, d3, d16, d8}, false, al, "F32 d3 d16 d8", "F32_d3_d16_d8"},
     {{F32, d1, d0, d13}, false, al, "F32 d1 d0 d13", "F32_d1_d0_d13"},
     {{F64, d11, d11, d4}, false, al, "F64 d11 d11 d4", "F64_d11_d11_d4"},
     {{F64, d10, d22, d24}, false, al, "F64 d10 d22 d24", "F64_d10_d22_d24"},
     {{F64, d16, d22, d3}, false, al, "F64 d16 d22 d3", "F64_d16_d22_d3"},
     {{F64, d1, d22, d17}, false, al, "F64 d1 d22 d17", "F64_d1_d22_d17"},
     {{F32, d5, d26, d24}, false, al, "F32 d5 d26 d24", "F32_d5_d26_d24"},
     {{F64, d17, d1, d18}, false, al, "F64 d17 d1 d18", "F64_d17_d1_d18"},
     {{F32, d19, d15, d0}, false, al, "F32 d19 d15 d0", "F32_d19_d15_d0"},
     {{F32, d22, d3, d3}, false, al, "F32 d22 d3 d3", "F32_d22_d3_d3"},
     {{F32, d30, d16, d4}, false, al, "F32 d30 d16 d4", "F32_d30_d16_d4"},
     {{F64, d31, d28, d16}, false, al, "F64 d31 d28 d16", "F64_d31_d28_d16"},
     {{F64, d9, d9, d26}, false, al, "F64 d9 d9 d26", "F64_d9_d9_d26"},
     {{F32, d18, d12, d5}, false, al, "F32 d18 d12 d5", "F32_d18_d12_d5"},
     {{F64, d1, d1, d14}, false, al, "F64 d1 d1 d14", "F64_d1_d1_d14"},
     {{F64, d10, d26, d27}, false, al, "F64 d10 d26 d27", "F64_d10_d26_d27"},
     {{F32, d4, d24, d28}, false, al, "F32 d4 d24 d28", "F32_d4_d24_d28"},
     {{F64, d5, d17, d1}, false, al, "F64 d5 d17 d1", "F64_d5_d17_d1"},
     {{F64, d20, d4, d8}, false, al, "F64 d20 d4 d8", "F64_d20_d4_d8"},
     {{F32, d14, d5, d27}, false, al, "F32 d14 d5 d27", "F32_d14_d5_d27"},
     {{F32, d19, d26, d13}, false, al, "F32 d19 d26 d13", "F32_d19_d26_d13"},
     {{F64, d26, d6, d19}, false, al, "F64 d26 d6 d19", "F64_d26_d6_d19"},
     {{F64, d5, d26, d31}, false, al, "F64 d5 d26 d31", "F64_d5_d26_d31"},
     {{F64, d7, d26, d7}, false, al, "F64 d7 d26 d7", "F64_d7_d26_d7"},
     {{F32, d14, d10, d17}, false, al, "F32 d14 d10 d17", "F32_d14_d10_d17"},
     {{F32, d4, d16, d30}, false, al, "F32 d4 d16 d30", "F32_d4_d16_d30"},
     {{F64, d10, d12, d2}, false, al, "F64 d10 d12 d2", "F64_d10_d12_d2"},
     {{F32, d15, d17, d10}, false, al, "F32 d15 d17 d10", "F32_d15_d17_d10"},
     {{F64, d20, d25, d20}, false, al, "F64 d20 d25 d20", "F64_d20_d25_d20"},
     {{F32, d12, d21, d28}, false, al, "F32 d12 d21 d28", "F32_d12_d21_d28"},
     {{F64, d20, d23, d16}, false, al, "F64 d20 d23 d16", "F64_d20_d23_d16"},
     {{F64, d11, d19, d18}, false, al, "F64 d11 d19 d18", "F64_d11_d19_d18"},
     {{F64, d3, d6, d7}, false, al, "F64 d3 d6 d7", "F64_d3_d6_d7"},
     {{F32, d23, d20, d7}, false, al, "F32 d23 d20 d7", "F32_d23_d20_d7"},
     {{F32, d3, d16, d19}, false, al, "F32 d3 d16 d19", "F32_d3_d16_d19"},
     {{F64, d2, d17, d31}, false, al, "F64 d2 d17 d31", "F64_d2_d17_d31"},
     {{F64, d10, d30, d23}, false, al, "F64 d10 d30 d23", "F64_d10_d30_d23"},
     {{F64, d27, d11, d2}, false, al, "F64 d27 d11 d2", "F64_d27_d11_d2"},
     {{F64, d19, d10, d19}, false, al, "F64 d19 d10 d19", "F64_d19_d10_d19"},
     {{F64, d16, d4, d15}, false, al, "F64 d16 d4 d15", "F64_d16_d4_d15"},
     {{F32, d8, d28, d1}, false, al, "F32 d8 d28 d1", "F32_d8_d28_d1"},
     {{F32, d13, d31, d5}, false, al, "F32 d13 d31 d5", "F32_d13_d31_d5"},
     {{F64, d17, d13, d29}, false, al, "F64 d17 d13 d29", "F64_d17_d13_d29"},
     {{F32, d27, d19, d10}, false, al, "F32 d27 d19 d10", "F32_d27_d19_d10"},
     {{F64, d8, d4, d0}, false, al, "F64 d8 d4 d0", "F64_d8_d4_d0"},
     {{F64, d31, d1, d29}, false, al, "F64 d31 d1 d29", "F64_d31_d1_d29"},
     {{F64, d6, d2, d12}, false, al, "F64 d6 d2 d12", "F64_d6_d2_d12"},
     {{F64, d25, d23, d24}, false, al, "F64 d25 d23 d24", "F64_d25_d23_d24"},
     {{F64, d3, d12, d1}, false, al, "F64 d3 d12 d1", "F64_d3_d12_d1"},
     {{F64, d24, d18, d16}, false, al, "F64 d24 d18 d16", "F64_d24_d18_d16"},
     {{F32, d13, d21, d16}, false, al, "F32 d13 d21 d16", "F32_d13_d21_d16"},
     {{F32, d9, d6, d23}, false, al, "F32 d9 d6 d23", "F32_d9_d6_d23"},
     {{F64, d14, d14, d13}, false, al, "F64 d14 d14 d13", "F64_d14_d14_d13"},
     {{F32, d25, d23, d17}, false, al, "F32 d25 d23 d17", "F32_d25_d23_d17"},
     {{F64, d13, d25, d3}, false, al, "F64 d13 d25 d3", "F64_d13_d25_d3"},
     {{F64, d9, d11, d8}, false, al, "F64 d9 d11 d8", "F64_d9_d11_d8"},
     {{F32, d18, d13, d26}, false, al, "F32 d18 d13 d26", "F32_d18_d13_d26"},
     {{F64, d16, d14, d15}, false, al, "F64 d16 d14 d15", "F64_d16_d14_d15"},
     {{F32, d5, d30, d13}, false, al, "F32 d5 d30 d13", "F32_d5_d30_d13"},
     {{F32, d4, d21, d14}, false, al, "F32 d4 d21 d14", "F32_d4_d21_d14"},
     {{F32, d7, d4, d5}, false, al, "F32 d7 d4 d5", "F32_d7_d4_d5"},
     {{F64, d2, d23, d13}, false, al, "F64 d2 d23 d13", "F64_d2_d23_d13"},
     {{F32, d16, d30, d23}, false, al, "F32 d16 d30 d23", "F32_d16_d30_d23"},
     {{F32, d18, d15, d28}, false, al, "F32 d18 d15 d28", "F32_d18_d15_d28"},
     {{F32, d6, d31, d23}, false, al, "F32 d6 d31 d23", "F32_d6_d31_d23"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-not-f16-vadd-t32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-not-f16-vsub-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(DataType dt,
                                   DRegister rd,
                                   DRegister rn,
                                   DRegister rm);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  unsigned total_error_count = 0;
  MacroAssembler masm(BUF_SIZE);

  masm.UseT32();

  for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
    // Values to pass to the macro-assembler.
    DataType dt = kTests[i].operands.dt;
    DRegister rd = kTests[i].operands.rd;
    DRegister rn = kTests[i].operands.rn;
    DRegister rm = kTests[i].operands.rm;

    int32_t start = masm.GetCursorOffset();
    {
      // We never generate more that 4 bytes, as IT instructions are only
      // allowed for narrow encodings.
      ExactAssemblyScope scope(&masm, 4, ExactAssemblyScope::kMaximumSize);
      if (kTests[i].in_it_block) {
        masm.it(kTests[i].it_condition);
      }
      (masm.*instruction)(dt, rd, rn, rm);
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
#define TEST(mnemonic)                                                      \
  void Test_##mnemonic() {                                                  \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic); \
  }                                                                         \
  Test test_##mnemonic(                                                     \
      "AARCH32_ASSEMBLER_COND_DT_DRT_DRD_DRN_DRM_FLOAT_NOT_F16_" #mnemonic  \
      "_T32",                                                               \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
