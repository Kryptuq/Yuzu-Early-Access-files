cmake_minimum_required(VERSION 3.8)
project(dynarmic LANGUAGES C CXX ASM VERSION 5)

# Determine if we're built as a subproject (using add_subdirectory)
# or if this is the master project.
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif()

# Dynarmic project options
option(DYNARMIC_ENABLE_CPU_FEATURE_DETECTION "Turning this off causes dynarmic to assume the host CPU doesn't support anything later than SSE3" ON)
option(DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT "Enables support for systems that require W^X" OFF)
option(DYNARMIC_FATAL_ERRORS "Errors are fatal" OFF)
option(DYNARMIC_IGNORE_ASSERTS "Ignore asserts" OFF)
option(DYNARMIC_TESTS "Build tests" ${MASTER_PROJECT})
option(DYNARMIC_TESTS_USE_UNICORN "Enable fuzzing tests against unicorn" OFF)
option(DYNARMIC_USE_LLVM "Support disassembly of jitted x86_64 code using LLVM" OFF)
option(DYNARMIC_WARNINGS_AS_ERRORS "Warnings as errors" ${MASTER_PROJECT})
if (NOT DEFINED DYNARMIC_FRONTENDS)
    set(DYNARMIC_FRONTENDS "A32;A64" CACHE STRING "Selects which frontends to enable")
endif()

# Default to a Release build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message(STATUS "Defaulting to a Release build")
endif()

# Set hard requirements for C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    set(DYNARMIC_CXX_FLAGS
        /std:c++latest # CMAKE_CXX_STANDARD has no effect on MSVC until CMake 3.10.
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

    if (DYNARMIC_WARNINGS_AS_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
             /WX)
    endif()

    if (CMAKE_VS_PLATFORM_TOOLSET MATCHES "LLVM-vs[0-9]+")
        list(APPEND DYNARMIC_CXX_FLAGS
             -Qunused-arguments
             -Wno-missing-braces)
    endif()
else()
    set(DYNARMIC_CXX_FLAGS
        -Wall
        -Wextra
        -Wcast-qual
        -pedantic
        -pedantic-errors
        -Wno-missing-braces)

    if (DYNARMIC_WARNINGS_AS_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
             -Werror)
    endif()

    if (DYNARMIC_FATAL_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
             -Wfatal-errors)
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
        # Bracket depth determines maximum size of a fold expression in Clang since 9c9974c3ccb6.
        # And this in turns limits the size of a std::array.
        list(APPEND DYNARMIC_CXX_FLAGS -fbracket-depth=1024)
    endif()
endif()

# Arch detection
include(DetectArchitecture)
if (NOT DEFINED ARCHITECTURE)
    message(FATAL_ERROR "Unsupported architecture encountered. Ending CMake generation.")
endif()
message(STATUS "Target architecture: ${ARCHITECTURE}")

# Include Boost
if (NOT TARGET boost)
    if (NOT Boost_INCLUDE_DIRS)
        find_package(Boost 1.57.0 REQUIRED)
    endif()
    add_library(boost INTERFACE)
    target_include_directories(boost SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
endif()

if (DYNARMIC_NO_BUNDLED_FMT AND NOT TARGET fmt AND NOT TARGET fmt::fmt)
    find_package(fmt REQUIRED)
    add_library(fmt ALIAS fmt::fmt)
endif()

if (DYNARMIC_NO_BUNDLED_ROBIN_MAP AND NOT TARGET tsl::robin_map)
    find_package(tsl-robin-map REQUIRED)
endif()

if (DYNARMIC_NO_BUNDLED_XBYAK AND NOT TARGET xbyak)
    if (ARCHITECTURE STREQUAL "x86" OR ARCHITECTURE STREQUAL "x86_64")
        find_package(xbyak REQUIRED)
        add_library(xbyak ALIAS xbyak::xbyak)
    endif()
endif()

if (DYNARMIC_NO_BUNDLED_ZYDIS AND NOT TARGET Zydis)
    find_package(Zydis REQUIRED)
    add_library(Zydis ALIAS Zydis::Zydis)
endif()

# Enable unit-testing.
enable_testing(true)

# Include LLVM
if (DYNARMIC_USE_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(-DDYNARMIC_USE_LLVM ${LLVM_DEFINITIONS})
    llvm_map_components_to_libnames(llvm_libs armdesc armdisassembler aarch64desc aarch64disassembler x86desc x86disassembler)
endif()

if (DYNARMIC_TESTS_USE_UNICORN)
    find_package(Unicorn REQUIRED)
endif()

if (DYNARMIC_TESTS AND DYNARMIC_NO_BUNDLED_CATCH AND NOT TARGET catch)
    find_package(Catch2 REQUIRED)
    add_library(catch ALIAS Catch2::Catch2)
endif()

# Pull in externals CMakeLists for libs where available
add_subdirectory(externals)

# Dynarmic project files
add_subdirectory(src/dynarmic)
if (DYNARMIC_TESTS)
    add_subdirectory(tests)
endif()

#
# Install
#
if (MASTER_PROJECT)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS dynarmic EXPORT dynarmicTargets)
    install(EXPORT dynarmicTargets
        NAMESPACE dynarmic::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dynarmic"
    )

    configure_package_config_file(CMakeModules/dynarmicConfig.cmake.in
        dynarmicConfig.cmake
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dynarmic"
    )
    write_basic_package_version_file(dynarmicConfigVersion.cmake
        COMPATIBILITY SameMajorVersion
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/dynarmicConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/dynarmicConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dynarmic"
    )

    install(DIRECTORY src/dynarmic TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
endif()
