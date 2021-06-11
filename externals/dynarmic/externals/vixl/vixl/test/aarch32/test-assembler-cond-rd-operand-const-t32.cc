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
  M(cmn)                       \
  M(cmp)                       \
  M(mov)                       \
  M(movs)                      \
  M(mvn)                       \
  M(mvns)                      \
  M(teq)                       \
  M(tst)


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
    {{{al, r0, 0x000001fe}, false, al, "al r0 0x000001fe", "al_r0_0x000001fe"},
     {{al, r0, 0x000003fc}, false, al, "al r0 0x000003fc", "al_r0_0x000003fc"},
     {{al, r0, 0x000007f8}, false, al, "al r0 0x000007f8", "al_r0_0x000007f8"},
     {{al, r0, 0x00000ff0}, false, al, "al r0 0x00000ff0", "al_r0_0x00000ff0"},
     {{al, r0, 0x00001fe0}, false, al, "al r0 0x00001fe0", "al_r0_0x00001fe0"},
     {{al, r0, 0x00003fc0}, false, al, "al r0 0x00003fc0", "al_r0_0x00003fc0"},
     {{al, r0, 0x00007f80}, false, al, "al r0 0x00007f80", "al_r0_0x00007f80"},
     {{al, r0, 0x0000ff00}, false, al, "al r0 0x0000ff00", "al_r0_0x0000ff00"},
     {{al, r0, 0x0001fe00}, false, al, "al r0 0x0001fe00", "al_r0_0x0001fe00"},
     {{al, r0, 0x0003fc00}, false, al, "al r0 0x0003fc00", "al_r0_0x0003fc00"},
     {{al, r0, 0x0007f800}, false, al, "al r0 0x0007f800", "al_r0_0x0007f800"},
     {{al, r0, 0x000ff000}, false, al, "al r0 0x000ff000", "al_r0_0x000ff000"},
     {{al, r0, 0x001fe000}, false, al, "al r0 0x001fe000", "al_r0_0x001fe000"},
     {{al, r0, 0x003fc000}, false, al, "al r0 0x003fc000", "al_r0_0x003fc000"},
     {{al, r0, 0x007f8000}, false, al, "al r0 0x007f8000", "al_r0_0x007f8000"},
     {{al, r0, 0x00ff0000}, false, al, "al r0 0x00ff0000", "al_r0_0x00ff0000"},
     {{al, r0, 0x01fe0000}, false, al, "al r0 0x01fe0000", "al_r0_0x01fe0000"},
     {{al, r0, 0x03fc0000}, false, al, "al r0 0x03fc0000", "al_r0_0x03fc0000"},
     {{al, r0, 0x07f80000}, false, al, "al r0 0x07f80000", "al_r0_0x07f80000"},
     {{al, r0, 0x0ff00000}, false, al, "al r0 0x0ff00000", "al_r0_0x0ff00000"},
     {{al, r0, 0x1fe00000}, false, al, "al r0 0x1fe00000", "al_r0_0x1fe00000"},
     {{al, r0, 0x3fc00000}, false, al, "al r0 0x3fc00000", "al_r0_0x3fc00000"},
     {{al, r0, 0x7f800000}, false, al, "al r0 0x7f800000", "al_r0_0x7f800000"},
     {{al, r0, 0xff000000}, false, al, "al r0 0xff000000", "al_r0_0xff000000"},
     {{al, r0, 0x000000ff}, false, al, "al r0 0x000000ff", "al_r0_0x000000ff"},
     {{al, r0, 0x00ff00ff}, false, al, "al r0 0x00ff00ff", "al_r0_0x00ff00ff"},
     {{al, r0, 0xff00ff00}, false, al, "al r0 0xff00ff00", "al_r0_0xff00ff00"},
     {{al, r0, 0xffffffff}, false, al, "al r0 0xffffffff", "al_r0_0xffffffff"},
     {{al, r0, 0x00000156}, false, al, "al r0 0x00000156", "al_r0_0x00000156"},
     {{al, r0, 0x000002ac}, false, al, "al r0 0x000002ac", "al_r0_0x000002ac"},
     {{al, r0, 0x00000558}, false, al, "al r0 0x00000558", "al_r0_0x00000558"},
     {{al, r0, 0x00000ab0}, false, al, "al r0 0x00000ab0", "al_r0_0x00000ab0"},
     {{al, r0, 0x00001560}, false, al, "al r0 0x00001560", "al_r0_0x00001560"},
     {{al, r0, 0x00002ac0}, false, al, "al r0 0x00002ac0", "al_r0_0x00002ac0"},
     {{al, r0, 0x00005580}, false, al, "al r0 0x00005580", "al_r0_0x00005580"},
     {{al, r0, 0x0000ab00}, false, al, "al r0 0x0000ab00", "al_r0_0x0000ab00"},
     {{al, r0, 0x00015600}, false, al, "al r0 0x00015600", "al_r0_0x00015600"},
     {{al, r0, 0x0002ac00}, false, al, "al r0 0x0002ac00", "al_r0_0x0002ac00"},
     {{al, r0, 0x00055800}, false, al, "al r0 0x00055800", "al_r0_0x00055800"},
     {{al, r0, 0x000ab000}, false, al, "al r0 0x000ab000", "al_r0_0x000ab000"},
     {{al, r0, 0x00156000}, false, al, "al r0 0x00156000", "al_r0_0x00156000"},
     {{al, r0, 0x002ac000}, false, al, "al r0 0x002ac000", "al_r0_0x002ac000"},
     {{al, r0, 0x00558000}, false, al, "al r0 0x00558000", "al_r0_0x00558000"},
     {{al, r0, 0x00ab0000}, false, al, "al r0 0x00ab0000", "al_r0_0x00ab0000"},
     {{al, r0, 0x01560000}, false, al, "al r0 0x01560000", "al_r0_0x01560000"},
     {{al, r0, 0x02ac0000}, false, al, "al r0 0x02ac0000", "al_r0_0x02ac0000"},
     {{al, r0, 0x05580000}, false, al, "al r0 0x05580000", "al_r0_0x05580000"},
     {{al, r0, 0x0ab00000}, false, al, "al r0 0x0ab00000", "al_r0_0x0ab00000"},
     {{al, r0, 0x15600000}, false, al, "al r0 0x15600000", "al_r0_0x15600000"},
     {{al, r0, 0x2ac00000}, false, al, "al r0 0x2ac00000", "al_r0_0x2ac00000"},
     {{al, r0, 0x55800000}, false, al, "al r0 0x55800000", "al_r0_0x55800000"},
     {{al, r0, 0xab000000}, false, al, "al r0 0xab000000", "al_r0_0xab000000"},
     {{al, r0, 0x000000ab}, false, al, "al r0 0x000000ab", "al_r0_0x000000ab"},
     {{al, r0, 0x00ab00ab}, false, al, "al r0 0x00ab00ab", "al_r0_0x00ab00ab"},
     {{al, r0, 0xab00ab00}, false, al, "al r0 0xab00ab00", "al_r0_0xab00ab00"},
     {{al, r0, 0xabababab}, false, al, "al r0 0xabababab", "al_r0_0xabababab"},
     {{al, r1, 0x000001fe}, false, al, "al r1 0x000001fe", "al_r1_0x000001fe"},
     {{al, r1, 0x000003fc}, false, al, "al r1 0x000003fc", "al_r1_0x000003fc"},
     {{al, r1, 0x000007f8}, false, al, "al r1 0x000007f8", "al_r1_0x000007f8"},
     {{al, r1, 0x00000ff0}, false, al, "al r1 0x00000ff0", "al_r1_0x00000ff0"},
     {{al, r1, 0x00001fe0}, false, al, "al r1 0x00001fe0", "al_r1_0x00001fe0"},
     {{al, r1, 0x00003fc0}, false, al, "al r1 0x00003fc0", "al_r1_0x00003fc0"},
     {{al, r1, 0x00007f80}, false, al, "al r1 0x00007f80", "al_r1_0x00007f80"},
     {{al, r1, 0x0000ff00}, false, al, "al r1 0x0000ff00", "al_r1_0x0000ff00"},
     {{al, r1, 0x0001fe00}, false, al, "al r1 0x0001fe00", "al_r1_0x0001fe00"},
     {{al, r1, 0x0003fc00}, false, al, "al r1 0x0003fc00", "al_r1_0x0003fc00"},
     {{al, r1, 0x0007f800}, false, al, "al r1 0x0007f800", "al_r1_0x0007f800"},
     {{al, r1, 0x000ff000}, false, al, "al r1 0x000ff000", "al_r1_0x000ff000"},
     {{al, r1, 0x001fe000}, false, al, "al r1 0x001fe000", "al_r1_0x001fe000"},
     {{al, r1, 0x003fc000}, false, al, "al r1 0x003fc000", "al_r1_0x003fc000"},
     {{al, r1, 0x007f8000}, false, al, "al r1 0x007f8000", "al_r1_0x007f8000"},
     {{al, r1, 0x00ff0000}, false, al, "al r1 0x00ff0000", "al_r1_0x00ff0000"},
     {{al, r1, 0x01fe0000}, false, al, "al r1 0x01fe0000", "al_r1_0x01fe0000"},
     {{al, r1, 0x03fc0000}, false, al, "al r1 0x03fc0000", "al_r1_0x03fc0000"},
     {{al, r1, 0x07f80000}, false, al, "al r1 0x07f80000", "al_r1_0x07f80000"},
     {{al, r1, 0x0ff00000}, false, al, "al r1 0x0ff00000", "al_r1_0x0ff00000"},
     {{al, r1, 0x1fe00000}, false, al, "al r1 0x1fe00000", "al_r1_0x1fe00000"},
     {{al, r1, 0x3fc00000}, false, al, "al r1 0x3fc00000", "al_r1_0x3fc00000"},
     {{al, r1, 0x7f800000}, false, al, "al r1 0x7f800000", "al_r1_0x7f800000"},
     {{al, r1, 0xff000000}, false, al, "al r1 0xff000000", "al_r1_0xff000000"},
     {{al, r1, 0x000000ff}, false, al, "al r1 0x000000ff", "al_r1_0x000000ff"},
     {{al, r1, 0x00ff00ff}, false, al, "al r1 0x00ff00ff", "al_r1_0x00ff00ff"},
     {{al, r1, 0xff00ff00}, false, al, "al r1 0xff00ff00", "al_r1_0xff00ff00"},
     {{al, r1, 0xffffffff}, false, al, "al r1 0xffffffff", "al_r1_0xffffffff"},
     {{al, r1, 0x00000156}, false, al, "al r1 0x00000156", "al_r1_0x00000156"},
     {{al, r1, 0x000002ac}, false, al, "al r1 0x000002ac", "al_r1_0x000002ac"},
     {{al, r1, 0x00000558}, false, al, "al r1 0x00000558", "al_r1_0x00000558"},
     {{al, r1, 0x00000ab0}, false, al, "al r1 0x00000ab0", "al_r1_0x00000ab0"},
     {{al, r1, 0x00001560}, false, al, "al r1 0x00001560", "al_r1_0x00001560"},
     {{al, r1, 0x00002ac0}, false, al, "al r1 0x00002ac0", "al_r1_0x00002ac0"},
     {{al, r1, 0x00005580}, false, al, "al r1 0x00005580", "al_r1_0x00005580"},
     {{al, r1, 0x0000ab00}, false, al, "al r1 0x0000ab00", "al_r1_0x0000ab00"},
     {{al, r1, 0x00015600}, false, al, "al r1 0x00015600", "al_r1_0x00015600"},
     {{al, r1, 0x0002ac00}, false, al, "al r1 0x0002ac00", "al_r1_0x0002ac00"},
     {{al, r1, 0x00055800}, false, al, "al r1 0x00055800", "al_r1_0x00055800"},
     {{al, r1, 0x000ab000}, false, al, "al r1 0x000ab000", "al_r1_0x000ab000"},
     {{al, r1, 0x00156000}, false, al, "al r1 0x00156000", "al_r1_0x00156000"},
     {{al, r1, 0x002ac000}, false, al, "al r1 0x002ac000", "al_r1_0x002ac000"},
     {{al, r1, 0x00558000}, false, al, "al r1 0x00558000", "al_r1_0x00558000"},
     {{al, r1, 0x00ab0000}, false, al, "al r1 0x00ab0000", "al_r1_0x00ab0000"},
     {{al, r1, 0x01560000}, false, al, "al r1 0x01560000", "al_r1_0x01560000"},
     {{al, r1, 0x02ac0000}, false, al, "al r1 0x02ac0000", "al_r1_0x02ac0000"},
     {{al, r1, 0x05580000}, false, al, "al r1 0x05580000", "al_r1_0x05580000"},
     {{al, r1, 0x0ab00000}, false, al, "al r1 0x0ab00000", "al_r1_0x0ab00000"},
     {{al, r1, 0x15600000}, false, al, "al r1 0x15600000", "al_r1_0x15600000"},
     {{al, r1, 0x2ac00000}, false, al, "al r1 0x2ac00000", "al_r1_0x2ac00000"},
     {{al, r1, 0x55800000}, false, al, "al r1 0x55800000", "al_r1_0x55800000"},
     {{al, r1, 0xab000000}, false, al, "al r1 0xab000000", "al_r1_0xab000000"},
     {{al, r1, 0x000000ab}, false, al, "al r1 0x000000ab", "al_r1_0x000000ab"},
     {{al, r1, 0x00ab00ab}, false, al, "al r1 0x00ab00ab", "al_r1_0x00ab00ab"},
     {{al, r1, 0xab00ab00}, false, al, "al r1 0xab00ab00", "al_r1_0xab00ab00"},
     {{al, r1, 0xabababab}, false, al, "al r1 0xabababab", "al_r1_0xabababab"},
     {{al, r2, 0x000001fe}, false, al, "al r2 0x000001fe", "al_r2_0x000001fe"},
     {{al, r2, 0x000003fc}, false, al, "al r2 0x000003fc", "al_r2_0x000003fc"},
     {{al, r2, 0x000007f8}, false, al, "al r2 0x000007f8", "al_r2_0x000007f8"},
     {{al, r2, 0x00000ff0}, false, al, "al r2 0x00000ff0", "al_r2_0x00000ff0"},
     {{al, r2, 0x00001fe0}, false, al, "al r2 0x00001fe0", "al_r2_0x00001fe0"},
     {{al, r2, 0x00003fc0}, false, al, "al r2 0x00003fc0", "al_r2_0x00003fc0"},
     {{al, r2, 0x00007f80}, false, al, "al r2 0x00007f80", "al_r2_0x00007f80"},
     {{al, r2, 0x0000ff00}, false, al, "al r2 0x0000ff00", "al_r2_0x0000ff00"},
     {{al, r2, 0x0001fe00}, false, al, "al r2 0x0001fe00", "al_r2_0x0001fe00"},
     {{al, r2, 0x0003fc00}, false, al, "al r2 0x0003fc00", "al_r2_0x0003fc00"},
     {{al, r2, 0x0007f800}, false, al, "al r2 0x0007f800", "al_r2_0x0007f800"},
     {{al, r2, 0x000ff000}, false, al, "al r2 0x000ff000", "al_r2_0x000ff000"},
     {{al, r2, 0x001fe000}, false, al, "al r2 0x001fe000", "al_r2_0x001fe000"},
     {{al, r2, 0x003fc000}, false, al, "al r2 0x003fc000", "al_r2_0x003fc000"},
     {{al, r2, 0x007f8000}, false, al, "al r2 0x007f8000", "al_r2_0x007f8000"},
     {{al, r2, 0x00ff0000}, false, al, "al r2 0x00ff0000", "al_r2_0x00ff0000"},
     {{al, r2, 0x01fe0000}, false, al, "al r2 0x01fe0000", "al_r2_0x01fe0000"},
     {{al, r2, 0x03fc0000}, false, al, "al r2 0x03fc0000", "al_r2_0x03fc0000"},
     {{al, r2, 0x07f80000}, false, al, "al r2 0x07f80000", "al_r2_0x07f80000"},
     {{al, r2, 0x0ff00000}, false, al, "al r2 0x0ff00000", "al_r2_0x0ff00000"},
     {{al, r2, 0x1fe00000}, false, al, "al r2 0x1fe00000", "al_r2_0x1fe00000"},
     {{al, r2, 0x3fc00000}, false, al, "al r2 0x3fc00000", "al_r2_0x3fc00000"},
     {{al, r2, 0x7f800000}, false, al, "al r2 0x7f800000", "al_r2_0x7f800000"},
     {{al, r2, 0xff000000}, false, al, "al r2 0xff000000", "al_r2_0xff000000"},
     {{al, r2, 0x000000ff}, false, al, "al r2 0x000000ff", "al_r2_0x000000ff"},
     {{al, r2, 0x00ff00ff}, false, al, "al r2 0x00ff00ff", "al_r2_0x00ff00ff"},
     {{al, r2, 0xff00ff00}, false, al, "al r2 0xff00ff00", "al_r2_0xff00ff00"},
     {{al, r2, 0xffffffff}, false, al, "al r2 0xffffffff", "al_r2_0xffffffff"},
     {{al, r2, 0x00000156}, false, al, "al r2 0x00000156", "al_r2_0x00000156"},
     {{al, r2, 0x000002ac}, false, al, "al r2 0x000002ac", "al_r2_0x000002ac"},
     {{al, r2, 0x00000558}, false, al, "al r2 0x00000558", "al_r2_0x00000558"},
     {{al, r2, 0x00000ab0}, false, al, "al r2 0x00000ab0", "al_r2_0x00000ab0"},
     {{al, r2, 0x00001560}, false, al, "al r2 0x00001560", "al_r2_0x00001560"},
     {{al, r2, 0x00002ac0}, false, al, "al r2 0x00002ac0", "al_r2_0x00002ac0"},
     {{al, r2, 0x00005580}, false, al, "al r2 0x00005580", "al_r2_0x00005580"},
     {{al, r2, 0x0000ab00}, false, al, "al r2 0x0000ab00", "al_r2_0x0000ab00"},
     {{al, r2, 0x00015600}, false, al, "al r2 0x00015600", "al_r2_0x00015600"},
     {{al, r2, 0x0002ac00}, false, al, "al r2 0x0002ac00", "al_r2_0x0002ac00"},
     {{al, r2, 0x00055800}, false, al, "al r2 0x00055800", "al_r2_0x00055800"},
     {{al, r2, 0x000ab000}, false, al, "al r2 0x000ab000", "al_r2_0x000ab000"},
     {{al, r2, 0x00156000}, false, al, "al r2 0x00156000", "al_r2_0x00156000"},
     {{al, r2, 0x002ac000}, false, al, "al r2 0x002ac000", "al_r2_0x002ac000"},
     {{al, r2, 0x00558000}, false, al, "al r2 0x00558000", "al_r2_0x00558000"},
     {{al, r2, 0x00ab0000}, false, al, "al r2 0x00ab0000", "al_r2_0x00ab0000"},
     {{al, r2, 0x01560000}, false, al, "al r2 0x01560000", "al_r2_0x01560000"},
     {{al, r2, 0x02ac0000}, false, al, "al r2 0x02ac0000", "al_r2_0x02ac0000"},
     {{al, r2, 0x05580000}, false, al, "al r2 0x05580000", "al_r2_0x05580000"},
     {{al, r2, 0x0ab00000}, false, al, "al r2 0x0ab00000", "al_r2_0x0ab00000"},
     {{al, r2, 0x15600000}, false, al, "al r2 0x15600000", "al_r2_0x15600000"},
     {{al, r2, 0x2ac00000}, false, al, "al r2 0x2ac00000", "al_r2_0x2ac00000"},
     {{al, r2, 0x55800000}, false, al, "al r2 0x55800000", "al_r2_0x55800000"},
     {{al, r2, 0xab000000}, false, al, "al r2 0xab000000", "al_r2_0xab000000"},
     {{al, r2, 0x000000ab}, false, al, "al r2 0x000000ab", "al_r2_0x000000ab"},
     {{al, r2, 0x00ab00ab}, false, al, "al r2 0x00ab00ab", "al_r2_0x00ab00ab"},
     {{al, r2, 0xab00ab00}, false, al, "al r2 0xab00ab00", "al_r2_0xab00ab00"},
     {{al, r2, 0xabababab}, false, al, "al r2 0xabababab", "al_r2_0xabababab"},
     {{al, r3, 0x000001fe}, false, al, "al r3 0x000001fe", "al_r3_0x000001fe"},
     {{al, r3, 0x000003fc}, false, al, "al r3 0x000003fc", "al_r3_0x000003fc"},
     {{al, r3, 0x000007f8}, false, al, "al r3 0x000007f8", "al_r3_0x000007f8"},
     {{al, r3, 0x00000ff0}, false, al, "al r3 0x00000ff0", "al_r3_0x00000ff0"},
     {{al, r3, 0x00001fe0}, false, al, "al r3 0x00001fe0", "al_r3_0x00001fe0"},
     {{al, r3, 0x00003fc0}, false, al, "al r3 0x00003fc0", "al_r3_0x00003fc0"},
     {{al, r3, 0x00007f80}, false, al, "al r3 0x00007f80", "al_r3_0x00007f80"},
     {{al, r3, 0x0000ff00}, false, al, "al r3 0x0000ff00", "al_r3_0x0000ff00"},
     {{al, r3, 0x0001fe00}, false, al, "al r3 0x0001fe00", "al_r3_0x0001fe00"},
     {{al, r3, 0x0003fc00}, false, al, "al r3 0x0003fc00", "al_r3_0x0003fc00"},
     {{al, r3, 0x0007f800}, false, al, "al r3 0x0007f800", "al_r3_0x0007f800"},
     {{al, r3, 0x000ff000}, false, al, "al r3 0x000ff000", "al_r3_0x000ff000"},
     {{al, r3, 0x001fe000}, false, al, "al r3 0x001fe000", "al_r3_0x001fe000"},
     {{al, r3, 0x003fc000}, false, al, "al r3 0x003fc000", "al_r3_0x003fc000"},
     {{al, r3, 0x007f8000}, false, al, "al r3 0x007f8000", "al_r3_0x007f8000"},
     {{al, r3, 0x00ff0000}, false, al, "al r3 0x00ff0000", "al_r3_0x00ff0000"},
     {{al, r3, 0x01fe0000}, false, al, "al r3 0x01fe0000", "al_r3_0x01fe0000"},
     {{al, r3, 0x03fc0000}, false, al, "al r3 0x03fc0000", "al_r3_0x03fc0000"},
     {{al, r3, 0x07f80000}, false, al, "al r3 0x07f80000", "al_r3_0x07f80000"},
     {{al, r3, 0x0ff00000}, false, al, "al r3 0x0ff00000", "al_r3_0x0ff00000"},
     {{al, r3, 0x1fe00000}, false, al, "al r3 0x1fe00000", "al_r3_0x1fe00000"},
     {{al, r3, 0x3fc00000}, false, al, "al r3 0x3fc00000", "al_r3_0x3fc00000"},
     {{al, r3, 0x7f800000}, false, al, "al r3 0x7f800000", "al_r3_0x7f800000"},
     {{al, r3, 0xff000000}, false, al, "al r3 0xff000000", "al_r3_0xff000000"},
     {{al, r3, 0x000000ff}, false, al, "al r3 0x000000ff", "al_r3_0x000000ff"},
     {{al, r3, 0x00ff00ff}, false, al, "al r3 0x00ff00ff", "al_r3_0x00ff00ff"},
     {{al, r3, 0xff00ff00}, false, al, "al r3 0xff00ff00", "al_r3_0xff00ff00"},
     {{al, r3, 0xffffffff}, false, al, "al r3 0xffffffff", "al_r3_0xffffffff"},
     {{al, r3, 0x00000156}, false, al, "al r3 0x00000156", "al_r3_0x00000156"},
     {{al, r3, 0x000002ac}, false, al, "al r3 0x000002ac", "al_r3_0x000002ac"},
     {{al, r3, 0x00000558}, false, al, "al r3 0x00000558", "al_r3_0x00000558"},
     {{al, r3, 0x00000ab0}, false, al, "al r3 0x00000ab0", "al_r3_0x00000ab0"},
     {{al, r3, 0x00001560}, false, al, "al r3 0x00001560", "al_r3_0x00001560"},
     {{al, r3, 0x00002ac0}, false, al, "al r3 0x00002ac0", "al_r3_0x00002ac0"},
     {{al, r3, 0x00005580}, false, al, "al r3 0x00005580", "al_r3_0x00005580"},
     {{al, r3, 0x0000ab00}, false, al, "al r3 0x0000ab00", "al_r3_0x0000ab00"},
     {{al, r3, 0x00015600}, false, al, "al r3 0x00015600", "al_r3_0x00015600"},
     {{al, r3, 0x0002ac00}, false, al, "al r3 0x0002ac00", "al_r3_0x0002ac00"},
     {{al, r3, 0x00055800}, false, al, "al r3 0x00055800", "al_r3_0x00055800"},
     {{al, r3, 0x000ab000}, false, al, "al r3 0x000ab000", "al_r3_0x000ab000"},
     {{al, r3, 0x00156000}, false, al, "al r3 0x00156000", "al_r3_0x00156000"},
     {{al, r3, 0x002ac000}, false, al, "al r3 0x002ac000", "al_r3_0x002ac000"},
     {{al, r3, 0x00558000}, false, al, "al r3 0x00558000", "al_r3_0x00558000"},
     {{al, r3, 0x00ab0000}, false, al, "al r3 0x00ab0000", "al_r3_0x00ab0000"},
     {{al, r3, 0x01560000}, false, al, "al r3 0x01560000", "al_r3_0x01560000"},
     {{al, r3, 0x02ac0000}, false, al, "al r3 0x02ac0000", "al_r3_0x02ac0000"},
     {{al, r3, 0x05580000}, false, al, "al r3 0x05580000", "al_r3_0x05580000"},
     {{al, r3, 0x0ab00000}, false, al, "al r3 0x0ab00000", "al_r3_0x0ab00000"},
     {{al, r3, 0x15600000}, false, al, "al r3 0x15600000", "al_r3_0x15600000"},
     {{al, r3, 0x2ac00000}, false, al, "al r3 0x2ac00000", "al_r3_0x2ac00000"},
     {{al, r3, 0x55800000}, false, al, "al r3 0x55800000", "al_r3_0x55800000"},
     {{al, r3, 0xab000000}, false, al, "al r3 0xab000000", "al_r3_0xab000000"},
     {{al, r3, 0x000000ab}, false, al, "al r3 0x000000ab", "al_r3_0x000000ab"},
     {{al, r3, 0x00ab00ab}, false, al, "al r3 0x00ab00ab", "al_r3_0x00ab00ab"},
     {{al, r3, 0xab00ab00}, false, al, "al r3 0xab00ab00", "al_r3_0xab00ab00"},
     {{al, r3, 0xabababab}, false, al, "al r3 0xabababab", "al_r3_0xabababab"},
     {{al, r4, 0x000001fe}, false, al, "al r4 0x000001fe", "al_r4_0x000001fe"},
     {{al, r4, 0x000003fc}, false, al, "al r4 0x000003fc", "al_r4_0x000003fc"},
     {{al, r4, 0x000007f8}, false, al, "al r4 0x000007f8", "al_r4_0x000007f8"},
     {{al, r4, 0x00000ff0}, false, al, "al r4 0x00000ff0", "al_r4_0x00000ff0"},
     {{al, r4, 0x00001fe0}, false, al, "al r4 0x00001fe0", "al_r4_0x00001fe0"},
     {{al, r4, 0x00003fc0}, false, al, "al r4 0x00003fc0", "al_r4_0x00003fc0"},
     {{al, r4, 0x00007f80}, false, al, "al r4 0x00007f80", "al_r4_0x00007f80"},
     {{al, r4, 0x0000ff00}, false, al, "al r4 0x0000ff00", "al_r4_0x0000ff00"},
     {{al, r4, 0x0001fe00}, false, al, "al r4 0x0001fe00", "al_r4_0x0001fe00"},
     {{al, r4, 0x0003fc00}, false, al, "al r4 0x0003fc00", "al_r4_0x0003fc00"},
     {{al, r4, 0x0007f800}, false, al, "al r4 0x0007f800", "al_r4_0x0007f800"},
     {{al, r4, 0x000ff000}, false, al, "al r4 0x000ff000", "al_r4_0x000ff000"},
     {{al, r4, 0x001fe000}, false, al, "al r4 0x001fe000", "al_r4_0x001fe000"},
     {{al, r4, 0x003fc000}, false, al, "al r4 0x003fc000", "al_r4_0x003fc000"},
     {{al, r4, 0x007f8000}, false, al, "al r4 0x007f8000", "al_r4_0x007f8000"},
     {{al, r4, 0x00ff0000}, false, al, "al r4 0x00ff0000", "al_r4_0x00ff0000"},
     {{al, r4, 0x01fe0000}, false, al, "al r4 0x01fe0000", "al_r4_0x01fe0000"},
     {{al, r4, 0x03fc0000}, false, al, "al r4 0x03fc0000", "al_r4_0x03fc0000"},
     {{al, r4, 0x07f80000}, false, al, "al r4 0x07f80000", "al_r4_0x07f80000"},
     {{al, r4, 0x0ff00000}, false, al, "al r4 0x0ff00000", "al_r4_0x0ff00000"},
     {{al, r4, 0x1fe00000}, false, al, "al r4 0x1fe00000", "al_r4_0x1fe00000"},
     {{al, r4, 0x3fc00000}, false, al, "al r4 0x3fc00000", "al_r4_0x3fc00000"},
     {{al, r4, 0x7f800000}, false, al, "al r4 0x7f800000", "al_r4_0x7f800000"},
     {{al, r4, 0xff000000}, false, al, "al r4 0xff000000", "al_r4_0xff000000"},
     {{al, r4, 0x000000ff}, false, al, "al r4 0x000000ff", "al_r4_0x000000ff"},
     {{al, r4, 0x00ff00ff}, false, al, "al r4 0x00ff00ff", "al_r4_0x00ff00ff"},
     {{al, r4, 0xff00ff00}, false, al, "al r4 0xff00ff00", "al_r4_0xff00ff00"},
     {{al, r4, 0xffffffff}, false, al, "al r4 0xffffffff", "al_r4_0xffffffff"},
     {{al, r4, 0x00000156}, false, al, "al r4 0x00000156", "al_r4_0x00000156"},
     {{al, r4, 0x000002ac}, false, al, "al r4 0x000002ac", "al_r4_0x000002ac"},
     {{al, r4, 0x00000558}, false, al, "al r4 0x00000558", "al_r4_0x00000558"},
     {{al, r4, 0x00000ab0}, false, al, "al r4 0x00000ab0", "al_r4_0x00000ab0"},
     {{al, r4, 0x00001560}, false, al, "al r4 0x00001560", "al_r4_0x00001560"},
     {{al, r4, 0x00002ac0}, false, al, "al r4 0x00002ac0", "al_r4_0x00002ac0"},
     {{al, r4, 0x00005580}, false, al, "al r4 0x00005580", "al_r4_0x00005580"},
     {{al, r4, 0x0000ab00}, false, al, "al r4 0x0000ab00", "al_r4_0x0000ab00"},
     {{al, r4, 0x00015600}, false, al, "al r4 0x00015600", "al_r4_0x00015600"},
     {{al, r4, 0x0002ac00}, false, al, "al r4 0x0002ac00", "al_r4_0x0002ac00"},
     {{al, r4, 0x00055800}, false, al, "al r4 0x00055800", "al_r4_0x00055800"},
     {{al, r4, 0x000ab000}, false, al, "al r4 0x000ab000", "al_r4_0x000ab000"},
     {{al, r4, 0x00156000}, false, al, "al r4 0x00156000", "al_r4_0x00156000"},
     {{al, r4, 0x002ac000}, false, al, "al r4 0x002ac000", "al_r4_0x002ac000"},
     {{al, r4, 0x00558000}, false, al, "al r4 0x00558000", "al_r4_0x00558000"},
     {{al, r4, 0x00ab0000}, false, al, "al r4 0x00ab0000", "al_r4_0x00ab0000"},
     {{al, r4, 0x01560000}, false, al, "al r4 0x01560000", "al_r4_0x01560000"},
     {{al, r4, 0x02ac0000}, false, al, "al r4 0x02ac0000", "al_r4_0x02ac0000"},
     {{al, r4, 0x05580000}, false, al, "al r4 0x05580000", "al_r4_0x05580000"},
     {{al, r4, 0x0ab00000}, false, al, "al r4 0x0ab00000", "al_r4_0x0ab00000"},
     {{al, r4, 0x15600000}, false, al, "al r4 0x15600000", "al_r4_0x15600000"},
     {{al, r4, 0x2ac00000}, false, al, "al r4 0x2ac00000", "al_r4_0x2ac00000"},
     {{al, r4, 0x55800000}, false, al, "al r4 0x55800000", "al_r4_0x55800000"},
     {{al, r4, 0xab000000}, false, al, "al r4 0xab000000", "al_r4_0xab000000"},
     {{al, r4, 0x000000ab}, false, al, "al r4 0x000000ab", "al_r4_0x000000ab"},
     {{al, r4, 0x00ab00ab}, false, al, "al r4 0x00ab00ab", "al_r4_0x00ab00ab"},
     {{al, r4, 0xab00ab00}, false, al, "al r4 0xab00ab00", "al_r4_0xab00ab00"},
     {{al, r4, 0xabababab}, false, al, "al r4 0xabababab", "al_r4_0xabababab"},
     {{al, r5, 0x000001fe}, false, al, "al r5 0x000001fe", "al_r5_0x000001fe"},
     {{al, r5, 0x000003fc}, false, al, "al r5 0x000003fc", "al_r5_0x000003fc"},
     {{al, r5, 0x000007f8}, false, al, "al r5 0x000007f8", "al_r5_0x000007f8"},
     {{al, r5, 0x00000ff0}, false, al, "al r5 0x00000ff0", "al_r5_0x00000ff0"},
     {{al, r5, 0x00001fe0}, false, al, "al r5 0x00001fe0", "al_r5_0x00001fe0"},
     {{al, r5, 0x00003fc0}, false, al, "al r5 0x00003fc0", "al_r5_0x00003fc0"},
     {{al, r5, 0x00007f80}, false, al, "al r5 0x00007f80", "al_r5_0x00007f80"},
     {{al, r5, 0x0000ff00}, false, al, "al r5 0x0000ff00", "al_r5_0x0000ff00"},
     {{al, r5, 0x0001fe00}, false, al, "al r5 0x0001fe00", "al_r5_0x0001fe00"},
     {{al, r5, 0x0003fc00}, false, al, "al r5 0x0003fc00", "al_r5_0x0003fc00"},
     {{al, r5, 0x0007f800}, false, al, "al r5 0x0007f800", "al_r5_0x0007f800"},
     {{al, r5, 0x000ff000}, false, al, "al r5 0x000ff000", "al_r5_0x000ff000"},
     {{al, r5, 0x001fe000}, false, al, "al r5 0x001fe000", "al_r5_0x001fe000"},
     {{al, r5, 0x003fc000}, false, al, "al r5 0x003fc000", "al_r5_0x003fc000"},
     {{al, r5, 0x007f8000}, false, al, "al r5 0x007f8000", "al_r5_0x007f8000"},
     {{al, r5, 0x00ff0000}, false, al, "al r5 0x00ff0000", "al_r5_0x00ff0000"},
     {{al, r5, 0x01fe0000}, false, al, "al r5 0x01fe0000", "al_r5_0x01fe0000"},
     {{al, r5, 0x03fc0000}, false, al, "al r5 0x03fc0000", "al_r5_0x03fc0000"},
     {{al, r5, 0x07f80000}, false, al, "al r5 0x07f80000", "al_r5_0x07f80000"},
     {{al, r5, 0x0ff00000}, false, al, "al r5 0x0ff00000", "al_r5_0x0ff00000"},
     {{al, r5, 0x1fe00000}, false, al, "al r5 0x1fe00000", "al_r5_0x1fe00000"},
     {{al, r5, 0x3fc00000}, false, al, "al r5 0x3fc00000", "al_r5_0x3fc00000"},
     {{al, r5, 0x7f800000}, false, al, "al r5 0x7f800000", "al_r5_0x7f800000"},
     {{al, r5, 0xff000000}, false, al, "al r5 0xff000000", "al_r5_0xff000000"},
     {{al, r5, 0x000000ff}, false, al, "al r5 0x000000ff", "al_r5_0x000000ff"},
     {{al, r5, 0x00ff00ff}, false, al, "al r5 0x00ff00ff", "al_r5_0x00ff00ff"},
     {{al, r5, 0xff00ff00}, false, al, "al r5 0xff00ff00", "al_r5_0xff00ff00"},
     {{al, r5, 0xffffffff}, false, al, "al r5 0xffffffff", "al_r5_0xffffffff"},
     {{al, r5, 0x00000156}, false, al, "al r5 0x00000156", "al_r5_0x00000156"},
     {{al, r5, 0x000002ac}, false, al, "al r5 0x000002ac", "al_r5_0x000002ac"},
     {{al, r5, 0x00000558}, false, al, "al r5 0x00000558", "al_r5_0x00000558"},
     {{al, r5, 0x00000ab0}, false, al, "al r5 0x00000ab0", "al_r5_0x00000ab0"},
     {{al, r5, 0x00001560}, false, al, "al r5 0x00001560", "al_r5_0x00001560"},
     {{al, r5, 0x00002ac0}, false, al, "al r5 0x00002ac0", "al_r5_0x00002ac0"},
     {{al, r5, 0x00005580}, false, al, "al r5 0x00005580", "al_r5_0x00005580"},
     {{al, r5, 0x0000ab00}, false, al, "al r5 0x0000ab00", "al_r5_0x0000ab00"},
     {{al, r5, 0x00015600}, false, al, "al r5 0x00015600", "al_r5_0x00015600"},
     {{al, r5, 0x0002ac00}, false, al, "al r5 0x0002ac00", "al_r5_0x0002ac00"},
     {{al, r5, 0x00055800}, false, al, "al r5 0x00055800", "al_r5_0x00055800"},
     {{al, r5, 0x000ab000}, false, al, "al r5 0x000ab000", "al_r5_0x000ab000"},
     {{al, r5, 0x00156000}, false, al, "al r5 0x00156000", "al_r5_0x00156000"},
     {{al, r5, 0x002ac000}, false, al, "al r5 0x002ac000", "al_r5_0x002ac000"},
     {{al, r5, 0x00558000}, false, al, "al r5 0x00558000", "al_r5_0x00558000"},
     {{al, r5, 0x00ab0000}, false, al, "al r5 0x00ab0000", "al_r5_0x00ab0000"},
     {{al, r5, 0x01560000}, false, al, "al r5 0x01560000", "al_r5_0x01560000"},
     {{al, r5, 0x02ac0000}, false, al, "al r5 0x02ac0000", "al_r5_0x02ac0000"},
     {{al, r5, 0x05580000}, false, al, "al r5 0x05580000", "al_r5_0x05580000"},
     {{al, r5, 0x0ab00000}, false, al, "al r5 0x0ab00000", "al_r5_0x0ab00000"},
     {{al, r5, 0x15600000}, false, al, "al r5 0x15600000", "al_r5_0x15600000"},
     {{al, r5, 0x2ac00000}, false, al, "al r5 0x2ac00000", "al_r5_0x2ac00000"},
     {{al, r5, 0x55800000}, false, al, "al r5 0x55800000", "al_r5_0x55800000"},
     {{al, r5, 0xab000000}, false, al, "al r5 0xab000000", "al_r5_0xab000000"},
     {{al, r5, 0x000000ab}, false, al, "al r5 0x000000ab", "al_r5_0x000000ab"},
     {{al, r5, 0x00ab00ab}, false, al, "al r5 0x00ab00ab", "al_r5_0x00ab00ab"},
     {{al, r5, 0xab00ab00}, false, al, "al r5 0xab00ab00", "al_r5_0xab00ab00"},
     {{al, r5, 0xabababab}, false, al, "al r5 0xabababab", "al_r5_0xabababab"},
     {{al, r6, 0x000001fe}, false, al, "al r6 0x000001fe", "al_r6_0x000001fe"},
     {{al, r6, 0x000003fc}, false, al, "al r6 0x000003fc", "al_r6_0x000003fc"},
     {{al, r6, 0x000007f8}, false, al, "al r6 0x000007f8", "al_r6_0x000007f8"},
     {{al, r6, 0x00000ff0}, false, al, "al r6 0x00000ff0", "al_r6_0x00000ff0"},
     {{al, r6, 0x00001fe0}, false, al, "al r6 0x00001fe0", "al_r6_0x00001fe0"},
     {{al, r6, 0x00003fc0}, false, al, "al r6 0x00003fc0", "al_r6_0x00003fc0"},
     {{al, r6, 0x00007f80}, false, al, "al r6 0x00007f80", "al_r6_0x00007f80"},
     {{al, r6, 0x0000ff00}, false, al, "al r6 0x0000ff00", "al_r6_0x0000ff00"},
     {{al, r6, 0x0001fe00}, false, al, "al r6 0x0001fe00", "al_r6_0x0001fe00"},
     {{al, r6, 0x0003fc00}, false, al, "al r6 0x0003fc00", "al_r6_0x0003fc00"},
     {{al, r6, 0x0007f800}, false, al, "al r6 0x0007f800", "al_r6_0x0007f800"},
     {{al, r6, 0x000ff000}, false, al, "al r6 0x000ff000", "al_r6_0x000ff000"},
     {{al, r6, 0x001fe000}, false, al, "al r6 0x001fe000", "al_r6_0x001fe000"},
     {{al, r6, 0x003fc000}, false, al, "al r6 0x003fc000", "al_r6_0x003fc000"},
     {{al, r6, 0x007f8000}, false, al, "al r6 0x007f8000", "al_r6_0x007f8000"},
     {{al, r6, 0x00ff0000}, false, al, "al r6 0x00ff0000", "al_r6_0x00ff0000"},
     {{al, r6, 0x01fe0000}, false, al, "al r6 0x01fe0000", "al_r6_0x01fe0000"},
     {{al, r6, 0x03fc0000}, false, al, "al r6 0x03fc0000", "al_r6_0x03fc0000"},
     {{al, r6, 0x07f80000}, false, al, "al r6 0x07f80000", "al_r6_0x07f80000"},
     {{al, r6, 0x0ff00000}, false, al, "al r6 0x0ff00000", "al_r6_0x0ff00000"},
     {{al, r6, 0x1fe00000}, false, al, "al r6 0x1fe00000", "al_r6_0x1fe00000"},
     {{al, r6, 0x3fc00000}, false, al, "al r6 0x3fc00000", "al_r6_0x3fc00000"},
     {{al, r6, 0x7f800000}, false, al, "al r6 0x7f800000", "al_r6_0x7f800000"},
     {{al, r6, 0xff000000}, false, al, "al r6 0xff000000", "al_r6_0xff000000"},
     {{al, r6, 0x000000ff}, false, al, "al r6 0x000000ff", "al_r6_0x000000ff"},
     {{al, r6, 0x00ff00ff}, false, al, "al r6 0x00ff00ff", "al_r6_0x00ff00ff"},
     {{al, r6, 0xff00ff00}, false, al, "al r6 0xff00ff00", "al_r6_0xff00ff00"},
     {{al, r6, 0xffffffff}, false, al, "al r6 0xffffffff", "al_r6_0xffffffff"},
     {{al, r6, 0x00000156}, false, al, "al r6 0x00000156", "al_r6_0x00000156"},
     {{al, r6, 0x000002ac}, false, al, "al r6 0x000002ac", "al_r6_0x000002ac"},
     {{al, r6, 0x00000558}, false, al, "al r6 0x00000558", "al_r6_0x00000558"},
     {{al, r6, 0x00000ab0}, false, al, "al r6 0x00000ab0", "al_r6_0x00000ab0"},
     {{al, r6, 0x00001560}, false, al, "al r6 0x00001560", "al_r6_0x00001560"},
     {{al, r6, 0x00002ac0}, false, al, "al r6 0x00002ac0", "al_r6_0x00002ac0"},
     {{al, r6, 0x00005580}, false, al, "al r6 0x00005580", "al_r6_0x00005580"},
     {{al, r6, 0x0000ab00}, false, al, "al r6 0x0000ab00", "al_r6_0x0000ab00"},
     {{al, r6, 0x00015600}, false, al, "al r6 0x00015600", "al_r6_0x00015600"},
     {{al, r6, 0x0002ac00}, false, al, "al r6 0x0002ac00", "al_r6_0x0002ac00"},
     {{al, r6, 0x00055800}, false, al, "al r6 0x00055800", "al_r6_0x00055800"},
     {{al, r6, 0x000ab000}, false, al, "al r6 0x000ab000", "al_r6_0x000ab000"},
     {{al, r6, 0x00156000}, false, al, "al r6 0x00156000", "al_r6_0x00156000"},
     {{al, r6, 0x002ac000}, false, al, "al r6 0x002ac000", "al_r6_0x002ac000"},
     {{al, r6, 0x00558000}, false, al, "al r6 0x00558000", "al_r6_0x00558000"},
     {{al, r6, 0x00ab0000}, false, al, "al r6 0x00ab0000", "al_r6_0x00ab0000"},
     {{al, r6, 0x01560000}, false, al, "al r6 0x01560000", "al_r6_0x01560000"},
     {{al, r6, 0x02ac0000}, false, al, "al r6 0x02ac0000", "al_r6_0x02ac0000"},
     {{al, r6, 0x05580000}, false, al, "al r6 0x05580000", "al_r6_0x05580000"},
     {{al, r6, 0x0ab00000}, false, al, "al r6 0x0ab00000", "al_r6_0x0ab00000"},
     {{al, r6, 0x15600000}, false, al, "al r6 0x15600000", "al_r6_0x15600000"},
     {{al, r6, 0x2ac00000}, false, al, "al r6 0x2ac00000", "al_r6_0x2ac00000"},
     {{al, r6, 0x55800000}, false, al, "al r6 0x55800000", "al_r6_0x55800000"},
     {{al, r6, 0xab000000}, false, al, "al r6 0xab000000", "al_r6_0xab000000"},
     {{al, r6, 0x000000ab}, false, al, "al r6 0x000000ab", "al_r6_0x000000ab"},
     {{al, r6, 0x00ab00ab}, false, al, "al r6 0x00ab00ab", "al_r6_0x00ab00ab"},
     {{al, r6, 0xab00ab00}, false, al, "al r6 0xab00ab00", "al_r6_0xab00ab00"},
     {{al, r6, 0xabababab}, false, al, "al r6 0xabababab", "al_r6_0xabababab"},
     {{al, r7, 0x000001fe}, false, al, "al r7 0x000001fe", "al_r7_0x000001fe"},
     {{al, r7, 0x000003fc}, false, al, "al r7 0x000003fc", "al_r7_0x000003fc"},
     {{al, r7, 0x000007f8}, false, al, "al r7 0x000007f8", "al_r7_0x000007f8"},
     {{al, r7, 0x00000ff0}, false, al, "al r7 0x00000ff0", "al_r7_0x00000ff0"},
     {{al, r7, 0x00001fe0}, false, al, "al r7 0x00001fe0", "al_r7_0x00001fe0"},
     {{al, r7, 0x00003fc0}, false, al, "al r7 0x00003fc0", "al_r7_0x00003fc0"},
     {{al, r7, 0x00007f80}, false, al, "al r7 0x00007f80", "al_r7_0x00007f80"},
     {{al, r7, 0x0000ff00}, false, al, "al r7 0x0000ff00", "al_r7_0x0000ff00"},
     {{al, r7, 0x0001fe00}, false, al, "al r7 0x0001fe00", "al_r7_0x0001fe00"},
     {{al, r7, 0x0003fc00}, false, al, "al r7 0x0003fc00", "al_r7_0x0003fc00"},
     {{al, r7, 0x0007f800}, false, al, "al r7 0x0007f800", "al_r7_0x0007f800"},
     {{al, r7, 0x000ff000}, false, al, "al r7 0x000ff000", "al_r7_0x000ff000"},
     {{al, r7, 0x001fe000}, false, al, "al r7 0x001fe000", "al_r7_0x001fe000"},
     {{al, r7, 0x003fc000}, false, al, "al r7 0x003fc000", "al_r7_0x003fc000"},
     {{al, r7, 0x007f8000}, false, al, "al r7 0x007f8000", "al_r7_0x007f8000"},
     {{al, r7, 0x00ff0000}, false, al, "al r7 0x00ff0000", "al_r7_0x00ff0000"},
     {{al, r7, 0x01fe0000}, false, al, "al r7 0x01fe0000", "al_r7_0x01fe0000"},
     {{al, r7, 0x03fc0000}, false, al, "al r7 0x03fc0000", "al_r7_0x03fc0000"},
     {{al, r7, 0x07f80000}, false, al, "al r7 0x07f80000", "al_r7_0x07f80000"},
     {{al, r7, 0x0ff00000}, false, al, "al r7 0x0ff00000", "al_r7_0x0ff00000"},
     {{al, r7, 0x1fe00000}, false, al, "al r7 0x1fe00000", "al_r7_0x1fe00000"},
     {{al, r7, 0x3fc00000}, false, al, "al r7 0x3fc00000", "al_r7_0x3fc00000"},
     {{al, r7, 0x7f800000}, false, al, "al r7 0x7f800000", "al_r7_0x7f800000"},
     {{al, r7, 0xff000000}, false, al, "al r7 0xff000000", "al_r7_0xff000000"},
     {{al, r7, 0x000000ff}, false, al, "al r7 0x000000ff", "al_r7_0x000000ff"},
     {{al, r7, 0x00ff00ff}, false, al, "al r7 0x00ff00ff", "al_r7_0x00ff00ff"},
     {{al, r7, 0xff00ff00}, false, al, "al r7 0xff00ff00", "al_r7_0xff00ff00"},
     {{al, r7, 0xffffffff}, false, al, "al r7 0xffffffff", "al_r7_0xffffffff"},
     {{al, r7, 0x00000156}, false, al, "al r7 0x00000156", "al_r7_0x00000156"},
     {{al, r7, 0x000002ac}, false, al, "al r7 0x000002ac", "al_r7_0x000002ac"},
     {{al, r7, 0x00000558}, false, al, "al r7 0x00000558", "al_r7_0x00000558"},
     {{al, r7, 0x00000ab0}, false, al, "al r7 0x00000ab0", "al_r7_0x00000ab0"},
     {{al, r7, 0x00001560}, false, al, "al r7 0x00001560", "al_r7_0x00001560"},
     {{al, r7, 0x00002ac0}, false, al, "al r7 0x00002ac0", "al_r7_0x00002ac0"},
     {{al, r7, 0x00005580}, false, al, "al r7 0x00005580", "al_r7_0x00005580"},
     {{al, r7, 0x0000ab00}, false, al, "al r7 0x0000ab00", "al_r7_0x0000ab00"},
     {{al, r7, 0x00015600}, false, al, "al r7 0x00015600", "al_r7_0x00015600"},
     {{al, r7, 0x0002ac00}, false, al, "al r7 0x0002ac00", "al_r7_0x0002ac00"},
     {{al, r7, 0x00055800}, false, al, "al r7 0x00055800", "al_r7_0x00055800"},
     {{al, r7, 0x000ab000}, false, al, "al r7 0x000ab000", "al_r7_0x000ab000"},
     {{al, r7, 0x00156000}, false, al, "al r7 0x00156000", "al_r7_0x00156000"},
     {{al, r7, 0x002ac000}, false, al, "al r7 0x002ac000", "al_r7_0x002ac000"},
     {{al, r7, 0x00558000}, false, al, "al r7 0x00558000", "al_r7_0x00558000"},
     {{al, r7, 0x00ab0000}, false, al, "al r7 0x00ab0000", "al_r7_0x00ab0000"},
     {{al, r7, 0x01560000}, false, al, "al r7 0x01560000", "al_r7_0x01560000"},
     {{al, r7, 0x02ac0000}, false, al, "al r7 0x02ac0000", "al_r7_0x02ac0000"},
     {{al, r7, 0x05580000}, false, al, "al r7 0x05580000", "al_r7_0x05580000"},
     {{al, r7, 0x0ab00000}, false, al, "al r7 0x0ab00000", "al_r7_0x0ab00000"},
     {{al, r7, 0x15600000}, false, al, "al r7 0x15600000", "al_r7_0x15600000"},
     {{al, r7, 0x2ac00000}, false, al, "al r7 0x2ac00000", "al_r7_0x2ac00000"},
     {{al, r7, 0x55800000}, false, al, "al r7 0x55800000", "al_r7_0x55800000"},
     {{al, r7, 0xab000000}, false, al, "al r7 0xab000000", "al_r7_0xab000000"},
     {{al, r7, 0x000000ab}, false, al, "al r7 0x000000ab", "al_r7_0x000000ab"},
     {{al, r7, 0x00ab00ab}, false, al, "al r7 0x00ab00ab", "al_r7_0x00ab00ab"},
     {{al, r7, 0xab00ab00}, false, al, "al r7 0xab00ab00", "al_r7_0xab00ab00"},
     {{al, r7, 0xabababab}, false, al, "al r7 0xabababab", "al_r7_0xabababab"},
     {{al, r8, 0x000001fe}, false, al, "al r8 0x000001fe", "al_r8_0x000001fe"},
     {{al, r8, 0x000003fc}, false, al, "al r8 0x000003fc", "al_r8_0x000003fc"},
     {{al, r8, 0x000007f8}, false, al, "al r8 0x000007f8", "al_r8_0x000007f8"},
     {{al, r8, 0x00000ff0}, false, al, "al r8 0x00000ff0", "al_r8_0x00000ff0"},
     {{al, r8, 0x00001fe0}, false, al, "al r8 0x00001fe0", "al_r8_0x00001fe0"},
     {{al, r8, 0x00003fc0}, false, al, "al r8 0x00003fc0", "al_r8_0x00003fc0"},
     {{al, r8, 0x00007f80}, false, al, "al r8 0x00007f80", "al_r8_0x00007f80"},
     {{al, r8, 0x0000ff00}, false, al, "al r8 0x0000ff00", "al_r8_0x0000ff00"},
     {{al, r8, 0x0001fe00}, false, al, "al r8 0x0001fe00", "al_r8_0x0001fe00"},
     {{al, r8, 0x0003fc00}, false, al, "al r8 0x0003fc00", "al_r8_0x0003fc00"},
     {{al, r8, 0x0007f800}, false, al, "al r8 0x0007f800", "al_r8_0x0007f800"},
     {{al, r8, 0x000ff000}, false, al, "al r8 0x000ff000", "al_r8_0x000ff000"},
     {{al, r8, 0x001fe000}, false, al, "al r8 0x001fe000", "al_r8_0x001fe000"},
     {{al, r8, 0x003fc000}, false, al, "al r8 0x003fc000", "al_r8_0x003fc000"},
     {{al, r8, 0x007f8000}, false, al, "al r8 0x007f8000", "al_r8_0x007f8000"},
     {{al, r8, 0x00ff0000}, false, al, "al r8 0x00ff0000", "al_r8_0x00ff0000"},
     {{al, r8, 0x01fe0000}, false, al, "al r8 0x01fe0000", "al_r8_0x01fe0000"},
     {{al, r8, 0x03fc0000}, false, al, "al r8 0x03fc0000", "al_r8_0x03fc0000"},
     {{al, r8, 0x07f80000}, false, al, "al r8 0x07f80000", "al_r8_0x07f80000"},
     {{al, r8, 0x0ff00000}, false, al, "al r8 0x0ff00000", "al_r8_0x0ff00000"},
     {{al, r8, 0x1fe00000}, false, al, "al r8 0x1fe00000", "al_r8_0x1fe00000"},
     {{al, r8, 0x3fc00000}, false, al, "al r8 0x3fc00000", "al_r8_0x3fc00000"},
     {{al, r8, 0x7f800000}, false, al, "al r8 0x7f800000", "al_r8_0x7f800000"},
     {{al, r8, 0xff000000}, false, al, "al r8 0xff000000", "al_r8_0xff000000"},
     {{al, r8, 0x000000ff}, false, al, "al r8 0x000000ff", "al_r8_0x000000ff"},
     {{al, r8, 0x00ff00ff}, false, al, "al r8 0x00ff00ff", "al_r8_0x00ff00ff"},
     {{al, r8, 0xff00ff00}, false, al, "al r8 0xff00ff00", "al_r8_0xff00ff00"},
     {{al, r8, 0xffffffff}, false, al, "al r8 0xffffffff", "al_r8_0xffffffff"},
     {{al, r8, 0x00000156}, false, al, "al r8 0x00000156", "al_r8_0x00000156"},
     {{al, r8, 0x000002ac}, false, al, "al r8 0x000002ac", "al_r8_0x000002ac"},
     {{al, r8, 0x00000558}, false, al, "al r8 0x00000558", "al_r8_0x00000558"},
     {{al, r8, 0x00000ab0}, false, al, "al r8 0x00000ab0", "al_r8_0x00000ab0"},
     {{al, r8, 0x00001560}, false, al, "al r8 0x00001560", "al_r8_0x00001560"},
     {{al, r8, 0x00002ac0}, false, al, "al r8 0x00002ac0", "al_r8_0x00002ac0"},
     {{al, r8, 0x00005580}, false, al, "al r8 0x00005580", "al_r8_0x00005580"},
     {{al, r8, 0x0000ab00}, false, al, "al r8 0x0000ab00", "al_r8_0x0000ab00"},
     {{al, r8, 0x00015600}, false, al, "al r8 0x00015600", "al_r8_0x00015600"},
     {{al, r8, 0x0002ac00}, false, al, "al r8 0x0002ac00", "al_r8_0x0002ac00"},
     {{al, r8, 0x00055800}, false, al, "al r8 0x00055800", "al_r8_0x00055800"},
     {{al, r8, 0x000ab000}, false, al, "al r8 0x000ab000", "al_r8_0x000ab000"},
     {{al, r8, 0x00156000}, false, al, "al r8 0x00156000", "al_r8_0x00156000"},
     {{al, r8, 0x002ac000}, false, al, "al r8 0x002ac000", "al_r8_0x002ac000"},
     {{al, r8, 0x00558000}, false, al, "al r8 0x00558000", "al_r8_0x00558000"},
     {{al, r8, 0x00ab0000}, false, al, "al r8 0x00ab0000", "al_r8_0x00ab0000"},
     {{al, r8, 0x01560000}, false, al, "al r8 0x01560000", "al_r8_0x01560000"},
     {{al, r8, 0x02ac0000}, false, al, "al r8 0x02ac0000", "al_r8_0x02ac0000"},
     {{al, r8, 0x05580000}, false, al, "al r8 0x05580000", "al_r8_0x05580000"},
     {{al, r8, 0x0ab00000}, false, al, "al r8 0x0ab00000", "al_r8_0x0ab00000"},
     {{al, r8, 0x15600000}, false, al, "al r8 0x15600000", "al_r8_0x15600000"},
     {{al, r8, 0x2ac00000}, false, al, "al r8 0x2ac00000", "al_r8_0x2ac00000"},
     {{al, r8, 0x55800000}, false, al, "al r8 0x55800000", "al_r8_0x55800000"},
     {{al, r8, 0xab000000}, false, al, "al r8 0xab000000", "al_r8_0xab000000"},
     {{al, r8, 0x000000ab}, false, al, "al r8 0x000000ab", "al_r8_0x000000ab"},
     {{al, r8, 0x00ab00ab}, false, al, "al r8 0x00ab00ab", "al_r8_0x00ab00ab"},
     {{al, r8, 0xab00ab00}, false, al, "al r8 0xab00ab00", "al_r8_0xab00ab00"},
     {{al, r8, 0xabababab}, false, al, "al r8 0xabababab", "al_r8_0xabababab"},
     {{al, r9, 0x000001fe}, false, al, "al r9 0x000001fe", "al_r9_0x000001fe"},
     {{al, r9, 0x000003fc}, false, al, "al r9 0x000003fc", "al_r9_0x000003fc"},
     {{al, r9, 0x000007f8}, false, al, "al r9 0x000007f8", "al_r9_0x000007f8"},
     {{al, r9, 0x00000ff0}, false, al, "al r9 0x00000ff0", "al_r9_0x00000ff0"},
     {{al, r9, 0x00001fe0}, false, al, "al r9 0x00001fe0", "al_r9_0x00001fe0"},
     {{al, r9, 0x00003fc0}, false, al, "al r9 0x00003fc0", "al_r9_0x00003fc0"},
     {{al, r9, 0x00007f80}, false, al, "al r9 0x00007f80", "al_r9_0x00007f80"},
     {{al, r9, 0x0000ff00}, false, al, "al r9 0x0000ff00", "al_r9_0x0000ff00"},
     {{al, r9, 0x0001fe00}, false, al, "al r9 0x0001fe00", "al_r9_0x0001fe00"},
     {{al, r9, 0x0003fc00}, false, al, "al r9 0x0003fc00", "al_r9_0x0003fc00"},
     {{al, r9, 0x0007f800}, false, al, "al r9 0x0007f800", "al_r9_0x0007f800"},
     {{al, r9, 0x000ff000}, false, al, "al r9 0x000ff000", "al_r9_0x000ff000"},
     {{al, r9, 0x001fe000}, false, al, "al r9 0x001fe000", "al_r9_0x001fe000"},
     {{al, r9, 0x003fc000}, false, al, "al r9 0x003fc000", "al_r9_0x003fc000"},
     {{al, r9, 0x007f8000}, false, al, "al r9 0x007f8000", "al_r9_0x007f8000"},
     {{al, r9, 0x00ff0000}, false, al, "al r9 0x00ff0000", "al_r9_0x00ff0000"},
     {{al, r9, 0x01fe0000}, false, al, "al r9 0x01fe0000", "al_r9_0x01fe0000"},
     {{al, r9, 0x03fc0000}, false, al, "al r9 0x03fc0000", "al_r9_0x03fc0000"},
     {{al, r9, 0x07f80000}, false, al, "al r9 0x07f80000", "al_r9_0x07f80000"},
     {{al, r9, 0x0ff00000}, false, al, "al r9 0x0ff00000", "al_r9_0x0ff00000"},
     {{al, r9, 0x1fe00000}, false, al, "al r9 0x1fe00000", "al_r9_0x1fe00000"},
     {{al, r9, 0x3fc00000}, false, al, "al r9 0x3fc00000", "al_r9_0x3fc00000"},
     {{al, r9, 0x7f800000}, false, al, "al r9 0x7f800000", "al_r9_0x7f800000"},
     {{al, r9, 0xff000000}, false, al, "al r9 0xff000000", "al_r9_0xff000000"},
     {{al, r9, 0x000000ff}, false, al, "al r9 0x000000ff", "al_r9_0x000000ff"},
     {{al, r9, 0x00ff00ff}, false, al, "al r9 0x00ff00ff", "al_r9_0x00ff00ff"},
     {{al, r9, 0xff00ff00}, false, al, "al r9 0xff00ff00", "al_r9_0xff00ff00"},
     {{al, r9, 0xffffffff}, false, al, "al r9 0xffffffff", "al_r9_0xffffffff"},
     {{al, r9, 0x00000156}, false, al, "al r9 0x00000156", "al_r9_0x00000156"},
     {{al, r9, 0x000002ac}, false, al, "al r9 0x000002ac", "al_r9_0x000002ac"},
     {{al, r9, 0x00000558}, false, al, "al r9 0x00000558", "al_r9_0x00000558"},
     {{al, r9, 0x00000ab0}, false, al, "al r9 0x00000ab0", "al_r9_0x00000ab0"},
     {{al, r9, 0x00001560}, false, al, "al r9 0x00001560", "al_r9_0x00001560"},
     {{al, r9, 0x00002ac0}, false, al, "al r9 0x00002ac0", "al_r9_0x00002ac0"},
     {{al, r9, 0x00005580}, false, al, "al r9 0x00005580", "al_r9_0x00005580"},
     {{al, r9, 0x0000ab00}, false, al, "al r9 0x0000ab00", "al_r9_0x0000ab00"},
     {{al, r9, 0x00015600}, false, al, "al r9 0x00015600", "al_r9_0x00015600"},
     {{al, r9, 0x0002ac00}, false, al, "al r9 0x0002ac00", "al_r9_0x0002ac00"},
     {{al, r9, 0x00055800}, false, al, "al r9 0x00055800", "al_r9_0x00055800"},
     {{al, r9, 0x000ab000}, false, al, "al r9 0x000ab000", "al_r9_0x000ab000"},
     {{al, r9, 0x00156000}, false, al, "al r9 0x00156000", "al_r9_0x00156000"},
     {{al, r9, 0x002ac000}, false, al, "al r9 0x002ac000", "al_r9_0x002ac000"},
     {{al, r9, 0x00558000}, false, al, "al r9 0x00558000", "al_r9_0x00558000"},
     {{al, r9, 0x00ab0000}, false, al, "al r9 0x00ab0000", "al_r9_0x00ab0000"},
     {{al, r9, 0x01560000}, false, al, "al r9 0x01560000", "al_r9_0x01560000"},
     {{al, r9, 0x02ac0000}, false, al, "al r9 0x02ac0000", "al_r9_0x02ac0000"},
     {{al, r9, 0x05580000}, false, al, "al r9 0x05580000", "al_r9_0x05580000"},
     {{al, r9, 0x0ab00000}, false, al, "al r9 0x0ab00000", "al_r9_0x0ab00000"},
     {{al, r9, 0x15600000}, false, al, "al r9 0x15600000", "al_r9_0x15600000"},
     {{al, r9, 0x2ac00000}, false, al, "al r9 0x2ac00000", "al_r9_0x2ac00000"},
     {{al, r9, 0x55800000}, false, al, "al r9 0x55800000", "al_r9_0x55800000"},
     {{al, r9, 0xab000000}, false, al, "al r9 0xab000000", "al_r9_0xab000000"},
     {{al, r9, 0x000000ab}, false, al, "al r9 0x000000ab", "al_r9_0x000000ab"},
     {{al, r9, 0x00ab00ab}, false, al, "al r9 0x00ab00ab", "al_r9_0x00ab00ab"},
     {{al, r9, 0xab00ab00}, false, al, "al r9 0xab00ab00", "al_r9_0xab00ab00"},
     {{al, r9, 0xabababab}, false, al, "al r9 0xabababab", "al_r9_0xabababab"},
     {{al, r10, 0x000001fe},
      false,
      al,
      "al r10 0x000001fe",
      "al_r10_"
      "0x000001fe"},
     {{al, r10, 0x000003fc},
      false,
      al,
      "al r10 0x000003fc",
      "al_r10_"
      "0x000003fc"},
     {{al, r10, 0x000007f8},
      false,
      al,
      "al r10 0x000007f8",
      "al_r10_"
      "0x000007f8"},
     {{al, r10, 0x00000ff0},
      false,
      al,
      "al r10 0x00000ff0",
      "al_r10_"
      "0x00000ff0"},
     {{al, r10, 0x00001fe0},
      false,
      al,
      "al r10 0x00001fe0",
      "al_r10_"
      "0x00001fe0"},
     {{al, r10, 0x00003fc0},
      false,
      al,
      "al r10 0x00003fc0",
      "al_r10_"
      "0x00003fc0"},
     {{al, r10, 0x00007f80},
      false,
      al,
      "al r10 0x00007f80",
      "al_r10_"
      "0x00007f80"},
     {{al, r10, 0x0000ff00},
      false,
      al,
      "al r10 0x0000ff00",
      "al_r10_"
      "0x0000ff00"},
     {{al, r10, 0x0001fe00},
      false,
      al,
      "al r10 0x0001fe00",
      "al_r10_"
      "0x0001fe00"},
     {{al, r10, 0x0003fc00},
      false,
      al,
      "al r10 0x0003fc00",
      "al_r10_"
      "0x0003fc00"},
     {{al, r10, 0x0007f800},
      false,
      al,
      "al r10 0x0007f800",
      "al_r10_"
      "0x0007f800"},
     {{al, r10, 0x000ff000},
      false,
      al,
      "al r10 0x000ff000",
      "al_r10_"
      "0x000ff000"},
     {{al, r10, 0x001fe000},
      false,
      al,
      "al r10 0x001fe000",
      "al_r10_"
      "0x001fe000"},
     {{al, r10, 0x003fc000},
      false,
      al,
      "al r10 0x003fc000",
      "al_r10_"
      "0x003fc000"},
     {{al, r10, 0x007f8000},
      false,
      al,
      "al r10 0x007f8000",
      "al_r10_"
      "0x007f8000"},
     {{al, r10, 0x00ff0000},
      false,
      al,
      "al r10 0x00ff0000",
      "al_r10_"
      "0x00ff0000"},
     {{al, r10, 0x01fe0000},
      false,
      al,
      "al r10 0x01fe0000",
      "al_r10_"
      "0x01fe0000"},
     {{al, r10, 0x03fc0000},
      false,
      al,
      "al r10 0x03fc0000",
      "al_r10_"
      "0x03fc0000"},
     {{al, r10, 0x07f80000},
      false,
      al,
      "al r10 0x07f80000",
      "al_r10_"
      "0x07f80000"},
     {{al, r10, 0x0ff00000},
      false,
      al,
      "al r10 0x0ff00000",
      "al_r10_"
      "0x0ff00000"},
     {{al, r10, 0x1fe00000},
      false,
      al,
      "al r10 0x1fe00000",
      "al_r10_"
      "0x1fe00000"},
     {{al, r10, 0x3fc00000},
      false,
      al,
      "al r10 0x3fc00000",
      "al_r10_"
      "0x3fc00000"},
     {{al, r10, 0x7f800000},
      false,
      al,
      "al r10 0x7f800000",
      "al_r10_"
      "0x7f800000"},
     {{al, r10, 0xff000000},
      false,
      al,
      "al r10 0xff000000",
      "al_r10_"
      "0xff000000"},
     {{al, r10, 0x000000ff},
      false,
      al,
      "al r10 0x000000ff",
      "al_r10_"
      "0x000000ff"},
     {{al, r10, 0x00ff00ff},
      false,
      al,
      "al r10 0x00ff00ff",
      "al_r10_"
      "0x00ff00ff"},
     {{al, r10, 0xff00ff00},
      false,
      al,
      "al r10 0xff00ff00",
      "al_r10_"
      "0xff00ff00"},
     {{al, r10, 0xffffffff},
      false,
      al,
      "al r10 0xffffffff",
      "al_r10_"
      "0xffffffff"},
     {{al, r10, 0x00000156},
      false,
      al,
      "al r10 0x00000156",
      "al_r10_"
      "0x00000156"},
     {{al, r10, 0x000002ac},
      false,
      al,
      "al r10 0x000002ac",
      "al_r10_"
      "0x000002ac"},
     {{al, r10, 0x00000558},
      false,
      al,
      "al r10 0x00000558",
      "al_r10_"
      "0x00000558"},
     {{al, r10, 0x00000ab0},
      false,
      al,
      "al r10 0x00000ab0",
      "al_r10_"
      "0x00000ab0"},
     {{al, r10, 0x00001560},
      false,
      al,
      "al r10 0x00001560",
      "al_r10_"
      "0x00001560"},
     {{al, r10, 0x00002ac0},
      false,
      al,
      "al r10 0x00002ac0",
      "al_r10_"
      "0x00002ac0"},
     {{al, r10, 0x00005580},
      false,
      al,
      "al r10 0x00005580",
      "al_r10_"
      "0x00005580"},
     {{al, r10, 0x0000ab00},
      false,
      al,
      "al r10 0x0000ab00",
      "al_r10_"
      "0x0000ab00"},
     {{al, r10, 0x00015600},
      false,
      al,
      "al r10 0x00015600",
      "al_r10_"
      "0x00015600"},
     {{al, r10, 0x0002ac00},
      false,
      al,
      "al r10 0x0002ac00",
      "al_r10_"
      "0x0002ac00"},
     {{al, r10, 0x00055800},
      false,
      al,
      "al r10 0x00055800",
      "al_r10_"
      "0x00055800"},
     {{al, r10, 0x000ab000},
      false,
      al,
      "al r10 0x000ab000",
      "al_r10_"
      "0x000ab000"},
     {{al, r10, 0x00156000},
      false,
      al,
      "al r10 0x00156000",
      "al_r10_"
      "0x00156000"},
     {{al, r10, 0x002ac000},
      false,
      al,
      "al r10 0x002ac000",
      "al_r10_"
      "0x002ac000"},
     {{al, r10, 0x00558000},
      false,
      al,
      "al r10 0x00558000",
      "al_r10_"
      "0x00558000"},
     {{al, r10, 0x00ab0000},
      false,
      al,
      "al r10 0x00ab0000",
      "al_r10_"
      "0x00ab0000"},
     {{al, r10, 0x01560000},
      false,
      al,
      "al r10 0x01560000",
      "al_r10_"
      "0x01560000"},
     {{al, r10, 0x02ac0000},
      false,
      al,
      "al r10 0x02ac0000",
      "al_r10_"
      "0x02ac0000"},
     {{al, r10, 0x05580000},
      false,
      al,
      "al r10 0x05580000",
      "al_r10_"
      "0x05580000"},
     {{al, r10, 0x0ab00000},
      false,
      al,
      "al r10 0x0ab00000",
      "al_r10_"
      "0x0ab00000"},
     {{al, r10, 0x15600000},
      false,
      al,
      "al r10 0x15600000",
      "al_r10_"
      "0x15600000"},
     {{al, r10, 0x2ac00000},
      false,
      al,
      "al r10 0x2ac00000",
      "al_r10_"
      "0x2ac00000"},
     {{al, r10, 0x55800000},
      false,
      al,
      "al r10 0x55800000",
      "al_r10_"
      "0x55800000"},
     {{al, r10, 0xab000000},
      false,
      al,
      "al r10 0xab000000",
      "al_r10_"
      "0xab000000"},
     {{al, r10, 0x000000ab},
      false,
      al,
      "al r10 0x000000ab",
      "al_r10_"
      "0x000000ab"},
     {{al, r10, 0x00ab00ab},
      false,
      al,
      "al r10 0x00ab00ab",
      "al_r10_"
      "0x00ab00ab"},
     {{al, r10, 0xab00ab00},
      false,
      al,
      "al r10 0xab00ab00",
      "al_r10_"
      "0xab00ab00"},
     {{al, r10, 0xabababab},
      false,
      al,
      "al r10 0xabababab",
      "al_r10_"
      "0xabababab"},
     {{al, r11, 0x000001fe},
      false,
      al,
      "al r11 0x000001fe",
      "al_r11_"
      "0x000001fe"},
     {{al, r11, 0x000003fc},
      false,
      al,
      "al r11 0x000003fc",
      "al_r11_"
      "0x000003fc"},
     {{al, r11, 0x000007f8},
      false,
      al,
      "al r11 0x000007f8",
      "al_r11_"
      "0x000007f8"},
     {{al, r11, 0x00000ff0},
      false,
      al,
      "al r11 0x00000ff0",
      "al_r11_"
      "0x00000ff0"},
     {{al, r11, 0x00001fe0},
      false,
      al,
      "al r11 0x00001fe0",
      "al_r11_"
      "0x00001fe0"},
     {{al, r11, 0x00003fc0},
      false,
      al,
      "al r11 0x00003fc0",
      "al_r11_"
      "0x00003fc0"},
     {{al, r11, 0x00007f80},
      false,
      al,
      "al r11 0x00007f80",
      "al_r11_"
      "0x00007f80"},
     {{al, r11, 0x0000ff00},
      false,
      al,
      "al r11 0x0000ff00",
      "al_r11_"
      "0x0000ff00"},
     {{al, r11, 0x0001fe00},
      false,
      al,
      "al r11 0x0001fe00",
      "al_r11_"
      "0x0001fe00"},
     {{al, r11, 0x0003fc00},
      false,
      al,
      "al r11 0x0003fc00",
      "al_r11_"
      "0x0003fc00"},
     {{al, r11, 0x0007f800},
      false,
      al,
      "al r11 0x0007f800",
      "al_r11_"
      "0x0007f800"},
     {{al, r11, 0x000ff000},
      false,
      al,
      "al r11 0x000ff000",
      "al_r11_"
      "0x000ff000"},
     {{al, r11, 0x001fe000},
      false,
      al,
      "al r11 0x001fe000",
      "al_r11_"
      "0x001fe000"},
     {{al, r11, 0x003fc000},
      false,
      al,
      "al r11 0x003fc000",
      "al_r11_"
      "0x003fc000"},
     {{al, r11, 0x007f8000},
      false,
      al,
      "al r11 0x007f8000",
      "al_r11_"
      "0x007f8000"},
     {{al, r11, 0x00ff0000},
      false,
      al,
      "al r11 0x00ff0000",
      "al_r11_"
      "0x00ff0000"},
     {{al, r11, 0x01fe0000},
      false,
      al,
      "al r11 0x01fe0000",
      "al_r11_"
      "0x01fe0000"},
     {{al, r11, 0x03fc0000},
      false,
      al,
      "al r11 0x03fc0000",
      "al_r11_"
      "0x03fc0000"},
     {{al, r11, 0x07f80000},
      false,
      al,
      "al r11 0x07f80000",
      "al_r11_"
      "0x07f80000"},
     {{al, r11, 0x0ff00000},
      false,
      al,
      "al r11 0x0ff00000",
      "al_r11_"
      "0x0ff00000"},
     {{al, r11, 0x1fe00000},
      false,
      al,
      "al r11 0x1fe00000",
      "al_r11_"
      "0x1fe00000"},
     {{al, r11, 0x3fc00000},
      false,
      al,
      "al r11 0x3fc00000",
      "al_r11_"
      "0x3fc00000"},
     {{al, r11, 0x7f800000},
      false,
      al,
      "al r11 0x7f800000",
      "al_r11_"
      "0x7f800000"},
     {{al, r11, 0xff000000},
      false,
      al,
      "al r11 0xff000000",
      "al_r11_"
      "0xff000000"},
     {{al, r11, 0x000000ff},
      false,
      al,
      "al r11 0x000000ff",
      "al_r11_"
      "0x000000ff"},
     {{al, r11, 0x00ff00ff},
      false,
      al,
      "al r11 0x00ff00ff",
      "al_r11_"
      "0x00ff00ff"},
     {{al, r11, 0xff00ff00},
      false,
      al,
      "al r11 0xff00ff00",
      "al_r11_"
      "0xff00ff00"},
     {{al, r11, 0xffffffff},
      false,
      al,
      "al r11 0xffffffff",
      "al_r11_"
      "0xffffffff"},
     {{al, r11, 0x00000156},
      false,
      al,
      "al r11 0x00000156",
      "al_r11_"
      "0x00000156"},
     {{al, r11, 0x000002ac},
      false,
      al,
      "al r11 0x000002ac",
      "al_r11_"
      "0x000002ac"},
     {{al, r11, 0x00000558},
      false,
      al,
      "al r11 0x00000558",
      "al_r11_"
      "0x00000558"},
     {{al, r11, 0x00000ab0},
      false,
      al,
      "al r11 0x00000ab0",
      "al_r11_"
      "0x00000ab0"},
     {{al, r11, 0x00001560},
      false,
      al,
      "al r11 0x00001560",
      "al_r11_"
      "0x00001560"},
     {{al, r11, 0x00002ac0},
      false,
      al,
      "al r11 0x00002ac0",
      "al_r11_"
      "0x00002ac0"},
     {{al, r11, 0x00005580},
      false,
      al,
      "al r11 0x00005580",
      "al_r11_"
      "0x00005580"},
     {{al, r11, 0x0000ab00},
      false,
      al,
      "al r11 0x0000ab00",
      "al_r11_"
      "0x0000ab00"},
     {{al, r11, 0x00015600},
      false,
      al,
      "al r11 0x00015600",
      "al_r11_"
      "0x00015600"},
     {{al, r11, 0x0002ac00},
      false,
      al,
      "al r11 0x0002ac00",
      "al_r11_"
      "0x0002ac00"},
     {{al, r11, 0x00055800},
      false,
      al,
      "al r11 0x00055800",
      "al_r11_"
      "0x00055800"},
     {{al, r11, 0x000ab000},
      false,
      al,
      "al r11 0x000ab000",
      "al_r11_"
      "0x000ab000"},
     {{al, r11, 0x00156000},
      false,
      al,
      "al r11 0x00156000",
      "al_r11_"
      "0x00156000"},
     {{al, r11, 0x002ac000},
      false,
      al,
      "al r11 0x002ac000",
      "al_r11_"
      "0x002ac000"},
     {{al, r11, 0x00558000},
      false,
      al,
      "al r11 0x00558000",
      "al_r11_"
      "0x00558000"},
     {{al, r11, 0x00ab0000},
      false,
      al,
      "al r11 0x00ab0000",
      "al_r11_"
      "0x00ab0000"},
     {{al, r11, 0x01560000},
      false,
      al,
      "al r11 0x01560000",
      "al_r11_"
      "0x01560000"},
     {{al, r11, 0x02ac0000},
      false,
      al,
      "al r11 0x02ac0000",
      "al_r11_"
      "0x02ac0000"},
     {{al, r11, 0x05580000},
      false,
      al,
      "al r11 0x05580000",
      "al_r11_"
      "0x05580000"},
     {{al, r11, 0x0ab00000},
      false,
      al,
      "al r11 0x0ab00000",
      "al_r11_"
      "0x0ab00000"},
     {{al, r11, 0x15600000},
      false,
      al,
      "al r11 0x15600000",
      "al_r11_"
      "0x15600000"},
     {{al, r11, 0x2ac00000},
      false,
      al,
      "al r11 0x2ac00000",
      "al_r11_"
      "0x2ac00000"},
     {{al, r11, 0x55800000},
      false,
      al,
      "al r11 0x55800000",
      "al_r11_"
      "0x55800000"},
     {{al, r11, 0xab000000},
      false,
      al,
      "al r11 0xab000000",
      "al_r11_"
      "0xab000000"},
     {{al, r11, 0x000000ab},
      false,
      al,
      "al r11 0x000000ab",
      "al_r11_"
      "0x000000ab"},
     {{al, r11, 0x00ab00ab},
      false,
      al,
      "al r11 0x00ab00ab",
      "al_r11_"
      "0x00ab00ab"},
     {{al, r11, 0xab00ab00},
      false,
      al,
      "al r11 0xab00ab00",
      "al_r11_"
      "0xab00ab00"},
     {{al, r11, 0xabababab},
      false,
      al,
      "al r11 0xabababab",
      "al_r11_"
      "0xabababab"},
     {{al, r12, 0x000001fe},
      false,
      al,
      "al r12 0x000001fe",
      "al_r12_"
      "0x000001fe"},
     {{al, r12, 0x000003fc},
      false,
      al,
      "al r12 0x000003fc",
      "al_r12_"
      "0x000003fc"},
     {{al, r12, 0x000007f8},
      false,
      al,
      "al r12 0x000007f8",
      "al_r12_"
      "0x000007f8"},
     {{al, r12, 0x00000ff0},
      false,
      al,
      "al r12 0x00000ff0",
      "al_r12_"
      "0x00000ff0"},
     {{al, r12, 0x00001fe0},
      false,
      al,
      "al r12 0x00001fe0",
      "al_r12_"
      "0x00001fe0"},
     {{al, r12, 0x00003fc0},
      false,
      al,
      "al r12 0x00003fc0",
      "al_r12_"
      "0x00003fc0"},
     {{al, r12, 0x00007f80},
      false,
      al,
      "al r12 0x00007f80",
      "al_r12_"
      "0x00007f80"},
     {{al, r12, 0x0000ff00},
      false,
      al,
      "al r12 0x0000ff00",
      "al_r12_"
      "0x0000ff00"},
     {{al, r12, 0x0001fe00},
      false,
      al,
      "al r12 0x0001fe00",
      "al_r12_"
      "0x0001fe00"},
     {{al, r12, 0x0003fc00},
      false,
      al,
      "al r12 0x0003fc00",
      "al_r12_"
      "0x0003fc00"},
     {{al, r12, 0x0007f800},
      false,
      al,
      "al r12 0x0007f800",
      "al_r12_"
      "0x0007f800"},
     {{al, r12, 0x000ff000},
      false,
      al,
      "al r12 0x000ff000",
      "al_r12_"
      "0x000ff000"},
     {{al, r12, 0x001fe000},
      false,
      al,
      "al r12 0x001fe000",
      "al_r12_"
      "0x001fe000"},
     {{al, r12, 0x003fc000},
      false,
      al,
      "al r12 0x003fc000",
      "al_r12_"
      "0x003fc000"},
     {{al, r12, 0x007f8000},
      false,
      al,
      "al r12 0x007f8000",
      "al_r12_"
      "0x007f8000"},
     {{al, r12, 0x00ff0000},
      false,
      al,
      "al r12 0x00ff0000",
      "al_r12_"
      "0x00ff0000"},
     {{al, r12, 0x01fe0000},
      false,
      al,
      "al r12 0x01fe0000",
      "al_r12_"
      "0x01fe0000"},
     {{al, r12, 0x03fc0000},
      false,
      al,
      "al r12 0x03fc0000",
      "al_r12_"
      "0x03fc0000"},
     {{al, r12, 0x07f80000},
      false,
      al,
      "al r12 0x07f80000",
      "al_r12_"
      "0x07f80000"},
     {{al, r12, 0x0ff00000},
      false,
      al,
      "al r12 0x0ff00000",
      "al_r12_"
      "0x0ff00000"},
     {{al, r12, 0x1fe00000},
      false,
      al,
      "al r12 0x1fe00000",
      "al_r12_"
      "0x1fe00000"},
     {{al, r12, 0x3fc00000},
      false,
      al,
      "al r12 0x3fc00000",
      "al_r12_"
      "0x3fc00000"},
     {{al, r12, 0x7f800000},
      false,
      al,
      "al r12 0x7f800000",
      "al_r12_"
      "0x7f800000"},
     {{al, r12, 0xff000000},
      false,
      al,
      "al r12 0xff000000",
      "al_r12_"
      "0xff000000"},
     {{al, r12, 0x000000ff},
      false,
      al,
      "al r12 0x000000ff",
      "al_r12_"
      "0x000000ff"},
     {{al, r12, 0x00ff00ff},
      false,
      al,
      "al r12 0x00ff00ff",
      "al_r12_"
      "0x00ff00ff"},
     {{al, r12, 0xff00ff00},
      false,
      al,
      "al r12 0xff00ff00",
      "al_r12_"
      "0xff00ff00"},
     {{al, r12, 0xffffffff},
      false,
      al,
      "al r12 0xffffffff",
      "al_r12_"
      "0xffffffff"},
     {{al, r12, 0x00000156},
      false,
      al,
      "al r12 0x00000156",
      "al_r12_"
      "0x00000156"},
     {{al, r12, 0x000002ac},
      false,
      al,
      "al r12 0x000002ac",
      "al_r12_"
      "0x000002ac"},
     {{al, r12, 0x00000558},
      false,
      al,
      "al r12 0x00000558",
      "al_r12_"
      "0x00000558"},
     {{al, r12, 0x00000ab0},
      false,
      al,
      "al r12 0x00000ab0",
      "al_r12_"
      "0x00000ab0"},
     {{al, r12, 0x00001560},
      false,
      al,
      "al r12 0x00001560",
      "al_r12_"
      "0x00001560"},
     {{al, r12, 0x00002ac0},
      false,
      al,
      "al r12 0x00002ac0",
      "al_r12_"
      "0x00002ac0"},
     {{al, r12, 0x00005580},
      false,
      al,
      "al r12 0x00005580",
      "al_r12_"
      "0x00005580"},
     {{al, r12, 0x0000ab00},
      false,
      al,
      "al r12 0x0000ab00",
      "al_r12_"
      "0x0000ab00"},
     {{al, r12, 0x00015600},
      false,
      al,
      "al r12 0x00015600",
      "al_r12_"
      "0x00015600"},
     {{al, r12, 0x0002ac00},
      false,
      al,
      "al r12 0x0002ac00",
      "al_r12_"
      "0x0002ac00"},
     {{al, r12, 0x00055800},
      false,
      al,
      "al r12 0x00055800",
      "al_r12_"
      "0x00055800"},
     {{al, r12, 0x000ab000},
      false,
      al,
      "al r12 0x000ab000",
      "al_r12_"
      "0x000ab000"},
     {{al, r12, 0x00156000},
      false,
      al,
      "al r12 0x00156000",
      "al_r12_"
      "0x00156000"},
     {{al, r12, 0x002ac000},
      false,
      al,
      "al r12 0x002ac000",
      "al_r12_"
      "0x002ac000"},
     {{al, r12, 0x00558000},
      false,
      al,
      "al r12 0x00558000",
      "al_r12_"
      "0x00558000"},
     {{al, r12, 0x00ab0000},
      false,
      al,
      "al r12 0x00ab0000",
      "al_r12_"
      "0x00ab0000"},
     {{al, r12, 0x01560000},
      false,
      al,
      "al r12 0x01560000",
      "al_r12_"
      "0x01560000"},
     {{al, r12, 0x02ac0000},
      false,
      al,
      "al r12 0x02ac0000",
      "al_r12_"
      "0x02ac0000"},
     {{al, r12, 0x05580000},
      false,
      al,
      "al r12 0x05580000",
      "al_r12_"
      "0x05580000"},
     {{al, r12, 0x0ab00000},
      false,
      al,
      "al r12 0x0ab00000",
      "al_r12_"
      "0x0ab00000"},
     {{al, r12, 0x15600000},
      false,
      al,
      "al r12 0x15600000",
      "al_r12_"
      "0x15600000"},
     {{al, r12, 0x2ac00000},
      false,
      al,
      "al r12 0x2ac00000",
      "al_r12_"
      "0x2ac00000"},
     {{al, r12, 0x55800000},
      false,
      al,
      "al r12 0x55800000",
      "al_r12_"
      "0x55800000"},
     {{al, r12, 0xab000000},
      false,
      al,
      "al r12 0xab000000",
      "al_r12_"
      "0xab000000"},
     {{al, r12, 0x000000ab},
      false,
      al,
      "al r12 0x000000ab",
      "al_r12_"
      "0x000000ab"},
     {{al, r12, 0x00ab00ab},
      false,
      al,
      "al r12 0x00ab00ab",
      "al_r12_"
      "0x00ab00ab"},
     {{al, r12, 0xab00ab00},
      false,
      al,
      "al r12 0xab00ab00",
      "al_r12_"
      "0xab00ab00"},
     {{al, r12, 0xabababab},
      false,
      al,
      "al r12 0xabababab",
      "al_r12_"
      "0xabababab"},
     {{al, r13, 0x000001fe},
      false,
      al,
      "al r13 0x000001fe",
      "al_r13_"
      "0x000001fe"},
     {{al, r13, 0x000003fc},
      false,
      al,
      "al r13 0x000003fc",
      "al_r13_"
      "0x000003fc"},
     {{al, r13, 0x000007f8},
      false,
      al,
      "al r13 0x000007f8",
      "al_r13_"
      "0x000007f8"},
     {{al, r13, 0x00000ff0},
      false,
      al,
      "al r13 0x00000ff0",
      "al_r13_"
      "0x00000ff0"},
     {{al, r13, 0x00001fe0},
      false,
      al,
      "al r13 0x00001fe0",
      "al_r13_"
      "0x00001fe0"},
     {{al, r13, 0x00003fc0},
      false,
      al,
      "al r13 0x00003fc0",
      "al_r13_"
      "0x00003fc0"},
     {{al, r13, 0x00007f80},
      false,
      al,
      "al r13 0x00007f80",
      "al_r13_"
      "0x00007f80"},
     {{al, r13, 0x0000ff00},
      false,
      al,
      "al r13 0x0000ff00",
      "al_r13_"
      "0x0000ff00"},
     {{al, r13, 0x0001fe00},
      false,
      al,
      "al r13 0x0001fe00",
      "al_r13_"
      "0x0001fe00"},
     {{al, r13, 0x0003fc00},
      false,
      al,
      "al r13 0x0003fc00",
      "al_r13_"
      "0x0003fc00"},
     {{al, r13, 0x0007f800},
      false,
      al,
      "al r13 0x0007f800",
      "al_r13_"
      "0x0007f800"},
     {{al, r13, 0x000ff000},
      false,
      al,
      "al r13 0x000ff000",
      "al_r13_"
      "0x000ff000"},
     {{al, r13, 0x001fe000},
      false,
      al,
      "al r13 0x001fe000",
      "al_r13_"
      "0x001fe000"},
     {{al, r13, 0x003fc000},
      false,
      al,
      "al r13 0x003fc000",
      "al_r13_"
      "0x003fc000"},
     {{al, r13, 0x007f8000},
      false,
      al,
      "al r13 0x007f8000",
      "al_r13_"
      "0x007f8000"},
     {{al, r13, 0x00ff0000},
      false,
      al,
      "al r13 0x00ff0000",
      "al_r13_"
      "0x00ff0000"},
     {{al, r13, 0x01fe0000},
      false,
      al,
      "al r13 0x01fe0000",
      "al_r13_"
      "0x01fe0000"},
     {{al, r13, 0x03fc0000},
      false,
      al,
      "al r13 0x03fc0000",
      "al_r13_"
      "0x03fc0000"},
     {{al, r13, 0x07f80000},
      false,
      al,
      "al r13 0x07f80000",
      "al_r13_"
      "0x07f80000"},
     {{al, r13, 0x0ff00000},
      false,
      al,
      "al r13 0x0ff00000",
      "al_r13_"
      "0x0ff00000"},
     {{al, r13, 0x1fe00000},
      false,
      al,
      "al r13 0x1fe00000",
      "al_r13_"
      "0x1fe00000"},
     {{al, r13, 0x3fc00000},
      false,
      al,
      "al r13 0x3fc00000",
      "al_r13_"
      "0x3fc00000"},
     {{al, r13, 0x7f800000},
      false,
      al,
      "al r13 0x7f800000",
      "al_r13_"
      "0x7f800000"},
     {{al, r13, 0xff000000},
      false,
      al,
      "al r13 0xff000000",
      "al_r13_"
      "0xff000000"},
     {{al, r13, 0x000000ff},
      false,
      al,
      "al r13 0x000000ff",
      "al_r13_"
      "0x000000ff"},
     {{al, r13, 0x00ff00ff},
      false,
      al,
      "al r13 0x00ff00ff",
      "al_r13_"
      "0x00ff00ff"},
     {{al, r13, 0xff00ff00},
      false,
      al,
      "al r13 0xff00ff00",
      "al_r13_"
      "0xff00ff00"},
     {{al, r13, 0xffffffff},
      false,
      al,
      "al r13 0xffffffff",
      "al_r13_"
      "0xffffffff"},
     {{al, r13, 0x00000156},
      false,
      al,
      "al r13 0x00000156",
      "al_r13_"
      "0x00000156"},
     {{al, r13, 0x000002ac},
      false,
      al,
      "al r13 0x000002ac",
      "al_r13_"
      "0x000002ac"},
     {{al, r13, 0x00000558},
      false,
      al,
      "al r13 0x00000558",
      "al_r13_"
      "0x00000558"},
     {{al, r13, 0x00000ab0},
      false,
      al,
      "al r13 0x00000ab0",
      "al_r13_"
      "0x00000ab0"},
     {{al, r13, 0x00001560},
      false,
      al,
      "al r13 0x00001560",
      "al_r13_"
      "0x00001560"},
     {{al, r13, 0x00002ac0},
      false,
      al,
      "al r13 0x00002ac0",
      "al_r13_"
      "0x00002ac0"},
     {{al, r13, 0x00005580},
      false,
      al,
      "al r13 0x00005580",
      "al_r13_"
      "0x00005580"},
     {{al, r13, 0x0000ab00},
      false,
      al,
      "al r13 0x0000ab00",
      "al_r13_"
      "0x0000ab00"},
     {{al, r13, 0x00015600},
      false,
      al,
      "al r13 0x00015600",
      "al_r13_"
      "0x00015600"},
     {{al, r13, 0x0002ac00},
      false,
      al,
      "al r13 0x0002ac00",
      "al_r13_"
      "0x0002ac00"},
     {{al, r13, 0x00055800},
      false,
      al,
      "al r13 0x00055800",
      "al_r13_"
      "0x00055800"},
     {{al, r13, 0x000ab000},
      false,
      al,
      "al r13 0x000ab000",
      "al_r13_"
      "0x000ab000"},
     {{al, r13, 0x00156000},
      false,
      al,
      "al r13 0x00156000",
      "al_r13_"
      "0x00156000"},
     {{al, r13, 0x002ac000},
      false,
      al,
      "al r13 0x002ac000",
      "al_r13_"
      "0x002ac000"},
     {{al, r13, 0x00558000},
      false,
      al,
      "al r13 0x00558000",
      "al_r13_"
      "0x00558000"},
     {{al, r13, 0x00ab0000},
      false,
      al,
      "al r13 0x00ab0000",
      "al_r13_"
      "0x00ab0000"},
     {{al, r13, 0x01560000},
      false,
      al,
      "al r13 0x01560000",
      "al_r13_"
      "0x01560000"},
     {{al, r13, 0x02ac0000},
      false,
      al,
      "al r13 0x02ac0000",
      "al_r13_"
      "0x02ac0000"},
     {{al, r13, 0x05580000},
      false,
      al,
      "al r13 0x05580000",
      "al_r13_"
      "0x05580000"},
     {{al, r13, 0x0ab00000},
      false,
      al,
      "al r13 0x0ab00000",
      "al_r13_"
      "0x0ab00000"},
     {{al, r13, 0x15600000},
      false,
      al,
      "al r13 0x15600000",
      "al_r13_"
      "0x15600000"},
     {{al, r13, 0x2ac00000},
      false,
      al,
      "al r13 0x2ac00000",
      "al_r13_"
      "0x2ac00000"},
     {{al, r13, 0x55800000},
      false,
      al,
      "al r13 0x55800000",
      "al_r13_"
      "0x55800000"},
     {{al, r13, 0xab000000},
      false,
      al,
      "al r13 0xab000000",
      "al_r13_"
      "0xab000000"},
     {{al, r13, 0x000000ab},
      false,
      al,
      "al r13 0x000000ab",
      "al_r13_"
      "0x000000ab"},
     {{al, r13, 0x00ab00ab},
      false,
      al,
      "al r13 0x00ab00ab",
      "al_r13_"
      "0x00ab00ab"},
     {{al, r13, 0xab00ab00},
      false,
      al,
      "al r13 0xab00ab00",
      "al_r13_"
      "0xab00ab00"},
     {{al, r13, 0xabababab},
      false,
      al,
      "al r13 0xabababab",
      "al_r13_"
      "0xabababab"},
     {{al, r14, 0x000001fe},
      false,
      al,
      "al r14 0x000001fe",
      "al_r14_"
      "0x000001fe"},
     {{al, r14, 0x000003fc},
      false,
      al,
      "al r14 0x000003fc",
      "al_r14_"
      "0x000003fc"},
     {{al, r14, 0x000007f8},
      false,
      al,
      "al r14 0x000007f8",
      "al_r14_"
      "0x000007f8"},
     {{al, r14, 0x00000ff0},
      false,
      al,
      "al r14 0x00000ff0",
      "al_r14_"
      "0x00000ff0"},
     {{al, r14, 0x00001fe0},
      false,
      al,
      "al r14 0x00001fe0",
      "al_r14_"
      "0x00001fe0"},
     {{al, r14, 0x00003fc0},
      false,
      al,
      "al r14 0x00003fc0",
      "al_r14_"
      "0x00003fc0"},
     {{al, r14, 0x00007f80},
      false,
      al,
      "al r14 0x00007f80",
      "al_r14_"
      "0x00007f80"},
     {{al, r14, 0x0000ff00},
      false,
      al,
      "al r14 0x0000ff00",
      "al_r14_"
      "0x0000ff00"},
     {{al, r14, 0x0001fe00},
      false,
      al,
      "al r14 0x0001fe00",
      "al_r14_"
      "0x0001fe00"},
     {{al, r14, 0x0003fc00},
      false,
      al,
      "al r14 0x0003fc00",
      "al_r14_"
      "0x0003fc00"},
     {{al, r14, 0x0007f800},
      false,
      al,
      "al r14 0x0007f800",
      "al_r14_"
      "0x0007f800"},
     {{al, r14, 0x000ff000},
      false,
      al,
      "al r14 0x000ff000",
      "al_r14_"
      "0x000ff000"},
     {{al, r14, 0x001fe000},
      false,
      al,
      "al r14 0x001fe000",
      "al_r14_"
      "0x001fe000"},
     {{al, r14, 0x003fc000},
      false,
      al,
      "al r14 0x003fc000",
      "al_r14_"
      "0x003fc000"},
     {{al, r14, 0x007f8000},
      false,
      al,
      "al r14 0x007f8000",
      "al_r14_"
      "0x007f8000"},
     {{al, r14, 0x00ff0000},
      false,
      al,
      "al r14 0x00ff0000",
      "al_r14_"
      "0x00ff0000"},
     {{al, r14, 0x01fe0000},
      false,
      al,
      "al r14 0x01fe0000",
      "al_r14_"
      "0x01fe0000"},
     {{al, r14, 0x03fc0000},
      false,
      al,
      "al r14 0x03fc0000",
      "al_r14_"
      "0x03fc0000"},
     {{al, r14, 0x07f80000},
      false,
      al,
      "al r14 0x07f80000",
      "al_r14_"
      "0x07f80000"},
     {{al, r14, 0x0ff00000},
      false,
      al,
      "al r14 0x0ff00000",
      "al_r14_"
      "0x0ff00000"},
     {{al, r14, 0x1fe00000},
      false,
      al,
      "al r14 0x1fe00000",
      "al_r14_"
      "0x1fe00000"},
     {{al, r14, 0x3fc00000},
      false,
      al,
      "al r14 0x3fc00000",
      "al_r14_"
      "0x3fc00000"},
     {{al, r14, 0x7f800000},
      false,
      al,
      "al r14 0x7f800000",
      "al_r14_"
      "0x7f800000"},
     {{al, r14, 0xff000000},
      false,
      al,
      "al r14 0xff000000",
      "al_r14_"
      "0xff000000"},
     {{al, r14, 0x000000ff},
      false,
      al,
      "al r14 0x000000ff",
      "al_r14_"
      "0x000000ff"},
     {{al, r14, 0x00ff00ff},
      false,
      al,
      "al r14 0x00ff00ff",
      "al_r14_"
      "0x00ff00ff"},
     {{al, r14, 0xff00ff00},
      false,
      al,
      "al r14 0xff00ff00",
      "al_r14_"
      "0xff00ff00"},
     {{al, r14, 0xffffffff},
      false,
      al,
      "al r14 0xffffffff",
      "al_r14_"
      "0xffffffff"},
     {{al, r14, 0x00000156},
      false,
      al,
      "al r14 0x00000156",
      "al_r14_"
      "0x00000156"},
     {{al, r14, 0x000002ac},
      false,
      al,
      "al r14 0x000002ac",
      "al_r14_"
      "0x000002ac"},
     {{al, r14, 0x00000558},
      false,
      al,
      "al r14 0x00000558",
      "al_r14_"
      "0x00000558"},
     {{al, r14, 0x00000ab0},
      false,
      al,
      "al r14 0x00000ab0",
      "al_r14_"
      "0x00000ab0"},
     {{al, r14, 0x00001560},
      false,
      al,
      "al r14 0x00001560",
      "al_r14_"
      "0x00001560"},
     {{al, r14, 0x00002ac0},
      false,
      al,
      "al r14 0x00002ac0",
      "al_r14_"
      "0x00002ac0"},
     {{al, r14, 0x00005580},
      false,
      al,
      "al r14 0x00005580",
      "al_r14_"
      "0x00005580"},
     {{al, r14, 0x0000ab00},
      false,
      al,
      "al r14 0x0000ab00",
      "al_r14_"
      "0x0000ab00"},
     {{al, r14, 0x00015600},
      false,
      al,
      "al r14 0x00015600",
      "al_r14_"
      "0x00015600"},
     {{al, r14, 0x0002ac00},
      false,
      al,
      "al r14 0x0002ac00",
      "al_r14_"
      "0x0002ac00"},
     {{al, r14, 0x00055800},
      false,
      al,
      "al r14 0x00055800",
      "al_r14_"
      "0x00055800"},
     {{al, r14, 0x000ab000},
      false,
      al,
      "al r14 0x000ab000",
      "al_r14_"
      "0x000ab000"},
     {{al, r14, 0x00156000},
      false,
      al,
      "al r14 0x00156000",
      "al_r14_"
      "0x00156000"},
     {{al, r14, 0x002ac000},
      false,
      al,
      "al r14 0x002ac000",
      "al_r14_"
      "0x002ac000"},
     {{al, r14, 0x00558000},
      false,
      al,
      "al r14 0x00558000",
      "al_r14_"
      "0x00558000"},
     {{al, r14, 0x00ab0000},
      false,
      al,
      "al r14 0x00ab0000",
      "al_r14_"
      "0x00ab0000"},
     {{al, r14, 0x01560000},
      false,
      al,
      "al r14 0x01560000",
      "al_r14_"
      "0x01560000"},
     {{al, r14, 0x02ac0000},
      false,
      al,
      "al r14 0x02ac0000",
      "al_r14_"
      "0x02ac0000"},
     {{al, r14, 0x05580000},
      false,
      al,
      "al r14 0x05580000",
      "al_r14_"
      "0x05580000"},
     {{al, r14, 0x0ab00000},
      false,
      al,
      "al r14 0x0ab00000",
      "al_r14_"
      "0x0ab00000"},
     {{al, r14, 0x15600000},
      false,
      al,
      "al r14 0x15600000",
      "al_r14_"
      "0x15600000"},
     {{al, r14, 0x2ac00000},
      false,
      al,
      "al r14 0x2ac00000",
      "al_r14_"
      "0x2ac00000"},
     {{al, r14, 0x55800000},
      false,
      al,
      "al r14 0x55800000",
      "al_r14_"
      "0x55800000"},
     {{al, r14, 0xab000000},
      false,
      al,
      "al r14 0xab000000",
      "al_r14_"
      "0xab000000"},
     {{al, r14, 0x000000ab},
      false,
      al,
      "al r14 0x000000ab",
      "al_r14_"
      "0x000000ab"},
     {{al, r14, 0x00ab00ab},
      false,
      al,
      "al r14 0x00ab00ab",
      "al_r14_"
      "0x00ab00ab"},
     {{al, r14, 0xab00ab00},
      false,
      al,
      "al r14 0xab00ab00",
      "al_r14_"
      "0xab00ab00"},
     {{al, r14, 0xabababab},
      false,
      al,
      "al r14 0xabababab",
      "al_r14_0xabababab"}};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/assembler-cond-rd-operand-const-cmn-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-cmp-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-mov-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-movs-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-mvn-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-mvns-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-teq-t32.h"
#include "aarch32/traces/assembler-cond-rd-operand-const-tst-t32.h"


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
  Test test_##mnemonic("AARCH32_ASSEMBLER_COND_RD_OPERAND_CONST_" #mnemonic \
                       "_T32",                                              \
                       &Test_##mnemonic);
FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
