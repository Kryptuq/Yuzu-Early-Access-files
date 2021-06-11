VIXL Change Log
===============

* 1.13
    + Improve code formatting and add tests using clang-format.
    + Fix bugs in disassembly of unallocated instruction encodings.
    + Fix some execution trace bugs, and add tests.
    + Other small bug fixes and improvements.

* 1.12
    + Bug fixes for toolchain compatibility.

* 1.11
    + Fix bug in simulation of add with carry.
    + Fix use-after-free bug in Literal handling.
    + Build system updates for Android.
    + Add option to run test.py under Valgrind.
    + Other small bug fixes and improvements.

* 1.10
    + Improved support for externally managed literals.
    + Reworked build and test infrastructure.
    + Other small bug fixes and improvements.

* 1.9
    + Improved compatibility with Android build system.
    + Improved compatibility with Clang toolchain.
    + Added support for `umulh` instruction.
    + Added support for `fcmpe` and `fccmpe` instructions.
    + Other small bug fixes and improvements.

* 1.8
    + Complete NEON instruction set support.
    + Support long branches using veneers.
    + Improved handling of literal pools.
    + Support some `ic` and `dc` cache op instructions.
    + Support CRC32 instructions.
    + Support half-precision floating point instructions.
    + MacroAssembler support for `bfm`, `ubfm` and `sbfm`.
    + Other small bug fixes and improvements.

* 1.7
    + Added support for `prfm` prefetch instructions.
    + Added support for all `frint` instruction variants.
    + Add support for disassembling as an offset from a given address.
    + Fixed the disassembly of `movz` and `movn`.
    + Provide static helpers for immediate generation.
    + Provide helpers to create CPURegList from list unions or intersections.
    + Improved register value tracing.
    + Multithreading test fixes.
    + Other small bug fixes and build system improvements.

* 1.6
    + Make literal pool management the responsibility of the macro assembler.
    + Move code buffer management out of the Assembler.
    + Support `ldrsw` for literals.
    + Support binding a label to a specific offset.
    + Add macro assembler support for load/store pair with arbitrary offset.
    + Support Peek and Poke for CPURegLists.
    + Fix disassembly of branch targets.
    + Fix Decoder visitor insertion order.
    + Separate Decoder visitors into const and non-const variants.
    + Fix simulator for branches to tagged addresses.
    + Add a VIM YouCompleteMe configuration file.
    + Other small bug fixes and build system improvements.

* 1.5
    + Tagged pointer support.
    + Implement support for exclusive access instructions.
    + Implement support for `adrp` instruction.
    + Faster code for logical immediate identification.
    + Generate better code for immediates passed to shift-capable instructions.
    + Allow explicit use of unscaled-offset loads and stores.
    + Build and test infrastructure improvements.
    + Corrected computation of cache line size.
    + Fix simulation of `extr` instruction.
    + Fixed a bug when moving kWMinInt to a register.
    + Other small bug fixes.

* 1.4
    + Added support for `frintm`.
    + Fixed simulation of `frintn` and `frinta` for corner cases.
    + Added more tests for floating point instruction simulation.
    + Modified `CalleeSave()` and `CalleeRestore()` to push general purpose
      registers before floating point registers on the stack.
    + Fixed Printf for mixed argument types, and use on real hardware.
    + Improved compatibility with some 32-bit compilers.

* 1.3
    + Address inaccuracies in the simulated floating point instructions.
    + Implement Default-NaN floating point mode.
    + Introduce `UseScratchRegisterScope` for controlling the use of temporary
      registers.
    + Enable building VIXL on 32-bit hosts.
    + Other small bug fixes and improvements.

* 1.2
    + Added support for `fmadd`, `fnmadd`, `fnmsub`, `fminnm`, `fmaxnm`,
      `frinta`, `fcvtau` and `fcvtas`.
    + Added support for assembling and disassembling `isb`, `dsb` and `dmb`.
    + Added support for automatic inversion of compare instructions when using
      negative immediates.
    + Added support for using `movn` when generating immediates.
    + Added explicit flag-setting 'S' instructions, and removed
      `SetFlags` and `LeaveFlags` arguments.
    + Added support for `Movk` in macro assembler.
    + Added support for W register parameters to `Tbz` and `Tbnz`.
    + Added support for using immediate operands with `Csel`.
    + Added new debugger syntax for memory inspection.
    + Fixed `smull`, `fmsub` and `sdiv` simulation.
    + Fixed sign extension for W->X conversions using `sxtb`, `sxth` and `sxtw`.
    + Prevented code generation for certain side-effect free operations,
      such as `add r, r, #0`, in the macro assembler.
    + Other small bug fixes.

* 1.1
    + Improved robustness of instruction decoder and disassembler.
    + Added support for double-to-float conversions using `fcvt`.
    + Added support for more fixed-point to floating-point conversions (`ucvtf`
      and `scvtf`).
    + Added instruction statistics collection class `instrument-a64.cc`.

* 1.0
    + Initial release.
