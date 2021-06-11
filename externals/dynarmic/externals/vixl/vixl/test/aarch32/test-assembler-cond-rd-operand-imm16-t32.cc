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
  M(mov)                       \
  M(movt)                      \
  M(movw)


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
    {{{al, r0, 0x0000}, false, al, "al r0 0x0000", "al_r0_0x0000"},
     {{al, r0, 0x0001}, false, al, "al r0 0x0001", "al_r0_0x0001"},
     {{al, r0, 0x0002}, false, al, "al r0 0x0002", "al_r0_0x0002"},
     {{al, r0, 0x0020}, false, al, "al r0 0x0020", "al_r0_0x0020"},
     {{al, r0, 0x007d}, false, al, "al r0 0x007d", "al_r0_0x007d"},
     {{al, r0, 0x007e}, false, al, "al r0 0x007e", "al_r0_0x007e"},
     {{al, r0, 0x007f}, false, al, "al r0 0x007f", "al_r0_0x007f"},
     {{al, r0, 0x7ffd}, false, al, "al r0 0x7ffd", "al_r0_0x7ffd"},
     {{al, r0, 0x7ffe}, false, al, "al r0 0x7ffe", "al_r0_0x7ffe"},
     {{al, r0, 0x7fff}, false, al, "al r0 0x7fff", "al_r0_0x7fff"},
     {{al, r0, 0x3333}, false, al, "al r0 0x3333", "al_r0_0x3333"},
     {{al, r0, 0x5555}, false, al, "al r0 0x5555", "al_r0_0x5555"},
     {{al, r0, 0xaaaa}, false, al, "al r0 0xaaaa", "al_r0_0xaaaa"},
     {{al, r0, 0xcccc}, false, al, "al r0 0xcccc", "al_r0_0xcccc"},
     {{al, r0, 0x8000}, false, al, "al r0 0x8000", "al_r0_0x8000"},
     {{al, r0, 0x8001}, false, al, "al r0 0x8001", "al_r0_0x8001"},
     {{al, r0, 0x8002}, false, al, "al r0 0x8002", "al_r0_0x8002"},
     {{al, r0, 0x8003}, false, al, "al r0 0x8003", "al_r0_0x8003"},
     {{al, r0, 0xff80}, false, al, "al r0 0xff80", "al_r0_0xff80"},
     {{al, r0, 0xff81}, false, al, "al r0 0xff81", "al_r0_0xff81"},
     {{al, r0, 0xff82}, false, al, "al r0 0xff82", "al_r0_0xff82"},
     {{al, r0, 0xff83}, false, al, "al r0 0xff83", "al_r0_0xff83"},
     {{al, r0, 0xffe0}, false, al, "al r0 0xffe0", "al_r0_0xffe0"},
     {{al, r0, 0xfffd}, false, al, "al r0 0xfffd", "al_r0_0xfffd"},
     {{al, r0, 0xfffe}, false, al, "al r0 0xfffe", "al_r0_0xfffe"},
     {{al, r0, 0xffff}, false, al, "al r0 0xffff", "al_r0_0xffff"},
     {{al, r1, 0x0000}, false, al, "al r1 0x0000", "al_r1_0x0000"},
     {{al, r1, 0x0001}, false, al, "al r1 0x0001", "al_r1_0x0001"},
     {{al, r1, 0x0002}, false, al, "al r1 0x0002", "al_r1_0x0002"},
     {{al, r1, 0x0020}, false, al, "al r1 0x0020", "al_r1_0x0020"},
     {{al, r1, 0x007d}, false, al, "al r1 0x007d", "al_r1_0x007d"},
     {{al, r1, 0x007e}, false, al, "al r1 0x007e", "al_r1_0x007e"},
     {{al, r1, 0x007f}, false, al, "al r1 0x007f", "al_r1_0x007f"},
     {{al, r1, 0x7ffd}, false, al, "al r1 0x7ffd", "al_r1_0x7ffd"},
     {{al, r1, 0x7ffe}, false, al, "al r1 0x7ffe", "al_r1_0x7ffe"},
     {{al, r1, 0x7fff}, false, al, "al r1 0x7fff", "al_r1_0x7fff"},
     {{al, r1, 0x3333}, false, al, "al r1 0x3333", "al_r1_0x3333"},
     {{al, r1, 0x5555}, false, al, "al r1 0x5555", "al_r1_0x5555"},
     {{al, r1, 0xaaaa}, false, al, "al r1 0xaaaa", "al_r1_0xaaaa"},
     {{al, r1, 0xcccc}, false, al, "al r1 0xcccc", "al_r1_0xcccc"},
     {{al, r1, 0x8000}, false, al, "al r1 0x8000", "al_r1_0x8000"},
     {{al, r1, 0x8001}, false, al, "al r1 0x8001", "al_r1_0x8001"},
     {{al, r1, 0x8002}, false, al, "al r1 0x8002", "al_r1_0x8002"},
     {{al, r1, 0x8003}, false, al, "al r1 0x8003", "al_r1_0x8003"},
     {{al, r1, 0xff80}, false, al, "al r1 0xff80", "al_r1_0xff80"},
     {{al, r1, 0xff81}, false, al, "al r1 0xff81", "al_r1_0xff81"},
     {{al, r1, 0xff82}, false, al, "al r1 0xff82", "al_r1_0xff82"},
     {{al, r1, 0xff83}, false, al, "al r1 0xff83", "al_r1_0xff83"},
     {{al, r1, 0xffe0}, false, al, "al r1 0xffe0", "al_r1_0xffe0"},
     {{al, r1, 0xfffd}, false, al, "al r1 0xfffd", "al_r1_0xfffd"},
     {{al, r1, 0xfffe}, false, al, "al r1 0xfffe", "al_r1_0xfffe"},
     {{al, r1, 0xffff}, false, al, "al r1 0xffff", "al_r1_0xffff"},
     {{al, r2, 0x0000}, false, al, "al r2 0x0000", "al_r2_0x0000"},
     {{al, r2, 0x0001}, false, al, "al r2 0x0001", "al_r2_0x0001"},
     {{al, r2, 0x0002}, false, al, "al r2 0x0002", "al_r2_0x0002"},
     {{al, r2, 0x0020}, false, al, "al r2 0x0020", "al_r2_0x0020"},
     {{al, r2, 0x007d}, false, al, "al r2 0x007d", "al_r2_0x007d"},
     {{al, r2, 0x007e}, false, al, "al r2 0x007e", "al_r2_0x007e"},
     {{al, r2, 0x007f}, false, al, "al r2 0x007f", "al_r2_0x007f"},
     {{al, r2, 0x7ffd}, false, al, "al r2 0x7ffd", "al_r2_0x7ffd"},
     {{al, r2, 0x7ffe}, false, al, "al r2 0x7ffe", "al_r2_0x7ffe"},
     {{al, r2, 0x7fff}, false, al, "al r2 0x7fff", "al_r2_0x7fff"},
     {{al, r2, 0x3333}, false, al, "al r2 0x3333", "al_r2_0x3333"},
     {{al, r2, 0x5555}, false, al, "al r2 0x5555", "al_r2_0x5555"},
     {{al, r2, 0xaaaa}, false, al, "al r2 0xaaaa", "al_r2_0xaaaa"},
     {{al, r2, 0xcccc}, false, al, "al r2 0xcccc", "al_r2_0xcccc"},
     {{al, r2, 0x8000}, false, al, "al r2 0x8000", "al_r2_0x8000"},
     {{al, r2, 0x8001}, false, al, "al r2 0x8001", "al_r2_0x8001"},
     {{al, r2, 0x8002}, false, al, "al r2 0x8002", "al_r2_0x8002"},
     {{al, r2, 0x8003}, false, al, "al r2 0x8003", "al_r2_0x8003"},
     {{al, r2, 0xff80}, false, al, "al r2 0xff80", "al_r2_0xff80"},
     {{al, r2, 0xff81}, false, al, "al r2 0xff81", "al_r2_0xff81"},
     {{al, r2, 0xff82}, false, al, "al r2 0xff82", "al_r2_0xff82"},
     {{al, r2, 0xff83}, false, al, "al r2 0xff83", "al_r2_0xff83"},
     {{al, r2, 0xffe0}, false, al, "al r2 0xffe0", "al_r2_0xffe0"},
     {{al, r2, 0xfffd}, false, al, "al r2 0xfffd", "al_r2_0xfffd"},
     {{al, r2, 0xfffe}, false, al, "al r2 0xfffe", "al_r2_0xfffe"},
     {{al, r2, 0xffff}, false, al, "al r2 0xffff", "al_r2_0xffff"},
     {{al, r3, 0x0000}, false, al, "al r3 0x0000", "al_r3_0x0000"},
     {{al, r3, 0x0001}, false, al, "al r3 0x0001", "al_r3_0x0001"},
     {{al, r3, 0x0002}, false, al, "al r3 0x0002", "al_r3_0x0002"},
     {{al, r3, 0x0020}, false, al, "al r3 0x0020", "al_r3_0x0020"},
     {{al, r3, 0x007d}, false, al, "al r3 0x007d", "al_r3_0x007d"},
     {{al, r3, 0x007e}, false, al, "al r3 0x007e", "al_r3_0x007e"},
     {{al, r3, 0x007f}, false, al, "al r3 0x007f", "al_r3_0x007f"},
     {{al, r3, 0x7ffd}, false, al, "al r3 0x7ffd", "al_r3_0x7ffd"},
     {{al, r3, 0x7ffe}, false, al, "al r3 0x7ffe", "al_r3_0x7ffe"},
     {{al, r3, 0x7fff}, false, al, "al r3 0x7fff", "al_r3_0x7fff"},
     {{al, r3, 0x3333}, false, al, "al r3 0x3333", "al_r3_0x3333"},
     {{al, r3, 0x5555}, false, al, "al r3 0x5555", "al_r3_0x5555"},
     {{al, r3, 0xaaaa}, false, al, "al r3 0xaaaa", "al_r3_0xaaaa"},
     {{al, r3, 0xcccc}, false, al, "al r3 0xcccc", "al_r3_0xcccc"},
     {{al, r3, 0x8000}, false, al, "al r3 0x8000", "al_r3_0x8000"},
     {{al, r3, 0x8001}, false, al, "al r3 0x8001", "al_r3_0x8001"},
     {{al, r3, 0x8002}, false, al, "al r3 0x8002", "al_r3_0x8002"},
     {{al, r3, 0x8003}, false, al, "al r3 0x8003", "al_r3_0x8003"},
     {{al, r3, 0xff80}, false, al, "al r3 0xff80", "al_r3_0xff80"},
     {{al, r3, 0xff81}, false, al, "al r3 0xff81", "al_r3_0xff81"},
     {{al, r3, 0xff82}, false, al, "al r3 0xff82", "al_r3_0xff82"},
     {{al, r3, 0xff83}, false, al, "al r3 0xff83", "al_r3_0xff83"},
     {{al, r3, 0xffe0}, false, al, "al r3 0xffe0", "al_r3_0xffe0"},
     {{al, r3, 0xfffd}, false, al, "al r3 0xfffd", "al_r3_0xfffd"},
     {{al, r3, 0xfffe}, false, al, "al r3 0xfffe", "al_r3_0xfffe"},
     {{al, r3, 0xffff}, false, al, "al r3 0xffff", "al_r3_0xffff"},
     {{al, r4, 0x0000}, false, al, "al r4 0x0000", "al_r4_0x0000"},
     {{al, r4, 0x0001}, false, al, "al r4 0x0001", "al_r4_0x0001"},
     {{al, r4, 0x0002}, false, al, "al r4 0x0002", "al_r4_0x0002"},
     {{al, r4, 0x0020}, false, al, "al r4 0x0020", "al_r4_0x0020"},
     {{al, r4, 0x007d}, false, al, "al r4 0x007d", "al_r4_0x007d"},
     {{al, r4, 0x007e}, false, al, "al r4 0x007e", "al_r4_0x007e"},
     {{al, r4, 0x007f}, false, al, "al r4 0x007f", "al_r4_0x007f"},
     {{al, r4, 0x7ffd}, false, al, "al r4 0x7ffd", "al_r4_0x7ffd"},
     {{al, r4, 0x7ffe}, false, al, "al r4 0x7ffe", "al_r4_0x7ffe"},
     {{al, r4, 0x7fff}, false, al, "al r4 0x7fff", "al_r4_0x7fff"},
     {{al, r4, 0x3333}, false, al, "al r4 0x3333", "al_r4_0x3333"},
     {{al, r4, 0x5555}, false, al, "al r4 0x5555", "al_r4_0x5555"},
     {{al, r4, 0xaaaa}, false, al, "al r4 0xaaaa", "al_r4_0xaaaa"},
     {{al, r4, 0xcccc}, false, al, "al r4 0xcccc", "al_r4_0xcccc"},
     {{al, r4, 0x8000}, false, al, "al r4 0x8000", "al_r4_0x8000"},
     {{al, r4, 0x8001}, false, al, "al r4 0x8001", "al_r4_0x8001"},
     {{al, r4, 0x8002}, false, al, "al r4 0x8002", "al_r4_0x8002"},
     {{al, r4, 0x8003}, false, al, "al r4 0x8003", "al_r4_0x8003"},
     {{al, r4, 0xff80}, false, al, "al r4 0xff80", "al_r4_0xff80"},
     {{al, r4, 0xff81}, false, al, "al r4 0xff81", "al_r4_0xff81"},
     {{al, r4, 0xff82}, false, al, "al r4 0xff82", "al_r4_0xff82"},
     {{al, r4, 0xff83}, false, al, "al r4 0xff83", "al_r4_0xff83"},
     {{al, r4, 0xffe0}, false, al, "al r4 0xffe0", "al_r4_0xffe0"},
     {{al, r4, 0xfffd}, false, al, "al r4 0xfffd", "al_r4_0xfffd"},
     {{al, r4, 0xfffe}, false, al, "al r4 0xfffe", "al_r4_0xfffe"},
     {{al, r4, 0xffff}, false, al, "al r4 0xffff", "al_r4_0xffff"},
     {{al, r5, 0x0000}, false, al, "al r5 0x0000", "al_r5_0x0000"},
     {{al, r5, 0x0001}, false, al, "al r5 0x0001", "al_r5_0x0001"},
     {{al, r5, 0x0002}, false, al, "al r5 0x0002", "al_r5_0x0002"},
     {{al, r5, 0x0020}, false, al, "al r5 0x0020", "al_r5_0x0020"},
     {{al, r5, 0x007d}, false, al, "al r5 0x007d", "al_r5_0x007d"},
     {{al, r5, 0x007e}, false, al, "al r5 0x007e", "al_r5_0x007e"},
     {{al, r5, 0x007f}, false, al, "al r5 0x007f", "al_r5_0x007f"},
     {{al, r5, 0x7ffd}, false, al, "al r5 0x7ffd", "al_r5_0x7ffd"},
     {{al, r5, 0x7ffe}, false, al, "al r5 0x7ffe", "al_r5_0x7ffe"},
     {{al, r5, 0x7fff}, false, al, "al r5 0x7fff", "al_r5_0x7fff"},
     {{al, r5, 0x3333}, false, al, "al r5 0x3333", "al_r5_0x3333"},
     {{al, r5, 0x5555}, false, al, "al r5 0x5555", "al_r5_0x5555"},
     {{al, r5, 0xaaaa}, false, al, "al r5 0xaaaa", "al_r5_0xaaaa"},
     {{al, r5, 0xcccc}, false, al, "al r5 0xcccc", "al_r5_0xcccc"},
     {{al, r5, 0x8000}, false, al, "al r5 0x8000", "al_r5_0x8000"},
     {{al, r5, 0x8001}, false, al, "al r5 0x8001", "al_r5_0x8001"},
     {{al, r5, 0x8002}, false, al, "al r5 0x8002", "al_r5_0x8002"},
     {{al, r5, 0x8003}, false, al, "al r5 0x8003", "al_r5_0x8003"},
     {{al, r5, 0xff80}, false, al, "al r5 0xff80", "al_r5_0xff80"},
     {{al, r5, 0xff81}, false, al, "al r5 0xff81", "al_r5_0xff81"},
     {{al, r5, 0xff82}, false, al, "al r5 0xff82", "al_r5_0xff82"},
     {{al, r5, 0xff83}, false, al, "al r5 0xff83", "al_r5_0xff83"},
     {{al, r5, 0xffe0}, false, al, "al r5 0xffe0", "al_r5_0xffe0"},
     {{al, r5, 0xfffd}, false, al, "al r5 0xfffd", "al_r5_0xfffd"},
     {{al, r5, 0xfffe}, false, al, "al r5 0xfffe", "al_r5_0xfffe"},
     {{al, r5, 0xffff}, false, al, "al r5 0xffff", "al_r5_0xffff"},
     {{al, r6, 0x0000}, false, al, "al r6 0x0000", "al_r6_0x0000"},
     {{al, r6, 0x0001}, false, al, "al r6 0x0001", "al_r6_0x0001"},
     {{al, r6, 0x0002}, false, al, "al r6 0x0002", "al_r6_0x0002"},
     {{al, r6, 0x0020}, false, al, "al r6 0x0020", "al_r6_0x0020"},
     {{al, r6, 0x007d}, false, al, "al r6 0x007d", "al_r6_0x007d"},
     {{al, r6, 0x007e}, false, al, "al r6 0x007e", "al_r6_0x007e"},
     {{al, r6, 0x007f}, false, al, "al r6 0x007f", "al_r6_0x007f"},
     {{al, r6, 0x7ffd}, false, al, "al r6 0x7ffd", "al_r6_0x7ffd"},
     {{al, r6, 0x7ffe}, false, al, "al r6 0x7ffe", "al_r6_0x7ffe"},
     {{al, r6, 0x7fff}, false, al, "al r6 0x7fff", "al_r6_0x7fff"},
     {{al, r6, 0x3333}, false, al, "al r6 0x3333", "al_r6_0x3333"},
     {{al, r6, 0x5555}, false, al, "al r6 0x5555", "al_r6_0x5555"},
     {{al, r6, 0xaaaa}, false, al, "al r6 0xaaaa", "al_r6_0xaaaa"},
     {{al, r6, 0xcccc}, false, al, "al r6 0xcccc", "al_r6_0xcccc"},
     {{al, r6, 0x8000}, false, al, "al r6 0x8000", "al_r6_0x8000"},
     {{al, r6, 0x8001}, false, al, "al r6 0x8001", "al_r6_0x8001"},
     {{al, r6, 0x8002}, false, al, "al r6 0x8002", "al_r6_0x8002"},
     {{al, r6, 0x8003}, false, al, "al r6 0x8003", "al_r6_0x8003"},
     {{al, r6, 0xff80}, false, al, "al r6 0xff80", "al_r6_0xff80"},
     {{al, r6, 0xff81}, false, al, "al r6 0xff81", "al_r6_0xff81"},
     {{al, r6, 0xff82}, false, al, "al r6 0xff82", "al_r6_0xff82"},
     {{al, r6, 0xff83}, false, al, "al r6 0xff83", "al_r6_0xff83"},
     {{al, r6, 0xffe0}, false, al, "al r6 0xffe0", "al_r6_0xffe0"},
     {{al, r6, 0xfffd}, false, al, "al r6 0xfffd", "al_r6_0xfffd"},
     {{al, r6, 0xfffe}, false, al, "al r6 0xfffe", "al_r6_0xfffe"},
     {{al, r6, 0xffff}, false, al, "al r6 0xffff", "al_r6_0xffff"},
     {{al, r7, 0x0000}, false, al, "al r7 0x0000", "al_r7_0x0000"},
     {{al, r7, 0x0001}, false, al, "al r7 0x0001", "al_r7_0x0001"},
     {{al, r7, 0x0002}, false, al, "al r7 0x0002", "al_r7_0x0002"},
     {{al, r7, 0x0020}, false, al, "al r7 0x0020", "al_r7_0x0020"},
     {{al, r7, 0x007d}, false, al, "al r7 0x007d", "al_r7_0x007d"},
     {{al, r7, 0x007e}, false, al, "al r7 0x007e", "al_r7_0x007e"},
     {{al, r7, 0x007f}, false, al, "al r7 0x007f", "al_r7_0x007f"},
     {{al, r7, 0x7ffd}, false, al, "al r7 0x7ffd", "al_r7_0x7ffd"},
     {{al, r7, 0x7ffe}, false, al, "al r7 0x7ffe", "al_r7_0x7ffe"},
     {{al, r7, 0x7fff}, false, al, "al r7 0x7fff", "al_r7_0x7fff"},
     {{al, r7, 0x3333}, false, al, "al r7 0x3333", "al_r7_0x3333"},
     {{al, r7, 0x5555}, false, al, "al r7 0x5555", "al_r7_0x5555"},
     {{al, r7, 0xaaaa}, false, al, "al r7 0xaaaa", "al_r7_0xaaaa"},
     {{al, r7, 0xcccc}, false, al, "al r7 0xcccc", "al_r7_0xcccc"},
     {{al, r7, 0x8000}, false, al, "al r7 0x8000", "al_r7_0x8000"},
     {{al, r7, 0x8001}, false, al, "al r7 0x8001", "al_r7_0x8001"},
     {{al, r7, 0x8002}, false, al, "al r7 0x8002", "al_r7_0x8002"},
     {{al, r7, 0x8003}, false, al, "al r7 0x8003", "al_r7_0x8003"},
     {{al, r7, 0xff80}, false, al, "al r7 0xff80", "al_r7_0xff80"},
     {{al, r7, 0xff81}, false, al, "al r7 0xff81", "al_r7_0xff81"},
     {{al, r7, 0xff82}, false, al, "al r7 0xff82", "al_r7_0xff82"},
     {{al, r7, 0xff83}, false, al, "al r7 0xff83", "al_r7_0xff83"},
     {{al, r7, 0xffe0}, false, al, "al r7 0xffe0", "al_r7_0xffe0"},
     {{al, r7, 0xfffd}, false, al, "al r7 0xfffd", "al_r7_0xfffd"},
     {{al, r7, 0xfffe}, false, al, "al r7 0xfffe", "al_r7_0xfffe"},
     {{al, r7, 0xffff}, false, al, "al r7 0xffff", "al_r7_0xffff"},
     {{al, r8, 0x0000}, false, al, "al r8 0x0000", "al_r8_0x0000"},
     {{al, r8, 0x0001}, false, al, "al r8 0x0001", "al_r8_0x0001"},
     {{al, r8, 0x0002}, false, al, "al r8 0x0002", "al_r8_0x0002"},
     {{al, r8, 0x0020}, false, al, "al r8 0x0020", "al_r8_0x0020"},
     {{al, r8, 0x007d}, false, al, "al r8 0x007d", "al_r8_0x007d"},
     {{al, r8, 0x007e}, false, al, "al r8 0x007e", "al_r8_0x007e"},
     {{al, r8, 0x007f}, false, al, "al r8 0x007f", "al_r8_0x007f"},
     {{al, r8, 0x7ffd}, false, al, "al r8 0x7ffd", "al_r8_0x7ffd"},
     {{al, r8, 0x7ffe}, false, al, "al r8 0x7ffe", "al_r8_0x7ffe"},
     {{al, r8, 0x7fff}, false, al, "al r8 0x7fff", "al_r8_0x7fff"},
     {{al, r8, 0x3333}, false, al, "al r8 0x3333", "al_r8_0x3333"},
     {{al, r8, 0x5555}, false, al, "al r8 0x5555", "al_r8_0x5555"},
     {{al, r8, 0xaaaa}, false, al, "al r8 0xaaaa", "al_r8_0xaaaa"},
     {{al, r8, 0xcccc}, false, al, "al r8 0xcccc", "al_r8_0xcccc"},
     {{al, r8, 0x8000}, false, al, "al r8 0x8000", "al_r8_0x8000"},
     {{al, r8, 0x8001}, false, al, "al r8 0x8001", "al_r8_0x8001"},
     {{al, r8, 0x8002}, false, al, "al r8 0x8002", "al_r8_0x8002"},
     {{al, r8, 0x8003}, false, al, "al r8 0x8003", "al_r8_0x8003"},
     {{al, r8, 0xff80}, false, al, "al r8 0xff80", "al_r8_0xff80"},
     {{al, r8, 0xff81}, false, al, "al r8 0xff81", "al_r8_0xff81"},
     {{al, r8, 0xff82}, false, al, "al r8 0xff82", "al_r8_0xff82"},
     {{al, r8, 0xff83}, false, al, "al r8 0xff83", "al_r8_0xff83"},
     {{al, r8, 0xffe0}, false, al, "al r8 0xffe0", "al_r8_0xffe0"},
     {{al, r8, 0xfffd}, false, al, "al r8 0xfffd", "al_r8_0xfffd"},
     {{al, r8, 0xfffe}, false, al, "al r8 0xfffe", "al_r8_0xfffe"},
     {{al, r8, 0xffff}, false, al, "al r8 0xffff", "al_r8_0xffff"},
     {{al, r9, 0x0000}, false, al, "al r9 0x0000", "al_r9_0x0000"},
     {{al, r9, 0x0001}, false, al, "al r9 0x0001", "al_r9_0x0001"},
     {{al, r9, 0x0002}, false, al, "al r9 0x0002", "al_r9_0x0002"},
     {{al, r9, 0x0020}, false, al, "al r9 0x0020", "al_r9_0x0020"},
     {{al, r9, 0x007d}, false, al, "al r9 0x007d", "al_r9_0x007d"},
     {{al, r9, 0x007e}, false, al, "al r9 0x007e", "al_r9_0x007e"},
     {{al, r9, 0x007f}, false, al, "al r9 0x007f", "al_r9_0x007f"},
     {{al, r9, 0x7ffd}, false, al, "al r9 0x7ffd", "al_r9_0x7ffd"},
     {{al, r9, 0x7ffe}, false, al, "al r9 0x7ffe", "al_r9_0x7ffe"},
     {{al, r9, 0x7fff}, false, al, "al r9 0x7fff", "al_r9_0x7fff"},
     {{al, r9, 0x3333}, false, al, "al r9 0x3333", "al_r9_0x3333"},
     {{al, r9, 0x5555}, false, al, "al r9 0x5555", "al_r9_0x5555"},
     {{al, r9, 0xaaaa}, false, al, "al r9 0xaaaa", "al_r9_0xaaaa"},
     {{al, r9, 0xcccc}, false, al, "al r9 0xcccc", "al_r9_0xcccc"},
     {{al, r9, 0x8000}, false, al, "al r9 0x8000", "al_r9_0x8000"},
     {{al, r9, 0x8001}, false, al, "al r9 0x8001", "al_r9_0x8001"},
     {{al, r9, 0x8002}, false, al, "al r9 0x8002", "al_r9_0x8002"},
     {{al, r9, 0x8003}, false, al, "al r9 0x8003", "al_r9_0x8003"},
     {{al, r9, 0xff80}, false, al, "al r9 0xff80", "al_r9_0xff80"},
     {{al, r9, 0xff81}, false, al, "al r9 0xff81", "al_r9_0xff81"},
     {{al, r9, 0xff82}, false, al, "al r9 0xff82", "al_r9_0xff82"},
     {{al, r9, 0xff83}, false, al, "al r9 0xff83", "al_r9_0xff83"},
     {{al, r9, 0xffe0}, false, al, "al r9 0xffe0", "al_r9_0xffe0"},
     {{al, r9, 0xfffd}, false, al, "al r9 0xfffd", "al_r9_0xfffd"},
     {{al, r9, 0xfffe}, false, al, "al r9 0xfffe", "al_r9_0xfffe"},
     {{al, r9, 0xffff}, false, al, "al r9 0xffff", "al_r9_0xffff"},
     {{al, r10, 0x0000}, false, al, "al r10 0x0000", "al_r10_0x0000"},
     {{al, r10, 0x0001}, false, al, "al r10 0x0001", "al_r10_0x0001"},
     {{al, r10, 0x0002}, false, al, "al r10 0x0002", "al_r10_0x0002"},
     {{al, r10, 0x0020}, false, al, "al r10 0x0020", "al_r10_0x0020"},
     {{al, r10, 0x007d}, false, al, "al r10 0x007d", "al_r10_0x007d"},
     {{al, r10, 0x007e}, false, al, "al r10 0x007e", "al_r10_0x007e"},
     {{al, r10, 0x007f}, false, al, "al r10 0x007f", "al_r10_0x007f"},
     {{al, r10, 0x7ffd}, false, al, "al r10 0x7ffd", "al_r10_0x7ffd"},
     {{al, r10, 0x7ffe}, false, al, "al r10 0x7ffe", "al_r10_0x7ffe"},
     {{al, r10, 0x7fff}, false, al, "al r10 0x7fff", "al_r10_0x7fff"},
     {{al, r10, 0x3333}, false, al, "al r10 0x3333", "al_r10_0x3333"},
     {{al, r10, 0x5555}, false, al, "al r10 0x5555", "al_r10_0x5555"},
     {{al, r10, 0xaaaa}, false, al, "al r10 0xaaaa", "al_r10_0xaaaa"},
     {{al, r10, 0xcccc}, false, al, "al r10 0xcccc", "al_r10_0xcccc"},
     {{al, r10, 0x8000}, false, al, "al r10 0x8000", "al_r10_0x8000"},
     {{al, r10, 0x8001}, false, al, "al r10 0x8001", "al_r10_0x8001"},
     {{al, r10, 0x8002}, false, al, "al r10 0x8002", "al_r10_0x8002"},
     {{al, r10, 0x8003}, false, al, "al r10 0x8003", "al_r10_0x8003"},
     {{al, r10, 0xff80}, false, al, "al r10 0xff80", "al_r10_0xff80"},
     {{al, r10, 0xff81}, false, al, "al r10 0xff81", "al_r10_0xff81"},
     {{al, r10, 0xff82}, false, al, "al r10 0xff82", "al_r10_0xff82"},
     {{al, r10, 0xff83}, false, al, "al r10 0xff83", "al_r10_0xff83"},
     {{al, r10, 0xffe0}, false, al, "al r10 0xffe0", "al_r10_0xffe0"},
     {{al, r10, 0xfffd}, false, al, "al r10 0xfffd", "al_r10_0xfffd"},
     {{al, r10, 0xfffe}, false, al, "al r10 0xfffe", "al_r10_0xfffe"},
     {{al, r10, 0xffff}, false, al, "al r10 0xffff", "al_r10_0xffff"},
     {{al, r11, 0x0000}, false, al, "al r11 0x0000", "al_r11_0x0000"},
     {{al, r11, 0x0001}, false, al, "al r11 0x0001", "al_r11_0x0001"},
     {{al, r11, 0x0002}, false, al, "al r11 0x0002", "al_r11_0x0002"},
     {{al, r11, 0x0020}, false, al, "al r11 0x0020", "al_r11_0x0020"},
     {{al, r11, 0x007d}, false, al, "al r11 0x007d", "al_r11_0x007d"},
     {{al, r11, 0x007e}, false, al, "al r11 0x007e", "al_r11_0x007e"},
     {{al, r11, 0x007f}, false, al, "al r11 0x007f", "al_r11_0x007f"},
     {{al, r11, 0x7ffd}, false, al, "al r11 0x7ffd", "al_r11_0x7ffd"},
     {{al, r11, 0x7ffe}, false, al, "al r11 0x7ffe", "al_r11_0x7ffe"},
     {{al, r11, 0x7fff}, false, al, "al r11 0x7fff", "al_r11_0x7fff"},
     {{al, r11, 0x3333}, false, al, "al r11 0x3333", "al_r11_0x3333"},
     {{al, r11, 0x5555}, false, al, "al r11 0x5555", "al_r11_0x5555"},
     {{al, r11, 0xaaaa}, false, al, "al r11 0xaaaa", "al_r11_0xaaaa"},
     {{al, r11, 0xcccc}, false, al, "al r11 0xcccc", "al_r11_0xcccc"},
     {{al, r11, 0x8000}, false, al, "al r11 0x8000", "al_r11_0x8000"},
     {{al, r11, 0x8001}, false, al, "al r11 0x8001", "al_r11_0x8001"},
     {{al, r11, 0x8002}, false, al, "al r11 0x8002", "al_r11_0x8002"},
     {{al, r11, 0x8003}, false, al, "al r11 0x8003", "al_r11_0x8003"},
     {{al, r11, 0xff80}, false, al, "al r11 0xff80", "al_r11_0xff80"},
     {{al, r11, 0xff81}, false, al, "al r11 0xff81", "al_r11_0xff81"},
     {{al, r11, 0xff82}, false, al, "al r11 0xff82", "al_r11_0xff82"},
     {{al, r11, 0xff83}, false, al, "al r11 0xff83", "al_r11_0xff83"},
     {{al, r11, 0xffe0}, false, al, "al r11 0xffe0", "al_r11_0xffe0"},
     {{al, r11, 0xfffd}, false, al, "al r11 0xfffd", "al_r11_0xfffd"},
     {{al, r11, 0xfffe}, false, al, "al r11 0xfffe", "al_r11_0xfffe"},
     {{al, r11, 0xffff}, false, al, "al r11 0xffff", "al_r11_0xffff"},
     {{al, r12, 0x0000}, false, al, "al r12 0x0000", "al_r12_0x0000"},
     {{al, r12, 0x0001}, false, al, "al r12 0x0001", "al_r12_0x0001"},
     {{al, r12, 0x0002}, false, al, "al r12 0x0002", "al_r12_0x0002"},
     {{al, r12, 0x0020}, false, al, "al r12 0x0020", "al_r12_0x0020"},
     {{al, r12, 0x007d}, false, al, "al r12 0x007d", "al_r12_0x007d"},
     {{al, r12, 0x007e}, false, al, "al r12 0x007e", "al_r12_0x007e"},
     {{al, r12, 0x007f}, false, al, "al r12 0x007f", "al_r12_0x007f"},
     {{al, r12, 0x7ffd}, false, al, "al r12 0x7ffd", "al_r12_0x7ffd"},
     {{al, r12, 0x7ffe}, false, al, "al r12 0x7ffe", "al_r12_0x7ffe"},
     {{al, r12, 0x7fff}, false, al, "al r12 0x7fff", "al_r12_0x7fff"},
     {{al, r12, 0x3333}, false, al, "al r12 0x3333", "al_r12_0x3333"},
     {{al, r12, 0x5555}, false, al, "al r12 0x5555", "al_r12_0x5555"},
     {{al, r12, 0xaaaa}, false, al, "al r12 0xaaaa", "al_r12_0xaaaa"},
     {{al, r12, 0xcccc}, false, al, "al r12 0xcccc", "al_r12_0xcccc"},
     {{al, r12, 0x8000}, false, al, "al r12 0x8000", "al_r12_0x8000"},
     {{al, r12, 0x8001}, false, al, "al r12 0x8001", "al_r12_0x8001"},
     {{al, r12, 0x8002}, false, al, "al r12 0x8002", "al_r12_0x8002"},
     {{al, r12, 0x8003}, false, al, "al r12 0x8003", "al_r12_0x8003"},
     {{al, r12, 0xff80}, false, al, "al r12 0xff80", "al_r12_0xff80"},
     {{al, r12, 0xff81}, false, al, "al r12 0xff81", "al_r12_0xff81"},
     {{al, r12, 0xff82}, false, al, "al r12 0xff82", "al_r12_0xff82"},
     {{al, r12, 0xff83}, false, al, "al r12 0xff83", "al_r12_0xff83"},
     {{al, r12, 0xffe0}, false, al, "al r12 0xffe0", "al_r12_0xffe0"},
     {{al, r12, 0xfffd}, false, al, "al r12 0xfffd", "al_r12_0xfffd"},
     {{al, r12, 0xfffe}, false, al, "al r12 0xfffe", "al_r12_0xfffe"},
     {{al, r12, 0xffff}, false, al, "al r12 0xffff", "al_r12_0xffff"},
     {{al, r13, 0x0000}, false, al, "al r13 0x0000", "al_r13_0x0000"},
     {{al, r13, 0x0001}, false, al, "al r13 0x0001", "al_r13_0x0001"},
     {{al, r13, 0x0002}, false, al, "al r13 0x0002", "al_r13_0x0002"},
     {{al, r13, 0x0020}, false, al, "al r13 0x0020", "al_r13_0x0020"},
     {{al, r13, 0x007d}, false, al, "al r13 0x007d", "al_r13_0x007d"},
     {{al, r13, 0x007e}, false, al, "al r13 0x007e", "al_r13_0x007e"},
     {{al, r13, 0x007f}, false, al, "al r13 0x007f", "al_r13_0x007f"},
     {{al, r13, 0x7ffd}, false, al, "al r13 0x7ffd", "al_r13_0x7ffd"},
     {{al, r13, 0x7ffe}, false, al, "al r13 0x7ffe", "al_r13_0x7ffe"},
     {{al, r13, 0x7fff}, false, al, "al r13 0x7fff", "al_r13_0x7fff"},
     {{al, r13, 0x3333}, false, al, "al r13 0x3333", "al_r13_0x3333"},
     {{al, r13, 0x5555}, false, al, "al r13 0x5555", "al_r13_0x5555"},
     {{al, r13, 0xaaaa}, false, al, "al r13 0xaaaa", "al_r13_0xaaaa"},
     {{al, r13, 0xcccc}, false, al, "al r13 0xcccc", "al_r13_0xcccc"},
     {{al, r13, 0x8000}, false, al, "al r13 0x8000", "al_r13_0x8000"},
     {{al, r13, 0x8001}, false, al, "al r13 0x8001", "al_r13_0x8001"},
     {{al, r13, 0x8002}, false, al, "al r13 0x8002", "al_r13_0x8002"},
     {{al, r13, 0x8003}, false, al, "al r13 0x8003", "al_r13_0x8003"},
     {{al, r13, 0xff80}, false, al, "al r13 0xff80", "al_r13_0xff80"},
     {{al, r13, 0xff81}, false, al, "al r13 0xff81", "al_r13_0xff81"},
     {{al, r13, 0xff82}, false, al, "al r13 0xff82", "al_r13_0xff82"},
     {{al, r13, 0xff83}, false, al, "al r13 0xff83", "al_r13_0xff83"},
     {{al, r13, 0xffe0}, false, al, "al r13 0xffe0", "al_r13_0xffe0"},
     {{al, r13, 0xfffd}, false, al, "al r13 0xfffd", "al_r13_0xfffd"},
     {{al, r13, 0xfffe}, false, al, "al r13 0xfffe", "al_r13_0xfffe"},
     {{al, r13, 0xffff}, false, al, "al r13 0xffff", "al_r13_0xffff"},
     {{al, r14, 0x0000}, false, al, "al r14 0x0000", "al_r14_0x0000"},
     {{al, r14, 0x0001}, false, al, "al r14 0x0001", "al_r14_0x0001"},
     {{al, r14, 0x0002}, false, al, "al r14 0x0002", "al_r14_0x0002"},
     {{al, r14, 0x0020}, false, al, "al r14 0x0020", "al_r14_0x0020"},
     {{al, r14, 0x007d}, false, al, "al r14 0x007d", "al_r14_0x007d"},
     {{al, r14, 0x007e}, false, al, "al r14 0x007e", "al_r14_0x007e"},
     {{al, r14, 0x007f}, false, al, "al r14 0x007f", "al_r14_0x007f"},
     {{al, r14, 0x7ffd}, false, al, "al r14 0x7ffd", "al_r14_0x7ffd"},
     {{al, r14, 0x7ffe}, false, al, "al r14 0x7ffe", "al_r14_0x7ffe"},
     {{al, r14, 0x7fff}, false, al, "al r14 0x7fff", "al_r14_0x7fff"},
     {{al, r14, 0x3333}, false, al, "al r14 0x3333", "al_r14_0x3333"},
     {{al, r14, 0x5555}, false, al, "al r14 0x5555", "al_r14_0x5555"},
     {{al, r14, 0xaaaa}, false, al, "al r14 0xaaaa", "al_r14_0xaaaa"},
     {{al, r14, 0xcccc}, false, al, "al r14 0xcccc", "al_r14_0xcccc"},
     {{al, r14, 0x8000}, false, al, "al r14 0x8000", "al_r14_0x8000"},
     {{al, r14, 0x8001}, false, al, "al r14 0x8001", "al_r14_0x8001"},
     {{al, r14, 0x8002}, false, al, "al r14 0x8002", "al_r14_0x8002"},
     {{al, r14, 0x8003}, false, al, "al r14 0x8003", "al_r14_0x8003"},
     {{al, r14, 0xff80}, false, al, "al r14 0xff80", "al_r14_0xff80"},
     {{al, r14, 0xff81}, false, al, "al r14 0xff81", "al_r14_0xff81"},
     {{al, r14, 0xff82}, false, al, "al r14 0xff82", "al_r14_0xff82"},
     {{al, r14, 0xff83}, false, al, "al r14 0xff83", "al_r14_0xff83"},
     {{al, r14, 0xffe0}, false, al, "al r14 0xffe0", "al_r14_0xffe0"},
     {{al, r14, 0xfffd}, false, al, "al r14 0xfffd", "al_r14_0xfffd"},
     {{al, r14, 0xfffe}, false, al, "al r14 0xfffe", "al_r14_0xfffe"},
     {{al, r14, 0xffff}, false, al, "al r14 0xffff", "al_r14_0xffff"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-operand-imm16-mov-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-imm16-movt-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-imm16-movw-t32.h"


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
  Test test_##mnemonic("AARCH32_ASSEMBLER_COND_RD_OPERAND_IMM16_" #mnemonic \
                       "_T32",                                              \
                       &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
