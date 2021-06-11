#!/usr/bin/env python3

# Copyright 2016, VIXL authors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of ARM Limited nor the names of its contributors may be
#     used to endorse or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
Generating tests
================

From the VIXL toplevel directory run:

    $ ./tools/generate_tests.py

The script assumes that `clang-format-4.0` is in the current path. If it isn't,
you can provide your own:

    $ ./tools/generate_tests.py --clang-format /patch/to/clang-format

Once the script has finished, it will have generated test files, as many as
present in the `default_config_files` list. For example:

- test/aarch32/test-assembler-cond-rd-rn-immediate-a32.cc
- test/aarch32/test-assembler-cond-rd-rn-rm-a32.cc
- test/aarch32/test-assembler-cond-rd-rn-rm-q-a32.cc
- test/aarch32/test-assembler-cond-rd-rn-rm-ge-a32.cc

Because these test cases need traces in order to build, the script will have
generated placeholder trace files in `test/aarch32/traces/`. If you look at them
you'll see they are basically empty:

    $ cat test/aarch32/traces/sim-cond-rd-rn-immediate-adc-a32.h
    static const TestResult *kReferenceAdc = NULL;

So of course, we can now build the test cases but running them will crash. We
need to re-generate traces with real hardware; the test cases do not support
running in the simulator just yet.

Generating traces
=================

You need to have either compiled natively for ARM, or cross-compiled
`test-runner`. The traces can then be generated in the same way as with VIXL64.
Note that it takes a few minutes to generate everything.

    ./tools/generate_simulator_traces.py --runner /path/to/test-runner \
      --aarch32-only

You can now rebuild everything. If it all goes well, running the new tests
should pass.

Test configuration format
=========================

TODO: Write a simple and well documented complete example configuration file and
      mention it here.

The underlying `test_generator` framework reads JSON description files and
generates tests according to them. These files live in `test/aarch32/config` by
default, but you may provide your own files with the `--config-files FILE ...`
flag. The JSON format was extended to support C++ like one-line comments.

Each configuration file will serve to generate one or more test files,
we even use its file name to choose the name of the test:

    test/aarch32/config/cond-rd-rn-immediate-a32.json
    `-> test/aarch32/test-simulator-cond-rd-rn-immediate-a32.cc
    `-> test/aarch32/test-assembler-cond-rd-rn-immediate-a32.cc

In addition to these test configuration files, we also provide a JSON
description with shared information. This information represents data types that
instructions use and lives in `test/aarch32/config/data-types.json`.

Data types description
----------------------

We refer to two kinds of data types: `operand` and `input`.

An `operand` represents an argument passed to the macro-assembler to generate an
instruction. For example, a register or an immediate are operands. We can think
of it as "assemble-time" data.

As opposed to `operands`, an `input` represents data passed to an instruction at
runtime. For example, it will be the value you write to a register before
executing the instruction under test.

The `data-types.json` file has the following structure:

~~~
{
  "operands": [
    // List of operand types.
  ],
  "inputs": [
    // List of input types.
  ]
}
~~~

Each operand is described with the following structure:

~~~
{
  // Unique name for this operand type.
  "name": "AllRegistersButPC",
  // C++ type used by VIXL to represent this operand.
  "type": "Register",
  // List of possible variants.
  "variants": [
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14"
  ],
  // Default variant to use.
  "default": "r0"
}
~~~

The "name" field of the operand will be used by test configuration files in
order to specify what kind of operands an instruction takes. The "type" field
simply tells the generator what C++ type should be generated, e.g. "Condition",
"Register", "uint32_t", "ShiftType", ...etc.

Inputs are described in a very similar way:

~~~
{
  // Unique name for this input type.
  "name": "Register",
  // Python type from `test_generator.data_types` to use to generate C++ code
  // for this input.
  "type": "Register",
  // List of possible values.
  "values": [
    "0x00000000",
    "0xffffffff",
    "0xabababab",
    "0x5a5a5a5a"
  ],
  // Default value.
  "default": "0xabababab"
}
~~~

The "name" field has the same purpose as for operands. The "type" field however,
is the name of a Python class in `test_generator.data_types`. The type will
specify what C++ code to generate in order to load and record the input value,
e.g. how to load a value into a register, how to read and record it.

When adding more tests, one may have to create new data types in this file. For
example, when we want to test an instruction with a different set of registers.
If adding new input types which need different C++ code to load and record them,
one will have to add it to `test_generator.data_types` and override the
`Epilogue` and `Prologue` methods.

Test configuration
------------------

Once we have all the data types we need described, we need test configuration
files to describe what instructions to test and with what `inputs` and
`operands` they take.

These files have the following structure:

~~~
{
  "mnemonics": [
    // List of instruction mnemonics to use. These must correspond to
    // `MacroAssembler` methods.
  ],
  "description": {
    "operands": [
      // List of operands the instruction takes.
    ],
    "inputs: [
      // List of inputs the instruction can be affected by.
    ]
  },
  // List of files to generate.
  "test-files": [
    {
      "type": "assembler",
      "mnemonics": [
        // Optional list of instruction mnemonics to use, overriding the
        // top-level list.
      ],
      "test-cases": [
        // List of test cases for "assembler" tests, see below for
        // details.
      ]
    },
    {
      "type": "simulator",
      "test-cases": [
        // List of test cases for "simulator" tests, see below for
        // details.
      ]
    }
  ]
}
~~~

- List of operands:

The operand list describes the actual argument to the `MacroAssembler` method.
For example, if we take instruction in the form
"XXX.cond rd rn rm shift #amount":

We want to generate C++ code as such:

~~~
Condition cond = ...;
Register rd = ...;
Register rn = ...;
Register rm = ...;
ShiftType type = ...;
uint32_t amount = ...;
Operand op(rm, type, amount);

__ Xxx(cond, rd, rn, op);
~~~

We will have the following operand list:

~~~
"operands": [
  {
    "name": "cond",
    "type": "Condition"
  },
  {
    "name": "rd",
    "type": "AllRegistersButPC"
  },
  {
    "name": "rn",
    "type": "AllRegistersButPC"
  },
  {
    "name": "op",
    "wrapper": "Operand",
    "operands": [
      {
        "name": "rm",
        "operand": "AllRegistersButPC"
      },
      {
        "name": "type",
        "operand": "Shift"
      },
      {
        "name": "amount",
        "operand": "ImmediateShiftAmount"
      }
    ]
  }
]
~~~

The "name" field represents the identifier of the operand and will be used as a
variable name in the generated code. The "type" field corresponds to an operand
type described in the `data-types.json` file as described above.

We can see that we've wrapped the last three operands into an "op"
wrapper object. This allows us to tell the generator to wrap these
operands into a `Operand` C++ object.

- List of inputs:

This structure is similar to the operand list, but this time it describes what
input data the instructions may be affected by at runtime. If we take the same
example as above, we will have the following list:

~~~
"inputs": [
  {
    "name": "apsr",
    "type": "NZCV"
  },
  {
    "name": "rd",
    "type": "Register"
  },
  {
    "name": "rn",
    "type": "Register"
  },
  {
    "name": "rm",
    "type": "Register"
  }
]
~~~

This will specify what C++ code to generate before and after emitting the
instruction under test. The C++ code will set and record register values for
example. See `test_generator.data_types` for more details.

- Test files and test cases:

Up until now, we've only just described the environment in which instructions
can operate. We need to express what files we want generating, what instructions
we want to test and what we want them to do.

As previously mentioned, a configuration file can control the generation of
several test files. We will generate one file per element in the "test-files"
array:

~~~
"test-files": [
  {
    "type": "assembler",
    "test-cases": [
      // List of test cases for "assembler" tests, see below for
      // details.
    ]
  },
  {
    "type": "assembler",
    "name": "special-case",
    "mnemonics": [
      // Override the top-level list with a subset of instructions concerned
      // with this special case.
    ],
    "test-cases": [
      // List of test cases for "assembler" tests, see below for
      // details.
    ]
  },
  {
    "type": "simulator",
    "test-cases": [
      // List of test cases for "simulator" tests, see below for
      // details.
    ]
  }
]
~~~

Above, we've decided to generate three tests: a "simulator" test and two
"assembler" tests. The resulting files will have names with the following
pattern.

  - "test/aarch32/test-assembler-{configuration name}-a32.cc"
  - "test/aarch32/test-assembler-{configuration name}-special-case-a32.cc"
  - "test/aarch32/test-simulator-{configuration name}-a32.cc"

The "type" field describes the kind of testing we want to do, these types are
recognized by the generator and, at the moment, can be one of "simulator",
"assembler", "macro-assembler" and "assembler-negative". Simulator tests will
run each instruction and record the changes while assembler tests will only
record the code buffer and never execute anything. MacroAssembler tests
currently only generate code to check that the MacroAssembler does not crash;
the output itself is not yet tested. Because you may want to generate more than
one test of the same type, as we are doing in the example, we need a way to
differentiate them. You may use the optional "name" field for this. Negative
assembler tests check that the instructions described are not allowed, which
means that an exception is raised when VIXL is built in negative testing mode.

Finally, we describe how to test the instruction by declaring a list of test
cases with the "test-cases" field.

Here is an example of what we can express:
~~~
[
  // Generate all combinations of instructions where "rd" an "rn" are the same
  // register and "cond" and "rm" are just the default.
  // For example:
  //   __ Xxx(al, r0, r0, r0);
  //   __ Xxx(al, r1, r1, r0);
  //   __ Xxx(al, r2, r2, r0);
  //   ...
  //   __ Xxx(al, r12, r12, r0);
  //   __ Xxx(al, r13, r13, r0);
  //   __ Xxx(al, r14, r14, r0);
  //
  // For each of the instructions above, run them with a different value in "rd"
  // and "rn".
  {
    "name": "RdIsRn",
    "operands": [
      "rd", "rn"
    ],
    "operand-filter": "rd == rn",
    "inputs": [
      "rd", "rn"
    ],
    "input-filter": "rd == rn"
  },
  // Generate all combinations of instructions with different condition codes.
  // For example:
  //   __ Xxx(eq, r0, r0, r0);
  //   __ Xxx(ne, r0, r0, r0);
  //   __ Xxx(cs, r0, r0, r0);
  //   ...
  //   __ Xxx(gt, r0, r0, r0);
  //   __ Xxx(le, r0, r0, r0);
  //   __ Xxx(al, r0, r0, r0);
  //
  // For each of the instructions above, run them against all combinations of
  // NZCV bits.
  {
    "name": "ConditionVersusNZCV",
    "operands": [
      "cond"
    ],
    "inputs": [
      "apsr"
    ]
  },
  // We are interested in testing that the Q bit gets set and cleared, so we've
  // limited the instruction generation to a single instruction and instead have
  // stressed the values put in "rn" and "rm".
  //
  // So for this instruction, we choose to run it will all combinations of
  // values in "rn" and "rm". Additionally, we include "qbit" in the inputs,
  // which will make the test set or clear it before executing the instruction.
  // Note that "qbit" needs to be declared as an input in the instruction
  // description (see "List of inputs" section).
  {
    "name": "Qbit",
    "operands": [
      "rn", "rm"
    ],
    "inputs": [
      "qbit", "rn", "rm"
    ],
    "operand-filter": "rn != rm'",
    "operand-limit": 1
  },
  // Generate 10 random instructions with all different registers but use the
  // default condition.
  // For example:
  //   __ Xxx(al, r5, r1, r0);
  //   __ Xxx(al, r8, r9, r7);
  //   __ Xxx(al, r9, r1, r2);
  //   __ Xxx(al, r0, r6, r2);
  //   __ Xxx(al, r11, r9, r11);
  //   __ Xxx(al, r14, r2, r11);
  //   __ Xxx(al, r8, r2, r5);
  //   __ Xxx(al, r10, r0, r1);
  //   __ Xxx(al, r11, r2, r7);
  //   __ Xxx(al, r2, r6, r1);
  //
  // For each instruction, feed it 200 different combination of values in the
  // three registers.
  {
    "name": "RegisterSimulatorTest",
    "operands": [
      "rd", "rn", "rm"
    ],
    "inputs": [
      "rd", "rn", "rm"
    ],
    "operand-limit": 10,
    "input-limit": 200
  }
]
~~~

Assembler test cases are much simpler, here are some examples:
~~~
// Generate 2000 random instructions out of all possible operand combinations.
{
  "name": "LotsOfRandomInstructions",
  "operands": [
    "cond", "rd", "rn", "rm"
  ],
  "operand-limit": 2000
},
// Same as above but limit the test to 200 instructions where rd == rn.
{
  "name": "RdIsRn",
  "operands": [
    "cond", "rd", "rn", "rm"
  ],
  "operand-filter": "rd == rn",
  "operand-limit": 200
}
~~~

As can be expected, assembler test do not have the notion of "inputs".

Here are details about each field. Note that all of them except for "name" are
optional.

  * "name":

    A unique name should be given to the test case, it will be used to give the
    generated C++ `const Input[]` array a name.

  * "operands":

    List of operand names that we are interested in testing. The generator will
    lookup the list of variants for each operand and build the product of all of
    them. It will then choose the default variant for the operands not specified
    here.

  * "operand-filter":

    As you would expect, the product of all operand variants may be huge. To
    prevent this, you may specify a Python expression to filter the list.

  * "operand-limit":

    We can potentially obtain a *massive* set of variants of instructions, as we
    are computing a product of operand variants in "operands". This field allows
    us to limit this by choosing a random sample from the computed variants.
    Note that this is a seeded pseudo-random sample, and the seed corresponds to
    the test case description. The same test case description will always
    generate the same code.

  * "inputs":

    This is exactly the same as "operands" but for inputs.

  * "input-filter":

    Ditto.

  * "input-limit":

    Ditto.

Here is an example of the C++ code that will be generated for a given test case.
For simplicity, let's generate tests for an instruction with only `NZCV` and two
registers as inputs.

For the following test case, which will target encodings where `rd` and `rn` are
the same registers:

~~~
{
  "name": "RdIsRn",
  "operands": [
    "rd", "rn"
  ],
  "operand-filter": "rd == rn",
  "inputs": [
    "rd", "rn"
  ],
  "input-filter": "rd == rn"
},
~~~

It will generate the following input array.

~~~
//                                apsr,        rd,         rn
static const Inputs kRdIsRn[] = {{NoFlag, 0x00000000, 0x00000000},
                                 {NoFlag, 0xffffffff, 0xffffffff},
                                 {NoFlag, 0xabababab, 0xabababab},
                                 {NoFlag, 0x5a5a5a5a, 0x5a5a5a5a}};
~~~

We can see that the default apsr value was chosen (NoFlag), as apsr is not in
the list of "inputs".

It will also generate a list of instructions to test:

~~~
static const TestLoopData kTests[] = {
    {{al, r1, r1, 0x000000ab}, ARRAY_SIZE(kRdIsRn), kRdIsRn, "RdIsRn"},
    {{al, r2, r2, 0x000000ab}, ARRAY_SIZE(kRdIsRn), kRdIsRn, "RdIsRn"},
    {{al, r8, r8, 0x000000ab}, ARRAY_SIZE(kRdIsRn), kRdIsRn, "RdIsRn"},
    {{al, r9, r9, 0x000000ab}, ARRAY_SIZE(kRdIsRn), kRdIsRn, "RdIsRn"},
};
~~~

As a result, the new test we will assemble each instructions in "mnemonics" with
all of the operands described in `kTests` above. And each instruction will be
executed and passed all inputs in `kRdIsRn`.
"""

import subprocess
import argparse
import string
import re
import multiprocessing
import functools

import test_generator.parser


default_config_files = [
    # A32 and T32 tests
    'test/aarch32/config/rd-rn-rm.json',
    'test/aarch32/config/cond-dt-drt-drd-drn-drm-float.json',

    # A32 specific tests
    'test/aarch32/config/cond-rd-rn-operand-const-a32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-a32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-shift-amount-1to31-a32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-shift-amount-1to32-a32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-shift-rs-a32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-ror-amount-a32.json',
    'test/aarch32/config/cond-rd-rn-a32.json',
    'test/aarch32/config/cond-rd-rn-pc-a32.json',
    'test/aarch32/config/cond-rd-rn-rm-a32.json',
    'test/aarch32/config/cond-rd-operand-const-a32.json',
    'test/aarch32/config/cond-rd-operand-rn-a32.json',
    'test/aarch32/config/cond-rd-operand-rn-shift-amount-1to31-a32.json',
    'test/aarch32/config/cond-rd-operand-rn-shift-amount-1to32-a32.json',
    'test/aarch32/config/cond-rd-operand-rn-shift-rs-a32.json',
    'test/aarch32/config/cond-rd-operand-rn-ror-amount-a32.json',
    'test/aarch32/config/cond-rd-memop-immediate-512-a32.json',
    'test/aarch32/config/cond-rd-memop-immediate-8192-a32.json',
    'test/aarch32/config/cond-rd-memop-rs-a32.json',
    'test/aarch32/config/cond-rd-memop-rs-shift-amount-1to31-a32.json',
    'test/aarch32/config/cond-rd-memop-rs-shift-amount-1to32-a32.json',

    # T32 specific tests
    'test/aarch32/config/cond-rd-rn-t32.json',
    'test/aarch32/config/cond-rd-rn-rm-t32.json',
    'test/aarch32/config/cond-rdlow-rnlow-rmlow-t32.json',
    'test/aarch32/config/cond-rd-rn-operand-const-t32.json',
    'test/aarch32/config/cond-rd-pc-operand-imm12-t32.json',
    'test/aarch32/config/cond-rd-rn-operand-imm12-t32.json',
    'test/aarch32/config/cond-rd-pc-operand-imm8-t32.json',
    'test/aarch32/config/cond-rd-sp-operand-imm8-t32.json',
    'test/aarch32/config/cond-rdlow-rnlow-operand-immediate-t32.json',
    'test/aarch32/config/cond-sp-sp-operand-imm7-t32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-t32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-shift-amount-1to31-t32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-shift-amount-1to32-t32.json',
    'test/aarch32/config/cond-rd-rn-operand-rm-ror-amount-t32.json',
    'test/aarch32/config/cond-rd-operand-const-t32.json',
    'test/aarch32/config/cond-rd-operand-imm16-t32.json',
    'test/aarch32/config/cond-rdlow-operand-imm8-t32.json',
    'test/aarch32/config/cond-rd-operand-rn-shift-amount-1to31-t32.json',
    'test/aarch32/config/cond-rd-operand-rn-shift-amount-1to32-t32.json',
    'test/aarch32/config/cond-rd-operand-rn-shift-rs-t32.json',
    'test/aarch32/config/cond-rd-operand-rn-ror-amount-t32.json',
    'test/aarch32/config/cond-rd-operand-rn-t32.json',
]


# Link a test type with a template file.
template_files = {
    'simulator': "test/aarch32/config/template-simulator-aarch32.cc.in",
    'assembler': "test/aarch32/config/template-assembler-aarch32.cc.in",
    'macro-assembler': "test/aarch32/config/template-macro-assembler-aarch32.cc.in",
    'assembler-negative': "test/aarch32/config/template-assembler-negative-aarch32.cc.in",
}


def BuildOptions():
  result = argparse.ArgumentParser(
      description = 'Test generator for AArch32.',
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  result.add_argument('--config-files', nargs='+',
                      default=default_config_files,
                      metavar='FILE',
                      help='Configuration files, each will generate a test file.')
  result.add_argument('--clang-format',
                      default='clang-format-4.0', help='Path to clang-format.')
  result.add_argument('--jobs', '-j', type=int, metavar='N',
                      default=multiprocessing.cpu_count(),
                      help='Allow N jobs at once')
  result.add_argument('--skip-traces', action='store_true',
                      help='Skip generation of placeholder traces.')
  return result.parse_args()


def DoNotEditComment(template_file):
  # We rely on `clang-format` to wrap this comment to 80 characters.
  return """
// -----------------------------------------------------------------------------
// This file is auto generated from the {} template file using tools/generate_tests.py.
//
// PLEASE DO NOT EDIT.
// -----------------------------------------------------------------------------
    """.format(template_file)

def GenerateTest(generator, clang_format, skip_traces):
  template_file = template_files[generator.test_type]
  generated_file = ""
  with open(template_file, "r") as f:
    # Strip out comments starting with three forward slashes before creating the
    # string.Template object.
    template = string.Template(re.sub("\/\/\/.*", "", f.read()))

    # The `generator` object has methods generating strings to fill the template.
    generated_file = template.substitute({
      # Add a top comment stating this file is auto-generated.
      'do_not_edit_comment': DoNotEditComment(template_file),

      # List of mnemonics.
      'instruction_list_declaration': generator.InstructionListDeclaration(),

      # Declarations.
      'operand_list_declaration': generator.OperandDeclarations(),
      'input_declarations': generator.InputDeclarations(),

      # Definitions.
      'input_definitions': generator.InputDefinitions(),
      'test_case_definitions': generator.TestCaseDefinitions(),

      # Include traces.
      'include_trace_files': generator.IncludeTraceFiles(),

      # Define a typedef for the MacroAssembler method.
      'macroassembler_method_args': generator.MacroAssemblerMethodArgs(),

      # Generate code to switch instruction set.
      'macroassembler_set_isa': generator.MacroAssemblerSetISA(),

      # Generate code to emit instructions.
      'code_instantiate_operands': generator.CodeInstantiateOperands(),
      'code_prologue': generator.CodePrologue(),
      'code_epilogue': generator.CodeEpilogue(),
      'code_parameter_list': generator.CodeParameterList(),

      # Generate code to trace the execution and print C++.
      'trace_print_outputs': generator.TracePrintOutputs(),

      # Generate code to compare the results against a trace.
      'check_instantiate_results': generator.CheckInstantiateResults(),
      'check_instantiate_inputs': generator.CheckInstantiateInputs(),
      'check_instantiate_references': generator.CheckInstantiateReferences(),
      'check_results_against_references':
          generator.CheckResultsAgainstReferences(),
      'check_print_input': generator.CheckPrintInput(),
      'check_print_expected': generator.CheckPrintExpected(),
      'check_print_found': generator.CheckPrintFound(),

      'test_isa': generator.TestISA(),
      'test_name': generator.TestName(),
      'isa_guard': generator.GetIsaGuard()
    })
  # Create the test case and pipe it through `clang-format` before writing it.
  with open(
      "test/aarch32/test-{}-{}-{}.cc".format(generator.test_type,
                                             generator.test_name,
                                             generator.test_isa),
      "w") as f:
    proc = subprocess.Popen([clang_format], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE)
    out, _ = proc.communicate(generated_file.encode())
    f.write(out.decode())
  if not skip_traces:
    # Write placeholder trace files into 'test/aarch32/traces/'.
    generator.WriteEmptyTraces("test/aarch32/traces/")
  print("Generated {} {} test for \"{}\".".format(generator.test_isa.upper(),
                                                  generator.test_type,
                                                  generator.test_name))

if __name__ == '__main__':
  args = BuildOptions()

  # Each file in `args.config_files` populates a `Generator` object.
  generators = test_generator.parser.Parse('test/aarch32/config/data-types.json',
                                           args.config_files)

  # Call the `GenerateTest` function for each generator object in parallel. This
  # will use as many processes as defined by `-jN`, which defaults to 1.
  with multiprocessing.Pool(processes=args.jobs) as pool:
    pool.map(functools.partial(GenerateTest, clang_format=args.clang_format,
                                             skip_traces=args.skip_traces),
             generators)
