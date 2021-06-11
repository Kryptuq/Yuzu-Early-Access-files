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
  M(Adc)                       \
  M(Adcs)                      \
  M(Add)                       \
  M(Adds)                      \
  M(And)                       \
  M(Ands)                      \
  M(Bic)                       \
  M(Bics)                      \
  M(Eor)                       \
  M(Eors)                      \
  M(Orn)                       \
  M(Orns)                      \
  M(Orr)                       \
  M(Orrs)                      \
  M(Rsb)                       \
  M(Rsbs)                      \
  M(Sbc)                       \
  M(Sbcs)                      \
  M(Sub)                       \
  M(Subs)                      \
  M(Sxtab)                     \
  M(Sxtab16)                   \
  M(Sxtah)                     \
  M(Uxtab)                     \
  M(Uxtab16)                   \
  M(Uxtah)                     \
  M(Asr)                       \
  M(Asrs)                      \
  M(Lsl)                       \
  M(Lsls)                      \
  M(Lsr)                       \
  M(Lsrs)                      \
  M(Ror)                       \
  M(Rors)


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
  Register rm;
};

// Input data to feed to the instruction.
struct Inputs {
  uint32_t apsr;
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

static const Inputs kCondition[] =
    {{NFlag, 0xabababab, 0xabababab, 0xabababab},
     {ZFlag, 0xabababab, 0xabababab, 0xabababab},
     {CFlag, 0xabababab, 0xabababab, 0xabababab},
     {VFlag, 0xabababab, 0xabababab, 0xabababab},
     {NZFlag, 0xabababab, 0xabababab, 0xabababab},
     {NCFlag, 0xabababab, 0xabababab, 0xabababab},
     {NVFlag, 0xabababab, 0xabababab, 0xabababab},
     {ZCFlag, 0xabababab, 0xabababab, 0xabababab},
     {ZVFlag, 0xabababab, 0xabababab, 0xabababab},
     {CVFlag, 0xabababab, 0xabababab, 0xabababab},
     {NZCFlag, 0xabababab, 0xabababab, 0xabababab},
     {NZVFlag, 0xabababab, 0xabababab, 0xabababab},
     {NCVFlag, 0xabababab, 0xabababab, 0xabababab},
     {ZCVFlag, 0xabababab, 0xabababab, 0xabababab},
     {NZCVFlag, 0xabababab, 0xabababab, 0xabababab}};

static const Inputs kRdIsRn[] = {{ZCFlag, 0x00000000, 0x00000000, 0xffffff80},
                                 {NZCFlag, 0x00007ffe, 0x00007ffe, 0x7ffffffe},
                                 {NVFlag, 0x00000001, 0x00000001, 0x0000007d},
                                 {NZFlag, 0xffffff80, 0xffffff80, 0xaaaaaaaa},
                                 {NZFlag, 0x55555555, 0x55555555, 0x00000020},
                                 {NCVFlag, 0x00000001, 0x00000001, 0x00000002},
                                 {NCVFlag, 0xffffffff, 0xffffffff, 0x55555555},
                                 {NVFlag, 0x80000001, 0x80000001, 0x00007ffd},
                                 {NZCFlag, 0x00007fff, 0x00007fff, 0x80000000},
                                 {ZFlag, 0x00000001, 0x00000001, 0x00000001},
                                 {CVFlag, 0xffff8001, 0xffff8001, 0x00000002},
                                 {NCVFlag, 0xffffffff, 0xffffffff, 0xffffff82},
                                 {ZCFlag, 0x00000001, 0x00000001, 0x00000001},
                                 {ZCFlag, 0xffffff81, 0xffffff81, 0x80000000},
                                 {NZCVFlag, 0x00007ffe, 0x00007ffe, 0xfffffffe},
                                 {NZFlag, 0x00000002, 0x00000002, 0x00007fff},
                                 {VFlag, 0x00007fff, 0x00007fff, 0x7ffffffe},
                                 {NZCVFlag, 0x00000002, 0x00000002, 0xfffffffd},
                                 {CFlag, 0xffffff80, 0xffffff80, 0xffff8003},
                                 {NZCVFlag, 0xfffffffd, 0xfffffffd, 0x00007ffe},
                                 {NCVFlag, 0xffffff83, 0xffffff83, 0xffffffff},
                                 {NFlag, 0x0000007f, 0x0000007f, 0x00000002},
                                 {NZFlag, 0xffff8002, 0xffff8002, 0x00000001},
                                 {NZCVFlag, 0x00007fff, 0x00007fff, 0x00007ffd},
                                 {NZCVFlag, 0x55555555, 0x55555555, 0xffffff82},
                                 {NCFlag, 0xffff8003, 0xffff8003, 0x7ffffffe},
                                 {NCFlag, 0xffffff80, 0xffffff80, 0x7fffffff},
                                 {ZVFlag, 0xfffffffd, 0xfffffffd, 0xffffffe0},
                                 {NVFlag, 0x55555555, 0x55555555, 0x80000001},
                                 {CVFlag, 0xffffff82, 0xffffff82, 0x00007ffe},
                                 {NVFlag, 0xffffffff, 0xffffffff, 0x0000007f},
                                 {ZFlag, 0x00007fff, 0x00007fff, 0x55555555},
                                 {VFlag, 0x00000001, 0x00000001, 0x7ffffffe},
                                 {NFlag, 0xffff8000, 0xffff8000, 0xffff8003},
                                 {ZVFlag, 0x00007ffd, 0x00007ffd, 0xffffff80},
                                 {NZFlag, 0xffffff83, 0xffffff83, 0xfffffffe},
                                 {NCVFlag, 0xffffffe0, 0xffffffe0, 0x00000000},
                                 {ZCFlag, 0x0000007d, 0x0000007d, 0x00000000},
                                 {ZFlag, 0x00000020, 0x00000020, 0xffff8002},
                                 {ZFlag, 0xfffffffe, 0xfffffffe, 0x00000001},
                                 {CFlag, 0xffffffff, 0xffffffff, 0x0000007f},
                                 {NZFlag, 0xcccccccc, 0xcccccccc, 0x00000001},
                                 {ZFlag, 0x00000001, 0x00000001, 0x7ffffffe},
                                 {NFlag, 0xffffffe0, 0xffffffe0, 0xffffffff},
                                 {NCFlag, 0xffffffff, 0xffffffff, 0x00000002},
                                 {NVFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0xffff8000},
                                 {NZFlag, 0xffffffe0, 0xffffffe0, 0xffffffe0},
                                 {NZFlag, 0x00000000, 0x00000000, 0x00007fff},
                                 {ZFlag, 0x33333333, 0x33333333, 0xffff8000},
                                 {VFlag, 0x55555555, 0x55555555, 0x00000001},
                                 {NCVFlag, 0x00000001, 0x00000001, 0x00000020},
                                 {NZCFlag, 0xfffffffd, 0xfffffffd, 0x00000001},
                                 {NFlag, 0xffff8002, 0xffff8002, 0x00000001},
                                 {NZCFlag, 0x0000007d, 0x0000007d, 0xffffff81},
                                 {NZFlag, 0x55555555, 0x55555555, 0x00000000},
                                 {NZVFlag, 0xffffffff, 0xffffffff, 0xfffffffe},
                                 {NZFlag, 0x00000000, 0x00000000, 0xfffffffd},
                                 {NCVFlag, 0x00007ffd, 0x00007ffd, 0xffff8001},
                                 {NCFlag, 0x0000007d, 0x0000007d, 0x00007ffe},
                                 {NFlag, 0xffff8003, 0xffff8003, 0x00000020},
                                 {NZCVFlag, 0x0000007e, 0x0000007e, 0x33333333},
                                 {ZCFlag, 0x00000000, 0x00000000, 0x80000000},
                                 {VFlag, 0x80000001, 0x80000001, 0x80000000},
                                 {NZVFlag, 0x00000002, 0x00000002, 0x80000001},
                                 {NZFlag, 0x0000007f, 0x0000007f, 0xffffff80},
                                 {NCFlag, 0xcccccccc, 0xcccccccc, 0x00007ffe},
                                 {NZVFlag, 0x7ffffffd, 0x7ffffffd, 0xffffff80},
                                 {CVFlag, 0x0000007d, 0x0000007d, 0xffff8003},
                                 {ZFlag, 0x7ffffffd, 0x7ffffffd, 0x80000001},
                                 {NZCFlag, 0xffff8003, 0xffff8003, 0xffffff81},
                                 {CFlag, 0x00000000, 0x00000000, 0x80000001},
                                 {VFlag, 0x80000001, 0x80000001, 0x00000000},
                                 {NZVFlag, 0xffffff82, 0xffffff82, 0x00007ffd},
                                 {NZCFlag, 0xffffff80, 0xffffff80, 0xfffffffd},
                                 {ZVFlag, 0x7ffffffe, 0x7ffffffe, 0xfffffffd},
                                 {NCFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0xffff8003},
                                 {CFlag, 0xffffff81, 0xffffff81, 0x00007ffd},
                                 {NZVFlag, 0xffff8002, 0xffff8002, 0x80000000},
                                 {NZCVFlag, 0x7ffffffd, 0x7ffffffd, 0x0000007e},
                                 {CVFlag, 0xffffffff, 0xffffffff, 0x7ffffffd},
                                 {NCFlag, 0x00000001, 0x00000001, 0x00007fff},
                                 {NZVFlag, 0xffffff83, 0xffffff83, 0x00000020},
                                 {NZFlag, 0x00000001, 0x00000001, 0x00007ffd},
                                 {NZVFlag, 0xcccccccc, 0xcccccccc, 0x00000020},
                                 {NCFlag, 0xffffff80, 0xffffff80, 0xaaaaaaaa},
                                 {VFlag, 0x7ffffffe, 0x7ffffffe, 0x80000001},
                                 {NZCVFlag, 0x00000002, 0x00000002, 0x33333333},
                                 {NCFlag, 0xffffff81, 0xffffff81, 0xffff8000},
                                 {ZVFlag, 0xffffffe0, 0xffffffe0, 0x55555555},
                                 {NCVFlag, 0xfffffffd, 0xfffffffd, 0x7ffffffe},
                                 {NVFlag, 0xffffff83, 0xffffff83, 0x0000007f},
                                 {ZVFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0xfffffffd},
                                 {NFlag, 0xffff8001, 0xffff8001, 0x00000000},
                                 {NCFlag, 0x0000007d, 0x0000007d, 0x00007fff},
                                 {NVFlag, 0x0000007f, 0x0000007f, 0x80000000},
                                 {ZCFlag, 0xffffff80, 0xffffff80, 0x7ffffffe},
                                 {ZFlag, 0xffff8002, 0xffff8002, 0xffffff80},
                                 {NVFlag, 0xffff8001, 0xffff8001, 0x33333333},
                                 {NCVFlag, 0xffff8002, 0xffff8002, 0x00007ffd},
                                 {NCFlag, 0x00000020, 0x00000020, 0xffff8002},
                                 {NCVFlag, 0x33333333, 0x33333333, 0x0000007d},
                                 {VFlag, 0x55555555, 0x55555555, 0x00007ffd},
                                 {NZFlag, 0xffff8000, 0xffff8000, 0x33333333},
                                 {NFlag, 0xffffffff, 0xffffffff, 0x80000001},
                                 {ZFlag, 0x7ffffffe, 0x7ffffffe, 0x00000020},
                                 {ZVFlag, 0x0000007e, 0x0000007e, 0x0000007f},
                                 {ZCFlag, 0xffffff80, 0xffffff80, 0xffffffff},
                                 {ZCVFlag, 0x00000020, 0x00000020, 0xaaaaaaaa},
                                 {NZCVFlag, 0x00007ffd, 0x00007ffd, 0x00000000},
                                 {NZVFlag, 0xffff8002, 0xffff8002, 0x0000007f},
                                 {NZFlag, 0x7ffffffd, 0x7ffffffd, 0xffffffe0},
                                 {NZCVFlag, 0x00000002, 0x00000002, 0xffff8001},
                                 {ZCFlag, 0xffffff81, 0xffffff81, 0x33333333},
                                 {ZCVFlag, 0x00000001, 0x00000001, 0x7ffffffd},
                                 {ZCVFlag, 0x0000007d, 0x0000007d, 0xaaaaaaaa},
                                 {NFlag, 0xffffff83, 0xffffff83, 0x33333333},
                                 {ZFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0x00000000},
                                 {ZVFlag, 0xffff8001, 0xffff8001, 0x00007ffe},
                                 {NZFlag, 0x55555555, 0x55555555, 0x7fffffff},
                                 {ZCVFlag, 0x00007ffd, 0x00007ffd, 0xffffffff},
                                 {VFlag, 0x00007ffe, 0x00007ffe, 0xffff8002},
                                 {CVFlag, 0x0000007e, 0x0000007e, 0xffff8002},
                                 {CFlag, 0xffffff82, 0xffffff82, 0xffff8002},
                                 {NCFlag, 0x00007ffd, 0x00007ffd, 0x00000000},
                                 {NCFlag, 0x00007ffe, 0x00007ffe, 0x7fffffff},
                                 {ZCFlag, 0xfffffffd, 0xfffffffd, 0x0000007d},
                                 {NZVFlag, 0x0000007e, 0x0000007e, 0x33333333},
                                 {NZVFlag, 0x55555555, 0x55555555, 0x00007fff},
                                 {ZVFlag, 0xffff8002, 0xffff8002, 0xaaaaaaaa},
                                 {NZFlag, 0xfffffffe, 0xfffffffe, 0x7fffffff},
                                 {ZCFlag, 0x80000001, 0x80000001, 0x80000001},
                                 {ZCFlag, 0x7fffffff, 0x7fffffff, 0x80000001},
                                 {NZCVFlag, 0xffff8002, 0xffff8002, 0xffffff83},
                                 {NZCVFlag, 0x00000000, 0x00000000, 0x80000000},
                                 {ZCVFlag, 0x55555555, 0x55555555, 0xfffffffe},
                                 {NZVFlag, 0x00007ffe, 0x00007ffe, 0x00007fff},
                                 {ZCFlag, 0x7ffffffe, 0x7ffffffe, 0xffffff81},
                                 {ZFlag, 0x00000020, 0x00000020, 0xfffffffd},
                                 {NZCFlag, 0x00007fff, 0x00007fff, 0x7fffffff},
                                 {NZCVFlag, 0x7ffffffd, 0x7ffffffd, 0xaaaaaaaa},
                                 {NCFlag, 0xffffff82, 0xffffff82, 0x00000001},
                                 {CFlag, 0xffffff81, 0xffffff81, 0xfffffffd},
                                 {NCVFlag, 0xffffff82, 0xffffff82, 0xffff8001},
                                 {NZCFlag, 0xcccccccc, 0xcccccccc, 0xffffff82},
                                 {NCFlag, 0x55555555, 0x55555555, 0xffffffff},
                                 {CFlag, 0x7ffffffe, 0x7ffffffe, 0x00000001},
                                 {ZFlag, 0x00007fff, 0x00007fff, 0x0000007e},
                                 {NVFlag, 0xfffffffe, 0xfffffffe, 0xffff8002},
                                 {NVFlag, 0x0000007d, 0x0000007d, 0xfffffffd},
                                 {CFlag, 0x55555555, 0x55555555, 0xffffffe0},
                                 {ZCFlag, 0xcccccccc, 0xcccccccc, 0x0000007d},
                                 {NCVFlag, 0xffff8001, 0xffff8001, 0x0000007f},
                                 {NFlag, 0xffff8001, 0xffff8001, 0x0000007f},
                                 {CFlag, 0x80000001, 0x80000001, 0xffff8003},
                                 {NCVFlag, 0x00000000, 0x00000000, 0x0000007f},
                                 {NZCVFlag, 0xffff8000, 0xffff8000, 0xfffffffd},
                                 {NCFlag, 0x0000007d, 0x0000007d, 0x55555555},
                                 {NZFlag, 0xffffff80, 0xffffff80, 0xffffff83},
                                 {NCFlag, 0x7ffffffe, 0x7ffffffe, 0xffffff80},
                                 {NVFlag, 0xffff8002, 0xffff8002, 0xcccccccc},
                                 {NZFlag, 0xfffffffd, 0xfffffffd, 0x00007fff},
                                 {NZCVFlag, 0x0000007f, 0x0000007f, 0xffffffff},
                                 {NZCFlag, 0x00000020, 0x00000020, 0x55555555},
                                 {NVFlag, 0x7ffffffe, 0x7ffffffe, 0x00007ffe},
                                 {NZCFlag, 0x00007ffd, 0x00007ffd, 0xcccccccc},
                                 {CVFlag, 0x00000001, 0x00000001, 0xffff8003},
                                 {NCFlag, 0x55555555, 0x55555555, 0xffffff80},
                                 {NCVFlag, 0x00007ffe, 0x00007ffe, 0xffffff83},
                                 {ZCFlag, 0xffffffff, 0xffffffff, 0x00000002},
                                 {NZFlag, 0xffffff81, 0xffffff81, 0xffffff83},
                                 {CVFlag, 0x00007fff, 0x00007fff, 0xffffffe0},
                                 {NZFlag, 0xfffffffd, 0xfffffffd, 0x00000002},
                                 {NCFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0x80000000},
                                 {NZCVFlag, 0x00007fff, 0x00007fff, 0xffffff82},
                                 {NFlag, 0xffffffff, 0xffffffff, 0x00007ffe},
                                 {CVFlag, 0x00007ffd, 0x00007ffd, 0x00000001},
                                 {NZCFlag, 0xffff8003, 0xffff8003, 0xffffff82},
                                 {CVFlag, 0x00000020, 0x00000020, 0x00007ffd},
                                 {VFlag, 0x00000000, 0x00000000, 0x00007ffd},
                                 {CVFlag, 0x80000000, 0x80000000, 0xffffff82},
                                 {ZCVFlag, 0xffffffe0, 0xffffffe0, 0x00000001},
                                 {NZFlag, 0x7ffffffe, 0x7ffffffe, 0x55555555},
                                 {ZCVFlag, 0x7fffffff, 0x7fffffff, 0x55555555},
                                 {CVFlag, 0x00000001, 0x00000001, 0x80000001},
                                 {NZFlag, 0xffff8000, 0xffff8000, 0xffff8001},
                                 {NZFlag, 0x7ffffffd, 0x7ffffffd, 0x00007ffe},
                                 {ZCVFlag, 0xffffff82, 0xffffff82, 0x7ffffffd},
                                 {CFlag, 0xffffff82, 0xffffff82, 0x00007fff},
                                 {NCVFlag, 0xcccccccc, 0xcccccccc, 0x80000000},
                                 {NCVFlag, 0x80000000, 0x80000000, 0xaaaaaaaa},
                                 {NZFlag, 0xffff8002, 0xffff8002, 0xffffff80},
                                 {NCFlag, 0x7fffffff, 0x7fffffff, 0x00000001},
                                 {ZVFlag, 0xfffffffd, 0xfffffffd, 0xffffff83},
                                 {NZCFlag, 0x00007ffe, 0x00007ffe, 0xaaaaaaaa},
                                 {NFlag, 0xffffff83, 0xffffff83, 0xffffff81},
                                 {NCFlag, 0xffffff83, 0xffffff83, 0x0000007f},
                                 {ZFlag, 0xffffffe0, 0xffffffe0, 0x0000007f},
                                 {ZCFlag, 0x33333333, 0x33333333, 0x00007ffe},
                                 {ZCFlag, 0x7ffffffe, 0x7ffffffe, 0xffff8002},
                                 {NCFlag, 0x0000007d, 0x0000007d, 0x00007ffd}};

static const Inputs kRdIsRm[] = {{NZVFlag, 0xffff8002, 0x00000020, 0xffff8002},
                                 {NVFlag, 0x00000002, 0xfffffffd, 0x00000002},
                                 {ZCVFlag, 0x00000000, 0x00007ffd, 0x00000000},
                                 {NCVFlag, 0x0000007f, 0xffff8000, 0x0000007f},
                                 {NFlag, 0x00007fff, 0xffffff83, 0x00007fff},
                                 {CVFlag, 0xffffff80, 0x7ffffffd, 0xffffff80},
                                 {ZVFlag, 0xffffff82, 0x0000007d, 0xffffff82},
                                 {ZFlag, 0xfffffffd, 0x80000001, 0xfffffffd},
                                 {ZVFlag, 0xffffff82, 0x7fffffff, 0xffffff82},
                                 {CFlag, 0xffff8000, 0x00007ffe, 0xffff8000},
                                 {NZFlag, 0x0000007e, 0xffffff80, 0x0000007e},
                                 {NVFlag, 0xffffff83, 0xffff8000, 0xffffff83},
                                 {NVFlag, 0xcccccccc, 0x80000001, 0xcccccccc},
                                 {CFlag, 0xffffff81, 0xffff8003, 0xffffff81},
                                 {NZCVFlag, 0x0000007e, 0xfffffffe, 0x0000007e},
                                 {VFlag, 0x0000007f, 0xfffffffd, 0x0000007f},
                                 {NVFlag, 0xcccccccc, 0xffff8001, 0xcccccccc},
                                 {CFlag, 0x80000000, 0x00000000, 0x80000000},
                                 {NVFlag, 0x80000000, 0xffff8002, 0x80000000},
                                 {ZVFlag, 0x00000001, 0x7ffffffd, 0x00000001},
                                 {ZVFlag, 0xffff8001, 0x00000020, 0xffff8001},
                                 {CFlag, 0xffffff81, 0x0000007e, 0xffffff81},
                                 {NVFlag, 0xffff8000, 0x00000002, 0xffff8000},
                                 {NCVFlag, 0xffff8000, 0xffff8002, 0xffff8000},
                                 {NZVFlag, 0xcccccccc, 0x00000020, 0xcccccccc},
                                 {ZFlag, 0x7ffffffd, 0xffff8002, 0x7ffffffd},
                                 {NFlag, 0xffffff81, 0x00000020, 0xffffff81},
                                 {NFlag, 0x80000000, 0xfffffffe, 0x80000000},
                                 {NZVFlag, 0xaaaaaaaa, 0xffffff82, 0xaaaaaaaa},
                                 {NZVFlag, 0x0000007f, 0x00007ffe, 0x0000007f},
                                 {NVFlag, 0x80000001, 0x00007ffe, 0x80000001},
                                 {NZVFlag, 0xffff8003, 0x00000020, 0xffff8003},
                                 {NZVFlag, 0xffffff81, 0xfffffffe, 0xffffff81},
                                 {ZVFlag, 0x00000001, 0xffffff83, 0x00000001},
                                 {ZCVFlag, 0x0000007d, 0x7fffffff, 0x0000007d},
                                 {ZFlag, 0x80000001, 0x33333333, 0x80000001},
                                 {NCFlag, 0x7ffffffd, 0xffff8003, 0x7ffffffd},
                                 {NZCFlag, 0xcccccccc, 0xaaaaaaaa, 0xcccccccc},
                                 {NVFlag, 0x7ffffffe, 0xffffffe0, 0x7ffffffe},
                                 {NCFlag, 0xffffff82, 0x7ffffffe, 0xffffff82},
                                 {NZFlag, 0x7fffffff, 0xffffff81, 0x7fffffff},
                                 {CFlag, 0xffffff81, 0xfffffffd, 0xffffff81},
                                 {NZCVFlag, 0x00000020, 0xaaaaaaaa, 0x00000020},
                                 {ZCFlag, 0x80000000, 0xffff8001, 0x80000000},
                                 {NZCVFlag, 0x00000000, 0xffff8000, 0x00000000},
                                 {ZVFlag, 0x00000002, 0x0000007d, 0x00000002},
                                 {ZCVFlag, 0x7fffffff, 0xffff8003, 0x7fffffff},
                                 {NZFlag, 0x80000000, 0xffffffe0, 0x80000000},
                                 {CVFlag, 0xffffff81, 0x33333333, 0xffffff81},
                                 {ZVFlag, 0x55555555, 0x0000007f, 0x55555555},
                                 {NVFlag, 0xffff8000, 0x33333333, 0xffff8000},
                                 {VFlag, 0x00007ffd, 0xffffffff, 0x00007ffd},
                                 {NCVFlag, 0x7ffffffe, 0xffffffff, 0x7ffffffe},
                                 {ZCFlag, 0xffff8000, 0x7ffffffd, 0xffff8000},
                                 {NZCVFlag, 0xfffffffd, 0x33333333, 0xfffffffd},
                                 {NCFlag, 0xffffff81, 0x00007ffe, 0xffffff81},
                                 {CFlag, 0xffff8003, 0x00000020, 0xffff8003},
                                 {NCVFlag, 0xaaaaaaaa, 0x80000001, 0xaaaaaaaa},
                                 {NCFlag, 0xffffff82, 0x7fffffff, 0xffffff82},
                                 {NZVFlag, 0x7ffffffe, 0xffff8001, 0x7ffffffe},
                                 {NZVFlag, 0x00000001, 0x00000000, 0x00000001},
                                 {NZVFlag, 0x00000020, 0x33333333, 0x00000020},
                                 {NCVFlag, 0x00000002, 0x00000002, 0x00000002},
                                 {CFlag, 0x55555555, 0x0000007e, 0x55555555},
                                 {ZFlag, 0x00000000, 0x80000001, 0x00000000},
                                 {NZFlag, 0x33333333, 0xffffff81, 0x33333333},
                                 {CFlag, 0x0000007e, 0x0000007f, 0x0000007e},
                                 {NCFlag, 0xffffff82, 0x0000007f, 0xffffff82},
                                 {CFlag, 0x00007ffe, 0x00007ffe, 0x00007ffe},
                                 {ZFlag, 0x7ffffffe, 0x0000007d, 0x7ffffffe},
                                 {ZCVFlag, 0xaaaaaaaa, 0xffffff82, 0xaaaaaaaa},
                                 {NCVFlag, 0xfffffffd, 0x0000007e, 0xfffffffd},
                                 {NVFlag, 0xcccccccc, 0x7ffffffe, 0xcccccccc},
                                 {NCFlag, 0xffff8001, 0x7ffffffe, 0xffff8001},
                                 {NCFlag, 0x7ffffffe, 0x0000007e, 0x7ffffffe},
                                 {CVFlag, 0x0000007f, 0x0000007f, 0x0000007f},
                                 {NZVFlag, 0x00000020, 0x0000007f, 0x00000020},
                                 {ZCFlag, 0xffffff80, 0x00000001, 0xffffff80},
                                 {ZCVFlag, 0x00000020, 0xffffffff, 0x00000020},
                                 {NFlag, 0xffffffe0, 0x7fffffff, 0xffffffe0},
                                 {NZCFlag, 0x00007fff, 0xffffffff, 0x00007fff},
                                 {CVFlag, 0xffff8003, 0x7ffffffe, 0xffff8003},
                                 {CVFlag, 0x00000020, 0x7fffffff, 0x00000020},
                                 {NFlag, 0xffffff82, 0x80000001, 0xffffff82},
                                 {NFlag, 0xffffff83, 0xffffff81, 0xffffff83},
                                 {NZCVFlag, 0xffffffe0, 0xffffff80, 0xffffffe0},
                                 {ZCVFlag, 0x7fffffff, 0x00007ffe, 0x7fffffff},
                                 {ZCVFlag, 0x0000007d, 0x00000002, 0x0000007d},
                                 {NVFlag, 0xffffff80, 0xcccccccc, 0xffffff80},
                                 {ZCFlag, 0xffffff83, 0xfffffffe, 0xffffff83},
                                 {ZVFlag, 0x7ffffffd, 0x7ffffffe, 0x7ffffffd},
                                 {NZVFlag, 0x00007ffd, 0xffff8001, 0x00007ffd},
                                 {NCVFlag, 0x0000007e, 0x80000000, 0x0000007e},
                                 {NZVFlag, 0x7fffffff, 0xcccccccc, 0x7fffffff},
                                 {VFlag, 0x00007ffd, 0xfffffffe, 0x00007ffd},
                                 {ZFlag, 0xaaaaaaaa, 0xffffff81, 0xaaaaaaaa},
                                 {ZCVFlag, 0x80000001, 0x00000000, 0x80000001},
                                 {VFlag, 0xffffffe0, 0x00000000, 0xffffffe0},
                                 {NZVFlag, 0x0000007f, 0x80000000, 0x0000007f},
                                 {ZFlag, 0x80000001, 0x00007fff, 0x80000001},
                                 {ZCFlag, 0x00007ffd, 0x0000007e, 0x00007ffd},
                                 {NCVFlag, 0x0000007f, 0xffff8003, 0x0000007f},
                                 {NFlag, 0xfffffffd, 0xffff8000, 0xfffffffd},
                                 {NZVFlag, 0xffffff80, 0x80000000, 0xffffff80},
                                 {NFlag, 0x33333333, 0xffffff80, 0x33333333},
                                 {NZFlag, 0x0000007f, 0x7ffffffe, 0x0000007f},
                                 {ZVFlag, 0x00007ffd, 0x00000002, 0x00007ffd},
                                 {ZCFlag, 0x00000020, 0xffff8003, 0x00000020},
                                 {NZFlag, 0x0000007e, 0x00000002, 0x0000007e},
                                 {NCVFlag, 0x00000000, 0xffffff80, 0x00000000},
                                 {NCVFlag, 0xffff8002, 0x00007ffe, 0xffff8002},
                                 {NZCVFlag, 0xffffff80, 0xffff8003, 0xffffff80},
                                 {NZFlag, 0xffff8003, 0x00007ffe, 0xffff8003},
                                 {NFlag, 0x00000000, 0xffff8003, 0x00000000},
                                 {NCFlag, 0x00000020, 0xaaaaaaaa, 0x00000020},
                                 {NFlag, 0xffffff83, 0x80000000, 0xffffff83},
                                 {ZFlag, 0xcccccccc, 0x0000007f, 0xcccccccc},
                                 {NZVFlag, 0xffffffe0, 0x00000002, 0xffffffe0},
                                 {NZVFlag, 0x33333333, 0xffffff81, 0x33333333},
                                 {NZFlag, 0x7ffffffe, 0x00007ffd, 0x7ffffffe},
                                 {VFlag, 0x0000007f, 0x80000001, 0x0000007f},
                                 {NVFlag, 0xffff8001, 0x00000000, 0xffff8001},
                                 {NFlag, 0x0000007d, 0xcccccccc, 0x0000007d},
                                 {ZVFlag, 0xffffff82, 0x55555555, 0xffffff82},
                                 {NZVFlag, 0x00007ffe, 0x0000007d, 0x00007ffe},
                                 {NCFlag, 0xaaaaaaaa, 0x80000001, 0xaaaaaaaa},
                                 {ZVFlag, 0x33333333, 0x00000020, 0x33333333},
                                 {NZCFlag, 0x0000007f, 0xffffffe0, 0x0000007f},
                                 {NCVFlag, 0x80000001, 0xffffff83, 0x80000001},
                                 {ZVFlag, 0x00000001, 0xffffffe0, 0x00000001},
                                 {NZFlag, 0xffffff82, 0xffffff80, 0xffffff82},
                                 {NCVFlag, 0xffffff80, 0x00007ffd, 0xffffff80},
                                 {VFlag, 0x0000007e, 0x0000007e, 0x0000007e},
                                 {CVFlag, 0x7fffffff, 0x7fffffff, 0x7fffffff},
                                 {CVFlag, 0x00007ffd, 0xffffff83, 0x00007ffd},
                                 {VFlag, 0xffff8000, 0x80000001, 0xffff8000},
                                 {ZFlag, 0x0000007f, 0x00000020, 0x0000007f},
                                 {NZFlag, 0xffff8003, 0x33333333, 0xffff8003},
                                 {ZFlag, 0x00000020, 0xfffffffd, 0x00000020},
                                 {ZCVFlag, 0xffffff81, 0xffff8000, 0xffffff81},
                                 {NFlag, 0xffffffe0, 0xffffff83, 0xffffffe0},
                                 {ZCVFlag, 0x80000001, 0x55555555, 0x80000001},
                                 {ZCVFlag, 0x7fffffff, 0x00000020, 0x7fffffff},
                                 {NCFlag, 0x80000001, 0xcccccccc, 0x80000001},
                                 {ZVFlag, 0x55555555, 0x7ffffffd, 0x55555555},
                                 {CVFlag, 0xffffffff, 0xffff8001, 0xffffffff},
                                 {CFlag, 0xfffffffe, 0xffff8000, 0xfffffffe},
                                 {NZFlag, 0xcccccccc, 0xffffff82, 0xcccccccc},
                                 {NZVFlag, 0x55555555, 0xffffff81, 0x55555555},
                                 {NFlag, 0xfffffffd, 0xffffffe0, 0xfffffffd},
                                 {NCVFlag, 0xfffffffd, 0xffff8003, 0xfffffffd},
                                 {NFlag, 0xfffffffe, 0xffffff80, 0xfffffffe},
                                 {ZFlag, 0xffff8002, 0x00007ffd, 0xffff8002},
                                 {CFlag, 0xffffff81, 0xffff8002, 0xffffff81},
                                 {ZVFlag, 0xffffff81, 0x0000007f, 0xffffff81},
                                 {NFlag, 0x00000020, 0xffffff83, 0x00000020},
                                 {NZFlag, 0xfffffffd, 0x0000007e, 0xfffffffd},
                                 {NZVFlag, 0xfffffffd, 0x0000007e, 0xfffffffd},
                                 {NCVFlag, 0xffffffe0, 0x80000000, 0xffffffe0},
                                 {NFlag, 0xffffff80, 0x55555555, 0xffffff80},
                                 {ZFlag, 0xffffff80, 0x00007ffd, 0xffffff80},
                                 {ZCVFlag, 0x00007ffe, 0xffff8003, 0x00007ffe},
                                 {VFlag, 0x7fffffff, 0x80000000, 0x7fffffff},
                                 {NVFlag, 0xcccccccc, 0xffffff80, 0xcccccccc},
                                 {NZCVFlag, 0xcccccccc, 0x7ffffffd, 0xcccccccc},
                                 {NZCFlag, 0x33333333, 0xffff8001, 0x33333333},
                                 {ZFlag, 0xffffff81, 0xffff8000, 0xffffff81},
                                 {NZCVFlag, 0x0000007f, 0xffff8003, 0x0000007f},
                                 {NFlag, 0x0000007d, 0x00007ffd, 0x0000007d},
                                 {ZFlag, 0x00007ffd, 0x00000020, 0x00007ffd},
                                 {NFlag, 0xffffff81, 0xfffffffd, 0xffffff81},
                                 {NZFlag, 0xffffff83, 0xffffff80, 0xffffff83},
                                 {NCFlag, 0x00007ffd, 0x00000001, 0x00007ffd},
                                 {NVFlag, 0x7fffffff, 0xffffff82, 0x7fffffff},
                                 {NZVFlag, 0xffff8000, 0x7ffffffd, 0xffff8000},
                                 {NZCFlag, 0xaaaaaaaa, 0x0000007f, 0xaaaaaaaa},
                                 {NZFlag, 0x7ffffffe, 0xffffffff, 0x7ffffffe},
                                 {NZFlag, 0x00000000, 0x00007ffd, 0x00000000},
                                 {NZFlag, 0x55555555, 0x0000007f, 0x55555555},
                                 {CFlag, 0xcccccccc, 0x0000007f, 0xcccccccc},
                                 {NZCVFlag, 0x00007fff, 0x33333333, 0x00007fff},
                                 {NCFlag, 0xffff8000, 0xffffff81, 0xffff8000},
                                 {ZCVFlag, 0xffffff82, 0xffffff83, 0xffffff82},
                                 {NVFlag, 0xffffffe0, 0x0000007e, 0xffffffe0},
                                 {NCFlag, 0x7ffffffe, 0x00007fff, 0x7ffffffe},
                                 {VFlag, 0x7ffffffd, 0x00000020, 0x7ffffffd},
                                 {ZCVFlag, 0xffffff81, 0xffffff81, 0xffffff81},
                                 {NVFlag, 0xaaaaaaaa, 0x00000000, 0xaaaaaaaa},
                                 {CFlag, 0x33333333, 0x00000002, 0x33333333},
                                 {CVFlag, 0x55555555, 0xfffffffd, 0x55555555},
                                 {ZCFlag, 0x00000000, 0xffffff80, 0x00000000},
                                 {NZCFlag, 0x00007fff, 0x80000000, 0x00007fff},
                                 {NCVFlag, 0xffffff80, 0xffffffe0, 0xffffff80},
                                 {NZCVFlag, 0x00000001, 0x80000001, 0x00000001},
                                 {NZCFlag, 0x55555555, 0x0000007d, 0x55555555},
                                 {NCFlag, 0x80000001, 0xffff8001, 0x80000001},
                                 {NCVFlag, 0x7fffffff, 0xffffff82, 0x7fffffff},
                                 {NCFlag, 0x00000000, 0x00007ffd, 0x00000000},
                                 {NZFlag, 0x0000007e, 0xffffffff, 0x0000007e},
                                 {NVFlag, 0xffffff81, 0x00007ffd, 0xffffff81}};

static const Inputs kRdIsNotRnIsNotRm[] =
    {{ZCVFlag, 0x0000007d, 0x7ffffffe, 0x00000000},
     {NZCVFlag, 0x00007ffe, 0x00007ffe, 0xaaaaaaaa},
     {CFlag, 0xfffffffd, 0xffff8001, 0x55555555},
     {NVFlag, 0x7fffffff, 0x00007ffd, 0xffff8002},
     {NZVFlag, 0x0000007f, 0xffff8003, 0x00007fff},
     {CFlag, 0x00000000, 0xaaaaaaaa, 0x7ffffffe},
     {NFlag, 0xffffffe0, 0xcccccccc, 0x80000001},
     {NFlag, 0x7fffffff, 0xffffff80, 0xcccccccc},
     {NFlag, 0xffff8001, 0x00000002, 0x00000001},
     {ZVFlag, 0xcccccccc, 0xaaaaaaaa, 0x00000001},
     {NCFlag, 0xffff8001, 0xffff8001, 0xffff8003},
     {VFlag, 0x33333333, 0x7ffffffe, 0xfffffffd},
     {CFlag, 0x00000002, 0xffffffe0, 0x00007ffd},
     {CFlag, 0xffffff82, 0x00000002, 0xfffffffd},
     {ZCVFlag, 0x80000001, 0xffffffe0, 0x00000020},
     {ZFlag, 0xffff8001, 0x80000000, 0x80000001},
     {ZVFlag, 0x00000020, 0xcccccccc, 0xffffff80},
     {VFlag, 0x00007fff, 0x00000002, 0x55555555},
     {NVFlag, 0x0000007f, 0xffff8000, 0xaaaaaaaa},
     {NFlag, 0xffffff81, 0x0000007f, 0x00000002},
     {CVFlag, 0x00000020, 0x0000007f, 0x00000000},
     {NZCFlag, 0x00007ffd, 0xffff8002, 0xaaaaaaaa},
     {ZCVFlag, 0x00007ffe, 0xffffffff, 0x80000000},
     {NZCFlag, 0x00000020, 0xffffff81, 0x0000007d},
     {NZCVFlag, 0x00000020, 0x0000007d, 0xffff8001},
     {NZCFlag, 0xffffff80, 0xfffffffe, 0xffffffe0},
     {ZCVFlag, 0xffff8001, 0xfffffffd, 0x00000002},
     {NCFlag, 0xffff8001, 0x00007ffe, 0xffffff80},
     {NZVFlag, 0xfffffffd, 0xcccccccc, 0x7ffffffe},
     {ZCVFlag, 0x00007ffe, 0xffff8003, 0x00007ffd},
     {VFlag, 0x00007ffe, 0x0000007d, 0x00007ffd},
     {NCFlag, 0x00000002, 0xffff8002, 0xfffffffd},
     {NZFlag, 0x00000002, 0x0000007f, 0x0000007d},
     {VFlag, 0x80000000, 0xffffff83, 0x00000000},
     {NZVFlag, 0xfffffffd, 0x0000007e, 0xfffffffd},
     {NFlag, 0x80000000, 0x00000020, 0x00007ffe},
     {NVFlag, 0xffffff81, 0x00000002, 0x00000000},
     {NZCVFlag, 0x33333333, 0x00007ffd, 0xaaaaaaaa},
     {NVFlag, 0xffff8003, 0xcccccccc, 0x0000007d},
     {NZVFlag, 0xfffffffd, 0xffffff80, 0xffffffe0},
     {NCFlag, 0x0000007d, 0xffffffff, 0x0000007d},
     {ZCFlag, 0x0000007d, 0xffffff83, 0x00000001},
     {NFlag, 0xffff8000, 0x80000000, 0x00000002},
     {CFlag, 0x33333333, 0xffff8001, 0x0000007f},
     {NCFlag, 0xfffffffd, 0x33333333, 0x0000007d},
     {CVFlag, 0x0000007f, 0x7ffffffd, 0xffff8003},
     {CVFlag, 0x80000000, 0xffff8000, 0xffffffe0},
     {ZFlag, 0x00007ffe, 0xcccccccc, 0x33333333},
     {NZCVFlag, 0xfffffffe, 0x0000007e, 0x0000007d},
     {CVFlag, 0xffffff82, 0x80000001, 0xffffff80},
     {CFlag, 0xffff8002, 0x7fffffff, 0xffff8001},
     {ZVFlag, 0x7fffffff, 0xffffff83, 0x00007fff},
     {CVFlag, 0xffffffe0, 0x7ffffffe, 0x7fffffff},
     {ZCVFlag, 0xffffffe0, 0x00007ffd, 0xffffff82},
     {ZFlag, 0x00000020, 0xffff8001, 0x00000020},
     {NFlag, 0x00007ffd, 0xfffffffe, 0x00000000},
     {NFlag, 0xfffffffe, 0x80000001, 0xffffff80},
     {CVFlag, 0x00000002, 0xcccccccc, 0x00007ffd},
     {ZCFlag, 0x80000001, 0x00000020, 0xaaaaaaaa},
     {NZVFlag, 0x00007ffe, 0x00000000, 0x0000007d},
     {ZFlag, 0x00000000, 0xffff8003, 0xffff8003},
     {NCVFlag, 0xffffff83, 0xffffff81, 0xffffff83},
     {NVFlag, 0xffffffff, 0xffffff81, 0x33333333},
     {CVFlag, 0x00007ffd, 0xcccccccc, 0x55555555},
     {ZCVFlag, 0xffffffff, 0x80000001, 0x55555555},
     {NZVFlag, 0x7ffffffd, 0x0000007e, 0xcccccccc},
     {NZCFlag, 0xfffffffe, 0xfffffffd, 0x7fffffff},
     {ZCVFlag, 0x0000007f, 0x80000001, 0xffffffff},
     {NZCVFlag, 0xffffff83, 0x00007ffd, 0x55555555},
     {NVFlag, 0xffffffff, 0xffffff82, 0x00000002},
     {NCVFlag, 0x00000001, 0x00000002, 0xffff8002},
     {NVFlag, 0x0000007d, 0x80000000, 0xffff8000},
     {ZFlag, 0x00000000, 0x0000007f, 0xffffff81},
     {CVFlag, 0x00000000, 0x33333333, 0x00000000},
     {ZFlag, 0x80000001, 0x0000007d, 0xcccccccc},
     {ZFlag, 0x80000000, 0x0000007f, 0x00000000},
     {ZFlag, 0x7ffffffe, 0xfffffffe, 0x0000007e},
     {NZVFlag, 0xcccccccc, 0x80000000, 0xcccccccc},
     {VFlag, 0x80000001, 0xffff8000, 0x7fffffff},
     {NFlag, 0x55555555, 0x0000007e, 0x7ffffffd},
     {NFlag, 0x00007fff, 0x00007ffe, 0xffffff80},
     {ZFlag, 0x7fffffff, 0xffffff83, 0xffffff80},
     {VFlag, 0x7fffffff, 0x7fffffff, 0x0000007d},
     {ZVFlag, 0xfffffffe, 0xffffff80, 0x33333333},
     {NZFlag, 0x55555555, 0x0000007e, 0x7ffffffd},
     {VFlag, 0x55555555, 0xcccccccc, 0x0000007e},
     {NVFlag, 0x00007ffd, 0x7ffffffd, 0xffffff80},
     {NZCFlag, 0x00007ffe, 0xfffffffd, 0x00000002},
     {NZCVFlag, 0x7ffffffd, 0x7fffffff, 0xffffff81},
     {NZCFlag, 0x7ffffffd, 0x80000000, 0x7ffffffe},
     {CFlag, 0x7ffffffd, 0x00007fff, 0x0000007f},
     {CVFlag, 0xffff8001, 0xcccccccc, 0xffffffff},
     {ZFlag, 0x00007fff, 0xffffffff, 0xffffff82},
     {ZCVFlag, 0x80000000, 0x00000001, 0x80000001},
     {ZVFlag, 0xffffff82, 0xfffffffe, 0x0000007e},
     {NZCFlag, 0xffffffe0, 0x55555555, 0x00000000},
     {NZFlag, 0xffff8001, 0xffff8003, 0xffffff82},
     {ZFlag, 0x7fffffff, 0x00000002, 0xffffffe0},
     {ZCFlag, 0x00000001, 0xffffff81, 0xffffff81},
     {ZFlag, 0x7ffffffe, 0xffffff81, 0xfffffffe},
     {CFlag, 0x00007ffe, 0x7ffffffd, 0x80000000},
     {NZVFlag, 0x00000000, 0xaaaaaaaa, 0x0000007d},
     {NZVFlag, 0x7ffffffd, 0xffff8001, 0x00007fff},
     {NZCFlag, 0x55555555, 0x7fffffff, 0x80000001},
     {ZCFlag, 0xfffffffe, 0x0000007e, 0xcccccccc},
     {NVFlag, 0x80000000, 0xfffffffe, 0x0000007e},
     {NVFlag, 0xffffff83, 0xffffff82, 0x00000000},
     {NVFlag, 0xfffffffe, 0x33333333, 0x80000000},
     {ZVFlag, 0x80000000, 0xffff8002, 0xffffff82},
     {CFlag, 0xcccccccc, 0x00007fff, 0x00000000},
     {ZVFlag, 0xfffffffe, 0x7ffffffe, 0x00007ffe},
     {NZCVFlag, 0x00007fff, 0xffff8002, 0xffff8001},
     {NZVFlag, 0x00000002, 0xffffff81, 0xfffffffe},
     {NCVFlag, 0xffffff83, 0xffff8001, 0x00007ffe},
     {ZFlag, 0x0000007d, 0x00007ffe, 0xffff8000},
     {NCVFlag, 0x55555555, 0xaaaaaaaa, 0x7fffffff},
     {ZVFlag, 0x0000007e, 0xfffffffd, 0x7ffffffe},
     {ZCVFlag, 0xffffff83, 0x33333333, 0xffffff82},
     {ZFlag, 0xffff8001, 0x00000020, 0xffff8003},
     {NCFlag, 0xfffffffe, 0xaaaaaaaa, 0x80000001},
     {NZFlag, 0x80000000, 0xaaaaaaaa, 0xffffffff},
     {NZCFlag, 0xffffffe0, 0x00007ffe, 0x0000007e},
     {NFlag, 0x0000007f, 0x80000000, 0x33333333},
     {NZFlag, 0xaaaaaaaa, 0xaaaaaaaa, 0x80000000},
     {ZVFlag, 0x7ffffffd, 0xffffff83, 0x0000007e},
     {NZVFlag, 0x0000007d, 0x00000002, 0x80000000},
     {ZFlag, 0xffffff83, 0xfffffffe, 0xffffffe0},
     {CVFlag, 0x0000007e, 0x00000020, 0x0000007f},
     {CFlag, 0xffffffe0, 0x00007ffd, 0x80000000},
     {NZCVFlag, 0x00007ffd, 0xffff8001, 0xffff8001},
     {NZFlag, 0x33333333, 0x0000007d, 0xffffffe0},
     {NCVFlag, 0x55555555, 0x7ffffffe, 0x00000000},
     {NZFlag, 0x0000007e, 0xffffffff, 0xffffffff},
     {NZCFlag, 0x00007ffd, 0xfffffffd, 0xffff8002},
     {NZCVFlag, 0x33333333, 0xffff8003, 0xffff8002},
     {NCVFlag, 0x0000007f, 0x33333333, 0x0000007d},
     {NFlag, 0x7ffffffe, 0xffffff81, 0xffffffe0},
     {ZCFlag, 0xaaaaaaaa, 0xffffff83, 0xffff8002},
     {NFlag, 0x00000000, 0x00000002, 0x0000007e},
     {CVFlag, 0x0000007e, 0x0000007f, 0x80000001},
     {NZCFlag, 0x7ffffffd, 0xffffff81, 0xffff8003},
     {NCVFlag, 0xffffff83, 0x00000020, 0x00000002},
     {NCFlag, 0xffffff82, 0x7fffffff, 0xffff8003},
     {NZVFlag, 0x0000007f, 0xffffffff, 0xffffff82},
     {ZCFlag, 0xffffff80, 0x00007ffd, 0x33333333},
     {NZVFlag, 0xfffffffe, 0xcccccccc, 0xffff8002},
     {ZVFlag, 0x80000001, 0xfffffffe, 0x0000007d},
     {ZCFlag, 0x0000007f, 0xfffffffd, 0x80000000},
     {CFlag, 0xfffffffe, 0xffff8001, 0x33333333},
     {NFlag, 0x80000001, 0x80000001, 0xcccccccc},
     {NCVFlag, 0xffffff81, 0x80000000, 0x00007ffe},
     {NCFlag, 0xffffffff, 0xfffffffd, 0xffffff80},
     {NCFlag, 0x00000020, 0x00000002, 0x80000001},
     {NZVFlag, 0xcccccccc, 0xffff8000, 0xffff8003},
     {NZCFlag, 0x80000000, 0xaaaaaaaa, 0xffffff83},
     {ZCFlag, 0xffff8000, 0x00000002, 0xffff8002},
     {ZCVFlag, 0x00000001, 0xffff8001, 0x00007ffe},
     {NVFlag, 0xfffffffd, 0xffff8003, 0xffffff82},
     {NZCVFlag, 0x80000000, 0x7ffffffd, 0xffffff80},
     {ZCFlag, 0x33333333, 0x0000007e, 0xfffffffd},
     {NZCFlag, 0x0000007d, 0xffffffe0, 0x00000001},
     {NZCVFlag, 0xcccccccc, 0xaaaaaaaa, 0x7fffffff},
     {ZVFlag, 0x00007ffe, 0x00000000, 0x55555555},
     {NZFlag, 0x80000001, 0x7ffffffe, 0x7ffffffe},
     {ZFlag, 0x33333333, 0x0000007e, 0xcccccccc},
     {NZCFlag, 0x0000007e, 0xffffffe0, 0x33333333},
     {NCVFlag, 0xffffff83, 0xffff8002, 0x80000001},
     {CFlag, 0x7fffffff, 0xffffff81, 0x33333333},
     {NCFlag, 0xcccccccc, 0x33333333, 0xffff8003},
     {NVFlag, 0x0000007e, 0xffffff80, 0x00007fff},
     {NCFlag, 0xffff8003, 0x0000007d, 0xffffff82},
     {ZCVFlag, 0xaaaaaaaa, 0x7fffffff, 0xffff8001},
     {ZCVFlag, 0x00007ffe, 0x55555555, 0x00000020},
     {ZVFlag, 0x00000001, 0x80000001, 0x80000001},
     {CVFlag, 0x00000001, 0xffffff80, 0x0000007f},
     {NZCVFlag, 0xfffffffd, 0xffffff81, 0x00000020},
     {NZCVFlag, 0x00000020, 0x80000000, 0xffffff81},
     {NZCVFlag, 0xffffff81, 0x7fffffff, 0xffff8002},
     {NZVFlag, 0xfffffffe, 0x00000000, 0xffff8000},
     {ZCVFlag, 0xffff8000, 0xffffff83, 0xffffff81},
     {NZCVFlag, 0x00007ffd, 0xaaaaaaaa, 0x00007ffe},
     {NZCFlag, 0x0000007e, 0xcccccccc, 0x80000000},
     {ZVFlag, 0xffff8002, 0x7ffffffd, 0xffffffe0},
     {ZCFlag, 0xaaaaaaaa, 0x7ffffffe, 0x0000007e},
     {VFlag, 0xffffffff, 0x7fffffff, 0xcccccccc},
     {NZCFlag, 0xfffffffe, 0xffff8001, 0x80000001},
     {VFlag, 0xfffffffd, 0xffff8000, 0xffff8003},
     {CVFlag, 0x80000001, 0x80000001, 0x00007ffd},
     {NVFlag, 0x55555555, 0x0000007e, 0xffffff82},
     {NCFlag, 0x0000007f, 0x7ffffffe, 0x00000001},
     {NVFlag, 0x00000001, 0xffffff83, 0xffffff82},
     {CFlag, 0xffffff81, 0xffff8002, 0xffff8000},
     {CFlag, 0xfffffffe, 0x00000020, 0x0000007d},
     {CVFlag, 0x00007ffd, 0xffffff80, 0x00000002},
     {NVFlag, 0x7ffffffe, 0x00007ffd, 0x00000001},
     {CFlag, 0x80000001, 0x7fffffff, 0xffff8003},
     {ZVFlag, 0x00007ffe, 0xffff8000, 0xffffffff},
     {ZFlag, 0x00000001, 0xffff8002, 0x0000007e},
     {NZCVFlag, 0xffffffff, 0x33333333, 0x0000007f},
     {NZFlag, 0x7ffffffd, 0x00007ffe, 0x00000020}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{eq, r0, r0, r0},
                                "eq r0 r0 r0",
                                "Condition_eq_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ne, r0, r0, r0},
                                "ne r0 r0 r0",
                                "Condition_ne_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{cs, r0, r0, r0},
                                "cs r0 r0 r0",
                                "Condition_cs_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{cc, r0, r0, r0},
                                "cc r0 r0 r0",
                                "Condition_cc_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{mi, r0, r0, r0},
                                "mi r0 r0 r0",
                                "Condition_mi_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{pl, r0, r0, r0},
                                "pl r0 r0 r0",
                                "Condition_pl_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{vs, r0, r0, r0},
                                "vs r0 r0 r0",
                                "Condition_vs_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{vc, r0, r0, r0},
                                "vc r0 r0 r0",
                                "Condition_vc_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{hi, r0, r0, r0},
                                "hi r0 r0 r0",
                                "Condition_hi_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ls, r0, r0, r0},
                                "ls r0 r0 r0",
                                "Condition_ls_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ge, r0, r0, r0},
                                "ge r0 r0 r0",
                                "Condition_ge_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{lt, r0, r0, r0},
                                "lt r0 r0 r0",
                                "Condition_lt_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{gt, r0, r0, r0},
                                "gt r0 r0 r0",
                                "Condition_gt_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{le, r0, r0, r0},
                                "le r0 r0 r0",
                                "Condition_le_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{al, r0, r0, r0},
                                "al r0 r0 r0",
                                "Condition_al_r0_r0_r0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ls, r3, r3, r11},
                                "ls r3 r3 r11",
                                "RdIsRn_ls_r3_r3_r11",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{le, r7, r7, r11},
                                "le r7 r7 r11",
                                "RdIsRn_le_r7_r7_r11",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{vc, r14, r14, r7},
                                "vc r14 r14 r7",
                                "RdIsRn_vc_r14_r14_r7",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{vs, r3, r3, r14},
                                "vs r3 r3 r14",
                                "RdIsRn_vs_r3_r3_r14",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{pl, r10, r10, r7},
                                "pl r10 r10 r7",
                                "RdIsRn_pl_r10_r10_r7",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{hi, r8, r8, r7},
                                "hi r8 r8 r7",
                                "RdIsRn_hi_r8_r8_r7",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{le, r8, r8, r6},
                                "le r8 r8 r6",
                                "RdIsRn_le_r8_r8_r6",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ne, r5, r5, r1},
                                "ne r5 r5 r1",
                                "RdIsRn_ne_r5_r5_r1",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{gt, r10, r10, r3},
                                "gt r10 r10 r3",
                                "RdIsRn_gt_r10_r10_r3",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{ls, r3, r3, r14},
                                "ls r3 r3 r14",
                                "RdIsRn_ls_r3_r3_r14",
                                ARRAY_SIZE(kRdIsRn),
                                kRdIsRn},
                               {{hi, r0, r9, r0},
                                "hi r0 r9 r0",
                                "RdIsRm_hi_r0_r9_r0",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{eq, r5, r12, r5},
                                "eq r5 r12 r5",
                                "RdIsRm_eq_r5_r12_r5",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{gt, r12, r3, r12},
                                "gt r12 r3 r12",
                                "RdIsRm_gt_r12_r3_r12",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{lt, r8, r11, r8},
                                "lt r8 r11 r8",
                                "RdIsRm_lt_r8_r11_r8",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{cs, r7, r6, r7},
                                "cs r7 r6 r7",
                                "RdIsRm_cs_r7_r6_r7",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{lt, r9, r0, r9},
                                "lt r9 r0 r9",
                                "RdIsRm_lt_r9_r0_r9",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{cc, r6, r5, r6},
                                "cc r6 r5 r6",
                                "RdIsRm_cc_r6_r5_r6",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{pl, r7, r4, r7},
                                "pl r7 r4 r7",
                                "RdIsRm_pl_r7_r4_r7",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{ls, r0, r10, r0},
                                "ls r0 r10 r0",
                                "RdIsRm_ls_r0_r10_r0",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{hi, r9, r14, r9},
                                "hi r9 r14 r9",
                                "RdIsRm_hi_r9_r14_r9",
                                ARRAY_SIZE(kRdIsRm),
                                kRdIsRm},
                               {{mi, r8, r5, r10},
                                "mi r8 r5 r10",
                                "RdIsNotRnIsNotRm_mi_r8_r5_r10",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{ge, r2, r5, r4},
                                "ge r2 r5 r4",
                                "RdIsNotRnIsNotRm_ge_r2_r5_r4",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{cc, r2, r7, r1},
                                "cc r2 r7 r1",
                                "RdIsNotRnIsNotRm_cc_r2_r7_r1",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{ne, r5, r9, r0},
                                "ne r5 r9 r0",
                                "RdIsNotRnIsNotRm_ne_r5_r9_r0",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{eq, r10, r6, r5},
                                "eq r10 r6 r5",
                                "RdIsNotRnIsNotRm_eq_r10_r6_r5",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{ne, r0, r4, r6},
                                "ne r0 r4 r6",
                                "RdIsNotRnIsNotRm_ne_r0_r4_r6",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{le, r7, r6, r0},
                                "le r7 r6 r0",
                                "RdIsNotRnIsNotRm_le_r7_r6_r0",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{hi, r12, r11, r3},
                                "hi r12 r11 r3",
                                "RdIsNotRnIsNotRm_hi_r12_r11_r3",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{pl, r3, r7, r12},
                                "pl r3 r7 r12",
                                "RdIsNotRnIsNotRm_pl_r3_r7_r12",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm},
                               {{cc, r4, r1, r7},
                                "cc r4 r1 r7",
                                "RdIsNotRnIsNotRm_cc_r4_r1_r7",
                                ARRAY_SIZE(kRdIsNotRnIsNotRm),
                                kRdIsNotRnIsNotRm}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-adc-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-adcs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-add-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-adds-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-and-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-ands-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-asr-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-asrs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-bic-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-bics-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-eor-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-eors-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-lsl-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-lsls-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-lsr-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-lsrs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-orn-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-orns-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-orr-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-orrs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-ror-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-rors-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-rsb-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-rsbs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-sbc-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-sbcs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-sub-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-subs-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-sxtab-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-sxtab16-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-sxtah-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-uxtab-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-uxtab16-t32.h"
#include "aarch32/traces/simulator-cond-rd-rn-operand-rm-uxtah-t32.h"


// The maximum number of errors to report in detail for each test.
const unsigned kErrorReportLimit = 8;

typedef void (MacroAssembler::*Fn)(Condition cond,
                                   Register rd,
                                   Register rn,
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
    Register rm = kTests[i].operands.rm;
    Operand op(rm);
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
    __ Ldr(rd, MemOperand(input_ptr, offsetof(Inputs, rd)));
    __ Ldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));
    __ Ldr(rm, MemOperand(input_ptr, offsetof(Inputs, rm)));

    (masm.*instruction)(cond, rd, rn, op);

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
        uint32_t rd = results[i]->outputs[j].rd;
        uint32_t rn = results[i]->outputs[j].rn;
        uint32_t rm = results[i]->outputs[j].rm;
        uint32_t apsr_input = kTests[i].inputs[j].apsr;
        uint32_t rd_input = kTests[i].inputs[j].rd;
        uint32_t rn_input = kTests[i].inputs[j].rn;
        uint32_t rm_input = kTests[i].inputs[j].rm;
        uint32_t apsr_ref = reference[i].outputs[j].apsr;
        uint32_t rd_ref = reference[i].outputs[j].rd;
        uint32_t rn_ref = reference[i].outputs[j].rn;
        uint32_t rm_ref = reference[i].outputs[j].rm;

        if (((apsr != apsr_ref) || (rd != rd_ref) || (rn != rn_ref) ||
             (rm != rm_ref)) &&
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
          printf("0x%08" PRIx32, rm_input);
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, apsr_ref);
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
#define TEST(mnemonic)                                                      \
  void Test_##mnemonic() {                                                  \
    TestHelper(&MacroAssembler::mnemonic, #mnemonic, kReference##mnemonic); \
  }                                                                         \
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RD_RN_OPERAND_RM_" #mnemonic \
                       "_T32",                                              \
                       &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                      \
  void Test_##mnemonic() {                                                  \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");             \
    USE(TestHelper);                                                        \
  }                                                                         \
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RD_RN_OPERAND_RM_" #mnemonic \
                       "_T32",                                              \
                       &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
