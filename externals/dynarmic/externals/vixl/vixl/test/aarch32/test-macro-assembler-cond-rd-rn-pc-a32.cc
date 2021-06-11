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
#define FOREACH_INSTRUCTION(M) M(Rrx)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` has
// various layouts across generated tests so it absolutely cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_A32
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
const TestData kTests[] = {{{eq, r0, r15}, "eq, r0, r15", "eq_r0_r15"},
                           {{eq, r1, r15}, "eq, r1, r15", "eq_r1_r15"},
                           {{eq, r2, r15}, "eq, r2, r15", "eq_r2_r15"},
                           {{eq, r3, r15}, "eq, r3, r15", "eq_r3_r15"},
                           {{eq, r4, r15}, "eq, r4, r15", "eq_r4_r15"},
                           {{eq, r5, r15}, "eq, r5, r15", "eq_r5_r15"},
                           {{eq, r6, r15}, "eq, r6, r15", "eq_r6_r15"},
                           {{eq, r7, r15}, "eq, r7, r15", "eq_r7_r15"},
                           {{eq, r8, r15}, "eq, r8, r15", "eq_r8_r15"},
                           {{eq, r9, r15}, "eq, r9, r15", "eq_r9_r15"},
                           {{eq, r10, r15}, "eq, r10, r15", "eq_r10_r15"},
                           {{eq, r11, r15}, "eq, r11, r15", "eq_r11_r15"},
                           {{eq, r12, r15}, "eq, r12, r15", "eq_r12_r15"},
                           {{eq, r13, r15}, "eq, r13, r15", "eq_r13_r15"},
                           {{eq, r14, r15}, "eq, r14, r15", "eq_r14_r15"},
                           {{eq, r15, r0}, "eq, r15, r0", "eq_r15_r0"},
                           {{eq, r15, r1}, "eq, r15, r1", "eq_r15_r1"},
                           {{eq, r15, r2}, "eq, r15, r2", "eq_r15_r2"},
                           {{eq, r15, r3}, "eq, r15, r3", "eq_r15_r3"},
                           {{eq, r15, r4}, "eq, r15, r4", "eq_r15_r4"},
                           {{eq, r15, r5}, "eq, r15, r5", "eq_r15_r5"},
                           {{eq, r15, r6}, "eq, r15, r6", "eq_r15_r6"},
                           {{eq, r15, r7}, "eq, r15, r7", "eq_r15_r7"},
                           {{eq, r15, r8}, "eq, r15, r8", "eq_r15_r8"},
                           {{eq, r15, r9}, "eq, r15, r9", "eq_r15_r9"},
                           {{eq, r15, r10}, "eq, r15, r10", "eq_r15_r10"},
                           {{eq, r15, r11}, "eq, r15, r11", "eq_r15_r11"},
                           {{eq, r15, r12}, "eq, r15, r12", "eq_r15_r12"},
                           {{eq, r15, r13}, "eq, r15, r13", "eq_r15_r13"},
                           {{eq, r15, r14}, "eq, r15, r14", "eq_r15_r14"},
                           {{eq, r15, r15}, "eq, r15, r15", "eq_r15_r15"},
                           {{ne, r0, r15}, "ne, r0, r15", "ne_r0_r15"},
                           {{ne, r1, r15}, "ne, r1, r15", "ne_r1_r15"},
                           {{ne, r2, r15}, "ne, r2, r15", "ne_r2_r15"},
                           {{ne, r3, r15}, "ne, r3, r15", "ne_r3_r15"},
                           {{ne, r4, r15}, "ne, r4, r15", "ne_r4_r15"},
                           {{ne, r5, r15}, "ne, r5, r15", "ne_r5_r15"},
                           {{ne, r6, r15}, "ne, r6, r15", "ne_r6_r15"},
                           {{ne, r7, r15}, "ne, r7, r15", "ne_r7_r15"},
                           {{ne, r8, r15}, "ne, r8, r15", "ne_r8_r15"},
                           {{ne, r9, r15}, "ne, r9, r15", "ne_r9_r15"},
                           {{ne, r10, r15}, "ne, r10, r15", "ne_r10_r15"},
                           {{ne, r11, r15}, "ne, r11, r15", "ne_r11_r15"},
                           {{ne, r12, r15}, "ne, r12, r15", "ne_r12_r15"},
                           {{ne, r13, r15}, "ne, r13, r15", "ne_r13_r15"},
                           {{ne, r14, r15}, "ne, r14, r15", "ne_r14_r15"},
                           {{ne, r15, r0}, "ne, r15, r0", "ne_r15_r0"},
                           {{ne, r15, r1}, "ne, r15, r1", "ne_r15_r1"},
                           {{ne, r15, r2}, "ne, r15, r2", "ne_r15_r2"},
                           {{ne, r15, r3}, "ne, r15, r3", "ne_r15_r3"},
                           {{ne, r15, r4}, "ne, r15, r4", "ne_r15_r4"},
                           {{ne, r15, r5}, "ne, r15, r5", "ne_r15_r5"},
                           {{ne, r15, r6}, "ne, r15, r6", "ne_r15_r6"},
                           {{ne, r15, r7}, "ne, r15, r7", "ne_r15_r7"},
                           {{ne, r15, r8}, "ne, r15, r8", "ne_r15_r8"},
                           {{ne, r15, r9}, "ne, r15, r9", "ne_r15_r9"},
                           {{ne, r15, r10}, "ne, r15, r10", "ne_r15_r10"},
                           {{ne, r15, r11}, "ne, r15, r11", "ne_r15_r11"},
                           {{ne, r15, r12}, "ne, r15, r12", "ne_r15_r12"},
                           {{ne, r15, r13}, "ne, r15, r13", "ne_r15_r13"},
                           {{ne, r15, r14}, "ne, r15, r14", "ne_r15_r14"},
                           {{ne, r15, r15}, "ne, r15, r15", "ne_r15_r15"},
                           {{cs, r0, r15}, "cs, r0, r15", "cs_r0_r15"},
                           {{cs, r1, r15}, "cs, r1, r15", "cs_r1_r15"},
                           {{cs, r2, r15}, "cs, r2, r15", "cs_r2_r15"},
                           {{cs, r3, r15}, "cs, r3, r15", "cs_r3_r15"},
                           {{cs, r4, r15}, "cs, r4, r15", "cs_r4_r15"},
                           {{cs, r5, r15}, "cs, r5, r15", "cs_r5_r15"},
                           {{cs, r6, r15}, "cs, r6, r15", "cs_r6_r15"},
                           {{cs, r7, r15}, "cs, r7, r15", "cs_r7_r15"},
                           {{cs, r8, r15}, "cs, r8, r15", "cs_r8_r15"},
                           {{cs, r9, r15}, "cs, r9, r15", "cs_r9_r15"},
                           {{cs, r10, r15}, "cs, r10, r15", "cs_r10_r15"},
                           {{cs, r11, r15}, "cs, r11, r15", "cs_r11_r15"},
                           {{cs, r12, r15}, "cs, r12, r15", "cs_r12_r15"},
                           {{cs, r13, r15}, "cs, r13, r15", "cs_r13_r15"},
                           {{cs, r14, r15}, "cs, r14, r15", "cs_r14_r15"},
                           {{cs, r15, r0}, "cs, r15, r0", "cs_r15_r0"},
                           {{cs, r15, r1}, "cs, r15, r1", "cs_r15_r1"},
                           {{cs, r15, r2}, "cs, r15, r2", "cs_r15_r2"},
                           {{cs, r15, r3}, "cs, r15, r3", "cs_r15_r3"},
                           {{cs, r15, r4}, "cs, r15, r4", "cs_r15_r4"},
                           {{cs, r15, r5}, "cs, r15, r5", "cs_r15_r5"},
                           {{cs, r15, r6}, "cs, r15, r6", "cs_r15_r6"},
                           {{cs, r15, r7}, "cs, r15, r7", "cs_r15_r7"},
                           {{cs, r15, r8}, "cs, r15, r8", "cs_r15_r8"},
                           {{cs, r15, r9}, "cs, r15, r9", "cs_r15_r9"},
                           {{cs, r15, r10}, "cs, r15, r10", "cs_r15_r10"},
                           {{cs, r15, r11}, "cs, r15, r11", "cs_r15_r11"},
                           {{cs, r15, r12}, "cs, r15, r12", "cs_r15_r12"},
                           {{cs, r15, r13}, "cs, r15, r13", "cs_r15_r13"},
                           {{cs, r15, r14}, "cs, r15, r14", "cs_r15_r14"},
                           {{cs, r15, r15}, "cs, r15, r15", "cs_r15_r15"},
                           {{cc, r0, r15}, "cc, r0, r15", "cc_r0_r15"},
                           {{cc, r1, r15}, "cc, r1, r15", "cc_r1_r15"},
                           {{cc, r2, r15}, "cc, r2, r15", "cc_r2_r15"},
                           {{cc, r3, r15}, "cc, r3, r15", "cc_r3_r15"},
                           {{cc, r4, r15}, "cc, r4, r15", "cc_r4_r15"},
                           {{cc, r5, r15}, "cc, r5, r15", "cc_r5_r15"},
                           {{cc, r6, r15}, "cc, r6, r15", "cc_r6_r15"},
                           {{cc, r7, r15}, "cc, r7, r15", "cc_r7_r15"},
                           {{cc, r8, r15}, "cc, r8, r15", "cc_r8_r15"},
                           {{cc, r9, r15}, "cc, r9, r15", "cc_r9_r15"},
                           {{cc, r10, r15}, "cc, r10, r15", "cc_r10_r15"},
                           {{cc, r11, r15}, "cc, r11, r15", "cc_r11_r15"},
                           {{cc, r12, r15}, "cc, r12, r15", "cc_r12_r15"},
                           {{cc, r13, r15}, "cc, r13, r15", "cc_r13_r15"},
                           {{cc, r14, r15}, "cc, r14, r15", "cc_r14_r15"},
                           {{cc, r15, r0}, "cc, r15, r0", "cc_r15_r0"},
                           {{cc, r15, r1}, "cc, r15, r1", "cc_r15_r1"},
                           {{cc, r15, r2}, "cc, r15, r2", "cc_r15_r2"},
                           {{cc, r15, r3}, "cc, r15, r3", "cc_r15_r3"},
                           {{cc, r15, r4}, "cc, r15, r4", "cc_r15_r4"},
                           {{cc, r15, r5}, "cc, r15, r5", "cc_r15_r5"},
                           {{cc, r15, r6}, "cc, r15, r6", "cc_r15_r6"},
                           {{cc, r15, r7}, "cc, r15, r7", "cc_r15_r7"},
                           {{cc, r15, r8}, "cc, r15, r8", "cc_r15_r8"},
                           {{cc, r15, r9}, "cc, r15, r9", "cc_r15_r9"},
                           {{cc, r15, r10}, "cc, r15, r10", "cc_r15_r10"},
                           {{cc, r15, r11}, "cc, r15, r11", "cc_r15_r11"},
                           {{cc, r15, r12}, "cc, r15, r12", "cc_r15_r12"},
                           {{cc, r15, r13}, "cc, r15, r13", "cc_r15_r13"},
                           {{cc, r15, r14}, "cc, r15, r14", "cc_r15_r14"},
                           {{cc, r15, r15}, "cc, r15, r15", "cc_r15_r15"},
                           {{mi, r0, r15}, "mi, r0, r15", "mi_r0_r15"},
                           {{mi, r1, r15}, "mi, r1, r15", "mi_r1_r15"},
                           {{mi, r2, r15}, "mi, r2, r15", "mi_r2_r15"},
                           {{mi, r3, r15}, "mi, r3, r15", "mi_r3_r15"},
                           {{mi, r4, r15}, "mi, r4, r15", "mi_r4_r15"},
                           {{mi, r5, r15}, "mi, r5, r15", "mi_r5_r15"},
                           {{mi, r6, r15}, "mi, r6, r15", "mi_r6_r15"},
                           {{mi, r7, r15}, "mi, r7, r15", "mi_r7_r15"},
                           {{mi, r8, r15}, "mi, r8, r15", "mi_r8_r15"},
                           {{mi, r9, r15}, "mi, r9, r15", "mi_r9_r15"},
                           {{mi, r10, r15}, "mi, r10, r15", "mi_r10_r15"},
                           {{mi, r11, r15}, "mi, r11, r15", "mi_r11_r15"},
                           {{mi, r12, r15}, "mi, r12, r15", "mi_r12_r15"},
                           {{mi, r13, r15}, "mi, r13, r15", "mi_r13_r15"},
                           {{mi, r14, r15}, "mi, r14, r15", "mi_r14_r15"},
                           {{mi, r15, r0}, "mi, r15, r0", "mi_r15_r0"},
                           {{mi, r15, r1}, "mi, r15, r1", "mi_r15_r1"},
                           {{mi, r15, r2}, "mi, r15, r2", "mi_r15_r2"},
                           {{mi, r15, r3}, "mi, r15, r3", "mi_r15_r3"},
                           {{mi, r15, r4}, "mi, r15, r4", "mi_r15_r4"},
                           {{mi, r15, r5}, "mi, r15, r5", "mi_r15_r5"},
                           {{mi, r15, r6}, "mi, r15, r6", "mi_r15_r6"},
                           {{mi, r15, r7}, "mi, r15, r7", "mi_r15_r7"},
                           {{mi, r15, r8}, "mi, r15, r8", "mi_r15_r8"},
                           {{mi, r15, r9}, "mi, r15, r9", "mi_r15_r9"},
                           {{mi, r15, r10}, "mi, r15, r10", "mi_r15_r10"},
                           {{mi, r15, r11}, "mi, r15, r11", "mi_r15_r11"},
                           {{mi, r15, r12}, "mi, r15, r12", "mi_r15_r12"},
                           {{mi, r15, r13}, "mi, r15, r13", "mi_r15_r13"},
                           {{mi, r15, r14}, "mi, r15, r14", "mi_r15_r14"},
                           {{mi, r15, r15}, "mi, r15, r15", "mi_r15_r15"},
                           {{pl, r0, r15}, "pl, r0, r15", "pl_r0_r15"},
                           {{pl, r1, r15}, "pl, r1, r15", "pl_r1_r15"},
                           {{pl, r2, r15}, "pl, r2, r15", "pl_r2_r15"},
                           {{pl, r3, r15}, "pl, r3, r15", "pl_r3_r15"},
                           {{pl, r4, r15}, "pl, r4, r15", "pl_r4_r15"},
                           {{pl, r5, r15}, "pl, r5, r15", "pl_r5_r15"},
                           {{pl, r6, r15}, "pl, r6, r15", "pl_r6_r15"},
                           {{pl, r7, r15}, "pl, r7, r15", "pl_r7_r15"},
                           {{pl, r8, r15}, "pl, r8, r15", "pl_r8_r15"},
                           {{pl, r9, r15}, "pl, r9, r15", "pl_r9_r15"},
                           {{pl, r10, r15}, "pl, r10, r15", "pl_r10_r15"},
                           {{pl, r11, r15}, "pl, r11, r15", "pl_r11_r15"},
                           {{pl, r12, r15}, "pl, r12, r15", "pl_r12_r15"},
                           {{pl, r13, r15}, "pl, r13, r15", "pl_r13_r15"},
                           {{pl, r14, r15}, "pl, r14, r15", "pl_r14_r15"},
                           {{pl, r15, r0}, "pl, r15, r0", "pl_r15_r0"},
                           {{pl, r15, r1}, "pl, r15, r1", "pl_r15_r1"},
                           {{pl, r15, r2}, "pl, r15, r2", "pl_r15_r2"},
                           {{pl, r15, r3}, "pl, r15, r3", "pl_r15_r3"},
                           {{pl, r15, r4}, "pl, r15, r4", "pl_r15_r4"},
                           {{pl, r15, r5}, "pl, r15, r5", "pl_r15_r5"},
                           {{pl, r15, r6}, "pl, r15, r6", "pl_r15_r6"},
                           {{pl, r15, r7}, "pl, r15, r7", "pl_r15_r7"},
                           {{pl, r15, r8}, "pl, r15, r8", "pl_r15_r8"},
                           {{pl, r15, r9}, "pl, r15, r9", "pl_r15_r9"},
                           {{pl, r15, r10}, "pl, r15, r10", "pl_r15_r10"},
                           {{pl, r15, r11}, "pl, r15, r11", "pl_r15_r11"},
                           {{pl, r15, r12}, "pl, r15, r12", "pl_r15_r12"},
                           {{pl, r15, r13}, "pl, r15, r13", "pl_r15_r13"},
                           {{pl, r15, r14}, "pl, r15, r14", "pl_r15_r14"},
                           {{pl, r15, r15}, "pl, r15, r15", "pl_r15_r15"},
                           {{vs, r0, r15}, "vs, r0, r15", "vs_r0_r15"},
                           {{vs, r1, r15}, "vs, r1, r15", "vs_r1_r15"},
                           {{vs, r2, r15}, "vs, r2, r15", "vs_r2_r15"},
                           {{vs, r3, r15}, "vs, r3, r15", "vs_r3_r15"},
                           {{vs, r4, r15}, "vs, r4, r15", "vs_r4_r15"},
                           {{vs, r5, r15}, "vs, r5, r15", "vs_r5_r15"},
                           {{vs, r6, r15}, "vs, r6, r15", "vs_r6_r15"},
                           {{vs, r7, r15}, "vs, r7, r15", "vs_r7_r15"},
                           {{vs, r8, r15}, "vs, r8, r15", "vs_r8_r15"},
                           {{vs, r9, r15}, "vs, r9, r15", "vs_r9_r15"},
                           {{vs, r10, r15}, "vs, r10, r15", "vs_r10_r15"},
                           {{vs, r11, r15}, "vs, r11, r15", "vs_r11_r15"},
                           {{vs, r12, r15}, "vs, r12, r15", "vs_r12_r15"},
                           {{vs, r13, r15}, "vs, r13, r15", "vs_r13_r15"},
                           {{vs, r14, r15}, "vs, r14, r15", "vs_r14_r15"},
                           {{vs, r15, r0}, "vs, r15, r0", "vs_r15_r0"},
                           {{vs, r15, r1}, "vs, r15, r1", "vs_r15_r1"},
                           {{vs, r15, r2}, "vs, r15, r2", "vs_r15_r2"},
                           {{vs, r15, r3}, "vs, r15, r3", "vs_r15_r3"},
                           {{vs, r15, r4}, "vs, r15, r4", "vs_r15_r4"},
                           {{vs, r15, r5}, "vs, r15, r5", "vs_r15_r5"},
                           {{vs, r15, r6}, "vs, r15, r6", "vs_r15_r6"},
                           {{vs, r15, r7}, "vs, r15, r7", "vs_r15_r7"},
                           {{vs, r15, r8}, "vs, r15, r8", "vs_r15_r8"},
                           {{vs, r15, r9}, "vs, r15, r9", "vs_r15_r9"},
                           {{vs, r15, r10}, "vs, r15, r10", "vs_r15_r10"},
                           {{vs, r15, r11}, "vs, r15, r11", "vs_r15_r11"},
                           {{vs, r15, r12}, "vs, r15, r12", "vs_r15_r12"},
                           {{vs, r15, r13}, "vs, r15, r13", "vs_r15_r13"},
                           {{vs, r15, r14}, "vs, r15, r14", "vs_r15_r14"},
                           {{vs, r15, r15}, "vs, r15, r15", "vs_r15_r15"},
                           {{vc, r0, r15}, "vc, r0, r15", "vc_r0_r15"},
                           {{vc, r1, r15}, "vc, r1, r15", "vc_r1_r15"},
                           {{vc, r2, r15}, "vc, r2, r15", "vc_r2_r15"},
                           {{vc, r3, r15}, "vc, r3, r15", "vc_r3_r15"},
                           {{vc, r4, r15}, "vc, r4, r15", "vc_r4_r15"},
                           {{vc, r5, r15}, "vc, r5, r15", "vc_r5_r15"},
                           {{vc, r6, r15}, "vc, r6, r15", "vc_r6_r15"},
                           {{vc, r7, r15}, "vc, r7, r15", "vc_r7_r15"},
                           {{vc, r8, r15}, "vc, r8, r15", "vc_r8_r15"},
                           {{vc, r9, r15}, "vc, r9, r15", "vc_r9_r15"},
                           {{vc, r10, r15}, "vc, r10, r15", "vc_r10_r15"},
                           {{vc, r11, r15}, "vc, r11, r15", "vc_r11_r15"},
                           {{vc, r12, r15}, "vc, r12, r15", "vc_r12_r15"},
                           {{vc, r13, r15}, "vc, r13, r15", "vc_r13_r15"},
                           {{vc, r14, r15}, "vc, r14, r15", "vc_r14_r15"},
                           {{vc, r15, r0}, "vc, r15, r0", "vc_r15_r0"},
                           {{vc, r15, r1}, "vc, r15, r1", "vc_r15_r1"},
                           {{vc, r15, r2}, "vc, r15, r2", "vc_r15_r2"},
                           {{vc, r15, r3}, "vc, r15, r3", "vc_r15_r3"},
                           {{vc, r15, r4}, "vc, r15, r4", "vc_r15_r4"},
                           {{vc, r15, r5}, "vc, r15, r5", "vc_r15_r5"},
                           {{vc, r15, r6}, "vc, r15, r6", "vc_r15_r6"},
                           {{vc, r15, r7}, "vc, r15, r7", "vc_r15_r7"},
                           {{vc, r15, r8}, "vc, r15, r8", "vc_r15_r8"},
                           {{vc, r15, r9}, "vc, r15, r9", "vc_r15_r9"},
                           {{vc, r15, r10}, "vc, r15, r10", "vc_r15_r10"},
                           {{vc, r15, r11}, "vc, r15, r11", "vc_r15_r11"},
                           {{vc, r15, r12}, "vc, r15, r12", "vc_r15_r12"},
                           {{vc, r15, r13}, "vc, r15, r13", "vc_r15_r13"},
                           {{vc, r15, r14}, "vc, r15, r14", "vc_r15_r14"},
                           {{vc, r15, r15}, "vc, r15, r15", "vc_r15_r15"},
                           {{hi, r0, r15}, "hi, r0, r15", "hi_r0_r15"},
                           {{hi, r1, r15}, "hi, r1, r15", "hi_r1_r15"},
                           {{hi, r2, r15}, "hi, r2, r15", "hi_r2_r15"},
                           {{hi, r3, r15}, "hi, r3, r15", "hi_r3_r15"},
                           {{hi, r4, r15}, "hi, r4, r15", "hi_r4_r15"},
                           {{hi, r5, r15}, "hi, r5, r15", "hi_r5_r15"},
                           {{hi, r6, r15}, "hi, r6, r15", "hi_r6_r15"},
                           {{hi, r7, r15}, "hi, r7, r15", "hi_r7_r15"},
                           {{hi, r8, r15}, "hi, r8, r15", "hi_r8_r15"},
                           {{hi, r9, r15}, "hi, r9, r15", "hi_r9_r15"},
                           {{hi, r10, r15}, "hi, r10, r15", "hi_r10_r15"},
                           {{hi, r11, r15}, "hi, r11, r15", "hi_r11_r15"},
                           {{hi, r12, r15}, "hi, r12, r15", "hi_r12_r15"},
                           {{hi, r13, r15}, "hi, r13, r15", "hi_r13_r15"},
                           {{hi, r14, r15}, "hi, r14, r15", "hi_r14_r15"},
                           {{hi, r15, r0}, "hi, r15, r0", "hi_r15_r0"},
                           {{hi, r15, r1}, "hi, r15, r1", "hi_r15_r1"},
                           {{hi, r15, r2}, "hi, r15, r2", "hi_r15_r2"},
                           {{hi, r15, r3}, "hi, r15, r3", "hi_r15_r3"},
                           {{hi, r15, r4}, "hi, r15, r4", "hi_r15_r4"},
                           {{hi, r15, r5}, "hi, r15, r5", "hi_r15_r5"},
                           {{hi, r15, r6}, "hi, r15, r6", "hi_r15_r6"},
                           {{hi, r15, r7}, "hi, r15, r7", "hi_r15_r7"},
                           {{hi, r15, r8}, "hi, r15, r8", "hi_r15_r8"},
                           {{hi, r15, r9}, "hi, r15, r9", "hi_r15_r9"},
                           {{hi, r15, r10}, "hi, r15, r10", "hi_r15_r10"},
                           {{hi, r15, r11}, "hi, r15, r11", "hi_r15_r11"},
                           {{hi, r15, r12}, "hi, r15, r12", "hi_r15_r12"},
                           {{hi, r15, r13}, "hi, r15, r13", "hi_r15_r13"},
                           {{hi, r15, r14}, "hi, r15, r14", "hi_r15_r14"},
                           {{hi, r15, r15}, "hi, r15, r15", "hi_r15_r15"},
                           {{ls, r0, r15}, "ls, r0, r15", "ls_r0_r15"},
                           {{ls, r1, r15}, "ls, r1, r15", "ls_r1_r15"},
                           {{ls, r2, r15}, "ls, r2, r15", "ls_r2_r15"},
                           {{ls, r3, r15}, "ls, r3, r15", "ls_r3_r15"},
                           {{ls, r4, r15}, "ls, r4, r15", "ls_r4_r15"},
                           {{ls, r5, r15}, "ls, r5, r15", "ls_r5_r15"},
                           {{ls, r6, r15}, "ls, r6, r15", "ls_r6_r15"},
                           {{ls, r7, r15}, "ls, r7, r15", "ls_r7_r15"},
                           {{ls, r8, r15}, "ls, r8, r15", "ls_r8_r15"},
                           {{ls, r9, r15}, "ls, r9, r15", "ls_r9_r15"},
                           {{ls, r10, r15}, "ls, r10, r15", "ls_r10_r15"},
                           {{ls, r11, r15}, "ls, r11, r15", "ls_r11_r15"},
                           {{ls, r12, r15}, "ls, r12, r15", "ls_r12_r15"},
                           {{ls, r13, r15}, "ls, r13, r15", "ls_r13_r15"},
                           {{ls, r14, r15}, "ls, r14, r15", "ls_r14_r15"},
                           {{ls, r15, r0}, "ls, r15, r0", "ls_r15_r0"},
                           {{ls, r15, r1}, "ls, r15, r1", "ls_r15_r1"},
                           {{ls, r15, r2}, "ls, r15, r2", "ls_r15_r2"},
                           {{ls, r15, r3}, "ls, r15, r3", "ls_r15_r3"},
                           {{ls, r15, r4}, "ls, r15, r4", "ls_r15_r4"},
                           {{ls, r15, r5}, "ls, r15, r5", "ls_r15_r5"},
                           {{ls, r15, r6}, "ls, r15, r6", "ls_r15_r6"},
                           {{ls, r15, r7}, "ls, r15, r7", "ls_r15_r7"},
                           {{ls, r15, r8}, "ls, r15, r8", "ls_r15_r8"},
                           {{ls, r15, r9}, "ls, r15, r9", "ls_r15_r9"},
                           {{ls, r15, r10}, "ls, r15, r10", "ls_r15_r10"},
                           {{ls, r15, r11}, "ls, r15, r11", "ls_r15_r11"},
                           {{ls, r15, r12}, "ls, r15, r12", "ls_r15_r12"},
                           {{ls, r15, r13}, "ls, r15, r13", "ls_r15_r13"},
                           {{ls, r15, r14}, "ls, r15, r14", "ls_r15_r14"},
                           {{ls, r15, r15}, "ls, r15, r15", "ls_r15_r15"},
                           {{ge, r0, r15}, "ge, r0, r15", "ge_r0_r15"},
                           {{ge, r1, r15}, "ge, r1, r15", "ge_r1_r15"},
                           {{ge, r2, r15}, "ge, r2, r15", "ge_r2_r15"},
                           {{ge, r3, r15}, "ge, r3, r15", "ge_r3_r15"},
                           {{ge, r4, r15}, "ge, r4, r15", "ge_r4_r15"},
                           {{ge, r5, r15}, "ge, r5, r15", "ge_r5_r15"},
                           {{ge, r6, r15}, "ge, r6, r15", "ge_r6_r15"},
                           {{ge, r7, r15}, "ge, r7, r15", "ge_r7_r15"},
                           {{ge, r8, r15}, "ge, r8, r15", "ge_r8_r15"},
                           {{ge, r9, r15}, "ge, r9, r15", "ge_r9_r15"},
                           {{ge, r10, r15}, "ge, r10, r15", "ge_r10_r15"},
                           {{ge, r11, r15}, "ge, r11, r15", "ge_r11_r15"},
                           {{ge, r12, r15}, "ge, r12, r15", "ge_r12_r15"},
                           {{ge, r13, r15}, "ge, r13, r15", "ge_r13_r15"},
                           {{ge, r14, r15}, "ge, r14, r15", "ge_r14_r15"},
                           {{ge, r15, r0}, "ge, r15, r0", "ge_r15_r0"},
                           {{ge, r15, r1}, "ge, r15, r1", "ge_r15_r1"},
                           {{ge, r15, r2}, "ge, r15, r2", "ge_r15_r2"},
                           {{ge, r15, r3}, "ge, r15, r3", "ge_r15_r3"},
                           {{ge, r15, r4}, "ge, r15, r4", "ge_r15_r4"},
                           {{ge, r15, r5}, "ge, r15, r5", "ge_r15_r5"},
                           {{ge, r15, r6}, "ge, r15, r6", "ge_r15_r6"},
                           {{ge, r15, r7}, "ge, r15, r7", "ge_r15_r7"},
                           {{ge, r15, r8}, "ge, r15, r8", "ge_r15_r8"},
                           {{ge, r15, r9}, "ge, r15, r9", "ge_r15_r9"},
                           {{ge, r15, r10}, "ge, r15, r10", "ge_r15_r10"},
                           {{ge, r15, r11}, "ge, r15, r11", "ge_r15_r11"},
                           {{ge, r15, r12}, "ge, r15, r12", "ge_r15_r12"},
                           {{ge, r15, r13}, "ge, r15, r13", "ge_r15_r13"},
                           {{ge, r15, r14}, "ge, r15, r14", "ge_r15_r14"},
                           {{ge, r15, r15}, "ge, r15, r15", "ge_r15_r15"},
                           {{lt, r0, r15}, "lt, r0, r15", "lt_r0_r15"},
                           {{lt, r1, r15}, "lt, r1, r15", "lt_r1_r15"},
                           {{lt, r2, r15}, "lt, r2, r15", "lt_r2_r15"},
                           {{lt, r3, r15}, "lt, r3, r15", "lt_r3_r15"},
                           {{lt, r4, r15}, "lt, r4, r15", "lt_r4_r15"},
                           {{lt, r5, r15}, "lt, r5, r15", "lt_r5_r15"},
                           {{lt, r6, r15}, "lt, r6, r15", "lt_r6_r15"},
                           {{lt, r7, r15}, "lt, r7, r15", "lt_r7_r15"},
                           {{lt, r8, r15}, "lt, r8, r15", "lt_r8_r15"},
                           {{lt, r9, r15}, "lt, r9, r15", "lt_r9_r15"},
                           {{lt, r10, r15}, "lt, r10, r15", "lt_r10_r15"},
                           {{lt, r11, r15}, "lt, r11, r15", "lt_r11_r15"},
                           {{lt, r12, r15}, "lt, r12, r15", "lt_r12_r15"},
                           {{lt, r13, r15}, "lt, r13, r15", "lt_r13_r15"},
                           {{lt, r14, r15}, "lt, r14, r15", "lt_r14_r15"},
                           {{lt, r15, r0}, "lt, r15, r0", "lt_r15_r0"},
                           {{lt, r15, r1}, "lt, r15, r1", "lt_r15_r1"},
                           {{lt, r15, r2}, "lt, r15, r2", "lt_r15_r2"},
                           {{lt, r15, r3}, "lt, r15, r3", "lt_r15_r3"},
                           {{lt, r15, r4}, "lt, r15, r4", "lt_r15_r4"},
                           {{lt, r15, r5}, "lt, r15, r5", "lt_r15_r5"},
                           {{lt, r15, r6}, "lt, r15, r6", "lt_r15_r6"},
                           {{lt, r15, r7}, "lt, r15, r7", "lt_r15_r7"},
                           {{lt, r15, r8}, "lt, r15, r8", "lt_r15_r8"},
                           {{lt, r15, r9}, "lt, r15, r9", "lt_r15_r9"},
                           {{lt, r15, r10}, "lt, r15, r10", "lt_r15_r10"},
                           {{lt, r15, r11}, "lt, r15, r11", "lt_r15_r11"},
                           {{lt, r15, r12}, "lt, r15, r12", "lt_r15_r12"},
                           {{lt, r15, r13}, "lt, r15, r13", "lt_r15_r13"},
                           {{lt, r15, r14}, "lt, r15, r14", "lt_r15_r14"},
                           {{lt, r15, r15}, "lt, r15, r15", "lt_r15_r15"},
                           {{gt, r0, r15}, "gt, r0, r15", "gt_r0_r15"},
                           {{gt, r1, r15}, "gt, r1, r15", "gt_r1_r15"},
                           {{gt, r2, r15}, "gt, r2, r15", "gt_r2_r15"},
                           {{gt, r3, r15}, "gt, r3, r15", "gt_r3_r15"},
                           {{gt, r4, r15}, "gt, r4, r15", "gt_r4_r15"},
                           {{gt, r5, r15}, "gt, r5, r15", "gt_r5_r15"},
                           {{gt, r6, r15}, "gt, r6, r15", "gt_r6_r15"},
                           {{gt, r7, r15}, "gt, r7, r15", "gt_r7_r15"},
                           {{gt, r8, r15}, "gt, r8, r15", "gt_r8_r15"},
                           {{gt, r9, r15}, "gt, r9, r15", "gt_r9_r15"},
                           {{gt, r10, r15}, "gt, r10, r15", "gt_r10_r15"},
                           {{gt, r11, r15}, "gt, r11, r15", "gt_r11_r15"},
                           {{gt, r12, r15}, "gt, r12, r15", "gt_r12_r15"},
                           {{gt, r13, r15}, "gt, r13, r15", "gt_r13_r15"},
                           {{gt, r14, r15}, "gt, r14, r15", "gt_r14_r15"},
                           {{gt, r15, r0}, "gt, r15, r0", "gt_r15_r0"},
                           {{gt, r15, r1}, "gt, r15, r1", "gt_r15_r1"},
                           {{gt, r15, r2}, "gt, r15, r2", "gt_r15_r2"},
                           {{gt, r15, r3}, "gt, r15, r3", "gt_r15_r3"},
                           {{gt, r15, r4}, "gt, r15, r4", "gt_r15_r4"},
                           {{gt, r15, r5}, "gt, r15, r5", "gt_r15_r5"},
                           {{gt, r15, r6}, "gt, r15, r6", "gt_r15_r6"},
                           {{gt, r15, r7}, "gt, r15, r7", "gt_r15_r7"},
                           {{gt, r15, r8}, "gt, r15, r8", "gt_r15_r8"},
                           {{gt, r15, r9}, "gt, r15, r9", "gt_r15_r9"},
                           {{gt, r15, r10}, "gt, r15, r10", "gt_r15_r10"},
                           {{gt, r15, r11}, "gt, r15, r11", "gt_r15_r11"},
                           {{gt, r15, r12}, "gt, r15, r12", "gt_r15_r12"},
                           {{gt, r15, r13}, "gt, r15, r13", "gt_r15_r13"},
                           {{gt, r15, r14}, "gt, r15, r14", "gt_r15_r14"},
                           {{gt, r15, r15}, "gt, r15, r15", "gt_r15_r15"},
                           {{le, r0, r15}, "le, r0, r15", "le_r0_r15"},
                           {{le, r1, r15}, "le, r1, r15", "le_r1_r15"},
                           {{le, r2, r15}, "le, r2, r15", "le_r2_r15"},
                           {{le, r3, r15}, "le, r3, r15", "le_r3_r15"},
                           {{le, r4, r15}, "le, r4, r15", "le_r4_r15"},
                           {{le, r5, r15}, "le, r5, r15", "le_r5_r15"},
                           {{le, r6, r15}, "le, r6, r15", "le_r6_r15"},
                           {{le, r7, r15}, "le, r7, r15", "le_r7_r15"},
                           {{le, r8, r15}, "le, r8, r15", "le_r8_r15"},
                           {{le, r9, r15}, "le, r9, r15", "le_r9_r15"},
                           {{le, r10, r15}, "le, r10, r15", "le_r10_r15"},
                           {{le, r11, r15}, "le, r11, r15", "le_r11_r15"},
                           {{le, r12, r15}, "le, r12, r15", "le_r12_r15"},
                           {{le, r13, r15}, "le, r13, r15", "le_r13_r15"},
                           {{le, r14, r15}, "le, r14, r15", "le_r14_r15"},
                           {{le, r15, r0}, "le, r15, r0", "le_r15_r0"},
                           {{le, r15, r1}, "le, r15, r1", "le_r15_r1"},
                           {{le, r15, r2}, "le, r15, r2", "le_r15_r2"},
                           {{le, r15, r3}, "le, r15, r3", "le_r15_r3"},
                           {{le, r15, r4}, "le, r15, r4", "le_r15_r4"},
                           {{le, r15, r5}, "le, r15, r5", "le_r15_r5"},
                           {{le, r15, r6}, "le, r15, r6", "le_r15_r6"},
                           {{le, r15, r7}, "le, r15, r7", "le_r15_r7"},
                           {{le, r15, r8}, "le, r15, r8", "le_r15_r8"},
                           {{le, r15, r9}, "le, r15, r9", "le_r15_r9"},
                           {{le, r15, r10}, "le, r15, r10", "le_r15_r10"},
                           {{le, r15, r11}, "le, r15, r11", "le_r15_r11"},
                           {{le, r15, r12}, "le, r15, r12", "le_r15_r12"},
                           {{le, r15, r13}, "le, r15, r13", "le_r15_r13"},
                           {{le, r15, r14}, "le, r15, r14", "le_r15_r14"},
                           {{le, r15, r15}, "le, r15, r15", "le_r15_r15"},
                           {{al, r0, r15}, "al, r0, r15", "al_r0_r15"},
                           {{al, r1, r15}, "al, r1, r15", "al_r1_r15"},
                           {{al, r2, r15}, "al, r2, r15", "al_r2_r15"},
                           {{al, r3, r15}, "al, r3, r15", "al_r3_r15"},
                           {{al, r4, r15}, "al, r4, r15", "al_r4_r15"},
                           {{al, r5, r15}, "al, r5, r15", "al_r5_r15"},
                           {{al, r6, r15}, "al, r6, r15", "al_r6_r15"},
                           {{al, r7, r15}, "al, r7, r15", "al_r7_r15"},
                           {{al, r8, r15}, "al, r8, r15", "al_r8_r15"},
                           {{al, r9, r15}, "al, r9, r15", "al_r9_r15"},
                           {{al, r10, r15}, "al, r10, r15", "al_r10_r15"},
                           {{al, r11, r15}, "al, r11, r15", "al_r11_r15"},
                           {{al, r12, r15}, "al, r12, r15", "al_r12_r15"},
                           {{al, r13, r15}, "al, r13, r15", "al_r13_r15"},
                           {{al, r14, r15}, "al, r14, r15", "al_r14_r15"},
                           {{al, r15, r0}, "al, r15, r0", "al_r15_r0"},
                           {{al, r15, r1}, "al, r15, r1", "al_r15_r1"},
                           {{al, r15, r2}, "al, r15, r2", "al_r15_r2"},
                           {{al, r15, r3}, "al, r15, r3", "al_r15_r3"},
                           {{al, r15, r4}, "al, r15, r4", "al_r15_r4"},
                           {{al, r15, r5}, "al, r15, r5", "al_r15_r5"},
                           {{al, r15, r6}, "al, r15, r6", "al_r15_r6"},
                           {{al, r15, r7}, "al, r15, r7", "al_r15_r7"},
                           {{al, r15, r8}, "al, r15, r8", "al_r15_r8"},
                           {{al, r15, r9}, "al, r15, r9", "al_r15_r9"},
                           {{al, r15, r10}, "al, r15, r10", "al_r15_r10"},
                           {{al, r15, r11}, "al, r15, r11", "al_r15_r11"},
                           {{al, r15, r12}, "al, r15, r12", "al_r15_r12"},
                           {{al, r15, r13}, "al, r15, r13", "al_r15_r13"},
                           {{al, r15, r14}, "al, r15, r14", "al_r15_r14"},
                           {{al, r15, r15}, "al, r15, r15", "al_r15_r15"}};

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

  masm.UseA32();

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
  Test test_##mnemonic("AARCH32_MACRO_ASSEMBLER_COND_RD_RN_PC_" #mnemonic,     \
                       &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
