Getting Started with VIXL AArch64
=================================


This guide will show you how to use the VIXL framework for AArch64. We will see
how to set up the VIXL assembler and generate some code. We will also go into
details on a few useful features provided by VIXL and see how to run the
generated code in the VIXL simulator.

The source code of the example developed in this guide can be found in the
`examples/aarch64` directory (`examples/aarch64/getting-started.cc`).


Creating the macro assembler and the simulator.
-----------------------------------------------

First of all you need to make sure that the header files for the assembler and
the simulator are included. You should have the following lines at the beginning
of your source file:

    #include "aarch64/simulator-aarch64.h"
    #include "aarch64/macro-assembler-aarch64.h"

All VIXL components are declared in the `vixl::aarch64` namespace, so let's add
this to the beginning of the file for convenience:

    using namespace vixl::aarch64;

Creating a macro assembler is as simple as

    MacroAssembler masm;

VIXL's assembler will generate some code at run-time, and this code needs to
be stored in a buffer. By default the assembler will automatically manage
the code buffer. However constructors are available that allow manual management
of the code buffer.

We also need to set up the simulator. The simulator uses a Decoder object to
read and decode the instructions from the code buffer. We need to create a
decoder and bind our simulator to this decoder.

    Decoder decoder;
    Simulator simulator(&decoder);


Generating some code.
---------------------

We are now ready to generate some code. The macro assembler provides methods
for all the instructions that you can use. As it's a macro assembler,
the instructions that you tell it to generate may not directly map to a single
hardware instruction. Instead, it can produce a short sequence of instructions
that has the same effect.

For instance, the hardware `add` instruction can only take a 12-bit immediate
optionally shifted by 12, but the macro assembler can generate one or more
instructions to handle any 64-bit immediate. For example, `Add(x0, x0, -1)`
will be turned into `Sub(x0, x0, 1)`.

Before looking at how to generate some code, let's introduce a simple but handy
macro:

    #define __ masm->

It allows us to write `__ Mov(x0, 42);` instead of `masm->Mov(x0, 42);` to
generate code.

Now we are going to write a C++ function to generate our first assembly
code fragment.

    void GenerateDemoFunction(MacroAssembler *masm) {
      __ Ldr(x1, 0x1122334455667788);
      __ And(x0, x0, x1);
      __ Ret();
    }

The generated code corresponds to a function with the following C prototype:

    uint64_t demo_function(uint64_t x);

This function doesn't perform any useful operation. It loads the value
0x1122334455667788 into x1 and performs a bitwise `and` operation with
the function's argument (stored in x0). The result of this `and` operation
is returned by the function in x0.

Now in our program main function, we only need to create a label to represent
the entry point of the assembly function and to call `GenerateDemoFunction` to
generate the code.

    Label demo_function;
    masm.Bind(&demo_function);
    GenerateDemoFunction(&masm);
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
    __ Mov(x0, 42);
    __ Bind(&foo);  // Actual address of foo is here.
    __ Mov(x1, 0xc001);

If we run this code fragment the `Mov(x0, 42)` will never be executed since
the first thing this code does is to jump to `foo`, which correspond to the
`Mov(x1, 0xc001)` instruction.

When working with labels you need to know that they are only to be used for
local branches, and should be passed around with care. There are two reasons
for this:

  - They can't safely be passed or returned by value because this can trigger
    multiple constructor and destructor calls. The destructor has assertions
    to check that we don't try to branch to a label that hasn't been bound.

  - The `B` instruction does not branch to labels which are out of range of the
    branch. The `B` instruction has a range of 2^28 bytes, but other variants
    (such as conditional or `CBZ`-like branches) have smaller ranges. Confining
    them to local ranges doesn't mean that we won't hit these limits, but it
    makes the lifetime of the labels much shorter and eases the debugging of
    these kinds of issues.


### Literal Pool

On ARMv8 instructions are 32 bits long, thus immediate values encoded in the
instructions have limited size. If you want to load a constant bigger than this
limit you have two possibilities:

1. Use multiple instructions to load the constant in multiple steps. This
  solution is already handled in VIXL. For instance you can write:

  `__ Mov(x0, 0x1122334455667788);`

  The previous instruction would not be legal since the immediate value is too
  big. However, VIXL's macro assembler will automatically rewrite this line into
  multiple instructions to efficiently generate the value.


2. Store the constant in memory and load this value from the memory. The value
  needs to be written near the code that will load it since we use a PC-relative
  offset to indicate the address of this value. This solution has the advantage
  of making the value easily modifiable at run-time; since it does not reside
  in the instruction stream, it doesn't require cache maintenance when updated.

  VIXL also provides a way to do this:

  `__ Ldr(x0, 0x1122334455667788);`

  The assembler will store the immediate value in a "literal pool", a set of
  constants embedded in the code. VIXL will emit literal pools after natural
  breaks in the control flow, such as unconditional branches or return
  instructions.

  Literal pools are emitted regularly, such that they are within range of the
  instructions that refer to them. However, you can force a literal pool to be
  emitted using `masm.EmitLiteralPool()`.


Running the code in the simulator.
----------------------------------

Now we are going to see how to use the simulator to run the code that we
generated previously.

Use the simulator to assign a value to the registers. Our previous code example
uses the register x0 as an input, so let's set the value of this register.

    simulator.WriteXRegister(0, 0x8899aabbccddeeff);

Now we can jump to the "entry" label to execute the code:

    simulator.RunFrom(entry.target());

When the execution is finished and the simulator returned, you can inspect
the value of the registers after the execution. For instance:

    printf("x0 = %" PRIx64 "\n", simulator.ReadXRegister(0));

The example shown in this tutorial is very simple, because the goal was to
demonstrate the basics of the VIXL framework. There are more complex code
examples in the VIXL `examples/aarch64` directory showing more features of both the
macro assembler and the ARMv8 architecture.


Extras
------

In addition to this document and the [examples](/examples/aarch64), you can find
documentation and guides on various topics that may be helpful
[here](/doc/aarch64/topics/index.md).
