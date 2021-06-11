add_library(vixl
    vixl/src/aarch64/abi-aarch64.h
    vixl/src/aarch64/assembler-aarch64.cc
    vixl/src/aarch64/assembler-aarch64.h
    vixl/src/aarch64/constants-aarch64.h
    vixl/src/aarch64/cpu-aarch64.cc
    vixl/src/aarch64/cpu-aarch64.h
    vixl/src/aarch64/cpu-features-auditor-aarch64.cc
    vixl/src/aarch64/cpu-features-auditor-aarch64.h
    vixl/src/aarch64/decoder-aarch64.cc
    vixl/src/aarch64/decoder-aarch64.h
    vixl/src/aarch64/decoder-constants-aarch64.h
    vixl/src/aarch64/disasm-aarch64.cc
    vixl/src/aarch64/disasm-aarch64.h
    vixl/src/aarch64/instructions-aarch64.cc
    vixl/src/aarch64/instructions-aarch64.h
    vixl/src/aarch64/logic-aarch64.cc
    vixl/src/aarch64/macro-assembler-aarch64.cc
    vixl/src/aarch64/macro-assembler-aarch64.h
    vixl/src/aarch64/operands-aarch64.cc
    vixl/src/aarch64/operands-aarch64.h
    vixl/src/aarch64/pointer-auth-aarch64.cc
    vixl/src/aarch64/simulator-aarch64.cc
    vixl/src/aarch64/simulator-aarch64.h
    vixl/src/aarch64/simulator-constants-aarch64.h
    vixl/src/assembler-base-vixl.h
    vixl/src/code-buffer-vixl.cc
    vixl/src/code-buffer-vixl.h
    vixl/src/code-generation-scopes-vixl.h
    vixl/src/compiler-intrinsics-vixl.cc
    vixl/src/compiler-intrinsics-vixl.h
    vixl/src/cpu-features.cc
    vixl/src/cpu-features.h
    vixl/src/globals-vixl.h
    vixl/src/invalset-vixl.h
    vixl/src/macro-assembler-interface.h
    vixl/src/platform-vixl.h
    vixl/src/pool-manager-impl.h
    vixl/src/pool-manager.h
    vixl/src/utils-vixl.cc
    vixl/src/utils-vixl.h
)
target_include_directories(vixl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vixl/src)
target_compile_definitions(vixl PUBLIC VIXL_INCLUDE_TARGET_AARCH64=1)
if (MSVC)
    target_compile_options(vixl PUBLIC /Zc:__cplusplus)
endif()
if (UNIX AND NOT APPLE)
    target_compile_definitions(vixl PUBLIC VIXL_CODE_BUFFER_MMAP=1)
else()
    target_compile_definitions(vixl PUBLIC VIXL_CODE_BUFFER_MALLOC=1)
endif()
