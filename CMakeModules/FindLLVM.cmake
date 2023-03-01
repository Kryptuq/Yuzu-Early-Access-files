# SPDX-FileCopyrightText: 2023 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(LLVM QUIET COMPONENTS CONFIG)
if (LLVM_FOUND)
    separate_arguments(LLVM_DEFINITIONS)
    if (LLVMDemangle IN_LIST LLVM_AVAILABLE_LIBS)
        set(LLVM_Demangle_FOUND TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM HANDLE_COMPONENTS CONFIG_MODE)

if (LLVM_FOUND AND LLVM_Demangle_FOUND AND NOT TARGET LLVM::Demangle)
    add_library(LLVM::Demangle INTERFACE IMPORTED)
    target_compile_definitions(LLVM::Demangle INTERFACE ${LLVM_DEFINITIONS})
    target_include_directories(LLVM::Demangle INTERFACE ${LLVM_INCLUDE_DIRS})
    # prefer shared LLVM: https://github.com/llvm/llvm-project/issues/34593
    # but use ugly hack because llvm_config doesn't support interface library
    add_library(_dummy_lib SHARED EXCLUDE_FROM_ALL src/yuzu/main.cpp)
    llvm_config(_dummy_lib USE_SHARED demangle)
    get_target_property(LLVM_LIBRARIES _dummy_lib LINK_LIBRARIES)
    target_link_libraries(LLVM::Demangle INTERFACE ${LLVM_LIBRARIES})
endif()
