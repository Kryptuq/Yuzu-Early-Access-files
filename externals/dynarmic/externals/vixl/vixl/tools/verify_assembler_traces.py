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
Verify generated AArch32 assembler traces against `llvm-mc`.

This script will find all files in `test/aarch32/traces/` with names starting
will `assembler`, and check them against `llvm-mc`. It checks our assembler is
correct by looking up what instruction we meant to asssemble, assemble it with
`llvm` and check the result is bit identical to what our assembler generated.

You may run the script with no arguments from VIXL's top-level directory as long
as `llvm-mc` is in your PATH. You may provide a different `llvm-mc` path with
the `--llvm-mc` option. This script relies on version 3.8 or higher of
LLVM. Previous versions refuse to assemble some instructions that ARMv8 allows,
but ARMv7 did not.

For example, let's say we have the following assembler trace for CLZ
(the real trace is a lot bigger):

~~~
static const byte kInstruction_Clz_eq_r0_r0[] = {
  0x10, 0x0f, 0x6f, 0x01 // Clz eq r0 r0
};
static const byte kInstruction_Clz_eq_r0_r1[] = {
  0x11, 0x0f, 0x6f, 0x01 // Clz eq r0 r1
};
static const byte kInstruction_Clz_eq_r0_r2[] = {
  0x12, 0x0f, 0x6f, 0x01 // Clz eq r0 r2
};
static const TestResult kReferenceClz[] = {
  {
    ARRAY_SIZE(kInstruction_Clz_eq_r0_r0),
    kInstruction_Clz_eq_r0_r0,
  },
  {
    ARRAY_SIZE(kInstruction_Clz_eq_r0_r1),
    kInstruction_Clz_eq_r0_r1,
  },
  {
    ARRAY_SIZE(kInstruction_Clz_eq_r0_r2),
    kInstruction_Clz_eq_r0_r2,
  },
};
~~~

The traces contain both the list of bytes that were encoded as well as a comment
with a description of the instruction this is. This script searches for these
lines and checks them.

With our example, the script will find the following:

    [
      ("Clz eq r0 r0", ["0x10", "0x0f", "0x6f", "0x01"]),
      ("Clz eq r0 r1", ["0x11", "0x0f", "0x6f", "0x01"]),
      ("Clz eq r0 r2", ["0x12", "0x0f", "0x6f", "0x01"])
    ]

Then the tricky part is to convert the description of the instruction into the
following valid assembly syntax:

    clzeq r0, r0
    clzeq r0, r1
    clzeq r0, r2

Our example is easy, but it gets more complicated with load and store
instructions for example. We can feed this as input to `llvm-mc`:

    $ echo "
      clzeq r0, r0
      clzeq r0, r1
      clzeq r0, r2
    " | llvm-mc -assemble -arch=arm -mattr=v8,crc -show-encoding

And we will get the following output:

            .text
            clzeq   r0, r0                  @ encoding: [0x10,0x0f,0x6f,0x01]
            clzeq   r0, r1                  @ encoding: [0x11,0x0f,0x6f,0x01]
            clzeq   r0, r2                  @ encoding: [0x12,0x0f,0x6f,0x01]

The script will finally extract the encoding and compare it to what VIXL
generated.
"""

import argparse
import subprocess
import os
import re
import itertools
import types

def BuildOptions():
  result = argparse.ArgumentParser(
      description = 'Use `llvm-mc` to check the assembler traces are correct.',
      formatter_class = argparse.ArgumentDefaultsHelpFormatter)
  result.add_argument('--llvm-mc', default='llvm-mc', help='Path to llvm-mc')
  result.add_argument('--verbose', '-v', action='store_true')
  return result.parse_args()


def CheckLLVMVersion(llvm_mc):
  version = subprocess.check_output([llvm_mc, '-version'])
  m = re.search("^  LLVM version (\d)\.(\d)\.\d$", version.decode(), re.M)
  major, minor = m.groups()
  if int(major) < 3 or (int(major) == 3 and int(minor) < 8):
    raise Exception("This script requires LLVM version 3.8 or higher.")


def ConvertToLLVMFormat(vixl_instruction, triple):
  """
  Take an string representing an instruction and convert it to assembly syntax
  for LLVM. VIXL's test generation framework will print instruction
  representations as a space seperated list. The first element is the mnemonic
  and the following elements are operands.
  """

  def DtUntypedToLLVM(matches):
    dt = ""
    if matches[1] == "untyped8":
      dt = "8"
    elif matches[1] == "untyped16":
      dt = "16"
    elif matches[1] == "untyped32":
      dt = "32"
    else:
      raise Exception()

    return "{}.{} {}, {}, {}".format(matches[0], dt, matches[2], matches[3], matches[4])

  # Dictionnary of patterns. The key is an identifier used in
  # `llvm_mc_instruction_converters` below. The value needs to be a capturing
  # regular expression.
  pattern_matchers = {
      # Allow an optional underscore in case this an "and" instruction.
      "mnemonic": "(\w+?)_?",
      "condition":
          "(al|eq|ne|cs|cc|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)",
      "register":
          "(r0|r1|r2|r3|r4|r5|r6|r7|r8|r9|r10|r11|r12|r13|r14|r15|pc|sp|lr)",
      "immediate": "(0x[0-9a-f]+|[0-9]+)",
      "shift": "(lsl|lsr|asr|ror)",
      "dregister": "(d[0-9]|d[12][0-9]|d3[01])",
      "dt": "(s8|s16|s32|s64|u8|u16|u32|u64|f16|f32|f64|i8|i16|i32|i64|p8|p64)",
      "dt_untyped": "(untyped8|untyped16|untyped32)"
  }

  # List of converters. Each of them represents an instruction form and what to
  # convert it to. This list needs to be complete; an exception is raised if we
  # couldn't find a converter for the instruction.
  #
  # The first part of each tuple is a pattern to match. It's simply a regular
  # expression. Additionally, each identifier in curly braces is replaced by the
  # corresponding pattern from `pattern_matchers`.
  #
  # The second part of the tuple is a string that describes what the result will
  # look like. Empty curly braces are replaced by matches, in order.
  llvm_mc_instruction_converters = [
      ("it {condition}", "it {}"),
      ("{mnemonic} {condition} {register} {immediate}",
       "{}{} {}, #{}"),
      ("{mnemonic} {condition} {register} {register} {immediate}",
       "{}{} {}, {}, #{}"),
      ("{mnemonic} {condition} {register} {register}",
       "{}{} {}, {}"),
      ("{mnemonic} {condition} {register} {register} {register}",
       "{}{} {}, {}, {}"),
      ("{mnemonic} {register} {register} {register}",
       "{} {}, {}, {}"),
      ("{mnemonic} {condition} {register} {register} {immediate}",
       "{}{} {}, {}, #{}"),
      ("{mnemonic} {condition} {register} {register} {register} {shift} "
           "{immediate}",
       "{}{} {}, {}, {}, {} #{}"),
      ("{mnemonic} {condition} {register} {register} {register} {shift} "
           "{register}",
       "{}{} {}, {}, {}, {} {}"),
      ("{mnemonic} {condition} {register} {register} {shift} {immediate}",
       "{}{} {}, {}, {} #{}"),
      ("{mnemonic} {condition} {register} {register} {shift} {register}",
       "{}{} {}, {}, {} {}"),
      ("{mnemonic} {condition} {register} {register} plus {immediate} offset",
       "{}{} {}, [{}, #{}]"),
      ("{mnemonic} {condition} {register} {register} minus {immediate} offset",
       "{}{} {}, [{}, #-{}]"),
      ("{mnemonic} {condition} {register} {register} plus {immediate} postindex",
       "{}{} {}, [{}], #{}"),
      ("{mnemonic} {condition} {register} {register} minus {immediate} "
           "postindex",
       "{}{} {}, [{}], #-{}"),
      ("{mnemonic} {condition} {register} {register} plus {immediate} preindex",
       "{}{} {}, [{}, #{}]!"),
      ("{mnemonic} {condition} {register} {register} minus {immediate} "
           "preindex",
       "{}{} {}, [{}, #-{}]!"),
      ("{mnemonic} {condition} {register} {register} plus {register} offset",
       "{}{} {}, [{}, {}]"),
      ("{mnemonic} {condition} {register} {register} minus {register} offset",
       "{}{} {}, [{}, -{}]"),
      ("{mnemonic} {condition} {register} {register} plus {register} postindex",
       "{}{} {}, [{}], {}"),
      ("{mnemonic} {condition} {register} {register} minus {register} "
           "postindex",
       "{}{} {}, [{}], -{}"),
      ("{mnemonic} {condition} {register} {register} plus {register} preindex",
       "{}{} {}, [{}, {}]!"),
      ("{mnemonic} {condition} {register} {register} minus {register} preindex",
       "{}{} {}, [{}, -{}]!"),
      ("{mnemonic} {condition} {register} {register} plus {register} {shift} "
           "{immediate} offset",
       "{}{} {}, [{}, {}, {} #{}]"),
      ("{mnemonic} {condition} {register} {register} minus {register} {shift} "
           "{immediate} offset",
       "{}{} {}, [{}, -{}, {} #{}]"),
      ("{mnemonic} {condition} {register} {register} plus {register} {shift} "
           "{immediate} postindex",
       "{}{} {}, [{}], {}, {} #{}"),
      ("{mnemonic} {condition} {register} {register} minus {register} {shift} "
           "{immediate} postindex",
       "{}{} {}, [{}], -{}, {} #{}"),
      ("{mnemonic} {condition} {register} {register} plus {register} {shift} "
           "{immediate} preindex",
       "{}{} {}, [{}, {}, {} #{}]!"),
      ("{mnemonic} {condition} {register} {register} minus {register} {shift} "
           "{immediate} preindex",
       "{}{} {}, [{}, -{}, {} #{}]!"),
      ("{mnemonic} {dt} {dregister} {dregister} {dregister}",
       "{}.{} {}, {}, {}"),
      ("{mnemonic} {dt_untyped} {dregister} {dregister} {dregister}", DtUntypedToLLVM)
  ]

  # Work around issues in LLVM 3.8.
  if triple == "thumbv8":
    def ConvertMovRdImm(matches):
      """
      LLVM chooses the T3 encoding for `mov <rd>, #<immediate>` when the
      immediate fits both into a modified immediate (T2 encoding) and 16
      bits (T3 encoding). Adding the `.W` modifier forces the T2 encoding to
      be used.
      """
      # The immediate is the second capture in "mov al {register} {immediate}".
      imm = int(matches[1], 16)
      if imm <= 0xffff:
        lsb = imm & -imm
        if (imm >> 8) < lsb:
          return "mov.w {}, #{}".format(*matches)
      # Fall back to a LLVM making the right decision.
      return "mov {}, #{}".format(*matches)
    llvm_mc_instruction_converters[:0] = [
        # The ARM ARM specifies that if <Rn> is PC in either an ADD or SUB
        # instruction with an immediate, the assembler should use the ADR
        # encoding. LLVM does not know about this subtlety. We get around this
        # by manually translating the instruction to their ADR form.
        ("add al {register} pc {immediate}", "adr {}, #{}"),
        ("sub al {register} pc {immediate}", "adr {}, #-{}"),

        # LLVM is (rightfully) being helpful by swapping register operands so
        # that the 16 bit encoding of the following instructions is used.
        # However, VIXL does not do this. These rules specifically add the `.w`
        # modifier to force LLVM to use the 32 bit encoding if the last register
        # is identical to first one. But at the same time, we should still use
        # the narrow encoding if all registers are the same.
        ("adcs al {register} (\\1) (\\1)", "adcs.n {}, {}, {}"),
        ("adcs al {register} {register} (\\1)", "adcs.w {}, {}, {}"),
        ("orrs al {register} (\\1) (\\1)", "orrs.n {}, {}, {}"),
        ("orrs al {register} {register} (\\1)", "orrs.w {}, {}, {}"),
        ("eors al {register} (\\1) (\\1)", "eors.n {}, {}, {}"),
        ("eors al {register} {register} (\\1)", "eors.w {}, {}, {}"),
        ("ands al {register} (\\1) (\\1)", "ands.n {}, {}, {}"),
        ("ands al {register} {register} (\\1)", "ands.w {}, {}, {}"),
        # Solve the same issue as for the previous rules, however, we need to
        # take into account that ADD instructions with the stack pointer have
        # additional 16 bit forms.
        ("add al {register} (\\1) (\\1)", "add.n {}, {}, {}"),
        ("add al {register} (\\1) r13", "add.w {}, {}, sp"),
        ("add al {register} r13 (\\1)", "add.n {}, sp, {}"),
        ("add al {register} {register} (\\1)", "add.w {}, {}, {}"),
        ("mov al {register} {immediate}", ConvertMovRdImm)
    ]

  # Our test generator framework uses mnemonics starting with a capital letters.
  # We need everythin to be lower case for LLVM.
  vixl_instruction = vixl_instruction.lower()

  llvm_instruction = []

  # VIXL may have generated more than one instruction seperated by ';'
  # (an IT instruction for example).
  for instruction in vixl_instruction.split(';'):
    # Strip out extra white spaces.
    instruction = instruction.strip()
    # Try all converters in the list.
    for pattern, result in llvm_mc_instruction_converters:
      # Build the regular expression for this converter.
      instruction_matcher = "^" + pattern.format(**pattern_matchers) + "$"
      match = re.match(instruction_matcher, instruction)
      if match:
        # If we have a match, the object will contain a tuple of substrings.
        if isinstance(result, types.FunctionType):
          # `result` is a function, call it produce the instruction.
          llvm_instruction.append(result(match.groups()))
        else:
          # `result` is a string, use it as the format string.
          assert(isinstance(result, str))
          llvm_instruction.append(result.format(*match.groups()))
        break

  if llvm_instruction:
    return "\n".join(llvm_instruction)

  # No converters worked so raise an exception.
  raise Exception("Unsupported instruction {}.".format(instruction))


def ReadTrace(trace):
  """
  Receive the content of an assembler trace, extract the relevant information
  and return it as a list of tuples. The first part of each typle is a string
  representing the instruction. The second part is a list of bytes representing
  the encoding.

  For example:

      [
        ("Clz eq r0 r0", ["0x10", "0x0f", "0x6f", "0x01"]),
        ("Clz eq r0 r1", ["0x11", "0x0f", "0x6f", "0x01"]),
        ("Clz eq r0 r2", ["0x12", "0x0f", "0x6f", "0x01"])
      ]
  """

  pattern = re.compile(
      "^  (?P<encoding>(:?0x[0-9a-f]{2}, )+0x[0-9a-f]{2}) // (?P<instruction>.*)$",
      re.M)
  return [
      (m.group('instruction'), m.group('encoding').replace(" ", "").split(","))
      for m in re.finditer(pattern, trace)
  ]


def VerifyInstructionsWithLLVMMC(llvm_mc, f, triple):
  """
  Extract all instructions from `f`, feed them to `llvm-mc` and make sure it's
  encoded them the same way as VIXL. `triple` allows us to specify either
  "thumbv8" or "armv8".
  """

  vixl_reference = ReadTrace(f.read())
  vixl_instructions, vixl_encodings = zip(*vixl_reference)
  instructions = [
      ConvertToLLVMFormat(instruction, triple)
      for instruction in vixl_instructions
  ]
  llvm_mc_proc = subprocess.Popen(
      [llvm_mc, '-assemble', '-triple={}'.format(triple), '-mattr=v8,crc',
       # LLVM fails to recognize some instructions as valid T32 when we do not
       # set `-mcpu`.
       '-mcpu=cortex-a53', '-show-encoding'],
      stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  out, err = llvm_mc_proc.communicate("\n".join(instructions).encode())
  # If `llvm-mc` printed something to stderr then stop.
  if err:
    print(err.decode())
    return

  # Extract list of bytes from `llvm-mc` output. It's in the following form:
  #
  #         clzeq   r0, r0                  @ encoding: [0x10,0x0f,0x6f,0x01]
  #                                                      ^^^^ ^^^^ ^^^^ ^^^^
  llvm_encodings = [
      match_object.group('encoding').replace(" ", "").split(",")
      for match_object in re.finditer(".*@ encoding: \[(?P<encoding>.*)\]",
                                      out.decode())
  ]

  # If LLVM has generated exactly twice as much instructions, we assume this is
  # due to IT instructions preceding every instruction under test. VIXL's
  # assembly reference files will contain a single array of 4 bytes encoding
  # both the IT and the following instruction. While LLVM will have decoded them
  # into two seperate 2 bytes arrays.
  if len(llvm_encodings) == 2 * len(vixl_encodings):
    llvm_encodings = [
        llvm_encodings[i * 2] + llvm_encodings[(i * 2) + 1]
        for i in range(0, len(vixl_encodings))
    ]

  # Check the encodings from LLVM are identical to VIXL's.
  if len(llvm_encodings) != len(vixl_encodings):
    print("""Error: llvm-mc generated {} instructions than there are in the
generated trace.
        """.format("fewer" if len(llvm_encodings) < len(vixl_encodings) else "more"))
  else:
    for i in range(0, len(vixl_encodings)):
      if llvm_encodings[i] != vixl_encodings[i]:
        print("""Error: llvm-mc disagrees on the encoding of \"{instruction}\":
  LLVM-MC: {llvm}
  VIXL:    {vixl}
            """.format(instruction=vixl_instructions[i].replace("\n", "; "),
                       llvm=llvm_encodings[i],
                       vixl=vixl_encodings[i]))


if __name__ == "__main__":
  args = BuildOptions()

  CheckLLVMVersion(args.llvm_mc)

  trace_dir = 'test/aarch32/traces/'
  trace_files = [
      trace_file
      for trace_file in os.listdir(trace_dir)
      if trace_file.startswith("assembler-")
  ]
  trace_files.sort()
  for trace_file in trace_files:
    if args.verbose:
      print("Verifying \"" + trace_file + "\".")
    with open(os.path.join(trace_dir, trace_file), "r") as f:
      if "t32" in trace_file:
        VerifyInstructionsWithLLVMMC(args.llvm_mc, f, "thumbv8")
      elif "a32" in trace_file:
        VerifyInstructionsWithLLVMMC(args.llvm_mc, f, "armv8")
      else:
        raise Exception("Failed to recognize the ISA in \"" + trace_file + "\".")
