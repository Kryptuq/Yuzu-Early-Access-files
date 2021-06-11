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
  M(adds)                      \
  M(subs)


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
  int32_t immediate;
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
    {{{al, r0, r0, 0}, false, al, "al r0 r0 0", "al_r0_r0_0"},
     {{al, r0, r0, 1}, false, al, "al r0 r0 1", "al_r0_r0_1"},
     {{al, r0, r0, 2}, false, al, "al r0 r0 2", "al_r0_r0_2"},
     {{al, r0, r0, 3}, false, al, "al r0 r0 3", "al_r0_r0_3"},
     {{al, r0, r0, 4}, false, al, "al r0 r0 4", "al_r0_r0_4"},
     {{al, r0, r0, 5}, false, al, "al r0 r0 5", "al_r0_r0_5"},
     {{al, r0, r0, 6}, false, al, "al r0 r0 6", "al_r0_r0_6"},
     {{al, r0, r0, 7}, false, al, "al r0 r0 7", "al_r0_r0_7"},
     {{al, r0, r1, 0}, false, al, "al r0 r1 0", "al_r0_r1_0"},
     {{al, r0, r1, 1}, false, al, "al r0 r1 1", "al_r0_r1_1"},
     {{al, r0, r1, 2}, false, al, "al r0 r1 2", "al_r0_r1_2"},
     {{al, r0, r1, 3}, false, al, "al r0 r1 3", "al_r0_r1_3"},
     {{al, r0, r1, 4}, false, al, "al r0 r1 4", "al_r0_r1_4"},
     {{al, r0, r1, 5}, false, al, "al r0 r1 5", "al_r0_r1_5"},
     {{al, r0, r1, 6}, false, al, "al r0 r1 6", "al_r0_r1_6"},
     {{al, r0, r1, 7}, false, al, "al r0 r1 7", "al_r0_r1_7"},
     {{al, r0, r2, 0}, false, al, "al r0 r2 0", "al_r0_r2_0"},
     {{al, r0, r2, 1}, false, al, "al r0 r2 1", "al_r0_r2_1"},
     {{al, r0, r2, 2}, false, al, "al r0 r2 2", "al_r0_r2_2"},
     {{al, r0, r2, 3}, false, al, "al r0 r2 3", "al_r0_r2_3"},
     {{al, r0, r2, 4}, false, al, "al r0 r2 4", "al_r0_r2_4"},
     {{al, r0, r2, 5}, false, al, "al r0 r2 5", "al_r0_r2_5"},
     {{al, r0, r2, 6}, false, al, "al r0 r2 6", "al_r0_r2_6"},
     {{al, r0, r2, 7}, false, al, "al r0 r2 7", "al_r0_r2_7"},
     {{al, r0, r3, 0}, false, al, "al r0 r3 0", "al_r0_r3_0"},
     {{al, r0, r3, 1}, false, al, "al r0 r3 1", "al_r0_r3_1"},
     {{al, r0, r3, 2}, false, al, "al r0 r3 2", "al_r0_r3_2"},
     {{al, r0, r3, 3}, false, al, "al r0 r3 3", "al_r0_r3_3"},
     {{al, r0, r3, 4}, false, al, "al r0 r3 4", "al_r0_r3_4"},
     {{al, r0, r3, 5}, false, al, "al r0 r3 5", "al_r0_r3_5"},
     {{al, r0, r3, 6}, false, al, "al r0 r3 6", "al_r0_r3_6"},
     {{al, r0, r3, 7}, false, al, "al r0 r3 7", "al_r0_r3_7"},
     {{al, r0, r4, 0}, false, al, "al r0 r4 0", "al_r0_r4_0"},
     {{al, r0, r4, 1}, false, al, "al r0 r4 1", "al_r0_r4_1"},
     {{al, r0, r4, 2}, false, al, "al r0 r4 2", "al_r0_r4_2"},
     {{al, r0, r4, 3}, false, al, "al r0 r4 3", "al_r0_r4_3"},
     {{al, r0, r4, 4}, false, al, "al r0 r4 4", "al_r0_r4_4"},
     {{al, r0, r4, 5}, false, al, "al r0 r4 5", "al_r0_r4_5"},
     {{al, r0, r4, 6}, false, al, "al r0 r4 6", "al_r0_r4_6"},
     {{al, r0, r4, 7}, false, al, "al r0 r4 7", "al_r0_r4_7"},
     {{al, r0, r5, 0}, false, al, "al r0 r5 0", "al_r0_r5_0"},
     {{al, r0, r5, 1}, false, al, "al r0 r5 1", "al_r0_r5_1"},
     {{al, r0, r5, 2}, false, al, "al r0 r5 2", "al_r0_r5_2"},
     {{al, r0, r5, 3}, false, al, "al r0 r5 3", "al_r0_r5_3"},
     {{al, r0, r5, 4}, false, al, "al r0 r5 4", "al_r0_r5_4"},
     {{al, r0, r5, 5}, false, al, "al r0 r5 5", "al_r0_r5_5"},
     {{al, r0, r5, 6}, false, al, "al r0 r5 6", "al_r0_r5_6"},
     {{al, r0, r5, 7}, false, al, "al r0 r5 7", "al_r0_r5_7"},
     {{al, r0, r6, 0}, false, al, "al r0 r6 0", "al_r0_r6_0"},
     {{al, r0, r6, 1}, false, al, "al r0 r6 1", "al_r0_r6_1"},
     {{al, r0, r6, 2}, false, al, "al r0 r6 2", "al_r0_r6_2"},
     {{al, r0, r6, 3}, false, al, "al r0 r6 3", "al_r0_r6_3"},
     {{al, r0, r6, 4}, false, al, "al r0 r6 4", "al_r0_r6_4"},
     {{al, r0, r6, 5}, false, al, "al r0 r6 5", "al_r0_r6_5"},
     {{al, r0, r6, 6}, false, al, "al r0 r6 6", "al_r0_r6_6"},
     {{al, r0, r6, 7}, false, al, "al r0 r6 7", "al_r0_r6_7"},
     {{al, r0, r7, 0}, false, al, "al r0 r7 0", "al_r0_r7_0"},
     {{al, r0, r7, 1}, false, al, "al r0 r7 1", "al_r0_r7_1"},
     {{al, r0, r7, 2}, false, al, "al r0 r7 2", "al_r0_r7_2"},
     {{al, r0, r7, 3}, false, al, "al r0 r7 3", "al_r0_r7_3"},
     {{al, r0, r7, 4}, false, al, "al r0 r7 4", "al_r0_r7_4"},
     {{al, r0, r7, 5}, false, al, "al r0 r7 5", "al_r0_r7_5"},
     {{al, r0, r7, 6}, false, al, "al r0 r7 6", "al_r0_r7_6"},
     {{al, r0, r7, 7}, false, al, "al r0 r7 7", "al_r0_r7_7"},
     {{al, r1, r0, 0}, false, al, "al r1 r0 0", "al_r1_r0_0"},
     {{al, r1, r0, 1}, false, al, "al r1 r0 1", "al_r1_r0_1"},
     {{al, r1, r0, 2}, false, al, "al r1 r0 2", "al_r1_r0_2"},
     {{al, r1, r0, 3}, false, al, "al r1 r0 3", "al_r1_r0_3"},
     {{al, r1, r0, 4}, false, al, "al r1 r0 4", "al_r1_r0_4"},
     {{al, r1, r0, 5}, false, al, "al r1 r0 5", "al_r1_r0_5"},
     {{al, r1, r0, 6}, false, al, "al r1 r0 6", "al_r1_r0_6"},
     {{al, r1, r0, 7}, false, al, "al r1 r0 7", "al_r1_r0_7"},
     {{al, r1, r1, 0}, false, al, "al r1 r1 0", "al_r1_r1_0"},
     {{al, r1, r1, 1}, false, al, "al r1 r1 1", "al_r1_r1_1"},
     {{al, r1, r1, 2}, false, al, "al r1 r1 2", "al_r1_r1_2"},
     {{al, r1, r1, 3}, false, al, "al r1 r1 3", "al_r1_r1_3"},
     {{al, r1, r1, 4}, false, al, "al r1 r1 4", "al_r1_r1_4"},
     {{al, r1, r1, 5}, false, al, "al r1 r1 5", "al_r1_r1_5"},
     {{al, r1, r1, 6}, false, al, "al r1 r1 6", "al_r1_r1_6"},
     {{al, r1, r1, 7}, false, al, "al r1 r1 7", "al_r1_r1_7"},
     {{al, r1, r2, 0}, false, al, "al r1 r2 0", "al_r1_r2_0"},
     {{al, r1, r2, 1}, false, al, "al r1 r2 1", "al_r1_r2_1"},
     {{al, r1, r2, 2}, false, al, "al r1 r2 2", "al_r1_r2_2"},
     {{al, r1, r2, 3}, false, al, "al r1 r2 3", "al_r1_r2_3"},
     {{al, r1, r2, 4}, false, al, "al r1 r2 4", "al_r1_r2_4"},
     {{al, r1, r2, 5}, false, al, "al r1 r2 5", "al_r1_r2_5"},
     {{al, r1, r2, 6}, false, al, "al r1 r2 6", "al_r1_r2_6"},
     {{al, r1, r2, 7}, false, al, "al r1 r2 7", "al_r1_r2_7"},
     {{al, r1, r3, 0}, false, al, "al r1 r3 0", "al_r1_r3_0"},
     {{al, r1, r3, 1}, false, al, "al r1 r3 1", "al_r1_r3_1"},
     {{al, r1, r3, 2}, false, al, "al r1 r3 2", "al_r1_r3_2"},
     {{al, r1, r3, 3}, false, al, "al r1 r3 3", "al_r1_r3_3"},
     {{al, r1, r3, 4}, false, al, "al r1 r3 4", "al_r1_r3_4"},
     {{al, r1, r3, 5}, false, al, "al r1 r3 5", "al_r1_r3_5"},
     {{al, r1, r3, 6}, false, al, "al r1 r3 6", "al_r1_r3_6"},
     {{al, r1, r3, 7}, false, al, "al r1 r3 7", "al_r1_r3_7"},
     {{al, r1, r4, 0}, false, al, "al r1 r4 0", "al_r1_r4_0"},
     {{al, r1, r4, 1}, false, al, "al r1 r4 1", "al_r1_r4_1"},
     {{al, r1, r4, 2}, false, al, "al r1 r4 2", "al_r1_r4_2"},
     {{al, r1, r4, 3}, false, al, "al r1 r4 3", "al_r1_r4_3"},
     {{al, r1, r4, 4}, false, al, "al r1 r4 4", "al_r1_r4_4"},
     {{al, r1, r4, 5}, false, al, "al r1 r4 5", "al_r1_r4_5"},
     {{al, r1, r4, 6}, false, al, "al r1 r4 6", "al_r1_r4_6"},
     {{al, r1, r4, 7}, false, al, "al r1 r4 7", "al_r1_r4_7"},
     {{al, r1, r5, 0}, false, al, "al r1 r5 0", "al_r1_r5_0"},
     {{al, r1, r5, 1}, false, al, "al r1 r5 1", "al_r1_r5_1"},
     {{al, r1, r5, 2}, false, al, "al r1 r5 2", "al_r1_r5_2"},
     {{al, r1, r5, 3}, false, al, "al r1 r5 3", "al_r1_r5_3"},
     {{al, r1, r5, 4}, false, al, "al r1 r5 4", "al_r1_r5_4"},
     {{al, r1, r5, 5}, false, al, "al r1 r5 5", "al_r1_r5_5"},
     {{al, r1, r5, 6}, false, al, "al r1 r5 6", "al_r1_r5_6"},
     {{al, r1, r5, 7}, false, al, "al r1 r5 7", "al_r1_r5_7"},
     {{al, r1, r6, 0}, false, al, "al r1 r6 0", "al_r1_r6_0"},
     {{al, r1, r6, 1}, false, al, "al r1 r6 1", "al_r1_r6_1"},
     {{al, r1, r6, 2}, false, al, "al r1 r6 2", "al_r1_r6_2"},
     {{al, r1, r6, 3}, false, al, "al r1 r6 3", "al_r1_r6_3"},
     {{al, r1, r6, 4}, false, al, "al r1 r6 4", "al_r1_r6_4"},
     {{al, r1, r6, 5}, false, al, "al r1 r6 5", "al_r1_r6_5"},
     {{al, r1, r6, 6}, false, al, "al r1 r6 6", "al_r1_r6_6"},
     {{al, r1, r6, 7}, false, al, "al r1 r6 7", "al_r1_r6_7"},
     {{al, r1, r7, 0}, false, al, "al r1 r7 0", "al_r1_r7_0"},
     {{al, r1, r7, 1}, false, al, "al r1 r7 1", "al_r1_r7_1"},
     {{al, r1, r7, 2}, false, al, "al r1 r7 2", "al_r1_r7_2"},
     {{al, r1, r7, 3}, false, al, "al r1 r7 3", "al_r1_r7_3"},
     {{al, r1, r7, 4}, false, al, "al r1 r7 4", "al_r1_r7_4"},
     {{al, r1, r7, 5}, false, al, "al r1 r7 5", "al_r1_r7_5"},
     {{al, r1, r7, 6}, false, al, "al r1 r7 6", "al_r1_r7_6"},
     {{al, r1, r7, 7}, false, al, "al r1 r7 7", "al_r1_r7_7"},
     {{al, r2, r0, 0}, false, al, "al r2 r0 0", "al_r2_r0_0"},
     {{al, r2, r0, 1}, false, al, "al r2 r0 1", "al_r2_r0_1"},
     {{al, r2, r0, 2}, false, al, "al r2 r0 2", "al_r2_r0_2"},
     {{al, r2, r0, 3}, false, al, "al r2 r0 3", "al_r2_r0_3"},
     {{al, r2, r0, 4}, false, al, "al r2 r0 4", "al_r2_r0_4"},
     {{al, r2, r0, 5}, false, al, "al r2 r0 5", "al_r2_r0_5"},
     {{al, r2, r0, 6}, false, al, "al r2 r0 6", "al_r2_r0_6"},
     {{al, r2, r0, 7}, false, al, "al r2 r0 7", "al_r2_r0_7"},
     {{al, r2, r1, 0}, false, al, "al r2 r1 0", "al_r2_r1_0"},
     {{al, r2, r1, 1}, false, al, "al r2 r1 1", "al_r2_r1_1"},
     {{al, r2, r1, 2}, false, al, "al r2 r1 2", "al_r2_r1_2"},
     {{al, r2, r1, 3}, false, al, "al r2 r1 3", "al_r2_r1_3"},
     {{al, r2, r1, 4}, false, al, "al r2 r1 4", "al_r2_r1_4"},
     {{al, r2, r1, 5}, false, al, "al r2 r1 5", "al_r2_r1_5"},
     {{al, r2, r1, 6}, false, al, "al r2 r1 6", "al_r2_r1_6"},
     {{al, r2, r1, 7}, false, al, "al r2 r1 7", "al_r2_r1_7"},
     {{al, r2, r2, 0}, false, al, "al r2 r2 0", "al_r2_r2_0"},
     {{al, r2, r2, 1}, false, al, "al r2 r2 1", "al_r2_r2_1"},
     {{al, r2, r2, 2}, false, al, "al r2 r2 2", "al_r2_r2_2"},
     {{al, r2, r2, 3}, false, al, "al r2 r2 3", "al_r2_r2_3"},
     {{al, r2, r2, 4}, false, al, "al r2 r2 4", "al_r2_r2_4"},
     {{al, r2, r2, 5}, false, al, "al r2 r2 5", "al_r2_r2_5"},
     {{al, r2, r2, 6}, false, al, "al r2 r2 6", "al_r2_r2_6"},
     {{al, r2, r2, 7}, false, al, "al r2 r2 7", "al_r2_r2_7"},
     {{al, r2, r3, 0}, false, al, "al r2 r3 0", "al_r2_r3_0"},
     {{al, r2, r3, 1}, false, al, "al r2 r3 1", "al_r2_r3_1"},
     {{al, r2, r3, 2}, false, al, "al r2 r3 2", "al_r2_r3_2"},
     {{al, r2, r3, 3}, false, al, "al r2 r3 3", "al_r2_r3_3"},
     {{al, r2, r3, 4}, false, al, "al r2 r3 4", "al_r2_r3_4"},
     {{al, r2, r3, 5}, false, al, "al r2 r3 5", "al_r2_r3_5"},
     {{al, r2, r3, 6}, false, al, "al r2 r3 6", "al_r2_r3_6"},
     {{al, r2, r3, 7}, false, al, "al r2 r3 7", "al_r2_r3_7"},
     {{al, r2, r4, 0}, false, al, "al r2 r4 0", "al_r2_r4_0"},
     {{al, r2, r4, 1}, false, al, "al r2 r4 1", "al_r2_r4_1"},
     {{al, r2, r4, 2}, false, al, "al r2 r4 2", "al_r2_r4_2"},
     {{al, r2, r4, 3}, false, al, "al r2 r4 3", "al_r2_r4_3"},
     {{al, r2, r4, 4}, false, al, "al r2 r4 4", "al_r2_r4_4"},
     {{al, r2, r4, 5}, false, al, "al r2 r4 5", "al_r2_r4_5"},
     {{al, r2, r4, 6}, false, al, "al r2 r4 6", "al_r2_r4_6"},
     {{al, r2, r4, 7}, false, al, "al r2 r4 7", "al_r2_r4_7"},
     {{al, r2, r5, 0}, false, al, "al r2 r5 0", "al_r2_r5_0"},
     {{al, r2, r5, 1}, false, al, "al r2 r5 1", "al_r2_r5_1"},
     {{al, r2, r5, 2}, false, al, "al r2 r5 2", "al_r2_r5_2"},
     {{al, r2, r5, 3}, false, al, "al r2 r5 3", "al_r2_r5_3"},
     {{al, r2, r5, 4}, false, al, "al r2 r5 4", "al_r2_r5_4"},
     {{al, r2, r5, 5}, false, al, "al r2 r5 5", "al_r2_r5_5"},
     {{al, r2, r5, 6}, false, al, "al r2 r5 6", "al_r2_r5_6"},
     {{al, r2, r5, 7}, false, al, "al r2 r5 7", "al_r2_r5_7"},
     {{al, r2, r6, 0}, false, al, "al r2 r6 0", "al_r2_r6_0"},
     {{al, r2, r6, 1}, false, al, "al r2 r6 1", "al_r2_r6_1"},
     {{al, r2, r6, 2}, false, al, "al r2 r6 2", "al_r2_r6_2"},
     {{al, r2, r6, 3}, false, al, "al r2 r6 3", "al_r2_r6_3"},
     {{al, r2, r6, 4}, false, al, "al r2 r6 4", "al_r2_r6_4"},
     {{al, r2, r6, 5}, false, al, "al r2 r6 5", "al_r2_r6_5"},
     {{al, r2, r6, 6}, false, al, "al r2 r6 6", "al_r2_r6_6"},
     {{al, r2, r6, 7}, false, al, "al r2 r6 7", "al_r2_r6_7"},
     {{al, r2, r7, 0}, false, al, "al r2 r7 0", "al_r2_r7_0"},
     {{al, r2, r7, 1}, false, al, "al r2 r7 1", "al_r2_r7_1"},
     {{al, r2, r7, 2}, false, al, "al r2 r7 2", "al_r2_r7_2"},
     {{al, r2, r7, 3}, false, al, "al r2 r7 3", "al_r2_r7_3"},
     {{al, r2, r7, 4}, false, al, "al r2 r7 4", "al_r2_r7_4"},
     {{al, r2, r7, 5}, false, al, "al r2 r7 5", "al_r2_r7_5"},
     {{al, r2, r7, 6}, false, al, "al r2 r7 6", "al_r2_r7_6"},
     {{al, r2, r7, 7}, false, al, "al r2 r7 7", "al_r2_r7_7"},
     {{al, r3, r0, 0}, false, al, "al r3 r0 0", "al_r3_r0_0"},
     {{al, r3, r0, 1}, false, al, "al r3 r0 1", "al_r3_r0_1"},
     {{al, r3, r0, 2}, false, al, "al r3 r0 2", "al_r3_r0_2"},
     {{al, r3, r0, 3}, false, al, "al r3 r0 3", "al_r3_r0_3"},
     {{al, r3, r0, 4}, false, al, "al r3 r0 4", "al_r3_r0_4"},
     {{al, r3, r0, 5}, false, al, "al r3 r0 5", "al_r3_r0_5"},
     {{al, r3, r0, 6}, false, al, "al r3 r0 6", "al_r3_r0_6"},
     {{al, r3, r0, 7}, false, al, "al r3 r0 7", "al_r3_r0_7"},
     {{al, r3, r1, 0}, false, al, "al r3 r1 0", "al_r3_r1_0"},
     {{al, r3, r1, 1}, false, al, "al r3 r1 1", "al_r3_r1_1"},
     {{al, r3, r1, 2}, false, al, "al r3 r1 2", "al_r3_r1_2"},
     {{al, r3, r1, 3}, false, al, "al r3 r1 3", "al_r3_r1_3"},
     {{al, r3, r1, 4}, false, al, "al r3 r1 4", "al_r3_r1_4"},
     {{al, r3, r1, 5}, false, al, "al r3 r1 5", "al_r3_r1_5"},
     {{al, r3, r1, 6}, false, al, "al r3 r1 6", "al_r3_r1_6"},
     {{al, r3, r1, 7}, false, al, "al r3 r1 7", "al_r3_r1_7"},
     {{al, r3, r2, 0}, false, al, "al r3 r2 0", "al_r3_r2_0"},
     {{al, r3, r2, 1}, false, al, "al r3 r2 1", "al_r3_r2_1"},
     {{al, r3, r2, 2}, false, al, "al r3 r2 2", "al_r3_r2_2"},
     {{al, r3, r2, 3}, false, al, "al r3 r2 3", "al_r3_r2_3"},
     {{al, r3, r2, 4}, false, al, "al r3 r2 4", "al_r3_r2_4"},
     {{al, r3, r2, 5}, false, al, "al r3 r2 5", "al_r3_r2_5"},
     {{al, r3, r2, 6}, false, al, "al r3 r2 6", "al_r3_r2_6"},
     {{al, r3, r2, 7}, false, al, "al r3 r2 7", "al_r3_r2_7"},
     {{al, r3, r3, 0}, false, al, "al r3 r3 0", "al_r3_r3_0"},
     {{al, r3, r3, 1}, false, al, "al r3 r3 1", "al_r3_r3_1"},
     {{al, r3, r3, 2}, false, al, "al r3 r3 2", "al_r3_r3_2"},
     {{al, r3, r3, 3}, false, al, "al r3 r3 3", "al_r3_r3_3"},
     {{al, r3, r3, 4}, false, al, "al r3 r3 4", "al_r3_r3_4"},
     {{al, r3, r3, 5}, false, al, "al r3 r3 5", "al_r3_r3_5"},
     {{al, r3, r3, 6}, false, al, "al r3 r3 6", "al_r3_r3_6"},
     {{al, r3, r3, 7}, false, al, "al r3 r3 7", "al_r3_r3_7"},
     {{al, r3, r4, 0}, false, al, "al r3 r4 0", "al_r3_r4_0"},
     {{al, r3, r4, 1}, false, al, "al r3 r4 1", "al_r3_r4_1"},
     {{al, r3, r4, 2}, false, al, "al r3 r4 2", "al_r3_r4_2"},
     {{al, r3, r4, 3}, false, al, "al r3 r4 3", "al_r3_r4_3"},
     {{al, r3, r4, 4}, false, al, "al r3 r4 4", "al_r3_r4_4"},
     {{al, r3, r4, 5}, false, al, "al r3 r4 5", "al_r3_r4_5"},
     {{al, r3, r4, 6}, false, al, "al r3 r4 6", "al_r3_r4_6"},
     {{al, r3, r4, 7}, false, al, "al r3 r4 7", "al_r3_r4_7"},
     {{al, r3, r5, 0}, false, al, "al r3 r5 0", "al_r3_r5_0"},
     {{al, r3, r5, 1}, false, al, "al r3 r5 1", "al_r3_r5_1"},
     {{al, r3, r5, 2}, false, al, "al r3 r5 2", "al_r3_r5_2"},
     {{al, r3, r5, 3}, false, al, "al r3 r5 3", "al_r3_r5_3"},
     {{al, r3, r5, 4}, false, al, "al r3 r5 4", "al_r3_r5_4"},
     {{al, r3, r5, 5}, false, al, "al r3 r5 5", "al_r3_r5_5"},
     {{al, r3, r5, 6}, false, al, "al r3 r5 6", "al_r3_r5_6"},
     {{al, r3, r5, 7}, false, al, "al r3 r5 7", "al_r3_r5_7"},
     {{al, r3, r6, 0}, false, al, "al r3 r6 0", "al_r3_r6_0"},
     {{al, r3, r6, 1}, false, al, "al r3 r6 1", "al_r3_r6_1"},
     {{al, r3, r6, 2}, false, al, "al r3 r6 2", "al_r3_r6_2"},
     {{al, r3, r6, 3}, false, al, "al r3 r6 3", "al_r3_r6_3"},
     {{al, r3, r6, 4}, false, al, "al r3 r6 4", "al_r3_r6_4"},
     {{al, r3, r6, 5}, false, al, "al r3 r6 5", "al_r3_r6_5"},
     {{al, r3, r6, 6}, false, al, "al r3 r6 6", "al_r3_r6_6"},
     {{al, r3, r6, 7}, false, al, "al r3 r6 7", "al_r3_r6_7"},
     {{al, r3, r7, 0}, false, al, "al r3 r7 0", "al_r3_r7_0"},
     {{al, r3, r7, 1}, false, al, "al r3 r7 1", "al_r3_r7_1"},
     {{al, r3, r7, 2}, false, al, "al r3 r7 2", "al_r3_r7_2"},
     {{al, r3, r7, 3}, false, al, "al r3 r7 3", "al_r3_r7_3"},
     {{al, r3, r7, 4}, false, al, "al r3 r7 4", "al_r3_r7_4"},
     {{al, r3, r7, 5}, false, al, "al r3 r7 5", "al_r3_r7_5"},
     {{al, r3, r7, 6}, false, al, "al r3 r7 6", "al_r3_r7_6"},
     {{al, r3, r7, 7}, false, al, "al r3 r7 7", "al_r3_r7_7"},
     {{al, r4, r0, 0}, false, al, "al r4 r0 0", "al_r4_r0_0"},
     {{al, r4, r0, 1}, false, al, "al r4 r0 1", "al_r4_r0_1"},
     {{al, r4, r0, 2}, false, al, "al r4 r0 2", "al_r4_r0_2"},
     {{al, r4, r0, 3}, false, al, "al r4 r0 3", "al_r4_r0_3"},
     {{al, r4, r0, 4}, false, al, "al r4 r0 4", "al_r4_r0_4"},
     {{al, r4, r0, 5}, false, al, "al r4 r0 5", "al_r4_r0_5"},
     {{al, r4, r0, 6}, false, al, "al r4 r0 6", "al_r4_r0_6"},
     {{al, r4, r0, 7}, false, al, "al r4 r0 7", "al_r4_r0_7"},
     {{al, r4, r1, 0}, false, al, "al r4 r1 0", "al_r4_r1_0"},
     {{al, r4, r1, 1}, false, al, "al r4 r1 1", "al_r4_r1_1"},
     {{al, r4, r1, 2}, false, al, "al r4 r1 2", "al_r4_r1_2"},
     {{al, r4, r1, 3}, false, al, "al r4 r1 3", "al_r4_r1_3"},
     {{al, r4, r1, 4}, false, al, "al r4 r1 4", "al_r4_r1_4"},
     {{al, r4, r1, 5}, false, al, "al r4 r1 5", "al_r4_r1_5"},
     {{al, r4, r1, 6}, false, al, "al r4 r1 6", "al_r4_r1_6"},
     {{al, r4, r1, 7}, false, al, "al r4 r1 7", "al_r4_r1_7"},
     {{al, r4, r2, 0}, false, al, "al r4 r2 0", "al_r4_r2_0"},
     {{al, r4, r2, 1}, false, al, "al r4 r2 1", "al_r4_r2_1"},
     {{al, r4, r2, 2}, false, al, "al r4 r2 2", "al_r4_r2_2"},
     {{al, r4, r2, 3}, false, al, "al r4 r2 3", "al_r4_r2_3"},
     {{al, r4, r2, 4}, false, al, "al r4 r2 4", "al_r4_r2_4"},
     {{al, r4, r2, 5}, false, al, "al r4 r2 5", "al_r4_r2_5"},
     {{al, r4, r2, 6}, false, al, "al r4 r2 6", "al_r4_r2_6"},
     {{al, r4, r2, 7}, false, al, "al r4 r2 7", "al_r4_r2_7"},
     {{al, r4, r3, 0}, false, al, "al r4 r3 0", "al_r4_r3_0"},
     {{al, r4, r3, 1}, false, al, "al r4 r3 1", "al_r4_r3_1"},
     {{al, r4, r3, 2}, false, al, "al r4 r3 2", "al_r4_r3_2"},
     {{al, r4, r3, 3}, false, al, "al r4 r3 3", "al_r4_r3_3"},
     {{al, r4, r3, 4}, false, al, "al r4 r3 4", "al_r4_r3_4"},
     {{al, r4, r3, 5}, false, al, "al r4 r3 5", "al_r4_r3_5"},
     {{al, r4, r3, 6}, false, al, "al r4 r3 6", "al_r4_r3_6"},
     {{al, r4, r3, 7}, false, al, "al r4 r3 7", "al_r4_r3_7"},
     {{al, r4, r4, 0}, false, al, "al r4 r4 0", "al_r4_r4_0"},
     {{al, r4, r4, 1}, false, al, "al r4 r4 1", "al_r4_r4_1"},
     {{al, r4, r4, 2}, false, al, "al r4 r4 2", "al_r4_r4_2"},
     {{al, r4, r4, 3}, false, al, "al r4 r4 3", "al_r4_r4_3"},
     {{al, r4, r4, 4}, false, al, "al r4 r4 4", "al_r4_r4_4"},
     {{al, r4, r4, 5}, false, al, "al r4 r4 5", "al_r4_r4_5"},
     {{al, r4, r4, 6}, false, al, "al r4 r4 6", "al_r4_r4_6"},
     {{al, r4, r4, 7}, false, al, "al r4 r4 7", "al_r4_r4_7"},
     {{al, r4, r5, 0}, false, al, "al r4 r5 0", "al_r4_r5_0"},
     {{al, r4, r5, 1}, false, al, "al r4 r5 1", "al_r4_r5_1"},
     {{al, r4, r5, 2}, false, al, "al r4 r5 2", "al_r4_r5_2"},
     {{al, r4, r5, 3}, false, al, "al r4 r5 3", "al_r4_r5_3"},
     {{al, r4, r5, 4}, false, al, "al r4 r5 4", "al_r4_r5_4"},
     {{al, r4, r5, 5}, false, al, "al r4 r5 5", "al_r4_r5_5"},
     {{al, r4, r5, 6}, false, al, "al r4 r5 6", "al_r4_r5_6"},
     {{al, r4, r5, 7}, false, al, "al r4 r5 7", "al_r4_r5_7"},
     {{al, r4, r6, 0}, false, al, "al r4 r6 0", "al_r4_r6_0"},
     {{al, r4, r6, 1}, false, al, "al r4 r6 1", "al_r4_r6_1"},
     {{al, r4, r6, 2}, false, al, "al r4 r6 2", "al_r4_r6_2"},
     {{al, r4, r6, 3}, false, al, "al r4 r6 3", "al_r4_r6_3"},
     {{al, r4, r6, 4}, false, al, "al r4 r6 4", "al_r4_r6_4"},
     {{al, r4, r6, 5}, false, al, "al r4 r6 5", "al_r4_r6_5"},
     {{al, r4, r6, 6}, false, al, "al r4 r6 6", "al_r4_r6_6"},
     {{al, r4, r6, 7}, false, al, "al r4 r6 7", "al_r4_r6_7"},
     {{al, r4, r7, 0}, false, al, "al r4 r7 0", "al_r4_r7_0"},
     {{al, r4, r7, 1}, false, al, "al r4 r7 1", "al_r4_r7_1"},
     {{al, r4, r7, 2}, false, al, "al r4 r7 2", "al_r4_r7_2"},
     {{al, r4, r7, 3}, false, al, "al r4 r7 3", "al_r4_r7_3"},
     {{al, r4, r7, 4}, false, al, "al r4 r7 4", "al_r4_r7_4"},
     {{al, r4, r7, 5}, false, al, "al r4 r7 5", "al_r4_r7_5"},
     {{al, r4, r7, 6}, false, al, "al r4 r7 6", "al_r4_r7_6"},
     {{al, r4, r7, 7}, false, al, "al r4 r7 7", "al_r4_r7_7"},
     {{al, r5, r0, 0}, false, al, "al r5 r0 0", "al_r5_r0_0"},
     {{al, r5, r0, 1}, false, al, "al r5 r0 1", "al_r5_r0_1"},
     {{al, r5, r0, 2}, false, al, "al r5 r0 2", "al_r5_r0_2"},
     {{al, r5, r0, 3}, false, al, "al r5 r0 3", "al_r5_r0_3"},
     {{al, r5, r0, 4}, false, al, "al r5 r0 4", "al_r5_r0_4"},
     {{al, r5, r0, 5}, false, al, "al r5 r0 5", "al_r5_r0_5"},
     {{al, r5, r0, 6}, false, al, "al r5 r0 6", "al_r5_r0_6"},
     {{al, r5, r0, 7}, false, al, "al r5 r0 7", "al_r5_r0_7"},
     {{al, r5, r1, 0}, false, al, "al r5 r1 0", "al_r5_r1_0"},
     {{al, r5, r1, 1}, false, al, "al r5 r1 1", "al_r5_r1_1"},
     {{al, r5, r1, 2}, false, al, "al r5 r1 2", "al_r5_r1_2"},
     {{al, r5, r1, 3}, false, al, "al r5 r1 3", "al_r5_r1_3"},
     {{al, r5, r1, 4}, false, al, "al r5 r1 4", "al_r5_r1_4"},
     {{al, r5, r1, 5}, false, al, "al r5 r1 5", "al_r5_r1_5"},
     {{al, r5, r1, 6}, false, al, "al r5 r1 6", "al_r5_r1_6"},
     {{al, r5, r1, 7}, false, al, "al r5 r1 7", "al_r5_r1_7"},
     {{al, r5, r2, 0}, false, al, "al r5 r2 0", "al_r5_r2_0"},
     {{al, r5, r2, 1}, false, al, "al r5 r2 1", "al_r5_r2_1"},
     {{al, r5, r2, 2}, false, al, "al r5 r2 2", "al_r5_r2_2"},
     {{al, r5, r2, 3}, false, al, "al r5 r2 3", "al_r5_r2_3"},
     {{al, r5, r2, 4}, false, al, "al r5 r2 4", "al_r5_r2_4"},
     {{al, r5, r2, 5}, false, al, "al r5 r2 5", "al_r5_r2_5"},
     {{al, r5, r2, 6}, false, al, "al r5 r2 6", "al_r5_r2_6"},
     {{al, r5, r2, 7}, false, al, "al r5 r2 7", "al_r5_r2_7"},
     {{al, r5, r3, 0}, false, al, "al r5 r3 0", "al_r5_r3_0"},
     {{al, r5, r3, 1}, false, al, "al r5 r3 1", "al_r5_r3_1"},
     {{al, r5, r3, 2}, false, al, "al r5 r3 2", "al_r5_r3_2"},
     {{al, r5, r3, 3}, false, al, "al r5 r3 3", "al_r5_r3_3"},
     {{al, r5, r3, 4}, false, al, "al r5 r3 4", "al_r5_r3_4"},
     {{al, r5, r3, 5}, false, al, "al r5 r3 5", "al_r5_r3_5"},
     {{al, r5, r3, 6}, false, al, "al r5 r3 6", "al_r5_r3_6"},
     {{al, r5, r3, 7}, false, al, "al r5 r3 7", "al_r5_r3_7"},
     {{al, r5, r4, 0}, false, al, "al r5 r4 0", "al_r5_r4_0"},
     {{al, r5, r4, 1}, false, al, "al r5 r4 1", "al_r5_r4_1"},
     {{al, r5, r4, 2}, false, al, "al r5 r4 2", "al_r5_r4_2"},
     {{al, r5, r4, 3}, false, al, "al r5 r4 3", "al_r5_r4_3"},
     {{al, r5, r4, 4}, false, al, "al r5 r4 4", "al_r5_r4_4"},
     {{al, r5, r4, 5}, false, al, "al r5 r4 5", "al_r5_r4_5"},
     {{al, r5, r4, 6}, false, al, "al r5 r4 6", "al_r5_r4_6"},
     {{al, r5, r4, 7}, false, al, "al r5 r4 7", "al_r5_r4_7"},
     {{al, r5, r5, 0}, false, al, "al r5 r5 0", "al_r5_r5_0"},
     {{al, r5, r5, 1}, false, al, "al r5 r5 1", "al_r5_r5_1"},
     {{al, r5, r5, 2}, false, al, "al r5 r5 2", "al_r5_r5_2"},
     {{al, r5, r5, 3}, false, al, "al r5 r5 3", "al_r5_r5_3"},
     {{al, r5, r5, 4}, false, al, "al r5 r5 4", "al_r5_r5_4"},
     {{al, r5, r5, 5}, false, al, "al r5 r5 5", "al_r5_r5_5"},
     {{al, r5, r5, 6}, false, al, "al r5 r5 6", "al_r5_r5_6"},
     {{al, r5, r5, 7}, false, al, "al r5 r5 7", "al_r5_r5_7"},
     {{al, r5, r6, 0}, false, al, "al r5 r6 0", "al_r5_r6_0"},
     {{al, r5, r6, 1}, false, al, "al r5 r6 1", "al_r5_r6_1"},
     {{al, r5, r6, 2}, false, al, "al r5 r6 2", "al_r5_r6_2"},
     {{al, r5, r6, 3}, false, al, "al r5 r6 3", "al_r5_r6_3"},
     {{al, r5, r6, 4}, false, al, "al r5 r6 4", "al_r5_r6_4"},
     {{al, r5, r6, 5}, false, al, "al r5 r6 5", "al_r5_r6_5"},
     {{al, r5, r6, 6}, false, al, "al r5 r6 6", "al_r5_r6_6"},
     {{al, r5, r6, 7}, false, al, "al r5 r6 7", "al_r5_r6_7"},
     {{al, r5, r7, 0}, false, al, "al r5 r7 0", "al_r5_r7_0"},
     {{al, r5, r7, 1}, false, al, "al r5 r7 1", "al_r5_r7_1"},
     {{al, r5, r7, 2}, false, al, "al r5 r7 2", "al_r5_r7_2"},
     {{al, r5, r7, 3}, false, al, "al r5 r7 3", "al_r5_r7_3"},
     {{al, r5, r7, 4}, false, al, "al r5 r7 4", "al_r5_r7_4"},
     {{al, r5, r7, 5}, false, al, "al r5 r7 5", "al_r5_r7_5"},
     {{al, r5, r7, 6}, false, al, "al r5 r7 6", "al_r5_r7_6"},
     {{al, r5, r7, 7}, false, al, "al r5 r7 7", "al_r5_r7_7"},
     {{al, r6, r0, 0}, false, al, "al r6 r0 0", "al_r6_r0_0"},
     {{al, r6, r0, 1}, false, al, "al r6 r0 1", "al_r6_r0_1"},
     {{al, r6, r0, 2}, false, al, "al r6 r0 2", "al_r6_r0_2"},
     {{al, r6, r0, 3}, false, al, "al r6 r0 3", "al_r6_r0_3"},
     {{al, r6, r0, 4}, false, al, "al r6 r0 4", "al_r6_r0_4"},
     {{al, r6, r0, 5}, false, al, "al r6 r0 5", "al_r6_r0_5"},
     {{al, r6, r0, 6}, false, al, "al r6 r0 6", "al_r6_r0_6"},
     {{al, r6, r0, 7}, false, al, "al r6 r0 7", "al_r6_r0_7"},
     {{al, r6, r1, 0}, false, al, "al r6 r1 0", "al_r6_r1_0"},
     {{al, r6, r1, 1}, false, al, "al r6 r1 1", "al_r6_r1_1"},
     {{al, r6, r1, 2}, false, al, "al r6 r1 2", "al_r6_r1_2"},
     {{al, r6, r1, 3}, false, al, "al r6 r1 3", "al_r6_r1_3"},
     {{al, r6, r1, 4}, false, al, "al r6 r1 4", "al_r6_r1_4"},
     {{al, r6, r1, 5}, false, al, "al r6 r1 5", "al_r6_r1_5"},
     {{al, r6, r1, 6}, false, al, "al r6 r1 6", "al_r6_r1_6"},
     {{al, r6, r1, 7}, false, al, "al r6 r1 7", "al_r6_r1_7"},
     {{al, r6, r2, 0}, false, al, "al r6 r2 0", "al_r6_r2_0"},
     {{al, r6, r2, 1}, false, al, "al r6 r2 1", "al_r6_r2_1"},
     {{al, r6, r2, 2}, false, al, "al r6 r2 2", "al_r6_r2_2"},
     {{al, r6, r2, 3}, false, al, "al r6 r2 3", "al_r6_r2_3"},
     {{al, r6, r2, 4}, false, al, "al r6 r2 4", "al_r6_r2_4"},
     {{al, r6, r2, 5}, false, al, "al r6 r2 5", "al_r6_r2_5"},
     {{al, r6, r2, 6}, false, al, "al r6 r2 6", "al_r6_r2_6"},
     {{al, r6, r2, 7}, false, al, "al r6 r2 7", "al_r6_r2_7"},
     {{al, r6, r3, 0}, false, al, "al r6 r3 0", "al_r6_r3_0"},
     {{al, r6, r3, 1}, false, al, "al r6 r3 1", "al_r6_r3_1"},
     {{al, r6, r3, 2}, false, al, "al r6 r3 2", "al_r6_r3_2"},
     {{al, r6, r3, 3}, false, al, "al r6 r3 3", "al_r6_r3_3"},
     {{al, r6, r3, 4}, false, al, "al r6 r3 4", "al_r6_r3_4"},
     {{al, r6, r3, 5}, false, al, "al r6 r3 5", "al_r6_r3_5"},
     {{al, r6, r3, 6}, false, al, "al r6 r3 6", "al_r6_r3_6"},
     {{al, r6, r3, 7}, false, al, "al r6 r3 7", "al_r6_r3_7"},
     {{al, r6, r4, 0}, false, al, "al r6 r4 0", "al_r6_r4_0"},
     {{al, r6, r4, 1}, false, al, "al r6 r4 1", "al_r6_r4_1"},
     {{al, r6, r4, 2}, false, al, "al r6 r4 2", "al_r6_r4_2"},
     {{al, r6, r4, 3}, false, al, "al r6 r4 3", "al_r6_r4_3"},
     {{al, r6, r4, 4}, false, al, "al r6 r4 4", "al_r6_r4_4"},
     {{al, r6, r4, 5}, false, al, "al r6 r4 5", "al_r6_r4_5"},
     {{al, r6, r4, 6}, false, al, "al r6 r4 6", "al_r6_r4_6"},
     {{al, r6, r4, 7}, false, al, "al r6 r4 7", "al_r6_r4_7"},
     {{al, r6, r5, 0}, false, al, "al r6 r5 0", "al_r6_r5_0"},
     {{al, r6, r5, 1}, false, al, "al r6 r5 1", "al_r6_r5_1"},
     {{al, r6, r5, 2}, false, al, "al r6 r5 2", "al_r6_r5_2"},
     {{al, r6, r5, 3}, false, al, "al r6 r5 3", "al_r6_r5_3"},
     {{al, r6, r5, 4}, false, al, "al r6 r5 4", "al_r6_r5_4"},
     {{al, r6, r5, 5}, false, al, "al r6 r5 5", "al_r6_r5_5"},
     {{al, r6, r5, 6}, false, al, "al r6 r5 6", "al_r6_r5_6"},
     {{al, r6, r5, 7}, false, al, "al r6 r5 7", "al_r6_r5_7"},
     {{al, r6, r6, 0}, false, al, "al r6 r6 0", "al_r6_r6_0"},
     {{al, r6, r6, 1}, false, al, "al r6 r6 1", "al_r6_r6_1"},
     {{al, r6, r6, 2}, false, al, "al r6 r6 2", "al_r6_r6_2"},
     {{al, r6, r6, 3}, false, al, "al r6 r6 3", "al_r6_r6_3"},
     {{al, r6, r6, 4}, false, al, "al r6 r6 4", "al_r6_r6_4"},
     {{al, r6, r6, 5}, false, al, "al r6 r6 5", "al_r6_r6_5"},
     {{al, r6, r6, 6}, false, al, "al r6 r6 6", "al_r6_r6_6"},
     {{al, r6, r6, 7}, false, al, "al r6 r6 7", "al_r6_r6_7"},
     {{al, r6, r7, 0}, false, al, "al r6 r7 0", "al_r6_r7_0"},
     {{al, r6, r7, 1}, false, al, "al r6 r7 1", "al_r6_r7_1"},
     {{al, r6, r7, 2}, false, al, "al r6 r7 2", "al_r6_r7_2"},
     {{al, r6, r7, 3}, false, al, "al r6 r7 3", "al_r6_r7_3"},
     {{al, r6, r7, 4}, false, al, "al r6 r7 4", "al_r6_r7_4"},
     {{al, r6, r7, 5}, false, al, "al r6 r7 5", "al_r6_r7_5"},
     {{al, r6, r7, 6}, false, al, "al r6 r7 6", "al_r6_r7_6"},
     {{al, r6, r7, 7}, false, al, "al r6 r7 7", "al_r6_r7_7"},
     {{al, r7, r0, 0}, false, al, "al r7 r0 0", "al_r7_r0_0"},
     {{al, r7, r0, 1}, false, al, "al r7 r0 1", "al_r7_r0_1"},
     {{al, r7, r0, 2}, false, al, "al r7 r0 2", "al_r7_r0_2"},
     {{al, r7, r0, 3}, false, al, "al r7 r0 3", "al_r7_r0_3"},
     {{al, r7, r0, 4}, false, al, "al r7 r0 4", "al_r7_r0_4"},
     {{al, r7, r0, 5}, false, al, "al r7 r0 5", "al_r7_r0_5"},
     {{al, r7, r0, 6}, false, al, "al r7 r0 6", "al_r7_r0_6"},
     {{al, r7, r0, 7}, false, al, "al r7 r0 7", "al_r7_r0_7"},
     {{al, r7, r1, 0}, false, al, "al r7 r1 0", "al_r7_r1_0"},
     {{al, r7, r1, 1}, false, al, "al r7 r1 1", "al_r7_r1_1"},
     {{al, r7, r1, 2}, false, al, "al r7 r1 2", "al_r7_r1_2"},
     {{al, r7, r1, 3}, false, al, "al r7 r1 3", "al_r7_r1_3"},
     {{al, r7, r1, 4}, false, al, "al r7 r1 4", "al_r7_r1_4"},
     {{al, r7, r1, 5}, false, al, "al r7 r1 5", "al_r7_r1_5"},
     {{al, r7, r1, 6}, false, al, "al r7 r1 6", "al_r7_r1_6"},
     {{al, r7, r1, 7}, false, al, "al r7 r1 7", "al_r7_r1_7"},
     {{al, r7, r2, 0}, false, al, "al r7 r2 0", "al_r7_r2_0"},
     {{al, r7, r2, 1}, false, al, "al r7 r2 1", "al_r7_r2_1"},
     {{al, r7, r2, 2}, false, al, "al r7 r2 2", "al_r7_r2_2"},
     {{al, r7, r2, 3}, false, al, "al r7 r2 3", "al_r7_r2_3"},
     {{al, r7, r2, 4}, false, al, "al r7 r2 4", "al_r7_r2_4"},
     {{al, r7, r2, 5}, false, al, "al r7 r2 5", "al_r7_r2_5"},
     {{al, r7, r2, 6}, false, al, "al r7 r2 6", "al_r7_r2_6"},
     {{al, r7, r2, 7}, false, al, "al r7 r2 7", "al_r7_r2_7"},
     {{al, r7, r3, 0}, false, al, "al r7 r3 0", "al_r7_r3_0"},
     {{al, r7, r3, 1}, false, al, "al r7 r3 1", "al_r7_r3_1"},
     {{al, r7, r3, 2}, false, al, "al r7 r3 2", "al_r7_r3_2"},
     {{al, r7, r3, 3}, false, al, "al r7 r3 3", "al_r7_r3_3"},
     {{al, r7, r3, 4}, false, al, "al r7 r3 4", "al_r7_r3_4"},
     {{al, r7, r3, 5}, false, al, "al r7 r3 5", "al_r7_r3_5"},
     {{al, r7, r3, 6}, false, al, "al r7 r3 6", "al_r7_r3_6"},
     {{al, r7, r3, 7}, false, al, "al r7 r3 7", "al_r7_r3_7"},
     {{al, r7, r4, 0}, false, al, "al r7 r4 0", "al_r7_r4_0"},
     {{al, r7, r4, 1}, false, al, "al r7 r4 1", "al_r7_r4_1"},
     {{al, r7, r4, 2}, false, al, "al r7 r4 2", "al_r7_r4_2"},
     {{al, r7, r4, 3}, false, al, "al r7 r4 3", "al_r7_r4_3"},
     {{al, r7, r4, 4}, false, al, "al r7 r4 4", "al_r7_r4_4"},
     {{al, r7, r4, 5}, false, al, "al r7 r4 5", "al_r7_r4_5"},
     {{al, r7, r4, 6}, false, al, "al r7 r4 6", "al_r7_r4_6"},
     {{al, r7, r4, 7}, false, al, "al r7 r4 7", "al_r7_r4_7"},
     {{al, r7, r5, 0}, false, al, "al r7 r5 0", "al_r7_r5_0"},
     {{al, r7, r5, 1}, false, al, "al r7 r5 1", "al_r7_r5_1"},
     {{al, r7, r5, 2}, false, al, "al r7 r5 2", "al_r7_r5_2"},
     {{al, r7, r5, 3}, false, al, "al r7 r5 3", "al_r7_r5_3"},
     {{al, r7, r5, 4}, false, al, "al r7 r5 4", "al_r7_r5_4"},
     {{al, r7, r5, 5}, false, al, "al r7 r5 5", "al_r7_r5_5"},
     {{al, r7, r5, 6}, false, al, "al r7 r5 6", "al_r7_r5_6"},
     {{al, r7, r5, 7}, false, al, "al r7 r5 7", "al_r7_r5_7"},
     {{al, r7, r6, 0}, false, al, "al r7 r6 0", "al_r7_r6_0"},
     {{al, r7, r6, 1}, false, al, "al r7 r6 1", "al_r7_r6_1"},
     {{al, r7, r6, 2}, false, al, "al r7 r6 2", "al_r7_r6_2"},
     {{al, r7, r6, 3}, false, al, "al r7 r6 3", "al_r7_r6_3"},
     {{al, r7, r6, 4}, false, al, "al r7 r6 4", "al_r7_r6_4"},
     {{al, r7, r6, 5}, false, al, "al r7 r6 5", "al_r7_r6_5"},
     {{al, r7, r6, 6}, false, al, "al r7 r6 6", "al_r7_r6_6"},
     {{al, r7, r6, 7}, false, al, "al r7 r6 7", "al_r7_r6_7"},
     {{al, r7, r7, 0}, false, al, "al r7 r7 0", "al_r7_r7_0"},
     {{al, r7, r7, 1}, false, al, "al r7 r7 1", "al_r7_r7_1"},
     {{al, r7, r7, 2}, false, al, "al r7 r7 2", "al_r7_r7_2"},
     {{al, r7, r7, 3}, false, al, "al r7 r7 3", "al_r7_r7_3"},
     {{al, r7, r7, 4}, false, al, "al r7 r7 4", "al_r7_r7_4"},
     {{al, r7, r7, 5}, false, al, "al r7 r7 5", "al_r7_r7_5"},
     {{al, r7, r7, 6}, false, al, "al r7 r7 6", "al_r7_r7_6"},
     {{al, r7, r7, 7}, false, al, "al r7 r7 7", "al_r7_r7_7"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rdlow-rnlow-operand-immediate-imm3-adds-t32.h"
#include "aarch32/traces/assembler-cond-rdlow-rnlow-operand-immediate-imm3-subs-t32.h"


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
    int32_t immediate = kTests[i].operands.immediate;
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
#define TEST(mnemonic)                                                       \
  void Test_##mnemonic() {                                                   \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic);  \
  }                                                                          \
  Test test_##mnemonic(                                                      \
      "AARCH32_ASSEMBLER_COND_RDLOW_RNLOW_OPERAND_IMMEDIATE_IMM3_" #mnemonic \
      "_T32",                                                                \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
