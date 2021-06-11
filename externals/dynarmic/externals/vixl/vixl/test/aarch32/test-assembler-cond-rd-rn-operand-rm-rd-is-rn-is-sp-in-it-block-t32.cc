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
    {{{eq, r13, r13, r0}, true, eq, "eq r13 r13 r0", "eq_r13_r13_r0"},
     {{eq, r13, r13, r1}, true, eq, "eq r13 r13 r1", "eq_r13_r13_r1"},
     {{eq, r13, r13, r2}, true, eq, "eq r13 r13 r2", "eq_r13_r13_r2"},
     {{eq, r13, r13, r3}, true, eq, "eq r13 r13 r3", "eq_r13_r13_r3"},
     {{eq, r13, r13, r4}, true, eq, "eq r13 r13 r4", "eq_r13_r13_r4"},
     {{eq, r13, r13, r5}, true, eq, "eq r13 r13 r5", "eq_r13_r13_r5"},
     {{eq, r13, r13, r6}, true, eq, "eq r13 r13 r6", "eq_r13_r13_r6"},
     {{eq, r13, r13, r7}, true, eq, "eq r13 r13 r7", "eq_r13_r13_r7"},
     {{eq, r13, r13, r8}, true, eq, "eq r13 r13 r8", "eq_r13_r13_r8"},
     {{eq, r13, r13, r9}, true, eq, "eq r13 r13 r9", "eq_r13_r13_r9"},
     {{eq, r13, r13, r10}, true, eq, "eq r13 r13 r10", "eq_r13_r13_r10"},
     {{eq, r13, r13, r11}, true, eq, "eq r13 r13 r11", "eq_r13_r13_r11"},
     {{eq, r13, r13, r12}, true, eq, "eq r13 r13 r12", "eq_r13_r13_r12"},
     {{eq, r13, r13, r13}, true, eq, "eq r13 r13 r13", "eq_r13_r13_r13"},
     {{eq, r13, r13, r14}, true, eq, "eq r13 r13 r14", "eq_r13_r13_r14"},
     {{ne, r13, r13, r0}, true, ne, "ne r13 r13 r0", "ne_r13_r13_r0"},
     {{ne, r13, r13, r1}, true, ne, "ne r13 r13 r1", "ne_r13_r13_r1"},
     {{ne, r13, r13, r2}, true, ne, "ne r13 r13 r2", "ne_r13_r13_r2"},
     {{ne, r13, r13, r3}, true, ne, "ne r13 r13 r3", "ne_r13_r13_r3"},
     {{ne, r13, r13, r4}, true, ne, "ne r13 r13 r4", "ne_r13_r13_r4"},
     {{ne, r13, r13, r5}, true, ne, "ne r13 r13 r5", "ne_r13_r13_r5"},
     {{ne, r13, r13, r6}, true, ne, "ne r13 r13 r6", "ne_r13_r13_r6"},
     {{ne, r13, r13, r7}, true, ne, "ne r13 r13 r7", "ne_r13_r13_r7"},
     {{ne, r13, r13, r8}, true, ne, "ne r13 r13 r8", "ne_r13_r13_r8"},
     {{ne, r13, r13, r9}, true, ne, "ne r13 r13 r9", "ne_r13_r13_r9"},
     {{ne, r13, r13, r10}, true, ne, "ne r13 r13 r10", "ne_r13_r13_r10"},
     {{ne, r13, r13, r11}, true, ne, "ne r13 r13 r11", "ne_r13_r13_r11"},
     {{ne, r13, r13, r12}, true, ne, "ne r13 r13 r12", "ne_r13_r13_r12"},
     {{ne, r13, r13, r13}, true, ne, "ne r13 r13 r13", "ne_r13_r13_r13"},
     {{ne, r13, r13, r14}, true, ne, "ne r13 r13 r14", "ne_r13_r13_r14"},
     {{cs, r13, r13, r0}, true, cs, "cs r13 r13 r0", "cs_r13_r13_r0"},
     {{cs, r13, r13, r1}, true, cs, "cs r13 r13 r1", "cs_r13_r13_r1"},
     {{cs, r13, r13, r2}, true, cs, "cs r13 r13 r2", "cs_r13_r13_r2"},
     {{cs, r13, r13, r3}, true, cs, "cs r13 r13 r3", "cs_r13_r13_r3"},
     {{cs, r13, r13, r4}, true, cs, "cs r13 r13 r4", "cs_r13_r13_r4"},
     {{cs, r13, r13, r5}, true, cs, "cs r13 r13 r5", "cs_r13_r13_r5"},
     {{cs, r13, r13, r6}, true, cs, "cs r13 r13 r6", "cs_r13_r13_r6"},
     {{cs, r13, r13, r7}, true, cs, "cs r13 r13 r7", "cs_r13_r13_r7"},
     {{cs, r13, r13, r8}, true, cs, "cs r13 r13 r8", "cs_r13_r13_r8"},
     {{cs, r13, r13, r9}, true, cs, "cs r13 r13 r9", "cs_r13_r13_r9"},
     {{cs, r13, r13, r10}, true, cs, "cs r13 r13 r10", "cs_r13_r13_r10"},
     {{cs, r13, r13, r11}, true, cs, "cs r13 r13 r11", "cs_r13_r13_r11"},
     {{cs, r13, r13, r12}, true, cs, "cs r13 r13 r12", "cs_r13_r13_r12"},
     {{cs, r13, r13, r13}, true, cs, "cs r13 r13 r13", "cs_r13_r13_r13"},
     {{cs, r13, r13, r14}, true, cs, "cs r13 r13 r14", "cs_r13_r13_r14"},
     {{cc, r13, r13, r0}, true, cc, "cc r13 r13 r0", "cc_r13_r13_r0"},
     {{cc, r13, r13, r1}, true, cc, "cc r13 r13 r1", "cc_r13_r13_r1"},
     {{cc, r13, r13, r2}, true, cc, "cc r13 r13 r2", "cc_r13_r13_r2"},
     {{cc, r13, r13, r3}, true, cc, "cc r13 r13 r3", "cc_r13_r13_r3"},
     {{cc, r13, r13, r4}, true, cc, "cc r13 r13 r4", "cc_r13_r13_r4"},
     {{cc, r13, r13, r5}, true, cc, "cc r13 r13 r5", "cc_r13_r13_r5"},
     {{cc, r13, r13, r6}, true, cc, "cc r13 r13 r6", "cc_r13_r13_r6"},
     {{cc, r13, r13, r7}, true, cc, "cc r13 r13 r7", "cc_r13_r13_r7"},
     {{cc, r13, r13, r8}, true, cc, "cc r13 r13 r8", "cc_r13_r13_r8"},
     {{cc, r13, r13, r9}, true, cc, "cc r13 r13 r9", "cc_r13_r13_r9"},
     {{cc, r13, r13, r10}, true, cc, "cc r13 r13 r10", "cc_r13_r13_r10"},
     {{cc, r13, r13, r11}, true, cc, "cc r13 r13 r11", "cc_r13_r13_r11"},
     {{cc, r13, r13, r12}, true, cc, "cc r13 r13 r12", "cc_r13_r13_r12"},
     {{cc, r13, r13, r13}, true, cc, "cc r13 r13 r13", "cc_r13_r13_r13"},
     {{cc, r13, r13, r14}, true, cc, "cc r13 r13 r14", "cc_r13_r13_r14"},
     {{mi, r13, r13, r0}, true, mi, "mi r13 r13 r0", "mi_r13_r13_r0"},
     {{mi, r13, r13, r1}, true, mi, "mi r13 r13 r1", "mi_r13_r13_r1"},
     {{mi, r13, r13, r2}, true, mi, "mi r13 r13 r2", "mi_r13_r13_r2"},
     {{mi, r13, r13, r3}, true, mi, "mi r13 r13 r3", "mi_r13_r13_r3"},
     {{mi, r13, r13, r4}, true, mi, "mi r13 r13 r4", "mi_r13_r13_r4"},
     {{mi, r13, r13, r5}, true, mi, "mi r13 r13 r5", "mi_r13_r13_r5"},
     {{mi, r13, r13, r6}, true, mi, "mi r13 r13 r6", "mi_r13_r13_r6"},
     {{mi, r13, r13, r7}, true, mi, "mi r13 r13 r7", "mi_r13_r13_r7"},
     {{mi, r13, r13, r8}, true, mi, "mi r13 r13 r8", "mi_r13_r13_r8"},
     {{mi, r13, r13, r9}, true, mi, "mi r13 r13 r9", "mi_r13_r13_r9"},
     {{mi, r13, r13, r10}, true, mi, "mi r13 r13 r10", "mi_r13_r13_r10"},
     {{mi, r13, r13, r11}, true, mi, "mi r13 r13 r11", "mi_r13_r13_r11"},
     {{mi, r13, r13, r12}, true, mi, "mi r13 r13 r12", "mi_r13_r13_r12"},
     {{mi, r13, r13, r13}, true, mi, "mi r13 r13 r13", "mi_r13_r13_r13"},
     {{mi, r13, r13, r14}, true, mi, "mi r13 r13 r14", "mi_r13_r13_r14"},
     {{pl, r13, r13, r0}, true, pl, "pl r13 r13 r0", "pl_r13_r13_r0"},
     {{pl, r13, r13, r1}, true, pl, "pl r13 r13 r1", "pl_r13_r13_r1"},
     {{pl, r13, r13, r2}, true, pl, "pl r13 r13 r2", "pl_r13_r13_r2"},
     {{pl, r13, r13, r3}, true, pl, "pl r13 r13 r3", "pl_r13_r13_r3"},
     {{pl, r13, r13, r4}, true, pl, "pl r13 r13 r4", "pl_r13_r13_r4"},
     {{pl, r13, r13, r5}, true, pl, "pl r13 r13 r5", "pl_r13_r13_r5"},
     {{pl, r13, r13, r6}, true, pl, "pl r13 r13 r6", "pl_r13_r13_r6"},
     {{pl, r13, r13, r7}, true, pl, "pl r13 r13 r7", "pl_r13_r13_r7"},
     {{pl, r13, r13, r8}, true, pl, "pl r13 r13 r8", "pl_r13_r13_r8"},
     {{pl, r13, r13, r9}, true, pl, "pl r13 r13 r9", "pl_r13_r13_r9"},
     {{pl, r13, r13, r10}, true, pl, "pl r13 r13 r10", "pl_r13_r13_r10"},
     {{pl, r13, r13, r11}, true, pl, "pl r13 r13 r11", "pl_r13_r13_r11"},
     {{pl, r13, r13, r12}, true, pl, "pl r13 r13 r12", "pl_r13_r13_r12"},
     {{pl, r13, r13, r13}, true, pl, "pl r13 r13 r13", "pl_r13_r13_r13"},
     {{pl, r13, r13, r14}, true, pl, "pl r13 r13 r14", "pl_r13_r13_r14"},
     {{vs, r13, r13, r0}, true, vs, "vs r13 r13 r0", "vs_r13_r13_r0"},
     {{vs, r13, r13, r1}, true, vs, "vs r13 r13 r1", "vs_r13_r13_r1"},
     {{vs, r13, r13, r2}, true, vs, "vs r13 r13 r2", "vs_r13_r13_r2"},
     {{vs, r13, r13, r3}, true, vs, "vs r13 r13 r3", "vs_r13_r13_r3"},
     {{vs, r13, r13, r4}, true, vs, "vs r13 r13 r4", "vs_r13_r13_r4"},
     {{vs, r13, r13, r5}, true, vs, "vs r13 r13 r5", "vs_r13_r13_r5"},
     {{vs, r13, r13, r6}, true, vs, "vs r13 r13 r6", "vs_r13_r13_r6"},
     {{vs, r13, r13, r7}, true, vs, "vs r13 r13 r7", "vs_r13_r13_r7"},
     {{vs, r13, r13, r8}, true, vs, "vs r13 r13 r8", "vs_r13_r13_r8"},
     {{vs, r13, r13, r9}, true, vs, "vs r13 r13 r9", "vs_r13_r13_r9"},
     {{vs, r13, r13, r10}, true, vs, "vs r13 r13 r10", "vs_r13_r13_r10"},
     {{vs, r13, r13, r11}, true, vs, "vs r13 r13 r11", "vs_r13_r13_r11"},
     {{vs, r13, r13, r12}, true, vs, "vs r13 r13 r12", "vs_r13_r13_r12"},
     {{vs, r13, r13, r13}, true, vs, "vs r13 r13 r13", "vs_r13_r13_r13"},
     {{vs, r13, r13, r14}, true, vs, "vs r13 r13 r14", "vs_r13_r13_r14"},
     {{vc, r13, r13, r0}, true, vc, "vc r13 r13 r0", "vc_r13_r13_r0"},
     {{vc, r13, r13, r1}, true, vc, "vc r13 r13 r1", "vc_r13_r13_r1"},
     {{vc, r13, r13, r2}, true, vc, "vc r13 r13 r2", "vc_r13_r13_r2"},
     {{vc, r13, r13, r3}, true, vc, "vc r13 r13 r3", "vc_r13_r13_r3"},
     {{vc, r13, r13, r4}, true, vc, "vc r13 r13 r4", "vc_r13_r13_r4"},
     {{vc, r13, r13, r5}, true, vc, "vc r13 r13 r5", "vc_r13_r13_r5"},
     {{vc, r13, r13, r6}, true, vc, "vc r13 r13 r6", "vc_r13_r13_r6"},
     {{vc, r13, r13, r7}, true, vc, "vc r13 r13 r7", "vc_r13_r13_r7"},
     {{vc, r13, r13, r8}, true, vc, "vc r13 r13 r8", "vc_r13_r13_r8"},
     {{vc, r13, r13, r9}, true, vc, "vc r13 r13 r9", "vc_r13_r13_r9"},
     {{vc, r13, r13, r10}, true, vc, "vc r13 r13 r10", "vc_r13_r13_r10"},
     {{vc, r13, r13, r11}, true, vc, "vc r13 r13 r11", "vc_r13_r13_r11"},
     {{vc, r13, r13, r12}, true, vc, "vc r13 r13 r12", "vc_r13_r13_r12"},
     {{vc, r13, r13, r13}, true, vc, "vc r13 r13 r13", "vc_r13_r13_r13"},
     {{vc, r13, r13, r14}, true, vc, "vc r13 r13 r14", "vc_r13_r13_r14"},
     {{hi, r13, r13, r0}, true, hi, "hi r13 r13 r0", "hi_r13_r13_r0"},
     {{hi, r13, r13, r1}, true, hi, "hi r13 r13 r1", "hi_r13_r13_r1"},
     {{hi, r13, r13, r2}, true, hi, "hi r13 r13 r2", "hi_r13_r13_r2"},
     {{hi, r13, r13, r3}, true, hi, "hi r13 r13 r3", "hi_r13_r13_r3"},
     {{hi, r13, r13, r4}, true, hi, "hi r13 r13 r4", "hi_r13_r13_r4"},
     {{hi, r13, r13, r5}, true, hi, "hi r13 r13 r5", "hi_r13_r13_r5"},
     {{hi, r13, r13, r6}, true, hi, "hi r13 r13 r6", "hi_r13_r13_r6"},
     {{hi, r13, r13, r7}, true, hi, "hi r13 r13 r7", "hi_r13_r13_r7"},
     {{hi, r13, r13, r8}, true, hi, "hi r13 r13 r8", "hi_r13_r13_r8"},
     {{hi, r13, r13, r9}, true, hi, "hi r13 r13 r9", "hi_r13_r13_r9"},
     {{hi, r13, r13, r10}, true, hi, "hi r13 r13 r10", "hi_r13_r13_r10"},
     {{hi, r13, r13, r11}, true, hi, "hi r13 r13 r11", "hi_r13_r13_r11"},
     {{hi, r13, r13, r12}, true, hi, "hi r13 r13 r12", "hi_r13_r13_r12"},
     {{hi, r13, r13, r13}, true, hi, "hi r13 r13 r13", "hi_r13_r13_r13"},
     {{hi, r13, r13, r14}, true, hi, "hi r13 r13 r14", "hi_r13_r13_r14"},
     {{ls, r13, r13, r0}, true, ls, "ls r13 r13 r0", "ls_r13_r13_r0"},
     {{ls, r13, r13, r1}, true, ls, "ls r13 r13 r1", "ls_r13_r13_r1"},
     {{ls, r13, r13, r2}, true, ls, "ls r13 r13 r2", "ls_r13_r13_r2"},
     {{ls, r13, r13, r3}, true, ls, "ls r13 r13 r3", "ls_r13_r13_r3"},
     {{ls, r13, r13, r4}, true, ls, "ls r13 r13 r4", "ls_r13_r13_r4"},
     {{ls, r13, r13, r5}, true, ls, "ls r13 r13 r5", "ls_r13_r13_r5"},
     {{ls, r13, r13, r6}, true, ls, "ls r13 r13 r6", "ls_r13_r13_r6"},
     {{ls, r13, r13, r7}, true, ls, "ls r13 r13 r7", "ls_r13_r13_r7"},
     {{ls, r13, r13, r8}, true, ls, "ls r13 r13 r8", "ls_r13_r13_r8"},
     {{ls, r13, r13, r9}, true, ls, "ls r13 r13 r9", "ls_r13_r13_r9"},
     {{ls, r13, r13, r10}, true, ls, "ls r13 r13 r10", "ls_r13_r13_r10"},
     {{ls, r13, r13, r11}, true, ls, "ls r13 r13 r11", "ls_r13_r13_r11"},
     {{ls, r13, r13, r12}, true, ls, "ls r13 r13 r12", "ls_r13_r13_r12"},
     {{ls, r13, r13, r13}, true, ls, "ls r13 r13 r13", "ls_r13_r13_r13"},
     {{ls, r13, r13, r14}, true, ls, "ls r13 r13 r14", "ls_r13_r13_r14"},
     {{ge, r13, r13, r0}, true, ge, "ge r13 r13 r0", "ge_r13_r13_r0"},
     {{ge, r13, r13, r1}, true, ge, "ge r13 r13 r1", "ge_r13_r13_r1"},
     {{ge, r13, r13, r2}, true, ge, "ge r13 r13 r2", "ge_r13_r13_r2"},
     {{ge, r13, r13, r3}, true, ge, "ge r13 r13 r3", "ge_r13_r13_r3"},
     {{ge, r13, r13, r4}, true, ge, "ge r13 r13 r4", "ge_r13_r13_r4"},
     {{ge, r13, r13, r5}, true, ge, "ge r13 r13 r5", "ge_r13_r13_r5"},
     {{ge, r13, r13, r6}, true, ge, "ge r13 r13 r6", "ge_r13_r13_r6"},
     {{ge, r13, r13, r7}, true, ge, "ge r13 r13 r7", "ge_r13_r13_r7"},
     {{ge, r13, r13, r8}, true, ge, "ge r13 r13 r8", "ge_r13_r13_r8"},
     {{ge, r13, r13, r9}, true, ge, "ge r13 r13 r9", "ge_r13_r13_r9"},
     {{ge, r13, r13, r10}, true, ge, "ge r13 r13 r10", "ge_r13_r13_r10"},
     {{ge, r13, r13, r11}, true, ge, "ge r13 r13 r11", "ge_r13_r13_r11"},
     {{ge, r13, r13, r12}, true, ge, "ge r13 r13 r12", "ge_r13_r13_r12"},
     {{ge, r13, r13, r13}, true, ge, "ge r13 r13 r13", "ge_r13_r13_r13"},
     {{ge, r13, r13, r14}, true, ge, "ge r13 r13 r14", "ge_r13_r13_r14"},
     {{lt, r13, r13, r0}, true, lt, "lt r13 r13 r0", "lt_r13_r13_r0"},
     {{lt, r13, r13, r1}, true, lt, "lt r13 r13 r1", "lt_r13_r13_r1"},
     {{lt, r13, r13, r2}, true, lt, "lt r13 r13 r2", "lt_r13_r13_r2"},
     {{lt, r13, r13, r3}, true, lt, "lt r13 r13 r3", "lt_r13_r13_r3"},
     {{lt, r13, r13, r4}, true, lt, "lt r13 r13 r4", "lt_r13_r13_r4"},
     {{lt, r13, r13, r5}, true, lt, "lt r13 r13 r5", "lt_r13_r13_r5"},
     {{lt, r13, r13, r6}, true, lt, "lt r13 r13 r6", "lt_r13_r13_r6"},
     {{lt, r13, r13, r7}, true, lt, "lt r13 r13 r7", "lt_r13_r13_r7"},
     {{lt, r13, r13, r8}, true, lt, "lt r13 r13 r8", "lt_r13_r13_r8"},
     {{lt, r13, r13, r9}, true, lt, "lt r13 r13 r9", "lt_r13_r13_r9"},
     {{lt, r13, r13, r10}, true, lt, "lt r13 r13 r10", "lt_r13_r13_r10"},
     {{lt, r13, r13, r11}, true, lt, "lt r13 r13 r11", "lt_r13_r13_r11"},
     {{lt, r13, r13, r12}, true, lt, "lt r13 r13 r12", "lt_r13_r13_r12"},
     {{lt, r13, r13, r13}, true, lt, "lt r13 r13 r13", "lt_r13_r13_r13"},
     {{lt, r13, r13, r14}, true, lt, "lt r13 r13 r14", "lt_r13_r13_r14"},
     {{gt, r13, r13, r0}, true, gt, "gt r13 r13 r0", "gt_r13_r13_r0"},
     {{gt, r13, r13, r1}, true, gt, "gt r13 r13 r1", "gt_r13_r13_r1"},
     {{gt, r13, r13, r2}, true, gt, "gt r13 r13 r2", "gt_r13_r13_r2"},
     {{gt, r13, r13, r3}, true, gt, "gt r13 r13 r3", "gt_r13_r13_r3"},
     {{gt, r13, r13, r4}, true, gt, "gt r13 r13 r4", "gt_r13_r13_r4"},
     {{gt, r13, r13, r5}, true, gt, "gt r13 r13 r5", "gt_r13_r13_r5"},
     {{gt, r13, r13, r6}, true, gt, "gt r13 r13 r6", "gt_r13_r13_r6"},
     {{gt, r13, r13, r7}, true, gt, "gt r13 r13 r7", "gt_r13_r13_r7"},
     {{gt, r13, r13, r8}, true, gt, "gt r13 r13 r8", "gt_r13_r13_r8"},
     {{gt, r13, r13, r9}, true, gt, "gt r13 r13 r9", "gt_r13_r13_r9"},
     {{gt, r13, r13, r10}, true, gt, "gt r13 r13 r10", "gt_r13_r13_r10"},
     {{gt, r13, r13, r11}, true, gt, "gt r13 r13 r11", "gt_r13_r13_r11"},
     {{gt, r13, r13, r12}, true, gt, "gt r13 r13 r12", "gt_r13_r13_r12"},
     {{gt, r13, r13, r13}, true, gt, "gt r13 r13 r13", "gt_r13_r13_r13"},
     {{gt, r13, r13, r14}, true, gt, "gt r13 r13 r14", "gt_r13_r13_r14"},
     {{le, r13, r13, r0}, true, le, "le r13 r13 r0", "le_r13_r13_r0"},
     {{le, r13, r13, r1}, true, le, "le r13 r13 r1", "le_r13_r13_r1"},
     {{le, r13, r13, r2}, true, le, "le r13 r13 r2", "le_r13_r13_r2"},
     {{le, r13, r13, r3}, true, le, "le r13 r13 r3", "le_r13_r13_r3"},
     {{le, r13, r13, r4}, true, le, "le r13 r13 r4", "le_r13_r13_r4"},
     {{le, r13, r13, r5}, true, le, "le r13 r13 r5", "le_r13_r13_r5"},
     {{le, r13, r13, r6}, true, le, "le r13 r13 r6", "le_r13_r13_r6"},
     {{le, r13, r13, r7}, true, le, "le r13 r13 r7", "le_r13_r13_r7"},
     {{le, r13, r13, r8}, true, le, "le r13 r13 r8", "le_r13_r13_r8"},
     {{le, r13, r13, r9}, true, le, "le r13 r13 r9", "le_r13_r13_r9"},
     {{le, r13, r13, r10}, true, le, "le r13 r13 r10", "le_r13_r13_r10"},
     {{le, r13, r13, r11}, true, le, "le r13 r13 r11", "le_r13_r13_r11"},
     {{le, r13, r13, r12}, true, le, "le r13 r13 r12", "le_r13_r13_r12"},
     {{le, r13, r13, r13}, true, le, "le r13 r13 r13", "le_r13_r13_r13"},
     {{le, r13, r13, r14}, true, le, "le r13 r13 r14", "le_r13_r13_r14"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-rn-operand-rm-rd-is-rn-is-sp-in-it-block-add-t32.h"


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
      "AARCH32_ASSEMBLER_COND_RD_RN_OPERAND_RM_RD_IS_RN_IS_SP_IN_IT_"       \
      "BLOCK_" #mnemonic "_T32",                                            \
      &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
