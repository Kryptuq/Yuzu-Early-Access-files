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

import json
import re
import os
import hashlib
import collections
import itertools

from test_generator import data_types
from test_generator import generator

class DataTypeBuilder(object):
  """
  Factory object for building `data_types.Operand` and `data_types.Input`
  objects. This object stores information about all operand and input types
  described in JSON as dictionnaries indexed by their identifier. See
  `test/a32/config/data-types.json` as a reference.

  Attributes:
    operand_types    Dictionnary of type names corresponding to the JSON
                     "type" field.
    operand_variants Dictionnary of (variants, default) tuples.

    input_types      Dictionnary of type names corresponding to the JSON
                     "type" field.
    input_values     Dictionnary of (values, default) tuples.
  """

  def __init__(self, operand_types, operand_variants, input_types,
               input_values):
    self.operand_types = operand_types
    self.operand_variants = operand_variants
    self.input_types = input_types
    self.input_values = input_values

  def BuildOperand(self, name, identifier):
    """
    Build a `data_types.Operand` object with the name `name`. `identifier`
    identifies which type we want to create, as declared in JSON.
    """
    type_name = self.operand_types[identifier]
    variants, default = self.operand_variants[identifier]
    # We simply pass the `type_name` as a parameter which will be used verbatim
    # in the code.
    return data_types.Operand(name, type_name, variants, default)

  def BuildInput(self, name, identifier):
    """
    Build a `data_types.Input` object with the name `name`. `identifier`
    identifies which type we want to create, as declared in JSON.
    """
    type_name = self.input_types[identifier]
    values, default = self.input_values[identifier]
    # For `data_types.Input` types, the `type_name` refers to the actual name of
    # the Python class, inheriting from `Input`. This is done so that different
    # input types can generate different C++ code by overriding the `Load` and
    # `Store` methods.
    input_constructor = getattr(data_types, type_name)
    return input_constructor(name, values, default)


def LoadJSON(filename):
  """
  Read `filename`, strip its comments and load as JSON.
  """
  with open(filename, "r") as f:
    match_cpp_comments = re.compile("//.*\n")
    # The order in which structures are described in JSON matters as we use them
    # as a seed. Computing a hash from a unordered dict always gives a different
    # value. We use the `object_pairs_hook` to make the json module create
    # `OrderedDict` objects instead of builtin `dict` objects.
    return json.loads(match_cpp_comments.sub("", f.read()),
                      object_pairs_hook=collections.OrderedDict)


def ParseDataTypes(json_data_types):
  """
  Build a `DataTypeBuilder` object containing all information from the JSON
  description in `json_data_types`.

  ~~~
  {
    "operands": [
      {
        "identifier": "AllRegistersButPC"
        "type": "Register"
        "variants": [
          "r0",
          "r1",
          "r2",
          "r3"
        ]
        "default": "r0"
      },
      {
        ...
      }
    ],
    "inputs": [
      {
        "identifier": "Register"
        "type": "Register"
        "values": [
          "0x00000000",
          "0xffffffff",
          "0xabababab"
        ]
        "default": "0xabababab"
      },
      {
        ...
      }
    ]
  }
  ~~~
  """
  operand_types = {
      json_operand_type["identifier"]: json_operand_type["type"]
      for json_operand_type in json_data_types["operands"]
  }
  operand_variants = {
      json_operand_type["identifier"]:
          (json_operand_type["variants"], json_operand_type["default"])
      for json_operand_type in json_data_types["operands"]
  }
  input_types = {
      json_input_type["identifier"]: json_input_type["type"]
      for json_input_type in json_data_types["inputs"]
  }
  input_values = {
      json_input_type["identifier"]:
          (json_input_type["values"], json_input_type["default"])
      for json_input_type in json_data_types["inputs"]
  }
  return DataTypeBuilder(operand_types, operand_variants, input_types, input_values)


def ParseDescription(data_type_builder, json_description):
  """
  Parse the instruction description into a
  (`generator.OperandList`, `generator.InputList`) tuple.

  Example for an instruction that takes a condidition code, two registers and an
  immediate as operand. It will also need inputs for the registers, as well as
  NZCV flags.
  ~~~
  {
    "operands": [
      {
        "name": "cond",
        "type": "Condition",
      },
      {
        "name": "rd",
        "type": "RegisterScratch",
      },
      {
        "name": "rn",
        "type": "RegisterScratch",
      },
      // The last operand needs to be wrapped into a C++ `Operand` object. We
      // declare the operands that need to be wrapped as a list.
      {
        "name": "op",
        "wrapper": "Operand",
        "operands": [
          {
            "name": "immediate",
            "type": "ModifiedImmediate",
          }
        ]
      }
    ],
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
      }
    ]
  ]
  ~~~
  """

  operands = []
  for json_operand in json_description["operands"]:
    if "name" in json_operand and "type" in json_operand:
      operands.append(data_type_builder.BuildOperand(json_operand["name"],
                                                     json_operand["type"]))
    elif "name" in json_operand and \
         "wrapper" in json_operand and \
         "operands" in json_operand:
      wrapped_operands = [
          data_type_builder.BuildOperand(json_wrapped_operand["name"],
                                         json_wrapped_operand["type"])
          for json_wrapped_operand in json_operand["operands"]
      ]
      operands.append(data_types.OperandWrapper(json_operand["name"],
                                                json_operand["wrapper"],
                                                wrapped_operands))
    else:
      raise Exception("Parser failed to recognize JSON \"description\".")
  operand_list = generator.OperandList(operands)

  json_description_inputs = json_description["inputs"]
  input_list = generator.InputList([
      data_type_builder.BuildInput(json_input["name"], json_input["type"])
      for json_input in json_description_inputs
  ])

  return operand_list, input_list


def ParseTestCase(json_test_case):
  """
  Build a `generator.TestCase` object from its JSON description.

  ~~~
  {
    "name": "RdIsNotRn",
    "operands": [
      "rd", "rn"
    ],
    "inputs": [
      "rd", "rn"
    ],
    "operand-filter": "rd != rn", // Python code to limit operand generation.
    "operand-limit": 10           // Choose a random sample of 10 operands.
  }
  ...
  {
    "name": "Flags",
    "operands": [
      "cond"
    ],
    "inputs": [
      "apsr", "q"
    ],
    "input-filter": "q == \"QFlag\"", // Python code to limit input generation
    "input-limit": 200                // Choose a random sample of 200 inputs.
  }
  ...
  {
    "name": "InITBlock",
    "operands": [
      "cond", "rd", "rn", "rm"
    ],
    "in-it-block": "{cond}", // Generate an extra IT instruction. This string
                             // will be used as the operand passed to IT. One
                             // needs to specify under what name the condition
                             // operand is represented, in braces.
    "operand-filter": "cond != 'al' and rd == rm"
  }
  ~~~
  """

  # TODO: The fields in "operands" and "inputs" respectively refer to operands
  # and inputs declared in the instruction description (see `ParseDescription`).
  # We should assert that the user hasn't miss typed them and raise an
  # exception.

  # If the fields are not present, give them default values (empty list,
  # "True", or "None").
  operand_names = json_test_case["operands"] \
      if "operands" in json_test_case else []
  input_names = json_test_case["inputs"] if "inputs" in json_test_case else []
  operand_filter = json_test_case["operand-filter"] \
      if "operand-filter" in json_test_case else "True"
  input_filter = json_test_case["input-filter"] \
      if "input-filter" in json_test_case else "True"
  operand_limit = json_test_case["operand-limit"] \
      if "operand-limit" in json_test_case else None
  input_limit = json_test_case["input-limit"] \
      if "input-limit" in json_test_case else None
  in_it_block = json_test_case["in-it-block"] \
      if "in-it-block" in json_test_case else None

  # Create a seed from the test case description. It will only change if the
  # test case has changed.
  md5 = hashlib.md5(str(json_test_case).encode())
  seed = md5.hexdigest()

  return generator.TestCase(json_test_case["name"], seed, operand_names, input_names,
                            operand_filter, input_filter, operand_limit,
                            input_limit, in_it_block)


def ParseTestFile(test_name, test_isa, mnemonics, operand_list, input_list,
                  json_test_file):
  """
  Build a `generator.Generator` object from a test file description. We have one
  for each generated test files.

  ~~~
  {
    "type": "simulator",  // Type of the test. This will control the prefix we
                          // use when naming the file to generate.
    "name": "special-case",  // Optional name that will be included in the
                             // generated filename.
    "mnemonics": [  // Optional list of instruction, overriding the top-level
      "Adc",        // one.
      "Add",
      ...
    ],
    "test-cases": [
      ... // Test case descriptions parsed with `ParseTestCase`.
    ]
  }
  ~~~
  """
  name = json_test_file["name"] if "name" in json_test_file else ""
  if name is not "":
    test_name = test_name + "-" + name
  # Override the top-level mnemonics list with a subset.
  if "mnemonics" in json_test_file:
    if set(json_test_file["mnemonics"]) == set(mnemonics):
      raise Exception(
          "Overriding mnemonic list is identical to the top-level list")
    if not(set(json_test_file["mnemonics"]) < set(mnemonics)):
      raise Exception(
          "Overriding mnemonic list should a subset of the top-level list")
    mnemonics = json_test_file["mnemonics"]
  test_cases = [
      ParseTestCase(json_test_case)
      for json_test_case in json_test_file["test-cases"]
  ]
  return generator.Generator(test_name, test_isa, json_test_file["type"],
                             mnemonics, operand_list, input_list, test_cases)


def ParseConfig(test_name, test_isas, data_type_builder, json_config):
  """
  Return a list of `generator.Generator` objects from a JSON description. This
  is the top-level description.

  ~~~
  {
    "mnemonics": [
      "Adc",
      "Add",
      ...
    ],
    "description": [
      ... // Instruction description parsed with `ParseDescription`.
    ],
    "test-files": [
      ... // Test files descriptions parsed with `ParseTestFile`.
    ]
  }
  ~~~
  """
  mnemonics = json_config["mnemonics"]
  operand_list, input_list = ParseDescription(
      data_type_builder, json_config["description"])

  return itertools.chain(*[[
          ParseTestFile(test_name, test_isa, mnemonics, operand_list,
                        input_list, json_test_file)
          for json_test_file in json_config["test-files"]
      ]
      for test_isa in test_isas
  ])


def GetTestNameAndISAFromFileName(filename):
  """
  Return a tuple (name, [isa, ...]) extracted from the file name.
  """
  # Strip the ".json" extension
  stripped_basename = os.path.splitext(os.path.basename(filename))[0]
  # The ISA is the last element in the filename, seperated with "-".
  if stripped_basename.endswith(('-a32', '-t32')):
    isa = [stripped_basename[-3:]]
    test_name = stripped_basename[:-4]
  else:
    # If the ISA is ommitted, support both.
    isa = ["a32", "t32"]
    test_name = stripped_basename

  return (test_name, isa)


def GetTestNameFromFileName(filename):
  """
  Return the name given to this test from its file name, stripped of the
  optional "a32" or "t32" at the end.
  """
  test_name, _ = GetTestNameAndISAFromFileName(filename)
  return test_name


def GetISAsFromFileName(filename):
  """
  Return a list of ISAs supported by the test, from the file name, either
  ["a32"], ["t32"] or both.
  """
  _, isas = GetTestNameAndISAFromFileName(filename)

  return isas

def Parse(data_type_file, config_files):
  """
  Parse the `data_type_file` and `test_case_files` json description files into a
  list of (name, test_case) tuples. Test cases are `generator.TestCase`
  objects that can be used to generate C++.
  """

  # Create a `DataTypeBuilder` object. This object will passed down and used to
  # instantiate `data_types.Operand` and `data_types.Input` objects.
  data_type_builder = ParseDataTypes(LoadJSON(data_type_file))

  # Build a list of (name, JSON) tuples to represent the new tests.
  json_configs = [
      # We create the name of the test by looking at the file name stripped of
      # its extension.
      (GetTestNameFromFileName(config_file), GetISAsFromFileName(config_file),
       LoadJSON(config_file))
      for config_file in config_files
  ]

  # Return a list of Generator objects. The generator is the entry point to
  # generate a file.
  # Note that `ParseConfig` returns a list of generators already. We use `chain`
  # here to flatten a list of lists into just a list.
  return itertools.chain(*[
      ParseConfig(test_name, test_isas, data_type_builder, json_config)
      for test_name, test_isas, json_config in json_configs
  ])
