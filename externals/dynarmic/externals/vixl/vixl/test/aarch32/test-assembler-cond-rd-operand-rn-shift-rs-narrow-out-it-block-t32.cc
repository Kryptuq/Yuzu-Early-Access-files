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
#define FOREACH_INSTRUCTION(M) M(movs)


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
  ShiftType shift;
  Register rs;
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
    {{{al, r0, r0, LSL, r0}, false, al, "al r0 r0 LSL r0", "al_r0_r0_LSL_r0"},
     {{al, r0, r0, LSL, r1}, false, al, "al r0 r0 LSL r1", "al_r0_r0_LSL_r1"},
     {{al, r0, r0, LSL, r2}, false, al, "al r0 r0 LSL r2", "al_r0_r0_LSL_r2"},
     {{al, r0, r0, LSL, r3}, false, al, "al r0 r0 LSL r3", "al_r0_r0_LSL_r3"},
     {{al, r0, r0, LSL, r4}, false, al, "al r0 r0 LSL r4", "al_r0_r0_LSL_r4"},
     {{al, r0, r0, LSL, r5}, false, al, "al r0 r0 LSL r5", "al_r0_r0_LSL_r5"},
     {{al, r0, r0, LSL, r6}, false, al, "al r0 r0 LSL r6", "al_r0_r0_LSL_r6"},
     {{al, r0, r0, LSL, r7}, false, al, "al r0 r0 LSL r7", "al_r0_r0_LSL_r7"},
     {{al, r0, r0, LSR, r0}, false, al, "al r0 r0 LSR r0", "al_r0_r0_LSR_r0"},
     {{al, r0, r0, LSR, r1}, false, al, "al r0 r0 LSR r1", "al_r0_r0_LSR_r1"},
     {{al, r0, r0, LSR, r2}, false, al, "al r0 r0 LSR r2", "al_r0_r0_LSR_r2"},
     {{al, r0, r0, LSR, r3}, false, al, "al r0 r0 LSR r3", "al_r0_r0_LSR_r3"},
     {{al, r0, r0, LSR, r4}, false, al, "al r0 r0 LSR r4", "al_r0_r0_LSR_r4"},
     {{al, r0, r0, LSR, r5}, false, al, "al r0 r0 LSR r5", "al_r0_r0_LSR_r5"},
     {{al, r0, r0, LSR, r6}, false, al, "al r0 r0 LSR r6", "al_r0_r0_LSR_r6"},
     {{al, r0, r0, LSR, r7}, false, al, "al r0 r0 LSR r7", "al_r0_r0_LSR_r7"},
     {{al, r0, r0, ASR, r0}, false, al, "al r0 r0 ASR r0", "al_r0_r0_ASR_r0"},
     {{al, r0, r0, ASR, r1}, false, al, "al r0 r0 ASR r1", "al_r0_r0_ASR_r1"},
     {{al, r0, r0, ASR, r2}, false, al, "al r0 r0 ASR r2", "al_r0_r0_ASR_r2"},
     {{al, r0, r0, ASR, r3}, false, al, "al r0 r0 ASR r3", "al_r0_r0_ASR_r3"},
     {{al, r0, r0, ASR, r4}, false, al, "al r0 r0 ASR r4", "al_r0_r0_ASR_r4"},
     {{al, r0, r0, ASR, r5}, false, al, "al r0 r0 ASR r5", "al_r0_r0_ASR_r5"},
     {{al, r0, r0, ASR, r6}, false, al, "al r0 r0 ASR r6", "al_r0_r0_ASR_r6"},
     {{al, r0, r0, ASR, r7}, false, al, "al r0 r0 ASR r7", "al_r0_r0_ASR_r7"},
     {{al, r0, r0, ROR, r0}, false, al, "al r0 r0 ROR r0", "al_r0_r0_ROR_r0"},
     {{al, r0, r0, ROR, r1}, false, al, "al r0 r0 ROR r1", "al_r0_r0_ROR_r1"},
     {{al, r0, r0, ROR, r2}, false, al, "al r0 r0 ROR r2", "al_r0_r0_ROR_r2"},
     {{al, r0, r0, ROR, r3}, false, al, "al r0 r0 ROR r3", "al_r0_r0_ROR_r3"},
     {{al, r0, r0, ROR, r4}, false, al, "al r0 r0 ROR r4", "al_r0_r0_ROR_r4"},
     {{al, r0, r0, ROR, r5}, false, al, "al r0 r0 ROR r5", "al_r0_r0_ROR_r5"},
     {{al, r0, r0, ROR, r6}, false, al, "al r0 r0 ROR r6", "al_r0_r0_ROR_r6"},
     {{al, r0, r0, ROR, r7}, false, al, "al r0 r0 ROR r7", "al_r0_r0_ROR_r7"},
     {{al, r1, r1, LSL, r0}, false, al, "al r1 r1 LSL r0", "al_r1_r1_LSL_r0"},
     {{al, r1, r1, LSL, r1}, false, al, "al r1 r1 LSL r1", "al_r1_r1_LSL_r1"},
     {{al, r1, r1, LSL, r2}, false, al, "al r1 r1 LSL r2", "al_r1_r1_LSL_r2"},
     {{al, r1, r1, LSL, r3}, false, al, "al r1 r1 LSL r3", "al_r1_r1_LSL_r3"},
     {{al, r1, r1, LSL, r4}, false, al, "al r1 r1 LSL r4", "al_r1_r1_LSL_r4"},
     {{al, r1, r1, LSL, r5}, false, al, "al r1 r1 LSL r5", "al_r1_r1_LSL_r5"},
     {{al, r1, r1, LSL, r6}, false, al, "al r1 r1 LSL r6", "al_r1_r1_LSL_r6"},
     {{al, r1, r1, LSL, r7}, false, al, "al r1 r1 LSL r7", "al_r1_r1_LSL_r7"},
     {{al, r1, r1, LSR, r0}, false, al, "al r1 r1 LSR r0", "al_r1_r1_LSR_r0"},
     {{al, r1, r1, LSR, r1}, false, al, "al r1 r1 LSR r1", "al_r1_r1_LSR_r1"},
     {{al, r1, r1, LSR, r2}, false, al, "al r1 r1 LSR r2", "al_r1_r1_LSR_r2"},
     {{al, r1, r1, LSR, r3}, false, al, "al r1 r1 LSR r3", "al_r1_r1_LSR_r3"},
     {{al, r1, r1, LSR, r4}, false, al, "al r1 r1 LSR r4", "al_r1_r1_LSR_r4"},
     {{al, r1, r1, LSR, r5}, false, al, "al r1 r1 LSR r5", "al_r1_r1_LSR_r5"},
     {{al, r1, r1, LSR, r6}, false, al, "al r1 r1 LSR r6", "al_r1_r1_LSR_r6"},
     {{al, r1, r1, LSR, r7}, false, al, "al r1 r1 LSR r7", "al_r1_r1_LSR_r7"},
     {{al, r1, r1, ASR, r0}, false, al, "al r1 r1 ASR r0", "al_r1_r1_ASR_r0"},
     {{al, r1, r1, ASR, r1}, false, al, "al r1 r1 ASR r1", "al_r1_r1_ASR_r1"},
     {{al, r1, r1, ASR, r2}, false, al, "al r1 r1 ASR r2", "al_r1_r1_ASR_r2"},
     {{al, r1, r1, ASR, r3}, false, al, "al r1 r1 ASR r3", "al_r1_r1_ASR_r3"},
     {{al, r1, r1, ASR, r4}, false, al, "al r1 r1 ASR r4", "al_r1_r1_ASR_r4"},
     {{al, r1, r1, ASR, r5}, false, al, "al r1 r1 ASR r5", "al_r1_r1_ASR_r5"},
     {{al, r1, r1, ASR, r6}, false, al, "al r1 r1 ASR r6", "al_r1_r1_ASR_r6"},
     {{al, r1, r1, ASR, r7}, false, al, "al r1 r1 ASR r7", "al_r1_r1_ASR_r7"},
     {{al, r1, r1, ROR, r0}, false, al, "al r1 r1 ROR r0", "al_r1_r1_ROR_r0"},
     {{al, r1, r1, ROR, r1}, false, al, "al r1 r1 ROR r1", "al_r1_r1_ROR_r1"},
     {{al, r1, r1, ROR, r2}, false, al, "al r1 r1 ROR r2", "al_r1_r1_ROR_r2"},
     {{al, r1, r1, ROR, r3}, false, al, "al r1 r1 ROR r3", "al_r1_r1_ROR_r3"},
     {{al, r1, r1, ROR, r4}, false, al, "al r1 r1 ROR r4", "al_r1_r1_ROR_r4"},
     {{al, r1, r1, ROR, r5}, false, al, "al r1 r1 ROR r5", "al_r1_r1_ROR_r5"},
     {{al, r1, r1, ROR, r6}, false, al, "al r1 r1 ROR r6", "al_r1_r1_ROR_r6"},
     {{al, r1, r1, ROR, r7}, false, al, "al r1 r1 ROR r7", "al_r1_r1_ROR_r7"},
     {{al, r2, r2, LSL, r0}, false, al, "al r2 r2 LSL r0", "al_r2_r2_LSL_r0"},
     {{al, r2, r2, LSL, r1}, false, al, "al r2 r2 LSL r1", "al_r2_r2_LSL_r1"},
     {{al, r2, r2, LSL, r2}, false, al, "al r2 r2 LSL r2", "al_r2_r2_LSL_r2"},
     {{al, r2, r2, LSL, r3}, false, al, "al r2 r2 LSL r3", "al_r2_r2_LSL_r3"},
     {{al, r2, r2, LSL, r4}, false, al, "al r2 r2 LSL r4", "al_r2_r2_LSL_r4"},
     {{al, r2, r2, LSL, r5}, false, al, "al r2 r2 LSL r5", "al_r2_r2_LSL_r5"},
     {{al, r2, r2, LSL, r6}, false, al, "al r2 r2 LSL r6", "al_r2_r2_LSL_r6"},
     {{al, r2, r2, LSL, r7}, false, al, "al r2 r2 LSL r7", "al_r2_r2_LSL_r7"},
     {{al, r2, r2, LSR, r0}, false, al, "al r2 r2 LSR r0", "al_r2_r2_LSR_r0"},
     {{al, r2, r2, LSR, r1}, false, al, "al r2 r2 LSR r1", "al_r2_r2_LSR_r1"},
     {{al, r2, r2, LSR, r2}, false, al, "al r2 r2 LSR r2", "al_r2_r2_LSR_r2"},
     {{al, r2, r2, LSR, r3}, false, al, "al r2 r2 LSR r3", "al_r2_r2_LSR_r3"},
     {{al, r2, r2, LSR, r4}, false, al, "al r2 r2 LSR r4", "al_r2_r2_LSR_r4"},
     {{al, r2, r2, LSR, r5}, false, al, "al r2 r2 LSR r5", "al_r2_r2_LSR_r5"},
     {{al, r2, r2, LSR, r6}, false, al, "al r2 r2 LSR r6", "al_r2_r2_LSR_r6"},
     {{al, r2, r2, LSR, r7}, false, al, "al r2 r2 LSR r7", "al_r2_r2_LSR_r7"},
     {{al, r2, r2, ASR, r0}, false, al, "al r2 r2 ASR r0", "al_r2_r2_ASR_r0"},
     {{al, r2, r2, ASR, r1}, false, al, "al r2 r2 ASR r1", "al_r2_r2_ASR_r1"},
     {{al, r2, r2, ASR, r2}, false, al, "al r2 r2 ASR r2", "al_r2_r2_ASR_r2"},
     {{al, r2, r2, ASR, r3}, false, al, "al r2 r2 ASR r3", "al_r2_r2_ASR_r3"},
     {{al, r2, r2, ASR, r4}, false, al, "al r2 r2 ASR r4", "al_r2_r2_ASR_r4"},
     {{al, r2, r2, ASR, r5}, false, al, "al r2 r2 ASR r5", "al_r2_r2_ASR_r5"},
     {{al, r2, r2, ASR, r6}, false, al, "al r2 r2 ASR r6", "al_r2_r2_ASR_r6"},
     {{al, r2, r2, ASR, r7}, false, al, "al r2 r2 ASR r7", "al_r2_r2_ASR_r7"},
     {{al, r2, r2, ROR, r0}, false, al, "al r2 r2 ROR r0", "al_r2_r2_ROR_r0"},
     {{al, r2, r2, ROR, r1}, false, al, "al r2 r2 ROR r1", "al_r2_r2_ROR_r1"},
     {{al, r2, r2, ROR, r2}, false, al, "al r2 r2 ROR r2", "al_r2_r2_ROR_r2"},
     {{al, r2, r2, ROR, r3}, false, al, "al r2 r2 ROR r3", "al_r2_r2_ROR_r3"},
     {{al, r2, r2, ROR, r4}, false, al, "al r2 r2 ROR r4", "al_r2_r2_ROR_r4"},
     {{al, r2, r2, ROR, r5}, false, al, "al r2 r2 ROR r5", "al_r2_r2_ROR_r5"},
     {{al, r2, r2, ROR, r6}, false, al, "al r2 r2 ROR r6", "al_r2_r2_ROR_r6"},
     {{al, r2, r2, ROR, r7}, false, al, "al r2 r2 ROR r7", "al_r2_r2_ROR_r7"},
     {{al, r3, r3, LSL, r0}, false, al, "al r3 r3 LSL r0", "al_r3_r3_LSL_r0"},
     {{al, r3, r3, LSL, r1}, false, al, "al r3 r3 LSL r1", "al_r3_r3_LSL_r1"},
     {{al, r3, r3, LSL, r2}, false, al, "al r3 r3 LSL r2", "al_r3_r3_LSL_r2"},
     {{al, r3, r3, LSL, r3}, false, al, "al r3 r3 LSL r3", "al_r3_r3_LSL_r3"},
     {{al, r3, r3, LSL, r4}, false, al, "al r3 r3 LSL r4", "al_r3_r3_LSL_r4"},
     {{al, r3, r3, LSL, r5}, false, al, "al r3 r3 LSL r5", "al_r3_r3_LSL_r5"},
     {{al, r3, r3, LSL, r6}, false, al, "al r3 r3 LSL r6", "al_r3_r3_LSL_r6"},
     {{al, r3, r3, LSL, r7}, false, al, "al r3 r3 LSL r7", "al_r3_r3_LSL_r7"},
     {{al, r3, r3, LSR, r0}, false, al, "al r3 r3 LSR r0", "al_r3_r3_LSR_r0"},
     {{al, r3, r3, LSR, r1}, false, al, "al r3 r3 LSR r1", "al_r3_r3_LSR_r1"},
     {{al, r3, r3, LSR, r2}, false, al, "al r3 r3 LSR r2", "al_r3_r3_LSR_r2"},
     {{al, r3, r3, LSR, r3}, false, al, "al r3 r3 LSR r3", "al_r3_r3_LSR_r3"},
     {{al, r3, r3, LSR, r4}, false, al, "al r3 r3 LSR r4", "al_r3_r3_LSR_r4"},
     {{al, r3, r3, LSR, r5}, false, al, "al r3 r3 LSR r5", "al_r3_r3_LSR_r5"},
     {{al, r3, r3, LSR, r6}, false, al, "al r3 r3 LSR r6", "al_r3_r3_LSR_r6"},
     {{al, r3, r3, LSR, r7}, false, al, "al r3 r3 LSR r7", "al_r3_r3_LSR_r7"},
     {{al, r3, r3, ASR, r0}, false, al, "al r3 r3 ASR r0", "al_r3_r3_ASR_r0"},
     {{al, r3, r3, ASR, r1}, false, al, "al r3 r3 ASR r1", "al_r3_r3_ASR_r1"},
     {{al, r3, r3, ASR, r2}, false, al, "al r3 r3 ASR r2", "al_r3_r3_ASR_r2"},
     {{al, r3, r3, ASR, r3}, false, al, "al r3 r3 ASR r3", "al_r3_r3_ASR_r3"},
     {{al, r3, r3, ASR, r4}, false, al, "al r3 r3 ASR r4", "al_r3_r3_ASR_r4"},
     {{al, r3, r3, ASR, r5}, false, al, "al r3 r3 ASR r5", "al_r3_r3_ASR_r5"},
     {{al, r3, r3, ASR, r6}, false, al, "al r3 r3 ASR r6", "al_r3_r3_ASR_r6"},
     {{al, r3, r3, ASR, r7}, false, al, "al r3 r3 ASR r7", "al_r3_r3_ASR_r7"},
     {{al, r3, r3, ROR, r0}, false, al, "al r3 r3 ROR r0", "al_r3_r3_ROR_r0"},
     {{al, r3, r3, ROR, r1}, false, al, "al r3 r3 ROR r1", "al_r3_r3_ROR_r1"},
     {{al, r3, r3, ROR, r2}, false, al, "al r3 r3 ROR r2", "al_r3_r3_ROR_r2"},
     {{al, r3, r3, ROR, r3}, false, al, "al r3 r3 ROR r3", "al_r3_r3_ROR_r3"},
     {{al, r3, r3, ROR, r4}, false, al, "al r3 r3 ROR r4", "al_r3_r3_ROR_r4"},
     {{al, r3, r3, ROR, r5}, false, al, "al r3 r3 ROR r5", "al_r3_r3_ROR_r5"},
     {{al, r3, r3, ROR, r6}, false, al, "al r3 r3 ROR r6", "al_r3_r3_ROR_r6"},
     {{al, r3, r3, ROR, r7}, false, al, "al r3 r3 ROR r7", "al_r3_r3_ROR_r7"},
     {{al, r4, r4, LSL, r0}, false, al, "al r4 r4 LSL r0", "al_r4_r4_LSL_r0"},
     {{al, r4, r4, LSL, r1}, false, al, "al r4 r4 LSL r1", "al_r4_r4_LSL_r1"},
     {{al, r4, r4, LSL, r2}, false, al, "al r4 r4 LSL r2", "al_r4_r4_LSL_r2"},
     {{al, r4, r4, LSL, r3}, false, al, "al r4 r4 LSL r3", "al_r4_r4_LSL_r3"},
     {{al, r4, r4, LSL, r4}, false, al, "al r4 r4 LSL r4", "al_r4_r4_LSL_r4"},
     {{al, r4, r4, LSL, r5}, false, al, "al r4 r4 LSL r5", "al_r4_r4_LSL_r5"},
     {{al, r4, r4, LSL, r6}, false, al, "al r4 r4 LSL r6", "al_r4_r4_LSL_r6"},
     {{al, r4, r4, LSL, r7}, false, al, "al r4 r4 LSL r7", "al_r4_r4_LSL_r7"},
     {{al, r4, r4, LSR, r0}, false, al, "al r4 r4 LSR r0", "al_r4_r4_LSR_r0"},
     {{al, r4, r4, LSR, r1}, false, al, "al r4 r4 LSR r1", "al_r4_r4_LSR_r1"},
     {{al, r4, r4, LSR, r2}, false, al, "al r4 r4 LSR r2", "al_r4_r4_LSR_r2"},
     {{al, r4, r4, LSR, r3}, false, al, "al r4 r4 LSR r3", "al_r4_r4_LSR_r3"},
     {{al, r4, r4, LSR, r4}, false, al, "al r4 r4 LSR r4", "al_r4_r4_LSR_r4"},
     {{al, r4, r4, LSR, r5}, false, al, "al r4 r4 LSR r5", "al_r4_r4_LSR_r5"},
     {{al, r4, r4, LSR, r6}, false, al, "al r4 r4 LSR r6", "al_r4_r4_LSR_r6"},
     {{al, r4, r4, LSR, r7}, false, al, "al r4 r4 LSR r7", "al_r4_r4_LSR_r7"},
     {{al, r4, r4, ASR, r0}, false, al, "al r4 r4 ASR r0", "al_r4_r4_ASR_r0"},
     {{al, r4, r4, ASR, r1}, false, al, "al r4 r4 ASR r1", "al_r4_r4_ASR_r1"},
     {{al, r4, r4, ASR, r2}, false, al, "al r4 r4 ASR r2", "al_r4_r4_ASR_r2"},
     {{al, r4, r4, ASR, r3}, false, al, "al r4 r4 ASR r3", "al_r4_r4_ASR_r3"},
     {{al, r4, r4, ASR, r4}, false, al, "al r4 r4 ASR r4", "al_r4_r4_ASR_r4"},
     {{al, r4, r4, ASR, r5}, false, al, "al r4 r4 ASR r5", "al_r4_r4_ASR_r5"},
     {{al, r4, r4, ASR, r6}, false, al, "al r4 r4 ASR r6", "al_r4_r4_ASR_r6"},
     {{al, r4, r4, ASR, r7}, false, al, "al r4 r4 ASR r7", "al_r4_r4_ASR_r7"},
     {{al, r4, r4, ROR, r0}, false, al, "al r4 r4 ROR r0", "al_r4_r4_ROR_r0"},
     {{al, r4, r4, ROR, r1}, false, al, "al r4 r4 ROR r1", "al_r4_r4_ROR_r1"},
     {{al, r4, r4, ROR, r2}, false, al, "al r4 r4 ROR r2", "al_r4_r4_ROR_r2"},
     {{al, r4, r4, ROR, r3}, false, al, "al r4 r4 ROR r3", "al_r4_r4_ROR_r3"},
     {{al, r4, r4, ROR, r4}, false, al, "al r4 r4 ROR r4", "al_r4_r4_ROR_r4"},
     {{al, r4, r4, ROR, r5}, false, al, "al r4 r4 ROR r5", "al_r4_r4_ROR_r5"},
     {{al, r4, r4, ROR, r6}, false, al, "al r4 r4 ROR r6", "al_r4_r4_ROR_r6"},
     {{al, r4, r4, ROR, r7}, false, al, "al r4 r4 ROR r7", "al_r4_r4_ROR_r7"},
     {{al, r5, r5, LSL, r0}, false, al, "al r5 r5 LSL r0", "al_r5_r5_LSL_r0"},
     {{al, r5, r5, LSL, r1}, false, al, "al r5 r5 LSL r1", "al_r5_r5_LSL_r1"},
     {{al, r5, r5, LSL, r2}, false, al, "al r5 r5 LSL r2", "al_r5_r5_LSL_r2"},
     {{al, r5, r5, LSL, r3}, false, al, "al r5 r5 LSL r3", "al_r5_r5_LSL_r3"},
     {{al, r5, r5, LSL, r4}, false, al, "al r5 r5 LSL r4", "al_r5_r5_LSL_r4"},
     {{al, r5, r5, LSL, r5}, false, al, "al r5 r5 LSL r5", "al_r5_r5_LSL_r5"},
     {{al, r5, r5, LSL, r6}, false, al, "al r5 r5 LSL r6", "al_r5_r5_LSL_r6"},
     {{al, r5, r5, LSL, r7}, false, al, "al r5 r5 LSL r7", "al_r5_r5_LSL_r7"},
     {{al, r5, r5, LSR, r0}, false, al, "al r5 r5 LSR r0", "al_r5_r5_LSR_r0"},
     {{al, r5, r5, LSR, r1}, false, al, "al r5 r5 LSR r1", "al_r5_r5_LSR_r1"},
     {{al, r5, r5, LSR, r2}, false, al, "al r5 r5 LSR r2", "al_r5_r5_LSR_r2"},
     {{al, r5, r5, LSR, r3}, false, al, "al r5 r5 LSR r3", "al_r5_r5_LSR_r3"},
     {{al, r5, r5, LSR, r4}, false, al, "al r5 r5 LSR r4", "al_r5_r5_LSR_r4"},
     {{al, r5, r5, LSR, r5}, false, al, "al r5 r5 LSR r5", "al_r5_r5_LSR_r5"},
     {{al, r5, r5, LSR, r6}, false, al, "al r5 r5 LSR r6", "al_r5_r5_LSR_r6"},
     {{al, r5, r5, LSR, r7}, false, al, "al r5 r5 LSR r7", "al_r5_r5_LSR_r7"},
     {{al, r5, r5, ASR, r0}, false, al, "al r5 r5 ASR r0", "al_r5_r5_ASR_r0"},
     {{al, r5, r5, ASR, r1}, false, al, "al r5 r5 ASR r1", "al_r5_r5_ASR_r1"},
     {{al, r5, r5, ASR, r2}, false, al, "al r5 r5 ASR r2", "al_r5_r5_ASR_r2"},
     {{al, r5, r5, ASR, r3}, false, al, "al r5 r5 ASR r3", "al_r5_r5_ASR_r3"},
     {{al, r5, r5, ASR, r4}, false, al, "al r5 r5 ASR r4", "al_r5_r5_ASR_r4"},
     {{al, r5, r5, ASR, r5}, false, al, "al r5 r5 ASR r5", "al_r5_r5_ASR_r5"},
     {{al, r5, r5, ASR, r6}, false, al, "al r5 r5 ASR r6", "al_r5_r5_ASR_r6"},
     {{al, r5, r5, ASR, r7}, false, al, "al r5 r5 ASR r7", "al_r5_r5_ASR_r7"},
     {{al, r5, r5, ROR, r0}, false, al, "al r5 r5 ROR r0", "al_r5_r5_ROR_r0"},
     {{al, r5, r5, ROR, r1}, false, al, "al r5 r5 ROR r1", "al_r5_r5_ROR_r1"},
     {{al, r5, r5, ROR, r2}, false, al, "al r5 r5 ROR r2", "al_r5_r5_ROR_r2"},
     {{al, r5, r5, ROR, r3}, false, al, "al r5 r5 ROR r3", "al_r5_r5_ROR_r3"},
     {{al, r5, r5, ROR, r4}, false, al, "al r5 r5 ROR r4", "al_r5_r5_ROR_r4"},
     {{al, r5, r5, ROR, r5}, false, al, "al r5 r5 ROR r5", "al_r5_r5_ROR_r5"},
     {{al, r5, r5, ROR, r6}, false, al, "al r5 r5 ROR r6", "al_r5_r5_ROR_r6"},
     {{al, r5, r5, ROR, r7}, false, al, "al r5 r5 ROR r7", "al_r5_r5_ROR_r7"},
     {{al, r6, r6, LSL, r0}, false, al, "al r6 r6 LSL r0", "al_r6_r6_LSL_r0"},
     {{al, r6, r6, LSL, r1}, false, al, "al r6 r6 LSL r1", "al_r6_r6_LSL_r1"},
     {{al, r6, r6, LSL, r2}, false, al, "al r6 r6 LSL r2", "al_r6_r6_LSL_r2"},
     {{al, r6, r6, LSL, r3}, false, al, "al r6 r6 LSL r3", "al_r6_r6_LSL_r3"},
     {{al, r6, r6, LSL, r4}, false, al, "al r6 r6 LSL r4", "al_r6_r6_LSL_r4"},
     {{al, r6, r6, LSL, r5}, false, al, "al r6 r6 LSL r5", "al_r6_r6_LSL_r5"},
     {{al, r6, r6, LSL, r6}, false, al, "al r6 r6 LSL r6", "al_r6_r6_LSL_r6"},
     {{al, r6, r6, LSL, r7}, false, al, "al r6 r6 LSL r7", "al_r6_r6_LSL_r7"},
     {{al, r6, r6, LSR, r0}, false, al, "al r6 r6 LSR r0", "al_r6_r6_LSR_r0"},
     {{al, r6, r6, LSR, r1}, false, al, "al r6 r6 LSR r1", "al_r6_r6_LSR_r1"},
     {{al, r6, r6, LSR, r2}, false, al, "al r6 r6 LSR r2", "al_r6_r6_LSR_r2"},
     {{al, r6, r6, LSR, r3}, false, al, "al r6 r6 LSR r3", "al_r6_r6_LSR_r3"},
     {{al, r6, r6, LSR, r4}, false, al, "al r6 r6 LSR r4", "al_r6_r6_LSR_r4"},
     {{al, r6, r6, LSR, r5}, false, al, "al r6 r6 LSR r5", "al_r6_r6_LSR_r5"},
     {{al, r6, r6, LSR, r6}, false, al, "al r6 r6 LSR r6", "al_r6_r6_LSR_r6"},
     {{al, r6, r6, LSR, r7}, false, al, "al r6 r6 LSR r7", "al_r6_r6_LSR_r7"},
     {{al, r6, r6, ASR, r0}, false, al, "al r6 r6 ASR r0", "al_r6_r6_ASR_r0"},
     {{al, r6, r6, ASR, r1}, false, al, "al r6 r6 ASR r1", "al_r6_r6_ASR_r1"},
     {{al, r6, r6, ASR, r2}, false, al, "al r6 r6 ASR r2", "al_r6_r6_ASR_r2"},
     {{al, r6, r6, ASR, r3}, false, al, "al r6 r6 ASR r3", "al_r6_r6_ASR_r3"},
     {{al, r6, r6, ASR, r4}, false, al, "al r6 r6 ASR r4", "al_r6_r6_ASR_r4"},
     {{al, r6, r6, ASR, r5}, false, al, "al r6 r6 ASR r5", "al_r6_r6_ASR_r5"},
     {{al, r6, r6, ASR, r6}, false, al, "al r6 r6 ASR r6", "al_r6_r6_ASR_r6"},
     {{al, r6, r6, ASR, r7}, false, al, "al r6 r6 ASR r7", "al_r6_r6_ASR_r7"},
     {{al, r6, r6, ROR, r0}, false, al, "al r6 r6 ROR r0", "al_r6_r6_ROR_r0"},
     {{al, r6, r6, ROR, r1}, false, al, "al r6 r6 ROR r1", "al_r6_r6_ROR_r1"},
     {{al, r6, r6, ROR, r2}, false, al, "al r6 r6 ROR r2", "al_r6_r6_ROR_r2"},
     {{al, r6, r6, ROR, r3}, false, al, "al r6 r6 ROR r3", "al_r6_r6_ROR_r3"},
     {{al, r6, r6, ROR, r4}, false, al, "al r6 r6 ROR r4", "al_r6_r6_ROR_r4"},
     {{al, r6, r6, ROR, r5}, false, al, "al r6 r6 ROR r5", "al_r6_r6_ROR_r5"},
     {{al, r6, r6, ROR, r6}, false, al, "al r6 r6 ROR r6", "al_r6_r6_ROR_r6"},
     {{al, r6, r6, ROR, r7}, false, al, "al r6 r6 ROR r7", "al_r6_r6_ROR_r7"},
     {{al, r7, r7, LSL, r0}, false, al, "al r7 r7 LSL r0", "al_r7_r7_LSL_r0"},
     {{al, r7, r7, LSL, r1}, false, al, "al r7 r7 LSL r1", "al_r7_r7_LSL_r1"},
     {{al, r7, r7, LSL, r2}, false, al, "al r7 r7 LSL r2", "al_r7_r7_LSL_r2"},
     {{al, r7, r7, LSL, r3}, false, al, "al r7 r7 LSL r3", "al_r7_r7_LSL_r3"},
     {{al, r7, r7, LSL, r4}, false, al, "al r7 r7 LSL r4", "al_r7_r7_LSL_r4"},
     {{al, r7, r7, LSL, r5}, false, al, "al r7 r7 LSL r5", "al_r7_r7_LSL_r5"},
     {{al, r7, r7, LSL, r6}, false, al, "al r7 r7 LSL r6", "al_r7_r7_LSL_r6"},
     {{al, r7, r7, LSL, r7}, false, al, "al r7 r7 LSL r7", "al_r7_r7_LSL_r7"},
     {{al, r7, r7, LSR, r0}, false, al, "al r7 r7 LSR r0", "al_r7_r7_LSR_r0"},
     {{al, r7, r7, LSR, r1}, false, al, "al r7 r7 LSR r1", "al_r7_r7_LSR_r1"},
     {{al, r7, r7, LSR, r2}, false, al, "al r7 r7 LSR r2", "al_r7_r7_LSR_r2"},
     {{al, r7, r7, LSR, r3}, false, al, "al r7 r7 LSR r3", "al_r7_r7_LSR_r3"},
     {{al, r7, r7, LSR, r4}, false, al, "al r7 r7 LSR r4", "al_r7_r7_LSR_r4"},
     {{al, r7, r7, LSR, r5}, false, al, "al r7 r7 LSR r5", "al_r7_r7_LSR_r5"},
     {{al, r7, r7, LSR, r6}, false, al, "al r7 r7 LSR r6", "al_r7_r7_LSR_r6"},
     {{al, r7, r7, LSR, r7}, false, al, "al r7 r7 LSR r7", "al_r7_r7_LSR_r7"},
     {{al, r7, r7, ASR, r0}, false, al, "al r7 r7 ASR r0", "al_r7_r7_ASR_r0"},
     {{al, r7, r7, ASR, r1}, false, al, "al r7 r7 ASR r1", "al_r7_r7_ASR_r1"},
     {{al, r7, r7, ASR, r2}, false, al, "al r7 r7 ASR r2", "al_r7_r7_ASR_r2"},
     {{al, r7, r7, ASR, r3}, false, al, "al r7 r7 ASR r3", "al_r7_r7_ASR_r3"},
     {{al, r7, r7, ASR, r4}, false, al, "al r7 r7 ASR r4", "al_r7_r7_ASR_r4"},
     {{al, r7, r7, ASR, r5}, false, al, "al r7 r7 ASR r5", "al_r7_r7_ASR_r5"},
     {{al, r7, r7, ASR, r6}, false, al, "al r7 r7 ASR r6", "al_r7_r7_ASR_r6"},
     {{al, r7, r7, ASR, r7}, false, al, "al r7 r7 ASR r7", "al_r7_r7_ASR_r7"},
     {{al, r7, r7, ROR, r0}, false, al, "al r7 r7 ROR r0", "al_r7_r7_ROR_r0"},
     {{al, r7, r7, ROR, r1}, false, al, "al r7 r7 ROR r1", "al_r7_r7_ROR_r1"},
     {{al, r7, r7, ROR, r2}, false, al, "al r7 r7 ROR r2", "al_r7_r7_ROR_r2"},
     {{al, r7, r7, ROR, r3}, false, al, "al r7 r7 ROR r3", "al_r7_r7_ROR_r3"},
     {{al, r7, r7, ROR, r4}, false, al, "al r7 r7 ROR r4", "al_r7_r7_ROR_r4"},
     {{al, r7, r7, ROR, r5}, false, al, "al r7 r7 ROR r5", "al_r7_r7_ROR_r5"},
     {{al, r7, r7, ROR, r6}, false, al, "al r7 r7 ROR r6", "al_r7_r7_ROR_r6"},
     {{al, r7, r7, ROR, r7}, false, al, "al r7 r7 ROR r7", "al_r7_r7_ROR_r7"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-operand-rn-shift-rs-narrow-out-it-block-movs-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
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
    ShiftType shift = kTests[i].operands.shift;
    Register rs = kTests[i].operands.rs;
    Operand op(rn, shift, rs);

    int32_t start = masm.GetCursorOffset();
    {
      // We never generate more that 4 bytes, as IT instructions are only
      // allowed for narrow encodings.
      ExactAssemblyScope scope(&masm, 4, ExactAssemblyScope::kMaximumSize);
      if (kTests[i].in_it_block) {
        masm.it(kTests[i].it_condition);
      }
      (masm.*instruction)(cond, rd, op);
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
      "AARCH32_ASSEMBLER_COND_RD_OPERAND_RN_SHIFT_RS_NARROW_OUT_IT_"        \
      "BLOCK_" #mnemonic "_T32",                                            \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
