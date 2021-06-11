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
#define FOREACH_INSTRUCTION(M) M(add)


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
  Register rm;
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
    {{{pl, r13, r13, r1}, true, pl, "pl r13 r13 r1", "pl_r13_r13_r1"},
     {{mi, r11, r11, r4}, true, mi, "mi r11 r11 r4", "mi_r11_r11_r4"},
     {{vs, r4, r4, r2}, true, vs, "vs r4 r4 r2", "vs_r4_r4_r2"},
     {{ls, r7, r7, r6}, true, ls, "ls r7 r7 r6", "ls_r7_r7_r6"},
     {{le, r4, r4, r2}, true, le, "le r4 r4 r2", "le_r4_r4_r2"},
     {{vc, r11, r11, r0}, true, vc, "vc r11 r11 r0", "vc_r11_r11_r0"},
     {{le, r6, r6, r11}, true, le, "le r6 r6 r11", "le_r6_r6_r11"},
     {{eq, r4, r4, r9}, true, eq, "eq r4 r4 r9", "eq_r4_r4_r9"},
     {{hi, r12, r12, r2}, true, hi, "hi r12 r12 r2", "hi_r12_r12_r2"},
     {{hi, r11, r11, r1}, true, hi, "hi r11 r11 r1", "hi_r11_r11_r1"},
     {{cc, r0, r0, r11}, true, cc, "cc r0 r0 r11", "cc_r0_r0_r11"},
     {{pl, r2, r2, r2}, true, pl, "pl r2 r2 r2", "pl_r2_r2_r2"},
     {{ge, r7, r7, r12}, true, ge, "ge r7 r7 r12", "ge_r7_r7_r12"},
     {{gt, r14, r14, r11}, true, gt, "gt r14 r14 r11", "gt_r14_r14_r11"},
     {{le, r10, r10, r7}, true, le, "le r10 r10 r7", "le_r10_r10_r7"},
     {{vs, r7, r7, r7}, true, vs, "vs r7 r7 r7", "vs_r7_r7_r7"},
     {{lt, r3, r3, r4}, true, lt, "lt r3 r3 r4", "lt_r3_r3_r4"},
     {{ne, r1, r1, r8}, true, ne, "ne r1 r1 r8", "ne_r1_r1_r8"},
     {{gt, r7, r7, r12}, true, gt, "gt r7 r7 r12", "gt_r7_r7_r12"},
     {{ls, r14, r14, r11}, true, ls, "ls r14 r14 r11", "ls_r14_r14_r11"},
     {{ne, r4, r4, r7}, true, ne, "ne r4 r4 r7", "ne_r4_r4_r7"},
     {{ge, r12, r12, r2}, true, ge, "ge r12 r12 r2", "ge_r12_r12_r2"},
     {{ne, r4, r4, r1}, true, ne, "ne r4 r4 r1", "ne_r4_r4_r1"},
     {{le, r11, r11, r4}, true, le, "le r11 r11 r4", "le_r11_r11_r4"},
     {{gt, r12, r12, r5}, true, gt, "gt r12 r12 r5", "gt_r12_r12_r5"},
     {{ge, r14, r14, r4}, true, ge, "ge r14 r14 r4", "ge_r14_r14_r4"},
     {{hi, r2, r2, r2}, true, hi, "hi r2 r2 r2", "hi_r2_r2_r2"},
     {{gt, r8, r8, r4}, true, gt, "gt r8 r8 r4", "gt_r8_r8_r4"},
     {{lt, r13, r13, r14}, true, lt, "lt r13 r13 r14", "lt_r13_r13_r14"},
     {{cs, r5, r5, r8}, true, cs, "cs r5 r5 r8", "cs_r5_r5_r8"},
     {{ge, r3, r3, r12}, true, ge, "ge r3 r3 r12", "ge_r3_r3_r12"},
     {{hi, r0, r0, r8}, true, hi, "hi r0 r0 r8", "hi_r0_r0_r8"},
     {{vs, r13, r13, r7}, true, vs, "vs r13 r13 r7", "vs_r13_r13_r7"},
     {{ne, r13, r13, r10}, true, ne, "ne r13 r13 r10", "ne_r13_r13_r10"},
     {{cc, r0, r0, r6}, true, cc, "cc r0 r0 r6", "cc_r0_r0_r6"},
     {{gt, r4, r4, r6}, true, gt, "gt r4 r4 r6", "gt_r4_r4_r6"},
     {{lt, r2, r2, r11}, true, lt, "lt r2 r2 r11", "lt_r2_r2_r11"},
     {{cc, r12, r12, r0}, true, cc, "cc r12 r12 r0", "cc_r12_r12_r0"},
     {{vc, r8, r8, r6}, true, vc, "vc r8 r8 r6", "vc_r8_r8_r6"},
     {{vs, r13, r13, r11}, true, vs, "vs r13 r13 r11", "vs_r13_r13_r11"},
     {{cc, r1, r1, r2}, true, cc, "cc r1 r1 r2", "cc_r1_r1_r2"},
     {{hi, r13, r13, r8}, true, hi, "hi r13 r13 r8", "hi_r13_r13_r8"},
     {{ls, r10, r10, r14}, true, ls, "ls r10 r10 r14", "ls_r10_r10_r14"},
     {{gt, r5, r5, r2}, true, gt, "gt r5 r5 r2", "gt_r5_r5_r2"},
     {{ne, r4, r4, r11}, true, ne, "ne r4 r4 r11", "ne_r4_r4_r11"},
     {{eq, r4, r4, r14}, true, eq, "eq r4 r4 r14", "eq_r4_r4_r14"},
     {{ls, r13, r13, r11}, true, ls, "ls r13 r13 r11", "ls_r13_r13_r11"},
     {{cc, r4, r4, r6}, true, cc, "cc r4 r4 r6", "cc_r4_r4_r6"},
     {{vc, r6, r6, r14}, true, vc, "vc r6 r6 r14", "vc_r6_r6_r14"},
     {{hi, r9, r9, r6}, true, hi, "hi r9 r9 r6", "hi_r9_r9_r6"},
     {{ne, r7, r7, r5}, true, ne, "ne r7 r7 r5", "ne_r7_r7_r5"},
     {{cs, r8, r8, r14}, true, cs, "cs r8 r8 r14", "cs_r8_r8_r14"},
     {{mi, r5, r5, r3}, true, mi, "mi r5 r5 r3", "mi_r5_r5_r3"},
     {{vc, r10, r10, r8}, true, vc, "vc r10 r10 r8", "vc_r10_r10_r8"},
     {{mi, r9, r9, r7}, true, mi, "mi r9 r9 r7", "mi_r9_r9_r7"},
     {{ls, r4, r4, r5}, true, ls, "ls r4 r4 r5", "ls_r4_r4_r5"},
     {{vc, r1, r1, r7}, true, vc, "vc r1 r1 r7", "vc_r1_r1_r7"},
     {{pl, r4, r4, r2}, true, pl, "pl r4 r4 r2", "pl_r4_r4_r2"},
     {{mi, r2, r2, r12}, true, mi, "mi r2 r2 r12", "mi_r2_r2_r12"},
     {{gt, r14, r14, r9}, true, gt, "gt r14 r14 r9", "gt_r14_r14_r9"},
     {{cc, r5, r5, r12}, true, cc, "cc r5 r5 r12", "cc_r5_r5_r12"},
     {{ne, r5, r5, r7}, true, ne, "ne r5 r5 r7", "ne_r5_r5_r7"},
     {{pl, r14, r14, r10}, true, pl, "pl r14 r14 r10", "pl_r14_r14_r10"},
     {{eq, r4, r4, r7}, true, eq, "eq r4 r4 r7", "eq_r4_r4_r7"},
     {{ls, r7, r7, r9}, true, ls, "ls r7 r7 r9", "ls_r7_r7_r9"},
     {{vc, r9, r9, r12}, true, vc, "vc r9 r9 r12", "vc_r9_r9_r12"},
     {{cc, r1, r1, r6}, true, cc, "cc r1 r1 r6", "cc_r1_r1_r6"},
     {{lt, r14, r14, r9}, true, lt, "lt r14 r14 r9", "lt_r14_r14_r9"},
     {{mi, r4, r4, r2}, true, mi, "mi r4 r4 r2", "mi_r4_r4_r2"},
     {{ls, r4, r4, r3}, true, ls, "ls r4 r4 r3", "ls_r4_r4_r3"},
     {{gt, r0, r0, r10}, true, gt, "gt r0 r0 r10", "gt_r0_r0_r10"},
     {{vs, r4, r4, r11}, true, vs, "vs r4 r4 r11", "vs_r4_r4_r11"},
     {{gt, r3, r3, r10}, true, gt, "gt r3 r3 r10", "gt_r3_r3_r10"},
     {{ne, r2, r2, r4}, true, ne, "ne r2 r2 r4", "ne_r2_r2_r4"},
     {{ne, r2, r2, r7}, true, ne, "ne r2 r2 r7", "ne_r2_r2_r7"},
     {{gt, r0, r0, r1}, true, gt, "gt r0 r0 r1", "gt_r0_r0_r1"},
     {{pl, r8, r8, r10}, true, pl, "pl r8 r8 r10", "pl_r8_r8_r10"},
     {{pl, r10, r10, r1}, true, pl, "pl r10 r10 r1", "pl_r10_r10_r1"},
     {{lt, r13, r13, r9}, true, lt, "lt r13 r13 r9", "lt_r13_r13_r9"},
     {{mi, r10, r10, r8}, true, mi, "mi r10 r10 r8", "mi_r10_r10_r8"},
     {{le, r6, r6, r4}, true, le, "le r6 r6 r4", "le_r6_r6_r4"},
     {{le, r0, r0, r0}, true, le, "le r0 r0 r0", "le_r0_r0_r0"},
     {{vc, r9, r9, r11}, true, vc, "vc r9 r9 r11", "vc_r9_r9_r11"},
     {{ls, r7, r7, r8}, true, ls, "ls r7 r7 r8", "ls_r7_r7_r8"},
     {{eq, r9, r9, r3}, true, eq, "eq r9 r9 r3", "eq_r9_r9_r3"},
     {{gt, r2, r2, r5}, true, gt, "gt r2 r2 r5", "gt_r2_r2_r5"},
     {{hi, r2, r2, r0}, true, hi, "hi r2 r2 r0", "hi_r2_r2_r0"},
     {{mi, r3, r3, r3}, true, mi, "mi r3 r3 r3", "mi_r3_r3_r3"},
     {{cs, r9, r9, r0}, true, cs, "cs r9 r9 r0", "cs_r9_r9_r0"},
     {{le, r4, r4, r9}, true, le, "le r4 r4 r9", "le_r4_r4_r9"},
     {{mi, r9, r9, r5}, true, mi, "mi r9 r9 r5", "mi_r9_r9_r5"},
     {{hi, r14, r14, r0}, true, hi, "hi r14 r14 r0", "hi_r14_r14_r0"},
     {{le, r8, r8, r10}, true, le, "le r8 r8 r10", "le_r8_r8_r10"},
     {{lt, r9, r9, r7}, true, lt, "lt r9 r9 r7", "lt_r9_r9_r7"},
     {{cc, r12, r12, r7}, true, cc, "cc r12 r12 r7", "cc_r12_r12_r7"},
     {{le, r4, r4, r8}, true, le, "le r4 r4 r8", "le_r4_r4_r8"},
     {{mi, r3, r3, r6}, true, mi, "mi r3 r3 r6", "mi_r3_r3_r6"},
     {{mi, r1, r1, r10}, true, mi, "mi r1 r1 r10", "mi_r1_r1_r10"},
     {{ls, r3, r3, r14}, true, ls, "ls r3 r3 r14", "ls_r3_r3_r14"},
     {{vs, r8, r8, r5}, true, vs, "vs r8 r8 r5", "vs_r8_r8_r5"},
     {{cs, r8, r8, r3}, true, cs, "cs r8 r8 r3", "cs_r8_r8_r3"},
     {{le, r3, r3, r9}, true, le, "le r3 r3 r9", "le_r3_r3_r9"},
     {{eq, r10, r10, r5}, true, eq, "eq r10 r10 r5", "eq_r10_r10_r5"},
     {{le, r7, r7, r0}, true, le, "le r7 r7 r0", "le_r7_r7_r0"},
     {{vc, r12, r12, r10}, true, vc, "vc r12 r12 r10", "vc_r12_r12_r10"},
     {{cc, r6, r6, r3}, true, cc, "cc r6 r6 r3", "cc_r6_r6_r3"},
     {{mi, r4, r4, r8}, true, mi, "mi r4 r4 r8", "mi_r4_r4_r8"},
     {{pl, r5, r5, r1}, true, pl, "pl r5 r5 r1", "pl_r5_r5_r1"},
     {{ge, r5, r5, r6}, true, ge, "ge r5 r5 r6", "ge_r5_r5_r6"},
     {{le, r3, r3, r10}, true, le, "le r3 r3 r10", "le_r3_r3_r10"},
     {{lt, r1, r1, r5}, true, lt, "lt r1 r1 r5", "lt_r1_r1_r5"},
     {{ls, r9, r9, r10}, true, ls, "ls r9 r9 r10", "ls_r9_r9_r10"},
     {{vc, r13, r13, r2}, true, vc, "vc r13 r13 r2", "vc_r13_r13_r2"},
     {{ne, r2, r2, r1}, true, ne, "ne r2 r2 r1", "ne_r2_r2_r1"},
     {{vs, r9, r9, r0}, true, vs, "vs r9 r9 r0", "vs_r9_r9_r0"},
     {{cc, r6, r6, r14}, true, cc, "cc r6 r6 r14", "cc_r6_r6_r14"},
     {{cc, r12, r12, r11}, true, cc, "cc r12 r12 r11", "cc_r12_r12_r11"},
     {{pl, r1, r1, r8}, true, pl, "pl r1 r1 r8", "pl_r1_r1_r8"},
     {{eq, r13, r13, r14}, true, eq, "eq r13 r13 r14", "eq_r13_r13_r14"},
     {{eq, r1, r1, r0}, true, eq, "eq r1 r1 r0", "eq_r1_r1_r0"},
     {{ge, r11, r11, r2}, true, ge, "ge r11 r11 r2", "ge_r11_r11_r2"},
     {{pl, r2, r2, r14}, true, pl, "pl r2 r2 r14", "pl_r2_r2_r14"},
     {{mi, r5, r5, r14}, true, mi, "mi r5 r5 r14", "mi_r5_r5_r14"},
     {{eq, r6, r6, r9}, true, eq, "eq r6 r6 r9", "eq_r6_r6_r9"},
     {{mi, r4, r4, r3}, true, mi, "mi r4 r4 r3", "mi_r4_r4_r3"},
     {{hi, r13, r13, r2}, true, hi, "hi r13 r13 r2", "hi_r13_r13_r2"},
     {{pl, r3, r3, r0}, true, pl, "pl r3 r3 r0", "pl_r3_r3_r0"},
     {{eq, r12, r12, r1}, true, eq, "eq r12 r12 r1", "eq_r12_r12_r1"},
     {{cs, r4, r4, r14}, true, cs, "cs r4 r4 r14", "cs_r4_r4_r14"},
     {{hi, r1, r1, r5}, true, hi, "hi r1 r1 r5", "hi_r1_r1_r5"},
     {{cc, r5, r5, r7}, true, cc, "cc r5 r5 r7", "cc_r5_r5_r7"},
     {{cs, r13, r13, r6}, true, cs, "cs r13 r13 r6", "cs_r13_r13_r6"},
     {{mi, r4, r4, r5}, true, mi, "mi r4 r4 r5", "mi_r4_r4_r5"},
     {{lt, r11, r11, r11}, true, lt, "lt r11 r11 r11", "lt_r11_r11_r11"},
     {{cs, r12, r12, r8}, true, cs, "cs r12 r12 r8", "cs_r12_r12_r8"},
     {{hi, r8, r8, r5}, true, hi, "hi r8 r8 r5", "hi_r8_r8_r5"},
     {{vs, r5, r5, r14}, true, vs, "vs r5 r5 r14", "vs_r5_r5_r14"},
     {{pl, r9, r9, r9}, true, pl, "pl r9 r9 r9", "pl_r9_r9_r9"},
     {{mi, r6, r6, r11}, true, mi, "mi r6 r6 r11", "mi_r6_r6_r11"},
     {{ne, r12, r12, r10}, true, ne, "ne r12 r12 r10", "ne_r12_r12_r10"},
     {{gt, r3, r3, r6}, true, gt, "gt r3 r3 r6", "gt_r3_r3_r6"},
     {{gt, r6, r6, r4}, true, gt, "gt r6 r6 r4", "gt_r6_r6_r4"},
     {{le, r6, r6, r3}, true, le, "le r6 r6 r3", "le_r6_r6_r3"},
     {{ne, r6, r6, r12}, true, ne, "ne r6 r6 r12", "ne_r6_r6_r12"},
     {{cc, r6, r6, r12}, true, cc, "cc r6 r6 r12", "cc_r6_r6_r12"},
     {{ne, r10, r10, r4}, true, ne, "ne r10 r10 r4", "ne_r10_r10_r4"},
     {{eq, r0, r0, r12}, true, eq, "eq r0 r0 r12", "eq_r0_r0_r12"},
     {{ge, r4, r4, r9}, true, ge, "ge r4 r4 r9", "ge_r4_r4_r9"},
     {{gt, r6, r6, r9}, true, gt, "gt r6 r6 r9", "gt_r6_r6_r9"},
     {{le, r14, r14, r9}, true, le, "le r14 r14 r9", "le_r14_r14_r9"},
     {{pl, r9, r9, r3}, true, pl, "pl r9 r9 r3", "pl_r9_r9_r3"},
     {{lt, r4, r4, r8}, true, lt, "lt r4 r4 r8", "lt_r4_r4_r8"},
     {{cc, r7, r7, r12}, true, cc, "cc r7 r7 r12", "cc_r7_r7_r12"},
     {{vs, r9, r9, r12}, true, vs, "vs r9 r9 r12", "vs_r9_r9_r12"},
     {{pl, r5, r5, r14}, true, pl, "pl r5 r5 r14", "pl_r5_r5_r14"},
     {{le, r1, r1, r10}, true, le, "le r1 r1 r10", "le_r1_r1_r10"},
     {{lt, r3, r3, r10}, true, lt, "lt r3 r3 r10", "lt_r3_r3_r10"},
     {{cc, r4, r4, r1}, true, cc, "cc r4 r4 r1", "cc_r4_r4_r1"},
     {{ne, r4, r4, r14}, true, ne, "ne r4 r4 r14", "ne_r4_r4_r14"},
     {{vc, r14, r14, r2}, true, vc, "vc r14 r14 r2", "vc_r14_r14_r2"},
     {{pl, r3, r3, r11}, true, pl, "pl r3 r3 r11", "pl_r3_r3_r11"},
     {{pl, r12, r12, r5}, true, pl, "pl r12 r12 r5", "pl_r12_r12_r5"},
     {{hi, r10, r10, r6}, true, hi, "hi r10 r10 r6", "hi_r10_r10_r6"},
     {{gt, r3, r3, r4}, true, gt, "gt r3 r3 r4", "gt_r3_r3_r4"},
     {{gt, r13, r13, r5}, true, gt, "gt r13 r13 r5", "gt_r13_r13_r5"},
     {{ne, r12, r12, r6}, true, ne, "ne r12 r12 r6", "ne_r12_r12_r6"},
     {{eq, r6, r6, r5}, true, eq, "eq r6 r6 r5", "eq_r6_r6_r5"},
     {{vs, r6, r6, r12}, true, vs, "vs r6 r6 r12", "vs_r6_r6_r12"},
     {{lt, r14, r14, r8}, true, lt, "lt r14 r14 r8", "lt_r14_r14_r8"},
     {{ls, r10, r10, r4}, true, ls, "ls r10 r10 r4", "ls_r10_r10_r4"},
     {{cc, r12, r12, r3}, true, cc, "cc r12 r12 r3", "cc_r12_r12_r3"},
     {{ne, r13, r13, r11}, true, ne, "ne r13 r13 r11", "ne_r13_r13_r11"},
     {{lt, r4, r4, r0}, true, lt, "lt r4 r4 r0", "lt_r4_r4_r0"},
     {{mi, r14, r14, r2}, true, mi, "mi r14 r14 r2", "mi_r14_r14_r2"},
     {{ls, r11, r11, r1}, true, ls, "ls r11 r11 r1", "ls_r11_r11_r1"},
     {{hi, r1, r1, r11}, true, hi, "hi r1 r1 r11", "hi_r1_r1_r11"},
     {{ge, r14, r14, r12}, true, ge, "ge r14 r14 r12", "ge_r14_r14_r12"},
     {{cs, r4, r4, r3}, true, cs, "cs r4 r4 r3", "cs_r4_r4_r3"},
     {{hi, r5, r5, r3}, true, hi, "hi r5 r5 r3", "hi_r5_r5_r3"},
     {{vs, r13, r13, r4}, true, vs, "vs r13 r13 r4", "vs_r13_r13_r4"},
     {{le, r13, r13, r5}, true, le, "le r13 r13 r5", "le_r13_r13_r5"},
     {{vs, r11, r11, r9}, true, vs, "vs r11 r11 r9", "vs_r11_r11_r9"},
     {{lt, r8, r8, r6}, true, lt, "lt r8 r8 r6", "lt_r8_r8_r6"},
     {{gt, r7, r7, r0}, true, gt, "gt r7 r7 r0", "gt_r7_r7_r0"},
     {{eq, r8, r8, r1}, true, eq, "eq r8 r8 r1", "eq_r8_r8_r1"},
     {{hi, r3, r3, r2}, true, hi, "hi r3 r3 r2", "hi_r3_r3_r2"},
     {{pl, r4, r4, r1}, true, pl, "pl r4 r4 r1", "pl_r4_r4_r1"},
     {{pl, r8, r8, r14}, true, pl, "pl r8 r8 r14", "pl_r8_r8_r14"},
     {{lt, r8, r8, r7}, true, lt, "lt r8 r8 r7", "lt_r8_r8_r7"},
     {{lt, r3, r3, r11}, true, lt, "lt r3 r3 r11", "lt_r3_r3_r11"},
     {{ne, r5, r5, r2}, true, ne, "ne r5 r5 r2", "ne_r5_r5_r2"},
     {{mi, r10, r10, r6}, true, mi, "mi r10 r10 r6", "mi_r10_r10_r6"},
     {{vs, r3, r3, r5}, true, vs, "vs r3 r3 r5", "vs_r3_r3_r5"},
     {{cs, r5, r5, r1}, true, cs, "cs r5 r5 r1", "cs_r5_r5_r1"},
     {{ne, r0, r0, r0}, true, ne, "ne r0 r0 r0", "ne_r0_r0_r0"},
     {{mi, r11, r11, r12}, true, mi, "mi r11 r11 r12", "mi_r11_r11_r12"},
     {{ne, r7, r7, r14}, true, ne, "ne r7 r7 r14", "ne_r7_r7_r14"},
     {{eq, r14, r14, r2}, true, eq, "eq r14 r14 r2", "eq_r14_r14_r2"},
     {{ne, r1, r1, r0}, true, ne, "ne r1 r1 r0", "ne_r1_r1_r0"},
     {{hi, r5, r5, r14}, true, hi, "hi r5 r5 r14", "hi_r5_r5_r14"},
     {{lt, r7, r7, r11}, true, lt, "lt r7 r7 r11", "lt_r7_r7_r11"},
     {{gt, r10, r10, r12}, true, gt, "gt r10 r10 r12", "gt_r10_r10_r12"},
     {{lt, r9, r9, r6}, true, lt, "lt r9 r9 r6", "lt_r9_r9_r6"},
     {{pl, r7, r7, r14}, true, pl, "pl r7 r7 r14", "pl_r7_r7_r14"},
     {{eq, r8, r8, r3}, true, eq, "eq r8 r8 r3", "eq_r8_r8_r3"},
     {{cc, r2, r2, r1}, true, cc, "cc r2 r2 r1", "cc_r2_r2_r1"},
     {{mi, r2, r2, r1}, true, mi, "mi r2 r2 r1", "mi_r2_r2_r1"},
     {{vc, r13, r13, r6}, true, vc, "vc r13 r13 r6", "vc_r13_r13_r6"},
     {{vc, r14, r14, r9}, true, vc, "vc r14 r14 r9", "vc_r14_r14_r9"},
     {{ne, r6, r6, r0}, true, ne, "ne r6 r6 r0", "ne_r6_r6_r0"},
     {{ne, r8, r8, r9}, true, ne, "ne r8 r8 r9", "ne_r8_r8_r9"},
     {{mi, r6, r6, r4}, true, mi, "mi r6 r6 r4", "mi_r6_r6_r4"},
     {{vc, r4, r4, r11}, true, vc, "vc r4 r4 r11", "vc_r4_r4_r11"},
     {{lt, r3, r3, r1}, true, lt, "lt r3 r3 r1", "lt_r3_r3_r1"},
     {{pl, r5, r5, r11}, true, pl, "pl r5 r5 r11", "pl_r5_r5_r11"},
     {{mi, r5, r5, r11}, true, mi, "mi r5 r5 r11", "mi_r5_r5_r11"},
     {{hi, r1, r1, r7}, true, hi, "hi r1 r1 r7", "hi_r1_r1_r7"},
     {{hi, r7, r7, r9}, true, hi, "hi r7 r7 r9", "hi_r7_r7_r9"},
     {{vs, r11, r11, r1}, true, vs, "vs r11 r11 r1", "vs_r11_r11_r1"},
     {{cc, r13, r13, r4}, true, cc, "cc r13 r13 r4", "cc_r13_r13_r4"},
     {{mi, r6, r6, r0}, true, mi, "mi r6 r6 r0", "mi_r6_r6_r0"},
     {{mi, r2, r2, r6}, true, mi, "mi r2 r2 r6", "mi_r2_r2_r6"},
     {{eq, r9, r9, r14}, true, eq, "eq r9 r9 r14", "eq_r9_r9_r14"},
     {{eq, r5, r5, r12}, true, eq, "eq r5 r5 r12", "eq_r5_r5_r12"},
     {{cs, r13, r13, r14}, true, cs, "cs r13 r13 r14", "cs_r13_r13_r14"},
     {{eq, r3, r3, r12}, true, eq, "eq r3 r3 r12", "eq_r3_r3_r12"},
     {{cc, r13, r13, r10}, true, cc, "cc r13 r13 r10", "cc_r13_r13_r10"},
     {{eq, r11, r11, r11}, true, eq, "eq r11 r11 r11", "eq_r11_r11_r11"},
     {{vs, r12, r12, r10}, true, vs, "vs r12 r12 r10", "vs_r12_r12_r10"},
     {{vc, r10, r10, r6}, true, vc, "vc r10 r10 r6", "vc_r10_r10_r6"},
     {{le, r13, r13, r14}, true, le, "le r13 r13 r14", "le_r13_r13_r14"},
     {{eq, r2, r2, r10}, true, eq, "eq r2 r2 r10", "eq_r2_r2_r10"},
     {{lt, r8, r8, r11}, true, lt, "lt r8 r8 r11", "lt_r8_r8_r11"},
     {{vs, r0, r0, r10}, true, vs, "vs r0 r0 r10", "vs_r0_r0_r10"},
     {{hi, r10, r10, r14}, true, hi, "hi r10 r10 r14", "hi_r10_r10_r14"},
     {{mi, r4, r4, r1}, true, mi, "mi r4 r4 r1", "mi_r4_r4_r1"},
     {{ls, r12, r12, r8}, true, ls, "ls r12 r12 r8", "ls_r12_r12_r8"},
     {{mi, r7, r7, r14}, true, mi, "mi r7 r7 r14", "mi_r7_r7_r14"},
     {{ge, r6, r6, r11}, true, ge, "ge r6 r6 r11", "ge_r6_r6_r11"},
     {{vs, r11, r11, r0}, true, vs, "vs r11 r11 r0", "vs_r11_r11_r0"},
     {{vs, r11, r11, r12}, true, vs, "vs r11 r11 r12", "vs_r11_r11_r12"},
     {{vs, r2, r2, r12}, true, vs, "vs r2 r2 r12", "vs_r2_r2_r12"},
     {{ne, r7, r7, r7}, true, ne, "ne r7 r7 r7", "ne_r7_r7_r7"},
     {{vs, r12, r12, r6}, true, vs, "vs r12 r12 r6", "vs_r12_r12_r6"},
     {{cs, r11, r11, r14}, true, cs, "cs r11 r11 r14", "cs_r11_r11_r14"},
     {{pl, r3, r3, r12}, true, pl, "pl r3 r3 r12", "pl_r3_r3_r12"},
     {{eq, r11, r11, r9}, true, eq, "eq r11 r11 r9", "eq_r11_r11_r9"},
     {{ge, r13, r13, r4}, true, ge, "ge r13 r13 r4", "ge_r13_r13_r4"},
     {{eq, r14, r14, r5}, true, eq, "eq r14 r14 r5", "eq_r14_r14_r5"},
     {{lt, r5, r5, r14}, true, lt, "lt r5 r5 r14", "lt_r5_r5_r14"},
     {{ne, r2, r2, r6}, true, ne, "ne r2 r2 r6", "ne_r2_r2_r6"},
     {{gt, r1, r1, r14}, true, gt, "gt r1 r1 r14", "gt_r1_r1_r14"},
     {{ne, r14, r14, r0}, true, ne, "ne r14 r14 r0", "ne_r14_r14_r0"},
     {{le, r1, r1, r7}, true, le, "le r1 r1 r7", "le_r1_r1_r7"},
     {{vs, r7, r7, r3}, true, vs, "vs r7 r7 r3", "vs_r7_r7_r3"},
     {{le, r6, r6, r1}, true, le, "le r6 r6 r1", "le_r6_r6_r1"},
     {{lt, r6, r6, r9}, true, lt, "lt r6 r6 r9", "lt_r6_r6_r9"},
     {{cs, r7, r7, r12}, true, cs, "cs r7 r7 r12", "cs_r7_r7_r12"},
     {{pl, r14, r14, r7}, true, pl, "pl r14 r14 r7", "pl_r14_r14_r7"},
     {{gt, r13, r13, r0}, true, gt, "gt r13 r13 r0", "gt_r13_r13_r0"},
     {{cc, r7, r7, r8}, true, cc, "cc r7 r7 r8", "cc_r7_r7_r8"},
     {{vs, r6, r6, r10}, true, vs, "vs r6 r6 r10", "vs_r6_r6_r10"},
     {{eq, r11, r11, r2}, true, eq, "eq r11 r11 r2", "eq_r11_r11_r2"},
     {{vs, r1, r1, r0}, true, vs, "vs r1 r1 r0", "vs_r1_r1_r0"},
     {{gt, r11, r11, r3}, true, gt, "gt r11 r11 r3", "gt_r11_r11_r3"},
     {{lt, r2, r2, r6}, true, lt, "lt r2 r2 r6", "lt_r2_r2_r6"},
     {{ls, r7, r7, r4}, true, ls, "ls r7 r7 r4", "ls_r7_r7_r4"},
     {{vs, r12, r12, r7}, true, vs, "vs r12 r12 r7", "vs_r12_r12_r7"},
     {{eq, r10, r10, r8}, true, eq, "eq r10 r10 r8", "eq_r10_r10_r8"},
     {{gt, r12, r12, r1}, true, gt, "gt r12 r12 r1", "gt_r12_r12_r1"},
     {{ne, r6, r6, r1}, true, ne, "ne r6 r6 r1", "ne_r6_r6_r1"},
     {{gt, r5, r5, r3}, true, gt, "gt r5 r5 r3", "gt_r5_r5_r3"},
     {{vc, r4, r4, r8}, true, vc, "vc r4 r4 r8", "vc_r4_r4_r8"},
     {{mi, r14, r14, r9}, true, mi, "mi r14 r14 r9", "mi_r14_r14_r9"},
     {{le, r7, r7, r4}, true, le, "le r7 r7 r4", "le_r7_r7_r4"},
     {{ge, r6, r6, r5}, true, ge, "ge r6 r6 r5", "ge_r6_r6_r5"},
     {{ne, r10, r10, r14}, true, ne, "ne r10 r10 r14", "ne_r10_r10_r14"},
     {{lt, r5, r5, r0}, true, lt, "lt r5 r5 r0", "lt_r5_r5_r0"},
     {{mi, r9, r9, r14}, true, mi, "mi r9 r9 r14", "mi_r9_r9_r14"},
     {{ls, r8, r8, r2}, true, ls, "ls r8 r8 r2", "ls_r8_r8_r2"},
     {{gt, r9, r9, r9}, true, gt, "gt r9 r9 r9", "gt_r9_r9_r9"},
     {{hi, r0, r0, r10}, true, hi, "hi r0 r0 r10", "hi_r0_r0_r10"},
     {{lt, r7, r7, r8}, true, lt, "lt r7 r7 r8", "lt_r7_r7_r8"},
     {{eq, r6, r6, r4}, true, eq, "eq r6 r6 r4", "eq_r6_r6_r4"},
     {{cc, r5, r5, r2}, true, cc, "cc r5 r5 r2", "cc_r5_r5_r2"},
     {{le, r9, r9, r9}, true, le, "le r9 r9 r9", "le_r9_r9_r9"},
     {{gt, r11, r11, r8}, true, gt, "gt r11 r11 r8", "gt_r11_r11_r8"},
     {{le, r3, r3, r2}, true, le, "le r3 r3 r2", "le_r3_r3_r2"},
     {{ge, r0, r0, r0}, true, ge, "ge r0 r0 r0", "ge_r0_r0_r0"},
     {{ne, r10, r10, r9}, true, ne, "ne r10 r10 r9", "ne_r10_r10_r9"},
     {{vs, r5, r5, r2}, true, vs, "vs r5 r5 r2", "vs_r5_r5_r2"},
     {{vc, r1, r1, r4}, true, vc, "vc r1 r1 r4", "vc_r1_r1_r4"},
     {{vc, r8, r8, r4}, true, vc, "vc r8 r8 r4", "vc_r8_r8_r4"},
     {{ls, r12, r12, r4}, true, ls, "ls r12 r12 r4", "ls_r12_r12_r4"},
     {{lt, r10, r10, r11}, true, lt, "lt r10 r10 r11", "lt_r10_r10_r11"},
     {{ge, r14, r14, r0}, true, ge, "ge r14 r14 r0", "ge_r14_r14_r0"},
     {{vs, r6, r6, r9}, true, vs, "vs r6 r6 r9", "vs_r6_r6_r9"},
     {{hi, r2, r2, r4}, true, hi, "hi r2 r2 r4", "hi_r2_r2_r4"},
     {{lt, r0, r0, r5}, true, lt, "lt r0 r0 r5", "lt_r0_r0_r5"},
     {{ge, r9, r9, r5}, true, ge, "ge r9 r9 r5", "ge_r9_r9_r5"},
     {{mi, r13, r13, r11}, true, mi, "mi r13 r13 r11", "mi_r13_r13_r11"},
     {{vc, r8, r8, r2}, true, vc, "vc r8 r8 r2", "vc_r8_r8_r2"},
     {{cc, r12, r12, r1}, true, cc, "cc r12 r12 r1", "cc_r12_r12_r1"},
     {{ne, r8, r8, r6}, true, ne, "ne r8 r8 r6", "ne_r8_r8_r6"},
     {{vs, r12, r12, r9}, true, vs, "vs r12 r12 r9", "vs_r12_r12_r9"},
     {{ge, r8, r8, r12}, true, ge, "ge r8 r8 r12", "ge_r8_r8_r12"},
     {{ls, r6, r6, r12}, true, ls, "ls r6 r6 r12", "ls_r6_r6_r12"},
     {{vc, r11, r11, r2}, true, vc, "vc r11 r11 r2", "vc_r11_r11_r2"},
     {{le, r8, r8, r6}, true, le, "le r8 r8 r6", "le_r8_r8_r6"},
     {{ge, r7, r7, r6}, true, ge, "ge r7 r7 r6", "ge_r7_r7_r6"},
     {{pl, r11, r11, r12}, true, pl, "pl r11 r11 r12", "pl_r11_r11_r12"},
     {{ls, r12, r12, r7}, true, ls, "ls r12 r12 r7", "ls_r12_r12_r7"},
     {{ne, r14, r14, r10}, true, ne, "ne r14 r14 r10", "ne_r14_r14_r10"},
     {{gt, r2, r2, r9}, true, gt, "gt r2 r2 r9", "gt_r2_r2_r9"},
     {{ne, r3, r3, r11}, true, ne, "ne r3 r3 r11", "ne_r3_r3_r11"},
     {{mi, r3, r3, r11}, true, mi, "mi r3 r3 r11", "mi_r3_r3_r11"},
     {{ne, r8, r8, r2}, true, ne, "ne r8 r8 r2", "ne_r8_r8_r2"},
     {{le, r12, r12, r8}, true, le, "le r12 r12 r8", "le_r12_r12_r8"},
     {{hi, r0, r0, r14}, true, hi, "hi r0 r0 r14", "hi_r0_r0_r14"},
     {{le, r0, r0, r12}, true, le, "le r0 r0 r12", "le_r0_r0_r12"},
     {{ge, r7, r7, r0}, true, ge, "ge r7 r7 r0", "ge_r7_r7_r0"},
     {{gt, r9, r9, r11}, true, gt, "gt r9 r9 r11", "gt_r9_r9_r11"},
     {{ge, r0, r0, r14}, true, ge, "ge r0 r0 r14", "ge_r0_r0_r14"},
     {{cc, r9, r9, r2}, true, cc, "cc r9 r9 r2", "cc_r9_r9_r2"},
     {{gt, r1, r1, r9}, true, gt, "gt r1 r1 r9", "gt_r1_r1_r9"},
     {{vs, r7, r7, r9}, true, vs, "vs r7 r7 r9", "vs_r7_r7_r9"},
     {{ls, r2, r2, r11}, true, ls, "ls r2 r2 r11", "ls_r2_r2_r11"},
     {{cs, r14, r14, r2}, true, cs, "cs r14 r14 r2", "cs_r14_r14_r2"},
     {{vc, r6, r6, r6}, true, vc, "vc r6 r6 r6", "vc_r6_r6_r6"},
     {{gt, r5, r5, r11}, true, gt, "gt r5 r5 r11", "gt_r5_r5_r11"},
     {{ge, r12, r12, r0}, true, ge, "ge r12 r12 r0", "ge_r12_r12_r0"},
     {{lt, r1, r1, r1}, true, lt, "lt r1 r1 r1", "lt_r1_r1_r1"},
     {{cc, r8, r8, r10}, true, cc, "cc r8 r8 r10", "cc_r8_r8_r10"},
     {{cs, r12, r12, r11}, true, cs, "cs r12 r12 r11", "cs_r12_r12_r11"},
     {{mi, r3, r3, r4}, true, mi, "mi r3 r3 r4", "mi_r3_r3_r4"},
     {{cs, r14, r14, r1}, true, cs, "cs r14 r14 r1", "cs_r14_r14_r1"},
     {{ne, r2, r2, r5}, true, ne, "ne r2 r2 r5", "ne_r2_r2_r5"},
     {{cc, r7, r7, r2}, true, cc, "cc r7 r7 r2", "cc_r7_r7_r2"},
     {{cc, r11, r11, r2}, true, cc, "cc r11 r11 r2", "cc_r11_r11_r2"},
     {{hi, r10, r10, r12}, true, hi, "hi r10 r10 r12", "hi_r10_r10_r12"},
     {{ls, r12, r12, r6}, true, ls, "ls r12 r12 r6", "ls_r12_r12_r6"},
     {{le, r0, r0, r5}, true, le, "le r0 r0 r5", "le_r0_r0_r5"},
     {{ne, r9, r9, r3}, true, ne, "ne r9 r9 r3", "ne_r9_r9_r3"},
     {{le, r12, r12, r6}, true, le, "le r12 r12 r6", "le_r12_r12_r6"},
     {{cc, r8, r8, r12}, true, cc, "cc r8 r8 r12", "cc_r8_r8_r12"},
     {{lt, r10, r10, r2}, true, lt, "lt r10 r10 r2", "lt_r10_r10_r2"},
     {{eq, r9, r9, r11}, true, eq, "eq r9 r9 r11", "eq_r9_r9_r11"},
     {{cc, r10, r10, r5}, true, cc, "cc r10 r10 r5", "cc_r10_r10_r5"},
     {{vs, r10, r10, r11}, true, vs, "vs r10 r10 r11", "vs_r10_r10_r11"},
     {{ne, r6, r6, r14}, true, ne, "ne r6 r6 r14", "ne_r6_r6_r14"},
     {{ne, r2, r2, r0}, true, ne, "ne r2 r2 r0", "ne_r2_r2_r0"},
     {{ge, r11, r11, r14}, true, ge, "ge r11 r11 r14", "ge_r11_r11_r14"},
     {{cc, r4, r4, r14}, true, cc, "cc r4 r4 r14", "cc_r4_r4_r14"},
     {{eq, r7, r7, r8}, true, eq, "eq r7 r7 r8", "eq_r7_r7_r8"},
     {{vc, r14, r14, r6}, true, vc, "vc r14 r14 r6", "vc_r14_r14_r6"},
     {{cc, r12, r12, r12}, true, cc, "cc r12 r12 r12", "cc_r12_r12_r12"},
     {{eq, r4, r4, r4}, true, eq, "eq r4 r4 r4", "eq_r4_r4_r4"},
     {{vc, r9, r9, r2}, true, vc, "vc r9 r9 r2", "vc_r9_r9_r2"},
     {{mi, r13, r13, r2}, true, mi, "mi r13 r13 r2", "mi_r13_r13_r2"},
     {{vc, r9, r9, r10}, true, vc, "vc r9 r9 r10", "vc_r9_r9_r10"},
     {{vc, r4, r4, r0}, true, vc, "vc r4 r4 r0", "vc_r4_r4_r0"},
     {{ge, r2, r2, r5}, true, ge, "ge r2 r2 r5", "ge_r2_r2_r5"},
     {{mi, r10, r10, r7}, true, mi, "mi r10 r10 r7", "mi_r10_r10_r7"},
     {{le, r5, r5, r12}, true, le, "le r5 r5 r12", "le_r5_r5_r12"},
     {{ge, r2, r2, r9}, true, ge, "ge r2 r2 r9", "ge_r2_r2_r9"},
     {{eq, r7, r7, r14}, true, eq, "eq r7 r7 r14", "eq_r7_r7_r14"},
     {{cc, r7, r7, r7}, true, cc, "cc r7 r7 r7", "cc_r7_r7_r7"},
     {{vc, r4, r4, r9}, true, vc, "vc r4 r4 r9", "vc_r4_r4_r9"},
     {{mi, r13, r13, r5}, true, mi, "mi r13 r13 r5", "mi_r13_r13_r5"},
     {{eq, r14, r14, r1}, true, eq, "eq r14 r14 r1", "eq_r14_r14_r1"},
     {{vs, r8, r8, r8}, true, vs, "vs r8 r8 r8", "vs_r8_r8_r8"},
     {{eq, r7, r7, r12}, true, eq, "eq r7 r7 r12", "eq_r7_r7_r12"},
     {{cc, r13, r13, r3}, true, cc, "cc r13 r13 r3", "cc_r13_r13_r3"},
     {{ge, r1, r1, r12}, true, ge, "ge r1 r1 r12", "ge_r1_r1_r12"},
     {{ls, r10, r10, r10}, true, ls, "ls r10 r10 r10", "ls_r10_r10_r10"},
     {{le, r3, r3, r1}, true, le, "le r3 r3 r1", "le_r3_r3_r1"},
     {{vc, r13, r13, r3}, true, vc, "vc r13 r13 r3", "vc_r13_r13_r3"},
     {{le, r13, r13, r6}, true, le, "le r13 r13 r6", "le_r13_r13_r6"},
     {{le, r4, r4, r0}, true, le, "le r4 r4 r0", "le_r4_r4_r0"},
     {{cs, r2, r2, r11}, true, cs, "cs r2 r2 r11", "cs_r2_r2_r11"},
     {{vs, r13, r13, r0}, true, vs, "vs r13 r13 r0", "vs_r13_r13_r0"},
     {{gt, r6, r6, r12}, true, gt, "gt r6 r6 r12", "gt_r6_r6_r12"},
     {{cs, r14, r14, r0}, true, cs, "cs r14 r14 r0", "cs_r14_r14_r0"},
     {{ne, r11, r11, r3}, true, ne, "ne r11 r11 r3", "ne_r11_r11_r3"},
     {{ls, r3, r3, r3}, true, ls, "ls r3 r3 r3", "ls_r3_r3_r3"},
     {{ne, r12, r12, r8}, true, ne, "ne r12 r12 r8", "ne_r12_r12_r8"},
     {{ge, r1, r1, r8}, true, ge, "ge r1 r1 r8", "ge_r1_r1_r8"},
     {{ls, r2, r2, r2}, true, ls, "ls r2 r2 r2", "ls_r2_r2_r2"},
     {{ls, r14, r14, r6}, true, ls, "ls r14 r14 r6", "ls_r14_r14_r6"},
     {{ls, r1, r1, r12}, true, ls, "ls r1 r1 r12", "ls_r1_r1_r12"},
     {{ge, r11, r11, r0}, true, ge, "ge r11 r11 r0", "ge_r11_r11_r0"},
     {{le, r7, r7, r1}, true, le, "le r7 r7 r1", "le_r7_r7_r1"},
     {{mi, r0, r0, r2}, true, mi, "mi r0 r0 r2", "mi_r0_r0_r2"},
     {{cc, r1, r1, r10}, true, cc, "cc r1 r1 r10", "cc_r1_r1_r10"},
     {{hi, r6, r6, r7}, true, hi, "hi r6 r6 r7", "hi_r6_r6_r7"},
     {{vc, r0, r0, r8}, true, vc, "vc r0 r0 r8", "vc_r0_r0_r8"},
     {{hi, r1, r1, r6}, true, hi, "hi r1 r1 r6", "hi_r1_r1_r6"},
     {{pl, r2, r2, r10}, true, pl, "pl r2 r2 r10", "pl_r2_r2_r10"},
     {{vc, r14, r14, r0}, true, vc, "vc r14 r14 r0", "vc_r14_r14_r0"},
     {{hi, r13, r13, r1}, true, hi, "hi r13 r13 r1", "hi_r13_r13_r1"},
     {{le, r14, r14, r8}, true, le, "le r14 r14 r8", "le_r14_r14_r8"},
     {{ne, r2, r2, r2}, true, ne, "ne r2 r2 r2", "ne_r2_r2_r2"},
     {{pl, r4, r4, r0}, true, pl, "pl r4 r4 r0", "pl_r4_r4_r0"},
     {{gt, r10, r10, r10}, true, gt, "gt r10 r10 r10", "gt_r10_r10_r10"},
     {{eq, r9, r9, r6}, true, eq, "eq r9 r9 r6", "eq_r9_r9_r6"},
     {{eq, r9, r9, r10}, true, eq, "eq r9 r9 r10", "eq_r9_r9_r10"},
     {{eq, r5, r5, r1}, true, eq, "eq r5 r5 r1", "eq_r5_r5_r1"},
     {{pl, r6, r6, r10}, true, pl, "pl r6 r6 r10", "pl_r6_r6_r10"},
     {{vc, r6, r6, r12}, true, vc, "vc r6 r6 r12", "vc_r6_r6_r12"},
     {{vs, r12, r12, r8}, true, vs, "vs r12 r12 r8", "vs_r12_r12_r8"},
     {{cc, r10, r10, r3}, true, cc, "cc r10 r10 r3", "cc_r10_r10_r3"},
     {{le, r9, r9, r3}, true, le, "le r9 r9 r3", "le_r9_r9_r3"},
     {{mi, r1, r1, r4}, true, mi, "mi r1 r1 r4", "mi_r1_r1_r4"},
     {{ls, r3, r3, r10}, true, ls, "ls r3 r3 r10", "ls_r3_r3_r10"},
     {{ge, r8, r8, r11}, true, ge, "ge r8 r8 r11", "ge_r8_r8_r11"},
     {{eq, r11, r11, r3}, true, eq, "eq r11 r11 r3", "eq_r11_r11_r3"},
     {{gt, r0, r0, r11}, true, gt, "gt r0 r0 r11", "gt_r0_r0_r11"},
     {{cs, r3, r3, r8}, true, cs, "cs r3 r3 r8", "cs_r3_r3_r8"},
     {{eq, r8, r8, r4}, true, eq, "eq r8 r8 r4", "eq_r8_r8_r4"},
     {{vs, r8, r8, r12}, true, vs, "vs r8 r8 r12", "vs_r8_r8_r12"},
     {{vc, r3, r3, r7}, true, vc, "vc r3 r3 r7", "vc_r3_r3_r7"},
     {{mi, r8, r8, r14}, true, mi, "mi r8 r8 r14", "mi_r8_r8_r14"},
     {{cs, r0, r0, r12}, true, cs, "cs r0 r0 r12", "cs_r0_r0_r12"},
     {{cs, r12, r12, r12}, true, cs, "cs r12 r12 r12", "cs_r12_r12_r12"},
     {{le, r3, r3, r6}, true, le, "le r3 r3 r6", "le_r3_r3_r6"},
     {{vc, r10, r10, r12}, true, vc, "vc r10 r10 r12", "vc_r10_r10_r12"},
     {{ne, r12, r12, r5}, true, ne, "ne r12 r12 r5", "ne_r12_r12_r5"},
     {{vc, r1, r1, r9}, true, vc, "vc r1 r1 r9", "vc_r1_r1_r9"},
     {{pl, r2, r2, r11}, true, pl, "pl r2 r2 r11", "pl_r2_r2_r11"},
     {{eq, r4, r4, r2}, true, eq, "eq r4 r4 r2", "eq_r4_r4_r2"},
     {{vc, r0, r0, r9}, true, vc, "vc r0 r0 r9", "vc_r0_r0_r9"},
     {{gt, r10, r10, r14}, true, gt, "gt r10 r10 r14", "gt_r10_r10_r14"},
     {{vc, r6, r6, r0}, true, vc, "vc r6 r6 r0", "vc_r6_r6_r0"},
     {{le, r10, r10, r3}, true, le, "le r10 r10 r3", "le_r10_r10_r3"},
     {{mi, r1, r1, r7}, true, mi, "mi r1 r1 r7", "mi_r1_r1_r7"},
     {{le, r5, r5, r4}, true, le, "le r5 r5 r4", "le_r5_r5_r4"},
     {{mi, r4, r4, r0}, true, mi, "mi r4 r4 r0", "mi_r4_r4_r0"},
     {{ge, r3, r3, r10}, true, ge, "ge r3 r3 r10", "ge_r3_r3_r10"},
     {{vc, r4, r4, r10}, true, vc, "vc r4 r4 r10", "vc_r4_r4_r10"},
     {{ge, r4, r4, r2}, true, ge, "ge r4 r4 r2", "ge_r4_r4_r2"},
     {{eq, r6, r6, r8}, true, eq, "eq r6 r6 r8", "eq_r6_r6_r8"},
     {{lt, r6, r6, r12}, true, lt, "lt r6 r6 r12", "lt_r6_r6_r12"},
     {{cc, r1, r1, r1}, true, cc, "cc r1 r1 r1", "cc_r1_r1_r1"},
     {{pl, r6, r6, r14}, true, pl, "pl r6 r6 r14", "pl_r6_r6_r14"},
     {{cc, r11, r11, r6}, true, cc, "cc r11 r11 r6", "cc_r11_r11_r6"},
     {{pl, r0, r0, r6}, true, pl, "pl r0 r0 r6", "pl_r0_r0_r6"},
     {{eq, r4, r4, r11}, true, eq, "eq r4 r4 r11", "eq_r4_r4_r11"},
     {{pl, r4, r4, r4}, true, pl, "pl r4 r4 r4", "pl_r4_r4_r4"},
     {{pl, r1, r1, r14}, true, pl, "pl r1 r1 r14", "pl_r1_r1_r14"},
     {{ne, r3, r3, r7}, true, ne, "ne r3 r3 r7", "ne_r3_r3_r7"},
     {{cs, r3, r3, r11}, true, cs, "cs r3 r3 r11", "cs_r3_r3_r11"},
     {{eq, r1, r1, r14}, true, eq, "eq r1 r1 r14", "eq_r1_r1_r14"},
     {{lt, r4, r4, r11}, true, lt, "lt r4 r4 r11", "lt_r4_r4_r11"},
     {{lt, r2, r2, r9}, true, lt, "lt r2 r2 r9", "lt_r2_r2_r9"},
     {{cs, r10, r10, r8}, true, cs, "cs r10 r10 r8", "cs_r10_r10_r8"},
     {{ls, r2, r2, r1}, true, ls, "ls r2 r2 r1", "ls_r2_r2_r1"},
     {{le, r8, r8, r1}, true, le, "le r8 r8 r1", "le_r8_r8_r1"},
     {{cc, r4, r4, r3}, true, cc, "cc r4 r4 r3", "cc_r4_r4_r3"},
     {{cc, r9, r9, r3}, true, cc, "cc r9 r9 r3", "cc_r9_r9_r3"},
     {{ge, r6, r6, r0}, true, ge, "ge r6 r6 r0", "ge_r6_r6_r0"},
     {{pl, r6, r6, r2}, true, pl, "pl r6 r6 r2", "pl_r6_r6_r2"},
     {{cs, r0, r0, r7}, true, cs, "cs r0 r0 r7", "cs_r0_r0_r7"},
     {{pl, r7, r7, r8}, true, pl, "pl r7 r7 r8", "pl_r7_r7_r8"},
     {{vs, r14, r14, r4}, true, vs, "vs r14 r14 r4", "vs_r14_r14_r4"},
     {{gt, r4, r4, r8}, true, gt, "gt r4 r4 r8", "gt_r4_r4_r8"},
     {{eq, r0, r0, r4}, true, eq, "eq r0 r0 r4", "eq_r0_r0_r4"},
     {{cc, r8, r8, r6}, true, cc, "cc r8 r8 r6", "cc_r8_r8_r6"},
     {{vc, r11, r11, r11}, true, vc, "vc r11 r11 r11", "vc_r11_r11_r11"},
     {{vc, r14, r14, r5}, true, vc, "vc r14 r14 r5", "vc_r14_r14_r5"},
     {{gt, r7, r7, r11}, true, gt, "gt r7 r7 r11", "gt_r7_r7_r11"},
     {{cs, r1, r1, r4}, true, cs, "cs r1 r1 r4", "cs_r1_r1_r4"},
     {{lt, r10, r10, r4}, true, lt, "lt r10 r10 r4", "lt_r10_r10_r4"},
     {{ne, r11, r11, r11}, true, ne, "ne r11 r11 r11", "ne_r11_r11_r11"},
     {{le, r8, r8, r2}, true, le, "le r8 r8 r2", "le_r8_r8_r2"},
     {{le, r7, r7, r7}, true, le, "le r7 r7 r7", "le_r7_r7_r7"},
     {{vc, r6, r6, r2}, true, vc, "vc r6 r6 r2", "vc_r6_r6_r2"},
     {{eq, r14, r14, r9}, true, eq, "eq r14 r14 r9", "eq_r14_r14_r9"},
     {{cs, r10, r10, r12}, true, cs, "cs r10 r10 r12", "cs_r10_r10_r12"},
     {{lt, r0, r0, r8}, true, lt, "lt r0 r0 r8", "lt_r0_r0_r8"},
     {{ne, r0, r0, r10}, true, ne, "ne r0 r0 r10", "ne_r0_r0_r10"},
     {{cs, r0, r0, r1}, true, cs, "cs r0 r0 r1", "cs_r0_r0_r1"},
     {{gt, r1, r1, r1}, true, gt, "gt r1 r1 r1", "gt_r1_r1_r1"},
     {{hi, r11, r11, r2}, true, hi, "hi r11 r11 r2", "hi_r11_r11_r2"},
     {{mi, r11, r11, r7}, true, mi, "mi r11 r11 r7", "mi_r11_r11_r7"},
     {{mi, r14, r14, r1}, true, mi, "mi r14 r14 r1", "mi_r14_r14_r1"},
     {{lt, r11, r11, r6}, true, lt, "lt r11 r11 r6", "lt_r11_r11_r6"},
     {{le, r9, r9, r14}, true, le, "le r9 r9 r14", "le_r9_r9_r14"},
     {{eq, r11, r11, r5}, true, eq, "eq r11 r11 r5", "eq_r11_r11_r5"},
     {{hi, r5, r5, r4}, true, hi, "hi r5 r5 r4", "hi_r5_r5_r4"},
     {{gt, r6, r6, r11}, true, gt, "gt r6 r6 r11", "gt_r6_r6_r11"},
     {{cs, r14, r14, r8}, true, cs, "cs r14 r14 r8", "cs_r14_r14_r8"},
     {{ls, r14, r14, r8}, true, ls, "ls r14 r14 r8", "ls_r14_r14_r8"},
     {{cc, r10, r10, r14}, true, cc, "cc r10 r10 r14", "cc_r10_r10_r14"},
     {{hi, r13, r13, r14}, true, hi, "hi r13 r13 r14", "hi_r13_r13_r14"},
     {{mi, r4, r4, r7}, true, mi, "mi r4 r4 r7", "mi_r4_r4_r7"},
     {{mi, r4, r4, r14}, true, mi, "mi r4 r4 r14", "mi_r4_r4_r14"},
     {{eq, r0, r0, r6}, true, eq, "eq r0 r0 r6", "eq_r0_r0_r6"},
     {{cs, r8, r8, r11}, true, cs, "cs r8 r8 r11", "cs_r8_r8_r11"},
     {{vc, r0, r0, r10}, true, vc, "vc r0 r0 r10", "vc_r0_r0_r10"},
     {{cc, r0, r0, r14}, true, cc, "cc r0 r0 r14", "cc_r0_r0_r14"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-rn-operand-rm-rd-is-rn-in-it-block-add-t32.h"


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
    Register rm = kTests[i].operands.rm;
    Operand op(rm);

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
#define TEST(mnemonic)                                                      \
  void Test_##mnemonic() {                                                  \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic); \
  }                                                                         \
  Test test_##mnemonic(                                                     \
      "AARCH32_ASSEMBLER_COND_RD_RN_OPERAND_RM_RD_IS_RN_IN_IT_"             \
      "BLOCK_" #mnemonic "_T32",                                            \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
