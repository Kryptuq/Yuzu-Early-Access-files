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
  M(Qadd)                      \
  M(Qdadd)                     \
  M(Qdsub)                     \
  M(Qsub)


// The following definitions are defined again in each generated test, therefore
// we need to place them in an anomymous namespace. It expresses that they are
// local to this file only, and the compiler is not allowed to share these types
// across test files during template instantiation. Specifically, `Operands` and
// `Inputs` have various layouts across generated tests so they absolutely
// cannot be shared.

#ifdef VIXL_INCLUDE_TARGET_A32
namespace {

// Values to be passed to the assembler to produce the instruction under test.
struct Operands {
  Condition cond;
  Register rd;
  Register rn;
  Register rm;
};

// Input data to feed to the instruction.
struct Inputs {
  uint32_t apsr;
  uint32_t qbit;
  uint32_t ge;
  uint32_t rd;
  uint32_t rn;
  uint32_t rm;
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

static const Inputs kQOutput[] =
    {{NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff80, 0x0000007f},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000001, 0xffff8001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007f, 0x00007fff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8001, 0x80000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff81, 0x00000002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007f, 0x00000002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000000, 0x00000002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8003, 0x7fffffff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000002, 0x0000007d},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000000, 0x55555555},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x55555555, 0x00000020},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff80, 0xffffffff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8003, 0xcccccccc},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000000, 0x80000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7ffffffd, 0x00000001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000000, 0x00007ffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007e, 0xffffff80},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007d, 0x00007fff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff81, 0xffffff80},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000000, 0x7ffffffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffff, 0x55555555},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8000, 0xffffffe0},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0x00000020},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007f, 0x0000007d},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffe0, 0x00007ffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffe, 0xffffff83},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xfffffffe, 0x0000007d},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffe0, 0x00000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007ffe, 0xffff8002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffe0, 0xffffff82},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffe0, 0x7fffffff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffd, 0x00000002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007f, 0xfffffffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007fff, 0xaaaaaaaa},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff81, 0x80000000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xfffffffe, 0x0000007f},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007d, 0xffff8002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xcccccccc, 0x80000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8000, 0x00007ffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff83, 0xffffffff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000000, 0xfffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8000, 0xfffffffe},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff83, 0xffffff83},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007ffe, 0xfffffffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007fff, 0x00000002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff81, 0x00000001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000001, 0x0000007e},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xfffffffe, 0x7ffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7fffffff, 0x00000020},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff82, 0x00000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffe, 0x0000007e},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007f, 0xffff8000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007d, 0x7ffffffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffe, 0x0000007e},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0xfffffffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000000, 0x33333333},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x33333333, 0x00007fff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7fffffff, 0xffffff83},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffff, 0xffff8000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7ffffffd, 0xcccccccc},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000001, 0xffffffe0},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0x00007ffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xaaaaaaaa, 0x00000020},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007e, 0xffff8002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007f, 0x7ffffffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000000, 0x80000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000000, 0x0000007f},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff82, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffff, 0x00007fff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xfffffffd, 0x00000002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff81, 0x80000000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7fffffff, 0x33333333},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xcccccccc, 0x00007ffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000001, 0xcccccccc},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xaaaaaaaa, 0x7fffffff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000000, 0x00007ffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xcccccccc, 0x80000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007f, 0x00007fff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffff, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffe, 0x33333333},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8000, 0x00000002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffff, 0xffff8001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff82, 0xffffff82},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000020, 0xffff8002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x33333333, 0x00007ffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffe0, 0x0000007d},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffff, 0xaaaaaaaa},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x33333333, 0x33333333},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000002, 0xffff8000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x33333333, 0xffffff83},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007fff, 0x7ffffffe},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff82, 0xffffff81},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000000, 0x0000007f},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff80, 0x0000007d},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8001, 0xffff8000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000000, 0x00000000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000002, 0x80000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff80, 0x00000020},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffe, 0x00000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff82, 0xaaaaaaaa},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000001, 0x33333333},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xcccccccc, 0x00000020},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffe, 0xffffffe0},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffd, 0x00000002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffe0, 0xffff8000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8000, 0x00000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff80, 0x7fffffff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffff, 0x7ffffffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffd, 0x00007ffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000020, 0x00007fff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff83, 0xfffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7ffffffd, 0x55555555},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007d, 0x0000007e},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff81, 0xcccccccc},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007d, 0xffffff81},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffffe0, 0xffffff80},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007ffd, 0x00000000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000001, 0xcccccccc},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000020, 0xaaaaaaaa},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffd, 0x00000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffd, 0xffffff82},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x33333333, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff81, 0xffff8003},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0xffff8000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000002, 0xffff8001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffe, 0xffffff80},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8000, 0x0000007f},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8003, 0xffffffe0},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x55555555, 0x0000007f},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xfffffffd, 0xffffffe0},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff80, 0x00000001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000020, 0xffffff83},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7fffffff, 0x00007ffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x55555555, 0x80000000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffe, 0xffff8001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xfffffffd, 0x00000002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7ffffffe, 0x0000007e},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff83, 0xffff8001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xcccccccc, 0xffffff81},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8002, 0xfffffffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8002, 0x0000007f},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffd, 0xcccccccc},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000002, 0x00000002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7fffffff, 0x7ffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0xffff8001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000000, 0x00007ffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8000, 0x00007fff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000001, 0x55555555},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007fff, 0xffff8001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0x55555555},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000000, 0xffff8003},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7ffffffe, 0xffffff81},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x55555555, 0x7ffffffe},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000001, 0x00007fff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000002, 0x7ffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff82, 0xfffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff82, 0x0000007f},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007fff, 0x00007ffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xcccccccc, 0xfffffffe},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007fff, 0x80000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8003, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xcccccccc, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007d, 0xffffff81},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffd, 0x7ffffffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000001, 0xffff8002},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007e, 0x00000001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007fff, 0xffffffe0},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff81, 0x7fffffff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffe0, 0xffffffe0},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007f, 0xffff8000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x80000000, 0x00007ffd},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x0000007d, 0x00000001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xcccccccc, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffffe0, 0x0000007f},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x33333333, 0x00000000},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x7fffffff, 0x80000000},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffe, 0x00007ffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff80, 0x7ffffffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000020, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8002, 0x00007fff},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007ffe, 0xffff8003},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffe, 0xffffff82},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffffff81, 0xcccccccc},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00007fff, 0x00007ffd},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x7ffffffd, 0x00000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00007ffd, 0x80000001},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007e, 0x33333333},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x33333333, 0x7fffffff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007d, 0x00000002},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffffff81, 0xffffff81},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8002, 0x80000001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xcccccccc, 0xffffffff},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x0000007e, 0xffff8001},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xfffffffd, 0xffffff81},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x00000002, 0x0000007f},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xcccccccc, 0xaaaaaaaa},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0x00000000, 0x0000007f},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0x80000001, 0xffffff80},
     {NoFlag, NoFlag, NoFlag, 0xabababab, 0xffff8000, 0x00000020},
     {NoFlag, QFlag, NoFlag, 0xabababab, 0xffff8000, 0x00000020}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{al, r0, r9, r10},
                                "al r0 r9 r10",
                                "QOutput_al_r0_r9_r10",
                                ARRAY_SIZE(kQOutput),
                                kQOutput}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-rd-rn-rm-q-qadd-a32.h"
#include "aarch32/traces/simulator-cond-rd-rn-rm-q-qdadd-a32.h"
#include "aarch32/traces/simulator-cond-rd-rn-rm-q-qdsub-a32.h"
#include "aarch32/traces/simulator-cond-rd-rn-rm-q-qsub-a32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
                                   Register rn,
                                   Register rm);

void TestHelper(Fn instruction,
                const char* mnemonic,
                const TestResult reference[]) {
  SETUP();
  masm.UseA32();
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
    Register rm = kTests[i].operands.rm;
    scratch_registers.Exclude(rd);
    scratch_registers.Exclude(rn);
    scratch_registers.Exclude(rm);

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
    {
      UseScratchRegisterScope temp_registers(&masm);
      Register q_bit = temp_registers.Acquire();
      Register saved_nzcv_bits = temp_registers.Acquire();
      // Save the `NZCV` flags.
      __ Mrs(saved_nzcv_bits, APSR);
      __ And(saved_nzcv_bits, saved_nzcv_bits, NZCVFlag);
      // Set the `NZCV` and `Q` flags together.
      __ Ldr(q_bit, MemOperand(input_ptr, offsetof(Inputs, qbit)));
      __ Orr(q_bit, q_bit, saved_nzcv_bits);
      __ Msr(APSR_nzcvq, q_bit);
    }
    {
      UseScratchRegisterScope temp_registers(&masm);
      Register ge_bits = temp_registers.Acquire();
      __ Ldr(ge_bits, MemOperand(input_ptr, offsetof(Inputs, ge)));
      __ Msr(APSR_g, ge_bits);
    }
    __ Ldr(rd, MemOperand(input_ptr, offsetof(Inputs, rd)));
    __ Ldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));
    __ Ldr(rm, MemOperand(input_ptr, offsetof(Inputs, rm)));

    (masm.*instruction)(cond, rd, rn, rm);

    {
      UseScratchRegisterScope temp_registers(&masm);
      Register nzcv_bits = temp_registers.Acquire();
      __ Mrs(nzcv_bits, APSR);
      // Only record the NZCV bits.
      __ And(nzcv_bits, nzcv_bits, NZCVFlag);
      __ Str(nzcv_bits, MemOperand(result_ptr, offsetof(Inputs, apsr)));
    }
    {
      UseScratchRegisterScope temp_registers(&masm);
      Register q_bit = temp_registers.Acquire();
      __ Mrs(q_bit, APSR);
      // Only record the Q bit.
      __ And(q_bit, q_bit, QFlag);
      __ Str(q_bit, MemOperand(result_ptr, offsetof(Inputs, qbit)));
    }
    {
      UseScratchRegisterScope temp_registers(&masm);
      Register ge_bits = temp_registers.Acquire();
      __ Mrs(ge_bits, APSR);
      // Only record the GE bits.
      __ And(ge_bits, ge_bits, GEFlags);
      __ Str(ge_bits, MemOperand(result_ptr, offsetof(Inputs, ge)));
    }
    __ Str(rd, MemOperand(result_ptr, offsetof(Inputs, rd)));
    __ Str(rn, MemOperand(result_ptr, offsetof(Inputs, rn)));
    __ Str(rm, MemOperand(result_ptr, offsetof(Inputs, rm)));

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
        printf("0x%08" PRIx32, results[i]->outputs[j].qbit);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].ge);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rd);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rn);
        printf(", ");
        printf("0x%08" PRIx32, results[i]->outputs[j].rm);
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
        uint32_t qbit = results[i]->outputs[j].qbit;
        uint32_t ge = results[i]->outputs[j].ge;
        uint32_t rd = results[i]->outputs[j].rd;
        uint32_t rn = results[i]->outputs[j].rn;
        uint32_t rm = results[i]->outputs[j].rm;
        uint32_t apsr_input = kTests[i].inputs[j].apsr;
        uint32_t qbit_input = kTests[i].inputs[j].qbit;
        uint32_t ge_input = kTests[i].inputs[j].ge;
        uint32_t rd_input = kTests[i].inputs[j].rd;
        uint32_t rn_input = kTests[i].inputs[j].rn;
        uint32_t rm_input = kTests[i].inputs[j].rm;
        uint32_t apsr_ref = reference[i].outputs[j].apsr;
        uint32_t qbit_ref = reference[i].outputs[j].qbit;
        uint32_t ge_ref = reference[i].outputs[j].ge;
        uint32_t rd_ref = reference[i].outputs[j].rd;
        uint32_t rn_ref = reference[i].outputs[j].rn;
        uint32_t rm_ref = reference[i].outputs[j].rm;

        if (((apsr != apsr_ref) || (qbit != qbit_ref) || (ge != ge_ref) ||
             (rd != rd_ref) || (rn != rn_ref) || (rm != rm_ref)) &&
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
          printf("0x%08" PRIx32, qbit_input);
          printf(", ");
          printf("0x%08" PRIx32, ge_input);
          printf(", ");
          printf("0x%08" PRIx32, rd_input);
          printf(", ");
          printf("0x%08" PRIx32, rn_input);
          printf(", ");
          printf("0x%08" PRIx32, rm_input);
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, apsr_ref);
          printf(", ");
          printf("0x%08" PRIx32, qbit_ref);
          printf(", ");
          printf("0x%08" PRIx32, ge_ref);
          printf(", ");
          printf("0x%08" PRIx32, rd_ref);
          printf(", ");
          printf("0x%08" PRIx32, rn_ref);
          printf(", ");
          printf("0x%08" PRIx32, rm_ref);
          printf("\n");
          printf("  Found:    ");
          printf("0x%08" PRIx32, apsr);
          printf(", ");
          printf("0x%08" PRIx32, qbit);
          printf(", ");
          printf("0x%08" PRIx32, ge);
          printf(", ");
          printf("0x%08" PRIx32, rd);
          printf(", ");
          printf("0x%08" PRIx32, rn);
          printf(", ");
          printf("0x%08" PRIx32, rm);
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
#define TEST(mnemonic)                                                        \
  void Test_##mnemonic() {                                                    \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic);   \
  }                                                                           \
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RD_RN_RM_Q_" #mnemonic "_A32", \
                       &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                        \
  void Test_##mnemonic() {                                                    \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");               \
    USE(TestHelper);                                                          \
  }                                                                           \
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RD_RN_RM_Q_" #mnemonic "_A32", \
                       &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
