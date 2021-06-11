Extending the disassembler
==========================

The output of the disassembler can be extended and customized. This may be
useful for example to add comments and annotations to the disassembly, print
aliases for register names, or add an offset to disassembled addresses.

The general procedure to achieve this is to create a sub-class of
`Disassembler` and override the appropriate virtual functions.

The `Disassembler` class provides virtual methods that implement how specific
disassembly elements are printed. See
[src/aarch64/disasm-aarch64.h](/src/aarch64/disasm-aarch64.h) for details.
These include functions like:

    virtual void AppendRegisterNameToOutput(const Instruction* instr,
                                            const CPURegister& reg);
    virtual void AppendPCRelativeOffsetToOutput(const Instruction* instr,
                                                int64_t offset);
    virtual void AppendCodeRelativeAddressToOutput(const Instruction* instr,
                                                   const void* addr);
    virtual void AppendCodeRelativeCodeAddressToOutput(const Instruction* instr,
                                                       const void* addr);

They can be overridden for example to use different register names and annotate
code addresses.

More complex modifications can be performed by overriding the visitor functions
of the disassembler. The VIXL `Decoder` uses a visitor pattern implementation,
so the `Disassembler` (as a sub-class of `DecoderVisitor`) must provide a
visitor function for each sub-type of instructions. The complete list of
visitors is defined by the macro `VISITOR_LIST` in
[src/aarch64/decoder-aarch64.h](/src/aarch64/decoder-aarch64.h).

The [/examples/custom-disassembler.h](/examples/custom-disassembler.h) and
[/examples/custom-disassembler.cc](/examples/custom-disassembler.cc) example
files show how the methods can be overridden to use different register names,
map code addresses, annotate code addresses, and add comments:

    VIXL disasm  0x7fff04cb05e0:  add x10, x16, x17
    custom disasm -0x8: add x10, ip0, ip1 // add/sub to x10

    VIXL disasm  0x7fff04cb05e4:  cbz x10, #+0x28 (addr  0x7fff04cb060c)
    custom disasm -0x4: cbz x10, #+0x28 (addr  0x24 ; label: somewhere)

    VIXL disasm  0x7fff04cb05e8:  add x11, x16, x17
    custom disasm  0x0: add x11, ip0, ip1

    VIXL disasm  0x7fff04cb05ec:  add w5, w6, w30
    custom disasm  0x4: add w5, w6, w30

    VIXL disasm  0x7fff04cb05f0:  tbz w10, #2, #-0x10 (addr  0x7fff04cb05e0)
    custom disasm  0x8: tbz w10, #2, #-0x10 (addr -0x8)


One can refer to the implementation of visitor functions for the `Disassembler`
(in [src/aarch64/disasm-aarch64.cc](/src/aarch64/disasm-aarch64.cc)) or even
for the `Simulator`
(in [src/aarch64/simulator-aarch64.cc](/src/aarch64/simulator-aarch64.cc))
to see how to extract information from instructions.
