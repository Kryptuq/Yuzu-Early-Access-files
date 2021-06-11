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
// test/aarch32/config/template-simulator-aarch32.cc.in template file using
// tools/generate_tests.py.
//
// PLEASE DO NOT EDIT.
// -----------------------------------------------------------------------------


#include "test-runner.h"

#include "test-utils.h"
#include "test-utils-aarch32.h"

#include "aarch32/assembler-aarch32.h"
#include "aarch32/disasm-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#define __ masm.
#define BUF_SIZE (4096)

#ifdef VIXL_INCLUDE_SIMULATOR_AARCH32
// Run tests with the simulator.

#define SETUP() MacroAssembler masm(BUF_SIZE)

#define START() masm.GetBuffer()->Reset()

#define END() \
  __ Hlt(0);  \
  __ FinalizeCode();

// TODO: Run the tests in the simulator.
#define RUN()

#else  // ifdef VIXL_INCLUDE_SIMULATOR_AARCH32.

#define SETUP()                  \
  MacroAssembler masm(BUF_SIZE); \
  UseScratchRegisterScope harness_scratch;

#define START()                 \
  harness_scratch.Open(&masm);  \
  harness_scratch.ExcludeAll(); \
  masm.GetBuffer()->Reset();    \
  __ Push(r4);                  \
  __ Push(r5);                  \
  __ Push(r6);                  \
  __ Push(r7);                  \
  __ Push(r8);                  \
  __ Push(r9);                  \
  __ Push(r10);                 \
  __ Push(r11);                 \
  __ Push(lr);                  \
  harness_scratch.Include(ip);

#define END()                  \
  harness_scratch.Exclude(ip); \
  __ Pop(lr);                  \
  __ Pop(r11);                 \
  __ Pop(r10);                 \
  __ Pop(r9);                  \
  __ Pop(r8);                  \
  __ Pop(r7);                  \
  __ Pop(r6);                  \
  __ Pop(r5);                  \
  __ Pop(r4);                  \
  __ Bx(lr);                   \
  __ FinalizeCode();           \
  harness_scratch.Close();

#define RUN()                                                 \
  {                                                           \
    int pcs_offset = masm.IsUsingT32() ? 1 : 0;               \
    masm.GetBuffer()->SetExecutable();                        \
    ExecuteMemory(masm.GetBuffer()->GetStartAddress<byte*>(), \
                  masm.GetSizeOfCodeGenerated(),              \
                  pcs_offset);                                \
    masm.GetBuffer()->SetWritable();                          \
  }

#endif  // ifdef VIXL_INCLUDE_SIMULATOR_AARCH32

namespace vixl {
namespace aarch32 {

// List of instruction encodings:
#define FOREACH_INSTRUCTION(M) \
  M(Mov)                       \
  M(Movs)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` and
// `Inputs` have various layouts across generated tests so they absolutely
// cannot be shared.

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

// Input data to feed to the instruction.
struct Inputs {
  uint32_t apsr;
  uint32_t rd;
  uint32_t rn;
  uint32_t rs;
};

// This structure contains all input data needed to test one specific encoding.
// It used to generate a loop over an instruction.
struct TestLoopData {
  // The `operands` fields represents the values to pass to the assembler to
  // produce the instruction.
  Operands operands;
  // Description of the operands, used for error reporting.
  const char* operands_description;
  // Unique identifier, used for generating traces.
  const char* identifier;
  // Array of values to be fed to the instruction.
  size_t input_size;
  const Inputs* inputs;
};

static const Inputs kCondition[] = {{NFlag, 0xabababab, 0xabababab, 0},
                                    {ZFlag, 0xabababab, 0xabababab, 0},
                                    {CFlag, 0xabababab, 0xabababab, 0},
                                    {VFlag, 0xabababab, 0xabababab, 0},
                                    {NZFlag, 0xabababab, 0xabababab, 0},
                                    {NCFlag, 0xabababab, 0xabababab, 0},
                                    {NVFlag, 0xabababab, 0xabababab, 0},
                                    {ZCFlag, 0xabababab, 0xabababab, 0},
                                    {ZVFlag, 0xabababab, 0xabababab, 0},
                                    {CVFlag, 0xabababab, 0xabababab, 0},
                                    {NZCFlag, 0xabababab, 0xabababab, 0},
                                    {NZVFlag, 0xabababab, 0xabababab, 0},
                                    {NCVFlag, 0xabababab, 0xabababab, 0},
                                    {ZCVFlag, 0xabababab, 0xabababab, 0},
                                    {NZCVFlag, 0xabababab, 0xabababab, 0}};

static const Inputs kRdIsRn[] =
    {{NoFlag, 0x00000000, 0x00000000, 0}, {NoFlag, 0x00000001, 0x00000001, 0},
     {NoFlag, 0x00000002, 0x00000002, 0}, {NoFlag, 0x00000020, 0x00000020, 0},
     {NoFlag, 0x0000007d, 0x0000007d, 0}, {NoFlag, 0x0000007e, 0x0000007e, 0},
     {NoFlag, 0x0000007f, 0x0000007f, 0}, {NoFlag, 0x00007ffd, 0x00007ffd, 0},
     {NoFlag, 0x00007ffe, 0x00007ffe, 0}, {NoFlag, 0x00007fff, 0x00007fff, 0},
     {NoFlag, 0x33333333, 0x33333333, 0}, {NoFlag, 0x55555555, 0x55555555, 0},
     {NoFlag, 0x7ffffffd, 0x7ffffffd, 0}, {NoFlag, 0x7ffffffe, 0x7ffffffe, 0},
     {NoFlag, 0x7fffffff, 0x7fffffff, 0}, {NoFlag, 0x80000000, 0x80000000, 0},
     {NoFlag, 0x80000001, 0x80000001, 0}, {NoFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0},
     {NoFlag, 0xcccccccc, 0xcccccccc, 0}, {NoFlag, 0xffff8000, 0xffff8000, 0},
     {NoFlag, 0xffff8001, 0xffff8001, 0}, {NoFlag, 0xffff8002, 0xffff8002, 0},
     {NoFlag, 0xffff8003, 0xffff8003, 0}, {NoFlag, 0xffffff80, 0xffffff80, 0},
     {NoFlag, 0xffffff81, 0xffffff81, 0}, {NoFlag, 0xffffff82, 0xffffff82, 0},
     {NoFlag, 0xffffff83, 0xffffff83, 0}, {NoFlag, 0xffffffe0, 0xffffffe0, 0},
     {NoFlag, 0xfffffffd, 0xfffffffd, 0}, {NoFlag, 0xfffffffe, 0xfffffffe, 0},
     {NoFlag, 0xffffffff, 0xffffffff, 0}};

static const Inputs kRdIsNotRn[] =
    {{NoFlag, 0x00000002, 0xcccccccc, 0}, {NoFlag, 0x7ffffffd, 0x00007ffe, 0},
     {NoFlag, 0xffffff80, 0x00000020, 0}, {NoFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0},
     {NoFlag, 0x33333333, 0xffffff82, 0}, {NoFlag, 0xffff8001, 0x7ffffffe, 0},
     {NoFlag, 0xfffffffd, 0x00007ffe, 0}, {NoFlag, 0xffffff80, 0x80000000, 0},
     {NoFlag, 0x00000001, 0x33333333, 0}, {NoFlag, 0xcccccccc, 0x7ffffffe, 0},
     {NoFlag, 0x00000000, 0xcccccccc, 0}, {NoFlag, 0x00000000, 0x55555555, 0},
     {NoFlag, 0xffffffff, 0xffffffff, 0}, {NoFlag, 0x0000007e, 0xffff8002, 0},
     {NoFlag, 0x80000000, 0x7ffffffd, 0}, {NoFlag, 0xffffff81, 0x0000007e, 0},
     {NoFlag, 0x0000007f, 0xffff8001, 0}, {NoFlag, 0xffffffe0, 0x00007ffd, 0},
     {NoFlag, 0xffff8003, 0x00000002, 0}, {NoFlag, 0xffffff83, 0x55555555, 0},
     {NoFlag, 0xffffff83, 0xffffff80, 0}, {NoFlag, 0xffffff81, 0xffff8000, 0},
     {NoFlag, 0x00000020, 0x7ffffffe, 0}, {NoFlag, 0xffffffe0, 0x00000000, 0},
     {NoFlag, 0x7fffffff, 0x0000007e, 0}, {NoFlag, 0x80000001, 0xffffffff, 0},
     {NoFlag, 0x00000001, 0x80000001, 0}, {NoFlag, 0x00000002, 0x0000007f, 0},
     {NoFlag, 0x7fffffff, 0xcccccccc, 0}, {NoFlag, 0x80000001, 0x00007ffe, 0},
     {NoFlag, 0xffff8002, 0x0000007e, 0}, {NoFlag, 0x00007ffe, 0xcccccccc, 0},
     {NoFlag, 0x80000000, 0xffff8002, 0}, {NoFlag, 0xffffff83, 0x7ffffffe, 0},
     {NoFlag, 0xffff8001, 0x00000001, 0}, {NoFlag, 0xffffff81, 0x00000020, 0},
     {NoFlag, 0xfffffffe, 0xffff8001, 0}, {NoFlag, 0xffffffff, 0xfffffffe, 0},
     {NoFlag, 0xcccccccc, 0x55555555, 0}, {NoFlag, 0x00000020, 0xffffff83, 0},
     {NoFlag, 0xffffff83, 0xffff8001, 0}, {NoFlag, 0xffffff83, 0xffff8000, 0},
     {NoFlag, 0x00007fff, 0x00000002, 0}, {NoFlag, 0x55555555, 0xffff8000, 0},
     {NoFlag, 0x80000001, 0xffffff81, 0}, {NoFlag, 0x00000002, 0x00000000, 0},
     {NoFlag, 0x33333333, 0xffffff81, 0}, {NoFlag, 0xffff8001, 0xffffff82, 0},
     {NoFlag, 0xcccccccc, 0xffff8003, 0}, {NoFlag, 0xffff8003, 0x7ffffffd, 0},
     {NoFlag, 0x0000007d, 0x00007ffe, 0}, {NoFlag, 0xffffff80, 0x0000007d, 0},
     {NoFlag, 0xaaaaaaaa, 0x00007ffd, 0}, {NoFlag, 0x80000000, 0xffffff82, 0},
     {NoFlag, 0x00000002, 0x7ffffffe, 0}, {NoFlag, 0x00000002, 0xffffff83, 0},
     {NoFlag, 0x55555555, 0x00000002, 0}, {NoFlag, 0xffffffff, 0xffffff82, 0},
     {NoFlag, 0xaaaaaaaa, 0x00000020, 0}, {NoFlag, 0x00000001, 0xffffff82, 0},
     {NoFlag, 0x0000007f, 0xffffff82, 0}, {NoFlag, 0x7ffffffd, 0xaaaaaaaa, 0},
     {NoFlag, 0x00007ffe, 0x00000001, 0}, {NoFlag, 0xfffffffd, 0xffffffe0, 0},
     {NoFlag, 0xffffff81, 0xffffff83, 0}, {NoFlag, 0x0000007d, 0x00000000, 0},
     {NoFlag, 0x0000007d, 0xffff8000, 0}, {NoFlag, 0xffffff81, 0x7fffffff, 0},
     {NoFlag, 0xffffffff, 0x80000000, 0}, {NoFlag, 0x00000000, 0x00000001, 0},
     {NoFlag, 0x55555555, 0xffffff82, 0}, {NoFlag, 0x00007ffe, 0x00007ffe, 0},
     {NoFlag, 0x80000001, 0xfffffffd, 0}, {NoFlag, 0x00007fff, 0x33333333, 0},
     {NoFlag, 0x00007fff, 0x80000000, 0}, {NoFlag, 0xcccccccc, 0x00007fff, 0},
     {NoFlag, 0xfffffffe, 0xffffffe0, 0}, {NoFlag, 0x7ffffffe, 0x0000007f, 0},
     {NoFlag, 0x00007ffd, 0xffff8001, 0}, {NoFlag, 0x00000002, 0x00000001, 0},
     {NoFlag, 0x80000000, 0xffffffff, 0}, {NoFlag, 0xffffff83, 0xcccccccc, 0},
     {NoFlag, 0xffff8002, 0x7ffffffe, 0}, {NoFlag, 0xaaaaaaaa, 0x00000000, 0},
     {NoFlag, 0xffffff80, 0xcccccccc, 0}, {NoFlag, 0x33333333, 0xffffff83, 0},
     {NoFlag, 0x0000007e, 0xffffffe0, 0}, {NoFlag, 0x0000007e, 0x00007fff, 0},
     {NoFlag, 0x0000007f, 0x00000002, 0}, {NoFlag, 0x7ffffffe, 0xcccccccc, 0},
     {NoFlag, 0x0000007d, 0xffffff80, 0}, {NoFlag, 0x00007fff, 0x00000020, 0},
     {NoFlag, 0x7ffffffe, 0xfffffffe, 0}, {NoFlag, 0xfffffffe, 0xffffff81, 0},
     {NoFlag, 0xffffffff, 0x0000007f, 0}, {NoFlag, 0xffff8002, 0x7ffffffd, 0},
     {NoFlag, 0xffff8001, 0xfffffffe, 0}, {NoFlag, 0x33333333, 0xffff8002, 0},
     {NoFlag, 0x00000000, 0xffffffff, 0}, {NoFlag, 0x33333333, 0xffffff80, 0},
     {NoFlag, 0x0000007f, 0x00007fff, 0}, {NoFlag, 0xffffffff, 0xffff8001, 0},
     {NoFlag, 0x7fffffff, 0xffff8002, 0}, {NoFlag, 0x7ffffffd, 0xffffff83, 0},
     {NoFlag, 0x7fffffff, 0x0000007f, 0}, {NoFlag, 0xffffff83, 0xfffffffe, 0},
     {NoFlag, 0x7ffffffe, 0xffff8003, 0}, {NoFlag, 0xffff8002, 0xffff8002, 0},
     {NoFlag, 0x80000001, 0x0000007f, 0}, {NoFlag, 0x00000020, 0x00000002, 0},
     {NoFlag, 0xffffff82, 0xffff8001, 0}, {NoFlag, 0xffffffff, 0x00000001, 0},
     {NoFlag, 0xffffff80, 0xffff8002, 0}, {NoFlag, 0xffff8003, 0x7fffffff, 0},
     {NoFlag, 0xffffffff, 0xffff8000, 0}, {NoFlag, 0xffff8002, 0x00007ffd, 0},
     {NoFlag, 0x00000020, 0xffffff81, 0}, {NoFlag, 0x00000001, 0x55555555, 0},
     {NoFlag, 0x7ffffffe, 0x00000020, 0}, {NoFlag, 0x80000000, 0x00000001, 0},
     {NoFlag, 0x00007ffd, 0xffff8002, 0}, {NoFlag, 0x7fffffff, 0xfffffffe, 0},
     {NoFlag, 0xcccccccc, 0x00007ffd, 0}, {NoFlag, 0x00000000, 0xfffffffd, 0},
     {NoFlag, 0xffff8003, 0xffffff80, 0}, {NoFlag, 0x80000001, 0xffffff80, 0},
     {NoFlag, 0xffffffff, 0xffff8002, 0}, {NoFlag, 0x00007ffe, 0xffff8002, 0},
     {NoFlag, 0xffffff80, 0x00007ffe, 0}, {NoFlag, 0x80000001, 0xffff8001, 0},
     {NoFlag, 0x0000007f, 0xffffff80, 0}, {NoFlag, 0xffffff81, 0x80000000, 0},
     {NoFlag, 0x00007fff, 0x00007ffe, 0}, {NoFlag, 0x33333333, 0xffff8000, 0},
     {NoFlag, 0x33333333, 0x00007fff, 0}, {NoFlag, 0x00000000, 0x0000007d, 0},
     {NoFlag, 0x80000001, 0x00000000, 0}, {NoFlag, 0xffffffff, 0x55555555, 0},
     {NoFlag, 0x80000001, 0x80000000, 0}, {NoFlag, 0xffffffff, 0xffffff80, 0},
     {NoFlag, 0xffffff81, 0xffff8003, 0}, {NoFlag, 0x55555555, 0x80000001, 0},
     {NoFlag, 0x7fffffff, 0xffff8001, 0}, {NoFlag, 0xffffff83, 0x00000002, 0},
     {NoFlag, 0x0000007e, 0xffffff81, 0}, {NoFlag, 0x80000000, 0xffff8001, 0},
     {NoFlag, 0xffffff80, 0xfffffffe, 0}, {NoFlag, 0x0000007e, 0xfffffffd, 0},
     {NoFlag, 0xffffffe0, 0xffffffff, 0}, {NoFlag, 0x55555555, 0x80000000, 0},
     {NoFlag, 0x0000007d, 0x80000001, 0}, {NoFlag, 0xffffffe0, 0x7ffffffd, 0},
     {NoFlag, 0x00000000, 0x00000000, 0}, {NoFlag, 0x55555555, 0x00000001, 0},
     {NoFlag, 0x00007ffd, 0x7fffffff, 0}, {NoFlag, 0x55555555, 0xffffffff, 0},
     {NoFlag, 0xffff8003, 0x00007fff, 0}, {NoFlag, 0xffffff82, 0x00007fff, 0},
     {NoFlag, 0x33333333, 0x55555555, 0}, {NoFlag, 0x00000020, 0x33333333, 0},
     {NoFlag, 0x7ffffffe, 0xfffffffd, 0}, {NoFlag, 0x7ffffffe, 0x00000001, 0},
     {NoFlag, 0xffffff83, 0xffffffe0, 0}, {NoFlag, 0xfffffffe, 0xaaaaaaaa, 0},
     {NoFlag, 0xffff8002, 0x33333333, 0}, {NoFlag, 0xffff8002, 0xffff8003, 0},
     {NoFlag, 0x33333333, 0x7fffffff, 0}, {NoFlag, 0xfffffffd, 0xffffff83, 0},
     {NoFlag, 0x00000000, 0xffff8000, 0}, {NoFlag, 0xffffff82, 0x55555555, 0},
     {NoFlag, 0xffffff82, 0xffffff81, 0}, {NoFlag, 0xcccccccc, 0xfffffffe, 0},
     {NoFlag, 0xfffffffd, 0x7fffffff, 0}, {NoFlag, 0x00007fff, 0x7fffffff, 0},
     {NoFlag, 0xffffff83, 0xffff8003, 0}, {NoFlag, 0xfffffffe, 0xffffffff, 0},
     {NoFlag, 0x7ffffffd, 0x00007ffd, 0}, {NoFlag, 0x7ffffffd, 0x00007fff, 0},
     {NoFlag, 0x00007ffd, 0xffffffff, 0}, {NoFlag, 0x00000001, 0xffff8003, 0},
     {NoFlag, 0xffffff80, 0xfffffffd, 0}, {NoFlag, 0x33333333, 0x80000000, 0},
     {NoFlag, 0xffff8001, 0x00000020, 0}, {NoFlag, 0xcccccccc, 0x00000002, 0},
     {NoFlag, 0x00000000, 0x00000002, 0}, {NoFlag, 0x0000007d, 0x00007fff, 0},
     {NoFlag, 0xcccccccc, 0x00000001, 0}, {NoFlag, 0xffffff83, 0x00007fff, 0},
     {NoFlag, 0x80000001, 0x00000020, 0}, {NoFlag, 0xffff8003, 0xffffffe0, 0},
     {NoFlag, 0x00007ffd, 0xaaaaaaaa, 0}, {NoFlag, 0x33333333, 0xffff8001, 0},
     {NoFlag, 0xffffff83, 0x80000001, 0}, {NoFlag, 0xffff8000, 0xffff8000, 0},
     {NoFlag, 0x00007ffe, 0xffff8001, 0}, {NoFlag, 0x7ffffffd, 0x00000000, 0},
     {NoFlag, 0x00007ffe, 0x33333333, 0}, {NoFlag, 0xffff8001, 0xffffff80, 0},
     {NoFlag, 0xfffffffe, 0x55555555, 0}, {NoFlag, 0xffffff82, 0xffffffff, 0}};

static const Inputs kShiftTypes[] =
    {{NoFlag, 0xabababab, 0x00000000, 0},  {NoFlag, 0xabababab, 0x00000000, 1},
     {NoFlag, 0xabababab, 0x00000000, 2},  {NoFlag, 0xabababab, 0x00000000, 3},
     {NoFlag, 0xabababab, 0x00000000, 4},  {NoFlag, 0xabababab, 0x00000000, 5},
     {NoFlag, 0xabababab, 0x00000000, 6},  {NoFlag, 0xabababab, 0x00000000, 7},
     {NoFlag, 0xabababab, 0x00000000, 8},  {NoFlag, 0xabababab, 0x00000000, 9},
     {NoFlag, 0xabababab, 0x00000000, 10}, {NoFlag, 0xabababab, 0x00000000, 11},
     {NoFlag, 0xabababab, 0x00000000, 12}, {NoFlag, 0xabababab, 0x00000000, 13},
     {NoFlag, 0xabababab, 0x00000000, 14}, {NoFlag, 0xabababab, 0x00000000, 15},
     {NoFlag, 0xabababab, 0x00000000, 16}, {NoFlag, 0xabababab, 0x00000000, 17},
     {NoFlag, 0xabababab, 0x00000000, 18}, {NoFlag, 0xabababab, 0x00000000, 19},
     {NoFlag, 0xabababab, 0x00000000, 20}, {NoFlag, 0xabababab, 0x00000000, 21},
     {NoFlag, 0xabababab, 0x00000000, 22}, {NoFlag, 0xabababab, 0x00000000, 23},
     {NoFlag, 0xabababab, 0x00000000, 24}, {NoFlag, 0xabababab, 0x00000000, 25},
     {NoFlag, 0xabababab, 0x00000000, 26}, {NoFlag, 0xabababab, 0x00000000, 27},
     {NoFlag, 0xabababab, 0x00000000, 28}, {NoFlag, 0xabababab, 0x00000000, 29},
     {NoFlag, 0xabababab, 0x00000000, 30}, {NoFlag, 0xabababab, 0x00000000, 31},
     {NoFlag, 0xabababab, 0x00000000, 32}, {NoFlag, 0xabababab, 0x00000001, 0},
     {NoFlag, 0xabababab, 0x00000001, 1},  {NoFlag, 0xabababab, 0x00000001, 2},
     {NoFlag, 0xabababab, 0x00000001, 3},  {NoFlag, 0xabababab, 0x00000001, 4},
     {NoFlag, 0xabababab, 0x00000001, 5},  {NoFlag, 0xabababab, 0x00000001, 6},
     {NoFlag, 0xabababab, 0x00000001, 7},  {NoFlag, 0xabababab, 0x00000001, 8},
     {NoFlag, 0xabababab, 0x00000001, 9},  {NoFlag, 0xabababab, 0x00000001, 10},
     {NoFlag, 0xabababab, 0x00000001, 11}, {NoFlag, 0xabababab, 0x00000001, 12},
     {NoFlag, 0xabababab, 0x00000001, 13}, {NoFlag, 0xabababab, 0x00000001, 14},
     {NoFlag, 0xabababab, 0x00000001, 15}, {NoFlag, 0xabababab, 0x00000001, 16},
     {NoFlag, 0xabababab, 0x00000001, 17}, {NoFlag, 0xabababab, 0x00000001, 18},
     {NoFlag, 0xabababab, 0x00000001, 19}, {NoFlag, 0xabababab, 0x00000001, 20},
     {NoFlag, 0xabababab, 0x00000001, 21}, {NoFlag, 0xabababab, 0x00000001, 22},
     {NoFlag, 0xabababab, 0x00000001, 23}, {NoFlag, 0xabababab, 0x00000001, 24},
     {NoFlag, 0xabababab, 0x00000001, 25}, {NoFlag, 0xabababab, 0x00000001, 26},
     {NoFlag, 0xabababab, 0x00000001, 27}, {NoFlag, 0xabababab, 0x00000001, 28},
     {NoFlag, 0xabababab, 0x00000001, 29}, {NoFlag, 0xabababab, 0x00000001, 30},
     {NoFlag, 0xabababab, 0x00000001, 31}, {NoFlag, 0xabababab, 0x00000001, 32},
     {NoFlag, 0xabababab, 0x00000002, 0},  {NoFlag, 0xabababab, 0x00000002, 1},
     {NoFlag, 0xabababab, 0x00000002, 2},  {NoFlag, 0xabababab, 0x00000002, 3},
     {NoFlag, 0xabababab, 0x00000002, 4},  {NoFlag, 0xabababab, 0x00000002, 5},
     {NoFlag, 0xabababab, 0x00000002, 6},  {NoFlag, 0xabababab, 0x00000002, 7},
     {NoFlag, 0xabababab, 0x00000002, 8},  {NoFlag, 0xabababab, 0x00000002, 9},
     {NoFlag, 0xabababab, 0x00000002, 10}, {NoFlag, 0xabababab, 0x00000002, 11},
     {NoFlag, 0xabababab, 0x00000002, 12}, {NoFlag, 0xabababab, 0x00000002, 13},
     {NoFlag, 0xabababab, 0x00000002, 14}, {NoFlag, 0xabababab, 0x00000002, 15},
     {NoFlag, 0xabababab, 0x00000002, 16}, {NoFlag, 0xabababab, 0x00000002, 17},
     {NoFlag, 0xabababab, 0x00000002, 18}, {NoFlag, 0xabababab, 0x00000002, 19},
     {NoFlag, 0xabababab, 0x00000002, 20}, {NoFlag, 0xabababab, 0x00000002, 21},
     {NoFlag, 0xabababab, 0x00000002, 22}, {NoFlag, 0xabababab, 0x00000002, 23},
     {NoFlag, 0xabababab, 0x00000002, 24}, {NoFlag, 0xabababab, 0x00000002, 25},
     {NoFlag, 0xabababab, 0x00000002, 26}, {NoFlag, 0xabababab, 0x00000002, 27},
     {NoFlag, 0xabababab, 0x00000002, 28}, {NoFlag, 0xabababab, 0x00000002, 29},
     {NoFlag, 0xabababab, 0x00000002, 30}, {NoFlag, 0xabababab, 0x00000002, 31},
     {NoFlag, 0xabababab, 0x00000002, 32}, {NoFlag, 0xabababab, 0x00000020, 0},
     {NoFlag, 0xabababab, 0x00000020, 1},  {NoFlag, 0xabababab, 0x00000020, 2},
     {NoFlag, 0xabababab, 0x00000020, 3},  {NoFlag, 0xabababab, 0x00000020, 4},
     {NoFlag, 0xabababab, 0x00000020, 5},  {NoFlag, 0xabababab, 0x00000020, 6},
     {NoFlag, 0xabababab, 0x00000020, 7},  {NoFlag, 0xabababab, 0x00000020, 8},
     {NoFlag, 0xabababab, 0x00000020, 9},  {NoFlag, 0xabababab, 0x00000020, 10},
     {NoFlag, 0xabababab, 0x00000020, 11}, {NoFlag, 0xabababab, 0x00000020, 12},
     {NoFlag, 0xabababab, 0x00000020, 13}, {NoFlag, 0xabababab, 0x00000020, 14},
     {NoFlag, 0xabababab, 0x00000020, 15}, {NoFlag, 0xabababab, 0x00000020, 16},
     {NoFlag, 0xabababab, 0x00000020, 17}, {NoFlag, 0xabababab, 0x00000020, 18},
     {NoFlag, 0xabababab, 0x00000020, 19}, {NoFlag, 0xabababab, 0x00000020, 20},
     {NoFlag, 0xabababab, 0x00000020, 21}, {NoFlag, 0xabababab, 0x00000020, 22},
     {NoFlag, 0xabababab, 0x00000020, 23}, {NoFlag, 0xabababab, 0x00000020, 24},
     {NoFlag, 0xabababab, 0x00000020, 25}, {NoFlag, 0xabababab, 0x00000020, 26},
     {NoFlag, 0xabababab, 0x00000020, 27}, {NoFlag, 0xabababab, 0x00000020, 28},
     {NoFlag, 0xabababab, 0x00000020, 29}, {NoFlag, 0xabababab, 0x00000020, 30},
     {NoFlag, 0xabababab, 0x00000020, 31}, {NoFlag, 0xabababab, 0x00000020, 32},
     {NoFlag, 0xabababab, 0x0000007d, 0},  {NoFlag, 0xabababab, 0x0000007d, 1},
     {NoFlag, 0xabababab, 0x0000007d, 2},  {NoFlag, 0xabababab, 0x0000007d, 3},
     {NoFlag, 0xabababab, 0x0000007d, 4},  {NoFlag, 0xabababab, 0x0000007d, 5},
     {NoFlag, 0xabababab, 0x0000007d, 6},  {NoFlag, 0xabababab, 0x0000007d, 7},
     {NoFlag, 0xabababab, 0x0000007d, 8},  {NoFlag, 0xabababab, 0x0000007d, 9},
     {NoFlag, 0xabababab, 0x0000007d, 10}, {NoFlag, 0xabababab, 0x0000007d, 11},
     {NoFlag, 0xabababab, 0x0000007d, 12}, {NoFlag, 0xabababab, 0x0000007d, 13},
     {NoFlag, 0xabababab, 0x0000007d, 14}, {NoFlag, 0xabababab, 0x0000007d, 15},
     {NoFlag, 0xabababab, 0x0000007d, 16}, {NoFlag, 0xabababab, 0x0000007d, 17},
     {NoFlag, 0xabababab, 0x0000007d, 18}, {NoFlag, 0xabababab, 0x0000007d, 19},
     {NoFlag, 0xabababab, 0x0000007d, 20}, {NoFlag, 0xabababab, 0x0000007d, 21},
     {NoFlag, 0xabababab, 0x0000007d, 22}, {NoFlag, 0xabababab, 0x0000007d, 23},
     {NoFlag, 0xabababab, 0x0000007d, 24}, {NoFlag, 0xabababab, 0x0000007d, 25},
     {NoFlag, 0xabababab, 0x0000007d, 26}, {NoFlag, 0xabababab, 0x0000007d, 27},
     {NoFlag, 0xabababab, 0x0000007d, 28}, {NoFlag, 0xabababab, 0x0000007d, 29},
     {NoFlag, 0xabababab, 0x0000007d, 30}, {NoFlag, 0xabababab, 0x0000007d, 31},
     {NoFlag, 0xabababab, 0x0000007d, 32}, {NoFlag, 0xabababab, 0x0000007e, 0},
     {NoFlag, 0xabababab, 0x0000007e, 1},  {NoFlag, 0xabababab, 0x0000007e, 2},
     {NoFlag, 0xabababab, 0x0000007e, 3},  {NoFlag, 0xabababab, 0x0000007e, 4},
     {NoFlag, 0xabababab, 0x0000007e, 5},  {NoFlag, 0xabababab, 0x0000007e, 6},
     {NoFlag, 0xabababab, 0x0000007e, 7},  {NoFlag, 0xabababab, 0x0000007e, 8},
     {NoFlag, 0xabababab, 0x0000007e, 9},  {NoFlag, 0xabababab, 0x0000007e, 10},
     {NoFlag, 0xabababab, 0x0000007e, 11}, {NoFlag, 0xabababab, 0x0000007e, 12},
     {NoFlag, 0xabababab, 0x0000007e, 13}, {NoFlag, 0xabababab, 0x0000007e, 14},
     {NoFlag, 0xabababab, 0x0000007e, 15}, {NoFlag, 0xabababab, 0x0000007e, 16},
     {NoFlag, 0xabababab, 0x0000007e, 17}, {NoFlag, 0xabababab, 0x0000007e, 18},
     {NoFlag, 0xabababab, 0x0000007e, 19}, {NoFlag, 0xabababab, 0x0000007e, 20},
     {NoFlag, 0xabababab, 0x0000007e, 21}, {NoFlag, 0xabababab, 0x0000007e, 22},
     {NoFlag, 0xabababab, 0x0000007e, 23}, {NoFlag, 0xabababab, 0x0000007e, 24},
     {NoFlag, 0xabababab, 0x0000007e, 25}, {NoFlag, 0xabababab, 0x0000007e, 26},
     {NoFlag, 0xabababab, 0x0000007e, 27}, {NoFlag, 0xabababab, 0x0000007e, 28},
     {NoFlag, 0xabababab, 0x0000007e, 29}, {NoFlag, 0xabababab, 0x0000007e, 30},
     {NoFlag, 0xabababab, 0x0000007e, 31}, {NoFlag, 0xabababab, 0x0000007e, 32},
     {NoFlag, 0xabababab, 0x0000007f, 0},  {NoFlag, 0xabababab, 0x0000007f, 1},
     {NoFlag, 0xabababab, 0x0000007f, 2},  {NoFlag, 0xabababab, 0x0000007f, 3},
     {NoFlag, 0xabababab, 0x0000007f, 4},  {NoFlag, 0xabababab, 0x0000007f, 5},
     {NoFlag, 0xabababab, 0x0000007f, 6},  {NoFlag, 0xabababab, 0x0000007f, 7},
     {NoFlag, 0xabababab, 0x0000007f, 8},  {NoFlag, 0xabababab, 0x0000007f, 9},
     {NoFlag, 0xabababab, 0x0000007f, 10}, {NoFlag, 0xabababab, 0x0000007f, 11},
     {NoFlag, 0xabababab, 0x0000007f, 12}, {NoFlag, 0xabababab, 0x0000007f, 13},
     {NoFlag, 0xabababab, 0x0000007f, 14}, {NoFlag, 0xabababab, 0x0000007f, 15},
     {NoFlag, 0xabababab, 0x0000007f, 16}, {NoFlag, 0xabababab, 0x0000007f, 17},
     {NoFlag, 0xabababab, 0x0000007f, 18}, {NoFlag, 0xabababab, 0x0000007f, 19},
     {NoFlag, 0xabababab, 0x0000007f, 20}, {NoFlag, 0xabababab, 0x0000007f, 21},
     {NoFlag, 0xabababab, 0x0000007f, 22}, {NoFlag, 0xabababab, 0x0000007f, 23},
     {NoFlag, 0xabababab, 0x0000007f, 24}, {NoFlag, 0xabababab, 0x0000007f, 25},
     {NoFlag, 0xabababab, 0x0000007f, 26}, {NoFlag, 0xabababab, 0x0000007f, 27},
     {NoFlag, 0xabababab, 0x0000007f, 28}, {NoFlag, 0xabababab, 0x0000007f, 29},
     {NoFlag, 0xabababab, 0x0000007f, 30}, {NoFlag, 0xabababab, 0x0000007f, 31},
     {NoFlag, 0xabababab, 0x0000007f, 32}, {NoFlag, 0xabababab, 0x00007ffd, 0},
     {NoFlag, 0xabababab, 0x00007ffd, 1},  {NoFlag, 0xabababab, 0x00007ffd, 2},
     {NoFlag, 0xabababab, 0x00007ffd, 3},  {NoFlag, 0xabababab, 0x00007ffd, 4},
     {NoFlag, 0xabababab, 0x00007ffd, 5},  {NoFlag, 0xabababab, 0x00007ffd, 6},
     {NoFlag, 0xabababab, 0x00007ffd, 7},  {NoFlag, 0xabababab, 0x00007ffd, 8},
     {NoFlag, 0xabababab, 0x00007ffd, 9},  {NoFlag, 0xabababab, 0x00007ffd, 10},
     {NoFlag, 0xabababab, 0x00007ffd, 11}, {NoFlag, 0xabababab, 0x00007ffd, 12},
     {NoFlag, 0xabababab, 0x00007ffd, 13}, {NoFlag, 0xabababab, 0x00007ffd, 14},
     {NoFlag, 0xabababab, 0x00007ffd, 15}, {NoFlag, 0xabababab, 0x00007ffd, 16},
     {NoFlag, 0xabababab, 0x00007ffd, 17}, {NoFlag, 0xabababab, 0x00007ffd, 18},
     {NoFlag, 0xabababab, 0x00007ffd, 19}, {NoFlag, 0xabababab, 0x00007ffd, 20},
     {NoFlag, 0xabababab, 0x00007ffd, 21}, {NoFlag, 0xabababab, 0x00007ffd, 22},
     {NoFlag, 0xabababab, 0x00007ffd, 23}, {NoFlag, 0xabababab, 0x00007ffd, 24},
     {NoFlag, 0xabababab, 0x00007ffd, 25}, {NoFlag, 0xabababab, 0x00007ffd, 26},
     {NoFlag, 0xabababab, 0x00007ffd, 27}, {NoFlag, 0xabababab, 0x00007ffd, 28},
     {NoFlag, 0xabababab, 0x00007ffd, 29}, {NoFlag, 0xabababab, 0x00007ffd, 30},
     {NoFlag, 0xabababab, 0x00007ffd, 31}, {NoFlag, 0xabababab, 0x00007ffd, 32},
     {NoFlag, 0xabababab, 0x00007ffe, 0},  {NoFlag, 0xabababab, 0x00007ffe, 1},
     {NoFlag, 0xabababab, 0x00007ffe, 2},  {NoFlag, 0xabababab, 0x00007ffe, 3},
     {NoFlag, 0xabababab, 0x00007ffe, 4},  {NoFlag, 0xabababab, 0x00007ffe, 5},
     {NoFlag, 0xabababab, 0x00007ffe, 6},  {NoFlag, 0xabababab, 0x00007ffe, 7},
     {NoFlag, 0xabababab, 0x00007ffe, 8},  {NoFlag, 0xabababab, 0x00007ffe, 9},
     {NoFlag, 0xabababab, 0x00007ffe, 10}, {NoFlag, 0xabababab, 0x00007ffe, 11},
     {NoFlag, 0xabababab, 0x00007ffe, 12}, {NoFlag, 0xabababab, 0x00007ffe, 13},
     {NoFlag, 0xabababab, 0x00007ffe, 14}, {NoFlag, 0xabababab, 0x00007ffe, 15},
     {NoFlag, 0xabababab, 0x00007ffe, 16}, {NoFlag, 0xabababab, 0x00007ffe, 17},
     {NoFlag, 0xabababab, 0x00007ffe, 18}, {NoFlag, 0xabababab, 0x00007ffe, 19},
     {NoFlag, 0xabababab, 0x00007ffe, 20}, {NoFlag, 0xabababab, 0x00007ffe, 21},
     {NoFlag, 0xabababab, 0x00007ffe, 22}, {NoFlag, 0xabababab, 0x00007ffe, 23},
     {NoFlag, 0xabababab, 0x00007ffe, 24}, {NoFlag, 0xabababab, 0x00007ffe, 25},
     {NoFlag, 0xabababab, 0x00007ffe, 26}, {NoFlag, 0xabababab, 0x00007ffe, 27},
     {NoFlag, 0xabababab, 0x00007ffe, 28}, {NoFlag, 0xabababab, 0x00007ffe, 29},
     {NoFlag, 0xabababab, 0x00007ffe, 30}, {NoFlag, 0xabababab, 0x00007ffe, 31},
     {NoFlag, 0xabababab, 0x00007ffe, 32}, {NoFlag, 0xabababab, 0x00007fff, 0},
     {NoFlag, 0xabababab, 0x00007fff, 1},  {NoFlag, 0xabababab, 0x00007fff, 2},
     {NoFlag, 0xabababab, 0x00007fff, 3},  {NoFlag, 0xabababab, 0x00007fff, 4},
     {NoFlag, 0xabababab, 0x00007fff, 5},  {NoFlag, 0xabababab, 0x00007fff, 6},
     {NoFlag, 0xabababab, 0x00007fff, 7},  {NoFlag, 0xabababab, 0x00007fff, 8},
     {NoFlag, 0xabababab, 0x00007fff, 9},  {NoFlag, 0xabababab, 0x00007fff, 10},
     {NoFlag, 0xabababab, 0x00007fff, 11}, {NoFlag, 0xabababab, 0x00007fff, 12},
     {NoFlag, 0xabababab, 0x00007fff, 13}, {NoFlag, 0xabababab, 0x00007fff, 14},
     {NoFlag, 0xabababab, 0x00007fff, 15}, {NoFlag, 0xabababab, 0x00007fff, 16},
     {NoFlag, 0xabababab, 0x00007fff, 17}, {NoFlag, 0xabababab, 0x00007fff, 18},
     {NoFlag, 0xabababab, 0x00007fff, 19}, {NoFlag, 0xabababab, 0x00007fff, 20},
     {NoFlag, 0xabababab, 0x00007fff, 21}, {NoFlag, 0xabababab, 0x00007fff, 22},
     {NoFlag, 0xabababab, 0x00007fff, 23}, {NoFlag, 0xabababab, 0x00007fff, 24},
     {NoFlag, 0xabababab, 0x00007fff, 25}, {NoFlag, 0xabababab, 0x00007fff, 26},
     {NoFlag, 0xabababab, 0x00007fff, 27}, {NoFlag, 0xabababab, 0x00007fff, 28},
     {NoFlag, 0xabababab, 0x00007fff, 29}, {NoFlag, 0xabababab, 0x00007fff, 30},
     {NoFlag, 0xabababab, 0x00007fff, 31}, {NoFlag, 0xabababab, 0x00007fff, 32},
     {NoFlag, 0xabababab, 0x33333333, 0},  {NoFlag, 0xabababab, 0x33333333, 1},
     {NoFlag, 0xabababab, 0x33333333, 2},  {NoFlag, 0xabababab, 0x33333333, 3},
     {NoFlag, 0xabababab, 0x33333333, 4},  {NoFlag, 0xabababab, 0x33333333, 5},
     {NoFlag, 0xabababab, 0x33333333, 6},  {NoFlag, 0xabababab, 0x33333333, 7},
     {NoFlag, 0xabababab, 0x33333333, 8},  {NoFlag, 0xabababab, 0x33333333, 9},
     {NoFlag, 0xabababab, 0x33333333, 10}, {NoFlag, 0xabababab, 0x33333333, 11},
     {NoFlag, 0xabababab, 0x33333333, 12}, {NoFlag, 0xabababab, 0x33333333, 13},
     {NoFlag, 0xabababab, 0x33333333, 14}, {NoFlag, 0xabababab, 0x33333333, 15},
     {NoFlag, 0xabababab, 0x33333333, 16}, {NoFlag, 0xabababab, 0x33333333, 17},
     {NoFlag, 0xabababab, 0x33333333, 18}, {NoFlag, 0xabababab, 0x33333333, 19},
     {NoFlag, 0xabababab, 0x33333333, 20}, {NoFlag, 0xabababab, 0x33333333, 21},
     {NoFlag, 0xabababab, 0x33333333, 22}, {NoFlag, 0xabababab, 0x33333333, 23},
     {NoFlag, 0xabababab, 0x33333333, 24}, {NoFlag, 0xabababab, 0x33333333, 25},
     {NoFlag, 0xabababab, 0x33333333, 26}, {NoFlag, 0xabababab, 0x33333333, 27},
     {NoFlag, 0xabababab, 0x33333333, 28}, {NoFlag, 0xabababab, 0x33333333, 29},
     {NoFlag, 0xabababab, 0x33333333, 30}, {NoFlag, 0xabababab, 0x33333333, 31},
     {NoFlag, 0xabababab, 0x33333333, 32}, {NoFlag, 0xabababab, 0x55555555, 0},
     {NoFlag, 0xabababab, 0x55555555, 1},  {NoFlag, 0xabababab, 0x55555555, 2},
     {NoFlag, 0xabababab, 0x55555555, 3},  {NoFlag, 0xabababab, 0x55555555, 4},
     {NoFlag, 0xabababab, 0x55555555, 5},  {NoFlag, 0xabababab, 0x55555555, 6},
     {NoFlag, 0xabababab, 0x55555555, 7},  {NoFlag, 0xabababab, 0x55555555, 8},
     {NoFlag, 0xabababab, 0x55555555, 9},  {NoFlag, 0xabababab, 0x55555555, 10},
     {NoFlag, 0xabababab, 0x55555555, 11}, {NoFlag, 0xabababab, 0x55555555, 12},
     {NoFlag, 0xabababab, 0x55555555, 13}, {NoFlag, 0xabababab, 0x55555555, 14},
     {NoFlag, 0xabababab, 0x55555555, 15}, {NoFlag, 0xabababab, 0x55555555, 16},
     {NoFlag, 0xabababab, 0x55555555, 17}, {NoFlag, 0xabababab, 0x55555555, 18},
     {NoFlag, 0xabababab, 0x55555555, 19}, {NoFlag, 0xabababab, 0x55555555, 20},
     {NoFlag, 0xabababab, 0x55555555, 21}, {NoFlag, 0xabababab, 0x55555555, 22},
     {NoFlag, 0xabababab, 0x55555555, 23}, {NoFlag, 0xabababab, 0x55555555, 24},
     {NoFlag, 0xabababab, 0x55555555, 25}, {NoFlag, 0xabababab, 0x55555555, 26},
     {NoFlag, 0xabababab, 0x55555555, 27}, {NoFlag, 0xabababab, 0x55555555, 28},
     {NoFlag, 0xabababab, 0x55555555, 29}, {NoFlag, 0xabababab, 0x55555555, 30},
     {NoFlag, 0xabababab, 0x55555555, 31}, {NoFlag, 0xabababab, 0x55555555, 32},
     {NoFlag, 0xabababab, 0x7ffffffd, 0},  {NoFlag, 0xabababab, 0x7ffffffd, 1},
     {NoFlag, 0xabababab, 0x7ffffffd, 2},  {NoFlag, 0xabababab, 0x7ffffffd, 3},
     {NoFlag, 0xabababab, 0x7ffffffd, 4},  {NoFlag, 0xabababab, 0x7ffffffd, 5},
     {NoFlag, 0xabababab, 0x7ffffffd, 6},  {NoFlag, 0xabababab, 0x7ffffffd, 7},
     {NoFlag, 0xabababab, 0x7ffffffd, 8},  {NoFlag, 0xabababab, 0x7ffffffd, 9},
     {NoFlag, 0xabababab, 0x7ffffffd, 10}, {NoFlag, 0xabababab, 0x7ffffffd, 11},
     {NoFlag, 0xabababab, 0x7ffffffd, 12}, {NoFlag, 0xabababab, 0x7ffffffd, 13},
     {NoFlag, 0xabababab, 0x7ffffffd, 14}, {NoFlag, 0xabababab, 0x7ffffffd, 15},
     {NoFlag, 0xabababab, 0x7ffffffd, 16}, {NoFlag, 0xabababab, 0x7ffffffd, 17},
     {NoFlag, 0xabababab, 0x7ffffffd, 18}, {NoFlag, 0xabababab, 0x7ffffffd, 19},
     {NoFlag, 0xabababab, 0x7ffffffd, 20}, {NoFlag, 0xabababab, 0x7ffffffd, 21},
     {NoFlag, 0xabababab, 0x7ffffffd, 22}, {NoFlag, 0xabababab, 0x7ffffffd, 23},
     {NoFlag, 0xabababab, 0x7ffffffd, 24}, {NoFlag, 0xabababab, 0x7ffffffd, 25},
     {NoFlag, 0xabababab, 0x7ffffffd, 26}, {NoFlag, 0xabababab, 0x7ffffffd, 27},
     {NoFlag, 0xabababab, 0x7ffffffd, 28}, {NoFlag, 0xabababab, 0x7ffffffd, 29},
     {NoFlag, 0xabababab, 0x7ffffffd, 30}, {NoFlag, 0xabababab, 0x7ffffffd, 31},
     {NoFlag, 0xabababab, 0x7ffffffd, 32}, {NoFlag, 0xabababab, 0x7ffffffe, 0},
     {NoFlag, 0xabababab, 0x7ffffffe, 1},  {NoFlag, 0xabababab, 0x7ffffffe, 2},
     {NoFlag, 0xabababab, 0x7ffffffe, 3},  {NoFlag, 0xabababab, 0x7ffffffe, 4},
     {NoFlag, 0xabababab, 0x7ffffffe, 5},  {NoFlag, 0xabababab, 0x7ffffffe, 6},
     {NoFlag, 0xabababab, 0x7ffffffe, 7},  {NoFlag, 0xabababab, 0x7ffffffe, 8},
     {NoFlag, 0xabababab, 0x7ffffffe, 9},  {NoFlag, 0xabababab, 0x7ffffffe, 10},
     {NoFlag, 0xabababab, 0x7ffffffe, 11}, {NoFlag, 0xabababab, 0x7ffffffe, 12},
     {NoFlag, 0xabababab, 0x7ffffffe, 13}, {NoFlag, 0xabababab, 0x7ffffffe, 14},
     {NoFlag, 0xabababab, 0x7ffffffe, 15}, {NoFlag, 0xabababab, 0x7ffffffe, 16},
     {NoFlag, 0xabababab, 0x7ffffffe, 17}, {NoFlag, 0xabababab, 0x7ffffffe, 18},
     {NoFlag, 0xabababab, 0x7ffffffe, 19}, {NoFlag, 0xabababab, 0x7ffffffe, 20},
     {NoFlag, 0xabababab, 0x7ffffffe, 21}, {NoFlag, 0xabababab, 0x7ffffffe, 22},
     {NoFlag, 0xabababab, 0x7ffffffe, 23}, {NoFlag, 0xabababab, 0x7ffffffe, 24},
     {NoFlag, 0xabababab, 0x7ffffffe, 25}, {NoFlag, 0xabababab, 0x7ffffffe, 26},
     {NoFlag, 0xabababab, 0x7ffffffe, 27}, {NoFlag, 0xabababab, 0x7ffffffe, 28},
     {NoFlag, 0xabababab, 0x7ffffffe, 29}, {NoFlag, 0xabababab, 0x7ffffffe, 30},
     {NoFlag, 0xabababab, 0x7ffffffe, 31}, {NoFlag, 0xabababab, 0x7ffffffe, 32},
     {NoFlag, 0xabababab, 0x7fffffff, 0},  {NoFlag, 0xabababab, 0x7fffffff, 1},
     {NoFlag, 0xabababab, 0x7fffffff, 2},  {NoFlag, 0xabababab, 0x7fffffff, 3},
     {NoFlag, 0xabababab, 0x7fffffff, 4},  {NoFlag, 0xabababab, 0x7fffffff, 5},
     {NoFlag, 0xabababab, 0x7fffffff, 6},  {NoFlag, 0xabababab, 0x7fffffff, 7},
     {NoFlag, 0xabababab, 0x7fffffff, 8},  {NoFlag, 0xabababab, 0x7fffffff, 9},
     {NoFlag, 0xabababab, 0x7fffffff, 10}, {NoFlag, 0xabababab, 0x7fffffff, 11},
     {NoFlag, 0xabababab, 0x7fffffff, 12}, {NoFlag, 0xabababab, 0x7fffffff, 13},
     {NoFlag, 0xabababab, 0x7fffffff, 14}, {NoFlag, 0xabababab, 0x7fffffff, 15},
     {NoFlag, 0xabababab, 0x7fffffff, 16}, {NoFlag, 0xabababab, 0x7fffffff, 17},
     {NoFlag, 0xabababab, 0x7fffffff, 18}, {NoFlag, 0xabababab, 0x7fffffff, 19},
     {NoFlag, 0xabababab, 0x7fffffff, 20}, {NoFlag, 0xabababab, 0x7fffffff, 21},
     {NoFlag, 0xabababab, 0x7fffffff, 22}, {NoFlag, 0xabababab, 0x7fffffff, 23},
     {NoFlag, 0xabababab, 0x7fffffff, 24}, {NoFlag, 0xabababab, 0x7fffffff, 25},
     {NoFlag, 0xabababab, 0x7fffffff, 26}, {NoFlag, 0xabababab, 0x7fffffff, 27},
     {NoFlag, 0xabababab, 0x7fffffff, 28}, {NoFlag, 0xabababab, 0x7fffffff, 29},
     {NoFlag, 0xabababab, 0x7fffffff, 30}, {NoFlag, 0xabababab, 0x7fffffff, 31},
     {NoFlag, 0xabababab, 0x7fffffff, 32}, {NoFlag, 0xabababab, 0x80000000, 0},
     {NoFlag, 0xabababab, 0x80000000, 1},  {NoFlag, 0xabababab, 0x80000000, 2},
     {NoFlag, 0xabababab, 0x80000000, 3},  {NoFlag, 0xabababab, 0x80000000, 4},
     {NoFlag, 0xabababab, 0x80000000, 5},  {NoFlag, 0xabababab, 0x80000000, 6},
     {NoFlag, 0xabababab, 0x80000000, 7},  {NoFlag, 0xabababab, 0x80000000, 8},
     {NoFlag, 0xabababab, 0x80000000, 9},  {NoFlag, 0xabababab, 0x80000000, 10},
     {NoFlag, 0xabababab, 0x80000000, 11}, {NoFlag, 0xabababab, 0x80000000, 12},
     {NoFlag, 0xabababab, 0x80000000, 13}, {NoFlag, 0xabababab, 0x80000000, 14},
     {NoFlag, 0xabababab, 0x80000000, 15}, {NoFlag, 0xabababab, 0x80000000, 16},
     {NoFlag, 0xabababab, 0x80000000, 17}, {NoFlag, 0xabababab, 0x80000000, 18},
     {NoFlag, 0xabababab, 0x80000000, 19}, {NoFlag, 0xabababab, 0x80000000, 20},
     {NoFlag, 0xabababab, 0x80000000, 21}, {NoFlag, 0xabababab, 0x80000000, 22},
     {NoFlag, 0xabababab, 0x80000000, 23}, {NoFlag, 0xabababab, 0x80000000, 24},
     {NoFlag, 0xabababab, 0x80000000, 25}, {NoFlag, 0xabababab, 0x80000000, 26},
     {NoFlag, 0xabababab, 0x80000000, 27}, {NoFlag, 0xabababab, 0x80000000, 28},
     {NoFlag, 0xabababab, 0x80000000, 29}, {NoFlag, 0xabababab, 0x80000000, 30},
     {NoFlag, 0xabababab, 0x80000000, 31}, {NoFlag, 0xabababab, 0x80000000, 32},
     {NoFlag, 0xabababab, 0x80000001, 0},  {NoFlag, 0xabababab, 0x80000001, 1},
     {NoFlag, 0xabababab, 0x80000001, 2},  {NoFlag, 0xabababab, 0x80000001, 3},
     {NoFlag, 0xabababab, 0x80000001, 4},  {NoFlag, 0xabababab, 0x80000001, 5},
     {NoFlag, 0xabababab, 0x80000001, 6},  {NoFlag, 0xabababab, 0x80000001, 7},
     {NoFlag, 0xabababab, 0x80000001, 8},  {NoFlag, 0xabababab, 0x80000001, 9},
     {NoFlag, 0xabababab, 0x80000001, 10}, {NoFlag, 0xabababab, 0x80000001, 11},
     {NoFlag, 0xabababab, 0x80000001, 12}, {NoFlag, 0xabababab, 0x80000001, 13},
     {NoFlag, 0xabababab, 0x80000001, 14}, {NoFlag, 0xabababab, 0x80000001, 15},
     {NoFlag, 0xabababab, 0x80000001, 16}, {NoFlag, 0xabababab, 0x80000001, 17},
     {NoFlag, 0xabababab, 0x80000001, 18}, {NoFlag, 0xabababab, 0x80000001, 19},
     {NoFlag, 0xabababab, 0x80000001, 20}, {NoFlag, 0xabababab, 0x80000001, 21},
     {NoFlag, 0xabababab, 0x80000001, 22}, {NoFlag, 0xabababab, 0x80000001, 23},
     {NoFlag, 0xabababab, 0x80000001, 24}, {NoFlag, 0xabababab, 0x80000001, 25},
     {NoFlag, 0xabababab, 0x80000001, 26}, {NoFlag, 0xabababab, 0x80000001, 27},
     {NoFlag, 0xabababab, 0x80000001, 28}, {NoFlag, 0xabababab, 0x80000001, 29},
     {NoFlag, 0xabababab, 0x80000001, 30}, {NoFlag, 0xabababab, 0x80000001, 31},
     {NoFlag, 0xabababab, 0x80000001, 32}, {NoFlag, 0xabababab, 0xaaaaaaaa, 0},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 1},  {NoFlag, 0xabababab, 0xaaaaaaaa, 2},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 3},  {NoFlag, 0xabababab, 0xaaaaaaaa, 4},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 5},  {NoFlag, 0xabababab, 0xaaaaaaaa, 6},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 7},  {NoFlag, 0xabababab, 0xaaaaaaaa, 8},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 9},  {NoFlag, 0xabababab, 0xaaaaaaaa, 10},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 11}, {NoFlag, 0xabababab, 0xaaaaaaaa, 12},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 13}, {NoFlag, 0xabababab, 0xaaaaaaaa, 14},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 15}, {NoFlag, 0xabababab, 0xaaaaaaaa, 16},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 17}, {NoFlag, 0xabababab, 0xaaaaaaaa, 18},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 19}, {NoFlag, 0xabababab, 0xaaaaaaaa, 20},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 21}, {NoFlag, 0xabababab, 0xaaaaaaaa, 22},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 23}, {NoFlag, 0xabababab, 0xaaaaaaaa, 24},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 25}, {NoFlag, 0xabababab, 0xaaaaaaaa, 26},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 27}, {NoFlag, 0xabababab, 0xaaaaaaaa, 28},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 29}, {NoFlag, 0xabababab, 0xaaaaaaaa, 30},
     {NoFlag, 0xabababab, 0xaaaaaaaa, 31}, {NoFlag, 0xabababab, 0xaaaaaaaa, 32},
     {NoFlag, 0xabababab, 0xcccccccc, 0},  {NoFlag, 0xabababab, 0xcccccccc, 1},
     {NoFlag, 0xabababab, 0xcccccccc, 2},  {NoFlag, 0xabababab, 0xcccccccc, 3},
     {NoFlag, 0xabababab, 0xcccccccc, 4},  {NoFlag, 0xabababab, 0xcccccccc, 5},
     {NoFlag, 0xabababab, 0xcccccccc, 6},  {NoFlag, 0xabababab, 0xcccccccc, 7},
     {NoFlag, 0xabababab, 0xcccccccc, 8},  {NoFlag, 0xabababab, 0xcccccccc, 9},
     {NoFlag, 0xabababab, 0xcccccccc, 10}, {NoFlag, 0xabababab, 0xcccccccc, 11},
     {NoFlag, 0xabababab, 0xcccccccc, 12}, {NoFlag, 0xabababab, 0xcccccccc, 13},
     {NoFlag, 0xabababab, 0xcccccccc, 14}, {NoFlag, 0xabababab, 0xcccccccc, 15},
     {NoFlag, 0xabababab, 0xcccccccc, 16}, {NoFlag, 0xabababab, 0xcccccccc, 17},
     {NoFlag, 0xabababab, 0xcccccccc, 18}, {NoFlag, 0xabababab, 0xcccccccc, 19},
     {NoFlag, 0xabababab, 0xcccccccc, 20}, {NoFlag, 0xabababab, 0xcccccccc, 21},
     {NoFlag, 0xabababab, 0xcccccccc, 22}, {NoFlag, 0xabababab, 0xcccccccc, 23},
     {NoFlag, 0xabababab, 0xcccccccc, 24}, {NoFlag, 0xabababab, 0xcccccccc, 25},
     {NoFlag, 0xabababab, 0xcccccccc, 26}, {NoFlag, 0xabababab, 0xcccccccc, 27},
     {NoFlag, 0xabababab, 0xcccccccc, 28}, {NoFlag, 0xabababab, 0xcccccccc, 29},
     {NoFlag, 0xabababab, 0xcccccccc, 30}, {NoFlag, 0xabababab, 0xcccccccc, 31},
     {NoFlag, 0xabababab, 0xcccccccc, 32}, {NoFlag, 0xabababab, 0xffff8000, 0},
     {NoFlag, 0xabababab, 0xffff8000, 1},  {NoFlag, 0xabababab, 0xffff8000, 2},
     {NoFlag, 0xabababab, 0xffff8000, 3},  {NoFlag, 0xabababab, 0xffff8000, 4},
     {NoFlag, 0xabababab, 0xffff8000, 5},  {NoFlag, 0xabababab, 0xffff8000, 6},
     {NoFlag, 0xabababab, 0xffff8000, 7},  {NoFlag, 0xabababab, 0xffff8000, 8},
     {NoFlag, 0xabababab, 0xffff8000, 9},  {NoFlag, 0xabababab, 0xffff8000, 10},
     {NoFlag, 0xabababab, 0xffff8000, 11}, {NoFlag, 0xabababab, 0xffff8000, 12},
     {NoFlag, 0xabababab, 0xffff8000, 13}, {NoFlag, 0xabababab, 0xffff8000, 14},
     {NoFlag, 0xabababab, 0xffff8000, 15}, {NoFlag, 0xabababab, 0xffff8000, 16},
     {NoFlag, 0xabababab, 0xffff8000, 17}, {NoFlag, 0xabababab, 0xffff8000, 18},
     {NoFlag, 0xabababab, 0xffff8000, 19}, {NoFlag, 0xabababab, 0xffff8000, 20},
     {NoFlag, 0xabababab, 0xffff8000, 21}, {NoFlag, 0xabababab, 0xffff8000, 22},
     {NoFlag, 0xabababab, 0xffff8000, 23}, {NoFlag, 0xabababab, 0xffff8000, 24},
     {NoFlag, 0xabababab, 0xffff8000, 25}, {NoFlag, 0xabababab, 0xffff8000, 26},
     {NoFlag, 0xabababab, 0xffff8000, 27}, {NoFlag, 0xabababab, 0xffff8000, 28},
     {NoFlag, 0xabababab, 0xffff8000, 29}, {NoFlag, 0xabababab, 0xffff8000, 30},
     {NoFlag, 0xabababab, 0xffff8000, 31}, {NoFlag, 0xabababab, 0xffff8000, 32},
     {NoFlag, 0xabababab, 0xffff8001, 0},  {NoFlag, 0xabababab, 0xffff8001, 1},
     {NoFlag, 0xabababab, 0xffff8001, 2},  {NoFlag, 0xabababab, 0xffff8001, 3},
     {NoFlag, 0xabababab, 0xffff8001, 4},  {NoFlag, 0xabababab, 0xffff8001, 5},
     {NoFlag, 0xabababab, 0xffff8001, 6},  {NoFlag, 0xabababab, 0xffff8001, 7},
     {NoFlag, 0xabababab, 0xffff8001, 8},  {NoFlag, 0xabababab, 0xffff8001, 9},
     {NoFlag, 0xabababab, 0xffff8001, 10}, {NoFlag, 0xabababab, 0xffff8001, 11},
     {NoFlag, 0xabababab, 0xffff8001, 12}, {NoFlag, 0xabababab, 0xffff8001, 13},
     {NoFlag, 0xabababab, 0xffff8001, 14}, {NoFlag, 0xabababab, 0xffff8001, 15},
     {NoFlag, 0xabababab, 0xffff8001, 16}, {NoFlag, 0xabababab, 0xffff8001, 17},
     {NoFlag, 0xabababab, 0xffff8001, 18}, {NoFlag, 0xabababab, 0xffff8001, 19},
     {NoFlag, 0xabababab, 0xffff8001, 20}, {NoFlag, 0xabababab, 0xffff8001, 21},
     {NoFlag, 0xabababab, 0xffff8001, 22}, {NoFlag, 0xabababab, 0xffff8001, 23},
     {NoFlag, 0xabababab, 0xffff8001, 24}, {NoFlag, 0xabababab, 0xffff8001, 25},
     {NoFlag, 0xabababab, 0xffff8001, 26}, {NoFlag, 0xabababab, 0xffff8001, 27},
     {NoFlag, 0xabababab, 0xffff8001, 28}, {NoFlag, 0xabababab, 0xffff8001, 29},
     {NoFlag, 0xabababab, 0xffff8001, 30}, {NoFlag, 0xabababab, 0xffff8001, 31},
     {NoFlag, 0xabababab, 0xffff8001, 32}, {NoFlag, 0xabababab, 0xffff8002, 0},
     {NoFlag, 0xabababab, 0xffff8002, 1},  {NoFlag, 0xabababab, 0xffff8002, 2},
     {NoFlag, 0xabababab, 0xffff8002, 3},  {NoFlag, 0xabababab, 0xffff8002, 4},
     {NoFlag, 0xabababab, 0xffff8002, 5},  {NoFlag, 0xabababab, 0xffff8002, 6},
     {NoFlag, 0xabababab, 0xffff8002, 7},  {NoFlag, 0xabababab, 0xffff8002, 8},
     {NoFlag, 0xabababab, 0xffff8002, 9},  {NoFlag, 0xabababab, 0xffff8002, 10},
     {NoFlag, 0xabababab, 0xffff8002, 11}, {NoFlag, 0xabababab, 0xffff8002, 12},
     {NoFlag, 0xabababab, 0xffff8002, 13}, {NoFlag, 0xabababab, 0xffff8002, 14},
     {NoFlag, 0xabababab, 0xffff8002, 15}, {NoFlag, 0xabababab, 0xffff8002, 16},
     {NoFlag, 0xabababab, 0xffff8002, 17}, {NoFlag, 0xabababab, 0xffff8002, 18},
     {NoFlag, 0xabababab, 0xffff8002, 19}, {NoFlag, 0xabababab, 0xffff8002, 20},
     {NoFlag, 0xabababab, 0xffff8002, 21}, {NoFlag, 0xabababab, 0xffff8002, 22},
     {NoFlag, 0xabababab, 0xffff8002, 23}, {NoFlag, 0xabababab, 0xffff8002, 24},
     {NoFlag, 0xabababab, 0xffff8002, 25}, {NoFlag, 0xabababab, 0xffff8002, 26},
     {NoFlag, 0xabababab, 0xffff8002, 27}, {NoFlag, 0xabababab, 0xffff8002, 28},
     {NoFlag, 0xabababab, 0xffff8002, 29}, {NoFlag, 0xabababab, 0xffff8002, 30},
     {NoFlag, 0xabababab, 0xffff8002, 31}, {NoFlag, 0xabababab, 0xffff8002, 32},
     {NoFlag, 0xabababab, 0xffff8003, 0},  {NoFlag, 0xabababab, 0xffff8003, 1},
     {NoFlag, 0xabababab, 0xffff8003, 2},  {NoFlag, 0xabababab, 0xffff8003, 3},
     {NoFlag, 0xabababab, 0xffff8003, 4},  {NoFlag, 0xabababab, 0xffff8003, 5},
     {NoFlag, 0xabababab, 0xffff8003, 6},  {NoFlag, 0xabababab, 0xffff8003, 7},
     {NoFlag, 0xabababab, 0xffff8003, 8},  {NoFlag, 0xabababab, 0xffff8003, 9},
     {NoFlag, 0xabababab, 0xffff8003, 10}, {NoFlag, 0xabababab, 0xffff8003, 11},
     {NoFlag, 0xabababab, 0xffff8003, 12}, {NoFlag, 0xabababab, 0xffff8003, 13},
     {NoFlag, 0xabababab, 0xffff8003, 14}, {NoFlag, 0xabababab, 0xffff8003, 15},
     {NoFlag, 0xabababab, 0xffff8003, 16}, {NoFlag, 0xabababab, 0xffff8003, 17},
     {NoFlag, 0xabababab, 0xffff8003, 18}, {NoFlag, 0xabababab, 0xffff8003, 19},
     {NoFlag, 0xabababab, 0xffff8003, 20}, {NoFlag, 0xabababab, 0xffff8003, 21},
     {NoFlag, 0xabababab, 0xffff8003, 22}, {NoFlag, 0xabababab, 0xffff8003, 23},
     {NoFlag, 0xabababab, 0xffff8003, 24}, {NoFlag, 0xabababab, 0xffff8003, 25},
     {NoFlag, 0xabababab, 0xffff8003, 26}, {NoFlag, 0xabababab, 0xffff8003, 27},
     {NoFlag, 0xabababab, 0xffff8003, 28}, {NoFlag, 0xabababab, 0xffff8003, 29},
     {NoFlag, 0xabababab, 0xffff8003, 30}, {NoFlag, 0xabababab, 0xffff8003, 31},
     {NoFlag, 0xabababab, 0xffff8003, 32}, {NoFlag, 0xabababab, 0xffffff80, 0},
     {NoFlag, 0xabababab, 0xffffff80, 1},  {NoFlag, 0xabababab, 0xffffff80, 2},
     {NoFlag, 0xabababab, 0xffffff80, 3},  {NoFlag, 0xabababab, 0xffffff80, 4},
     {NoFlag, 0xabababab, 0xffffff80, 5},  {NoFlag, 0xabababab, 0xffffff80, 6},
     {NoFlag, 0xabababab, 0xffffff80, 7},  {NoFlag, 0xabababab, 0xffffff80, 8},
     {NoFlag, 0xabababab, 0xffffff80, 9},  {NoFlag, 0xabababab, 0xffffff80, 10},
     {NoFlag, 0xabababab, 0xffffff80, 11}, {NoFlag, 0xabababab, 0xffffff80, 12},
     {NoFlag, 0xabababab, 0xffffff80, 13}, {NoFlag, 0xabababab, 0xffffff80, 14},
     {NoFlag, 0xabababab, 0xffffff80, 15}, {NoFlag, 0xabababab, 0xffffff80, 16},
     {NoFlag, 0xabababab, 0xffffff80, 17}, {NoFlag, 0xabababab, 0xffffff80, 18},
     {NoFlag, 0xabababab, 0xffffff80, 19}, {NoFlag, 0xabababab, 0xffffff80, 20},
     {NoFlag, 0xabababab, 0xffffff80, 21}, {NoFlag, 0xabababab, 0xffffff80, 22},
     {NoFlag, 0xabababab, 0xffffff80, 23}, {NoFlag, 0xabababab, 0xffffff80, 24},
     {NoFlag, 0xabababab, 0xffffff80, 25}, {NoFlag, 0xabababab, 0xffffff80, 26},
     {NoFlag, 0xabababab, 0xffffff80, 27}, {NoFlag, 0xabababab, 0xffffff80, 28},
     {NoFlag, 0xabababab, 0xffffff80, 29}, {NoFlag, 0xabababab, 0xffffff80, 30},
     {NoFlag, 0xabababab, 0xffffff80, 31}, {NoFlag, 0xabababab, 0xffffff80, 32},
     {NoFlag, 0xabababab, 0xffffff81, 0},  {NoFlag, 0xabababab, 0xffffff81, 1},
     {NoFlag, 0xabababab, 0xffffff81, 2},  {NoFlag, 0xabababab, 0xffffff81, 3},
     {NoFlag, 0xabababab, 0xffffff81, 4},  {NoFlag, 0xabababab, 0xffffff81, 5},
     {NoFlag, 0xabababab, 0xffffff81, 6},  {NoFlag, 0xabababab, 0xffffff81, 7},
     {NoFlag, 0xabababab, 0xffffff81, 8},  {NoFlag, 0xabababab, 0xffffff81, 9},
     {NoFlag, 0xabababab, 0xffffff81, 10}, {NoFlag, 0xabababab, 0xffffff81, 11},
     {NoFlag, 0xabababab, 0xffffff81, 12}, {NoFlag, 0xabababab, 0xffffff81, 13},
     {NoFlag, 0xabababab, 0xffffff81, 14}, {NoFlag, 0xabababab, 0xffffff81, 15},
     {NoFlag, 0xabababab, 0xffffff81, 16}, {NoFlag, 0xabababab, 0xffffff81, 17},
     {NoFlag, 0xabababab, 0xffffff81, 18}, {NoFlag, 0xabababab, 0xffffff81, 19},
     {NoFlag, 0xabababab, 0xffffff81, 20}, {NoFlag, 0xabababab, 0xffffff81, 21},
     {NoFlag, 0xabababab, 0xffffff81, 22}, {NoFlag, 0xabababab, 0xffffff81, 23},
     {NoFlag, 0xabababab, 0xffffff81, 24}, {NoFlag, 0xabababab, 0xffffff81, 25},
     {NoFlag, 0xabababab, 0xffffff81, 26}, {NoFlag, 0xabababab, 0xffffff81, 27},
     {NoFlag, 0xabababab, 0xffffff81, 28}, {NoFlag, 0xabababab, 0xffffff81, 29},
     {NoFlag, 0xabababab, 0xffffff81, 30}, {NoFlag, 0xabababab, 0xffffff81, 31},
     {NoFlag, 0xabababab, 0xffffff81, 32}, {NoFlag, 0xabababab, 0xffffff82, 0},
     {NoFlag, 0xabababab, 0xffffff82, 1},  {NoFlag, 0xabababab, 0xffffff82, 2},
     {NoFlag, 0xabababab, 0xffffff82, 3},  {NoFlag, 0xabababab, 0xffffff82, 4},
     {NoFlag, 0xabababab, 0xffffff82, 5},  {NoFlag, 0xabababab, 0xffffff82, 6},
     {NoFlag, 0xabababab, 0xffffff82, 7},  {NoFlag, 0xabababab, 0xffffff82, 8},
     {NoFlag, 0xabababab, 0xffffff82, 9},  {NoFlag, 0xabababab, 0xffffff82, 10},
     {NoFlag, 0xabababab, 0xffffff82, 11}, {NoFlag, 0xabababab, 0xffffff82, 12},
     {NoFlag, 0xabababab, 0xffffff82, 13}, {NoFlag, 0xabababab, 0xffffff82, 14},
     {NoFlag, 0xabababab, 0xffffff82, 15}, {NoFlag, 0xabababab, 0xffffff82, 16},
     {NoFlag, 0xabababab, 0xffffff82, 17}, {NoFlag, 0xabababab, 0xffffff82, 18},
     {NoFlag, 0xabababab, 0xffffff82, 19}, {NoFlag, 0xabababab, 0xffffff82, 20},
     {NoFlag, 0xabababab, 0xffffff82, 21}, {NoFlag, 0xabababab, 0xffffff82, 22},
     {NoFlag, 0xabababab, 0xffffff82, 23}, {NoFlag, 0xabababab, 0xffffff82, 24},
     {NoFlag, 0xabababab, 0xffffff82, 25}, {NoFlag, 0xabababab, 0xffffff82, 26},
     {NoFlag, 0xabababab, 0xffffff82, 27}, {NoFlag, 0xabababab, 0xffffff82, 28},
     {NoFlag, 0xabababab, 0xffffff82, 29}, {NoFlag, 0xabababab, 0xffffff82, 30},
     {NoFlag, 0xabababab, 0xffffff82, 31}, {NoFlag, 0xabababab, 0xffffff82, 32},
     {NoFlag, 0xabababab, 0xffffff83, 0},  {NoFlag, 0xabababab, 0xffffff83, 1},
     {NoFlag, 0xabababab, 0xffffff83, 2},  {NoFlag, 0xabababab, 0xffffff83, 3},
     {NoFlag, 0xabababab, 0xffffff83, 4},  {NoFlag, 0xabababab, 0xffffff83, 5},
     {NoFlag, 0xabababab, 0xffffff83, 6},  {NoFlag, 0xabababab, 0xffffff83, 7},
     {NoFlag, 0xabababab, 0xffffff83, 8},  {NoFlag, 0xabababab, 0xffffff83, 9},
     {NoFlag, 0xabababab, 0xffffff83, 10}, {NoFlag, 0xabababab, 0xffffff83, 11},
     {NoFlag, 0xabababab, 0xffffff83, 12}, {NoFlag, 0xabababab, 0xffffff83, 13},
     {NoFlag, 0xabababab, 0xffffff83, 14}, {NoFlag, 0xabababab, 0xffffff83, 15},
     {NoFlag, 0xabababab, 0xffffff83, 16}, {NoFlag, 0xabababab, 0xffffff83, 17},
     {NoFlag, 0xabababab, 0xffffff83, 18}, {NoFlag, 0xabababab, 0xffffff83, 19},
     {NoFlag, 0xabababab, 0xffffff83, 20}, {NoFlag, 0xabababab, 0xffffff83, 21},
     {NoFlag, 0xabababab, 0xffffff83, 22}, {NoFlag, 0xabababab, 0xffffff83, 23},
     {NoFlag, 0xabababab, 0xffffff83, 24}, {NoFlag, 0xabababab, 0xffffff83, 25},
     {NoFlag, 0xabababab, 0xffffff83, 26}, {NoFlag, 0xabababab, 0xffffff83, 27},
     {NoFlag, 0xabababab, 0xffffff83, 28}, {NoFlag, 0xabababab, 0xffffff83, 29},
     {NoFlag, 0xabababab, 0xffffff83, 30}, {NoFlag, 0xabababab, 0xffffff83, 31},
     {NoFlag, 0xabababab, 0xffffff83, 32}, {NoFlag, 0xabababab, 0xffffffe0, 0},
     {NoFlag, 0xabababab, 0xffffffe0, 1},  {NoFlag, 0xabababab, 0xffffffe0, 2},
     {NoFlag, 0xabababab, 0xffffffe0, 3},  {NoFlag, 0xabababab, 0xffffffe0, 4},
     {NoFlag, 0xabababab, 0xffffffe0, 5},  {NoFlag, 0xabababab, 0xffffffe0, 6},
     {NoFlag, 0xabababab, 0xffffffe0, 7},  {NoFlag, 0xabababab, 0xffffffe0, 8},
     {NoFlag, 0xabababab, 0xffffffe0, 9},  {NoFlag, 0xabababab, 0xffffffe0, 10},
     {NoFlag, 0xabababab, 0xffffffe0, 11}, {NoFlag, 0xabababab, 0xffffffe0, 12},
     {NoFlag, 0xabababab, 0xffffffe0, 13}, {NoFlag, 0xabababab, 0xffffffe0, 14},
     {NoFlag, 0xabababab, 0xffffffe0, 15}, {NoFlag, 0xabababab, 0xffffffe0, 16},
     {NoFlag, 0xabababab, 0xffffffe0, 17}, {NoFlag, 0xabababab, 0xffffffe0, 18},
     {NoFlag, 0xabababab, 0xffffffe0, 19}, {NoFlag, 0xabababab, 0xffffffe0, 20},
     {NoFlag, 0xabababab, 0xffffffe0, 21}, {NoFlag, 0xabababab, 0xffffffe0, 22},
     {NoFlag, 0xabababab, 0xffffffe0, 23}, {NoFlag, 0xabababab, 0xffffffe0, 24},
     {NoFlag, 0xabababab, 0xffffffe0, 25}, {NoFlag, 0xabababab, 0xffffffe0, 26},
     {NoFlag, 0xabababab, 0xffffffe0, 27}, {NoFlag, 0xabababab, 0xffffffe0, 28},
     {NoFlag, 0xabababab, 0xffffffe0, 29}, {NoFlag, 0xabababab, 0xffffffe0, 30},
     {NoFlag, 0xabababab, 0xffffffe0, 31}, {NoFlag, 0xabababab, 0xffffffe0, 32},
     {NoFlag, 0xabababab, 0xfffffffd, 0},  {NoFlag, 0xabababab, 0xfffffffd, 1},
     {NoFlag, 0xabababab, 0xfffffffd, 2},  {NoFlag, 0xabababab, 0xfffffffd, 3},
     {NoFlag, 0xabababab, 0xfffffffd, 4},  {NoFlag, 0xabababab, 0xfffffffd, 5},
     {NoFlag, 0xabababab, 0xfffffffd, 6},  {NoFlag, 0xabababab, 0xfffffffd, 7},
     {NoFlag, 0xabababab, 0xfffffffd, 8},  {NoFlag, 0xabababab, 0xfffffffd, 9},
     {NoFlag, 0xabababab, 0xfffffffd, 10}, {NoFlag, 0xabababab, 0xfffffffd, 11},
     {NoFlag, 0xabababab, 0xfffffffd, 12}, {NoFlag, 0xabababab, 0xfffffffd, 13},
     {NoFlag, 0xabababab, 0xfffffffd, 14}, {NoFlag, 0xabababab, 0xfffffffd, 15},
     {NoFlag, 0xabababab, 0xfffffffd, 16}, {NoFlag, 0xabababab, 0xfffffffd, 17},
     {NoFlag, 0xabababab, 0xfffffffd, 18}, {NoFlag, 0xabababab, 0xfffffffd, 19},
     {NoFlag, 0xabababab, 0xfffffffd, 20}, {NoFlag, 0xabababab, 0xfffffffd, 21},
     {NoFlag, 0xabababab, 0xfffffffd, 22}, {NoFlag, 0xabababab, 0xfffffffd, 23},
     {NoFlag, 0xabababab, 0xfffffffd, 24}, {NoFlag, 0xabababab, 0xfffffffd, 25},
     {NoFlag, 0xabababab, 0xfffffffd, 26}, {NoFlag, 0xabababab, 0xfffffffd, 27},
     {NoFlag, 0xabababab, 0xfffffffd, 28}, {NoFlag, 0xabababab, 0xfffffffd, 29},
     {NoFlag, 0xabababab, 0xfffffffd, 30}, {NoFlag, 0xabababab, 0xfffffffd, 31},
     {NoFlag, 0xabababab, 0xfffffffd, 32}, {NoFlag, 0xabababab, 0xfffffffe, 0},
     {NoFlag, 0xabababab, 0xfffffffe, 1},  {NoFlag, 0xabababab, 0xfffffffe, 2},
     {NoFlag, 0xabababab, 0xfffffffe, 3},  {NoFlag, 0xabababab, 0xfffffffe, 4},
     {NoFlag, 0xabababab, 0xfffffffe, 5},  {NoFlag, 0xabababab, 0xfffffffe, 6},
     {NoFlag, 0xabababab, 0xfffffffe, 7},  {NoFlag, 0xabababab, 0xfffffffe, 8},
     {NoFlag, 0xabababab, 0xfffffffe, 9},  {NoFlag, 0xabababab, 0xfffffffe, 10},
     {NoFlag, 0xabababab, 0xfffffffe, 11}, {NoFlag, 0xabababab, 0xfffffffe, 12},
     {NoFlag, 0xabababab, 0xfffffffe, 13}, {NoFlag, 0xabababab, 0xfffffffe, 14},
     {NoFlag, 0xabababab, 0xfffffffe, 15}, {NoFlag, 0xabababab, 0xfffffffe, 16},
     {NoFlag, 0xabababab, 0xfffffffe, 17}, {NoFlag, 0xabababab, 0xfffffffe, 18},
     {NoFlag, 0xabababab, 0xfffffffe, 19}, {NoFlag, 0xabababab, 0xfffffffe, 20},
     {NoFlag, 0xabababab, 0xfffffffe, 21}, {NoFlag, 0xabababab, 0xfffffffe, 22},
     {NoFlag, 0xabababab, 0xfffffffe, 23}, {NoFlag, 0xabababab, 0xfffffffe, 24},
     {NoFlag, 0xabababab, 0xfffffffe, 25}, {NoFlag, 0xabababab, 0xfffffffe, 26},
     {NoFlag, 0xabababab, 0xfffffffe, 27}, {NoFlag, 0xabababab, 0xfffffffe, 28},
     {NoFlag, 0xabababab, 0xfffffffe, 29}, {NoFlag, 0xabababab, 0xfffffffe, 30},
     {NoFlag, 0xabababab, 0xfffffffe, 31}, {NoFlag, 0xabababab, 0xfffffffe, 32},
     {NoFlag, 0xabababab, 0xffffffff, 0},  {NoFlag, 0xabababab, 0xffffffff, 1},
     {NoFlag, 0xabababab, 0xffffffff, 2},  {NoFlag, 0xabababab, 0xffffffff, 3},
     {NoFlag, 0xabababab, 0xffffffff, 4},  {NoFlag, 0xabababab, 0xffffffff, 5},
     {NoFlag, 0xabababab, 0xffffffff, 6},  {NoFlag, 0xabababab, 0xffffffff, 7},
     {NoFlag, 0xabababab, 0xffffffff, 8},  {NoFlag, 0xabababab, 0xffffffff, 9},
     {NoFlag, 0xabababab, 0xffffffff, 10}, {NoFlag, 0xabababab, 0xffffffff, 11},
     {NoFlag, 0xabababab, 0xffffffff, 12}, {NoFlag, 0xabababab, 0xffffffff, 13},
     {NoFlag, 0xabababab, 0xffffffff, 14}, {NoFlag, 0xabababab, 0xffffffff, 15},
     {NoFlag, 0xabababab, 0xffffffff, 16}, {NoFlag, 0xabababab, 0xffffffff, 17},
     {NoFlag, 0xabababab, 0xffffffff, 18}, {NoFlag, 0xabababab, 0xffffffff, 19},
     {NoFlag, 0xabababab, 0xffffffff, 20}, {NoFlag, 0xabababab, 0xffffffff, 21},
     {NoFlag, 0xabababab, 0xffffffff, 22}, {NoFlag, 0xabababab, 0xffffffff, 23},
     {NoFlag, 0xabababab, 0xffffffff, 24}, {NoFlag, 0xabababab, 0xffffffff, 25},
     {NoFlag, 0xabababab, 0xffffffff, 26}, {NoFlag, 0xabababab, 0xffffffff, 27},
     {NoFlag, 0xabababab, 0xffffffff, 28}, {NoFlag, 0xabababab, 0xffffffff, 29},
     {NoFlag, 0xabababab, 0xffffffff, 30}, {NoFlag, 0xabababab, 0xffffffff, 31},
     {NoFlag, 0xabababab, 0xffffffff, 32}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{eq, r0, r0, LSL, r0},
                                "eq r0 r0 LSL r0",
                                "Condition_eq_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ne, r0, r0, LSL, r0},
                                "ne r0 r0 LSL r0",
                                "Condition_ne_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{cs, r0, r0, LSL, r0},
                                "cs r0 r0 LSL r0",
                                "Condition_cs_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{cc, r0, r0, LSL, r0},
                                "cc r0 r0 LSL r0",
                                "Condition_cc_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{mi, r0, r0, LSL, r0},
                                "mi r0 r0 LSL r0",
                                "Condition_mi_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{pl, r0, r0, LSL, r0},
                                "pl r0 r0 LSL r0",
                                "Condition_pl_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{vs, r0, r0, LSL, r0},
                                "vs r0 r0 LSL r0",
                                "Condition_vs_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{vc, r0, r0, LSL, r0},
                                "vc r0 r0 LSL r0",
                                "Condition_vc_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{hi, r0, r0, LSL, r0},
                                "hi r0 r0 LSL r0",
                                "Condition_hi_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ls, r0, r0, LSL, r0},
                                "ls r0 r0 LSL r0",
                                "Condition_ls_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ge, r0, r0, LSL, r0},
                                "ge r0 r0 LSL r0",
                                "Condition_ge_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{lt, r0, r0, LSL, r0},
                                "lt r0 r0 LSL r0",
                                "Condition_lt_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{gt, r0, r0, LSL, r0},
                                "gt r0 r0 LSL r0",
                                "Condition_gt_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{le, r0, r0, LSL, r0},
                                "le r0 r0 LSL r0",
                                "Condition_le_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{al, r0, r0, LSL, r0},
                                "al r0 r0 LSL r0",
                                "Condition_al_r0_r0_LSL_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{al, r0, r0, LSL, r0},
                                "al r0 r0 LSL r0",
                                "RdIsRn_al_r0_r0_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r1, r1, LSL, r0},
                                "al r1 r1 LSL r0",
                                "RdIsRn_al_r1_r1_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r2, r2, LSL, r0},
                                "al r2 r2 LSL r0",
                                "RdIsRn_al_r2_r2_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r3, r3, LSL, r0},
                                "al r3 r3 LSL r0",
                                "RdIsRn_al_r3_r3_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r4, r4, LSL, r0},
                                "al r4 r4 LSL r0",
                                "RdIsRn_al_r4_r4_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r5, r5, LSL, r0},
                                "al r5 r5 LSL r0",
                                "RdIsRn_al_r5_r5_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r6, r6, LSL, r0},
                                "al r6 r6 LSL r0",
                                "RdIsRn_al_r6_r6_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r7, r7, LSL, r0},
                                "al r7 r7 LSL r0",
                                "RdIsRn_al_r7_r7_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r8, r8, LSL, r0},
                                "al r8 r8 LSL r0",
                                "RdIsRn_al_r8_r8_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r9, r9, LSL, r0},
                                "al r9 r9 LSL r0",
                                "RdIsRn_al_r9_r9_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r10, r10, LSL, r0},
                                "al r10 r10 LSL r0",
                                "RdIsRn_al_r10_r10_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r11, r11, LSL, r0},
                                "al r11 r11 LSL r0",
                                "RdIsRn_al_r11_r11_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r12, r12, LSL, r0},
                                "al r12 r12 LSL r0",
                                "RdIsRn_al_r12_r12_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r14, r14, LSL, r0},
                                "al r14 r14 LSL r0",
                                "RdIsRn_al_r14_r14_LSL_r0",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{al, r1, r8, LSL, r0},
                                "al r1 r8 LSL r0",
                                "RdIsNotRn_al_r1_r8_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r7, r4, LSL, r0},
                                "al r7 r4 LSL r0",
                                "RdIsNotRn_al_r7_r4_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r14, r10, LSL, r0},
                                "al r14 r10 LSL r0",
                                "RdIsNotRn_al_r14_r10_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r10, r6, LSL, r0},
                                "al r10 r6 LSL r0",
                                "RdIsNotRn_al_r10_r6_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r6, r5, LSL, r0},
                                "al r6 r5 LSL r0",
                                "RdIsNotRn_al_r6_r5_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r12, r2, LSL, r0},
                                "al r12 r2 LSL r0",
                                "RdIsNotRn_al_r12_r2_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r11, LSL, r0},
                                "al r0 r11 LSL r0",
                                "RdIsNotRn_al_r0_r11_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r10, r14, LSL, r0},
                                "al r10 r14 LSL r0",
                                "RdIsNotRn_al_r10_r14_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r5, LSL, r0},
                                "al r0 r5 LSL r0",
                                "RdIsNotRn_al_r0_r5_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r3, LSL, r0},
                                "al r0 r3 LSL r0",
                                "RdIsNotRn_al_r0_r3_LSL_r0",
                                ARRAY_SIZE(kRdIsNotRn),
                                kRdIsNotRn},
                               {{al, r0, r1, LSL, r2},
                                "al r0 r1 LSL r2",
                                "ShiftTypes_al_r0_r1_LSL_r2",
                                ARRAY_SIZE(kShiftTypes),
                                kShiftTypes},
                               {{al, r0, r1, LSR, r2},
                                "al r0 r1 LSR r2",
                                "ShiftTypes_al_r0_r1_LSR_r2",
                                ARRAY_SIZE(kShiftTypes),
                                kShiftTypes},
                               {{al, r0, r1, ASR, r2},
                                "al r0 r1 ASR r2",
                                "ShiftTypes_al_r0_r1_ASR_r2",
                                ARRAY_SIZE(kShiftTypes),
                                kShiftTypes},
                               {{al, r0, r1, ROR, r2},
                                "al r0 r1 ROR r2",
                                "ShiftTypes_al_r0_r1_ROR_r2",
                                ARRAY_SIZE(kShiftTypes),
                                kShiftTypes}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-rd-operand-rn-shift-rs-mov-t32.h"
#include "aarch32/traces/simulator-cond-rd-operand-rn-shift-rs-movs-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
                                   const Operand& op);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  SETUP();
  masm.UseT32();
  START();

  // Data to compare to `reference`.
  TestResult* results[ARRAY_SIZE(kTests)];

  // Test cases for memory bound instructions may allocate a buffer and save its
  // address in this array.
  byte* scratch_memory_buffers[ARRAY_SIZE(kTests)];

  // Generate a loop for each element in `kTests`. Each loop tests one specific
  // instruction.
  for (unsigned i = 0; i < ARRAY_SIZE(kTests); i++) {
    // Allocate results on the heap for this test.
    results[i] = new TestResult;
    results[i]->outputs = new Inputs[kTests[i].input_size];
    results[i]->output_size = kTests[i].input_size;

    size_t input_stride = sizeof(kTests[i].inputs[0]) * kTests[i].input_size;
    VIXL_ASSERT(IsUint32(input_stride));

    scratch_memory_buffers[i] = NULL;

    Label loop;
    UseScratchRegisterScope scratch_registers(&masm);
    // Include all registers from r0 ro r12.
    scratch_registers.Include(RegisterList(0x1fff));

    // Values to pass to the macro-assembler.
    Condition cond = kTests[i].operands.cond;
    Register rd = kTests[i].operands.rd;
    Register rn = kTests[i].operands.rn;
    ShiftType shift = kTests[i].operands.shift;
    Register rs = kTests[i].operands.rs;
    Operand op(rn, shift, rs);
    scratch_registers.Exclude(rd);
    scratch_registers.Exclude(rn);
    scratch_registers.Exclude(rs);

    // Allocate reserved registers for our own use.
    Register input_ptr = scratch_registers.Acquire();
    Register input_end = scratch_registers.Acquire();
    Register result_ptr = scratch_registers.Acquire();

    // Initialize `input_ptr` to the first element and `input_end` the address
    // after the array.
    __ Mov(input_ptr, Operand::From(kTests[i].inputs));
    __ Add(input_end, input_ptr, static_cast<uint32_t>(input_stride));
    __ Mov(result_ptr, Operand::From(results[i]->outputs));
    __ Bind(&loop);

    {
      UseScratchRegisterScope temp_registers(&masm);
      Register nzcv_bits = temp_registers.Acquire();
      Register saved_q_bit = temp_registers.Acquire();
      // Save the `Q` bit flag.
      __ Mrs(saved_q_bit, APSR);
      __ And(saved_q_bit, saved_q_bit, QFlag);
      // Set the `NZCV` and `Q` flags together.
      __ Ldr(nzcv_bits, MemOperand(input_ptr, offsetof(Inputs, apsr)));
      __ Orr(nzcv_bits, nzcv_bits, saved_q_bit);
      __ Msr(APSR_nzcvq, nzcv_bits);
    }
    __ Ldr(rd, MemOperand(input_ptr, offsetof(Inputs, rd)));
    __ Ldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));
    __ Ldr(rs, MemOperand(input_ptr, offsetof(Inputs, rs)));

    (masm.*instruction)(cond, rd, op);

    {
      UseScratchRegisterScope temp_registers(&masm);
      Register nzcv_bits = temp_registers.Acquire();
      __ Mrs(nzcv_bits, APSR);
      // Only record the NZCV bits.
      __ And(nzcv_bits, nzcv_bits, NZCVFlag);
      __ Str(nzcv_bits, MemOperand(result_ptr, offsetof(Inputs, apsr)));
    }
    __ Str(rd, MemOperand(result_ptr, offsetof(Inputs, rd)));
    __ Str(rn, MemOperand(result_ptr, offsetof(Inputs, rn)));
    __ Str(rs, MemOperand(result_ptr, offsetof(Inputs, rs)));

    // Advance the result pointer.
    __ Add(result_ptr, result_ptr, Operand::From(sizeof(kTests[i].inputs[0])));
    // Loop back until `input_ptr` is lower than `input_base`.
    __ Add(input_ptr, input_ptr, Operand::From(sizeof(kTests[i].inputs[0])));
    __ Cmp(input_ptr, input_end);
    __ B(ne, &loop);
  }

  END();

  RUN();

  if (Test::generate_test_trace()) {
    // Print the results.
    for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
      printf("const Inputs kOutputs_%s_%s[] = {\n",
             mnemonic,
             kTests[i].identifier);
      for (size_t j = 0; j < results[i]->output_size; j++) {
        printf("  { ");
        printf("0x%08" PRIx32, results[i]->outputs[j].apsr);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rd);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rn);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rs);
        printf(" },\n");
      }
      printf("};\n");
    }
    printf("const TestResult kReference%s[] = {\n", mnemonic);
    for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
      printf("  {\n");
      printf("    ARRAY_SIZE(kOutputs_%s_%s),\n",
             mnemonic,
             kTests[i].identifier);
      printf("    kOutputs_%s_%s,\n", mnemonic, kTests[i].identifier);
      printf("  },\n");
    }
    printf("};\n");
  } else if (kCheckSimulatorTestResults) {
    // Check the results.
    unsigned total_error_count = 0;
    for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
      bool instruction_has_errors = false;
      for (size_t j = 0; j < kTests[i].input_size; j++) {
        uint32_t apsr = results[i]->outputs[j].apsr;
        uint32_t rd = results[i]->outputs[j].rd;
        uint32_t rn = results[i]->outputs[j].rn;
        uint32_t rs = results[i]->outputs[j].rs;
        uint32_t apsr_input = kTests[i].inputs[j].apsr;
        uint32_t rd_input = kTests[i].inputs[j].rd;
        uint32_t rn_input = kTests[i].inputs[j].rn;
        uint32_t rs_input = kTests[i].inputs[j].rs;
        uint32_t apsr_ref = reference[i].outputs[j].apsr;
        uint32_t rd_ref = reference[i].outputs[j].rd;
        uint32_t rn_ref = reference[i].outputs[j].rn;
        uint32_t rs_ref = reference[i].outputs[j].rs;

        if (((apsr != apsr_ref) || (rd != rd_ref) || (rn != rn_ref) ||
             (rs != rs_ref)) &&
            (++total_error_count <= kErrorReportLimit)) {
          // Print the instruction once even if it triggered multiple failures.
          if (!instruction_has_errors) {
            printf("Error(s) when testing \"%s %s\":\n",
                   mnemonic,
                   kTests[i].operands_description);
            instruction_has_errors = true;
          }
          // Print subsequent errors.
          printf("  Input:    ");
          printf("0x%08" PRIx32, apsr_input);
          printf(", ");
          printf("0x%08" PRIx32, rd_input);
          printf(", ");
          printf("0x%08" PRIx32, rn_input);
          printf(", ");
          printf("0x%08" PRIx32, rs_input);
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, apsr_ref);
          printf(", ");
          printf("0x%08" PRIx32, rd_ref);
          printf(", ");
          printf("0x%08" PRIx32, rn_ref);
          printf(", ");
          printf("0x%08" PRIx32, rs_ref);
          printf("\n");
          printf("  Found:    ");
          printf("0x%08" PRIx32, apsr);
          printf(", ");
          printf("0x%08" PRIx32, rd);
          printf(", ");
          printf("0x%08" PRIx32, rn);
          printf(", ");
          printf("0x%08" PRIx32, rs);
          printf("\n\n");
        }
      }
    }

    if (total_error_count > kErrorReportLimit) {
      printf("%u other errors follow.\n",
             total_error_count - kErrorReportLimit);
    }
    VIXL_CHECK(total_error_count == 0);
  } else {
    VIXL_WARNING("Assembled the code, but did not run anything.\n");
  }

  for (size_t i = 0; i < ARRAY_SIZE(kTests); i++) {
    delete[] results[i]->outputs;
    delete results[i];
    delete[] scratch_memory_buffers[i];
  }
}

// Instantiate tests for each instruction in the list.
// TODO: Remove this limitation by having a sandboxing mechanism.
#if defined(VIXL_HOST_POINTER_32)
#define TEST(mnemonic)                                                      \
  void Test_##mnemonic() {                                                  \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic); \
  }                                                                         \
  Test test_##mnemonic(                                                     \
      "AARCH32_SIMULATOR_COND_RD_OPERAND_RN_SHIFT_RS_" #mnemonic "_T32",    \
      &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                   \
  void Test_##mnemonic() {                                               \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");          \
    USE(TestHelper);                                                     \
  }                                                                      \
  Test test_##mnemonic(                                                  \
      "AARCH32_SIMULATOR_COND_RD_OPERAND_RN_SHIFT_RS_" #mnemonic "_T32", \
      &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
