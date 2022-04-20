include(CheckSymbolExists)

function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE)
        set(CMAKE_REQUIRED_QUIET YES)
        check_symbol_exists("${symbol}" "" DETECT_ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        if (DETECT_ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
        endif()

        unset(DETECT_ARCHITECTURE_${arch} CACHE)
    endif()
endfunction()

detect_architecture("__ARM64__" arm64)
detect_architecture("__aarch64__" arm64)
detect_architecture("_M_ARM64" arm64)

detect_architecture("__arm__" arm32)
detect_architecture("__TARGET_ARCH_ARM" arm32)
detect_architecture("_M_ARM" arm32)

detect_architecture("__x86_64" x86_64)
detect_architecture("__x86_64__" x86_64)
detect_architecture("__amd64" x86_64)
detect_architecture("_M_X64" x86_64)

detect_architecture("__i386" x86_32)
detect_architecture("__i386__" x86_32)
detect_architecture("_M_IX86" x86_32)

detect_architecture("__ia64" ia64)
detect_architecture("__ia64__" ia64)
detect_architecture("_M_IA64" ia64)

detect_architecture("__mips" mips)
detect_architecture("__mips__" mips)
detect_architecture("_M_MRX000" mips)

detect_architecture("__ppc64__" ppc64)
detect_architecture("__powerpc64__" ppc64)

detect_architecture("__ppc__" ppc32)
detect_architecture("__ppc" ppc32)
detect_architecture("__powerpc__" ppc32)
detect_architecture("_ARCH_COM" ppc32)
detect_architecture("_ARCH_PWR" ppc32)
detect_architecture("_ARCH_PPC" ppc32)
detect_architecture("_M_MPPC" ppc32)
detect_architecture("_M_PPC" ppc32)

detect_architecture("__riscv" riscv)

detect_architecture("__EMSCRIPTEN__" wasm)
