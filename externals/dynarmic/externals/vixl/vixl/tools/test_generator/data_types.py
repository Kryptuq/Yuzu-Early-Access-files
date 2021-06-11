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

class OperandBase(object):
  """
  Base class for operands. An operand represents an argument passed to the
  macro-assembler to generate an instruction. For example, registers, conditions
  or C++ `Operand` and `MemOperand` objects are operands. We can think of them
  as "assemble-time" data.

  An operand is described with a type name, corresponding to the C++ type used
  to represent it (e.g. "Register", "ShiftType", "MemOperand", ...) and a name
  to identify it.

  Attributes:
    name      Name for the operand. It is used to declare variable names.
    type_name C++ type for the operand.
  """

  def __init__(self, name, type_name):
    self.name = name
    self.type_name = type_name

  @staticmethod
  def __iter__():
    """
    Operand types have to act as an iterator so that `generator.OperandList` can
    unwrap them (see `OperandWrapper` and `generator.OperandList.unwrap()`).
    """
    raise NotImplementedError()

  @staticmethod
  def GetArgumentType():
    """
    Return the type to be printed when passing this operand as an argument. For
    example, we could pass it by value or by reference.
    """
    raise NotImplementedError()

  @staticmethod
  def Declare():
    """
    Generate code to declare the operand `struct Operands`.
    """
    raise NotImplementedError()

  @staticmethod
  def Instantiate():
    """
    Generate code to instantiate the operand from inside a `kTests` loop, with
    `i` being the index variable.
    """
    raise NotImplementedError()


class Operand(OperandBase):
  """
  Representation of a single operand. An operand has different variants
  (e.g. "r0", "r1", "LSL", ...) and a default one.

  Attributes:
    variants  List of variants the operand can take.
    default   Default variant to use.

  Note that the `variants` and `default` attributes are not used from inside the
  class. They are public fields used by the `TestCase` object to generate C++.
  """

  def __init__(self, name, type_name, variants, default):
    super().__init__(name, type_name)
    self.variants = variants
    self.default = default

  def __iter__(self):
    """
    Iterating over a single operand just yields the operand object.
    """
    yield self

  def GetArgumentType(self):
    """
    A single operand is passed to the macro-assembler by value. We just print
    the type name as it is.
    """
    return self.type_name

  def Declare(self):
    """
    Generate code to declare the operand as a single member in
    `struct Operands`.
    """
    return "{type_name} {name};".format(type_name=self.type_name,
                                        name=self.name)

  def Instantiate(self):
    """
    Generate code to instantiate the operand as a single local variable.
    """
    code = "{type_name} {name} = kTests[i].operands.{name};"
    return code.format(type_name=self.type_name, name=self.name)


class OperandWrapper(OperandBase):
  """
  Representation for an operand type which wraps a list of more operands. It
  corresponds to C++ objects such as `Operand` or `MemOperand`.

  Attributes:
    operand_list  List of operand that this object wraps.
  """

  def __init__(self, name, type_name, operand_list):
    super().__init__(name, type_name)
    self.operand_list = operand_list

  def __iter__(self):
    """
    Iterate through the list of operands. This is required by
    `OperandList.unwrap()`.
    """
    return iter(self.operand_list)

  def GetArgumentType(self):
    """
    An operand wrapper object will be passed to the macro-assembler by
    reference.
    """
    return "const " + self.type_name + "&"

  def Declare(self):
    """
    An `OperandWrapper` object does not need to be declared in
    `struct Operands`. Although we do need to declare all underlying operands.
    """
    return "\n".join([operand.Declare() for operand in self.operand_list])

  def Instantiate(self):
    """
    Instantiate the underlying operands first and then finally instantiate the
    wrapper object.

    For example, if the object represents a C++ `Operand` type for a shifted
    register we would get:

    ~~~
    Register rm = kTests[i].operands.rm;
    Shift shift_type = kTests[i].operands.shift_type;
    uint32_t amount = kTests[i].operands.amount;
    Operand op(rm, shift_type, amount);
    ~~~
    """
    instantiate_wrapped_operands = "\n".join([
        operand.Instantiate()
        for operand in self.operand_list
    ])
    instantiate_this = "{type_name} {name}({arguments});".format(
        type_name=self.type_name,
        name=self.name,
        arguments=", ".join([operand.name for operand in self.operand_list]))
    return instantiate_wrapped_operands + "\n" + instantiate_this


class Input(object):
  """
  Base class for all input types instantiated from a JSON description. This
  class should not be instantiated directly, instead, we create subclasses for
  each kind of input we have.

  As opposed to operands, an input represents data passed to an instruction at
  runtime. For example, it will be the value you write to a register before
  executing the instruction under test.

  Attributes:
    name    Name of the input. It is used to declare variable names.

    values  List of values this input can take.
    default Default value to use.
  """

  def __init__(self, name, values, default):
    self.name = name

    self.values = values
    self.default = default

  @staticmethod
  def Prologue():
    """
    Return a string describing what C++ code to emit before the instruction
    under test is emitted.
    """
    raise NotImplementedError()

  @staticmethod
  def Epilogue():
    """
    Return a string describing what C++ code to emit after the instruction under
    test is emitted.
    """
    raise NotImplementedError()

  @staticmethod
  def Declare():
    """
    Generate code to declare the input in `struct Inputs`.
    """
    raise NotImplementedError()

  @staticmethod
  def PrintInput(suffix = ""):
    """
    Generate code to print the input referred to by `self.name`. Optionally add
    `suffix` to the input's name.
    """
    raise NotImplementedError()

  @staticmethod
  def PrintOutput():
    """
    Generate code to print the input from the result buffer, indexed with `i`
    and `j`.
    """
    raise NotImplementedError()

  @staticmethod
  def InstantiateResult(suffix = ""):
    """
    Generate code to instantiate an input from the result buffer, indexed with
    `i`. Optionally add `suffix` to the input's name.
    """
    raise NotImplementedError()

  @staticmethod
  def InstantiateInput(suffix = ""):
    """
    Generate code to instantiate an input from the input buffer, indexed with
    `i`. Optionally add `suffix` to the input's name.
    """
    raise NotImplementedError()

  @staticmethod
  def InstantiateReference(suffix = ""):
    """
    Generate code to instantiate an input from the reference buffer, indexed
    with `i`. Optionally add `suffix` to the input's name.
    """
    raise NotImplementedError()

  @staticmethod
  def Compare(left_suffix, operand, right_suffix):
    """
    Generate code as a C++ expression comparing two inputs of this type.
    """
    raise NotImplementedError()


class Scalar(Input):
  """
  Base class for inputs that are represented as a single scalar value.
  Subclasses should implement `TypeName()`, `Pri()` and `NDigit()` to specify
  how they should be represented and printed, see the `U32` class for an
  example.
  """

  @staticmethod
  def TypeName():
    """
    Return the type name as used to declare and instantiate this input.
    """
    raise NotImplementedError()

  @staticmethod
  def Pri():
    """
    Return the format string used to generate a call to `printf` to print this
    input type. For example, "PRIx32".
    """
    raise NotImplementedError()

  @staticmethod
  def NDigit():
    """
    Return the number of digits to use to print this input type.
    """
    raise NotImplementedError()

  def Declare(self):
    """
    A scalar input is declared as a single member in `struct Inputs`.
    """
    return "{type_name} {name};".format(type_name=self.TypeName(),
                                        name=self.name)

  def PrintInput(self, suffix = ""):
    code = "printf(\"0x%0{n_digit}\" {pri}, {name});"
    return code.format(n_digit=self.NDigit(), pri=self.Pri(),
                       name=self.name + suffix)

  def PrintOutput(self):
    code = "printf(\"0x%0{n_digit}\" {pri}, results[i]->outputs[j].{name});"
    return code.format(n_digit=self.NDigit(), pri=self.Pri(), name=self.name)

  def InstantiateResult(self, suffix = ""):
    code = "{type_name} {name}{suffix} = results[i]->outputs[j].{name};"
    return code.format(type_name=self.TypeName(), name=self.name, suffix=suffix)

  def InstantiateInput(self, suffix = ""):
    code = "{type_name} {name}{suffix} = kTests[i].inputs[j].{name};"
    return code.format(type_name=self.TypeName(), name=self.name, suffix=suffix)

  def InstantiateReference(self, suffix = ""):
    code = "{type_name} {name}{suffix} = reference[i].outputs[j].{name};"
    return code.format(type_name=self.TypeName(), name=self.name, suffix=suffix)

  def Compare(self, left_suffix, operand, right_suffix):
    return """
      ({name}{left_suffix} {operand} {name}{right_suffix})
      """.format(name=self.name, left_suffix=left_suffix, operand=operand,
                 right_suffix=right_suffix)


class U32(Scalar):
  """
  Base class for inputs that can be represented as 32 bit unsigned words.
  """

  @staticmethod
  def TypeName():
    return "uint32_t"

  @staticmethod
  def Pri():
    return "PRIx32"

  @staticmethod
  def NDigit():
    return 8


class F64(Scalar):
  @staticmethod
  def TypeName():
    return "uint64_t"

  @staticmethod
  def Pri():
    return "PRIx64"

  @staticmethod
  def NDigit():
    return 16

  def PrintInput(self, suffix = ""):
    code = "printf(\"0x%0{n_digit}\" {pri} \"(%g)\", {name}, RawbitsToDouble({name}));"
    return code.format(n_digit=self.NDigit(), pri=self.Pri(),
                       name=self.name + suffix)


class Register(U32):
  """
  Description of a Register input. The `Prologue` and `Epilogue` methods
  describe what C++ code to emit to set and record the value of a register
  before and after executing an instruction.
  """

  def Prologue(self):
    code = "__ Ldr({name}, MemOperand(input_ptr, offsetof(Inputs, {name})));"
    return code.format(name=self.name)

  def Epilogue(self):
    code = "__ Str({name}, MemOperand(result_ptr, offsetof(Inputs, {name})));"
    return code.format(name=self.name)


class DRegisterF64(F64):
  def Prologue(self):
    code = "__ Vldr({name}, MemOperand(input_ptr, offsetof(Inputs, {name})));"
    return code.format(name=self.name)

  def Epilogue(self):
    code = "__ Vstr({name}, MemOperand(result_ptr, offsetof(Inputs, {name})));"
    return code.format(name=self.name)


class NZCV(U32):
  """
  Description of NZCV flags as inputs to an instruction.

  The `Prologue` and `Epilogue` methods describe what C++ code to emit to set
  and record the NZCV flags before and after emitting the instruction under
  test.
  """

  def Prologue(self):
    # When setting the `NZCV` flags, we need to make sure we do not override the
    # `Q` bit. Therefore we use two scratch registers that we push on the stack
    # first to allow the instruction to use them as operands.
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register nzcv_bits = temp_registers.Acquire();
          Register saved_q_bit = temp_registers.Acquire();
          // Save the `Q` bit flag.
          __ Mrs(saved_q_bit, APSR);
          __ And(saved_q_bit, saved_q_bit, QFlag);
          // Set the `NZCV` and `Q` flags together.
          __ Ldr(nzcv_bits, MemOperand(input_ptr, offsetof(Inputs, {})));
          __ Orr(nzcv_bits, nzcv_bits, saved_q_bit);
          __ Msr(APSR_nzcvq, nzcv_bits);
        }}
        """
    return code.format(self.name)

  def Epilogue(self):
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register nzcv_bits = temp_registers.Acquire();
          __ Mrs(nzcv_bits, APSR);
          // Only record the NZCV bits.
          __ And(nzcv_bits, nzcv_bits, NZCVFlag);
          __ Str(nzcv_bits, MemOperand(result_ptr, offsetof(Inputs, {})));
        }}
        """
    return code.format(self.name)


class Q(U32):
  """
  Description of the Q flag as inputs to an instruction.

  The `Prologue` and `Epilogue` methods describe what C++ code to emit to set
  and record the Q flag before and after emitting the instruction under test.
  """

  def Prologue(self):
    # When clearing or setting the `Q` bit, we need to make sure the `NZCV`
    # flags are not overriden. Therefore we use two scratch registers that we
    # push on the stack first to allow the instruction to use them as operands.
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register q_bit = temp_registers.Acquire();
          Register saved_nzcv_bits = temp_registers.Acquire();
          // Save the `NZCV` flags.
          __ Mrs(saved_nzcv_bits, APSR);
          __ And(saved_nzcv_bits, saved_nzcv_bits, NZCVFlag);
          // Set the `NZCV` and `Q` flags together.
          __ Ldr(q_bit, MemOperand(input_ptr, offsetof(Inputs, {})));
          __ Orr(q_bit, q_bit, saved_nzcv_bits);
          __ Msr(APSR_nzcvq, q_bit);
        }}
        """
    return code.format(self.name)

  def Epilogue(self):
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register q_bit = temp_registers.Acquire();
          __ Mrs(q_bit, APSR);
          // Only record the Q bit.
          __ And(q_bit, q_bit, QFlag);
          __ Str(q_bit, MemOperand(result_ptr, offsetof(Inputs, {})));
        }}
        """
    return code.format(self.name)


class GE(U32):
  """
  Description of the GE flag as inputs to an instruction.

  The `Prologue` and `Epilogue` methods describe what C++ code to emit to set
  and record the GE flags before and after emitting the instruction under test.
  """

  def Prologue(self):
    # We need a scratch register to load the `GE` flags.
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register ge_bits = temp_registers.Acquire();
          __ Ldr(ge_bits, MemOperand(input_ptr, offsetof(Inputs, {})));
          __ Msr(APSR_g, ge_bits);
        }}
        """
    return code.format(self.name)

  def Epilogue(self):
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register ge_bits = temp_registers.Acquire();
          __ Mrs(ge_bits, APSR);
          // Only record the GE bits.
          __ And(ge_bits, ge_bits, GEFlags);
          __ Str(ge_bits, MemOperand(result_ptr, offsetof(Inputs, {})));
        }}
        """
    return code.format(self.name)


class FPSCR(U32):
  def Prologue(self):
    # We need a scratch register to load the `FPCSR` flags.
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register fpsr_bits = temp_registers.Acquire();
          __ Ldr(fpsr_bits, MemOperand(input_ptr, offsetof(Inputs, {})));
          __ Vmsr(FPSCR, fpsr_bits);
        }}
        """
    return code.format(self.name)

  def Epilogue(self):
    code = """{{
          UseScratchRegisterScope temp_registers(&masm);
          Register fpsr_bits = temp_registers.Acquire();
          __ Vmrs(RegisterOrAPSR_nzcv(fpsr_bits.GetCode()), FPSCR);
          __ Str(fpsr_bits, MemOperand(result_ptr, offsetof(Inputs, {})));
        }}
        """
    return code.format(self.name)


class MemOperand(Input):
  """
  Description of a memory location input, used to test a `MemOperand` operand.

  A memory location input is a compound type and is represented as an
  array of two `uint32_t` values, an "offset" and a "data":

  ~~~
  {
    0x0,       // Offset used to record the base register in case it was
               // updated.
    0xabababab // 32 bit value in memory the instruction should work with.
  }
  ~~~
  """

  def Declare(self):
    """
    Declare the input as a two element array.
    """
    return "uint32_t {name}[2];".format(name=self.name)

  def PrintInput(self, suffix = ""):
    code = '''printf("{{0x%08" PRIx32 ", 0x%08" PRIx32 "}}",
                     {name}[0], {name}[1]);'''
    return code.format(name=self.name + suffix)

  def PrintOutput(self):
    code = '''printf("{{0x%08" PRIx32 ", 0x%08" PRIx32 "}}",
                     results[i]->outputs[j].{name}[0],
                     results[i]->outputs[j].{name}[1]);'''
    return code.format(name=self.name)

  def InstantiateResult(self, suffix = ""):
    code = """uint32_t {name}{suffix}[2] = {{
        results[i]->outputs[j].{name}[0],
        results[i]->outputs[j].{name}[1]
      }};
      """
    return code.format(name=self.name, suffix=suffix)

  def InstantiateInput(self, suffix = ""):
    code = """uint32_t {name}{suffix}[2] = {{
        kTests[i].inputs[j].{name}[0],
        kTests[i].inputs[j].{name}[1]
      }};
      """
    return code.format(name=self.name, suffix=suffix)

  def InstantiateReference(self, suffix = ""):
    code = """uint32_t {name}{suffix}[2] = {{
        results[i]->outputs[j].{name}[0],
        results[i]->outputs[j].{name}[1]
      }};
      """
    return code.format(name=self.name, suffix=suffix)

  def Compare(self, left_suffix, operand, right_suffix):
    return """
      (({name}{left_suffix}[0] {operand} {name}{right_suffix}[0]) &&
       ({name}{left_suffix}[1] {operand} {name}{right_suffix}[1]))
      """.format(name=self.name, left_suffix=left_suffix, operand=operand,
                 right_suffix=right_suffix)

  def Prologue(self):
    return """
      // Allocate 4 bytes for the instruction to work with.
      scratch_memory_buffers[i] = new byte[4];
      {{
        UseScratchRegisterScope temp_registers(&masm);

        Register {name}_tmp = temp_registers.Acquire();
        Register base_register = {name}.GetBaseRegister();

        // Write the expected data into the scratch buffer.
        __ Mov(base_register, Operand::From(scratch_memory_buffers[i]));
        __ Ldr({name}_tmp, MemOperand(input_ptr, offsetof(Inputs, {name}) + 4));
        __ Str({name}_tmp, MemOperand(base_register));

        // Compute the address to put into the base register so that the
        // `MemOperand` points to the right location.
        // TODO: Support more kinds of `MemOperand`.
        if (!{name}.IsPostIndex()) {{
          if ({name}.IsImmediate()) {{
            if ({name}.GetSign().IsPlus()) {{
              __ Mov({name}_tmp, {name}.GetOffsetImmediate());
              __ Sub(base_register, base_register, {name}_tmp);
            }} else {{
              __ Mov({name}_tmp, -{name}.GetOffsetImmediate());
              __ Add(base_register, base_register, {name}_tmp);
            }}
          }} else if ({name}.IsShiftedRegister()) {{
            __ Mov({name}_tmp, Operand({name}.GetOffsetRegister(),
                                       {name}.GetShift(),
                                       {name}.GetShiftAmount()));
            if ({name}.GetSign().IsPlus()) {{
              __ Sub(base_register, base_register, {name}_tmp);
            }} else {{
              __ Add(base_register, base_register, {name}_tmp);
            }}
          }}
        }}
      }}
      """.format(name=self.name)

  def Epilogue(self):
    # TODO: This generated code does not support recording the state for
    # instructions where the base register is the same as another register used
    # in the instruction. It is possible to do so but requires more temporary
    # registers which is not trivial to implement without
    # `UseScratchRegisterScope`. We will be able to lift this restriction when
    # it is implemented.
    return """{{
        UseScratchRegisterScope temp_registers(&masm);
        Register {name}_tmp = temp_registers.Acquire();
        Register base_register = {name}.GetBaseRegister();

        // Compute the address of the scratch buffer by from the base register. If
        // the instruction has updated the base register, we will be able to
        // record it.
        if (!{name}.IsPostIndex()) {{
          if ({name}.IsImmediate()) {{
            if ({name}.GetSign().IsPlus()) {{
              __ Mov({name}_tmp, {name}.GetOffsetImmediate());
              __ Add(base_register, base_register, {name}_tmp);
            }} else {{
              __ Mov({name}_tmp, -{name}.GetOffsetImmediate());
              __ Sub(base_register, base_register, {name}_tmp);
            }}
          }} else if ({name}.IsShiftedRegister()) {{
            __ Mov({name}_tmp, Operand({name}.GetOffsetRegister(),
                                       {name}.GetShift(),
                                       {name}.GetShiftAmount()));
            if ({name}.GetSign().IsPlus()) {{
              __ Add(base_register, base_register, {name}_tmp);
            }} else {{
              __ Sub(base_register, base_register, {name}_tmp);
            }}
          }}
        }}

        // Record the value of the base register, as an offset from the scratch
        // buffer's address.
        __ Mov({name}_tmp, Operand::From(scratch_memory_buffers[i]));
        __ Sub(base_register, base_register, {name}_tmp);
        __ Str(base_register, MemOperand(result_ptr, offsetof(Inputs, {name})));

        // Record the 32 bit word from memory.
        __ Ldr({name}_tmp, MemOperand({name}_tmp));
        __ Str({name}_tmp, MemOperand(result_ptr, offsetof(Inputs, {name}) + 4));
      }}
      """.format(name=self.name)
