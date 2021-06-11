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

import itertools
import random
import os.path
from copy import deepcopy

class OperandList(object):
  """
  Convenience class representing a list of operand objects. It can be viewed is
  an iterator over operand objects.

  Attributes:
    operand_list
  """

  def __init__(self, operand_list):
    self.operand_list = operand_list

  def __iter__(self):
    return iter(self.operand_list)

  def unwrap(self):
    """
    Return a list of `Operand` objects, unwrapping `OperandWrapper` objects into
    `Operand` objects. For example:

    ~~~
    Condition, Register, Operand(Register, Shift, Register)
    ~~~

    Unwraps to:

    ~~~
    Condition, Register, Register, Shift, Register
    ~~~
    """
    return itertools.chain(*self.operand_list)

  def ExcludeVariants(self, type_name, variant_to_exclude):
    """
    Remove variants in `variant_to_exclude` from operands with type `type_name`.
    """
    # Find the list of operand with type `type_name`.
    relevant_operands = filter(lambda operand: operand.type_name == type_name,
                               self)
    for operand in relevant_operands:
      # Remove the intersection of the existing variants and variants we do not
      # want.
      for variant in set(operand.variants) & set(variant_to_exclude):
        operand.variants.remove(variant)

  def GetNames(self):
    """
    Return the list of all `Operand` names, excluding `OperandWrapper` objects.
    """
    return [operand.name for operand in self.unwrap()]


class InputList(object):
  """
  Convevience class representing a list of input objects.

  This class is an iterator over input objects.

  Attributes:
    inputs
  """

  def __init__(self, inputs):
    self.inputs = inputs

  def __iter__(self):
    return iter(self.inputs)

  def GetNames(self):
    """
    Return the list of input names.
    """
    return [input.name for input in self]


class TestCase(object):
  """
  Object representation of a test case, as described in JSON. This object is
  used to build sets of operands and inputs that will be used by the generator
  to produce C++ arrays.

  Attributes:
    name            Name of the test case, it is used to name the array to
                    produce.
    seed            Seed value to use for reproducable random generation.
    operand_names   List of operand names this test case covers.
    input_names     List of input names this test case covers.
    operand_filter  Python expression as a string to filter out operands.
    input_filter    Python expression as a string to filter out inputs.
    operand_limit   Optional limit of the number of operands to generate.
    input_limit     Optional limit of the number of inputs to generate.
    it_condition    If not None, an IT instruction needs to be generated for the
                    instruction under test to be valid. This member is a string
                    template indicating the name of the condition operand, to be
                    used with "format". For example, it will most likely have
                    the value "{cond}".
  """

  # Declare functions that will be callable from Python expressions in
  # `self.operand_filter`.
  operand_filter_runtime = {
      'register_is_low': lambda register:
          register in ["r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"]
  }

  def __init__(self, name, seed, operand_names, input_names, operand_filter,
               input_filter, operand_limit, input_limit, it_condition):
    self.name = name
    self.seed = seed
    self.operand_names = operand_names
    self.input_names = input_names
    self.operand_filter = operand_filter
    self.input_filter = input_filter
    self.operand_limit = operand_limit
    self.input_limit = input_limit
    self.it_condition = it_condition

  def GenerateOperands(self, operand_types):
    """
    Generate a list of tuples, each tuple describing what operands to pass to an
    instruction to encode it. We use this to generate operand definitions.

    The algorithm used is a simple product of all operand variants. To limit
    what we generate, we choose to apply the product only on operands with their
    name in the `self.operand_names` list.

    Additionally, we use the Python expression in `self.operand_filter` to
    filter out tuples we do not want.

    Argument:
      operand_types  The `OperandList` object that describe the form of the
                     instruction to generate code for.
    """
    # Build a list of all possible variants as a list of tuples. If the
    # operand's name is not in `self.operand_names`, then we restrict the list
    # to contain default variant. Each tuple in the list has the form
    # `(name, [variant1, variant2, ...])`. For example:
    #
    #   [
    #     ('cond', ['al', 'ne', 'eq', ...]), # All condition variants.
    #     ('rd', ['r0', 'r1', ...]),         # All register variants.
    #     ('rn', ['r0'])                     # Default register variant (r0).
    #     ...
    #   ]
    variants = [
        [(operand_type.name, variant) for variant in operand_type.variants]
            if operand_type.name in self.operand_names
            else [(operand_type.name, operand_type.default)]
        for operand_type in operand_types.unwrap()
    ]
    lambda_string = "lambda {args}: {expression}".format(
        args=",".join(operand_types.GetNames()),
        expression=self.operand_filter)
    filter_lambda = eval(lambda_string, self.operand_filter_runtime)

    def BuildOperandDefinition(operands):
      """
      Take a list of tuples describing the operands and build a definition from
      it. A definition is a tuple with a list of variants and a
      `expect_instruction_before` string.

      For example, we are turning this:

        [
          ('cond', 'ne'),
          ('rd', 'r0'),
          ('rn', 'r1'),
          ('rm', 'r0)
        [

      Into:

        (['ne', 'r0', 'r1', 'r0'], "It ne;")

      """
      return (
        # Build a list of operands by only keeping the second element of each
        # tuple.
        [operand[1] for operand in operands],
        # The next field is a boolean indicating if the test case needs to
        # generate an IT instruction.
        "true" if self.it_condition else "false",
        # If so, what condition should it be?
        self.it_condition.format(**dict(operands)) if self.it_condition else "al"
      )

    # Build and return a list of operand definitions by computing the product of
    # all variants and filtering them with `filter_lambda`.
    #
    # Operand definitions consist of a list with a list of variants and an
    # optional `expect_instruction_before` string. For example:
    #
    #   [
    #     (['al', 'r0', 'r1', 'r2'], ""),
    #     (['ne', 'r0', 'r1', 'r0'], "It ne;"),
    #     ...
    #   ]
    #
    # Here, the filtered product of variants builds a list of lists of tuples, as such:
    #
    #   [
    #     [('cond', 'al'), ('rd', 'r0'), ('rn', 'r1'), ('rn', 'r2')]
    #     [('cond', 'ne'), ('rd', 'r0'), ('rn', 'r1'), ('rn', 'r0')],
    #     ...
    #   ]
    #
    # We then pass them to `BuildOperandDefinition` to produce the expected form
    # out of it.
    result = [
        BuildOperandDefinition(operands)
        for operands in itertools.product(*variants)
        if filter_lambda(**dict(operands))
    ]
    if self.operand_limit is None:
      return result
    else:
      # Use a fixed seed to randomly choose a limited set of operands.
      random.seed(self.seed)
      return random.sample(result, self.operand_limit)

  def GenerateInputs(self, input_types):
    """
    Generate a list of tuples, each tuple describing what input to pass to an
    instruction at runtime. We use this to generate input definitions.

    The algorithm used is a simple product of all input values. To limit what
    we generate, we choose to apply the product only on inputs with their name
    in the `self.input_names` list.

    Additionally, we use the Python expression in `self.input_filter` to filter
    out tuples we do not want.

    Argument:
      input_types  The `InputList` object describing the list of inputs the
                   instruction can take.
    """
    # Build a list of all possible values as a list of lists. If the input's
    # name is not in `self.input_names`, then we restrict the list to the
    # default value.
    values = [
        input_type.values
            if input_type.name in self.input_names else [input_type.default]
        for input_type in input_types
    ]
    lambda_string = "lambda {args}: {expression}".format(
        args=", ".join(input_types.GetNames()),
        expression=self.input_filter)
    filter_lambda = eval(lambda_string)
    # Build and return a list of input definitions, such as
    # [('NoFlag', '0xffffffff', 0xabababab'), ...] for example.
    result = [
        input_definition
        for input_definition in itertools.product(*values)
        if filter_lambda(*input_definition)
    ]
    if self.input_limit is None:
      return result
    else:
      # Use a fixed seed to randomly choose a limited set of inputs.
      random.seed(self.seed)
      return random.sample(result, self.input_limit)


class Generator(object):
  """
  A `Generator` object contains all information needed to generate a test file.
  Each method will return a string used to fill a variable in a template.


  Attributes:
    test_name  Name of the test inferred from the name of the configuration
               file. It has the following form: `type-op1-op2-op3-isa`.
    test_isa   Instruction set supported by the test, either 'a32' or 't32'.
    test_type  Type of the test, extracted from test_name.
    mnemonics  List of instruction mnemonics.
    operands   `OperandList` object.
    inputs     `InputList` object.
    test_cases  List of `TestCase` objects.
  """

  def __init__(self, test_name, test_isa, test_type, mnemonics, operands,
               inputs, test_cases):
    self.test_name = test_name
    self.test_isa = test_isa
    self.test_type = test_type
    self.mnemonics = mnemonics
    self.inputs = inputs
    self.test_cases = test_cases

    # A simulator test cannot easily make use of the PC and SP registers.
    if self.test_type == "simulator":
      # We need to explicitely create our own deep copy the operands before we
      # can modify them.
      self.operands = deepcopy(operands)
      self.operands.ExcludeVariants("Register", ["r13", "r15"])
    else:
      self.operands = operands

  def MnemonicToMethodName(self, mnemonic):
    if self.test_type in ["simulator", "macro-assembler"]:
      # Return a MacroAssembler method name
      return mnemonic.capitalize()
    else:
      # Return an Assembler method name
      method_name = mnemonic.lower()
      return "and_" if method_name == "and" else method_name

  def InstructionListDeclaration(self):
    """
    ~~~
    M(Adc)  \
    M(Adcs) \
    M(Add)  \
    M(Adds) \
    M(And)  \
    ...
    ~~~
    """
    return "".join([
      "M({}) \\\n".format(self.MnemonicToMethodName(mnemonic))
      for mnemonic in self.mnemonics
    ])

  def OperandDeclarations(self):
    """
    ~~~
    Condition cond;
    Register rd;
    Register rn;
    ...
    ~~~
    """
    return "".join([operand.Declare() for operand in self.operands])

  def InputDeclarations(self):
    """
    ~~~
    uint32_t cond;
    uint32_t rd;
    uint32_t rn;
    ...
    ~~~
    """
    return "".join([input.Declare() for input in self.inputs])

  def InputDefinitions(self):
    """
    ~~~
    static const Inputs kCondition[] = {{...},{...}, ...};
    static const Inputs kRdIsRd[] = {{...},{...}, ...};
    ...
    ~~~
    """
    def InputDefinition(test_input):
      inputs = [
          "{{{}}}".format(",".join(input))
          for input in test_input.GenerateInputs(self.inputs)
      ]

      return """static const Inputs k{name}[] = {{ {input} }};
          """.format(name=test_input.name, input=",".join(inputs))

    return "\n".join(map(InputDefinition, self.test_cases))

  def TestCaseDefinitions(self):
    """
    For simulator tests:
    ~~~
    {{eq, r0, r0, ...},
     "eq r0 r0 ...",
     "Condition_eq_r0_...",
     ARRAY_SIZE(kCondition), kCondition},
    ...
    {{eq, r0, r0, ...},
     "eq r0 r0 ...",
     "RdIsRd_eq_r0_...",
     ARRAY_SIZE(kRdIsRd), kRdIsRn},
    ...
    ~~~

    For assembler tests:
    ~~~
    {{eq, r0, r0, ...},
     "",
     "eq r0 r0 ...",
     "Condition_eq_r0_...",
    ...
    {{eq, r0, r0, ...},
     "",
     "eq r0 r0 ...",
     "RdIsRd_eq_r0_..."}
    ...
    {{eq, r0, r0, ...},
     "It eq",
     "eq r0 r0 ...",
     "RdIsRd_eq_r0_..."}
    ...
    ~~~
    """
    def SimulatorTestCaseDefinition(test_case):
      test_cases = [
          """{{ {{ {operands} }},
             "{operands_description}",
             "{identifier}",
             ARRAY_SIZE(k{test_case_name}),
             k{test_case_name} }}
              """.format(operands=",".join(operand),
                         operands_description=" ".join(operand),
                         identifier=test_case.name + "_" + "_".join(operand),
                         test_case_name=test_case.name)
          for operand, _, _ in test_case.GenerateOperands(self.operands)
      ]
      return ",\n".join(test_cases)

    def AssemblerTestCaseDefinition(test_case):
      test_cases = [
          """{{ {{ {operands} }},
             {in_it_block},
             {it_condition},
             "{operands_description}",
             "{identifier}" }}
              """.format(operands=",".join(operand),
                         in_it_block=in_it_block,
                         it_condition=it_condition,
                         operands_description=" ".join(operand),
                         identifier="_".join(operand))
          for operand, in_it_block, it_condition
              in test_case.GenerateOperands(self.operands)
      ]
      return ",\n".join(test_cases)

    def MacroAssemblerTestCaseDefinition(test_case):
      test_cases = [
          """{{ {{ {operands} }},
             "{operands_description}",
             "{identifier}" }}
              """.format(operands=",".join(operand),
                         operands_description=", ".join(operand),
                         identifier="_".join(operand))
          for operand, _, _ in test_case.GenerateOperands(self.operands)
      ]
      return ",\n".join(test_cases)

    if self.test_type == "simulator":
      return ",\n".join(map(SimulatorTestCaseDefinition, self.test_cases))
    elif self.test_type == "assembler":
      return ",\n".join(map(AssemblerTestCaseDefinition, self.test_cases))
    elif self.test_type == "macro-assembler":
      return ",\n".join(map(MacroAssemblerTestCaseDefinition, self.test_cases))
    elif self.test_type == "assembler-negative":
      return ",\n".join(map(MacroAssemblerTestCaseDefinition, self.test_cases))
    else:
      raise Exception("Unrecognized test type \"{}\".".format(self.test_type))

  def IncludeTraceFiles(self):
    """
    ~~~
    #include "aarch32/traces/sim-...-a32.h"
    #include "aarch32/traces/sim-...-a32.h"
    ...
    ~~~
    """
    operands = "-".join(self.operands.GetNames())
    return "".join([
        "#include \"aarch32/traces/" + self.GetTraceFileName(mnemonic) + "\"\n"
        for mnemonic in self.mnemonics
    ])

  def MacroAssemblerMethodArgs(self):
    """
    ~~~
    Condition cond, Register rd, Register rm, const Operand& immediate
    ~~~
    """
    return ", ".join([
        operand.GetArgumentType() + " " + operand.name
        for operand in self.operands
    ])

  def MacroAssemblerSetISA(self):
    """
    Generate code to set the ISA.
    """
    if self.test_isa == "t32":
      return "masm.UseT32();"
    else:
      return "masm.UseA32();"

  def CodeInstantiateOperands(self):
    """
    ~~~
    Condition cond = kTests[i].operands.cond;
    Register rd = kTests[i].operands.rd;
    ...
    ~~~
    """
    code = "".join([operand.Instantiate() for operand in self.operands])
    if self.test_type in ["simulator", "macro-assembler"]:
      # Simulator tests need scratch registers to function and uses
      # `UseScratchRegisterScope` to dynamically allocate them. We need to
      # exclude all register operands from the list of available scratch
      # registers.
      # MacroAssembler tests also need to ensure that they don't try to run tests
      # with registers that are scratch registers; the MacroAssembler contains
      # assertions to protect against such usage.
      excluded_registers = [
          "scratch_registers.Exclude({});".format(operand.name)
          for operand in self.operands.unwrap()
          if operand.type_name == "Register"
      ]
      return code + "\n".join(excluded_registers)
    return code

  def CodePrologue(self):
    """
    ~~~
    __ Ldr(rn, MemOperand(input_ptr, offsetof(Inputs, rn)));
    __ Ldr(rm, MemOperand(input_ptr, offsetof(Inputs, rm)));
    ...
    ~~~
    """
    return "".join([input.Prologue() for input in self.inputs])

  def CodeEpilogue(self):
    """
    ~~~
    __ Str(rn, MemOperand(result_ptr, offsetof(Inputs, rn)));
    __ Str(rm, MemOperand(result_ptr, offsetof(Inputs, rm)));
    ...
    ~~~
    """
    return "".join([input.Epilogue() for input in self.inputs])

  def CodeParameterList(self):
    """
    ~~~
    cond, rd, rn, immediate
    ~~~
    """
    return ", ".join([
        operand.name
        for operand in self.operands
    ])

  def TracePrintOutputs(self):
    """
    ~~~
    printf("0x%08" PRIx32, results[i]->outputs[j].cond);
    printf(", ");
    printf("0x%08" PRIx32, results[i]->outputs[j].rd);
    printf(", ");
    ...
    ~~~
    """
    return "printf(\", \");".join(
        [input.PrintOutput() for input in self.inputs])


  def CheckInstantiateResults(self):
    """
    ~~~
    uint32_t cond = results[i]->outputs[j].cond;
    uint32_t rd = results[i]->outputs[j].rd;
    ...
    ~~~
    """
    return "".join([input.InstantiateResult() for input in self.inputs])

  def CheckInstantiateInputs(self):
    """
    ~~~
    uint32_t cond_input = kTests[i].inputs[j].cond;
    uint32_t rd_input = kTests[i].inputs[j].rd;
    ...
    ~~~
    """
    return "".join([input.InstantiateInput("_input") for input in self.inputs])

  def CheckInstantiateReferences(self):
    """
    ~~~
    uint32_t cond_ref = reference[i].outputs[j].cond;
    uint32_t rd_ref = reference[i].outputs[j].rd;
    ...
    ~~~
    """
    return "".join([input.InstantiateReference("_ref") for input in self.inputs])

  def CheckResultsAgainstReferences(self):
    """
    ~~~
    (cond != cond_ref) || (rd != rd_ref) || ...
    ~~~
    """
    return " || ".join([input.Compare("", "!=", "_ref") for input in self.inputs])

  def CheckPrintInput(self):
    """
    ~~~
    printf("0x%08" PRIx32, cond_input);
    printf(", ");
    printf("0x%08" PRIx32, rd_input);
    printf(", ");
    ...
    ~~~
    """
    return "printf(\", \");".join(
        [input.PrintInput("_input") for input in self.inputs])

  def CheckPrintExpected(self):
    """
    ~~~
    printf("0x%08" PRIx32, cond_ref);
    printf(", ");
    printf("0x%08" PRIx32, rd_ref);
    printf(", ");
    ...
    ~~~
    """
    return "printf(\", \");".join(
        [input.PrintInput("_ref") for input in self.inputs])

  def CheckPrintFound(self):
    """
    ~~~
    printf("0x%08" PRIx32, cond);
    printf(", ");
    printf("0x%08" PRIx32, rd);
    printf(", ");
    ...
    ~~~
    """
    return "printf(\", \");".join(
        [input.PrintInput("") for input in self.inputs])

  def TestName(self):
    """
    ~~~
    SIMULATOR_COND_RD_RN_RM_...
    ~~~
    """
    return self.test_type.replace("-", "_").upper() + "_" + \
        self.test_name.replace("-", "_").upper()

  def TestISA(self):
    return self.test_isa.upper()

  def GetTraceFileName(self, mnemonic):
    """
    Return the name of a trace file for a given mnemonic.
    """
    return self.test_type + "-" + self.test_name + "-" + \
        mnemonic.lower() + "-" + self.test_isa + ".h"

  def WriteEmptyTraces(self, output_directory):
    """
    Write out empty trace files so we can compile the new test cases.
    """
    for mnemonic in self.mnemonics:
      # The MacroAssembler and negative assembler tests have no traces.
      if self.test_type in ["macro-assembler", "assembler-negative"]: continue

      with open(os.path.join(output_directory, self.GetTraceFileName(mnemonic)),
                "w") as f:
        code = "static const TestResult *kReference{} = NULL;\n"
        f.write(code.format(self.MnemonicToMethodName(mnemonic)))

  def GetIsaGuard(self):
    """
    This guard ensure the ISA of the test is enabled.
    """
    if self.test_isa == 'a32':
      return 'VIXL_INCLUDE_TARGET_A32'
    else:
      assert self.test_isa == 't32'
      return 'VIXL_INCLUDE_TARGET_T32'

