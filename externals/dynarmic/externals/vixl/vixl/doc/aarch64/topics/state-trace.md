AArch64 Simulator state trace
=============================

The AArch64 Simulator can be configured to produce traces of instruction
execution, register contents, and memory accesses. The trace is designed to be
intuitive for human readers, but this document describes the format of the
trace, so that post-processing tools can confidently parse the output.

In VIXL's own test runner, the trace is controlled by the `--trace*` options.
Run `test-runner --help` for details.

Basic structure
---------------

Executed instructions show the address, the encoding of the instruction and the
disassembly (as produced by VIXL's Disassembler). For example:

    0x00007fbe2a6a9044  d299d200		mov x0, #0xce90

The first field is the address of the instruction, with exactly 16 hexadecimal
characters and a leading 0x, and is followed by two spaces. The second field is
the instruction encoding, with exactly eight hexadecimal characters (and no
leading 0x). This is followed by two _tab_ characters, and the instruction
disassembly. The following regular expression can be used to capture each field:

    (0x[0-9a-f]{16})  ([0-9a-f]{8})\t\t(.*)

Following each instruction are zero or more lines of state update. Most notably,
these represent the register state updates and memory accesses that occurred
during simulation of the instruction. All of these lines begin with a '#'
character, so that they can be easily identified, and filtered if necessary. For
example:

    0x00007fd2221c907c  8b82200e		add x14, x0, x2, asr #8
    #            x14: 0xfffedcba98765432
    0x00007fd2221c9080  0b81200f		add w15, w0, w1, asr #8
    #            w15:         0xff89abcd

Note that the Simulator uses these state update lines to describe its initial
state. As a result, there will be state trace output before the first simulated
instruction, and parsers need to be tolerant of this.

Note that padding white space is used liberally to keep values vertically
aligned throughout the trace (as shown with the write to `w15` in the example
above). Similarly, some compound values are split into parts using the C++14
literal separator (`'`) character. Refer to the "Memory accesses" section
(below) for examples.

Ordering
--------

VIXL guarantees that each instruction is printed before its associated state
trace.

State trace must be interpreted sequentially, line by line. VIXL avoids updating
the same register more than once (because it makes the trace hard for humans to
read), but this can occur in some situations, and should be supported by
parsers.

The state is intended to be consistent with architectural execution at the start
of each instruction and at the end of the whole trace, but no such guarantees
are made about the traced state _between_ instructions. VIXL prioritises
human-readability when choosing the ordering of state updates.

If simulated registers are modified externally, for example using
`WriteRegister` from C++ code, their state will (by default) be logged
immediately. In the full trace, it will appear as though the (runtime) call or
return instruction modified the state. This is consistent with the guarantees
above, but it can result in single instructions appearing to generate a large
number of state updates.

There is no upper limit on the number of state update lines that any one
instruction can generate.

Whole register trace
--------------------

The simplest form of state trace has the form "`REG: VALUE`", meaning that
the register `REG` has the specified value, and any high-order bits in aliased
registers are set to zero.

    0x00007fd2221c907c  8b82200e		add x14, x0, x2, asr #8
    #            x14: 0xfffedcba98765432

Note that to correctly track state, parsers need to be aware of architectural
register aliasing rules. Also, VIXL uses some standard register aliases, such as
`lr` (`x30`). To avoid misinterpreting a register alias (and thereby potentially
missing an aliased register update), some tools may need to treat an
unrecognised register name as an error.

This trace format attempts to represent _architectural_ register writes.
However, this is not strictly checked or enforced.

`VALUE` is always shown in hexadecimal (raw bits) form, with a leading `0x` and
enough digits to exactly fill `REG`. `VALUE` may also include annotations (for
example to show FP arithmetic values) in parentheses. These annotations are for
the benefit of human readers, and parsers may ignore them.

Note that SVE registers _always_ use the partial register trace format,
described below, so a plain `z` or `p` register will never be used in a whole
register trace. This is true even if the vector length is configured to 16
bytes.

Partial register trace
----------------------

Sometimes, VIXL needs to show _part_ of a register without implying that the
rest of the register is zeroed. A partial register value is indicated by a bit
range in angled brackets after the register name: "`REG<MSB:LSB>: VALUE`".
This format is used for stores, for example.

SVE register updates are split across multiple lines, and therefore always use
the partial register trace format. For example (with a 384-bit VL):

    0x00007fb1978da044  04214000		index z0.b, #0, #1
    #   z0<383:256>: 0x2f2e2d2c2b2a29282726252423222120
    #   z0<255:128>: 0x1f1e1d1c1b1a19181716151413121110
    #     z0<127:0>: 0x0f0e0d0c0b0a09080706050403020100

Note that VIXL will omit whole lines where they are unnecessary, for example if
they have no active (predicated) lanes. Parsers should not assume that every
part of a register will appear in such cases.

The `VALUE` has the same format as in the whole register trace, except in the
case of SVE `p` registers (as described below).

SVE `p` registers
-----------------

For `p` registers, we try to keep the lanes vertically aligned with the
corresponding parts of the `z` registers that they affect. To do this, we use a
binary format, with a leading `0b`, and spaces between each digit. For example:

    0x00007f66e539b0b8  04f54607		index z7.d, x16, #-11
    #     z7<127:0>: 0x00000000000000150000000000000020
    0x00007f66e539b0bc  25d8e3a7		ptrue p7.d, all
    #      p7<15:0>: 0b 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1

Memory accesses
---------------

The pattern for a memory access is "`VALUE OP ADDRESS`", where:

- `VALUE` is a hexadecimal value, with visual separators (') between
  structure components,
- `OP` is `"->"` for a store, or `"<-"` for a load,
- `ADDRESS` is the (hexadecimal) address of the access.

Accesses shown in this style are always contiguous, and with little-endian
semantics. However, a given instruction might have multiple lines of memory
access trace, particularly if the instruction performs non-contiguous accesses.

In the case of simple accesses, the `VALUE` is shared with register value trace:

    0x00007f3835372058  e400e401		st1b { z1.b }, p1, [x0]
    #      z1<127:0>: 0xd4d7dadde0e3e6e9eceff2f5f8fbfe01 -> 0x000055d170298e90

Sign-extending loads show the whole resulting register value, with the (smaller)
access represented on a separate line. This makes the (differing) values in the
register and in memory unambiguous, without parsers needing to understand the
instruction set:

    0x00007f47922d0068  79800306		ldrsh x6, [x24]
    #             x6: 0xffffffffffff8080
    #                                  ╙─ 0x8080 <- 0x00007fffbc197708

Some instructions access several different memory locations. In these cases,
each access is given its own line, with the highest lane index first so that
(for contiguous accesses) the lowest address ends up at the bottom:

    0x00007fa6001e9060  e4217c0a		st2b { z10.b, z11.b }, p7, [x0, x1]
    #     z10<127:0>: 0x0f0e0d0c0b0a09080706050403020100
    #     z11<127:0>: 0x1f1e1d1c1b1a19181716151413121110
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙─ 0x10'00 -> 0x00007ffe485d2f90
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙─── 0x11'01 -> 0x00007ffe485d2f92
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙───── 0x12'02 -> 0x00007ffe485d2f94
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙─────── 0x13'03 -> 0x00007ffe485d2f96
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙───────── 0x14'04 -> 0x00007ffe485d2f98
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙─────────── 0x15'05 -> 0x00007ffe485d2f9a
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ║ ╙───────────── 0x16'06 -> 0x00007ffe485d2f9c
    #                    ║ ║ ║ ║ ║ ║ ║ ║ ╙─────────────── 0x17'07 -> 0x00007ffe485d2f9e
    #                    ║ ║ ║ ║ ║ ║ ║ ╙───────────────── 0x18'08 -> 0x00007ffe485d2fa0
    #                    ║ ║ ║ ║ ║ ║ ╙─────────────────── 0x19'09 -> 0x00007ffe485d2fa2
    #                    ║ ║ ║ ║ ║ ╙───────────────────── 0x1a'0a -> 0x00007ffe485d2fa4
    #                    ║ ║ ║ ║ ╙─────────────────────── 0x1b'0b -> 0x00007ffe485d2fa6
    #                    ║ ║ ║ ╙───────────────────────── 0x1c'0c -> 0x00007ffe485d2fa8
    #                    ║ ║ ╙─────────────────────────── 0x1d'0d -> 0x00007ffe485d2faa
    #                    ║ ╙───────────────────────────── 0x1e'0e -> 0x00007ffe485d2fac
    #                    ╙─────────────────────────────── 0x1f'0f -> 0x00007ffe485d2fae

The line-drawing characters are encoded as UTF-8 (as is this document). There is
currently no locale handling in VIXL, so this is not configurable. However,
since these annotations are for the benefit of human readers, parsers can safely
ignore them, and treat the whole trace as an ASCII byte stream (ignoring 8-bit
characters). This is useful in situations where UTF-8 handling carries an
unacceptable performance cost.

In the future, VIXL may offer an option to avoid printing these annotations, so
that the trace is restricted to single-byte characters.

Floating-point value annotations
--------------------------------

Some floating-point operations produce register trace that annotates the raw
values with the corresponding FP arithmetic values. This is for the benefit of
human readers (and has limited precision). Such annotations follow the `VALUE`
in parentheses.

Scalar form:

    #             s1:                         0x3f800000 (1.000) <- 0x00007ffdc64d2314

Vector form, updating all S lanes using a load:

    #            v16: 0x1211100f0e0d0c0b0a09080706050403 (4.577e-28, 1.739e-30, 6.598e-33, 2.502e-35)
    #                          ║       ║       ║       ╙─ 0x06050403 <- 0x00007ffe56fd7863
    #                          ║       ║       ╙───────── 0x0a090807 <- 0x00007ffe56fd7867
    #                          ║       ╙───────────────── 0x0e0d0c0b <- 0x00007ffe56fd786b
    #                          ╙───────────────────────── 0x1211100f <- 0x00007ffe56fd786f

Vector form, updating a single S lane using a load:

    #             v2: 0x03020100040302017ff0f0027f80f000 (..., 1.540e-36, ...)
    #                                  ╙───────────────── 0x04030201 <- 0x00007ffc7b2e3ca1

Vector form, replicating a single struct load to all S lanes:

    #            v15: 0x100f0e0d100f0e0d100f0e0d100f0e0d (2.821e-29, 2.821e-29, 2.821e-29, 2.821e-29)
    #            v16: 0x14131211141312111413121114131211 (7.425e-27, 7.425e-27, 7.425e-27, 7.425e-27)
    #            v17: 0x18171615181716151817161518171615 (1.953e-24, 1.953e-24, 1.953e-24, 1.953e-24)
    #                          ╙───────╨───────╨───────╨─ 0x18171615'14131211'100f0e0d <- 0x00007ffdd64d847d
