Getting Started with VIXL for AArch32
=====================================


This guide will show you how to use the VIXL framework for AArch32. We will see
how to set up the VIXL assembler and generate some code. We will also go into
details on a few useful features provided by VIXL and see how to run the
generated code.

The source code of the example developed in this guide can be found in the
`examples/aarch32` directory (`examples/aarch32/getting-started.cc`).


Creating the macro assembler.
-----------------------------

First of all you need to make sure that the header files for the assembler are
included. You should have the following lines at the beginning of your source
file:

    // You may use <cstdint> if using C++11 or later.
    extern "C" {
    #include <stdint.h>
    }

    #include <cstdio>
    #include <string>
    #include "aarch32/constants-aarch32.h"
    #include "aarch32/instructions-aarch32.h"
    #include "aarch32/macro-assembler-aarch32.h"

In our case, those files are included by "examples.h".

All VIXL components are declared in the `vixl::aarch32` namespace, so let's add
this to the beginning of the file for convenience (once again, done in
"examples.h"):

    using namespace vixl::aarch32;

Now we are ready to create and initialise the different components.

First of all we need to create a macro assembler object.

    MacroAssembler masm;


Generating some code.
---------------------

We are now ready to generate some code. The macro assembler provides methods
for all the instructions that you can use. As it's a macro assembler,
the instructions that you tell it to generate may not directly map to a single
hardware instruction. Instead, it can produce a short sequence of instructions
that has the same effect.

Before looking at how to generate some code, let's introduce a simple but handy
macro:

    #define __ masm->

It allows us to write `__ Mov(r0, 42);` instead of `masm->Mov(r0, 42);` to
generate code.

Now we are going to write a C++ function to generate our first assembly
code fragment.

    void GenerateDemo(MacroAssembler *masm) {
      __ Ldr(r1, 0x12345678);
      __ And(r0, r0, r1);
      __ Bx(lr);
    }

The generated code corresponds to a function with the following C prototype:

    uint32_t demo(uint32_t x);

This function doesn't perform any useful operation. It loads the value
0x12345678 into r1 and performs a bitwise `and` operation with
the function's argument (stored in r0). The result of this `and` operation
is returned by the function in r0.

Now in our program main function, we only need to create a label to represent
the entry point of the assembly function and to call `GenerateDemo` to
generate the code.

    Label demo;
    masm.Bind(&demo);
    GenerateDemo(&masm);
    masm.Finalize();

Now we are going to learn a bit more on a couple of interesting VIXL features
which are used in this example.

### Label

VIXL's assembler provides a mechanism to represent labels with `Label` objects.
They are easy to use: simply create the C++ object and bind it to a location in
the generated instruction stream.

Creating a label is easy, since you only need to define the variable and bind it
to a location using the macro assembler.

    Label my_label;      // Create the label object.
    __ Bind(&my_label);  // Bind it to the current location.

The target of a branch using a label will be the address to which it has been
bound. For example, let's consider the following code fragment:

    Label foo;

    __ B(&foo);     // Branch to foo.
    __ Mov(r0, 42);
    __ Bind(&foo);  // Actual address of foo is here.
    __ Mov(r1, 0xc001);

If we run this code fragment the `Mov(r0, 42)` will never be executed since
the first thing this code does is to jump to `foo`, which correspond to the
`Mov(r1, 0xc001)` instruction.

When working with labels you need to know that they are only to be used for
local branches, and should be passed around with care. The major reason is
that they cannot safely be passed or returned by value because this can trigger
multiple constructor and destructor calls. The destructor has assertions
to check that we don't try to branch to a label that hasn't been bound.


### Literal Pool

On AArch32 instructions are 16 or 32 bits long, thus immediate values encoded in
the instructions have limited size. If you want to load a constant bigger than
this limit you have two possibilities:

1. Use multiple instructions to load the constant in multiple steps. This
  solution is already handled in VIXL. For instance you can write:

  `__ Mov(r0, 0x12345678);`

  The previous instruction would not be legal since the immediate value is too
  big. However, VIXL's macro assembler will automatically rewrite this line into
  multiple instructions efficiently generate the value, ultimately setting 'r0'
  with the correct value.


2. Store the constant in memory and load this value from the memory. The value
  needs to be written near the code that will load it since we use a PC-relative
  offset to indicate the address of this value. This solution has the advantage
  of making the value easily modifiable at run-time; since it does not reside
  in the instruction stream, it doesn't require cache maintenance when updated.

  VIXL also provides a way to do this:

  `__ Ldr(r0, 0x12345678);`

  The assembler will store the immediate value in a "literal pool", a set of
  constants embedded in the code. VIXL will emit the literal pool when needed.

  The literal pool is emitted regularly, such that they are within range of the
  instructions that refer to it. However, you can force the literal pool to be
  emitted using `masm.EmitLiteralPool()`. It generates a branch to skip the
  pool.


Running the code.
-----------------

We first need to run a few operations to get executable code. The
`ExecutableMemory` helper takes care of it:

    byte* code = masm.GetBuffer().GetBuffer();
    uint32_t code_size = masm.GetBuffer().GetSizeInBytes();
    ExecutableMemory memory(code, code_size);

Then we compute a pointer to the function we just generated and copy:

    uint32_t (*demo_function)(uint32_t) =
        memory.GetOffsetAddress<uint32_t (*)(uint32_t)>(0);

Now, we can call this function pointer exactly as if it were a pointer on a C
function:

    uint32_t input_value = 0x89abcdef;
    uint32_t output_value = (*demo_function)(input_value);

A little trace:

    printf("native: abs(%08x) = %08x\n", input_value, output_value);


The example shown in this tutorial is very simple, because the goal was to
demonstrate the basics of the VIXL framework. There are more complex code
examples in the VIXL `examples/aarch32` directory showing more features of both the
macro assembler and the AArch32 architecture.

Disassembling the generated code.
---------------------------------

Once you have generated something with the macro-assembler, you may want to
disassemble it.

First, you must include iostream.

    #include <iostream>

And the disassembler header file:

    #include "aarch32/disasm-aarch32.h"

Then you have to define the pc used to disassemble (the one which is used to
display the addresses not the location of the instructions):

    uint32_t display_pc = 0x1000;

Or, if you running on a 32 bit host, you can use the real address:

    uint32_t display_pc = static_cast<uintptr_t>(masm.GetBuffer().GetBuffer());

Then you can disassemble the macro assembler's buffer:

    PrintDisassembler disasm(std::cout, display_pc);
    disasm.DisassembleA32Buffer(
        masm.GetBuffer().GetOffsetAddress<uint32_t*>(0), masm.GetCursorOffset());

If you generated T32 code instead of A32 code, you must use
DisassembleT32Buffer. Warning: if your buffer contains some data or contains
mixed T32 and A32 code, the result won't be accurate (everything will be
disassembled as T32 or A32 code).

Example of disassembly:

    0x00001000  e30f0fff	mov r0, #65535
    0x00001004  e34f0fff	movt r0, #65535
    0x00001008  e3041567	mov r1, #17767
    0x0000100c  e3401123	movt r1, #291
    0x00001010  e3a02000	mov r2, #0
    0x00001014  e7c2001f	bfc r0, #0, #3
    0x00001018  e7d4081f	bfc r0, #16, #5
    0x0000101c  e7c72011	bfi r2, r1, #0, #8
    0x00001020  e7df2811	bfi r2, r1, #16, #16
    0x00001024  e1000070	hlt 0
