# SPDX-FileCopyrightText: 2023 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(LLVM QUIET CONFIG)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM CONFIG_MODE)

if (LLVM_FOUND AND NOT TARGET LLVM::Demangle)
    add_library(LLVM::Demangle INTERFACE IMPORTED)
    llvm_map_components_to_libnames(LLVM_LIBRARIES demangle)
    target_compile_definitions(LLVM::Demangle INTERFACE ${LLVM_DEFINITIONS})
    target_include_directories(LLVM::Demangle INTERFACE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(LLVM::Demangle INTERFACE ${LLVM_LIBRARIES})
endif()
