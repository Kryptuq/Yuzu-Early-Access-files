cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
include(GNUInstallDirs)

project(mcl LANGUAGES CXX VERSION 0.1.8)

# Project options
option(MCL_WARNINGS_AS_ERRORS "Warnings as errors" ON)

# Default to a Release build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message(STATUS "Defaulting to a Release build")
endif()

# Set hard requirements for C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warn on CMake API deprecations
set(CMAKE_WARN_DEPRECATED ON)

# Disable in-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(SEND_ERROR "In-source builds are not allowed.")
endif()

# Add the module directory to the list of paths
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

# Compiler flags
if (MSVC)
    set(MCL_CXX_FLAGS
        /std:c++latest
        /experimental:external
        /external:W0
        /external:anglebrackets
        /W4
        /w44263 # Non-virtual member function hides base class virtual function
        /w44265 # Class has virtual functions, but destructor is not virtual
        /w44456 # Declaration of 'var' hides previous local declaration
        /w44457 # Declaration of 'var' hides function parameter
        /w44458 # Declaration of 'var' hides class member
        /w44459 # Declaration of 'var' hides global definition
        /w44946 # Reinterpret-cast between related types
        /wd4592 # Symbol will be dynamically initialized (implementation limitation)
        /permissive- # Stricter C++ standards conformance
        /MP
        /Zi
        /Zo
        /EHsc
        /Zc:externConstexpr # Allows external linkage for variables declared "extern constexpr", as the standard permits.
        /Zc:inline          # Omits inline functions from object-file output.
        /Zc:throwingNew     # Assumes new (without std::nothrow) never returns null.
        /volatile:iso       # Use strict standard-abiding volatile semantics
        /bigobj             # Increase number of sections in .obj files
        /DNOMINMAX)

    if (MCL_WARNINGS_AS_ERRORS)
        list(APPEND MCL_CXX_FLAGS /WX)
    endif()

    if (CMAKE_VS_PLATFORM_TOOLSET MATCHES "LLVM-vs[0-9]+")
        list(APPEND MCL_CXX_FLAGS
             -Qunused-arguments
             -Wno-missing-braces)
    endif()
else()
    set(MCL_CXX_FLAGS
        -Wall
        -Wextra
        -Wcast-qual
        -pedantic
        -pedantic-errors
        -Wfatal-errors
        -Wno-missing-braces)

    if (MCL_WARNINGS_AS_ERRORS)
        list(APPEND MCL_CXX_FLAGS -Werror)
    endif()
endif()

# Dependencies

if (NOT TARGET Catch2::Catch2)
    find_package(Catch2 QUIET)
endif()

if (NOT TARGET fmt::fmt)
    find_package(fmt REQUIRED)
endif()

# Project files

add_subdirectory(src)
if (TARGET Catch2::Catch2)
    add_subdirectory(tests)
endif()

# Install instructions

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS mcl EXPORT mclTargets)
install(EXPORT mclTargets
    NAMESPACE merry::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcl"
)

configure_package_config_file(CMakeModules/mclConfig.cmake.in
    mclConfig.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcl"
)
write_basic_package_version_file(mclConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mclConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mclConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mcl"
)

install(DIRECTORY include/ TYPE INCLUDE FILES_MATCHING PATTERN "*.hpp")
