// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#if defined(__ARM64__) || defined(__aarch64__) || defined(_M_ARM64)
#    define MCL_ARCHITECTURE arm64
#    define MCL_ARCHITECTURE_ARM64 1
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM)
#    define MCL_ARCHITECTURE arm32
#    define MCL_ARCHITECTURE_ARM32 1
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#    define MCL_ARCHITECTURE x86_64
#    define MCL_ARCHITECTURE_X86_64 1
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#    define MCL_ARCHITECTURE x86_32
#    define MCL_ARCHITECTURE_X86_32 1
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#    define MCL_ARCHITECTURE ia64
#    define MCL_ARCHITECTURE_IA64 1
#elif defined(__mips) || defined(__mips__) || defined(_M_MRX000)
#    define MCL_ARCHITECTURE mips
#    define MCL_ARCHITECTURE_MIPS 1
#elif defined(__ppc64__) || defined(__powerpc64__)
#    define MCL_ARCHITECTURE ppc64
#    define MCL_ARCHITECTURE_PPC64 1
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC) || defined(_M_MPPC) || defined(_M_PPC)
#    define MCL_ARCHITECTURE ppc32
#    define MCL_ARCHITECTURE_PPC32 1
#elif defined(__riscv)
#    define MCL_ARCHITECTURE riscv
#    define MCL_ARCHITECTURE_RISCV 1
#elif defined(__EMSCRIPTEN__)
#    define MCL_ARCHITECTURE wasm
#    define MCL_ARCHITECTURE_WASM 1
#else
#    define MCL_ARCHITECTURE generic
#    define MCL_ARCHITECTURE_GENERIC 1
#endif
