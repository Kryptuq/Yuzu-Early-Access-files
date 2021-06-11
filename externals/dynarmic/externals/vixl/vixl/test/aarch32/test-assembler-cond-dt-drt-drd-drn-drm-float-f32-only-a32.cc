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
  M(vceq)                      \
  M(vpadd)                     \
  M(vabd)                      \
  M(vcge)                      \
  M(vcgt)                      \
  M(vcle)                      \
  M(vclt)                      \
  M(vmax)                      \
  M(vmin)                      \
  M(vpmax)                     \
  M(vpmin)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` has
// various layouts across generated tests so it absolutely cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_A32
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
    {{{F32, d1, d2, d30}, false, al, "F32 d1 d2 d30", "F32_d1_d2_d30"},
     {{F32, d12, d5, d13}, false, al, "F32 d12 d5 d13", "F32_d12_d5_d13"},
     {{F32, d3, d16, d2}, false, al, "F32 d3 d16 d2", "F32_d3_d16_d2"},
     {{F32, d21, d26, d7}, false, al, "F32 d21 d26 d7", "F32_d21_d26_d7"},
     {{F32, d30, d1, d16}, false, al, "F32 d30 d1 d16", "F32_d30_d1_d16"},
     {{F32, d17, d8, d6}, false, al, "F32 d17 d8 d6", "F32_d17_d8_d6"},
     {{F32, d26, d1, d7}, false, al, "F32 d26 d1 d7", "F32_d26_d1_d7"},
     {{F32, d17, d6, d28}, false, al, "F32 d17 d6 d28", "F32_d17_d6_d28"},
     {{F32, d30, d6, d27}, false, al, "F32 d30 d6 d27", "F32_d30_d6_d27"},
     {{F32, d6, d13, d11}, false, al, "F32 d6 d13 d11", "F32_d6_d13_d11"},
     {{F32, d30, d17, d15}, false, al, "F32 d30 d17 d15", "F32_d30_d17_d15"},
     {{F32, d10, d8, d14}, false, al, "F32 d10 d8 d14", "F32_d10_d8_d14"},
     {{F32, d17, d4, d0}, false, al, "F32 d17 d4 d0", "F32_d17_d4_d0"},
     {{F32, d25, d5, d14}, false, al, "F32 d25 d5 d14", "F32_d25_d5_d14"},
     {{F32, d30, d16, d0}, false, al, "F32 d30 d16 d0", "F32_d30_d16_d0"},
     {{F32, d22, d21, d2}, false, al, "F32 d22 d21 d2", "F32_d22_d21_d2"},
     {{F32, d15, d10, d12}, false, al, "F32 d15 d10 d12", "F32_d15_d10_d12"},
     {{F32, d6, d26, d20}, false, al, "F32 d6 d26 d20", "F32_d6_d26_d20"},
     {{F32, d2, d7, d6}, false, al, "F32 d2 d7 d6", "F32_d2_d7_d6"},
     {{F32, d27, d13, d30}, false, al, "F32 d27 d13 d30", "F32_d27_d13_d30"},
     {{F32, d6, d12, d13}, false, al, "F32 d6 d12 d13", "F32_d6_d12_d13"},
     {{F32, d6, d17, d17}, false, al, "F32 d6 d17 d17", "F32_d6_d17_d17"},
     {{F32, d18, d7, d6}, false, al, "F32 d18 d7 d6", "F32_d18_d7_d6"},
     {{F32, d24, d18, d8}, false, al, "F32 d24 d18 d8", "F32_d24_d18_d8"},
     {{F32, d27, d3, d3}, false, al, "F32 d27 d3 d3", "F32_d27_d3_d3"},
     {{F32, d20, d12, d4}, false, al, "F32 d20 d12 d4", "F32_d20_d12_d4"},
     {{F32, d16, d23, d26}, false, al, "F32 d16 d23 d26", "F32_d16_d23_d26"},
     {{F32, d14, d24, d11}, false, al, "F32 d14 d24 d11", "F32_d14_d24_d11"},
     {{F32, d14, d29, d28}, false, al, "F32 d14 d29 d28", "F32_d14_d29_d28"},
     {{F32, d20, d16, d11}, false, al, "F32 d20 d16 d11", "F32_d20_d16_d11"},
     {{F32, d28, d21, d9}, false, al, "F32 d28 d21 d9", "F32_d28_d21_d9"},
     {{F32, d0, d11, d1}, false, al, "F32 d0 d11 d1", "F32_d0_d11_d1"},
     {{F32, d19, d10, d4}, false, al, "F32 d19 d10 d4", "F32_d19_d10_d4"},
     {{F32, d19, d28, d20}, false, al, "F32 d19 d28 d20", "F32_d19_d28_d20"},
     {{F32, d16, d15, d6}, false, al, "F32 d16 d15 d6", "F32_d16_d15_d6"},
     {{F32, d12, d15, d2}, false, al, "F32 d12 d15 d2", "F32_d12_d15_d2"},
     {{F32, d0, d31, d2}, false, al, "F32 d0 d31 d2", "F32_d0_d31_d2"},
     {{F32, d6, d22, d19}, false, al, "F32 d6 d22 d19", "F32_d6_d22_d19"},
     {{F32, d26, d30, d0}, false, al, "F32 d26 d30 d0", "F32_d26_d30_d0"},
     {{F32, d5, d19, d18}, false, al, "F32 d5 d19 d18", "F32_d5_d19_d18"},
     {{F32, d7, d3, d3}, false, al, "F32 d7 d3 d3", "F32_d7_d3_d3"},
     {{F32, d17, d7, d20}, false, al, "F32 d17 d7 d20", "F32_d17_d7_d20"},
     {{F32, d29, d30, d24}, false, al, "F32 d29 d30 d24", "F32_d29_d30_d24"},
     {{F32, d26, d4, d30}, false, al, "F32 d26 d4 d30", "F32_d26_d4_d30"},
     {{F32, d30, d27, d9}, false, al, "F32 d30 d27 d9", "F32_d30_d27_d9"},
     {{F32, d6, d24, d17}, false, al, "F32 d6 d24 d17", "F32_d6_d24_d17"},
     {{F32, d16, d21, d20}, false, al, "F32 d16 d21 d20", "F32_d16_d21_d20"},
     {{F32, d6, d1, d12}, false, al, "F32 d6 d1 d12", "F32_d6_d1_d12"},
     {{F32, d2, d26, d18}, false, al, "F32 d2 d26 d18", "F32_d2_d26_d18"},
     {{F32, d21, d31, d7}, false, al, "F32 d21 d31 d7", "F32_d21_d31_d7"},
     {{F32, d8, d18, d17}, false, al, "F32 d8 d18 d17", "F32_d8_d18_d17"},
     {{F32, d18, d11, d8}, false, al, "F32 d18 d11 d8", "F32_d18_d11_d8"},
     {{F32, d25, d14, d5}, false, al, "F32 d25 d14 d5", "F32_d25_d14_d5"},
     {{F32, d0, d9, d5}, false, al, "F32 d0 d9 d5", "F32_d0_d9_d5"},
     {{F32, d14, d12, d7}, false, al, "F32 d14 d12 d7", "F32_d14_d12_d7"},
     {{F32, d23, d0, d22}, false, al, "F32 d23 d0 d22", "F32_d23_d0_d22"},
     {{F32, d23, d31, d10}, false, al, "F32 d23 d31 d10", "F32_d23_d31_d10"},
     {{F32, d3, d19, d28}, false, al, "F32 d3 d19 d28", "F32_d3_d19_d28"},
     {{F32, d9, d26, d15}, false, al, "F32 d9 d26 d15", "F32_d9_d26_d15"},
     {{F32, d17, d13, d26}, false, al, "F32 d17 d13 d26", "F32_d17_d13_d26"},
     {{F32, d2, d26, d5}, false, al, "F32 d2 d26 d5", "F32_d2_d26_d5"},
     {{F32, d18, d28, d11}, false, al, "F32 d18 d28 d11", "F32_d18_d28_d11"},
     {{F32, d20, d10, d2}, false, al, "F32 d20 d10 d2", "F32_d20_d10_d2"},
     {{F32, d19, d3, d9}, false, al, "F32 d19 d3 d9", "F32_d19_d3_d9"},
     {{F32, d4, d29, d17}, false, al, "F32 d4 d29 d17", "F32_d4_d29_d17"},
     {{F32, d22, d1, d31}, false, al, "F32 d22 d1 d31", "F32_d22_d1_d31"},
     {{F32, d29, d2, d17}, false, al, "F32 d29 d2 d17", "F32_d29_d2_d17"},
     {{F32, d18, d8, d10}, false, al, "F32 d18 d8 d10", "F32_d18_d8_d10"},
     {{F32, d16, d8, d22}, false, al, "F32 d16 d8 d22", "F32_d16_d8_d22"},
     {{F32, d0, d27, d19}, false, al, "F32 d0 d27 d19", "F32_d0_d27_d19"},
     {{F32, d26, d23, d0}, false, al, "F32 d26 d23 d0", "F32_d26_d23_d0"},
     {{F32, d30, d21, d22}, false, al, "F32 d30 d21 d22", "F32_d30_d21_d22"},
     {{F32, d31, d31, d8}, false, al, "F32 d31 d31 d8", "F32_d31_d31_d8"},
     {{F32, d4, d7, d2}, false, al, "F32 d4 d7 d2", "F32_d4_d7_d2"},
     {{F32, d8, d22, d31}, false, al, "F32 d8 d22 d31", "F32_d8_d22_d31"},
     {{F32, d27, d3, d14}, false, al, "F32 d27 d3 d14", "F32_d27_d3_d14"},
     {{F32, d11, d26, d6}, false, al, "F32 d11 d26 d6", "F32_d11_d26_d6"},
     {{F32, d20, d24, d4}, false, al, "F32 d20 d24 d4", "F32_d20_d24_d4"},
     {{F32, d1, d8, d3}, false, al, "F32 d1 d8 d3", "F32_d1_d8_d3"},
     {{F32, d27, d17, d17}, false, al, "F32 d27 d17 d17", "F32_d27_d17_d17"},
     {{F32, d25, d21, d8}, false, al, "F32 d25 d21 d8", "F32_d25_d21_d8"},
     {{F32, d6, d0, d15}, false, al, "F32 d6 d0 d15", "F32_d6_d0_d15"},
     {{F32, d4, d10, d9}, false, al, "F32 d4 d10 d9", "F32_d4_d10_d9"},
     {{F32, d13, d26, d11}, false, al, "F32 d13 d26 d11", "F32_d13_d26_d11"},
     {{F32, d7, d6, d18}, false, al, "F32 d7 d6 d18", "F32_d7_d6_d18"},
     {{F32, d11, d7, d10}, false, al, "F32 d11 d7 d10", "F32_d11_d7_d10"},
     {{F32, d17, d3, d10}, false, al, "F32 d17 d3 d10", "F32_d17_d3_d10"},
     {{F32, d9, d19, d11}, false, al, "F32 d9 d19 d11", "F32_d9_d19_d11"},
     {{F32, d9, d22, d10}, false, al, "F32 d9 d22 d10", "F32_d9_d22_d10"},
     {{F32, d30, d0, d21}, false, al, "F32 d30 d0 d21", "F32_d30_d0_d21"},
     {{F32, d13, d1, d15}, false, al, "F32 d13 d1 d15", "F32_d13_d1_d15"},
     {{F32, d27, d11, d10}, false, al, "F32 d27 d11 d10", "F32_d27_d11_d10"},
     {{F32, d7, d26, d9}, false, al, "F32 d7 d26 d9", "F32_d7_d26_d9"},
     {{F32, d1, d12, d21}, false, al, "F32 d1 d12 d21", "F32_d1_d12_d21"},
     {{F32, d26, d28, d30}, false, al, "F32 d26 d28 d30", "F32_d26_d28_d30"},
     {{F32, d31, d14, d5}, false, al, "F32 d31 d14 d5", "F32_d31_d14_d5"},
     {{F32, d29, d12, d7}, false, al, "F32 d29 d12 d7", "F32_d29_d12_d7"},
     {{F32, d19, d30, d1}, false, al, "F32 d19 d30 d1", "F32_d19_d30_d1"},
     {{F32, d4, d1, d26}, false, al, "F32 d4 d1 d26", "F32_d4_d1_d26"},
     {{F32, d2, d13, d10}, false, al, "F32 d2 d13 d10", "F32_d2_d13_d10"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vabd-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vceq-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vcge-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vcgt-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vcle-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vclt-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vmax-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vmin-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vpadd-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vpmax-a32.h"
#include "aarch32/traces/assembler-cond-dt-drt-drd-drn-drm-float-f32-only-vpmin-a32.h"


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

  masm.UseA32();

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
      "AARCH32_ASSEMBLER_COND_DT_DRT_DRD_DRN_DRM_FLOAT_F32_ONLY_" #mnemonic \
      "_A32",                                                               \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
