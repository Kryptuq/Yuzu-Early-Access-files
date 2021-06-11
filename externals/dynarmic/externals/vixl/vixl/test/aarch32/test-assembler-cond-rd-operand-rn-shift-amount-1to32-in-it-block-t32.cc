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
#define FOREACH_INSTRUCTION(M) M(mov)


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
  uint32_t amount;
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
    {{{ge, r7, r6, LSR, 20}, true, ge, "ge r7 r6 LSR 20", "ge_r7_r6_LSR_20"},
     {{gt, r4, r6, LSR, 32}, true, gt, "gt r4 r6 LSR 32", "gt_r4_r6_LSR_32"},
     {{hi, r6, r7, LSR, 29}, true, hi, "hi r6 r7 LSR 29", "hi_r6_r7_LSR_29"},
     {{mi, r7, r1, LSR, 10}, true, mi, "mi r7 r1 LSR 10", "mi_r7_r1_LSR_10"},
     {{ls, r7, r6, LSR, 14}, true, ls, "ls r7 r6 LSR 14", "ls_r7_r6_LSR_14"},
     {{eq, r7, r2, ASR, 14}, true, eq, "eq r7 r2 ASR 14", "eq_r7_r2_ASR_14"},
     {{le, r3, r7, LSR, 2}, true, le, "le r3 r7 LSR 2", "le_r3_r7_LSR_2"},
     {{mi, r2, r7, LSR, 32}, true, mi, "mi r2 r7 LSR 32", "mi_r2_r7_LSR_32"},
     {{ge, r5, r0, ASR, 23}, true, ge, "ge r5 r0 ASR 23", "ge_r5_r0_ASR_23"},
     {{ne, r3, r7, LSR, 28}, true, ne, "ne r3 r7 LSR 28", "ne_r3_r7_LSR_28"},
     {{gt, r6, r4, LSR, 13}, true, gt, "gt r6 r4 LSR 13", "gt_r6_r4_LSR_13"},
     {{hi, r6, r5, LSR, 12}, true, hi, "hi r6 r5 LSR 12", "hi_r6_r5_LSR_12"},
     {{pl, r6, r1, ASR, 12}, true, pl, "pl r6 r1 ASR 12", "pl_r6_r1_ASR_12"},
     {{hi, r7, r1, ASR, 30}, true, hi, "hi r7 r1 ASR 30", "hi_r7_r1_ASR_30"},
     {{mi, r7, r6, ASR, 20}, true, mi, "mi r7 r6 ASR 20", "mi_r7_r6_ASR_20"},
     {{cc, r3, r6, ASR, 22}, true, cc, "cc r3 r6 ASR 22", "cc_r3_r6_ASR_22"},
     {{vc, r2, r1, ASR, 29}, true, vc, "vc r2 r1 ASR 29", "vc_r2_r1_ASR_29"},
     {{pl, r0, r1, ASR, 15}, true, pl, "pl r0 r1 ASR 15", "pl_r0_r1_ASR_15"},
     {{mi, r2, r1, LSR, 22}, true, mi, "mi r2 r1 LSR 22", "mi_r2_r1_LSR_22"},
     {{pl, r6, r4, ASR, 6}, true, pl, "pl r6 r4 ASR 6", "pl_r6_r4_ASR_6"},
     {{hi, r1, r3, ASR, 15}, true, hi, "hi r1 r3 ASR 15", "hi_r1_r3_ASR_15"},
     {{eq, r3, r5, ASR, 13}, true, eq, "eq r3 r5 ASR 13", "eq_r3_r5_ASR_13"},
     {{le, r4, r6, ASR, 31}, true, le, "le r4 r6 ASR 31", "le_r4_r6_ASR_31"},
     {{gt, r5, r1, ASR, 15}, true, gt, "gt r5 r1 ASR 15", "gt_r5_r1_ASR_15"},
     {{pl, r3, r7, ASR, 29}, true, pl, "pl r3 r7 ASR 29", "pl_r3_r7_ASR_29"},
     {{cs, r3, r1, ASR, 15}, true, cs, "cs r3 r1 ASR 15", "cs_r3_r1_ASR_15"},
     {{le, r1, r7, ASR, 32}, true, le, "le r1 r7 ASR 32", "le_r1_r7_ASR_32"},
     {{vs, r5, r7, LSR, 5}, true, vs, "vs r5 r7 LSR 5", "vs_r5_r7_LSR_5"},
     {{cs, r3, r1, ASR, 30}, true, cs, "cs r3 r1 ASR 30", "cs_r3_r1_ASR_30"},
     {{gt, r7, r0, LSR, 26}, true, gt, "gt r7 r0 LSR 26", "gt_r7_r0_LSR_26"},
     {{ne, r2, r6, LSR, 24}, true, ne, "ne r2 r6 LSR 24", "ne_r2_r6_LSR_24"},
     {{pl, r2, r3, ASR, 5}, true, pl, "pl r2 r3 ASR 5", "pl_r2_r3_ASR_5"},
     {{le, r3, r4, ASR, 30}, true, le, "le r3 r4 ASR 30", "le_r3_r4_ASR_30"},
     {{cs, r5, r7, ASR, 13}, true, cs, "cs r5 r7 ASR 13", "cs_r5_r7_ASR_13"},
     {{eq, r0, r1, LSR, 31}, true, eq, "eq r0 r1 LSR 31", "eq_r0_r1_LSR_31"},
     {{eq, r7, r2, LSR, 2}, true, eq, "eq r7 r2 LSR 2", "eq_r7_r2_LSR_2"},
     {{vc, r2, r5, ASR, 30}, true, vc, "vc r2 r5 ASR 30", "vc_r2_r5_ASR_30"},
     {{eq, r2, r1, ASR, 5}, true, eq, "eq r2 r1 ASR 5", "eq_r2_r1_ASR_5"},
     {{le, r4, r7, LSR, 10}, true, le, "le r4 r7 LSR 10", "le_r4_r7_LSR_10"},
     {{cs, r6, r1, ASR, 13}, true, cs, "cs r6 r1 ASR 13", "cs_r6_r1_ASR_13"},
     {{ls, r7, r4, LSR, 26}, true, ls, "ls r7 r4 LSR 26", "ls_r7_r4_LSR_26"},
     {{ge, r7, r3, ASR, 28}, true, ge, "ge r7 r3 ASR 28", "ge_r7_r3_ASR_28"},
     {{mi, r4, r4, LSR, 16}, true, mi, "mi r4 r4 LSR 16", "mi_r4_r4_LSR_16"},
     {{ge, r3, r7, LSR, 23}, true, ge, "ge r3 r7 LSR 23", "ge_r3_r7_LSR_23"},
     {{ge, r3, r3, LSR, 25}, true, ge, "ge r3 r3 LSR 25", "ge_r3_r3_LSR_25"},
     {{ne, r2, r2, ASR, 19}, true, ne, "ne r2 r2 ASR 19", "ne_r2_r2_ASR_19"},
     {{hi, r0, r5, LSR, 29}, true, hi, "hi r0 r5 LSR 29", "hi_r0_r5_LSR_29"},
     {{vc, r0, r0, ASR, 18}, true, vc, "vc r0 r0 ASR 18", "vc_r0_r0_ASR_18"},
     {{gt, r5, r3, ASR, 3}, true, gt, "gt r5 r3 ASR 3", "gt_r5_r3_ASR_3"},
     {{mi, r6, r2, ASR, 28}, true, mi, "mi r6 r2 ASR 28", "mi_r6_r2_ASR_28"},
     {{ls, r2, r6, LSR, 31}, true, ls, "ls r2 r6 LSR 31", "ls_r2_r6_LSR_31"},
     {{cs, r4, r0, LSR, 5}, true, cs, "cs r4 r0 LSR 5", "cs_r4_r0_LSR_5"},
     {{eq, r6, r2, ASR, 17}, true, eq, "eq r6 r2 ASR 17", "eq_r6_r2_ASR_17"},
     {{cs, r7, r5, LSR, 2}, true, cs, "cs r7 r5 LSR 2", "cs_r7_r5_LSR_2"},
     {{vc, r5, r3, LSR, 17}, true, vc, "vc r5 r3 LSR 17", "vc_r5_r3_LSR_17"},
     {{pl, r5, r1, LSR, 12}, true, pl, "pl r5 r1 LSR 12", "pl_r5_r1_LSR_12"},
     {{lt, r2, r1, LSR, 2}, true, lt, "lt r2 r1 LSR 2", "lt_r2_r1_LSR_2"},
     {{hi, r6, r5, ASR, 3}, true, hi, "hi r6 r5 ASR 3", "hi_r6_r5_ASR_3"},
     {{gt, r1, r6, ASR, 19}, true, gt, "gt r1 r6 ASR 19", "gt_r1_r6_ASR_19"},
     {{ne, r3, r7, ASR, 24}, true, ne, "ne r3 r7 ASR 24", "ne_r3_r7_ASR_24"},
     {{mi, r3, r2, ASR, 27}, true, mi, "mi r3 r2 ASR 27", "mi_r3_r2_ASR_27"},
     {{ge, r0, r5, ASR, 24}, true, ge, "ge r0 r5 ASR 24", "ge_r0_r5_ASR_24"},
     {{hi, r0, r2, ASR, 12}, true, hi, "hi r0 r2 ASR 12", "hi_r0_r2_ASR_12"},
     {{vs, r4, r7, LSR, 8}, true, vs, "vs r4 r7 LSR 8", "vs_r4_r7_LSR_8"},
     {{vc, r0, r4, LSR, 6}, true, vc, "vc r0 r4 LSR 6", "vc_r0_r4_LSR_6"},
     {{cs, r3, r7, LSR, 2}, true, cs, "cs r3 r7 LSR 2", "cs_r3_r7_LSR_2"},
     {{cc, r2, r3, ASR, 7}, true, cc, "cc r2 r3 ASR 7", "cc_r2_r3_ASR_7"},
     {{lt, r5, r4, LSR, 26}, true, lt, "lt r5 r4 LSR 26", "lt_r5_r4_LSR_26"},
     {{vs, r6, r3, LSR, 16}, true, vs, "vs r6 r3 LSR 16", "vs_r6_r3_LSR_16"},
     {{hi, r5, r3, ASR, 19}, true, hi, "hi r5 r3 ASR 19", "hi_r5_r3_ASR_19"},
     {{cc, r3, r6, LSR, 16}, true, cc, "cc r3 r6 LSR 16", "cc_r3_r6_LSR_16"},
     {{cc, r0, r6, LSR, 31}, true, cc, "cc r0 r6 LSR 31", "cc_r0_r6_LSR_31"},
     {{hi, r6, r7, ASR, 32}, true, hi, "hi r6 r7 ASR 32", "hi_r6_r7_ASR_32"},
     {{vc, r3, r4, ASR, 12}, true, vc, "vc r3 r4 ASR 12", "vc_r3_r4_ASR_12"},
     {{cs, r5, r7, ASR, 9}, true, cs, "cs r5 r7 ASR 9", "cs_r5_r7_ASR_9"},
     {{lt, r5, r0, ASR, 4}, true, lt, "lt r5 r0 ASR 4", "lt_r5_r0_ASR_4"},
     {{cc, r3, r1, ASR, 17}, true, cc, "cc r3 r1 ASR 17", "cc_r3_r1_ASR_17"},
     {{vs, r7, r4, ASR, 16}, true, vs, "vs r7 r4 ASR 16", "vs_r7_r4_ASR_16"},
     {{eq, r7, r6, ASR, 14}, true, eq, "eq r7 r6 ASR 14", "eq_r7_r6_ASR_14"},
     {{ne, r1, r0, ASR, 21}, true, ne, "ne r1 r0 ASR 21", "ne_r1_r0_ASR_21"},
     {{ls, r0, r1, LSR, 19}, true, ls, "ls r0 r1 LSR 19", "ls_r0_r1_LSR_19"},
     {{cc, r1, r7, ASR, 6}, true, cc, "cc r1 r7 ASR 6", "cc_r1_r7_ASR_6"},
     {{lt, r5, r2, LSR, 1}, true, lt, "lt r5 r2 LSR 1", "lt_r5_r2_LSR_1"},
     {{lt, r7, r0, LSR, 13}, true, lt, "lt r7 r0 LSR 13", "lt_r7_r0_LSR_13"},
     {{gt, r2, r6, ASR, 14}, true, gt, "gt r2 r6 ASR 14", "gt_r2_r6_ASR_14"},
     {{ls, r5, r5, ASR, 23}, true, ls, "ls r5 r5 ASR 23", "ls_r5_r5_ASR_23"},
     {{cs, r1, r1, ASR, 12}, true, cs, "cs r1 r1 ASR 12", "cs_r1_r1_ASR_12"},
     {{pl, r0, r2, LSR, 3}, true, pl, "pl r0 r2 LSR 3", "pl_r0_r2_LSR_3"},
     {{le, r0, r1, LSR, 5}, true, le, "le r0 r1 LSR 5", "le_r0_r1_LSR_5"},
     {{ne, r0, r5, LSR, 29}, true, ne, "ne r0 r5 LSR 29", "ne_r0_r5_LSR_29"},
     {{vs, r6, r5, LSR, 14}, true, vs, "vs r6 r5 LSR 14", "vs_r6_r5_LSR_14"},
     {{hi, r1, r7, ASR, 25}, true, hi, "hi r1 r7 ASR 25", "hi_r1_r7_ASR_25"},
     {{mi, r2, r5, ASR, 24}, true, mi, "mi r2 r5 ASR 24", "mi_r2_r5_ASR_24"},
     {{vc, r3, r4, ASR, 3}, true, vc, "vc r3 r4 ASR 3", "vc_r3_r4_ASR_3"},
     {{cs, r4, r0, LSR, 29}, true, cs, "cs r4 r0 LSR 29", "cs_r4_r0_LSR_29"},
     {{ne, r6, r6, ASR, 24}, true, ne, "ne r6 r6 ASR 24", "ne_r6_r6_ASR_24"},
     {{lt, r1, r1, LSR, 1}, true, lt, "lt r1 r1 LSR 1", "lt_r1_r1_LSR_1"},
     {{gt, r5, r3, ASR, 8}, true, gt, "gt r5 r3 ASR 8", "gt_r5_r3_ASR_8"},
     {{mi, r4, r3, LSR, 10}, true, mi, "mi r4 r3 LSR 10", "mi_r4_r3_LSR_10"},
     {{ge, r0, r4, LSR, 24}, true, ge, "ge r0 r4 LSR 24", "ge_r0_r4_LSR_24"},
     {{hi, r0, r5, LSR, 17}, true, hi, "hi r0 r5 LSR 17", "hi_r0_r5_LSR_17"},
     {{gt, r6, r7, ASR, 12}, true, gt, "gt r6 r7 ASR 12", "gt_r6_r7_ASR_12"},
     {{mi, r2, r2, LSR, 15}, true, mi, "mi r2 r2 LSR 15", "mi_r2_r2_LSR_15"},
     {{ls, r3, r0, ASR, 1}, true, ls, "ls r3 r0 ASR 1", "ls_r3_r0_ASR_1"},
     {{lt, r4, r0, LSR, 5}, true, lt, "lt r4 r0 LSR 5", "lt_r4_r0_LSR_5"},
     {{hi, r0, r1, LSR, 18}, true, hi, "hi r0 r1 LSR 18", "hi_r0_r1_LSR_18"},
     {{hi, r6, r4, LSR, 28}, true, hi, "hi r6 r4 LSR 28", "hi_r6_r4_LSR_28"},
     {{pl, r1, r1, LSR, 24}, true, pl, "pl r1 r1 LSR 24", "pl_r1_r1_LSR_24"},
     {{vs, r6, r0, ASR, 8}, true, vs, "vs r6 r0 ASR 8", "vs_r6_r0_ASR_8"},
     {{vs, r3, r4, LSR, 8}, true, vs, "vs r3 r4 LSR 8", "vs_r3_r4_LSR_8"},
     {{cs, r2, r6, LSR, 13}, true, cs, "cs r2 r6 LSR 13", "cs_r2_r6_LSR_13"},
     {{lt, r7, r6, ASR, 15}, true, lt, "lt r7 r6 ASR 15", "lt_r7_r6_ASR_15"},
     {{le, r6, r1, ASR, 32}, true, le, "le r6 r1 ASR 32", "le_r6_r1_ASR_32"},
     {{cc, r7, r5, LSR, 3}, true, cc, "cc r7 r5 LSR 3", "cc_r7_r5_LSR_3"},
     {{hi, r5, r1, LSR, 7}, true, hi, "hi r5 r1 LSR 7", "hi_r5_r1_LSR_7"},
     {{lt, r5, r4, LSR, 5}, true, lt, "lt r5 r4 LSR 5", "lt_r5_r4_LSR_5"},
     {{mi, r4, r2, LSR, 9}, true, mi, "mi r4 r2 LSR 9", "mi_r4_r2_LSR_9"},
     {{pl, r3, r3, ASR, 10}, true, pl, "pl r3 r3 ASR 10", "pl_r3_r3_ASR_10"},
     {{ge, r3, r7, LSR, 17}, true, ge, "ge r3 r7 LSR 17", "ge_r3_r7_LSR_17"},
     {{ls, r4, r1, LSR, 5}, true, ls, "ls r4 r1 LSR 5", "ls_r4_r1_LSR_5"},
     {{mi, r4, r3, LSR, 28}, true, mi, "mi r4 r3 LSR 28", "mi_r4_r3_LSR_28"},
     {{ne, r7, r1, ASR, 14}, true, ne, "ne r7 r1 ASR 14", "ne_r7_r1_ASR_14"},
     {{ge, r7, r7, ASR, 20}, true, ge, "ge r7 r7 ASR 20", "ge_r7_r7_ASR_20"},
     {{ne, r2, r3, LSR, 11}, true, ne, "ne r2 r3 LSR 11", "ne_r2_r3_LSR_11"},
     {{lt, r6, r5, ASR, 15}, true, lt, "lt r6 r5 ASR 15", "lt_r6_r5_ASR_15"},
     {{cs, r6, r7, ASR, 3}, true, cs, "cs r6 r7 ASR 3", "cs_r6_r7_ASR_3"},
     {{hi, r7, r7, LSR, 12}, true, hi, "hi r7 r7 LSR 12", "hi_r7_r7_LSR_12"},
     {{cs, r2, r4, ASR, 4}, true, cs, "cs r2 r4 ASR 4", "cs_r2_r4_ASR_4"},
     {{lt, r7, r7, ASR, 3}, true, lt, "lt r7 r7 ASR 3", "lt_r7_r7_ASR_3"},
     {{le, r1, r3, ASR, 27}, true, le, "le r1 r3 ASR 27", "le_r1_r3_ASR_27"},
     {{vs, r2, r3, LSR, 28}, true, vs, "vs r2 r3 LSR 28", "vs_r2_r3_LSR_28"},
     {{pl, r2, r0, ASR, 8}, true, pl, "pl r2 r0 ASR 8", "pl_r2_r0_ASR_8"},
     {{hi, r5, r2, ASR, 15}, true, hi, "hi r5 r2 ASR 15", "hi_r5_r2_ASR_15"},
     {{vs, r6, r0, LSR, 5}, true, vs, "vs r6 r0 LSR 5", "vs_r6_r0_LSR_5"},
     {{le, r1, r6, ASR, 22}, true, le, "le r1 r6 ASR 22", "le_r1_r6_ASR_22"},
     {{hi, r3, r2, ASR, 17}, true, hi, "hi r3 r2 ASR 17", "hi_r3_r2_ASR_17"},
     {{gt, r1, r5, ASR, 16}, true, gt, "gt r1 r5 ASR 16", "gt_r1_r5_ASR_16"},
     {{hi, r2, r0, ASR, 26}, true, hi, "hi r2 r0 ASR 26", "hi_r2_r0_ASR_26"},
     {{vs, r1, r4, LSR, 27}, true, vs, "vs r1 r4 LSR 27", "vs_r1_r4_LSR_27"},
     {{ls, r3, r4, ASR, 15}, true, ls, "ls r3 r4 ASR 15", "ls_r3_r4_ASR_15"},
     {{ge, r7, r4, LSR, 15}, true, ge, "ge r7 r4 LSR 15", "ge_r7_r4_LSR_15"},
     {{vc, r7, r2, LSR, 21}, true, vc, "vc r7 r2 LSR 21", "vc_r7_r2_LSR_21"},
     {{ls, r2, r7, ASR, 22}, true, ls, "ls r2 r7 ASR 22", "ls_r2_r7_ASR_22"},
     {{pl, r5, r5, LSR, 21}, true, pl, "pl r5 r5 LSR 21", "pl_r5_r5_LSR_21"},
     {{cs, r2, r4, ASR, 25}, true, cs, "cs r2 r4 ASR 25", "cs_r2_r4_ASR_25"},
     {{gt, r7, r6, LSR, 5}, true, gt, "gt r7 r6 LSR 5", "gt_r7_r6_LSR_5"},
     {{gt, r5, r6, ASR, 25}, true, gt, "gt r5 r6 ASR 25", "gt_r5_r6_ASR_25"},
     {{vs, r3, r0, LSR, 30}, true, vs, "vs r3 r0 LSR 30", "vs_r3_r0_LSR_30"},
     {{pl, r6, r0, LSR, 17}, true, pl, "pl r6 r0 LSR 17", "pl_r6_r0_LSR_17"},
     {{pl, r6, r2, ASR, 7}, true, pl, "pl r6 r2 ASR 7", "pl_r6_r2_ASR_7"},
     {{lt, r4, r4, LSR, 17}, true, lt, "lt r4 r4 LSR 17", "lt_r4_r4_LSR_17"},
     {{vc, r4, r5, LSR, 3}, true, vc, "vc r4 r5 LSR 3", "vc_r4_r5_LSR_3"},
     {{cc, r4, r4, ASR, 21}, true, cc, "cc r4 r4 ASR 21", "cc_r4_r4_ASR_21"},
     {{vs, r4, r2, ASR, 10}, true, vs, "vs r4 r2 ASR 10", "vs_r4_r2_ASR_10"},
     {{cs, r7, r4, ASR, 10}, true, cs, "cs r7 r4 ASR 10", "cs_r7_r4_ASR_10"},
     {{hi, r6, r1, LSR, 17}, true, hi, "hi r6 r1 LSR 17", "hi_r6_r1_LSR_17"},
     {{vc, r4, r6, LSR, 24}, true, vc, "vc r4 r6 LSR 24", "vc_r4_r6_LSR_24"},
     {{pl, r3, r2, ASR, 7}, true, pl, "pl r3 r2 ASR 7", "pl_r3_r2_ASR_7"},
     {{mi, r3, r7, LSR, 17}, true, mi, "mi r3 r7 LSR 17", "mi_r3_r7_LSR_17"},
     {{pl, r7, r3, ASR, 7}, true, pl, "pl r7 r3 ASR 7", "pl_r7_r3_ASR_7"},
     {{vc, r0, r4, LSR, 10}, true, vc, "vc r0 r4 LSR 10", "vc_r0_r4_LSR_10"},
     {{le, r2, r1, ASR, 24}, true, le, "le r2 r1 ASR 24", "le_r2_r1_ASR_24"},
     {{le, r2, r7, LSR, 11}, true, le, "le r2 r7 LSR 11", "le_r2_r7_LSR_11"},
     {{cs, r6, r0, ASR, 13}, true, cs, "cs r6 r0 ASR 13", "cs_r6_r0_ASR_13"},
     {{ge, r3, r0, ASR, 27}, true, ge, "ge r3 r0 ASR 27", "ge_r3_r0_ASR_27"},
     {{cc, r6, r0, ASR, 17}, true, cc, "cc r6 r0 ASR 17", "cc_r6_r0_ASR_17"},
     {{hi, r7, r7, ASR, 6}, true, hi, "hi r7 r7 ASR 6", "hi_r7_r7_ASR_6"},
     {{vc, r6, r7, LSR, 21}, true, vc, "vc r6 r7 LSR 21", "vc_r6_r7_LSR_21"},
     {{ge, r6, r4, ASR, 25}, true, ge, "ge r6 r4 ASR 25", "ge_r6_r4_ASR_25"},
     {{gt, r5, r2, ASR, 7}, true, gt, "gt r5 r2 ASR 7", "gt_r5_r2_ASR_7"},
     {{gt, r6, r1, LSR, 8}, true, gt, "gt r6 r1 LSR 8", "gt_r6_r1_LSR_8"},
     {{le, r1, r6, ASR, 13}, true, le, "le r1 r6 ASR 13", "le_r1_r6_ASR_13"},
     {{hi, r6, r0, ASR, 24}, true, hi, "hi r6 r0 ASR 24", "hi_r6_r0_ASR_24"},
     {{le, r1, r5, ASR, 29}, true, le, "le r1 r5 ASR 29", "le_r1_r5_ASR_29"},
     {{vs, r5, r1, ASR, 23}, true, vs, "vs r5 r1 ASR 23", "vs_r5_r1_ASR_23"},
     {{le, r0, r6, LSR, 7}, true, le, "le r0 r6 LSR 7", "le_r0_r6_LSR_7"},
     {{pl, r7, r1, LSR, 17}, true, pl, "pl r7 r1 LSR 17", "pl_r7_r1_LSR_17"},
     {{ne, r4, r7, ASR, 5}, true, ne, "ne r4 r7 ASR 5", "ne_r4_r7_ASR_5"},
     {{ne, r5, r5, LSR, 22}, true, ne, "ne r5 r5 LSR 22", "ne_r5_r5_LSR_22"},
     {{lt, r7, r2, ASR, 29}, true, lt, "lt r7 r2 ASR 29", "lt_r7_r2_ASR_29"},
     {{le, r5, r3, LSR, 28}, true, le, "le r5 r3 LSR 28", "le_r5_r3_LSR_28"},
     {{eq, r7, r5, ASR, 29}, true, eq, "eq r7 r5 ASR 29", "eq_r7_r5_ASR_29"},
     {{cs, r2, r6, LSR, 4}, true, cs, "cs r2 r6 LSR 4", "cs_r2_r6_LSR_4"},
     {{ne, r1, r0, LSR, 17}, true, ne, "ne r1 r0 LSR 17", "ne_r1_r0_LSR_17"},
     {{ls, r2, r6, ASR, 28}, true, ls, "ls r2 r6 ASR 28", "ls_r2_r6_ASR_28"},
     {{ge, r5, r7, ASR, 32}, true, ge, "ge r5 r7 ASR 32", "ge_r5_r7_ASR_32"},
     {{cs, r0, r0, ASR, 1}, true, cs, "cs r0 r0 ASR 1", "cs_r0_r0_ASR_1"},
     {{mi, r6, r5, LSR, 11}, true, mi, "mi r6 r5 LSR 11", "mi_r6_r5_LSR_11"},
     {{mi, r5, r3, ASR, 29}, true, mi, "mi r5 r3 ASR 29", "mi_r5_r3_ASR_29"},
     {{eq, r2, r2, LSR, 30}, true, eq, "eq r2 r2 LSR 30", "eq_r2_r2_LSR_30"},
     {{mi, r4, r2, ASR, 25}, true, mi, "mi r4 r2 ASR 25", "mi_r4_r2_ASR_25"},
     {{cs, r5, r0, LSR, 13}, true, cs, "cs r5 r0 LSR 13", "cs_r5_r0_LSR_13"},
     {{cc, r7, r4, ASR, 10}, true, cc, "cc r7 r4 ASR 10", "cc_r7_r4_ASR_10"},
     {{mi, r1, r5, LSR, 21}, true, mi, "mi r1 r5 LSR 21", "mi_r1_r5_LSR_21"},
     {{vs, r3, r2, LSR, 2}, true, vs, "vs r3 r2 LSR 2", "vs_r3_r2_LSR_2"},
     {{ls, r4, r4, ASR, 14}, true, ls, "ls r4 r4 ASR 14", "ls_r4_r4_ASR_14"},
     {{eq, r5, r4, LSR, 23}, true, eq, "eq r5 r4 LSR 23", "eq_r5_r4_LSR_23"},
     {{gt, r7, r1, ASR, 20}, true, gt, "gt r7 r1 ASR 20", "gt_r7_r1_ASR_20"},
     {{vs, r2, r5, ASR, 5}, true, vs, "vs r2 r5 ASR 5", "vs_r2_r5_ASR_5"},
     {{le, r6, r0, LSR, 14}, true, le, "le r6 r0 LSR 14", "le_r6_r0_LSR_14"},
     {{cs, r6, r5, LSR, 14}, true, cs, "cs r6 r5 LSR 14", "cs_r6_r5_LSR_14"},
     {{ls, r7, r4, ASR, 22}, true, ls, "ls r7 r4 ASR 22", "ls_r7_r4_ASR_22"},
     {{cs, r3, r2, ASR, 1}, true, cs, "cs r3 r2 ASR 1", "cs_r3_r2_ASR_1"},
     {{ne, r7, r1, LSR, 30}, true, ne, "ne r7 r1 LSR 30", "ne_r7_r1_LSR_30"},
     {{ge, r3, r6, LSR, 32}, true, ge, "ge r3 r6 LSR 32", "ge_r3_r6_LSR_32"},
     {{ne, r2, r5, ASR, 11}, true, ne, "ne r2 r5 ASR 11", "ne_r2_r5_ASR_11"},
     {{cc, r4, r4, LSR, 4}, true, cc, "cc r4 r4 LSR 4", "cc_r4_r4_LSR_4"},
     {{lt, r6, r0, ASR, 31}, true, lt, "lt r6 r0 ASR 31", "lt_r6_r0_ASR_31"},
     {{cc, r7, r0, LSR, 26}, true, cc, "cc r7 r0 LSR 26", "cc_r7_r0_LSR_26"},
     {{vs, r1, r4, LSR, 31}, true, vs, "vs r1 r4 LSR 31", "vs_r1_r4_LSR_31"},
     {{vc, r3, r6, LSR, 11}, true, vc, "vc r3 r6 LSR 11", "vc_r3_r6_LSR_11"},
     {{cs, r0, r4, LSR, 26}, true, cs, "cs r0 r4 LSR 26", "cs_r0_r4_LSR_26"},
     {{eq, r0, r6, ASR, 31}, true, eq, "eq r0 r6 ASR 31", "eq_r0_r6_ASR_31"},
     {{eq, r0, r1, ASR, 27}, true, eq, "eq r0 r1 ASR 27", "eq_r0_r1_ASR_27"},
     {{ls, r2, r0, LSR, 16}, true, ls, "ls r2 r0 LSR 16", "ls_r2_r0_LSR_16"},
     {{vs, r1, r7, LSR, 10}, true, vs, "vs r1 r7 LSR 10", "vs_r1_r7_LSR_10"},
     {{gt, r1, r2, ASR, 7}, true, gt, "gt r1 r2 ASR 7", "gt_r1_r2_ASR_7"},
     {{vc, r6, r5, LSR, 6}, true, vc, "vc r6 r5 LSR 6", "vc_r6_r5_LSR_6"},
     {{ge, r7, r2, ASR, 4}, true, ge, "ge r7 r2 ASR 4", "ge_r7_r2_ASR_4"},
     {{cs, r6, r0, LSR, 24}, true, cs, "cs r6 r0 LSR 24", "cs_r6_r0_LSR_24"},
     {{vc, r3, r0, LSR, 25}, true, vc, "vc r3 r0 LSR 25", "vc_r3_r0_LSR_25"},
     {{vc, r6, r6, ASR, 4}, true, vc, "vc r6 r6 ASR 4", "vc_r6_r6_ASR_4"},
     {{vc, r1, r3, LSR, 15}, true, vc, "vc r1 r3 LSR 15", "vc_r1_r3_LSR_15"},
     {{cc, r4, r2, ASR, 26}, true, cc, "cc r4 r2 ASR 26", "cc_r4_r2_ASR_26"},
     {{pl, r2, r2, LSR, 10}, true, pl, "pl r2 r2 LSR 10", "pl_r2_r2_LSR_10"},
     {{pl, r3, r0, LSR, 27}, true, pl, "pl r3 r0 LSR 27", "pl_r3_r0_LSR_27"},
     {{pl, r7, r7, LSR, 21}, true, pl, "pl r7 r7 LSR 21", "pl_r7_r7_LSR_21"},
     {{le, r3, r3, LSR, 31}, true, le, "le r3 r3 LSR 31", "le_r3_r3_LSR_31"},
     {{ge, r2, r2, LSR, 10}, true, ge, "ge r2 r2 LSR 10", "ge_r2_r2_LSR_10"},
     {{le, r2, r1, LSR, 20}, true, le, "le r2 r1 LSR 20", "le_r2_r1_LSR_20"},
     {{eq, r3, r3, LSR, 4}, true, eq, "eq r3 r3 LSR 4", "eq_r3_r3_LSR_4"},
     {{mi, r3, r6, ASR, 7}, true, mi, "mi r3 r6 ASR 7", "mi_r3_r6_ASR_7"},
     {{hi, r2, r6, ASR, 29}, true, hi, "hi r2 r6 ASR 29", "hi_r2_r6_ASR_29"},
     {{pl, r2, r0, LSR, 14}, true, pl, "pl r2 r0 LSR 14", "pl_r2_r0_LSR_14"},
     {{gt, r0, r7, ASR, 26}, true, gt, "gt r0 r7 ASR 26", "gt_r0_r7_ASR_26"},
     {{lt, r1, r6, LSR, 20}, true, lt, "lt r1 r6 LSR 20", "lt_r1_r6_LSR_20"},
     {{le, r7, r1, ASR, 20}, true, le, "le r7 r1 ASR 20", "le_r7_r1_ASR_20"},
     {{mi, r5, r5, LSR, 7}, true, mi, "mi r5 r5 LSR 7", "mi_r5_r5_LSR_7"},
     {{cc, r2, r3, LSR, 23}, true, cc, "cc r2 r3 LSR 23", "cc_r2_r3_LSR_23"},
     {{gt, r4, r2, LSR, 4}, true, gt, "gt r4 r2 LSR 4", "gt_r4_r2_LSR_4"},
     {{vc, r3, r0, LSR, 32}, true, vc, "vc r3 r0 LSR 32", "vc_r3_r0_LSR_32"},
     {{mi, r4, r0, LSR, 15}, true, mi, "mi r4 r0 LSR 15", "mi_r4_r0_LSR_15"},
     {{pl, r4, r2, ASR, 19}, true, pl, "pl r4 r2 ASR 19", "pl_r4_r2_ASR_19"},
     {{ge, r3, r3, ASR, 3}, true, ge, "ge r3 r3 ASR 3", "ge_r3_r3_ASR_3"},
     {{cc, r4, r1, ASR, 11}, true, cc, "cc r4 r1 ASR 11", "cc_r4_r1_ASR_11"},
     {{ne, r4, r1, LSR, 14}, true, ne, "ne r4 r1 LSR 14", "ne_r4_r1_LSR_14"},
     {{hi, r4, r7, ASR, 29}, true, hi, "hi r4 r7 ASR 29", "hi_r4_r7_ASR_29"},
     {{ls, r0, r2, LSR, 6}, true, ls, "ls r0 r2 LSR 6", "ls_r0_r2_LSR_6"},
     {{hi, r3, r0, LSR, 18}, true, hi, "hi r3 r0 LSR 18", "hi_r3_r0_LSR_18"},
     {{ge, r2, r5, ASR, 27}, true, ge, "ge r2 r5 ASR 27", "ge_r2_r5_ASR_27"},
     {{vs, r6, r3, ASR, 10}, true, vs, "vs r6 r3 ASR 10", "vs_r6_r3_ASR_10"},
     {{cs, r7, r2, LSR, 19}, true, cs, "cs r7 r2 LSR 19", "cs_r7_r2_LSR_19"},
     {{vs, r2, r6, ASR, 4}, true, vs, "vs r2 r6 ASR 4", "vs_r2_r6_ASR_4"},
     {{vs, r3, r2, ASR, 26}, true, vs, "vs r3 r2 ASR 26", "vs_r3_r2_ASR_26"},
     {{gt, r3, r3, LSR, 14}, true, gt, "gt r3 r3 LSR 14", "gt_r3_r3_LSR_14"},
     {{mi, r1, r3, ASR, 20}, true, mi, "mi r1 r3 ASR 20", "mi_r1_r3_ASR_20"},
     {{eq, r2, r0, LSR, 3}, true, eq, "eq r2 r0 LSR 3", "eq_r2_r0_LSR_3"},
     {{lt, r4, r3, LSR, 14}, true, lt, "lt r4 r3 LSR 14", "lt_r4_r3_LSR_14"},
     {{vs, r2, r7, ASR, 13}, true, vs, "vs r2 r7 ASR 13", "vs_r2_r7_ASR_13"},
     {{ls, r4, r1, ASR, 31}, true, ls, "ls r4 r1 ASR 31", "ls_r4_r1_ASR_31"},
     {{le, r4, r3, ASR, 17}, true, le, "le r4 r3 ASR 17", "le_r4_r3_ASR_17"},
     {{mi, r7, r0, LSR, 14}, true, mi, "mi r7 r0 LSR 14", "mi_r7_r0_LSR_14"},
     {{ne, r5, r6, LSR, 6}, true, ne, "ne r5 r6 LSR 6", "ne_r5_r6_LSR_6"},
     {{lt, r5, r2, LSR, 6}, true, lt, "lt r5 r2 LSR 6", "lt_r5_r2_LSR_6"},
     {{gt, r6, r1, ASR, 30}, true, gt, "gt r6 r1 ASR 30", "gt_r6_r1_ASR_30"},
     {{ne, r7, r7, ASR, 30}, true, ne, "ne r7 r7 ASR 30", "ne_r7_r7_ASR_30"},
     {{ls, r5, r1, ASR, 25}, true, ls, "ls r5 r1 ASR 25", "ls_r5_r1_ASR_25"},
     {{lt, r3, r2, LSR, 29}, true, lt, "lt r3 r2 LSR 29", "lt_r3_r2_LSR_29"},
     {{le, r6, r1, LSR, 6}, true, le, "le r6 r1 LSR 6", "le_r6_r1_LSR_6"},
     {{gt, r7, r6, ASR, 12}, true, gt, "gt r7 r6 ASR 12", "gt_r7_r6_ASR_12"},
     {{ls, r6, r2, ASR, 10}, true, ls, "ls r6 r2 ASR 10", "ls_r6_r2_ASR_10"},
     {{ne, r7, r7, LSR, 6}, true, ne, "ne r7 r7 LSR 6", "ne_r7_r7_LSR_6"},
     {{mi, r0, r3, ASR, 5}, true, mi, "mi r0 r3 ASR 5", "mi_r0_r3_ASR_5"},
     {{ne, r0, r4, LSR, 3}, true, ne, "ne r0 r4 LSR 3", "ne_r0_r4_LSR_3"},
     {{ge, r3, r4, ASR, 16}, true, ge, "ge r3 r4 ASR 16", "ge_r3_r4_ASR_16"},
     {{ls, r4, r5, ASR, 19}, true, ls, "ls r4 r5 ASR 19", "ls_r4_r5_ASR_19"},
     {{eq, r1, r7, ASR, 21}, true, eq, "eq r1 r7 ASR 21", "eq_r1_r7_ASR_21"},
     {{hi, r6, r4, ASR, 30}, true, hi, "hi r6 r4 ASR 30", "hi_r6_r4_ASR_30"},
     {{eq, r7, r1, LSR, 3}, true, eq, "eq r7 r1 LSR 3", "eq_r7_r1_LSR_3"},
     {{pl, r6, r2, ASR, 12}, true, pl, "pl r6 r2 ASR 12", "pl_r6_r2_ASR_12"},
     {{eq, r6, r3, LSR, 28}, true, eq, "eq r6 r3 LSR 28", "eq_r6_r3_LSR_28"},
     {{cc, r6, r6, LSR, 5}, true, cc, "cc r6 r6 LSR 5", "cc_r6_r6_LSR_5"},
     {{le, r2, r4, ASR, 3}, true, le, "le r2 r4 ASR 3", "le_r2_r4_ASR_3"},
     {{vc, r7, r4, LSR, 30}, true, vc, "vc r7 r4 LSR 30", "vc_r7_r4_LSR_30"},
     {{ge, r0, r0, LSR, 25}, true, ge, "ge r0 r0 LSR 25", "ge_r0_r0_LSR_25"},
     {{ls, r0, r2, ASR, 27}, true, ls, "ls r0 r2 ASR 27", "ls_r0_r2_ASR_27"},
     {{cc, r5, r6, LSR, 22}, true, cc, "cc r5 r6 LSR 22", "cc_r5_r6_LSR_22"},
     {{lt, r0, r4, LSR, 17}, true, lt, "lt r0 r4 LSR 17", "lt_r0_r4_LSR_17"},
     {{le, r0, r4, ASR, 1}, true, le, "le r0 r4 ASR 1", "le_r0_r4_ASR_1"},
     {{hi, r5, r5, ASR, 6}, true, hi, "hi r5 r5 ASR 6", "hi_r5_r5_ASR_6"},
     {{vc, r7, r0, LSR, 16}, true, vc, "vc r7 r0 LSR 16", "vc_r7_r0_LSR_16"},
     {{ne, r6, r1, ASR, 16}, true, ne, "ne r6 r1 ASR 16", "ne_r6_r1_ASR_16"},
     {{vs, r7, r0, LSR, 31}, true, vs, "vs r7 r0 LSR 31", "vs_r7_r0_LSR_31"},
     {{ne, r3, r2, ASR, 18}, true, ne, "ne r3 r2 ASR 18", "ne_r3_r2_ASR_18"},
     {{vs, r2, r2, LSR, 18}, true, vs, "vs r2 r2 LSR 18", "vs_r2_r2_LSR_18"},
     {{mi, r6, r2, ASR, 25}, true, mi, "mi r6 r2 ASR 25", "mi_r6_r2_ASR_25"},
     {{hi, r7, r6, ASR, 28}, true, hi, "hi r7 r6 ASR 28", "hi_r7_r6_ASR_28"},
     {{gt, r2, r1, ASR, 3}, true, gt, "gt r2 r1 ASR 3", "gt_r2_r1_ASR_3"},
     {{ne, r0, r1, ASR, 31}, true, ne, "ne r0 r1 ASR 31", "ne_r0_r1_ASR_31"},
     {{hi, r2, r7, ASR, 24}, true, hi, "hi r2 r7 ASR 24", "hi_r2_r7_ASR_24"},
     {{hi, r2, r5, ASR, 30}, true, hi, "hi r2 r5 ASR 30", "hi_r2_r5_ASR_30"},
     {{gt, r3, r2, ASR, 11}, true, gt, "gt r3 r2 ASR 11", "gt_r3_r2_ASR_11"},
     {{ge, r4, r0, ASR, 3}, true, ge, "ge r4 r0 ASR 3", "ge_r4_r0_ASR_3"},
     {{mi, r0, r4, LSR, 19}, true, mi, "mi r0 r4 LSR 19", "mi_r0_r4_LSR_19"},
     {{gt, r0, r5, ASR, 30}, true, gt, "gt r0 r5 ASR 30", "gt_r0_r5_ASR_30"},
     {{vs, r0, r3, LSR, 16}, true, vs, "vs r0 r3 LSR 16", "vs_r0_r3_LSR_16"},
     {{ls, r4, r1, LSR, 3}, true, ls, "ls r4 r1 LSR 3", "ls_r4_r1_LSR_3"},
     {{vc, r4, r3, ASR, 17}, true, vc, "vc r4 r3 ASR 17", "vc_r4_r3_ASR_17"},
     {{gt, r0, r1, LSR, 5}, true, gt, "gt r0 r1 LSR 5", "gt_r0_r1_LSR_5"},
     {{ls, r1, r4, ASR, 6}, true, ls, "ls r1 r4 ASR 6", "ls_r1_r4_ASR_6"},
     {{ge, r1, r1, ASR, 23}, true, ge, "ge r1 r1 ASR 23", "ge_r1_r1_ASR_23"},
     {{hi, r1, r3, ASR, 28}, true, hi, "hi r1 r3 ASR 28", "hi_r1_r3_ASR_28"},
     {{vc, r1, r1, ASR, 11}, true, vc, "vc r1 r1 ASR 11", "vc_r1_r1_ASR_11"},
     {{le, r0, r4, ASR, 24}, true, le, "le r0 r4 ASR 24", "le_r0_r4_ASR_24"},
     {{cc, r4, r7, LSR, 4}, true, cc, "cc r4 r7 LSR 4", "cc_r4_r7_LSR_4"},
     {{hi, r6, r2, LSR, 7}, true, hi, "hi r6 r2 LSR 7", "hi_r6_r2_LSR_7"},
     {{le, r1, r1, LSR, 22}, true, le, "le r1 r1 LSR 22", "le_r1_r1_LSR_22"},
     {{le, r6, r4, LSR, 6}, true, le, "le r6 r4 LSR 6", "le_r6_r4_LSR_6"},
     {{le, r2, r5, ASR, 11}, true, le, "le r2 r5 ASR 11", "le_r2_r5_ASR_11"},
     {{vc, r5, r1, ASR, 2}, true, vc, "vc r5 r1 ASR 2", "vc_r5_r1_ASR_2"},
     {{lt, r7, r4, LSR, 31}, true, lt, "lt r7 r4 LSR 31", "lt_r7_r4_LSR_31"},
     {{hi, r6, r1, LSR, 28}, true, hi, "hi r6 r1 LSR 28", "hi_r6_r1_LSR_28"},
     {{ne, r7, r7, LSR, 24}, true, ne, "ne r7 r7 LSR 24", "ne_r7_r7_LSR_24"},
     {{ge, r7, r6, LSR, 32}, true, ge, "ge r7 r6 LSR 32", "ge_r7_r6_LSR_32"},
     {{hi, r6, r4, LSR, 25}, true, hi, "hi r6 r4 LSR 25", "hi_r6_r4_LSR_25"},
     {{pl, r2, r6, ASR, 3}, true, pl, "pl r2 r6 ASR 3", "pl_r2_r6_ASR_3"},
     {{ls, r5, r1, LSR, 20}, true, ls, "ls r5 r1 LSR 20", "ls_r5_r1_LSR_20"},
     {{hi, r1, r1, LSR, 10}, true, hi, "hi r1 r1 LSR 10", "hi_r1_r1_LSR_10"},
     {{lt, r6, r1, ASR, 29}, true, lt, "lt r6 r1 ASR 29", "lt_r6_r1_ASR_29"},
     {{mi, r7, r5, ASR, 14}, true, mi, "mi r7 r5 ASR 14", "mi_r7_r5_ASR_14"},
     {{le, r1, r0, ASR, 5}, true, le, "le r1 r0 ASR 5", "le_r1_r0_ASR_5"},
     {{gt, r7, r2, ASR, 18}, true, gt, "gt r7 r2 ASR 18", "gt_r7_r2_ASR_18"},
     {{pl, r5, r4, LSR, 12}, true, pl, "pl r5 r4 LSR 12", "pl_r5_r4_LSR_12"},
     {{mi, r1, r3, ASR, 21}, true, mi, "mi r1 r3 ASR 21", "mi_r1_r3_ASR_21"},
     {{mi, r7, r7, LSR, 13}, true, mi, "mi r7 r7 LSR 13", "mi_r7_r7_LSR_13"},
     {{gt, r1, r4, LSR, 7}, true, gt, "gt r1 r4 LSR 7", "gt_r1_r4_LSR_7"},
     {{vc, r5, r5, ASR, 28}, true, vc, "vc r5 r5 ASR 28", "vc_r5_r5_ASR_28"},
     {{mi, r3, r3, LSR, 24}, true, mi, "mi r3 r3 LSR 24", "mi_r3_r3_LSR_24"},
     {{ls, r4, r7, LSR, 32}, true, ls, "ls r4 r7 LSR 32", "ls_r4_r7_LSR_32"},
     {{mi, r1, r4, LSR, 18}, true, mi, "mi r1 r4 LSR 18", "mi_r1_r4_LSR_18"},
     {{le, r3, r6, LSR, 10}, true, le, "le r3 r6 LSR 10", "le_r3_r6_LSR_10"},
     {{gt, r3, r3, ASR, 17}, true, gt, "gt r3 r3 ASR 17", "gt_r3_r3_ASR_17"},
     {{ls, r1, r5, LSR, 25}, true, ls, "ls r1 r5 LSR 25", "ls_r1_r5_LSR_25"},
     {{vs, r1, r0, ASR, 11}, true, vs, "vs r1 r0 ASR 11", "vs_r1_r0_ASR_11"},
     {{cs, r0, r6, ASR, 17}, true, cs, "cs r0 r6 ASR 17", "cs_r0_r6_ASR_17"},
     {{cs, r0, r6, LSR, 14}, true, cs, "cs r0 r6 LSR 14", "cs_r0_r6_LSR_14"},
     {{lt, r4, r6, LSR, 13}, true, lt, "lt r4 r6 LSR 13", "lt_r4_r6_LSR_13"},
     {{vs, r6, r4, ASR, 23}, true, vs, "vs r6 r4 ASR 23", "vs_r6_r4_ASR_23"},
     {{cc, r4, r4, ASR, 12}, true, cc, "cc r4 r4 ASR 12", "cc_r4_r4_ASR_12"},
     {{lt, r1, r6, LSR, 28}, true, lt, "lt r1 r6 LSR 28", "lt_r1_r6_LSR_28"},
     {{cc, r4, r2, ASR, 2}, true, cc, "cc r4 r2 ASR 2", "cc_r4_r2_ASR_2"},
     {{ls, r2, r0, ASR, 27}, true, ls, "ls r2 r0 ASR 27", "ls_r2_r0_ASR_27"},
     {{le, r6, r3, LSR, 32}, true, le, "le r6 r3 LSR 32", "le_r6_r3_LSR_32"},
     {{cs, r6, r7, ASR, 10}, true, cs, "cs r6 r7 ASR 10", "cs_r6_r7_ASR_10"},
     {{vs, r0, r1, ASR, 31}, true, vs, "vs r0 r1 ASR 31", "vs_r0_r1_ASR_31"},
     {{vc, r2, r3, ASR, 2}, true, vc, "vc r2 r3 ASR 2", "vc_r2_r3_ASR_2"},
     {{ge, r1, r7, ASR, 24}, true, ge, "ge r1 r7 ASR 24", "ge_r1_r7_ASR_24"},
     {{eq, r3, r2, LSR, 21}, true, eq, "eq r3 r2 LSR 21", "eq_r3_r2_LSR_21"},
     {{ge, r4, r3, ASR, 22}, true, ge, "ge r4 r3 ASR 22", "ge_r4_r3_ASR_22"},
     {{hi, r2, r0, LSR, 1}, true, hi, "hi r2 r0 LSR 1", "hi_r2_r0_LSR_1"},
     {{vs, r2, r4, LSR, 9}, true, vs, "vs r2 r4 LSR 9", "vs_r2_r4_LSR_9"},
     {{ls, r5, r3, LSR, 20}, true, ls, "ls r5 r3 LSR 20", "ls_r5_r3_LSR_20"},
     {{cs, r4, r0, ASR, 5}, true, cs, "cs r4 r0 ASR 5", "cs_r4_r0_ASR_5"},
     {{lt, r6, r2, LSR, 5}, true, lt, "lt r6 r2 LSR 5", "lt_r6_r2_LSR_5"},
     {{ls, r4, r6, LSR, 24}, true, ls, "ls r4 r6 LSR 24", "ls_r4_r6_LSR_24"},
     {{le, r6, r3, LSR, 26}, true, le, "le r6 r3 LSR 26", "le_r6_r3_LSR_26"},
     {{ne, r4, r5, ASR, 27}, true, ne, "ne r4 r5 ASR 27", "ne_r4_r5_ASR_27"},
     {{hi, r2, r1, LSR, 19}, true, hi, "hi r2 r1 LSR 19", "hi_r2_r1_LSR_19"},
     {{mi, r2, r3, LSR, 17}, true, mi, "mi r2 r3 LSR 17", "mi_r2_r3_LSR_17"},
     {{eq, r6, r4, LSR, 3}, true, eq, "eq r6 r4 LSR 3", "eq_r6_r4_LSR_3"},
     {{ne, r3, r1, LSR, 5}, true, ne, "ne r3 r1 LSR 5", "ne_r3_r1_LSR_5"},
     {{vs, r1, r4, ASR, 4}, true, vs, "vs r1 r4 ASR 4", "vs_r1_r4_ASR_4"},
     {{ls, r0, r3, LSR, 22}, true, ls, "ls r0 r3 LSR 22", "ls_r0_r3_LSR_22"},
     {{mi, r6, r4, LSR, 1}, true, mi, "mi r6 r4 LSR 1", "mi_r6_r4_LSR_1"},
     {{hi, r5, r4, LSR, 12}, true, hi, "hi r5 r4 LSR 12", "hi_r5_r4_LSR_12"},
     {{le, r0, r3, LSR, 3}, true, le, "le r0 r3 LSR 3", "le_r0_r3_LSR_3"},
     {{pl, r1, r6, LSR, 30}, true, pl, "pl r1 r6 LSR 30", "pl_r1_r6_LSR_30"},
     {{ne, r7, r2, ASR, 31}, true, ne, "ne r7 r2 ASR 31", "ne_r7_r2_ASR_31"},
     {{ge, r6, r2, ASR, 11}, true, ge, "ge r6 r2 ASR 11", "ge_r6_r2_ASR_11"},
     {{pl, r0, r1, LSR, 28}, true, pl, "pl r0 r1 LSR 28", "pl_r0_r1_LSR_28"},
     {{lt, r6, r4, LSR, 23}, true, lt, "lt r6 r4 LSR 23", "lt_r6_r4_LSR_23"},
     {{mi, r2, r4, LSR, 1}, true, mi, "mi r2 r4 LSR 1", "mi_r2_r4_LSR_1"},
     {{mi, r7, r3, ASR, 18}, true, mi, "mi r7 r3 ASR 18", "mi_r7_r3_ASR_18"},
     {{ls, r4, r1, LSR, 24}, true, ls, "ls r4 r1 LSR 24", "ls_r4_r1_LSR_24"},
     {{ne, r4, r4, ASR, 7}, true, ne, "ne r4 r4 ASR 7", "ne_r4_r4_ASR_7"},
     {{lt, r4, r4, ASR, 23}, true, lt, "lt r4 r4 ASR 23", "lt_r4_r4_ASR_23"},
     {{vs, r7, r2, ASR, 24}, true, vs, "vs r7 r2 ASR 24", "vs_r7_r2_ASR_24"},
     {{cs, r2, r4, ASR, 24}, true, cs, "cs r2 r4 ASR 24", "cs_r2_r4_ASR_24"},
     {{ge, r6, r0, LSR, 9}, true, ge, "ge r6 r0 LSR 9", "ge_r6_r0_LSR_9"},
     {{mi, r2, r1, ASR, 10}, true, mi, "mi r2 r1 ASR 10", "mi_r2_r1_ASR_10"},
     {{mi, r6, r3, ASR, 13}, true, mi, "mi r6 r3 ASR 13", "mi_r6_r3_ASR_13"},
     {{vc, r4, r6, ASR, 28}, true, vc, "vc r4 r6 ASR 28", "vc_r4_r6_ASR_28"},
     {{pl, r6, r7, ASR, 1}, true, pl, "pl r6 r7 ASR 1", "pl_r6_r7_ASR_1"},
     {{gt, r1, r2, ASR, 18}, true, gt, "gt r1 r2 ASR 18", "gt_r1_r2_ASR_18"},
     {{hi, r6, r2, LSR, 32}, true, hi, "hi r6 r2 LSR 32", "hi_r6_r2_LSR_32"},
     {{eq, r3, r0, ASR, 10}, true, eq, "eq r3 r0 ASR 10", "eq_r3_r0_ASR_10"},
     {{mi, r0, r3, ASR, 8}, true, mi, "mi r0 r3 ASR 8", "mi_r0_r3_ASR_8"},
     {{pl, r6, r0, ASR, 27}, true, pl, "pl r6 r0 ASR 27", "pl_r6_r0_ASR_27"},
     {{lt, r6, r4, ASR, 28}, true, lt, "lt r6 r4 ASR 28", "lt_r6_r4_ASR_28"},
     {{ne, r6, r3, ASR, 25}, true, ne, "ne r6 r3 ASR 25", "ne_r6_r3_ASR_25"},
     {{lt, r1, r1, LSR, 16}, true, lt, "lt r1 r1 LSR 16", "lt_r1_r1_LSR_16"},
     {{vs, r4, r3, ASR, 18}, true, vs, "vs r4 r3 ASR 18", "vs_r4_r3_ASR_18"},
     {{vs, r3, r5, LSR, 16}, true, vs, "vs r3 r5 LSR 16", "vs_r3_r5_LSR_16"},
     {{le, r6, r1, LSR, 31}, true, le, "le r6 r1 LSR 31", "le_r6_r1_LSR_31"},
     {{ls, r6, r3, LSR, 14}, true, ls, "ls r6 r3 LSR 14", "ls_r6_r3_LSR_14"},
     {{pl, r3, r6, ASR, 17}, true, pl, "pl r3 r6 ASR 17", "pl_r3_r6_ASR_17"},
     {{vc, r7, r7, ASR, 1}, true, vc, "vc r7 r7 ASR 1", "vc_r7_r7_ASR_1"},
     {{cc, r2, r0, ASR, 17}, true, cc, "cc r2 r0 ASR 17", "cc_r2_r0_ASR_17"},
     {{le, r7, r1, LSR, 32}, true, le, "le r7 r1 LSR 32", "le_r7_r1_LSR_32"},
     {{eq, r1, r3, ASR, 8}, true, eq, "eq r1 r3 ASR 8", "eq_r1_r3_ASR_8"},
     {{vc, r4, r5, LSR, 18}, true, vc, "vc r4 r5 LSR 18", "vc_r4_r5_LSR_18"},
     {{hi, r0, r2, ASR, 32}, true, hi, "hi r0 r2 ASR 32", "hi_r0_r2_ASR_32"},
     {{le, r4, r6, ASR, 4}, true, le, "le r4 r6 ASR 4", "le_r4_r6_ASR_4"},
     {{mi, r5, r5, ASR, 7}, true, mi, "mi r5 r5 ASR 7", "mi_r5_r5_ASR_7"},
     {{eq, r6, r5, LSR, 24}, true, eq, "eq r6 r5 LSR 24", "eq_r6_r5_LSR_24"},
     {{pl, r5, r5, ASR, 9}, true, pl, "pl r5 r5 ASR 9", "pl_r5_r5_ASR_9"},
     {{ne, r6, r2, ASR, 24}, true, ne, "ne r6 r2 ASR 24", "ne_r6_r2_ASR_24"},
     {{eq, r3, r7, ASR, 3}, true, eq, "eq r3 r7 ASR 3", "eq_r3_r7_ASR_3"},
     {{lt, r6, r0, LSR, 25}, true, lt, "lt r6 r0 LSR 25", "lt_r6_r0_LSR_25"},
     {{ls, r5, r5, ASR, 8}, true, ls, "ls r5 r5 ASR 8", "ls_r5_r5_ASR_8"},
     {{hi, r5, r6, ASR, 17}, true, hi, "hi r5 r6 ASR 17", "hi_r5_r6_ASR_17"},
     {{ne, r4, r0, ASR, 18}, true, ne, "ne r4 r0 ASR 18", "ne_r4_r0_ASR_18"},
     {{mi, r2, r4, LSR, 30}, true, mi, "mi r2 r4 LSR 30", "mi_r2_r4_LSR_30"},
     {{cc, r1, r6, ASR, 5}, true, cc, "cc r1 r6 ASR 5", "cc_r1_r6_ASR_5"},
     {{hi, r7, r2, LSR, 15}, true, hi, "hi r7 r2 LSR 15", "hi_r7_r2_LSR_15"},
     {{cc, r7, r7, ASR, 29}, true, cc, "cc r7 r7 ASR 29", "cc_r7_r7_ASR_29"},
     {{eq, r7, r4, ASR, 22}, true, eq, "eq r7 r4 ASR 22", "eq_r7_r4_ASR_22"},
     {{mi, r3, r3, ASR, 2}, true, mi, "mi r3 r3 ASR 2", "mi_r3_r3_ASR_2"},
     {{le, r2, r5, LSR, 14}, true, le, "le r2 r5 LSR 14", "le_r2_r5_LSR_14"},
     {{pl, r6, r5, ASR, 12}, true, pl, "pl r6 r5 ASR 12", "pl_r6_r5_ASR_12"},
     {{ne, r5, r6, LSR, 11}, true, ne, "ne r5 r6 LSR 11", "ne_r5_r6_LSR_11"},
     {{cs, r0, r1, ASR, 29}, true, cs, "cs r0 r1 ASR 29", "cs_r0_r1_ASR_29"},
     {{cc, r3, r6, LSR, 5}, true, cc, "cc r3 r6 LSR 5", "cc_r3_r6_LSR_5"},
     {{ge, r5, r4, LSR, 10}, true, ge, "ge r5 r4 LSR 10", "ge_r5_r4_LSR_10"},
     {{vs, r7, r5, ASR, 9}, true, vs, "vs r7 r5 ASR 9", "vs_r7_r5_ASR_9"},
     {{ge, r6, r4, LSR, 22}, true, ge, "ge r6 r4 LSR 22", "ge_r6_r4_LSR_22"},
     {{vs, r0, r7, ASR, 20}, true, vs, "vs r0 r7 ASR 20", "vs_r0_r7_ASR_20"},
     {{ls, r1, r5, LSR, 21}, true, ls, "ls r1 r5 LSR 21", "ls_r1_r5_LSR_21"},
     {{cc, r3, r3, ASR, 16}, true, cc, "cc r3 r3 ASR 16", "cc_r3_r3_ASR_16"},
     {{hi, r2, r3, ASR, 30}, true, hi, "hi r2 r3 ASR 30", "hi_r2_r3_ASR_30"},
     {{cs, r5, r3, LSR, 12}, true, cs, "cs r5 r3 LSR 12", "cs_r5_r3_LSR_12"},
     {{cc, r5, r5, ASR, 24}, true, cc, "cc r5 r5 ASR 24", "cc_r5_r5_ASR_24"},
     {{vc, r7, r0, ASR, 18}, true, vc, "vc r7 r0 ASR 18", "vc_r7_r0_ASR_18"},
     {{mi, r4, r7, ASR, 30}, true, mi, "mi r4 r7 ASR 30", "mi_r4_r7_ASR_30"},
     {{vc, r6, r0, LSR, 17}, true, vc, "vc r6 r0 LSR 17", "vc_r6_r0_LSR_17"},
     {{eq, r3, r3, ASR, 2}, true, eq, "eq r3 r3 ASR 2", "eq_r3_r3_ASR_2"},
     {{ne, r5, r2, LSR, 31}, true, ne, "ne r5 r2 LSR 31", "ne_r5_r2_LSR_31"},
     {{ne, r4, r2, ASR, 6}, true, ne, "ne r4 r2 ASR 6", "ne_r4_r2_ASR_6"},
     {{eq, r3, r7, ASR, 25}, true, eq, "eq r3 r7 ASR 25", "eq_r3_r7_ASR_25"},
     {{pl, r1, r7, LSR, 11}, true, pl, "pl r1 r7 LSR 11", "pl_r1_r7_LSR_11"},
     {{lt, r4, r4, ASR, 1}, true, lt, "lt r4 r4 ASR 1", "lt_r4_r4_ASR_1"},
     {{vc, r7, r4, LSR, 8}, true, vc, "vc r7 r4 LSR 8", "vc_r7_r4_LSR_8"},
     {{ls, r1, r4, ASR, 4}, true, ls, "ls r1 r4 ASR 4", "ls_r1_r4_ASR_4"},
     {{cc, r2, r4, LSR, 18}, true, cc, "cc r2 r4 LSR 18", "cc_r2_r4_LSR_18"},
     {{gt, r0, r5, LSR, 8}, true, gt, "gt r0 r5 LSR 8", "gt_r0_r5_LSR_8"},
     {{lt, r4, r1, LSR, 4}, true, lt, "lt r4 r1 LSR 4", "lt_r4_r1_LSR_4"},
     {{gt, r7, r7, ASR, 12}, true, gt, "gt r7 r7 ASR 12", "gt_r7_r7_ASR_12"},
     {{vs, r0, r6, LSR, 28}, true, vs, "vs r0 r6 LSR 28", "vs_r0_r6_LSR_28"},
     {{vs, r0, r5, LSR, 25}, true, vs, "vs r0 r5 LSR 25", "vs_r0_r5_LSR_25"},
     {{pl, r7, r1, ASR, 13}, true, pl, "pl r7 r1 ASR 13", "pl_r7_r1_ASR_13"},
     {{le, r7, r0, LSR, 28}, true, le, "le r7 r0 LSR 28", "le_r7_r0_LSR_28"},
     {{vs, r2, r5, LSR, 25}, true, vs, "vs r2 r5 LSR 25", "vs_r2_r5_LSR_25"},
     {{cs, r0, r5, LSR, 14}, true, cs, "cs r0 r5 LSR 14", "cs_r0_r5_LSR_14"},
     {{cs, r0, r5, ASR, 31}, true, cs, "cs r0 r5 ASR 31", "cs_r0_r5_ASR_31"},
     {{pl, r5, r5, ASR, 13}, true, pl, "pl r5 r5 ASR 13", "pl_r5_r5_ASR_13"},
     {{vc, r2, r1, LSR, 18}, true, vc, "vc r2 r1 LSR 18", "vc_r2_r1_LSR_18"},
     {{hi, r2, r1, ASR, 31}, true, hi, "hi r2 r1 ASR 31", "hi_r2_r1_ASR_31"},
     {{cc, r6, r0, LSR, 11}, true, cc, "cc r6 r0 LSR 11", "cc_r6_r0_LSR_11"},
     {{pl, r6, r0, LSR, 1}, true, pl, "pl r6 r0 LSR 1", "pl_r6_r0_LSR_1"},
     {{lt, r5, r1, LSR, 3}, true, lt, "lt r5 r1 LSR 3", "lt_r5_r1_LSR_3"},
     {{eq, r3, r6, LSR, 2}, true, eq, "eq r3 r6 LSR 2", "eq_r3_r6_LSR_2"},
     {{mi, r7, r3, ASR, 5}, true, mi, "mi r7 r3 ASR 5", "mi_r7_r3_ASR_5"},
     {{vs, r5, r5, LSR, 3}, true, vs, "vs r5 r5 LSR 3", "vs_r5_r5_LSR_3"},
     {{hi, r5, r3, ASR, 18}, true, hi, "hi r5 r3 ASR 18", "hi_r5_r3_ASR_18"},
     {{cc, r1, r6, ASR, 13}, true, cc, "cc r1 r6 ASR 13", "cc_r1_r6_ASR_13"},
     {{vs, r7, r1, LSR, 25}, true, vs, "vs r7 r1 LSR 25", "vs_r7_r1_LSR_25"},
     {{lt, r1, r4, ASR, 22}, true, lt, "lt r1 r4 ASR 22", "lt_r1_r4_ASR_22"},
     {{ls, r4, r5, LSR, 13}, true, ls, "ls r4 r5 LSR 13", "ls_r4_r5_LSR_13"},
     {{cc, r4, r7, LSR, 26}, true, cc, "cc r4 r7 LSR 26", "cc_r4_r7_LSR_26"},
     {{cs, r7, r7, LSR, 21}, true, cs, "cs r7 r7 LSR 21", "cs_r7_r7_LSR_21"},
     {{lt, r4, r4, ASR, 2}, true, lt, "lt r4 r4 ASR 2", "lt_r4_r4_ASR_2"},
     {{eq, r7, r1, ASR, 22}, true, eq, "eq r7 r1 ASR 22", "eq_r7_r1_ASR_22"},
     {{vc, r7, r6, ASR, 32}, true, vc, "vc r7 r6 ASR 32", "vc_r7_r6_ASR_32"},
     {{cs, r1, r7, LSR, 5}, true, cs, "cs r1 r7 LSR 5", "cs_r1_r7_LSR_5"},
     {{vs, r6, r2, LSR, 19}, true, vs, "vs r6 r2 LSR 19", "vs_r6_r2_LSR_19"},
     {{cs, r3, r2, ASR, 16}, true, cs, "cs r3 r2 ASR 16", "cs_r3_r2_ASR_16"},
     {{vs, r2, r3, LSR, 27}, true, vs, "vs r2 r3 LSR 27", "vs_r2_r3_LSR_27"},
     {{pl, r3, r3, LSR, 29}, true, pl, "pl r3 r3 LSR 29", "pl_r3_r3_LSR_29"},
     {{lt, r3, r4, LSR, 24}, true, lt, "lt r3 r4 LSR 24", "lt_r3_r4_LSR_24"},
     {{le, r0, r4, LSR, 15}, true, le, "le r0 r4 LSR 15", "le_r0_r4_LSR_15"},
     {{ne, r6, r1, ASR, 8}, true, ne, "ne r6 r1 ASR 8", "ne_r6_r1_ASR_8"},
     {{pl, r2, r1, LSR, 31}, true, pl, "pl r2 r1 LSR 31", "pl_r2_r1_LSR_31"},
     {{vs, r1, r4, ASR, 19}, true, vs, "vs r1 r4 ASR 19", "vs_r1_r4_ASR_19"},
     {{pl, r4, r1, LSR, 15}, true, pl, "pl r4 r1 LSR 15", "pl_r4_r1_LSR_15"},
     {{pl, r0, r7, LSR, 10}, true, pl, "pl r0 r7 LSR 10", "pl_r0_r7_LSR_10"},
     {{eq, r6, r0, ASR, 7}, true, eq, "eq r6 r0 ASR 7", "eq_r6_r0_ASR_7"},
     {{ne, r2, r6, LSR, 12}, true, ne, "ne r2 r6 LSR 12", "ne_r2_r6_LSR_12"},
     {{ls, r0, r3, ASR, 4}, true, ls, "ls r0 r3 ASR 4", "ls_r0_r3_ASR_4"},
     {{cs, r3, r5, ASR, 11}, true, cs, "cs r3 r5 ASR 11", "cs_r3_r5_ASR_11"},
     {{gt, r7, r0, ASR, 19}, true, gt, "gt r7 r0 ASR 19", "gt_r7_r0_ASR_19"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-operand-rn-shift-amount-1to32-in-it-block-mov-t32.h"


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
    uint32_t amount = kTests[i].operands.amount;
    Operand op(rn, shift, amount);

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
      "AARCH32_ASSEMBLER_COND_RD_OPERAND_RN_SHIFT_AMOUNT_1TO32_IN_IT_"      \
      "BLOCK_" #mnemonic "_T32",                                            \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
