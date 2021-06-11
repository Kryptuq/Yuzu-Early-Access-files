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
  M(Cmp)                       \
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
  int32_t immediate;
};

// Input data to feed to the instruction.
struct Inputs {
  uint32_t apsr;
  uint32_t rd;
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

static const Inputs kCondition[] = {{NFlag, 0xabababab},
                                    {ZFlag, 0xabababab},
                                    {CFlag, 0xabababab},
                                    {VFlag, 0xabababab},
                                    {NZFlag, 0xabababab},
                                    {NCFlag, 0xabababab},
                                    {NVFlag, 0xabababab},
                                    {ZCFlag, 0xabababab},
                                    {ZVFlag, 0xabababab},
                                    {CVFlag, 0xabababab},
                                    {NZCFlag, 0xabababab},
                                    {NZVFlag, 0xabababab},
                                    {NCVFlag, 0xabababab},
                                    {ZCVFlag, 0xabababab},
                                    {NZCVFlag, 0xabababab}};

static const Inputs kModifiedImmediate[] =
    {{NoFlag, 0x00000000}, {NoFlag, 0x00000001}, {NoFlag, 0x00000002},
     {NoFlag, 0x00000020}, {NoFlag, 0x0000007d}, {NoFlag, 0x0000007e},
     {NoFlag, 0x0000007f}, {NoFlag, 0x00007ffd}, {NoFlag, 0x00007ffe},
     {NoFlag, 0x00007fff}, {NoFlag, 0x33333333}, {NoFlag, 0x55555555},
     {NoFlag, 0x7ffffffd}, {NoFlag, 0x7ffffffe}, {NoFlag, 0x7fffffff},
     {NoFlag, 0x80000000}, {NoFlag, 0x80000001}, {NoFlag, 0xaaaaaaaa},
     {NoFlag, 0xcccccccc}, {NoFlag, 0xffff8000}, {NoFlag, 0xffff8001},
     {NoFlag, 0xffff8002}, {NoFlag, 0xffff8003}, {NoFlag, 0xffffff80},
     {NoFlag, 0xffffff81}, {NoFlag, 0xffffff82}, {NoFlag, 0xffffff83},
     {NoFlag, 0xffffffe0}, {NoFlag, 0xfffffffd}, {NoFlag, 0xfffffffe},
     {NoFlag, 0xffffffff}};


// A loop will be generated for each element of this array.
const TestLoopData kTests[] = {{{eq, r0, 0},
                                "eq r0 0",
                                "Condition_eq_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ne, r0, 0},
                                "ne r0 0",
                                "Condition_ne_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{cs, r0, 0},
                                "cs r0 0",
                                "Condition_cs_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{cc, r0, 0},
                                "cc r0 0",
                                "Condition_cc_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{mi, r0, 0},
                                "mi r0 0",
                                "Condition_mi_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{pl, r0, 0},
                                "pl r0 0",
                                "Condition_pl_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{vs, r0, 0},
                                "vs r0 0",
                                "Condition_vs_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{vc, r0, 0},
                                "vc r0 0",
                                "Condition_vc_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{hi, r0, 0},
                                "hi r0 0",
                                "Condition_hi_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ls, r0, 0},
                                "ls r0 0",
                                "Condition_ls_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{ge, r0, 0},
                                "ge r0 0",
                                "Condition_ge_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{lt, r0, 0},
                                "lt r0 0",
                                "Condition_lt_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{gt, r0, 0},
                                "gt r0 0",
                                "Condition_gt_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{le, r0, 0},
                                "le r0 0",
                                "Condition_le_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{al, r0, 0},
                                "al r0 0",
                                "Condition_al_r0_0",
                                ARRAY_SIZE(kCondition),
                                kCondition},
                               {{al, r0, 0},
                                "al r0 0",
                                "ModifiedImmediate_al_r0_0",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 1},
                                "al r0 1",
                                "ModifiedImmediate_al_r0_1",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 2},
                                "al r0 2",
                                "ModifiedImmediate_al_r0_2",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 3},
                                "al r0 3",
                                "ModifiedImmediate_al_r0_3",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 4},
                                "al r0 4",
                                "ModifiedImmediate_al_r0_4",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 5},
                                "al r0 5",
                                "ModifiedImmediate_al_r0_5",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 6},
                                "al r0 6",
                                "ModifiedImmediate_al_r0_6",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 7},
                                "al r0 7",
                                "ModifiedImmediate_al_r0_7",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 8},
                                "al r0 8",
                                "ModifiedImmediate_al_r0_8",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 9},
                                "al r0 9",
                                "ModifiedImmediate_al_r0_9",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 10},
                                "al r0 10",
                                "ModifiedImmediate_al_r0_10",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 11},
                                "al r0 11",
                                "ModifiedImmediate_al_r0_11",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 12},
                                "al r0 12",
                                "ModifiedImmediate_al_r0_12",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 13},
                                "al r0 13",
                                "ModifiedImmediate_al_r0_13",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 14},
                                "al r0 14",
                                "ModifiedImmediate_al_r0_14",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 15},
                                "al r0 15",
                                "ModifiedImmediate_al_r0_15",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 16},
                                "al r0 16",
                                "ModifiedImmediate_al_r0_16",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 17},
                                "al r0 17",
                                "ModifiedImmediate_al_r0_17",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 18},
                                "al r0 18",
                                "ModifiedImmediate_al_r0_18",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 19},
                                "al r0 19",
                                "ModifiedImmediate_al_r0_19",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 20},
                                "al r0 20",
                                "ModifiedImmediate_al_r0_20",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 21},
                                "al r0 21",
                                "ModifiedImmediate_al_r0_21",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 22},
                                "al r0 22",
                                "ModifiedImmediate_al_r0_22",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 23},
                                "al r0 23",
                                "ModifiedImmediate_al_r0_23",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 24},
                                "al r0 24",
                                "ModifiedImmediate_al_r0_24",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 25},
                                "al r0 25",
                                "ModifiedImmediate_al_r0_25",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 26},
                                "al r0 26",
                                "ModifiedImmediate_al_r0_26",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 27},
                                "al r0 27",
                                "ModifiedImmediate_al_r0_27",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 28},
                                "al r0 28",
                                "ModifiedImmediate_al_r0_28",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 29},
                                "al r0 29",
                                "ModifiedImmediate_al_r0_29",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 30},
                                "al r0 30",
                                "ModifiedImmediate_al_r0_30",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 31},
                                "al r0 31",
                                "ModifiedImmediate_al_r0_31",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 32},
                                "al r0 32",
                                "ModifiedImmediate_al_r0_32",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 33},
                                "al r0 33",
                                "ModifiedImmediate_al_r0_33",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 34},
                                "al r0 34",
                                "ModifiedImmediate_al_r0_34",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 35},
                                "al r0 35",
                                "ModifiedImmediate_al_r0_35",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 36},
                                "al r0 36",
                                "ModifiedImmediate_al_r0_36",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 37},
                                "al r0 37",
                                "ModifiedImmediate_al_r0_37",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 38},
                                "al r0 38",
                                "ModifiedImmediate_al_r0_38",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 39},
                                "al r0 39",
                                "ModifiedImmediate_al_r0_39",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 40},
                                "al r0 40",
                                "ModifiedImmediate_al_r0_40",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 41},
                                "al r0 41",
                                "ModifiedImmediate_al_r0_41",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 42},
                                "al r0 42",
                                "ModifiedImmediate_al_r0_42",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 43},
                                "al r0 43",
                                "ModifiedImmediate_al_r0_43",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 44},
                                "al r0 44",
                                "ModifiedImmediate_al_r0_44",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 45},
                                "al r0 45",
                                "ModifiedImmediate_al_r0_45",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 46},
                                "al r0 46",
                                "ModifiedImmediate_al_r0_46",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 47},
                                "al r0 47",
                                "ModifiedImmediate_al_r0_47",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 48},
                                "al r0 48",
                                "ModifiedImmediate_al_r0_48",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 49},
                                "al r0 49",
                                "ModifiedImmediate_al_r0_49",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 50},
                                "al r0 50",
                                "ModifiedImmediate_al_r0_50",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 51},
                                "al r0 51",
                                "ModifiedImmediate_al_r0_51",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 52},
                                "al r0 52",
                                "ModifiedImmediate_al_r0_52",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 53},
                                "al r0 53",
                                "ModifiedImmediate_al_r0_53",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 54},
                                "al r0 54",
                                "ModifiedImmediate_al_r0_54",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 55},
                                "al r0 55",
                                "ModifiedImmediate_al_r0_55",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 56},
                                "al r0 56",
                                "ModifiedImmediate_al_r0_56",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 57},
                                "al r0 57",
                                "ModifiedImmediate_al_r0_57",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 58},
                                "al r0 58",
                                "ModifiedImmediate_al_r0_58",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 59},
                                "al r0 59",
                                "ModifiedImmediate_al_r0_59",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 60},
                                "al r0 60",
                                "ModifiedImmediate_al_r0_60",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 61},
                                "al r0 61",
                                "ModifiedImmediate_al_r0_61",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 62},
                                "al r0 62",
                                "ModifiedImmediate_al_r0_62",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 63},
                                "al r0 63",
                                "ModifiedImmediate_al_r0_63",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 64},
                                "al r0 64",
                                "ModifiedImmediate_al_r0_64",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 65},
                                "al r0 65",
                                "ModifiedImmediate_al_r0_65",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 66},
                                "al r0 66",
                                "ModifiedImmediate_al_r0_66",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 67},
                                "al r0 67",
                                "ModifiedImmediate_al_r0_67",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 68},
                                "al r0 68",
                                "ModifiedImmediate_al_r0_68",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 69},
                                "al r0 69",
                                "ModifiedImmediate_al_r0_69",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 70},
                                "al r0 70",
                                "ModifiedImmediate_al_r0_70",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 71},
                                "al r0 71",
                                "ModifiedImmediate_al_r0_71",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 72},
                                "al r0 72",
                                "ModifiedImmediate_al_r0_72",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 73},
                                "al r0 73",
                                "ModifiedImmediate_al_r0_73",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 74},
                                "al r0 74",
                                "ModifiedImmediate_al_r0_74",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 75},
                                "al r0 75",
                                "ModifiedImmediate_al_r0_75",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 76},
                                "al r0 76",
                                "ModifiedImmediate_al_r0_76",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 77},
                                "al r0 77",
                                "ModifiedImmediate_al_r0_77",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 78},
                                "al r0 78",
                                "ModifiedImmediate_al_r0_78",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 79},
                                "al r0 79",
                                "ModifiedImmediate_al_r0_79",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 80},
                                "al r0 80",
                                "ModifiedImmediate_al_r0_80",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 81},
                                "al r0 81",
                                "ModifiedImmediate_al_r0_81",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 82},
                                "al r0 82",
                                "ModifiedImmediate_al_r0_82",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 83},
                                "al r0 83",
                                "ModifiedImmediate_al_r0_83",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 84},
                                "al r0 84",
                                "ModifiedImmediate_al_r0_84",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 85},
                                "al r0 85",
                                "ModifiedImmediate_al_r0_85",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 86},
                                "al r0 86",
                                "ModifiedImmediate_al_r0_86",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 87},
                                "al r0 87",
                                "ModifiedImmediate_al_r0_87",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 88},
                                "al r0 88",
                                "ModifiedImmediate_al_r0_88",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 89},
                                "al r0 89",
                                "ModifiedImmediate_al_r0_89",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 90},
                                "al r0 90",
                                "ModifiedImmediate_al_r0_90",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 91},
                                "al r0 91",
                                "ModifiedImmediate_al_r0_91",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 92},
                                "al r0 92",
                                "ModifiedImmediate_al_r0_92",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 93},
                                "al r0 93",
                                "ModifiedImmediate_al_r0_93",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 94},
                                "al r0 94",
                                "ModifiedImmediate_al_r0_94",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 95},
                                "al r0 95",
                                "ModifiedImmediate_al_r0_95",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 96},
                                "al r0 96",
                                "ModifiedImmediate_al_r0_96",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 97},
                                "al r0 97",
                                "ModifiedImmediate_al_r0_97",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 98},
                                "al r0 98",
                                "ModifiedImmediate_al_r0_98",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 99},
                                "al r0 99",
                                "ModifiedImmediate_al_r0_99",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 100},
                                "al r0 100",
                                "ModifiedImmediate_al_r0_100",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 101},
                                "al r0 101",
                                "ModifiedImmediate_al_r0_101",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 102},
                                "al r0 102",
                                "ModifiedImmediate_al_r0_102",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 103},
                                "al r0 103",
                                "ModifiedImmediate_al_r0_103",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 104},
                                "al r0 104",
                                "ModifiedImmediate_al_r0_104",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 105},
                                "al r0 105",
                                "ModifiedImmediate_al_r0_105",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 106},
                                "al r0 106",
                                "ModifiedImmediate_al_r0_106",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 107},
                                "al r0 107",
                                "ModifiedImmediate_al_r0_107",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 108},
                                "al r0 108",
                                "ModifiedImmediate_al_r0_108",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 109},
                                "al r0 109",
                                "ModifiedImmediate_al_r0_109",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 110},
                                "al r0 110",
                                "ModifiedImmediate_al_r0_110",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 111},
                                "al r0 111",
                                "ModifiedImmediate_al_r0_111",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 112},
                                "al r0 112",
                                "ModifiedImmediate_al_r0_112",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 113},
                                "al r0 113",
                                "ModifiedImmediate_al_r0_113",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 114},
                                "al r0 114",
                                "ModifiedImmediate_al_r0_114",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 115},
                                "al r0 115",
                                "ModifiedImmediate_al_r0_115",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 116},
                                "al r0 116",
                                "ModifiedImmediate_al_r0_116",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 117},
                                "al r0 117",
                                "ModifiedImmediate_al_r0_117",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 118},
                                "al r0 118",
                                "ModifiedImmediate_al_r0_118",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 119},
                                "al r0 119",
                                "ModifiedImmediate_al_r0_119",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 120},
                                "al r0 120",
                                "ModifiedImmediate_al_r0_120",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 121},
                                "al r0 121",
                                "ModifiedImmediate_al_r0_121",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 122},
                                "al r0 122",
                                "ModifiedImmediate_al_r0_122",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 123},
                                "al r0 123",
                                "ModifiedImmediate_al_r0_123",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 124},
                                "al r0 124",
                                "ModifiedImmediate_al_r0_124",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 125},
                                "al r0 125",
                                "ModifiedImmediate_al_r0_125",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 126},
                                "al r0 126",
                                "ModifiedImmediate_al_r0_126",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 127},
                                "al r0 127",
                                "ModifiedImmediate_al_r0_127",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 128},
                                "al r0 128",
                                "ModifiedImmediate_al_r0_128",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 129},
                                "al r0 129",
                                "ModifiedImmediate_al_r0_129",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 130},
                                "al r0 130",
                                "ModifiedImmediate_al_r0_130",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 131},
                                "al r0 131",
                                "ModifiedImmediate_al_r0_131",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 132},
                                "al r0 132",
                                "ModifiedImmediate_al_r0_132",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 133},
                                "al r0 133",
                                "ModifiedImmediate_al_r0_133",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 134},
                                "al r0 134",
                                "ModifiedImmediate_al_r0_134",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 135},
                                "al r0 135",
                                "ModifiedImmediate_al_r0_135",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 136},
                                "al r0 136",
                                "ModifiedImmediate_al_r0_136",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 137},
                                "al r0 137",
                                "ModifiedImmediate_al_r0_137",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 138},
                                "al r0 138",
                                "ModifiedImmediate_al_r0_138",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 139},
                                "al r0 139",
                                "ModifiedImmediate_al_r0_139",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 140},
                                "al r0 140",
                                "ModifiedImmediate_al_r0_140",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 141},
                                "al r0 141",
                                "ModifiedImmediate_al_r0_141",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 142},
                                "al r0 142",
                                "ModifiedImmediate_al_r0_142",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 143},
                                "al r0 143",
                                "ModifiedImmediate_al_r0_143",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 144},
                                "al r0 144",
                                "ModifiedImmediate_al_r0_144",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 145},
                                "al r0 145",
                                "ModifiedImmediate_al_r0_145",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 146},
                                "al r0 146",
                                "ModifiedImmediate_al_r0_146",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 147},
                                "al r0 147",
                                "ModifiedImmediate_al_r0_147",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 148},
                                "al r0 148",
                                "ModifiedImmediate_al_r0_148",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 149},
                                "al r0 149",
                                "ModifiedImmediate_al_r0_149",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 150},
                                "al r0 150",
                                "ModifiedImmediate_al_r0_150",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 151},
                                "al r0 151",
                                "ModifiedImmediate_al_r0_151",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 152},
                                "al r0 152",
                                "ModifiedImmediate_al_r0_152",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 153},
                                "al r0 153",
                                "ModifiedImmediate_al_r0_153",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 154},
                                "al r0 154",
                                "ModifiedImmediate_al_r0_154",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 155},
                                "al r0 155",
                                "ModifiedImmediate_al_r0_155",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 156},
                                "al r0 156",
                                "ModifiedImmediate_al_r0_156",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 157},
                                "al r0 157",
                                "ModifiedImmediate_al_r0_157",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 158},
                                "al r0 158",
                                "ModifiedImmediate_al_r0_158",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 159},
                                "al r0 159",
                                "ModifiedImmediate_al_r0_159",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 160},
                                "al r0 160",
                                "ModifiedImmediate_al_r0_160",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 161},
                                "al r0 161",
                                "ModifiedImmediate_al_r0_161",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 162},
                                "al r0 162",
                                "ModifiedImmediate_al_r0_162",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 163},
                                "al r0 163",
                                "ModifiedImmediate_al_r0_163",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 164},
                                "al r0 164",
                                "ModifiedImmediate_al_r0_164",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 165},
                                "al r0 165",
                                "ModifiedImmediate_al_r0_165",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 166},
                                "al r0 166",
                                "ModifiedImmediate_al_r0_166",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 167},
                                "al r0 167",
                                "ModifiedImmediate_al_r0_167",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 168},
                                "al r0 168",
                                "ModifiedImmediate_al_r0_168",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 169},
                                "al r0 169",
                                "ModifiedImmediate_al_r0_169",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 170},
                                "al r0 170",
                                "ModifiedImmediate_al_r0_170",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 171},
                                "al r0 171",
                                "ModifiedImmediate_al_r0_171",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 172},
                                "al r0 172",
                                "ModifiedImmediate_al_r0_172",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 173},
                                "al r0 173",
                                "ModifiedImmediate_al_r0_173",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 174},
                                "al r0 174",
                                "ModifiedImmediate_al_r0_174",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 175},
                                "al r0 175",
                                "ModifiedImmediate_al_r0_175",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 176},
                                "al r0 176",
                                "ModifiedImmediate_al_r0_176",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 177},
                                "al r0 177",
                                "ModifiedImmediate_al_r0_177",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 178},
                                "al r0 178",
                                "ModifiedImmediate_al_r0_178",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 179},
                                "al r0 179",
                                "ModifiedImmediate_al_r0_179",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 180},
                                "al r0 180",
                                "ModifiedImmediate_al_r0_180",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 181},
                                "al r0 181",
                                "ModifiedImmediate_al_r0_181",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 182},
                                "al r0 182",
                                "ModifiedImmediate_al_r0_182",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 183},
                                "al r0 183",
                                "ModifiedImmediate_al_r0_183",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 184},
                                "al r0 184",
                                "ModifiedImmediate_al_r0_184",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 185},
                                "al r0 185",
                                "ModifiedImmediate_al_r0_185",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 186},
                                "al r0 186",
                                "ModifiedImmediate_al_r0_186",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 187},
                                "al r0 187",
                                "ModifiedImmediate_al_r0_187",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 188},
                                "al r0 188",
                                "ModifiedImmediate_al_r0_188",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 189},
                                "al r0 189",
                                "ModifiedImmediate_al_r0_189",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 190},
                                "al r0 190",
                                "ModifiedImmediate_al_r0_190",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 191},
                                "al r0 191",
                                "ModifiedImmediate_al_r0_191",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 192},
                                "al r0 192",
                                "ModifiedImmediate_al_r0_192",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 193},
                                "al r0 193",
                                "ModifiedImmediate_al_r0_193",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 194},
                                "al r0 194",
                                "ModifiedImmediate_al_r0_194",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 195},
                                "al r0 195",
                                "ModifiedImmediate_al_r0_195",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 196},
                                "al r0 196",
                                "ModifiedImmediate_al_r0_196",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 197},
                                "al r0 197",
                                "ModifiedImmediate_al_r0_197",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 198},
                                "al r0 198",
                                "ModifiedImmediate_al_r0_198",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 199},
                                "al r0 199",
                                "ModifiedImmediate_al_r0_199",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 200},
                                "al r0 200",
                                "ModifiedImmediate_al_r0_200",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 201},
                                "al r0 201",
                                "ModifiedImmediate_al_r0_201",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 202},
                                "al r0 202",
                                "ModifiedImmediate_al_r0_202",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 203},
                                "al r0 203",
                                "ModifiedImmediate_al_r0_203",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 204},
                                "al r0 204",
                                "ModifiedImmediate_al_r0_204",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 205},
                                "al r0 205",
                                "ModifiedImmediate_al_r0_205",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 206},
                                "al r0 206",
                                "ModifiedImmediate_al_r0_206",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 207},
                                "al r0 207",
                                "ModifiedImmediate_al_r0_207",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 208},
                                "al r0 208",
                                "ModifiedImmediate_al_r0_208",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 209},
                                "al r0 209",
                                "ModifiedImmediate_al_r0_209",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 210},
                                "al r0 210",
                                "ModifiedImmediate_al_r0_210",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 211},
                                "al r0 211",
                                "ModifiedImmediate_al_r0_211",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 212},
                                "al r0 212",
                                "ModifiedImmediate_al_r0_212",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 213},
                                "al r0 213",
                                "ModifiedImmediate_al_r0_213",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 214},
                                "al r0 214",
                                "ModifiedImmediate_al_r0_214",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 215},
                                "al r0 215",
                                "ModifiedImmediate_al_r0_215",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 216},
                                "al r0 216",
                                "ModifiedImmediate_al_r0_216",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 217},
                                "al r0 217",
                                "ModifiedImmediate_al_r0_217",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 218},
                                "al r0 218",
                                "ModifiedImmediate_al_r0_218",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 219},
                                "al r0 219",
                                "ModifiedImmediate_al_r0_219",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 220},
                                "al r0 220",
                                "ModifiedImmediate_al_r0_220",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 221},
                                "al r0 221",
                                "ModifiedImmediate_al_r0_221",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 222},
                                "al r0 222",
                                "ModifiedImmediate_al_r0_222",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 223},
                                "al r0 223",
                                "ModifiedImmediate_al_r0_223",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 224},
                                "al r0 224",
                                "ModifiedImmediate_al_r0_224",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 225},
                                "al r0 225",
                                "ModifiedImmediate_al_r0_225",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 226},
                                "al r0 226",
                                "ModifiedImmediate_al_r0_226",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 227},
                                "al r0 227",
                                "ModifiedImmediate_al_r0_227",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 228},
                                "al r0 228",
                                "ModifiedImmediate_al_r0_228",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 229},
                                "al r0 229",
                                "ModifiedImmediate_al_r0_229",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 230},
                                "al r0 230",
                                "ModifiedImmediate_al_r0_230",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 231},
                                "al r0 231",
                                "ModifiedImmediate_al_r0_231",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 232},
                                "al r0 232",
                                "ModifiedImmediate_al_r0_232",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 233},
                                "al r0 233",
                                "ModifiedImmediate_al_r0_233",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 234},
                                "al r0 234",
                                "ModifiedImmediate_al_r0_234",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 235},
                                "al r0 235",
                                "ModifiedImmediate_al_r0_235",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 236},
                                "al r0 236",
                                "ModifiedImmediate_al_r0_236",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 237},
                                "al r0 237",
                                "ModifiedImmediate_al_r0_237",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 238},
                                "al r0 238",
                                "ModifiedImmediate_al_r0_238",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 239},
                                "al r0 239",
                                "ModifiedImmediate_al_r0_239",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 240},
                                "al r0 240",
                                "ModifiedImmediate_al_r0_240",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 241},
                                "al r0 241",
                                "ModifiedImmediate_al_r0_241",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 242},
                                "al r0 242",
                                "ModifiedImmediate_al_r0_242",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 243},
                                "al r0 243",
                                "ModifiedImmediate_al_r0_243",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 244},
                                "al r0 244",
                                "ModifiedImmediate_al_r0_244",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 245},
                                "al r0 245",
                                "ModifiedImmediate_al_r0_245",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 246},
                                "al r0 246",
                                "ModifiedImmediate_al_r0_246",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 247},
                                "al r0 247",
                                "ModifiedImmediate_al_r0_247",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 248},
                                "al r0 248",
                                "ModifiedImmediate_al_r0_248",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 249},
                                "al r0 249",
                                "ModifiedImmediate_al_r0_249",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 250},
                                "al r0 250",
                                "ModifiedImmediate_al_r0_250",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 251},
                                "al r0 251",
                                "ModifiedImmediate_al_r0_251",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 252},
                                "al r0 252",
                                "ModifiedImmediate_al_r0_252",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 253},
                                "al r0 253",
                                "ModifiedImmediate_al_r0_253",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 254},
                                "al r0 254",
                                "ModifiedImmediate_al_r0_254",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate},
                               {{al, r0, 255},
                                "al r0 255",
                                "ModifiedImmediate_al_r0_255",
                                ARRAY_SIZE(kModifiedImmediate),
                                kModifiedImmediate}};

// We record all inputs to the instructions as outputs. This way, we also check
// that what shouldn't change didn't change.
struct TestResult {
  size_t output_size;
  const Inputs* outputs;
};

// These headers each contain an array of `TestResult` with the reference output
// values. The reference arrays are names `kReference{mnemonic}`.
#include "aarch32/traces/simulator-cond-rdlow-operand-imm8-cmp-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-operand-imm8-mov-t32.h"
#include "aarch32/traces/simulator-cond-rdlow-operand-imm8-movs-t32.h"


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
    int32_t immediate = kTests[i].operands.immediate;
    Operand op(immediate);
    scratch_registers.Exclude(rd);

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
        uint32_t apsr_input = kTests[i].inputs[j].apsr;
        uint32_t rd_input = kTests[i].inputs[j].rd;
        uint32_t apsr_ref = reference[i].outputs[j].apsr;
        uint32_t rd_ref = reference[i].outputs[j].rd;

        if (((apsr != apsr_ref) || (rd != rd_ref)) &&
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
          printf("\n");
          printf("  Expected: ");
          printf("0x%08" PRIx32, apsr_ref);
          printf(", ");
          printf("0x%08" PRIx32, rd_ref);
          printf("\n");
          printf("  Found:    ");
          printf("0x%08" PRIx32, apsr);
          printf(", ");
          printf("0x%08" PRIx32, rd);
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
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RDLOW_OPERAND_IMM8_" #mnemonic \
                       "_T32",                                                \
                       &Test_##mnemonic);
#else
#define TEST(mnemonic)                                                        \
  void Test_##mnemonic() {                                                    \
    VIXL_WARNING("This test can only run on a 32-bit host.\n");               \
    USE(TestHelper);                                                          \
  }                                                                           \
  Test test_##mnemonic("AARCH32_SIMULATOR_COND_RDLOW_OPERAND_IMM8_" #mnemonic \
                       "_T32",                                                \
                       &Test_##mnemonic);
#endif

FOREACH_INSTRUCTION(TEST)
#undef TEST

}  // namespace
#endif

}  // namespace aarch32
}  // namespace vixl
