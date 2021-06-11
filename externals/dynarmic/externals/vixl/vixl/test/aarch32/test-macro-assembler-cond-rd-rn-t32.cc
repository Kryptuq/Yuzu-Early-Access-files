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
// test/aarch32/config/template-macro-assembler-aarch32.cc.in template file
// using tools/generate_tests.py.
//
// PLEASE DO NOT EDIT.
// -----------------------------------------------------------------------------


#include <iostream>
#include <map>

#include "test-runner.h"

#include "test-utils.h"
#include "test-utils-aarch32.h"

#include "aarch32/assembler-aarch32.h"
#include "aarch32/disasm-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#define BUF_SIZE (4096)

namespace vixl {
namespace aarch32 {

// List of instruction mnemonics.
#define FOREACH_INSTRUCTION(M) \
  M(Clz)                       \
  M(Rbit)                      \
  M(Rev)                       \
  M(Rev16)                     \
  M(Revsh)                     \
  M(Rrx)                       \
  M(Rrxs)


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
};

// This structure contains all data needed to test one specific
// instruction.
struct TestData {
  // The `operands` field represents what to pass to the assembler to
  // produce the instruction.
  Operands operands;
  // Description of the operands, used for error reporting.
  const char* operands_description;
  // Unique identifier, used for generating traces.
  const char* identifier;
};

// Each element of this array produce one instruction encoding.
const TestData kTests[] = {{{cs, r12, r1}, "cs, r12, r1", "cs_r12_r1"},
                           {{hi, r6, r12}, "hi, r6, r12", "hi_r6_r12"},
                           {{lt, r11, r10}, "lt, r11, r10", "lt_r11_r10"},
                           {{vs, r10, r8}, "vs, r10, r8", "vs_r10_r8"},
                           {{pl, r5, r8}, "pl, r5, r8", "pl_r5_r8"},
                           {{ls, r14, r14}, "ls, r14, r14", "ls_r14_r14"},
                           {{gt, r8, r6}, "gt, r8, r6", "gt_r8_r6"},
                           {{vc, r7, r11}, "vc, r7, r11", "vc_r7_r11"},
                           {{pl, r4, r12}, "pl, r4, r12", "pl_r4_r12"},
                           {{gt, r12, r12}, "gt, r12, r12", "gt_r12_r12"},
                           {{ne, r8, r4}, "ne, r8, r4", "ne_r8_r4"},
                           {{le, r1, r11}, "le, r1, r11", "le_r1_r11"},
                           {{cc, r4, r10}, "cc, r4, r10", "cc_r4_r10"},
                           {{gt, r14, r0}, "gt, r14, r0", "gt_r14_r0"},
                           {{cs, r2, r11}, "cs, r2, r11", "cs_r2_r11"},
                           {{al, r3, r12}, "al, r3, r12", "al_r3_r12"},
                           {{hi, r6, r0}, "hi, r6, r0", "hi_r6_r0"},
                           {{ls, r10, r10}, "ls, r10, r10", "ls_r10_r10"},
                           {{ls, r4, r8}, "ls, r4, r8", "ls_r4_r8"},
                           {{le, r8, r0}, "le, r8, r0", "le_r8_r0"},
                           {{pl, r5, r3}, "pl, r5, r3", "pl_r5_r3"},
                           {{ls, r8, r5}, "ls, r8, r5", "ls_r8_r5"},
                           {{ge, r0, r10}, "ge, r0, r10", "ge_r0_r10"},
                           {{lt, r10, r6}, "lt, r10, r6", "lt_r10_r6"},
                           {{ne, r9, r9}, "ne, r9, r9", "ne_r9_r9"},
                           {{hi, r5, r0}, "hi, r5, r0", "hi_r5_r0"},
                           {{pl, r10, r6}, "pl, r10, r6", "pl_r10_r6"},
                           {{vs, r1, r3}, "vs, r1, r3", "vs_r1_r3"},
                           {{vs, r9, r8}, "vs, r9, r8", "vs_r9_r8"},
                           {{cc, r2, r10}, "cc, r2, r10", "cc_r2_r10"},
                           {{cs, r11, r3}, "cs, r11, r3", "cs_r11_r3"},
                           {{hi, r8, r2}, "hi, r8, r2", "hi_r8_r2"},
                           {{pl, r6, r0}, "pl, r6, r0", "pl_r6_r0"},
                           {{hi, r9, r2}, "hi, r9, r2", "hi_r9_r2"},
                           {{al, r14, r11}, "al, r14, r11", "al_r14_r11"},
                           {{eq, r8, r13}, "eq, r8, r13", "eq_r8_r13"},
                           {{ge, r12, r2}, "ge, r12, r2", "ge_r12_r2"},
                           {{cc, r2, r6}, "cc, r2, r6", "cc_r2_r6"},
                           {{ne, r13, r9}, "ne, r13, r9", "ne_r13_r9"},
                           {{ne, r13, r10}, "ne, r13, r10", "ne_r13_r10"},
                           {{mi, r10, r9}, "mi, r10, r9", "mi_r10_r9"},
                           {{lt, r13, r12}, "lt, r13, r12", "lt_r13_r12"},
                           {{lt, r14, r3}, "lt, r14, r3", "lt_r14_r3"},
                           {{mi, r12, r14}, "mi, r12, r14", "mi_r12_r14"},
                           {{vs, r11, r2}, "vs, r11, r2", "vs_r11_r2"},
                           {{ls, r11, r0}, "ls, r11, r0", "ls_r11_r0"},
                           {{gt, r10, r9}, "gt, r10, r9", "gt_r10_r9"},
                           {{le, r10, r12}, "le, r10, r12", "le_r10_r12"},
                           {{ge, r0, r14}, "ge, r0, r14", "ge_r0_r14"},
                           {{mi, r5, r8}, "mi, r5, r8", "mi_r5_r8"},
                           {{vs, r4, r11}, "vs, r4, r11", "vs_r4_r11"},
                           {{lt, r7, r12}, "lt, r7, r12", "lt_r7_r12"},
                           {{gt, r2, r8}, "gt, r2, r8", "gt_r2_r8"},
                           {{gt, r5, r7}, "gt, r5, r7", "gt_r5_r7"},
                           {{pl, r6, r3}, "pl, r6, r3", "pl_r6_r3"},
                           {{ne, r2, r12}, "ne, r2, r12", "ne_r2_r12"},
                           {{hi, r11, r11}, "hi, r11, r11", "hi_r11_r11"},
                           {{vs, r4, r14}, "vs, r4, r14", "vs_r4_r14"},
                           {{vs, r13, r1}, "vs, r13, r1", "vs_r13_r1"},
                           {{cs, r4, r3}, "cs, r4, r3", "cs_r4_r3"},
                           {{eq, r13, r6}, "eq, r13, r6", "eq_r13_r6"},
                           {{gt, r11, r14}, "gt, r11, r14", "gt_r11_r14"},
                           {{vc, r14, r4}, "vc, r14, r4", "vc_r14_r4"},
                           {{vc, r1, r10}, "vc, r1, r10", "vc_r1_r10"},
                           {{mi, r10, r3}, "mi, r10, r3", "mi_r10_r3"},
                           {{vs, r9, r3}, "vs, r9, r3", "vs_r9_r3"},
                           {{vc, r14, r5}, "vc, r14, r5", "vc_r14_r5"},
                           {{ne, r9, r1}, "ne, r9, r1", "ne_r9_r1"},
                           {{al, r9, r6}, "al, r9, r6", "al_r9_r6"},
                           {{vc, r7, r8}, "vc, r7, r8", "vc_r7_r8"},
                           {{ls, r6, r9}, "ls, r6, r9", "ls_r6_r9"},
                           {{le, r14, r2}, "le, r14, r2", "le_r14_r2"},
                           {{ls, r9, r13}, "ls, r9, r13", "ls_r9_r13"},
                           {{eq, r1, r5}, "eq, r1, r5", "eq_r1_r5"},
                           {{le, r9, r2}, "le, r9, r2", "le_r9_r2"},
                           {{eq, r2, r1}, "eq, r2, r1", "eq_r2_r1"},
                           {{ls, r9, r11}, "ls, r9, r11", "ls_r9_r11"},
                           {{lt, r7, r13}, "lt, r7, r13", "lt_r7_r13"},
                           {{cc, r8, r7}, "cc, r8, r7", "cc_r8_r7"},
                           {{ls, r5, r1}, "ls, r5, r1", "ls_r5_r1"},
                           {{le, r1, r9}, "le, r1, r9", "le_r1_r9"},
                           {{ls, r8, r14}, "ls, r8, r14", "ls_r8_r14"},
                           {{le, r9, r1}, "le, r9, r1", "le_r9_r1"},
                           {{gt, r1, r6}, "gt, r1, r6", "gt_r1_r6"},
                           {{gt, r11, r10}, "gt, r11, r10", "gt_r11_r10"},
                           {{hi, r0, r12}, "hi, r0, r12", "hi_r0_r12"},
                           {{gt, r1, r7}, "gt, r1, r7", "gt_r1_r7"},
                           {{mi, r7, r1}, "mi, r7, r1", "mi_r7_r1"},
                           {{mi, r7, r0}, "mi, r7, r0", "mi_r7_r0"},
                           {{ls, r1, r3}, "ls, r1, r3", "ls_r1_r3"},
                           {{mi, r13, r3}, "mi, r13, r3", "mi_r13_r3"},
                           {{eq, r4, r3}, "eq, r4, r3", "eq_r4_r3"},
                           {{vc, r14, r9}, "vc, r14, r9", "vc_r14_r9"},
                           {{pl, r3, r1}, "pl, r3, r1", "pl_r3_r1"},
                           {{mi, r0, r2}, "mi, r0, r2", "mi_r0_r2"},
                           {{hi, r2, r7}, "hi, r2, r7", "hi_r2_r7"},
                           {{hi, r13, r12}, "hi, r13, r12", "hi_r13_r12"},
                           {{ls, r12, r8}, "ls, r12, r8", "ls_r12_r8"},
                           {{ne, r5, r6}, "ne, r5, r6", "ne_r5_r6"},
                           {{pl, r5, r4}, "pl, r5, r4", "pl_r5_r4"},
                           {{ge, r1, r6}, "ge, r1, r6", "ge_r1_r6"},
                           {{eq, r8, r10}, "eq, r8, r10", "eq_r8_r10"},
                           {{cs, r5, r4}, "cs, r5, r4", "cs_r5_r4"},
                           {{eq, r11, r6}, "eq, r11, r6", "eq_r11_r6"},
                           {{cs, r13, r3}, "cs, r13, r3", "cs_r13_r3"},
                           {{pl, r2, r8}, "pl, r2, r8", "pl_r2_r8"},
                           {{gt, r10, r0}, "gt, r10, r0", "gt_r10_r0"},
                           {{lt, r4, r10}, "lt, r4, r10", "lt_r4_r10"},
                           {{vs, r14, r14}, "vs, r14, r14", "vs_r14_r14"},
                           {{hi, r0, r7}, "hi, r0, r7", "hi_r0_r7"},
                           {{cc, r9, r6}, "cc, r9, r6", "cc_r9_r6"},
                           {{ne, r7, r13}, "ne, r7, r13", "ne_r7_r13"},
                           {{eq, r2, r6}, "eq, r2, r6", "eq_r2_r6"},
                           {{cs, r12, r12}, "cs, r12, r12", "cs_r12_r12"},
                           {{mi, r3, r10}, "mi, r3, r10", "mi_r3_r10"},
                           {{le, r6, r13}, "le, r6, r13", "le_r6_r13"},
                           {{al, r2, r1}, "al, r2, r1", "al_r2_r1"},
                           {{le, r3, r4}, "le, r3, r4", "le_r3_r4"},
                           {{pl, r4, r4}, "pl, r4, r4", "pl_r4_r4"},
                           {{lt, r12, r3}, "lt, r12, r3", "lt_r12_r3"},
                           {{ls, r7, r4}, "ls, r7, r4", "ls_r7_r4"},
                           {{le, r9, r4}, "le, r9, r4", "le_r9_r4"},
                           {{vc, r13, r3}, "vc, r13, r3", "vc_r13_r3"},
                           {{vc, r8, r8}, "vc, r8, r8", "vc_r8_r8"},
                           {{ge, r2, r8}, "ge, r2, r8", "ge_r2_r8"},
                           {{le, r11, r13}, "le, r11, r13", "le_r11_r13"},
                           {{le, r1, r5}, "le, r1, r5", "le_r1_r5"},
                           {{ge, r9, r12}, "ge, r9, r12", "ge_r9_r12"},
                           {{cs, r14, r14}, "cs, r14, r14", "cs_r14_r14"},
                           {{cc, r1, r0}, "cc, r1, r0", "cc_r1_r0"},
                           {{ge, r3, r0}, "ge, r3, r0", "ge_r3_r0"},
                           {{al, r10, r0}, "al, r10, r0", "al_r10_r0"},
                           {{cc, r11, r10}, "cc, r11, r10", "cc_r11_r10"},
                           {{mi, r11, r11}, "mi, r11, r11", "mi_r11_r11"},
                           {{ne, r12, r7}, "ne, r12, r7", "ne_r12_r7"},
                           {{lt, r4, r12}, "lt, r4, r12", "lt_r4_r12"},
                           {{gt, r14, r5}, "gt, r14, r5", "gt_r14_r5"},
                           {{ge, r1, r7}, "ge, r1, r7", "ge_r1_r7"},
                           {{al, r7, r0}, "al, r7, r0", "al_r7_r0"},
                           {{ls, r2, r0}, "ls, r2, r0", "ls_r2_r0"},
                           {{pl, r6, r11}, "pl, r6, r11", "pl_r6_r11"},
                           {{vc, r0, r6}, "vc, r0, r6", "vc_r0_r6"},
                           {{pl, r6, r9}, "pl, r6, r9", "pl_r6_r9"},
                           {{vs, r8, r1}, "vs, r8, r1", "vs_r8_r1"},
                           {{cs, r1, r6}, "cs, r1, r6", "cs_r1_r6"},
                           {{hi, r4, r11}, "hi, r4, r11", "hi_r4_r11"},
                           {{gt, r5, r5}, "gt, r5, r5", "gt_r5_r5"},
                           {{al, r3, r4}, "al, r3, r4", "al_r3_r4"},
                           {{ge, r11, r12}, "ge, r11, r12", "ge_r11_r12"},
                           {{cc, r14, r2}, "cc, r14, r2", "cc_r14_r2"},
                           {{ge, r2, r4}, "ge, r2, r4", "ge_r2_r4"},
                           {{lt, r5, r11}, "lt, r5, r11", "lt_r5_r11"},
                           {{mi, r11, r14}, "mi, r11, r14", "mi_r11_r14"},
                           {{ne, r3, r3}, "ne, r3, r3", "ne_r3_r3"},
                           {{pl, r14, r0}, "pl, r14, r0", "pl_r14_r0"},
                           {{vc, r4, r10}, "vc, r4, r10", "vc_r4_r10"},
                           {{vs, r6, r8}, "vs, r6, r8", "vs_r6_r8"},
                           {{vc, r4, r7}, "vc, r4, r7", "vc_r4_r7"},
                           {{cs, r11, r10}, "cs, r11, r10", "cs_r11_r10"},
                           {{eq, r11, r8}, "eq, r11, r8", "eq_r11_r8"},
                           {{ne, r9, r2}, "ne, r9, r2", "ne_r9_r2"},
                           {{hi, r9, r7}, "hi, r9, r7", "hi_r9_r7"},
                           {{lt, r0, r6}, "lt, r0, r6", "lt_r0_r6"},
                           {{vc, r5, r8}, "vc, r5, r8", "vc_r5_r8"},
                           {{gt, r3, r1}, "gt, r3, r1", "gt_r3_r1"},
                           {{pl, r3, r5}, "pl, r3, r5", "pl_r3_r5"},
                           {{al, r11, r5}, "al, r11, r5", "al_r11_r5"},
                           {{hi, r8, r14}, "hi, r8, r14", "hi_r8_r14"},
                           {{le, r5, r4}, "le, r5, r4", "le_r5_r4"},
                           {{ge, r13, r3}, "ge, r13, r3", "ge_r13_r3"},
                           {{pl, r7, r14}, "pl, r7, r14", "pl_r7_r14"},
                           {{cs, r6, r9}, "cs, r6, r9", "cs_r6_r9"},
                           {{lt, r8, r12}, "lt, r8, r12", "lt_r8_r12"},
                           {{cc, r12, r0}, "cc, r12, r0", "cc_r12_r0"},
                           {{mi, r14, r13}, "mi, r14, r13", "mi_r14_r13"},
                           {{pl, r12, r13}, "pl, r12, r13", "pl_r12_r13"},
                           {{al, r4, r8}, "al, r4, r8", "al_r4_r8"},
                           {{ls, r5, r11}, "ls, r5, r11", "ls_r5_r11"},
                           {{ge, r11, r3}, "ge, r11, r3", "ge_r11_r3"},
                           {{le, r1, r0}, "le, r1, r0", "le_r1_r0"},
                           {{gt, r14, r4}, "gt, r14, r4", "gt_r14_r4"},
                           {{ne, r3, r14}, "ne, r3, r14", "ne_r3_r14"},
                           {{cc, r3, r8}, "cc, r3, r8", "cc_r3_r8"},
                           {{eq, r7, r10}, "eq, r7, r10", "eq_r7_r10"},
                           {{pl, r0, r10}, "pl, r0, r10", "pl_r0_r10"},
                           {{mi, r6, r13}, "mi, r6, r13", "mi_r6_r13"},
                           {{gt, r10, r5}, "gt, r10, r5", "gt_r10_r5"},
                           {{ne, r0, r1}, "ne, r0, r1", "ne_r0_r1"},
                           {{ge, r4, r8}, "ge, r4, r8", "ge_r4_r8"},
                           {{cs, r3, r7}, "cs, r3, r7", "cs_r3_r7"},
                           {{pl, r2, r12}, "pl, r2, r12", "pl_r2_r12"},
                           {{ls, r9, r12}, "ls, r9, r12", "ls_r9_r12"},
                           {{ge, r4, r9}, "ge, r4, r9", "ge_r4_r9"},
                           {{lt, r7, r2}, "lt, r7, r2", "lt_r7_r2"},
                           {{hi, r10, r10}, "hi, r10, r10", "hi_r10_r10"},
                           {{ls, r1, r7}, "ls, r1, r7", "ls_r1_r7"},
                           {{vs, r1, r12}, "vs, r1, r12", "vs_r1_r12"},
                           {{ge, r9, r14}, "ge, r9, r14", "ge_r9_r14"},
                           {{pl, r6, r4}, "pl, r6, r4", "pl_r6_r4"},
                           {{ls, r3, r2}, "ls, r3, r2", "ls_r3_r2"},
                           {{cs, r4, r4}, "cs, r4, r4", "cs_r4_r4"},
                           {{eq, r6, r2}, "eq, r6, r2", "eq_r6_r2"},
                           {{ge, r14, r0}, "ge, r14, r0", "ge_r14_r0"},
                           {{le, r11, r6}, "le, r11, r6", "le_r11_r6"},
                           {{vs, r0, r0}, "vs, r0, r0", "vs_r0_r0"},
                           {{vs, r4, r6}, "vs, r4, r6", "vs_r4_r6"},
                           {{gt, r6, r10}, "gt, r6, r10", "gt_r6_r10"},
                           {{vc, r12, r7}, "vc, r12, r7", "vc_r12_r7"},
                           {{gt, r8, r3}, "gt, r8, r3", "gt_r8_r3"},
                           {{hi, r14, r4}, "hi, r14, r4", "hi_r14_r4"},
                           {{hi, r9, r14}, "hi, r9, r14", "hi_r9_r14"},
                           {{vs, r6, r1}, "vs, r6, r1", "vs_r6_r1"},
                           {{hi, r5, r4}, "hi, r5, r4", "hi_r5_r4"},
                           {{lt, r10, r14}, "lt, r10, r14", "lt_r10_r14"},
                           {{cc, r8, r10}, "cc, r8, r10", "cc_r8_r10"},
                           {{lt, r11, r0}, "lt, r11, r0", "lt_r11_r0"},
                           {{ge, r4, r12}, "ge, r4, r12", "ge_r4_r12"},
                           {{cc, r5, r7}, "cc, r5, r7", "cc_r5_r7"},
                           {{gt, r3, r9}, "gt, r3, r9", "gt_r3_r9"},
                           {{vc, r11, r5}, "vc, r11, r5", "vc_r11_r5"},
                           {{lt, r7, r1}, "lt, r7, r1", "lt_r7_r1"},
                           {{lt, r1, r8}, "lt, r1, r8", "lt_r1_r8"},
                           {{hi, r11, r13}, "hi, r11, r13", "hi_r11_r13"},
                           {{vs, r10, r2}, "vs, r10, r2", "vs_r10_r2"},
                           {{ne, r1, r10}, "ne, r1, r10", "ne_r1_r10"},
                           {{vc, r10, r0}, "vc, r10, r0", "vc_r10_r0"},
                           {{al, r2, r2}, "al, r2, r2", "al_r2_r2"},
                           {{ne, r7, r8}, "ne, r7, r8", "ne_r7_r8"},
                           {{hi, r11, r14}, "hi, r11, r14", "hi_r11_r14"},
                           {{ne, r6, r14}, "ne, r6, r14", "ne_r6_r14"},
                           {{ge, r8, r5}, "ge, r8, r5", "ge_r8_r5"},
                           {{vs, r7, r3}, "vs, r7, r3", "vs_r7_r3"},
                           {{ne, r14, r13}, "ne, r14, r13", "ne_r14_r13"},
                           {{hi, r12, r11}, "hi, r12, r11", "hi_r12_r11"},
                           {{ls, r11, r9}, "ls, r11, r9", "ls_r11_r9"},
                           {{mi, r12, r7}, "mi, r12, r7", "mi_r12_r7"},
                           {{cc, r8, r8}, "cc, r8, r8", "cc_r8_r8"},
                           {{ls, r14, r12}, "ls, r14, r12", "ls_r14_r12"},
                           {{ls, r3, r6}, "ls, r3, r6", "ls_r3_r6"},
                           {{hi, r7, r8}, "hi, r7, r8", "hi_r7_r8"},
                           {{vs, r4, r10}, "vs, r4, r10", "vs_r4_r10"},
                           {{ne, r12, r1}, "ne, r12, r1", "ne_r12_r1"},
                           {{vs, r12, r4}, "vs, r12, r4", "vs_r12_r4"},
                           {{hi, r5, r5}, "hi, r5, r5", "hi_r5_r5"},
                           {{gt, r8, r8}, "gt, r8, r8", "gt_r8_r8"},
                           {{lt, r10, r10}, "lt, r10, r10", "lt_r10_r10"},
                           {{vs, r13, r4}, "vs, r13, r4", "vs_r13_r4"},
                           {{mi, r8, r3}, "mi, r8, r3", "mi_r8_r3"},
                           {{eq, r6, r13}, "eq, r6, r13", "eq_r6_r13"},
                           {{al, r2, r0}, "al, r2, r0", "al_r2_r0"},
                           {{ls, r6, r12}, "ls, r6, r12", "ls_r6_r12"},
                           {{vs, r1, r4}, "vs, r1, r4", "vs_r1_r4"},
                           {{vc, r11, r1}, "vc, r11, r1", "vc_r11_r1"},
                           {{ne, r9, r10}, "ne, r9, r10", "ne_r9_r10"},
                           {{ne, r3, r10}, "ne, r3, r10", "ne_r3_r10"},
                           {{hi, r7, r10}, "hi, r7, r10", "hi_r7_r10"},
                           {{pl, r4, r10}, "pl, r4, r10", "pl_r4_r10"},
                           {{le, r7, r1}, "le, r7, r1", "le_r7_r1"},
                           {{gt, r13, r10}, "gt, r13, r10", "gt_r13_r10"},
                           {{lt, r4, r14}, "lt, r4, r14", "lt_r4_r14"},
                           {{al, r10, r1}, "al, r10, r1", "al_r10_r1"},
                           {{mi, r6, r12}, "mi, r6, r12", "mi_r6_r12"},
                           {{eq, r13, r9}, "eq, r13, r9", "eq_r13_r9"},
                           {{ge, r11, r4}, "ge, r11, r4", "ge_r11_r4"},
                           {{hi, r1, r12}, "hi, r1, r12", "hi_r1_r12"},
                           {{ge, r0, r1}, "ge, r0, r1", "ge_r0_r1"},
                           {{lt, r2, r3}, "lt, r2, r3", "lt_r2_r3"},
                           {{al, r12, r9}, "al, r12, r9", "al_r12_r9"},
                           {{hi, r2, r10}, "hi, r2, r10", "hi_r2_r10"},
                           {{mi, r14, r11}, "mi, r14, r11", "mi_r14_r11"},
                           {{pl, r3, r10}, "pl, r3, r10", "pl_r3_r10"},
                           {{vs, r1, r6}, "vs, r1, r6", "vs_r1_r6"},
                           {{al, r7, r6}, "al, r7, r6", "al_r7_r6"},
                           {{ge, r6, r8}, "ge, r6, r8", "ge_r6_r8"},
                           {{eq, r3, r3}, "eq, r3, r3", "eq_r3_r3"},
                           {{ne, r14, r4}, "ne, r14, r4", "ne_r14_r4"},
                           {{vc, r13, r10}, "vc, r13, r10", "vc_r13_r10"},
                           {{mi, r3, r12}, "mi, r3, r12", "mi_r3_r12"},
                           {{pl, r9, r1}, "pl, r9, r1", "pl_r9_r1"},
                           {{hi, r14, r14}, "hi, r14, r14", "hi_r14_r14"},
                           {{ne, r0, r9}, "ne, r0, r9", "ne_r0_r9"},
                           {{mi, r1, r1}, "mi, r1, r1", "mi_r1_r1"},
                           {{hi, r7, r2}, "hi, r7, r2", "hi_r7_r2"},
                           {{gt, r2, r3}, "gt, r2, r3", "gt_r2_r3"},
                           {{eq, r2, r0}, "eq, r2, r0", "eq_r2_r0"},
                           {{vs, r10, r12}, "vs, r10, r12", "vs_r10_r12"},
                           {{gt, r11, r7}, "gt, r11, r7", "gt_r11_r7"},
                           {{vs, r13, r2}, "vs, r13, r2", "vs_r13_r2"},
                           {{ls, r11, r12}, "ls, r11, r12", "ls_r11_r12"},
                           {{al, r8, r8}, "al, r8, r8", "al_r8_r8"},
                           {{hi, r6, r10}, "hi, r6, r10", "hi_r6_r10"},
                           {{vs, r5, r1}, "vs, r5, r1", "vs_r5_r1"},
                           {{ls, r0, r10}, "ls, r0, r10", "ls_r0_r10"},
                           {{gt, r0, r1}, "gt, r0, r1", "gt_r0_r1"},
                           {{ne, r11, r12}, "ne, r11, r12", "ne_r11_r12"},
                           {{ne, r13, r5}, "ne, r13, r5", "ne_r13_r5"},
                           {{mi, r0, r12}, "mi, r0, r12", "mi_r0_r12"},
                           {{lt, r11, r6}, "lt, r11, r6", "lt_r11_r6"},
                           {{eq, r11, r14}, "eq, r11, r14", "eq_r11_r14"},
                           {{vc, r11, r10}, "vc, r11, r10", "vc_r11_r10"},
                           {{cs, r3, r3}, "cs, r3, r3", "cs_r3_r3"},
                           {{le, r12, r2}, "le, r12, r2", "le_r12_r2"},
                           {{hi, r13, r8}, "hi, r13, r8", "hi_r13_r8"},
                           {{pl, r6, r6}, "pl, r6, r6", "pl_r6_r6"},
                           {{al, r4, r5}, "al, r4, r5", "al_r4_r5"},
                           {{hi, r1, r0}, "hi, r1, r0", "hi_r1_r0"},
                           {{ls, r10, r11}, "ls, r10, r11", "ls_r10_r11"},
                           {{al, r8, r7}, "al, r8, r7", "al_r8_r7"},
                           {{vc, r7, r1}, "vc, r7, r1", "vc_r7_r1"},
                           {{ne, r7, r14}, "ne, r7, r14", "ne_r7_r14"},
                           {{lt, r5, r9}, "lt, r5, r9", "lt_r5_r9"},
                           {{al, r13, r8}, "al, r13, r8", "al_r13_r8"},
                           {{ls, r6, r14}, "ls, r6, r14", "ls_r6_r14"},
                           {{eq, r12, r0}, "eq, r12, r0", "eq_r12_r0"},
                           {{mi, r8, r14}, "mi, r8, r14", "mi_r8_r14"},
                           {{hi, r13, r2}, "hi, r13, r2", "hi_r13_r2"},
                           {{al, r13, r2}, "al, r13, r2", "al_r13_r2"},
                           {{vs, r8, r9}, "vs, r8, r9", "vs_r8_r9"},
                           {{cc, r6, r2}, "cc, r6, r2", "cc_r6_r2"},
                           {{lt, r8, r0}, "lt, r8, r0", "lt_r8_r0"},
                           {{ls, r3, r13}, "ls, r3, r13", "ls_r3_r13"},
                           {{gt, r10, r14}, "gt, r10, r14", "gt_r10_r14"},
                           {{pl, r9, r8}, "pl, r9, r8", "pl_r9_r8"},
                           {{pl, r14, r11}, "pl, r14, r11", "pl_r14_r11"},
                           {{lt, r9, r3}, "lt, r9, r3", "lt_r9_r3"},
                           {{hi, r9, r0}, "hi, r9, r0", "hi_r9_r0"},
                           {{cc, r9, r3}, "cc, r9, r3", "cc_r9_r3"},
                           {{ge, r10, r14}, "ge, r10, r14", "ge_r10_r14"},
                           {{vs, r2, r1}, "vs, r2, r1", "vs_r2_r1"},
                           {{vc, r4, r13}, "vc, r4, r13", "vc_r4_r13"},
                           {{ls, r1, r9}, "ls, r1, r9", "ls_r1_r9"},
                           {{lt, r13, r11}, "lt, r13, r11", "lt_r13_r11"},
                           {{cs, r2, r10}, "cs, r2, r10", "cs_r2_r10"},
                           {{le, r0, r13}, "le, r0, r13", "le_r0_r13"},
                           {{al, r13, r11}, "al, r13, r11", "al_r13_r11"},
                           {{eq, r5, r2}, "eq, r5, r2", "eq_r5_r2"},
                           {{vs, r5, r12}, "vs, r5, r12", "vs_r5_r12"},
                           {{al, r12, r0}, "al, r12, r0", "al_r12_r0"},
                           {{le, r13, r9}, "le, r13, r9", "le_r13_r9"},
                           {{cs, r14, r4}, "cs, r14, r4", "cs_r14_r4"},
                           {{ne, r5, r7}, "ne, r5, r7", "ne_r5_r7"},
                           {{al, r6, r6}, "al, r6, r6", "al_r6_r6"},
                           {{gt, r4, r8}, "gt, r4, r8", "gt_r4_r8"},
                           {{gt, r12, r8}, "gt, r12, r8", "gt_r12_r8"},
                           {{eq, r4, r12}, "eq, r4, r12", "eq_r4_r12"},
                           {{cs, r7, r13}, "cs, r7, r13", "cs_r7_r13"},
                           {{cs, r2, r4}, "cs, r2, r4", "cs_r2_r4"},
                           {{al, r10, r3}, "al, r10, r3", "al_r10_r3"},
                           {{cs, r8, r9}, "cs, r8, r9", "cs_r8_r9"},
                           {{cs, r1, r12}, "cs, r1, r12", "cs_r1_r12"},
                           {{gt, r13, r0}, "gt, r13, r0", "gt_r13_r0"},
                           {{vc, r13, r12}, "vc, r13, r12", "vc_r13_r12"},
                           {{lt, r12, r14}, "lt, r12, r14", "lt_r12_r14"},
                           {{lt, r0, r1}, "lt, r0, r1", "lt_r0_r1"},
                           {{cc, r10, r2}, "cc, r10, r2", "cc_r10_r2"},
                           {{le, r3, r5}, "le, r3, r5", "le_r3_r5"},
                           {{eq, r2, r11}, "eq, r2, r11", "eq_r2_r11"},
                           {{al, r12, r4}, "al, r12, r4", "al_r12_r4"},
                           {{cs, r6, r5}, "cs, r6, r5", "cs_r6_r5"},
                           {{hi, r13, r10}, "hi, r13, r10", "hi_r13_r10"},
                           {{vs, r4, r3}, "vs, r4, r3", "vs_r4_r3"},
                           {{ls, r7, r9}, "ls, r7, r9", "ls_r7_r9"},
                           {{vs, r14, r8}, "vs, r14, r8", "vs_r14_r8"},
                           {{cs, r1, r0}, "cs, r1, r0", "cs_r1_r0"},
                           {{mi, r10, r8}, "mi, r10, r8", "mi_r10_r8"},
                           {{ge, r10, r4}, "ge, r10, r4", "ge_r10_r4"},
                           {{cc, r5, r4}, "cc, r5, r4", "cc_r5_r4"},
                           {{lt, r0, r7}, "lt, r0, r7", "lt_r0_r7"},
                           {{mi, r14, r10}, "mi, r14, r10", "mi_r14_r10"},
                           {{mi, r0, r14}, "mi, r0, r14", "mi_r0_r14"},
                           {{eq, r14, r10}, "eq, r14, r10", "eq_r14_r10"},
                           {{ls, r4, r4}, "ls, r4, r4", "ls_r4_r4"},
                           {{pl, r14, r1}, "pl, r14, r1", "pl_r14_r1"},
                           {{eq, r10, r8}, "eq, r10, r8", "eq_r10_r8"},
                           {{cs, r10, r6}, "cs, r10, r6", "cs_r10_r6"},
                           {{al, r12, r2}, "al, r12, r2", "al_r12_r2"},
                           {{ls, r12, r1}, "ls, r12, r1", "ls_r12_r1"},
                           {{eq, r12, r11}, "eq, r12, r11", "eq_r12_r11"},
                           {{vc, r4, r1}, "vc, r4, r1", "vc_r4_r1"},
                           {{vs, r12, r2}, "vs, r12, r2", "vs_r12_r2"},
                           {{al, r11, r8}, "al, r11, r8", "al_r11_r8"},
                           {{hi, r6, r13}, "hi, r6, r13", "hi_r6_r13"},
                           {{eq, r2, r8}, "eq, r2, r8", "eq_r2_r8"},
                           {{cc, r9, r5}, "cc, r9, r5", "cc_r9_r5"},
                           {{cc, r9, r7}, "cc, r9, r7", "cc_r9_r7"},
                           {{lt, r4, r6}, "lt, r4, r6", "lt_r4_r6"},
                           {{cc, r14, r13}, "cc, r14, r13", "cc_r14_r13"},
                           {{vc, r3, r2}, "vc, r3, r2", "vc_r3_r2"},
                           {{al, r6, r13}, "al, r6, r13", "al_r6_r13"},
                           {{vs, r10, r10}, "vs, r10, r10", "vs_r10_r10"},
                           {{cs, r6, r10}, "cs, r6, r10", "cs_r6_r10"},
                           {{cc, r8, r12}, "cc, r8, r12", "cc_r8_r12"},
                           {{vs, r7, r5}, "vs, r7, r5", "vs_r7_r5"},
                           {{pl, r14, r10}, "pl, r14, r10", "pl_r14_r10"},
                           {{hi, r1, r14}, "hi, r1, r14", "hi_r1_r14"},
                           {{vc, r8, r12}, "vc, r8, r12", "vc_r8_r12"},
                           {{ls, r2, r4}, "ls, r2, r4", "ls_r2_r4"},
                           {{mi, r5, r12}, "mi, r5, r12", "mi_r5_r12"},
                           {{eq, r6, r12}, "eq, r6, r12", "eq_r6_r12"},
                           {{lt, r14, r9}, "lt, r14, r9", "lt_r14_r9"},
                           {{lt, r11, r9}, "lt, r11, r9", "lt_r11_r9"},
                           {{ne, r1, r9}, "ne, r1, r9", "ne_r1_r9"},
                           {{pl, r11, r8}, "pl, r11, r8", "pl_r11_r8"},
                           {{ne, r0, r6}, "ne, r0, r6", "ne_r0_r6"},
                           {{vs, r4, r4}, "vs, r4, r4", "vs_r4_r4"},
                           {{ls, r12, r9}, "ls, r12, r9", "ls_r12_r9"},
                           {{cs, r9, r7}, "cs, r9, r7", "cs_r9_r7"},
                           {{ne, r7, r2}, "ne, r7, r2", "ne_r7_r2"},
                           {{hi, r9, r10}, "hi, r9, r10", "hi_r9_r10"},
                           {{gt, r5, r2}, "gt, r5, r2", "gt_r5_r2"},
                           {{pl, r4, r6}, "pl, r4, r6", "pl_r4_r6"},
                           {{lt, r11, r8}, "lt, r11, r8", "lt_r11_r8"},
                           {{hi, r13, r4}, "hi, r13, r4", "hi_r13_r4"},
                           {{le, r1, r10}, "le, r1, r10", "le_r1_r10"},
                           {{le, r11, r3}, "le, r11, r3", "le_r11_r3"},
                           {{pl, r0, r11}, "pl, r0, r11", "pl_r0_r11"},
                           {{le, r7, r4}, "le, r7, r4", "le_r7_r4"},
                           {{le, r2, r11}, "le, r2, r11", "le_r2_r11"},
                           {{cc, r1, r10}, "cc, r1, r10", "cc_r1_r10"},
                           {{gt, r13, r14}, "gt, r13, r14", "gt_r13_r14"},
                           {{ls, r10, r6}, "ls, r10, r6", "ls_r10_r6"},
                           {{ls, r13, r4}, "ls, r13, r4", "ls_r13_r4"},
                           {{gt, r12, r13}, "gt, r12, r13", "gt_r12_r13"},
                           {{pl, r13, r7}, "pl, r13, r7", "pl_r13_r7"},
                           {{le, r14, r0}, "le, r14, r0", "le_r14_r0"},
                           {{gt, r1, r4}, "gt, r1, r4", "gt_r1_r4"},
                           {{mi, r3, r5}, "mi, r3, r5", "mi_r3_r5"},
                           {{vc, r4, r0}, "vc, r4, r0", "vc_r4_r0"},
                           {{gt, r4, r11}, "gt, r4, r11", "gt_r4_r11"},
                           {{vc, r5, r3}, "vc, r5, r3", "vc_r5_r3"},
                           {{pl, r4, r7}, "pl, r4, r7", "pl_r4_r7"},
                           {{al, r12, r6}, "al, r12, r6", "al_r12_r6"},
                           {{mi, r2, r3}, "mi, r2, r3", "mi_r2_r3"},
                           {{lt, r12, r4}, "lt, r12, r4", "lt_r12_r4"},
                           {{lt, r10, r4}, "lt, r10, r4", "lt_r10_r4"},
                           {{hi, r10, r8}, "hi, r10, r8", "hi_r10_r8"},
                           {{al, r0, r14}, "al, r0, r14", "al_r0_r14"},
                           {{cc, r3, r3}, "cc, r3, r3", "cc_r3_r3"},
                           {{pl, r10, r13}, "pl, r10, r13", "pl_r10_r13"},
                           {{lt, r7, r8}, "lt, r7, r8", "lt_r7_r8"},
                           {{le, r12, r7}, "le, r12, r7", "le_r12_r7"},
                           {{ge, r11, r8}, "ge, r11, r8", "ge_r11_r8"},
                           {{ls, r10, r8}, "ls, r10, r8", "ls_r10_r8"},
                           {{vs, r9, r1}, "vs, r9, r1", "vs_r9_r1"},
                           {{ne, r10, r10}, "ne, r10, r10", "ne_r10_r10"},
                           {{al, r6, r4}, "al, r6, r4", "al_r6_r4"},
                           {{ls, r10, r0}, "ls, r10, r0", "ls_r10_r0"},
                           {{eq, r7, r1}, "eq, r7, r1", "eq_r7_r1"},
                           {{al, r11, r6}, "al, r11, r6", "al_r11_r6"},
                           {{ge, r5, r10}, "ge, r5, r10", "ge_r5_r10"},
                           {{vs, r0, r8}, "vs, r0, r8", "vs_r0_r8"},
                           {{lt, r2, r1}, "lt, r2, r1", "lt_r2_r1"},
                           {{le, r13, r2}, "le, r13, r2", "le_r13_r2"},
                           {{al, r13, r6}, "al, r13, r6", "al_r13_r6"},
                           {{lt, r12, r10}, "lt, r12, r10", "lt_r12_r10"},
                           {{al, r13, r1}, "al, r13, r1", "al_r13_r1"},
                           {{ge, r8, r14}, "ge, r8, r14", "ge_r8_r14"},
                           {{mi, r11, r6}, "mi, r11, r6", "mi_r11_r6"},
                           {{pl, r12, r14}, "pl, r12, r14", "pl_r12_r14"},
                           {{ne, r14, r9}, "ne, r14, r9", "ne_r14_r9"},
                           {{cc, r3, r1}, "cc, r3, r1", "cc_r3_r1"},
                           {{ge, r0, r8}, "ge, r0, r8", "ge_r0_r8"},
                           {{vc, r7, r9}, "vc, r7, r9", "vc_r7_r9"},
                           {{gt, r11, r4}, "gt, r11, r4", "gt_r11_r4"},
                           {{mi, r4, r0}, "mi, r4, r0", "mi_r4_r0"},
                           {{ls, r3, r11}, "ls, r3, r11", "ls_r3_r11"},
                           {{le, r14, r5}, "le, r14, r5", "le_r14_r5"},
                           {{ls, r6, r7}, "ls, r6, r7", "ls_r6_r7"},
                           {{ne, r4, r6}, "ne, r4, r6", "ne_r4_r6"},
                           {{cc, r1, r14}, "cc, r1, r14", "cc_r1_r14"},
                           {{pl, r11, r7}, "pl, r11, r7", "pl_r11_r7"},
                           {{vc, r6, r5}, "vc, r6, r5", "vc_r6_r5"},
                           {{al, r1, r11}, "al, r1, r11", "al_r1_r11"},
                           {{ne, r2, r13}, "ne, r2, r13", "ne_r2_r13"},
                           {{vc, r14, r6}, "vc, r14, r6", "vc_r14_r6"},
                           {{gt, r3, r8}, "gt, r3, r8", "gt_r3_r8"},
                           {{pl, r1, r8}, "pl, r1, r8", "pl_r1_r8"},
                           {{vc, r2, r14}, "vc, r2, r14", "vc_r2_r14"},
                           {{pl, r12, r3}, "pl, r12, r3", "pl_r12_r3"},
                           {{ls, r0, r12}, "ls, r0, r12", "ls_r0_r12"},
                           {{le, r2, r1}, "le, r2, r1", "le_r2_r1"},
                           {{eq, r13, r5}, "eq, r13, r5", "eq_r13_r5"},
                           {{al, r11, r11}, "al, r11, r11", "al_r11_r11"},
                           {{ls, r9, r3}, "ls, r9, r3", "ls_r9_r3"},
                           {{ne, r7, r12}, "ne, r7, r12", "ne_r7_r12"},
                           {{al, r6, r2}, "al, r6, r2", "al_r6_r2"},
                           {{ne, r7, r4}, "ne, r7, r4", "ne_r7_r4"},
                           {{vc, r7, r5}, "vc, r7, r5", "vc_r7_r5"},
                           {{ne, r7, r1}, "ne, r7, r1", "ne_r7_r1"},
                           {{eq, r5, r3}, "eq, r5, r3", "eq_r5_r3"},
                           {{cs, r3, r5}, "cs, r3, r5", "cs_r3_r5"},
                           {{lt, r7, r4}, "lt, r7, r4", "lt_r7_r4"},
                           {{ls, r4, r5}, "ls, r4, r5", "ls_r4_r5"},
                           {{eq, r11, r2}, "eq, r11, r2", "eq_r11_r2"},
                           {{hi, r2, r6}, "hi, r2, r6", "hi_r2_r6"},
                           {{vc, r2, r3}, "vc, r2, r3", "vc_r2_r3"},
                           {{ls, r12, r12}, "ls, r12, r12", "ls_r12_r12"},
                           {{lt, r8, r2}, "lt, r8, r2", "lt_r8_r2"},
                           {{cc, r14, r8}, "cc, r14, r8", "cc_r14_r8"},
                           {{ge, r12, r8}, "ge, r12, r8", "ge_r12_r8"}};

typedef void (MacroAssembler::*Fn)(Condition cond, Register rd, Register rn);

// Use a customised disassembler to label test cases.
typedef std::map<uint32_t, int> TestCaseSymbolMap;

class TestDisassembler : public PrintDisassembler {
 public:
  TestDisassembler(std::ostream& os,  // NOLINT(runtime/references)
                   const TestCaseSymbolMap& symbols,
                   const char* mnemonic)
      : PrintDisassembler(os), symbols_(symbols), mnemonic_(mnemonic) {}

  virtual void PrintCodeAddress(uint32_t pc) VIXL_OVERRIDE {
    // Label test cases.
    TestCaseSymbolMap::const_iterator symbol = symbols_.find(pc);
    if (symbol != symbols_.end()) {
      int n = symbol->second;
      os().os() << "// " << mnemonic_ << "(" << kTests[n].operands_description
                << ")" << std::endl;
    }
    // Print the code address as normal.
    PrintDisassembler::PrintCodeAddress(pc);
  }

 private:
  const TestCaseSymbolMap& symbols_;
  const char* mnemonic_;
};

void TestHelper(Fn instruction, const char* mnemonic) {
  MacroAssembler masm(BUF_SIZE);

  masm.UseT32();

  TestCaseSymbolMap symbols;

  for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
    if (Test::disassemble()) {
      // TODO: This will fail if the MacroAssembler generates no code. We can
      // fix this with multimap but then we must take care to print the labels
      // in the correct order. (Insertion order is only preserved for C++11.)
      symbols.insert(std::pair<uint32_t, int>(masm.GetCursorOffset(), i));
    }

    // Values to pass to the macro-assembler.
    UseScratchRegisterScope scratch_registers(&masm);
    Condition cond = kTests[i].operands.cond;
    Register rd = kTests[i].operands.rd;
    Register rn = kTests[i].operands.rn;
    scratch_registers.Exclude(rd);
    scratch_registers.Exclude(rn);

    (masm.*instruction)(cond, rd, rn);

    // For now, these test don't currently produce (or check) any trace; we just
    // check that the MacroAssembler didn't crash.
    // TODO: We could generate disassembly as a trace here, to check for sane
    // output, though the trace would need to be manually checked.
  }

  masm.FinalizeCode();

  if (Test::disassemble()) {
    // Disassemble to stdout if given --disassemble on the command line.
    TestDisassembler dis(std::cout, symbols, mnemonic);
    if (masm.IsUsingT32()) {
      dis.DisassembleT32Buffer(masm.GetBuffer()->GetStartAddress<uint16_t*>(),
                               masm.GetCursorOffset());
    } else {
      dis.DisassembleA32Buffer(masm.GetBuffer()->GetStartAddress<uint32_t*>(),
                               masm.GetCursorOffset());
    }
  }
}

// Instantiate tests for each instruction in the list.
#define TEST(mnemonic)                                                         \
  void Test_##mnemonic() { TestHelper(&MacroAssembler::mnemonic, #mnemonic); } \
  Test test_##mnemonic("AARCH32_MACRO_ASSEMBLER_COND_RD_RN_" #mnemonic,        \
                       &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
